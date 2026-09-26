#include "right_of_way_commands.hpp"
#include "../model/network/right_of_way.hpp"
#include "detail.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

// M3.2.4: the conflict-area gestures the editor offers, built only from the M3.2.2 put/delete
// commands, so the structural checks and History's atomicity are the ones already tested.
namespace trafficsim {
namespace {
// Where an authored crossing's waiting line stands: 1 m short of its entry, the same setback a
// derived merge rule uses (D50), so a vehicle waiting there is clear of the area.
constexpr double kCrossingSetback = 1.0;
void requireDefaults(const PriorityDefaults& d) {
    if (!std::isfinite(d.gapTime) || d.gapTime <= 0 || !std::isfinite(d.headway) || d.headway <= 0)
        throw std::invalid_argument("EDIT_NO_PRIORITY_DEFAULTS");
}
// Every lane (Link) or lane path (Connector) of an object, as a control reference.
std::vector<ControlPathRef> lanesOf(const Network& n, const std::string& id) {
    std::vector<ControlPathRef> refs;
    for (const auto& l : n.links)
        if (l.id == id) { for (const auto& lane : l.lanes) refs.push_back({l.id, lane.id, "", "", ""}); return refs; }
    for (const auto& c : n.connectors)
        if (c.id == id) {
            for (const auto& p : connectorPaths(n, c)) refs.push_back({"", "", c.id, p.from.laneId, p.to.laneId});
            return refs;
        }
    throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
const ConflictArea& area(const ProjectDocument& d, const std::string& id) {
    for (const auto& a : d.network.rightOfWay.conflictAreas) if (a.id == id) return a;
    throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
}
std::vector<std::string> addCrossingAreas(ProjectDocument& d, const std::string& first, const std::string& second,
                                          const std::string& yielding, const PriorityDefaults& defaults) {
    if (first == second) throw std::invalid_argument("EDIT_SAME_OBJECT");
    if (yielding != first && yielding != second) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    requireDefaults(defaults);
    const auto a = lanesOf(d.network, first), b = lanesOf(d.network, second);
    struct Found { std::size_t i, j; SurfaceOverlap o; };
    std::vector<Found> found;
    for (std::size_t i = 0; i < a.size(); ++i)
        for (std::size_t j = 0; j < b.size(); ++j) {
            const auto o = surfaceOverlap(d.network, a[i], b[j]);
            if (o.status == SurfaceOverlap::Status::overlap) found.push_back({i, j, o}); // no guessed area (§1)
        }
    if (found.empty()) throw std::invalid_argument("EDIT_NO_CROSSING");
    // One line per lane, before the FIRST area that lane meets (D63): a line per area put the far
    // lane's line inside the near lane's area, where a Stop would halt a vehicle mid-crossing.
    // The areas behind one line are admitted together (A15), exactly as before.
    const auto lineFor = [&](const std::vector<ControlPathRef>& lanes, std::size_t k, bool onFirst) {
        double from = INFINITY;
        for (const auto& f : found)
            if ((onFirst ? f.i : f.j) == k) from = std::min(from, onFirst ? f.o.first.from : f.o.second.from);
        return putWaitingLine(d, {"", "", {lanes[k], std::max(0.0, from - kCrossingSetback)}});
    };
    std::map<std::size_t, std::string> linesA, linesB;
    for (const auto& f : found) {
        if (!linesA.contains(f.i)) linesA[f.i] = lineFor(a, f.i, true);
        if (!linesB.contains(f.j)) linesB[f.j] = lineFor(b, f.j, false);
    }
    std::vector<std::string> created;
    for (const auto& f : found) {
        const auto id = putConflictArea(d, {"", "", ConflictKind::crossing, {a[f.i], f.o.first.from, f.o.first.to, linesA[f.i]},
                                            {b[f.j], f.o.second.from, f.o.second.to, linesB[f.j]},
                                            yielding == first ? ConflictPriority::firstYields : ConflictPriority::secondYields});
        putPriorityRule(d, {"", "", id, defaults.gapTime, defaults.headway});
        created.push_back(id);
    }
    return created;
}
void setConflictControl(ProjectDocument& d, const std::string& areaId, const std::string& name,
                        ConflictPriority priority, double gapTime, double headway) {
    auto changed = area(d, areaId);
    // Another side giving way waits at another line, so the Stop/Yield the old line set no longer
    // applies to this area (M3.2.5); it is cleared rather than left naming the wrong line.
    // Only this area leaves the old line's control; the others still giving way there keep it.
    if (changed.priority != priority) {
        auto& controls = d.network.rightOfWay.stopControls;
        for (auto& c : controls) std::erase(c.conflictAreaIds, areaId);
        std::erase_if(controls, [](const auto& c) { return c.conflictAreaIds.empty(); });
    }
    changed.name = name; changed.priority = priority;
    putConflictArea(d, changed);
    auto& rules = d.network.rightOfWay.priorityRules;
    const auto rule = std::find_if(rules.begin(), rules.end(), [&](const auto& r) { return r.conflictAreaId == areaId; });
    auto updated = rule == rules.end() ? AuthoredPriorityRule{"", "", areaId, gapTime, headway} : *rule;
    updated.gapTime = gapTime; updated.headway = headway;
    putPriorityRule(d, updated); // the numbers are checked when the edit commits
}
ConflictPriority cycleConflictPriority(ProjectDocument& d, const std::string& areaId, const PriorityDefaults& defaults) {
    const auto a = area(d, areaId); // a copy: setConflictControl replaces the element
    const auto next = a.priority == ConflictPriority::firstYields ? ConflictPriority::secondYields
                    : a.priority == ConflictPriority::secondYields ? ConflictPriority::undetermined : ConflictPriority::firstYields;
    const auto& rules = d.network.rightOfWay.priorityRules;
    const auto rule = std::find_if(rules.begin(), rules.end(), [&](const auto& r) { return r.conflictAreaId == areaId; });
    if (rule == rules.end()) requireDefaults(defaults);
    const double gap = rule == rules.end() ? defaults.gapTime : rule->gapTime;
    const double headway = rule == rules.end() ? defaults.headway : rule->headway;
    setConflictControl(d, areaId, a.name, next, gap, headway);
    return next;
}
void moveWaitingLine(ProjectDocument& d, const std::string& lineId, double station) {
    const auto& lines = d.network.rightOfWay.waitingLines;
    const auto line = std::find_if(lines.begin(), lines.end(), [&](const auto& w) { return w.id == lineId; });
    if (line == lines.end()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    auto moved = *line; moved.point.station = station;
    putWaitingLine(d, moved);
}
std::vector<std::string> takeOverMergesOf(ProjectDocument& d, const std::string& connectorId, const PriorityDefaults& defaults) {
    requireDefaults(defaults);
    std::vector<std::string> created;
    const auto table = runtimeSections(d.network);
    const auto groups = mergeGroups(d.network, table);
    for (const auto& section : mergeSectionsOf(d.network, table, connectorId)) {
        const auto g = std::find_if(groups.begin(), groups.end(), [&](const auto& x) { return x.section == section; });
        if (g == groups.end() || g->explicitControl) continue;
        for (auto& id : takeOverMerge(d, section, defaults)) created.push_back(std::move(id));
    }
    if (created.empty()) throw std::invalid_argument("EDIT_NO_MERGE");
    return created;
}
void restoreAutomaticPriorityOf(ProjectDocument& d, const std::string& areaId) {
    const auto section = mergeSectionOfArea(d.network, runtimeSections(d.network), area(d, areaId));
    if (section.empty()) throw std::invalid_argument("EDIT_NO_MERGE");
    restoreAutomaticPriority(d, section);
}
void setAreaControl(ProjectDocument& d, const std::string& areaId, std::optional<StopMode> mode) {
    const auto a = area(d, areaId);
    if (a.priority == ConflictPriority::undetermined) throw std::invalid_argument("EDIT_UNDETERMINED_PRIORITY");
    const auto yieldingLine = [](const ConflictArea& x) {
        return x.priority == ConflictPriority::undetermined ? std::string{}
             : (x.priority == ConflictPriority::firstYields ? x.first : x.second).waitingLineId; };
    const auto line = yieldingLine(a);
    auto& row = d.network.rightOfWay;
    // The control is the line's (D62): every decided area giving way at it, never one of them.
    std::vector<std::string> behind;
    for (const auto& x : row.conflictAreas) if (yieldingLine(x) == line) behind.push_back(x.id);
    auto& controls = row.stopControls;
    for (auto& c : controls) std::erase_if(c.conflictAreaIds, [&](const auto& id) {
        return c.waitingLineId == line || std::find(behind.begin(), behind.end(), id) != behind.end(); });
    std::erase_if(controls, [](const auto& c) { return c.conflictAreaIds.empty(); });
    if (mode) putStopControl(d, {"", "", line, *mode, behind});
}
void removeConflictArea(ProjectDocument& d, const std::string& areaId) {
    const auto removed = area(d, areaId);
    deleteConflictArea(d, areaId);
    auto& row = d.network.rightOfWay;
    for (const auto* side : {&removed.first, &removed.second}) {
        const bool used = std::any_of(row.conflictAreas.begin(), row.conflictAreas.end(), [&](const auto& a) {
            return a.first.waitingLineId == side->waitingLineId || a.second.waitingLineId == side->waitingLineId; });
        if (!used) std::erase_if(row.waitingLines, [&](const auto& w) { return w.id == side->waitingLineId; });
    }
    detail::pruneQueueCounters(d);
}
}
namespace trafficsim {
std::string authorAutomaticConflict(ProjectDocument& d, const AutomaticConflict& automatic, const PriorityDefaults& defaults) {
    requireDefaults(defaults);
    if (automatic.kind == ConflictKind::merge) {
        // The take-over stores the whole group exactly as shown; the clicked pair is returned.
        const auto created = takeOverMerge(d, automatic.mergeSection, defaults);
        for (const auto& id : created) {
            const auto& a = area(d, id);
            if (a.first.path == automatic.first.path && a.second.path == automatic.second.path) return id;
        }
        throw std::invalid_argument("EDIT_NO_MERGE");
    }
    // Still passive? A stale click on a pair authored since is refused rather than doubled.
    const auto now = automaticConflicts(d.network);
    if (std::none_of(now.begin(), now.end(), [&](const auto& x) { return x.key == automatic.key; }))
        throw std::invalid_argument("EDIT_NO_CROSSING");
    // A lane's crossing areas share one waiting line before the first of them (D63): reuse the
    // line this path already waits at, moved upstream when the new area comes first.
    const auto lineFor = [&](const ConflictSide& side) {
        const double station = std::max(0.0, side.entryStation - kCrossingSetback);
        for (const auto& a : d.network.rightOfWay.conflictAreas) {
            if (a.kind != ConflictKind::crossing) continue;
            for (const auto* s : {&a.first, &a.second})
                if (s->path == side.path) {
                    for (const auto& w : d.network.rightOfWay.waitingLines)
                        if (w.id == s->waitingLineId && w.point.station > station) moveWaitingLine(d, w.id, station);
                    return s->waitingLineId;
                }
        }
        return putWaitingLine(d, {"", "", {side.path, station}});
    };
    auto first = automatic.first, second = automatic.second;
    first.waitingLineId = lineFor(first);
    second.waitingLineId = lineFor(second);
    // Who gives way first: a turning Connector yields to a Link, as a side road yields to the
    // through road; between two of a kind, the second in key order. One click cycles it after.
    const bool firstIsConnector = !first.path.connectorId.empty(), secondIsConnector = !second.path.connectorId.empty();
    const auto priority = firstIsConnector && !secondIsConnector ? ConflictPriority::firstYields : ConflictPriority::secondYields;
    const auto id = putConflictArea(d, {"", "", ConflictKind::crossing, first, second, priority});
    putPriorityRule(d, {"", "", id, defaults.gapTime, defaults.headway});
    return id;
}
}
