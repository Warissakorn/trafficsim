#include "right_of_way_commands.hpp"
#include "../model/network/right_of_way.hpp"
#include "detail.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trafficsim {
namespace {
template<class T> void put(std::vector<T>& values, T value) {
    for (auto& existing : values) if (existing.id == value.id) { existing = std::move(value); return; }
    values.push_back(std::move(value));
}
template<class T> void remove(std::vector<T>& values, const std::string& id) {
    if (!std::erase_if(values, [&](const auto& v) { return v.id == id; })) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
// How far upstream of the join a taken-over merge's waiting line stands: the same 1 m the derived
// rule waits short of the join (D50), so a take-over compiles to what the fallback already ran.
constexpr double kMergeSideLength = 1.0;
// A taken-over side in authored coordinates: its waiting line, entry and exit (the join).
struct Side { ControlPathRef path; double wait{}, entry{}, exit{}; };
// Every station is chosen on the runtime segment itself, then converted to the authored polyline.
// Choosing it on the authored polyline was only exact for a path that IS that polyline: lane 2
// of a curved range is a translated copy with other segment lengths (connector_paths.cpp).
// The entry is at most half the segment back, so it cannot fall into the previous section.
Side sideOf(const Network& n, const RuntimeSections& table, const std::string& segment) {
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
}
std::string putWaitingLine(ProjectDocument& d, WaitingLine value) {
    if (value.id.empty()) value.id = allocateId(d, "wait");
    const auto id = value.id; put(d.network.rightOfWay.waitingLines, std::move(value)); return id;
}
std::string putConflictArea(ProjectDocument& d, ConflictArea value) {
    if (value.id.empty()) value.id = allocateId(d, "conflict");
    const auto id = value.id; put(d.network.rightOfWay.conflictAreas, std::move(value)); return id;
}
std::string putPriorityRule(ProjectDocument& d, AuthoredPriorityRule value) {
    if (value.id.empty()) value.id = allocateId(d, "rule");
    const auto id = value.id; put(d.network.rightOfWay.priorityRules, std::move(value)); return id;
}
std::string putStopControl(ProjectDocument& d, StopControl value) {
    if (value.id.empty()) value.id = allocateId(d, "stop");
    const auto id = value.id; put(d.network.rightOfWay.stopControls, std::move(value)); return id;
}
void deleteStopControl(ProjectDocument& d, const std::string& id) { remove(d.network.rightOfWay.stopControls, id); }
std::string putQueueCounter(ProjectDocument& d, AuthoredQueueCounter value) {
    if (value.id.empty()) value.id = allocateId(d, "counter");
    const auto id = value.id; put(d.network.queueCounters, std::move(value)); return id;
}
void deleteQueueCounter(ProjectDocument& d, const std::string& id) { remove(d.network.queueCounters, id); }
void deleteWaitingLine(ProjectDocument& d, const std::string& id) {
    const auto& areas = d.network.rightOfWay.conflictAreas;
    const auto& controls = d.network.rightOfWay.stopControls;
    if (std::any_of(areas.begin(), areas.end(), [&](const auto& a) {
            return a.first.waitingLineId == id || a.second.waitingLineId == id; }) ||
        std::any_of(controls.begin(), controls.end(), [&](const auto& c) { return c.waitingLineId == id; }))
        throw std::invalid_argument("EDIT_REFERENCED");
    remove(d.network.rightOfWay.waitingLines, id);
    detail::pruneQueueCounters(d); // a counter measuring at the line loses that line
}
void deleteConflictArea(ProjectDocument& d, const std::string& id) {
    auto& row = d.network.rightOfWay;
    remove(row.conflictAreas, id);
    std::erase_if(row.priorityRules, [&](const auto& r) { return r.conflictAreaId == id; });
    detail::pruneStopControls(row);
}
void deletePriorityRule(ProjectDocument& d, const std::string& id) { remove(d.network.rightOfWay.priorityRules, id); }
std::vector<std::string> takeOverMerge(ProjectDocument& d, const std::string& section, const PriorityDefaults& defaults) {
    if (!std::isfinite(defaults.gapTime) || defaults.gapTime <= 0 || !std::isfinite(defaults.headway) || defaults.headway <= 0)
        throw std::invalid_argument("EDIT_NO_PRIORITY_DEFAULTS");
    const auto table = runtimeSections(d.network);
    const auto groups = mergeGroups(d.network, table);
    const auto group = std::find_if(groups.begin(), groups.end(), [&](const auto& g) { return g.section == section; });
    if (group == groups.end()) throw std::invalid_argument("EDIT_NO_MERGE");
    if (group->explicitControl) throw std::invalid_argument("EDIT_ALREADY_EXPLICIT");
    std::vector<Side> sides;
    for (const auto& segment : group->incoming) sides.push_back(sideOf(d.network, table, segment));
    std::vector<std::string> lines;
    for (const auto& side : sides) lines.push_back(putWaitingLine(d, {"", "", {side.path, side.wait}}));
    const auto sideAt = [&](std::size_t k) { return ConflictSide{sides[k].path, sides[k].entry, sides[k].exit, lines[k]}; };
    std::vector<std::string> created;
    // The fallback's order: each later incoming path gives way to every earlier one.
    for (std::size_t j = 1; j < sides.size(); ++j)
        for (std::size_t i = 0; i < j; ++i) {
            const auto area = putConflictArea(d, {"", "", ConflictKind::merge, sideAt(j), sideAt(i), ConflictPriority::firstYields});
            putPriorityRule(d, {"", "", area, defaults.gapTime, defaults.headway});
            created.push_back(area);
        }
    return created;
}
void restoreAutomaticPriority(ProjectDocument& d, const std::string& section) {
    const auto table = runtimeSections(d.network);
    const auto groups = mergeGroups(d.network, table);
    const auto group = std::find_if(groups.begin(), groups.end(), [&](const auto& g) { return g.section == section; });
    if (group == groups.end()) throw std::invalid_argument("EDIT_NO_MERGE");
    const auto member = [&](const ConflictSide& s) {
        const auto seg = resolveControlPath(d.network, table, s.path, s.entryStation);
        return std::find(group->incoming.begin(), group->incoming.end(), seg) != group->incoming.end();
    };
    auto& row = d.network.rightOfWay;
    std::vector<std::string> gone;
    for (const auto& a : row.conflictAreas)
        if (a.kind == ConflictKind::merge && member(a.first) && member(a.second)) gone.push_back(a.id);
    for (const auto& id : gone) deleteConflictArea(d, id);
    std::erase_if(row.waitingLines, [&](const auto& w) {
        return std::none_of(row.conflictAreas.begin(), row.conflictAreas.end(), [&](const auto& a) {
            return a.first.waitingLineId == w.id || a.second.waitingLineId == w.id; });
    });
    detail::pruneQueueCounters(d);
}
}
namespace trafficsim::detail {
namespace {
bool on(const ControlPathRef& p, const std::set<std::string>& links, const std::set<std::string>& connectors) {
    return p.connectorId.empty() ? links.contains(p.linkId) : connectors.contains(p.connectorId);
}
// Every path reference a control holds, with the stations measured along it.
template<class F> void eachPath(Network& n, F visit) {
    auto& row = n.rightOfWay;
    for (auto& w : row.waitingLines) visit(w.point.path, std::vector<double*>{&w.point.station});
    for (auto& a : row.conflictAreas)
        for (auto* s : {&a.first, &a.second}) visit(s->path, std::vector<double*>{&s->entryStation, &s->exitStation});
    // M3.2.6b: a queue counter's explicit measurement point is a place on a road like any other.
    for (auto& c : n.queueCounters)
        for (auto& l : c.lines) if (l.point) visit(l.point->path, std::vector<double*>{&l.point->station});
}
}
void removeControlsOn(ProjectDocument& d, const std::set<std::string>& links, const std::set<std::string>& connectors) {
    auto& row = d.network.rightOfWay;
    if (row.empty()) { pruneQueueCounters(d, links, connectors); return; }
    std::set<std::string> lost;
    for (const auto& w : row.waitingLines) if (on(w.point.path, links, connectors)) lost.insert(w.id);
    std::set<std::string> gone;
    for (const auto& a : row.conflictAreas)
        for (const auto* s : {&a.first, &a.second})
            if (on(s->path, links, connectors) || lost.contains(s->waitingLineId)) gone.insert(a.id);
    // A line on a surviving road that served only removed areas goes with them: it was that
    // area's line, not a standalone one, and would otherwise be left behind as debris.
    const auto serves = [](const ConflictArea& a, const std::string& id) { return a.first.waitingLineId == id || a.second.waitingLineId == id; };
    for (const auto& w : row.waitingLines) {
        bool servedGone = false, servedKept = false;
        for (const auto& a : row.conflictAreas) if (serves(a, w.id)) (gone.contains(a.id) ? servedGone : servedKept) = true;
        if (servedGone && !servedKept) lost.insert(w.id);
    }
    std::erase_if(row.conflictAreas, [&](const auto& a) { return gone.contains(a.id); });
    std::erase_if(row.priorityRules, [&](const auto& r) { return gone.contains(r.conflictAreaId); });
    std::erase_if(row.waitingLines, [&](const auto& w) { return lost.contains(w.id); });
    pruneStopControls(row);
    pruneQueueCounters(d, links, connectors);
}
void pruneQueueCounters(ProjectDocument& d, const std::set<std::string>& links, const std::set<std::string>& connectors) {
    auto& n = d.network;
    if (n.queueCounters.empty()) return;
    std::set<std::string> targets;
    for (const auto& h : n.signalHeads) targets.insert(h.id);
    for (const auto& w : n.rightOfWay.waitingLines) targets.insert(w.id);
    for (auto& c : n.queueCounters)
        std::erase_if(c.lines, [&](const auto& l) {
            return l.point ? on(l.point->path, links, connectors) : !targets.contains(l.referenceId); });
    std::erase_if(n.queueCounters, [](const auto& c) { return c.lines.empty(); });
}
void pruneStopControls(RightOfWay& row) {
    const auto hasArea = [&](const std::string& id) {
        return std::any_of(row.conflictAreas.begin(), row.conflictAreas.end(), [&](const auto& a) { return a.id == id; }); };
    const auto hasLine = [&](const std::string& id) {
        return std::any_of(row.waitingLines.begin(), row.waitingLines.end(), [&](const auto& w) { return w.id == id; }); };
    for (auto& c : row.stopControls) std::erase_if(c.conflictAreaIds, [&](const auto& id) { return !hasArea(id); });
    std::erase_if(row.stopControls, [&](const auto& c) { return c.conflictAreaIds.empty() || !hasLine(c.waitingLineId); });
}
bool controlsNameLink(const Network& n, const std::string& link) {
    bool named = false;
    auto copy = n;
    eachPath(copy, [&](const ControlPathRef& p, const auto&) { named = named || (p.connectorId.empty() && p.linkId == link); });
    return named;
}
void checkSplitControls(const Network& n, const std::string& link, double distance) {
    const double near = distance - 0.1, far = distance + 0.1;
    for (const auto& w : n.rightOfWay.waitingLines)
        if (w.point.path.connectorId.empty() && w.point.path.linkId == link && w.point.station > near && w.point.station < far)
            throw std::invalid_argument("EDIT_SPLIT_CONTROL");
    for (const auto& a : n.rightOfWay.conflictAreas)
        for (const auto* s : {&a.first, &a.second})
            if (s->path.connectorId.empty() && s->path.linkId == link && !(s->exitStation <= near || s->entryStation >= far))
                throw std::invalid_argument("EDIT_SPLIT_CONTROL");
    for (const auto& c : n.queueCounters)
        for (const auto& l : c.lines)
            if (l.point && l.point->path.connectorId.empty() && l.point->path.linkId == link && l.point->station > near && l.point->station < far)
                throw std::invalid_argument("EDIT_SPLIT_CONTROL");
}
void splitControls(ProjectDocument& d, const std::string& link, const std::string& downstream, double distance,
                   const std::map<std::string, std::string>& lanes) {
    const double far = distance + 0.1;
    std::map<std::string, const Connector*> connectors;
    for (const auto& c : d.network.connectors) connectors[c.id] = &c;
    eachPath(d.network, [&](ControlPathRef& p, const std::vector<double*>& stations) {
        if (p.connectorId.empty()) {
            // checkSplitControls already refused anything in or across the span.
            if (p.linkId != link || *stations.front() < far) return;
            p.linkId = downstream; p.laneId = lanes.at(p.laneId);
            for (auto* s : stations) *s -= far;
            return;
        }
        const auto it = connectors.find(p.connectorId);
        if (it == connectors.end()) return;
        if (it->second->from.linkId == downstream && lanes.contains(p.fromLaneId)) p.fromLaneId = lanes.at(p.fromLaneId);
        if (it->second->to.linkId == downstream && lanes.contains(p.toLaneId)) p.toLaneId = lanes.at(p.toLaneId);
    });
}
void copyControls(ProjectDocument& d, const Network& source, const std::map<std::string, std::string>& links,
                  const std::map<std::string, std::string>& lanes, const std::map<std::string, std::string>& connectors) {
    const auto copied = [&](const ControlPathRef& p) {
        return p.connectorId.empty() ? links.contains(p.linkId) && lanes.contains(p.laneId)
                                     : connectors.contains(p.connectorId) && lanes.contains(p.fromLaneId) && lanes.contains(p.toLaneId);
    };
    const auto remap = [&](ControlPathRef p) {
        if (p.connectorId.empty()) { p.linkId = links.at(p.linkId); p.laneId = lanes.at(p.laneId); }
        else { p.connectorId = connectors.at(p.connectorId); p.fromLaneId = lanes.at(p.fromLaneId); p.toLaneId = lanes.at(p.toLaneId); }
        return p;
    };
    const auto& row = source.rightOfWay;
    std::map<std::string, std::string> lines, areas;
    for (auto w : row.waitingLines) {
        if (!copied(w.point.path)) continue;
        const auto old = w.id;
        w.id = allocateId(d, "wait"); w.point.path = remap(w.point.path);
        lines[old] = w.id; d.network.rightOfWay.waitingLines.push_back(std::move(w));
    }
    for (auto a : row.conflictAreas) {
        const bool whole = copied(a.first.path) && copied(a.second.path) &&
                           lines.contains(a.first.waitingLineId) && lines.contains(a.second.waitingLineId);
        if (!whole) continue;
        const auto old = a.id;
        a.id = allocateId(d, "conflict");
        for (auto* s : {&a.first, &a.second}) { s->path = remap(s->path); s->waitingLineId = lines.at(s->waitingLineId); }
        areas[old] = a.id; d.network.rightOfWay.conflictAreas.push_back(std::move(a));
    }
    for (auto r : row.priorityRules) {
        if (!areas.contains(r.conflictAreaId)) continue;
        r.id = allocateId(d, "rule"); r.conflictAreaId = areas.at(r.conflictAreaId);
        d.network.rightOfWay.priorityRules.push_back(std::move(r));
    }
    // A control goes with its line and keeps the areas that were copied with it.
    for (auto c : row.stopControls) {
        if (!lines.contains(c.waitingLineId)) continue;
        std::erase_if(c.conflictAreaIds, [&](const auto& id) { return !areas.contains(id); });
        if (c.conflictAreaIds.empty()) continue;
        for (auto& id : c.conflictAreaIds) id = areas.at(id);
        c.id = allocateId(d, "stop"); c.waitingLineId = lines.at(c.waitingLineId);
        d.network.rightOfWay.stopControls.push_back(std::move(c));
    }
    // A line that only served areas left behind (one owner missing) would be an orphan copy.
    // One nothing referenced in the source is kept: it was a standalone line and still is.
    for (const auto& [old, fresh] : lines) {
        const auto serves = [&](const auto& a, const std::string& id) { return a.first.waitingLineId == id || a.second.waitingLineId == id; };
        const bool servedBefore = std::any_of(row.conflictAreas.begin(), row.conflictAreas.end(), [&](const auto& a) { return serves(a, old); });
        const auto& now = d.network.rightOfWay.conflictAreas;
        if (servedBefore && std::none_of(now.begin(), now.end(), [&](const auto& a) { return serves(a, fresh); }))
            std::erase_if(d.network.rightOfWay.waitingLines, [&](const auto& w) { return w.id == fresh; });
    }
}
}
