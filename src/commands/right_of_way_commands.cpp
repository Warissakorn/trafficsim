#include "right_of_way_commands.hpp"
#include "../model/network/right_of_way.hpp"
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
// How far upstream of the join a taken-over merge's side begins, and where its waiting line
// stands. The same 1 m the derived rule waits short of the join (D50), so a take-over compiles
// to what the fallback already ran.
constexpr double kMergeSideLength = 1.0;
struct Side { ControlPathRef path; double join{}; };
// The authored coordinates of an incoming runtime segment, at the point it joins the merge.
Side sideOf(const Network& n, const RuntimeSections& table, const std::string& segment) {
    for (const auto& path : table.paths) {
        if (path.id != segment) continue;
        for (const auto& c : n.connectors)
            for (int i = 0; i < std::max(c.fromLaneCount, c.toLaneCount); ++i)
                if (connectorPathId(c, i) == segment)
                    return {{"", "", c.id, path.from.laneId, path.to.laneId},
                            matchedStation(path.geometry, c.geometry, polylineLength(path.geometry))};
    }
    for (const auto& section : table.sections) {
        if (section.id != segment) continue;
        for (const auto& link : n.links)
            if (link.id == section.linkId)
                return {{link.id, section.laneId, "", "", ""},
                        matchedStation(laneGeometry(link, section.laneId, n.drivingSide), link.geometry, section.end)};
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
void deleteWaitingLine(ProjectDocument& d, const std::string& id) {
    const auto& areas = d.network.rightOfWay.conflictAreas;
    if (std::any_of(areas.begin(), areas.end(), [&](const auto& a) {
            return a.first.waitingLineId == id || a.second.waitingLineId == id; }))
        throw std::invalid_argument("EDIT_REFERENCED");
    remove(d.network.rightOfWay.waitingLines, id);
}
void deleteConflictArea(ProjectDocument& d, const std::string& id) {
    auto& row = d.network.rightOfWay;
    remove(row.conflictAreas, id);
    std::erase_if(row.priorityRules, [&](const auto& r) { return r.conflictAreaId == id; });
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
    for (const auto& side : sides)
        lines.push_back(putWaitingLine(d, {"", "", {side.path, std::max(0.0, side.join - kMergeSideLength)}}));
    const auto sideAt = [&](std::size_t k) {
        return ConflictSide{sides[k].path, std::max(0.0, sides[k].join - kMergeSideLength), sides[k].join, lines[k]};
    };
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
}
}
