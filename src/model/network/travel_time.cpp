#include "travel_time.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace trafficsim {
const char* sectionControlName(SectionControl c) { return c == SectionControl::signalised ? "signalised" : "unsignalised"; }
SectionControl sectionControlFromName(const std::string& name) {
    if (name == "signalised") return SectionControl::signalised;
    if (name == "unsignalised") return SectionControl::unsignalised;
    throw std::invalid_argument("INVALID_ENUM");
}
std::vector<ValidationIssue> travelTimeSectionIssues(const Network& n) {
    std::vector<ValidationIssue> issues;
    if (n.travelTimeSections.empty()) return issues;
    const auto add = [&](const char* code, const std::string& path) { issues.push_back({code, path}); };
    // Every other network id, so a section cannot take one (the same set the controls check).
    std::set<std::string> ids{n.id};
    for (const auto& l : n.links) { ids.insert(l.id); for (const auto& lane : l.lanes) ids.insert(lane.id); }
    for (const auto& c : n.connectors) {
        ids.insert(c.id);
        for (int i = 0; i < std::max(c.fromLaneCount, c.toLaneCount); ++i) ids.insert(connectorPathId(c, i));
    }
    for (const auto& h : n.signalHeads) ids.insert(h.id);
    const auto& row = n.rightOfWay;
    for (const auto& w : row.waitingLines) ids.insert(w.id);
    for (const auto& a : row.conflictAreas) ids.insert(a.id);
    for (const auto& r : row.priorityRules) ids.insert(r.id);
    for (const auto& c : row.stopControls) ids.insert(c.id);
    for (const auto& c : n.queueCounters) ids.insert(c.id);
    const auto linkExists = [&](const std::string& id) {
        return std::any_of(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; });
    };
    for (std::size_t i = 0; i < n.travelTimeSections.size(); ++i) {
        const auto& s = n.travelTimeSections[i];
        const auto path = "travelTimeSections[" + std::to_string(i) + "]";
        if (s.id.find_first_not_of(" \t\n\r") == std::string::npos) add("INVALID_ID", path + ".id");
        else if (!ids.insert(s.id).second) add("DUPLICATE_ID", path + ".id");
        for (const auto& [line, name] : {std::pair{&s.start, ".start"}, std::pair{&s.end, ".end"}}) {
            if (!linkExists(line->linkId)) add("UNKNOWN_SECTION_LINK", path + name + ".linkId");
            if (!std::isfinite(line->station) || line->station < 0) add("INVALID_POSITION", path + name + ".station");
        }
        if (s.start.linkId == s.end.linkId && !(s.start.station < s.end.station)) add("INVALID_SECTION_ORDER", path);
    }
    return issues;
}
std::vector<ControlLocation> locateSectionLine(const Network& n, const RuntimeSections& table, const SectionLine& line) {
    std::vector<ControlLocation> places;
    for (const auto& link : n.links) {
        if (link.id != line.linkId) continue;
        for (const auto& lane : link.lanes)
            if (const auto at = locateControlPoint(n, table, {{link.id, lane.id, {}, {}, {}}, line.station})) places.push_back(*at);
    }
    return places;
}
}
