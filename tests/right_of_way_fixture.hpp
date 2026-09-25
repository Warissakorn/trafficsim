#pragma once
// Shared by the right-of-way test files: small networks and lookups, one copy (hard rule 3).
#include "../tools/four_leg_network.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/model/network/right_of_way.hpp"
#include <algorithm>
#include <stdexcept>

namespace rowfixture {
using namespace trafficsim;
inline const PriorityDefaults kDefaults{3, 7};
inline bool has(const std::vector<ValidationIssue>& issues, const std::string& code) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code; });
}
inline int count(const std::vector<ValidationIssue>& issues, const std::string& code) {
    return static_cast<int>(std::count_if(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code; }));
}
inline RightOfWayResolution resolve(const ProjectDocument& d, PriorityDefaults defaults = kDefaults) {
    return resolveRightOfWay(d.network, runtimeSections(d.network), defaults);
}
inline int rulesWithPrefix(const RightOfWayResolution& r, const std::string& prefix) {
    return static_cast<int>(std::count_if(r.rules.begin(), r.rules.end(),
                                          [&](const auto& x) { return x.id.rfind(prefix, 0) == 0; }));
}
// Three single-lane Links ending short of X, each joined to X's lane start: one merge of three.
struct Three { ProjectDocument d; std::string x; };
inline Three threeWayMerge() {
    Three t;
    t.x = addLink(t.d, {{0, 0}, {100, 0}}, 1, 3.5);
    const auto xLane = editableLink(t.d, t.x).lanes[0].id;
    for (const double y : {0.0, 40.0, -40.0}) {
        const auto a = addLink(t.d, {{-100, y}, {-20, y * 0.25}}, 1, 3.5);
        addConnector(t.d, {a, editableLink(t.d, a).lanes[0].id}, {t.x, xLane});
    }
    return t;
}
inline std::string mergeAt(const ProjectDocument& d, std::size_t members) {
    for (const auto& g : mergeGroups(d.network, runtimeSections(d.network)))
        if (g.incoming.size() == members) return g.section;
    throw std::runtime_error("no merge of that size");
}
}
