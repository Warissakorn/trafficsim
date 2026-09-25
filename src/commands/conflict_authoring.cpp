#include "right_of_way_commands.hpp"
#include "../model/network/right_of_way.hpp"
#include <algorithm>
#include <cmath>
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
    std::vector<std::string> created;
    for (const auto& pa : a)
        for (const auto& pb : b) {
            const auto o = surfaceOverlap(d.network, pa, pb);
            if (o.status != SurfaceOverlap::Status::overlap) continue; // no guessed area (§1)
            const auto la = putWaitingLine(d, {"", "", {pa, std::max(0.0, o.first.from - kCrossingSetback)}});
            const auto lb = putWaitingLine(d, {"", "", {pb, std::max(0.0, o.second.from - kCrossingSetback)}});
            const auto id = putConflictArea(d, {"", "", ConflictKind::crossing, {pa, o.first.from, o.first.to, la},
                                                {pb, o.second.from, o.second.to, lb},
                                                yielding == first ? ConflictPriority::firstYields : ConflictPriority::secondYields});
            putPriorityRule(d, {"", "", id, defaults.gapTime, defaults.headway});
            created.push_back(id);
        }
    if (created.empty()) throw std::invalid_argument("EDIT_NO_CROSSING");
    return created;
}
void setConflictControl(ProjectDocument& d, const std::string& areaId, const std::string& name,
                        ConflictPriority priority, double gapTime, double headway) {
    auto changed = area(d, areaId);
    changed.name = name; changed.priority = priority;
    putConflictArea(d, changed);
    auto& rules = d.network.rightOfWay.priorityRules;
    const auto rule = std::find_if(rules.begin(), rules.end(), [&](const auto& r) { return r.conflictAreaId == areaId; });
    auto updated = rule == rules.end() ? AuthoredPriorityRule{"", "", areaId, gapTime, headway} : *rule;
    updated.gapTime = gapTime; updated.headway = headway;
    putPriorityRule(d, updated); // the numbers are checked when the edit commits
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
void removeConflictArea(ProjectDocument& d, const std::string& areaId) {
    const auto removed = area(d, areaId);
    deleteConflictArea(d, areaId);
    auto& row = d.network.rightOfWay;
    for (const auto* side : {&removed.first, &removed.second}) {
        const bool used = std::any_of(row.conflictAreas.begin(), row.conflictAreas.end(), [&](const auto& a) {
            return a.first.waitingLineId == side->waitingLineId || a.second.waitingLineId == side->waitingLineId; });
        if (!used) std::erase_if(row.waitingLines, [&](const auto& w) { return w.id == side->waitingLineId; });
    }
}
}
