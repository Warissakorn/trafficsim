#include "right_of_way.hpp"
#include "conflict_surface.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

// M3.2.2c: the crossing-coverage measurement (docs/reference/M3_CONTRACT.md §1). Two lane surfaces are
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
struct Strip { std::vector<Point> base, left, right; std::vector<double> stations{}; };
std::optional<Strip> stripOf(const Network& n, const ControlPathRef& ref) {
    // Calculation, outlines and waiting bars share one surface source.
    const auto surface=conflictSurface(n,ref);
    if(!surface)return std::nullopt;
    return Strip{surface->base,surface->left,surface->right,surface->stations};
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
        const double length = s.stations.empty() ? std::hypot(s.base[j + 1].x - s.base[j].x, s.base[j + 1].y - s.base[j].y) : s.stations[j+1]-s.stations[j];
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
std::vector<Point> controlPathPolyline(const Network& n, const ControlPathRef& ref) {
    const auto strip = stripOf(n, ref);
    return strip ? strip->base : std::vector<Point>{};
}
std::optional<ControlPoint> laneControlPoint(const Network& n, const LaneReference& lane, double laneStation) {
    for (const auto& l : n.links) if (l.id == lane.linkId)
        for (const auto& x : l.lanes) if (x.id == lane.laneId) {
            const auto along = laneGeometry(l, x.id, n.drivingSide);
            if (along.size() != l.geometry.size() || !std::isfinite(laneStation)) return std::nullopt;
            return ControlPoint{{l.id, x.id, "", "", ""}, matchedStation(along, l.geometry, laneStation)};
        }
    return std::nullopt;
}
std::optional<std::pair<Point, Point>> waitingLineBar(const Network& n, const ControlPoint& point) {
    try {
        const auto strip = stripOf(n, point.path);
        if (!strip || strip->left.size() != strip->base.size() || strip->right.size() != strip->base.size()) return std::nullopt;
        if (point.station < 0 || point.station > polylineLength(strip->base)) return std::nullopt;
        if(!point.path.connectorId.empty()) {
            // P1-P4 bends boundary vertices longitudinally. Matching their segment parameter
            // no longer puts a waiting bar on the normal at its authored runtime station.
            // Authoring stations stay on base; locate the bar on the same lane path as the car.
            const auto found=std::find_if(n.connectors.begin(),n.connectors.end(),[&](const auto& c){return c.id==point.path.connectorId;});
            if(found==n.connectors.end())return std::nullopt;
            const auto paths=connectorPaths(n,*found);
            const auto selected=std::find_if(paths.begin(),paths.end(),[&](const auto& path){
                return path.from.laneId==point.path.fromLaneId && path.to.laneId==point.path.toLaneId;
            });
            if(selected==paths.end())return std::nullopt;
            const auto& path=*selected;
            const double station=connectorRuntimeStation(*found,path,point.station);
            const auto origin=connectorPathPoint(path,station);
            const auto u=connectorPathDirection(path,station);
            const auto meet=[&](const std::vector<Point>& edge)->std::optional<Point> {
                std::optional<Point> best;double nearest=INFINITY;
                const auto guess=pointAlong(edge,matchedStation(strip->base,edge,point.station));
                for(std::size_t i=1;i<edge.size();++i) {
                    const auto d=sub(edge[i],edge[i-1]);const double denominator=d.x*u.x+d.y*u.y;
                    if(std::abs(denominator)<1e-12)continue;
                    const auto offset=sub(origin,edge[i-1]);
                    const double t=(offset.x*u.x+offset.y*u.y)/denominator;
                    if(t<0 || t>1)continue;
                    const Point hit{edge[i-1].x+t*d.x,edge[i-1].y+t*d.y};
                    const double distance=std::hypot(hit.x-guess.x,hit.y-guess.y);
                    if(distance<nearest){nearest=distance;best=hit;}
                }
                return best;
            };
            const auto a=meet(strip->left),b=meet(strip->right);
            if(!a || !b)return std::nullopt;
            return std::pair{*a,*b};
        }
        return std::pair{pointAlong(strip->left, matchedStation(strip->base, strip->left, point.station)),
                         pointAlong(strip->right, matchedStation(strip->base, strip->right, point.station))};
    } catch (const std::exception&) { return std::nullopt; }
}
// D72: every place two surfaces overlap, as separate areas. Pieces that touch on BOTH paths are
// one area; a pair that crosses twice is two. When nothing is measured the result is one element
// whose status says why (none / unsupported / unresolved).
std::vector<SurfaceOverlap> surfaceOverlaps(const Network& n, const ControlPathRef& first, const ControlPathRef& second) {
    SurfaceOverlap failed;
    const auto a = stripOf(n, first), b = stripOf(n, second);
    if (!a || !b) return {failed};
    failed.status = SurfaceOverlap::Status::unsupported;
    try {
        const auto qa = quadsOf(*a), qb = quadsOf(*b);
        if (!qa || !qb) return {failed};
        std::vector<StationInterval> onA, onB;
        ConflictPolygons clipped;
        for (const auto& x : *qa)
            for (const auto& y : *qb) {
                if (x.hi.x < y.lo.x || y.hi.x < x.lo.x || x.hi.y < y.lo.y || y.hi.y < x.lo.y) continue;
                const auto piece = clip(x.ccw, y.ccw);
                if (piece.size() < 3 || area(piece) < kMinOverlapArea) continue;
                StationInterval sa{INFINITY, -INFINITY}, sb{INFINITY, -INFINITY};
                for (const auto& p : piece) {
                    const double u = stationIn(x,p);
                    const double v = stationIn(y,p);
                    sa = {std::min(sa.from, u), std::max(sa.to, u)};
                    sb = {std::min(sb.from, v), std::max(sb.to, v)};
                }
                onA.push_back(sa); onB.push_back(sb); clipped.push_back(piece);
            }
        if (onA.empty()) { SurfaceOverlap none; none.status = SurfaceOverlap::Status::none; return {none}; }
        // Union-find over pieces touching on both paths.
        std::vector<std::size_t> root(onA.size());
        for (std::size_t i = 0; i < root.size(); ++i) root[i] = i;
        const auto find = [&](std::size_t i) { while (root[i] != i) i = root[i] = root[root[i]]; return i; };
        const auto touch = [](StationInterval p, StationInterval q) { return p.from <= q.to + kJoinGap && q.from <= p.to + kJoinGap; };
        for (std::size_t i = 0; i < onA.size(); ++i)
            for (std::size_t j = i + 1; j < onA.size(); ++j)
                if (touch(onA[i], onA[j]) && touch(onB[i], onB[j])) root[find(i)] = find(j);
        std::vector<SurfaceOverlap> result;
        std::vector<std::size_t> owner;
        for (std::size_t i = 0; i < onA.size(); ++i) {
            const auto r = find(i);
            const auto at = std::find(owner.begin(), owner.end(), r);
            if (at == owner.end()) { owner.push_back(r); result.push_back({SurfaceOverlap::Status::overlap, onA[i], onB[i], {clipped[i]}}); continue; }
            auto& o = result[static_cast<std::size_t>(at - owner.begin())];
            o.first = {std::min(o.first.from, onA[i].from), std::max(o.first.to, onA[i].to)};
            o.second = {std::min(o.second.from, onB[i].from), std::max(o.second.to, onB[i].to)};
            o.polygons.push_back(clipped[i]);
        }
        std::sort(result.begin(), result.end(), [](const auto& x, const auto& y) {
            return x.first.from != y.first.from ? x.first.from < y.first.from : x.second.from < y.second.from; });
        return result;
    } catch (const std::exception&) {}
    return {failed};
}
SurfaceOverlap surfaceOverlap(const Network& n, const ControlPathRef& first, const ControlPathRef& second) {
    const auto pieces = surfaceOverlaps(n, first, second);
    if (pieces.front().status != SurfaceOverlap::Status::overlap) return pieces.front();
    std::vector<StationInterval> onA, onB;
    ConflictPolygons polygons;
    for (const auto& p : pieces) {
        onA.push_back(p.first); onB.push_back(p.second);
        polygons.insert(polygons.end(), p.polygons.begin(), p.polygons.end());
    }
    const auto a = joined(onA), b = joined(onB);
    if (!a || !b) return {SurfaceOverlap::Status::unsupported, {}, {}};
    return {SurfaceOverlap::Status::overlap, *a, *b, std::move(polygons)};
}
}
