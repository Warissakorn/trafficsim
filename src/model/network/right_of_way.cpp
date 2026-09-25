#include "right_of_way.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <tuple>

namespace trafficsim {
const char* conflictKindName(ConflictKind kind) { return kind == ConflictKind::crossing ? "crossing" : "merge"; }
const char* conflictPriorityName(ConflictPriority priority) {
    switch (priority) {
    case ConflictPriority::firstYields: return "firstYields";
    case ConflictPriority::secondYields: return "secondYields";
    case ConflictPriority::undetermined: return "undetermined";
    }
    return "undetermined";
}
ConflictKind conflictKindFromName(const std::string& name) {
    if (name == "crossing") return ConflictKind::crossing;
    if (name == "merge") return ConflictKind::merge;
    throw std::invalid_argument("INVALID_ENUM");
}
ConflictPriority conflictPriorityFromName(const std::string& name) {
    if (name == "firstYields") return ConflictPriority::firstYields;
    if (name == "secondYields") return ConflictPriority::secondYields;
    if (name == "undetermined") return ConflictPriority::undetermined;
    throw std::invalid_argument("INVALID_ENUM");
}
namespace {
const Link* findLink(const Network& n, const std::string& id) {
    for (const auto& l : n.links) if (l.id == id) return &l;
    return nullptr;
}
const Connector* findConnector(const Network& n, const std::string& id) {
    for (const auto& c : n.connectors) if (c.id == id) return &c;
    return nullptr;
}
bool linkKind(const ControlPathRef& p) {
    return !p.linkId.empty() && !p.laneId.empty() && p.connectorId.empty() && p.fromLaneId.empty() && p.toLaneId.empty();
}
bool connectorKind(const ControlPathRef& p) {
    return p.linkId.empty() && p.laneId.empty() && !p.connectorId.empty() && !p.fromLaneId.empty() && !p.toLaneId.empty();
}
// A point on the runtime: the segment and the metres along it. Empty when the reference no longer
// resolves to exactly one path, or the station lies off the object it names.
struct Located { std::string segment; double position{}; double length{}; };
std::optional<Located> locateChecked(const Network& n, const RuntimeSections& table, const ControlPathRef& ref,
                                     double station) {
    if (!std::isfinite(station) || station < 0) return std::nullopt;
    if (linkKind(ref)) {
        const auto* link = findLink(n, ref.linkId);
        if (!link || std::none_of(link->lanes.begin(), link->lanes.end(),
                                  [&](const auto& l) { return l.id == ref.laneId; })) return std::nullopt;
        if (station > polylineLength(link->geometry)) return std::nullopt;
        // Link stations are on the reference polyline; the runtime runs on the lane polyline.
        const auto lane = laneGeometry(*link, ref.laneId, n.drivingSide);
        const double onLane = matchedStation(link->geometry, lane, station);
        const auto& section = sectionForStation(table, ref.laneId, onLane);
        return Located{section.id, onLane - section.start, section.end - section.start};
    }
    if (!connectorKind(ref)) return std::nullopt;
    const auto* connector = findConnector(n, ref.connectorId);
    if (!connector || station > polylineLength(connector->geometry)) return std::nullopt;
    std::vector<const ConnectorPath*> matches;
    for (int i = 0; i < std::max(connector->fromLaneCount, connector->toLaneCount); ++i) {
        const auto id = connectorPathId(*connector, i);
        for (const auto& path : table.paths)
            if (path.id == id && path.from.laneId == ref.fromLaneId && path.to.laneId == ref.toLaneId)
                matches.push_back(&path);
    }
    if (matches.size() != 1) return std::nullopt; // never pick one by ordinal
    const auto& path = *matches.front();
    const double length = polylineLength(path.geometry);
    return Located{path.id, matchedStation(connector->geometry, path.geometry, station), length};
}
// buildScenario is unchecked assembly that must not throw (network.hpp), and the geometry helpers
// throw on degenerate input. A reference that cannot be located is unresolved, never an exception.
std::optional<Located> locate(const Network& n, const RuntimeSections& table, const ControlPathRef& ref,
                              double station) {
    try { return locateChecked(n, table, ref, station); } catch (const std::exception&) { return std::nullopt; }
}
}
std::vector<ValidationIssue> rightOfWayStructuralIssues(const Network& n) {
    std::vector<ValidationIssue> issues;
    const auto& row = n.rightOfWay;
    if (row.empty()) return issues;
    const auto add = [&](const char* code, const std::string& path) { issues.push_back({code, path}); };
    std::set<std::string> ids{n.id};
    for (const auto& l : n.links) { ids.insert(l.id); for (const auto& lane : l.lanes) ids.insert(lane.id); }
    for (const auto& c : n.connectors) {
        ids.insert(c.id);
        for (int i = 0; i < std::max(c.fromLaneCount, c.toLaneCount); ++i) ids.insert(connectorPathId(c, i));
    }
    for (const auto& h : n.signalHeads) ids.insert(h.id);
    const auto id = [&](const std::string& value, const std::string& path) {
        if (value.find_first_not_of(" \t\n\r") == std::string::npos) add("INVALID_ID", path + ".id");
        else if (!ids.insert(value).second) add("DUPLICATE_ID", path + ".id");
    };
    const auto pathRef = [&](const ControlPathRef& ref, const std::string& path) {
        if (linkKind(ref)) { if (!findLink(n, ref.linkId)) add("UNKNOWN_CONTROL_OWNER", path); }
        else if (connectorKind(ref)) { if (!findConnector(n, ref.connectorId)) add("UNKNOWN_CONTROL_OWNER", path); }
        else add("INVALID_CONTROL_PATH", path);
    };
    const auto finite = [](double v) { return std::isfinite(v) && v >= 0; };
    std::set<std::string> lines, areas, ruled;
    for (std::size_t i = 0; i < row.waitingLines.size(); ++i) {
        const auto& w = row.waitingLines[i];
        const auto path = "rightOfWay.waitingLines[" + std::to_string(i) + "]";
        id(w.id, path); lines.insert(w.id);
        pathRef(w.point.path, path + ".point.path");
        if (!finite(w.point.station)) add("INVALID_POSITION", path + ".point.station");
    }
    for (std::size_t i = 0; i < row.conflictAreas.size(); ++i) {
        const auto& a = row.conflictAreas[i];
        const auto path = "rightOfWay.conflictAreas[" + std::to_string(i) + "]";
        id(a.id, path); areas.insert(a.id);
        for (const auto& [side, name] : {std::pair{&a.first, ".first"}, std::pair{&a.second, ".second"}}) {
            pathRef(side->path, path + name + ".path");
            if (!finite(side->entryStation) || !finite(side->exitStation) || side->entryStation >= side->exitStation)
                add("INVALID_CONFLICT_EXTENT", path + name);
            if (!lines.contains(side->waitingLineId)) add("UNKNOWN_WAITING_LINE", path + name + ".waitingLineId");
        }
    }
    for (std::size_t i = 0; i < row.priorityRules.size(); ++i) {
        const auto& r = row.priorityRules[i];
        const auto path = "rightOfWay.priorityRules[" + std::to_string(i) + "]";
        id(r.id, path);
        if (!areas.contains(r.conflictAreaId)) add("UNKNOWN_CONFLICT_AREA", path + ".conflictAreaId");
        else if (!ruled.insert(r.conflictAreaId).second) add("DUPLICATE_PRIORITY_RULE", path + ".conflictAreaId");
        if (!std::isfinite(r.gapTime) || r.gapTime <= 0 || !std::isfinite(r.headway) || r.headway <= 0)
            add("INVALID_PRIORITY_RULE", path);
    }
    return issues;
}
std::string resolveControlPath(const Network& n, const RuntimeSections& table, const ControlPathRef& ref,
                               double station) {
    const auto at = locate(n, table, ref, station);
    return at ? at->segment : std::string{};
}
std::vector<MergeGroup> mergeGroups(const Network& n, const RuntimeSections& table) {
    std::vector<MergeGroup> groups;
    for (const auto& section : table.sections) {
        MergeGroup group{section.id, {}, false};
        // The ranking derivedPriorityRules applies: the lane already carrying traffic first, then
        // arriving paths in drawing order. Each later member gives way to every earlier one.
        if (section.start > 0)
            for (const auto& up : table.sections)
                if (up.laneId == section.laneId && up.end == section.start) group.incoming.push_back(up.id);
        for (std::size_t p = 0; p < table.paths.size(); ++p)
            if (table.pathNext[p] == section.id) group.incoming.push_back(table.paths[p].id);
        if (group.incoming.size() < 2) continue;
        for (const auto& area : n.rightOfWay.conflictAreas) {
            if (area.kind != ConflictKind::merge) continue;
            const auto a = resolveControlPath(n, table, area.first.path, area.first.entryStation);
            const auto b = resolveControlPath(n, table, area.second.path, area.second.entryStation);
            const auto member = [&](const std::string& s) {
                return std::find(group.incoming.begin(), group.incoming.end(), s) != group.incoming.end();
            };
            if (!a.empty() && !b.empty() && a != b && member(a) && member(b)) group.explicitControl = true;
        }
        groups.push_back(std::move(group));
    }
    return groups;
}
namespace {
bool cyclic(const std::vector<std::string>& nodes, const std::multimap<std::string, std::string>& edges) {
    std::map<std::string, int> state; // 0 unseen, 1 on stack, 2 done
    std::function<bool(const std::string&)> visit = [&](const std::string& v) {
        state[v] = 1;
        for (auto [it, end] = edges.equal_range(v); it != end; ++it) {
            if (state[it->second] == 1) return true;
            if (state[it->second] == 0 && visit(it->second)) return true;
        }
        state[v] = 2;
        return false;
    };
    for (const auto& v : nodes) if (state[v] == 0 && visit(v)) return true;
    return false;
}
// The runtime graph read backwards: what feeds each segment, and each segment's length.
struct Upstream { std::map<std::string, std::vector<std::string>> feeding; std::map<std::string, double> length; };
Upstream upstreamOf(const RuntimeSections& table) {
    Upstream u;
    for (const auto& s : table.sections) {
        u.length[s.id] = s.end - s.start;
        for (const auto& next : s.next) u.feeding[next].push_back(s.id);
    }
    for (std::size_t p = 0; p < table.paths.size(); ++p) {
        u.length[table.paths[p].id] = polylineLength(table.paths[p].geometry);
        u.feeding[table.pathNext[p]].push_back(table.paths[p].id);
    }
    return u;
}
// Where a side's waiting line holds a vehicle, in metres along the side's entry segment --
// negative on the approach before it (M3.2.2c). The walk follows single predecessors only: the
// line must be upstream of entry on EVERY route (contract §1), so a line some route can go
// round is a named blocker, never a guess. `problem` is set exactly when `at` is empty.
struct Wait { std::optional<double> at; const char* problem{}; };
Wait waitOn(const Upstream& u, const Located& line, const Located& entry) {
    const auto reaches = [&](const std::string& from) { // is the line anywhere upstream?
        std::set<std::string> seen;
        std::vector<std::string> open{from};
        while (!open.empty()) {
            const auto at = open.back(); open.pop_back();
            if (at == line.segment) return true;
            if (!seen.insert(at).second) continue;
            if (const auto f = u.feeding.find(at); f != u.feeding.end())
                open.insert(open.end(), f->second.begin(), f->second.end());
        }
        return false;
    };
    double before = 0;
    std::set<std::string> seen;
    for (std::string at = entry.segment;;) {
        if (at == line.segment) return {before + line.position, nullptr};
        const auto f = u.feeding.find(at);
        if (f == u.feeding.end() || f->second.empty() || !seen.insert(at).second)
            return {std::nullopt, "CONFLICT_WAITING_LINE_NOT_UPSTREAM"};
        if (f->second.size() > 1)
            return {std::nullopt, reaches(at) ? "CONFLICT_WAITING_LINE_BYPASSED" : "CONFLICT_WAITING_LINE_NOT_UPSTREAM"};
        at = f->second.front();
        before -= u.length.at(at);
    }
}
// A side as the core sees it: the runtime segments from its entry to its exit, in travel order
// -- one for a Connector path, the lane's sections between the two for a Link lane. Empty when
// the exit does not locate or does not follow the entry on the same lane.
std::optional<ZoneSide> sideChain(const Network& n, const RuntimeSections& table, const ConflictSide& side) {
    const auto entry = locate(n, table, side.path, side.entryStation);
    const auto exit = locate(n, table, side.path, side.exitStation);
    if (!entry || !exit) return std::nullopt;
    if (entry->segment == exit->segment) return ZoneSide{{entry->segment}, entry->position, exit->position};
    if (!linkKind(side.path)) return std::nullopt;
    std::vector<std::string> chain;
    for (const auto& s : table.sections) {
        if (s.laneId != side.path.laneId) continue;
        if (s.id == entry->segment || !chain.empty()) chain.push_back(s.id);
        if (s.id == exit->segment) break;
    }
    if (chain.empty() || chain.back() != exit->segment) return std::nullopt;
    return ZoneSide{chain, entry->position, exit->position};
}
}
RightOfWayResolution resolveRightOfWay(const Network& n, const RuntimeSections& table,
                                       const PriorityDefaults& defaults) {
    RightOfWayResolution result;
    const auto derived = derivedPriorityRules(table, defaults);
    const auto& row = n.rightOfWay;
    if (row.conflictAreas.empty()) { result.rules = derived; return result; }
    const auto groups = mergeGroups(n, table);
    const auto groupOf = [&](const std::string& segment) -> const MergeGroup* {
        for (const auto& g : groups)
            if (std::find(g.incoming.begin(), g.incoming.end(), segment) != g.incoming.end()) return &g;
        return nullptr;
    };
    // Policy 1 and 2: a merge nobody overrode keeps its derived rules exactly; an overridden one
    // loses ALL of them, so no reciprocal of an authored decision survives beside it.
    for (const auto& rule : derived) {
        const auto* g = groupOf(rule.yieldSegmentId);
        if (g && g->explicitControl && groupOf(rule.conflictSegmentId) == g) continue;
        result.rules.push_back(rule);
    }
    const auto add = [&](const char* code, const std::string& path) { result.issues.push_back({code, path}); };
    // A line off the end of its path after a reshape is reported, never clamped onto it (§1).
    for (std::size_t i = 0; i < row.waitingLines.size(); ++i)
        if (!locate(n, table, row.waitingLines[i].point.path, row.waitingLines[i].point.station))
            add("CONFLICT_UNRESOLVED_PATH", "rightOfWay.waitingLines[" + std::to_string(i) + "]");
    struct Resolved {
        const ConflictArea* area; std::string path; std::optional<Located> first, second;
        std::optional<double> waitFirst, waitSecond;
        std::optional<ZoneSide> chainFirst, chainSecond; // the sides as the core runs them
    };
    std::vector<Resolved> resolved;
    const auto upstream = upstreamOf(table);
    for (std::size_t i = 0; i < row.conflictAreas.size(); ++i) {
        const auto& a = row.conflictAreas[i];
        const auto path = "rightOfWay.conflictAreas[" + std::to_string(i) + "]";
        const auto issuesBefore = result.issues.size();
        Resolved r{&a, path, locate(n, table, a.first.path, a.first.entryStation),
                   locate(n, table, a.second.path, a.second.entryStation), {}, {}, {}, {}};
        if (!r.first) add("CONFLICT_UNRESOLVED_PATH", path + ".first");
        if (!r.second) add("CONFLICT_UNRESOLVED_PATH", path + ".second");
        if (a.priority == ConflictPriority::undetermined) add("CONFLICT_UNDETERMINED", path);
        else if (std::none_of(row.priorityRules.begin(), row.priorityRules.end(),
                              [&](const auto& rule) { return rule.conflictAreaId == a.id; }))
            add("CONFLICT_RULE_MISSING", path);
        for (const auto& [side, entry, wait, name] :
             {std::tuple{&a.first, &r.first, &r.waitFirst, ".first"}, std::tuple{&a.second, &r.second, &r.waitSecond, ".second"}}) {
            const auto line = std::find_if(row.waitingLines.begin(), row.waitingLines.end(),
                                           [&](const auto& w) { return w.id == side->waitingLineId; });
            // A line or an entry that does not locate was reported above; nothing to measure.
            const auto at = line == row.waitingLines.end() ? std::nullopt
                                                           : locate(n, table, line->point.path, line->point.station);
            if (!at || !*entry) continue;
            const auto held = waitOn(upstream, *at, **entry);
            if (!held.at) add(held.problem, path + name);
            else if (*held.at > (*entry)->position) add("CONFLICT_WAITING_LINE_AFTER_ENTRY", path + name);
            else *wait = held.at;
        }
        if (a.kind == ConflictKind::crossing && r.first && r.second) {
            const auto overlap = surfaceOverlap(n, a.first.path, a.second.path);
            if (overlap.status == SurfaceOverlap::Status::none) add("CONFLICT_NO_OVERLAP", path);
            else if (overlap.status != SurfaceOverlap::Status::overlap) add("CONFLICT_GEOMETRY_UNSUPPORTED", path);
            else {
                // The authored extents must contain the real overlap; a larger area is allowed,
                // a smaller one would admit a vehicle into space another already occupies.
                if (a.first.entryStation > overlap.first.from || a.first.exitStation < overlap.first.to)
                    add("CONFLICT_EXTENT_UNCOVERED", path + ".first");
                if (a.second.entryStation > overlap.second.from || a.second.exitStation < overlap.second.to)
                    add("CONFLICT_EXTENT_UNCOVERED", path + ".second");
            }
        }
        if (r.first && r.second) {
            // Each side as the run of runtime segments from its entry to its exit: an area may lie
            // over a section cut (M3.2.3b).
            r.chainFirst = sideChain(n, table, a.first); r.chainSecond = sideChain(n, table, a.second);
            if (!r.chainFirst) add("CONFLICT_UNRESOLVED_PATH", path + ".first");
            if (!r.chainSecond) add("CONFLICT_UNRESOLVED_PATH", path + ".second");
        }
        if (a.kind == ConflictKind::crossing && r.first && r.second) {
            const auto& chainFirst = r.chainFirst; const auto& chainSecond = r.chainSecond;
            const bool firstYields = a.priority == ConflictPriority::firstYields;
            const auto& wait = firstYields ? r.waitFirst : r.waitSecond;
            const auto rule = std::find_if(row.priorityRules.begin(), row.priorityRules.end(),
                                           [&](const auto& x) { return x.conflictAreaId == a.id; });
            // Only an area with nothing reported against it runs; anything else already blocks.
            if (result.issues.size() == issuesBefore && chainFirst && chainSecond &&
                a.priority != ConflictPriority::undetermined && wait && rule != row.priorityRules.end()) {
                result.zones.push_back({"right-of-way/" + a.id, firstYields ? *chainSecond : *chainFirst,
                                        firstYields ? *chainFirst : *chainSecond, *wait, rule->gapTime, rule->headway});
            }
        }
        if (a.kind == ConflictKind::merge && r.first && r.second &&
            (r.first->segment == r.second->segment || !groupOf(r.first->segment) ||
             groupOf(r.first->segment) != groupOf(r.second->segment)))
            add("CONFLICT_MERGE_TOPOLOGY", path);
        resolved.push_back(std::move(r));
    }
    // Every overridden group must be totally ordered by its authored areas: every pair covered
    // once, every area decided and ruled, and no cycle. Otherwise the whole group is a draft.
    for (const auto& g : groups) {
        if (!g.explicitControl) continue;
        std::vector<const Resolved*> members;
        std::multimap<std::string, std::string> edges;
        std::string first;
        bool complete = true, held = true;
        for (std::size_t i = 0; i < g.incoming.size(); ++i)
            for (std::size_t j = i + 1; j < g.incoming.size(); ++j) {
                int covering = 0;
                for (const auto& r : resolved) {
                    if (r.area->kind != ConflictKind::merge || !r.first || !r.second) continue;
                    const auto& s1 = r.first->segment; const auto& s2 = r.second->segment;
                    if (!((s1 == g.incoming[i] && s2 == g.incoming[j]) || (s1 == g.incoming[j] && s2 == g.incoming[i]))) continue;
                    ++covering; members.push_back(&r);
                    if (first.empty()) first = r.path;
                    if (r.area->priority == ConflictPriority::firstYields) { edges.insert({s1, s2}); held &= r.waitFirst.has_value(); }
                    else if (r.area->priority == ConflictPriority::secondYields) { edges.insert({s2, s1}); held &= r.waitSecond.has_value(); }
                    else complete = false;
                    held &= r.chainFirst.has_value() && r.chainSecond.has_value();
                    if (std::none_of(row.priorityRules.begin(), row.priorityRules.end(),
                                     [&](const auto& rule) { return rule.conflictAreaId == r.area->id; }))
                        complete = false;
                }
                if (covering != 1) complete = false;
                if (covering > 1) add("CONFLICT_DUPLICATE_PAIR", first);
            }
        const bool loops = cyclic(g.incoming, edges);
        if (loops) for (const auto* r : members) add("CONFLICT_PRIORITY_CYCLE", r->path);
        if (!complete && !loops) add("CONFLICT_GROUP_INCOMPLETE", first);
        // A yielding side with no place to wait was reported by name above; compile nothing for it.
        if (!complete || loops || !held) continue;
        // An authored merge runs on the admission solver (M3.2.3c): the minor side waits on the
        // gap-time/headway threshold as before, and now the major side also waits at its entry for
        // a minor vehicle already admitted -- which the drawing-order rule never did.
        for (const auto* r : members) {
            const auto& a = *r->area;
            const bool firstYields = a.priority == ConflictPriority::firstYields;
            const auto rule = std::find_if(row.priorityRules.begin(), row.priorityRules.end(),
                                           [&](const auto& x) { return x.conflictAreaId == a.id; });
            result.zones.push_back({"right-of-way/" + a.id, firstYields ? *r->chainSecond : *r->chainFirst,
                                    firstYields ? *r->chainFirst : *r->chainSecond,
                                    firstYields ? *r->waitFirst : *r->waitSecond, rule->gapTime, rule->headway});
        }
    }
    return result;
}
}
