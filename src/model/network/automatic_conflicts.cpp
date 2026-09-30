#include "right_of_way.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

// M3.2.4c (D68): conflict areas the drawing implies, as Vissim generates them. See the header.
namespace trafficsim {
namespace {
// How far upstream of the join a taken-over merge's waiting line stands: the same 1 m the derived
// rule waits short of the join (D50), so a take-over compiles to what the fallback already ran.
constexpr double kMergeSideLength = 1.0;
std::string refKey(const ControlPathRef& r) {
    return r.connectorId.empty() ? r.linkId + "/" + r.laneId : r.connectorId + "/" + r.fromLaneId + ">" + r.toLaneId;
}
// One lane path a crossing can involve, with what the exclusions and the prefilter read.
struct Path {
    ControlPathRef ref; std::string owner, key, fromLink, toLink, fromLane, toLane;
    int level{}; Point lo{}, hi{};
};
void bound(Path& p, const std::vector<Point>& g, double margin) {
    p.lo = {INFINITY, INFINITY}; p.hi = {-INFINITY, -INFINITY};
    for (const auto& q : g) {
        p.lo = {std::min(p.lo.x, q.x - margin), std::min(p.lo.y, q.y - margin)};
        p.hi = {std::max(p.hi.x, q.x + margin), std::max(p.hi.y, q.y + margin)};
    }
}
// Holds the piece's middle, so a stale area that merely touches a piece does not hide it (D85).
bool holds(double from, double to, StationInterval i) { const double m = (i.from + i.to) / 2; return from <= m && m <= to; }
// Pairs whose overlap is not a crossing. One place, so a later case (a diverge) is lifted here.
bool sharedMouth(const Path& a, const Path& b) {
    if (a.owner == b.owner || a.level != b.level) return true;
    const bool ca = !a.ref.connectorId.empty(), cb = !b.ref.connectorId.empty();
    if (ca && cb) // same entry lane: a diverge; same exit lane: a merge (mergeGroups owns it)
        return (a.fromLink == b.fromLink && a.fromLane == b.fromLane) || (a.toLink == b.toLink && a.toLane == b.toLane);
    // A Connector's mouth lies on exactly one lane of each of its Links; its other lanes it may cross (D72).
    const auto mouth = [](const Path& c, const Path& l) {
        return (c.fromLink == l.owner && c.fromLane == l.ref.laneId) || (c.toLink == l.owner && c.toLane == l.ref.laneId); };
    if (ca && !cb) return mouth(a, b);
    if (cb && !ca) return mouth(b, a);
    return false;
}
}
// Every station is chosen on the runtime segment itself, then converted to the authored polyline.
// Choosing it on the authored polyline was only exact for a path that IS that polyline: lane 2
// of a curved range is a translated copy with other segment lengths (connector_paths.cpp).
// The entry is at most half the segment back, so it cannot fall into the previous section.
MergeSide mergeSide(const Network& n, const RuntimeSections& table, const std::string& segment) {
    for (const auto& path : table.paths) {
        if (path.id != segment) continue;
        for (const auto& c : n.connectors)
            for (int i = 0; i < std::max(c.fromLaneCount, c.toLaneCount); ++i)
                if (connectorPathId(c, i) == segment) {
                    const double length = polylineLength(path.geometry);
                    const auto base = [&](double s) { return matchedStation(path.geometry, c.geometry, s); };
                    return {{"", "", c.id, path.from.laneId, path.to.laneId},
                            base(std::max(0.0, length - kMergeSideLength)),
                            base(length - std::min(kMergeSideLength, length / 2)), base(length)};
                }
    }
    for (const auto& section : table.sections) {
        if (section.id != segment) continue;
        for (const auto& link : n.links)
            if (link.id == section.linkId) {
                const auto lane = laneGeometry(link, section.laneId, n.drivingSide);
                const auto reference = [&](double s) { return matchedStation(lane, link.geometry, s); };
                // A station exactly on a cut resolves upstream (sectionForStation), so the waiting
                // line never goes further back than the entry does.
                const double entry = section.end - std::min(kMergeSideLength, (section.end - section.start) / 2);
                return {{link.id, section.laneId, "", "", ""}, reference(entry), reference(entry), reference(section.end)};
            }
    }
    throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
// An authored area covers the piece it lies over, not every piece of the pair (D72).
bool authoredCovers(const Network& n, const ControlPathRef& a, const ControlPathRef& b, const SurfaceOverlap& o) {
    return std::any_of(n.rightOfWay.conflictAreas.begin(), n.rightOfWay.conflictAreas.end(), [&](const auto& x) {
        if (x.first.path == a && x.second.path == b)
            return holds(x.first.entryStation, x.first.exitStation, o.first) && holds(x.second.entryStation, x.second.exitStation, o.second);
        if (x.first.path == b && x.second.path == a)
            return holds(x.first.entryStation, x.first.exitStation, o.second) && holds(x.second.entryStation, x.second.exitStation, o.first);
        return false; });
}
std::vector<AutomaticConflict> automaticConflicts(const Network& n) {
    std::vector<AutomaticConflict> result;
    std::vector<Path> paths;
    for (const auto& l : n.links)
        for (const auto& lane : l.lanes) {
            Path p{{l.id, lane.id, "", "", ""}, l.id, "", "", "", "", "", l.level};
            bound(p, laneGeometry(l, lane.id, n.drivingSide), lane.width);
            paths.push_back(std::move(p));
        }
    for (const auto& c : n.connectors)
        for (const auto& cp : connectorPaths(n, c)) {
            Path p{{"", "", c.id, cp.from.laneId, cp.to.laneId}, c.id, "", c.from.linkId, c.to.linkId,
                   cp.from.laneId, cp.to.laneId, c.level};
            bound(p, cp.geometry, 4);
            paths.push_back(std::move(p));
        }
    for (auto& p : paths) p.key = refKey(p.ref);
    std::sort(paths.begin(), paths.end(), [](const auto& a, const auto& b) { return a.key < b.key; });
    for (std::size_t i = 0; i < paths.size(); ++i)
        for (std::size_t j = i + 1; j < paths.size(); ++j) {
            const auto& a = paths[i]; const auto& b = paths[j];
            if (sharedMouth(a, b)) continue;
            if (a.hi.x < b.lo.x || b.hi.x < a.lo.x || a.hi.y < b.lo.y || b.hi.y < a.lo.y) continue;
            const auto pieces = surfaceOverlaps(n, a.ref, b.ref);
            for (std::size_t k = 0; k < pieces.size(); ++k) {
                const auto& o = pieces[k];
                if (o.status != SurfaceOverlap::Status::overlap) continue; // no guessed area (§1)
                if (authoredCovers(n, a.ref, b.ref, o)) continue;
                result.push_back({ConflictKind::crossing, {a.ref, o.first.from, o.first.to, ""}, {b.ref, o.second.from, o.second.to, ""},
                                  ConflictPriority::undetermined,
                                  "auto/" + a.key + "|" + b.key + (k ? "#" + std::to_string(k) : std::string{}), ""});
            }
        }
    const auto table = runtimeSections(n);
    for (const auto& g : mergeGroups(n, table)) {
        if (g.explicitControl) continue;
        std::vector<MergeSide> sides;
        for (const auto& segment : g.incoming) sides.push_back(mergeSide(n, table, segment));
        // The fallback's order: each later incoming path gives way to every earlier one.
        for (std::size_t j = 1; j < sides.size(); ++j)
            for (std::size_t i = 0; i < j; ++i)
                result.push_back({ConflictKind::merge, {sides[j].path, sides[j].entry, sides[j].exit, ""},
                                  {sides[i].path, sides[i].entry, sides[i].exit, ""}, ConflictPriority::firstYields,
                                  "auto/" + g.section + "/" + std::to_string(j) + "/" + std::to_string(i), g.section});
    }
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.key < b.key; });
    return result;
}
}
