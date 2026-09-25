#include "right_of_way.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

// M3.2.2c: the crossing-coverage measurement (docs/M3_CONTRACT.md §1). Two lane surfaces are
// intersected quad by quad and the overlap is read back as stations, so a crossing area's two
// stored numbers can be checked against the geometry instead of trusted.
namespace trafficsim {
namespace {
// Named numerical constants, not calibration (contract §1).
constexpr double kMinOverlapArea = 1e-6; // m²: two surfaces closer than this only touch
constexpr double kJoinGap = 1e-6;        // m: overlaps nearer than this along a path are one
constexpr double kStraight = 1e-12;      // relative: a turn this small is no turn

Point sub(Point a, Point b) { return {a.x - b.x, a.y - b.y}; }
double cross(Point a, Point b) { return a.x * b.y - a.y * b.x; }
double area(const std::vector<Point>& p) {
    double sum = 0;
    for (std::size_t i = 0; i < p.size(); ++i) sum += cross(p[i], p[(i + 1) % p.size()]);
    return sum / 2;
}
// A lane surface: the two boundaries that bound it, point for point with the polyline its
// stations are authored on.
struct Strip { std::vector<Point> base, left, right; };
std::optional<Strip> stripOf(const Network& n, const ControlPathRef& ref) {
    try {
        if (!ref.linkId.empty() && ref.connectorId.empty()) {
            for (const auto& link : n.links) {
                if (link.id != ref.linkId) continue;
                for (std::size_t k = 0; k < link.lanes.size(); ++k)
                    if (link.lanes[k].id == ref.laneId)
                        return Strip{link.geometry, laneBoundaryGeometry(link, k, n.drivingSide),
                                     laneBoundaryGeometry(link, k + 1, n.drivingSide)};
            }
            return std::nullopt;
        }
        if (!ref.linkId.empty() || ref.connectorId.empty()) return std::nullopt;
        for (const auto& c : n.connectors) {
            if (c.id != ref.connectorId) continue;
            // Path i runs between boundaries i and i+1 -- connectorBoundaries builds them from the
            // same connectorPaths list. Exactly one path, never one picked by ordinal.
            const auto paths = connectorPaths(n, c);
            std::optional<std::size_t> match;
            for (std::size_t i = 0; i < paths.size(); ++i)
                if (paths[i].from.laneId == ref.fromLaneId && paths[i].to.laneId == ref.toLaneId) {
                    if (match) return std::nullopt;
                    match = i;
                }
            if (!match) return std::nullopt;
            const auto boundaries = connectorBoundaries(n, c);
            if (boundaries.size() < *match + 2) return Strip{c.geometry, {}, {}}; // reported unsupported
            return Strip{c.geometry, boundaries[*match], boundaries[*match + 1]};
        }
    } catch (const std::exception&) {}
    return std::nullopt;
}
// One quad of a strip, counter-clockwise for clipping, plus what maps a point in it to a station.
struct Quad { std::vector<Point> ccw; Point l0, l1, r0, r1; double start{}, length{}; Point lo, hi; };
// Empty when the strip folds over itself -- a quad that is not convex has no single station for
// a point in it, and guessing one is what the contract forbids.
std::optional<std::vector<Quad>> quadsOf(const Strip& s) {
    if (s.base.size() < 2 || s.left.size() != s.base.size() || s.right.size() != s.base.size()) return std::nullopt;
    std::vector<Quad> quads;
    double start = 0;
    for (std::size_t j = 0; j + 1 < s.base.size(); ++j) {
        const double length = std::hypot(s.base[j + 1].x - s.base[j].x, s.base[j + 1].y - s.base[j].y);
        Quad q{{}, s.left[j], s.left[j + 1], s.right[j], s.right[j + 1], start, length, {}, {}};
        start += length;
        for (const auto& p : {q.l0, q.l1, q.r1, q.r0})
            if (q.ccw.empty() || std::hypot(p.x - q.ccw.back().x, p.y - q.ccw.back().y) > 1e-12) q.ccw.push_back(p);
        if (q.ccw.size() > 1 && std::hypot(q.ccw.front().x - q.ccw.back().x, q.ccw.front().y - q.ccw.back().y) <= 1e-12)
            q.ccw.pop_back();
        const double a = q.ccw.size() < 3 ? 0 : area(q.ccw);
        if (std::abs(a) < kMinOverlapArea || length <= 0) continue; // no surface here
        if (a < 0) std::reverse(q.ccw.begin(), q.ccw.end());
        for (std::size_t i = 0; i < q.ccw.size(); ++i) {
            const auto& p = q.ccw[i]; const auto& r = q.ccw[(i + 1) % q.ccw.size()]; const auto& t = q.ccw[(i + 2) % q.ccw.size()];
            const auto u = sub(r, p), v = sub(t, r);
            if (cross(u, v) < -kStraight * std::hypot(u.x, u.y) * std::hypot(v.x, v.y)) return std::nullopt;
        }
        q.lo = q.hi = q.ccw.front();
        for (const auto& p : q.ccw) {
            q.lo = {std::min(q.lo.x, p.x), std::min(q.lo.y, p.y)};
            q.hi = {std::max(q.hi.x, p.x), std::max(q.hi.y, p.y)};
        }
        quads.push_back(std::move(q));
    }
    return quads;
}
// Sutherland-Hodgman: a convex polygon clipped by a convex counter-clockwise one.
std::vector<Point> clip(std::vector<Point> subject, const std::vector<Point>& by) {
    for (std::size_t e = 0; e < by.size() && !subject.empty(); ++e) {
        const auto a = by[e], edge = sub(by[(e + 1) % by.size()], a);
        const auto input = std::move(subject);
        subject.clear();
        for (std::size_t i = 0; i < input.size(); ++i) {
            const auto p = input[i], q = input[(i + 1) % input.size()];
            const double sp = cross(edge, sub(p, a)), sq = cross(edge, sub(q, a));
            if (sp >= 0) subject.push_back(p);
            if ((sp >= 0) != (sq >= 0)) {
                const double t = sp / (sp - sq);
                subject.push_back({p.x + t * (q.x - p.x), p.y + t * (q.y - p.y)});
            }
        }
    }
    return subject;
}
// The station of a point inside a quad: the t whose cross-section, from lerp(l0,l1,t) to
// lerp(r0,r1,t), passes through it. The same proportional convention matchedStation uses.
double stationIn(const Quad& q, Point p) {
    const auto e = sub(q.l1, q.l0), g = sub(q.r0, q.l0), h = sub(p, q.l0), k = sub(sub(q.r1, q.r0), e);
    const double a = -cross(k, e), b = cross(k, h) - cross(g, e), c = cross(g, h);
    double t;
    if (std::abs(a) <= kStraight * (std::abs(b) + std::abs(c)) || a == 0) t = b == 0 ? 0 : -c / b;
    else {
        const double root = std::sqrt(std::max(0.0, b * b - 4 * a * c));
        const double t1 = (-b + root) / (2 * a), t2 = (-b - root) / (2 * a);
        const auto off = [](double x) { return x < 0 ? -x : x > 1 ? x - 1 : 0; };
        t = off(t1) <= off(t2) ? t1 : t2;
    }
    return q.start + std::clamp(t, 0.0, 1.0) * q.length;
}
// The overlap along one path as one interval, or empty when it comes in pieces.
std::optional<StationInterval> joined(std::vector<StationInterval> pieces) {
    std::sort(pieces.begin(), pieces.end(), [](const auto& a, const auto& b) { return a.from < b.from; });
    StationInterval all = pieces.front();
    for (const auto& p : pieces) {
        if (p.from > all.to + kJoinGap) return std::nullopt;
        all.to = std::max(all.to, p.to);
    }
    return all;
}
}
std::vector<Point> conflictSideOutline(const Network& n, const ConflictSide& side) {
    try {
        const auto strip = stripOf(n, side.path);
        if (!strip || strip->left.size() != strip->base.size() || strip->right.size() != strip->base.size()) return {};
        const auto span = [&](const std::vector<Point>& edge) {
            return polylineSpan(edge, matchedStation(strip->base, edge, side.entryStation),
                                matchedStation(strip->base, edge, side.exitStation));
        };
        auto outline = span(strip->left);
        const auto right = span(strip->right);
        outline.insert(outline.end(), right.rbegin(), right.rend());
        return outline;
    } catch (const std::exception&) { return {}; }
}
std::optional<std::pair<Point, Point>> waitingLineBar(const Network& n, const ControlPoint& point) {
    try {
        const auto strip = stripOf(n, point.path);
        if (!strip || strip->left.size() != strip->base.size() || strip->right.size() != strip->base.size()) return std::nullopt;
        if (point.station < 0 || point.station > polylineLength(strip->base)) return std::nullopt;
        return std::pair{pointAlong(strip->left, matchedStation(strip->base, strip->left, point.station)),
                         pointAlong(strip->right, matchedStation(strip->base, strip->right, point.station))};
    } catch (const std::exception&) { return std::nullopt; }
}
SurfaceOverlap surfaceOverlap(const Network& n, const ControlPathRef& first, const ControlPathRef& second) {
    SurfaceOverlap result;
    const auto a = stripOf(n, first), b = stripOf(n, second);
    if (!a || !b) return result;
    result.status = SurfaceOverlap::Status::unsupported;
    try {
        const auto qa = quadsOf(*a), qb = quadsOf(*b);
        if (!qa || !qb) return result;
        std::vector<StationInterval> onA, onB;
        for (const auto& x : *qa)
            for (const auto& y : *qb) {
                if (x.hi.x < y.lo.x || y.hi.x < x.lo.x || x.hi.y < y.lo.y || y.hi.y < x.lo.y) continue;
                const auto piece = clip(x.ccw, y.ccw);
                if (piece.size() < 3 || area(piece) < kMinOverlapArea) continue;
                StationInterval sa{INFINITY, -INFINITY}, sb{INFINITY, -INFINITY};
                // The station at each corner bounds the piece: a cross-section is a straight line,
                // so the extreme cross-sections of a convex piece pass through its corners.
                for (const auto& p : piece) {
                    const double u = stationIn(x, p), v = stationIn(y, p);
                    sa = {std::min(sa.from, u), std::max(sa.to, u)};
                    sb = {std::min(sb.from, v), std::max(sb.to, v)};
                }
                onA.push_back(sa); onB.push_back(sb);
            }
        if (onA.empty()) { result.status = SurfaceOverlap::Status::none; return result; }
        const auto ja = joined(onA), jb = joined(onB);
        if (!ja || !jb) return result; // they cross twice: which area was meant is a guess
        result = {SurfaceOverlap::Status::overlap, *ja, *jb};
    } catch (const std::exception&) {}
    return result;
}
}
