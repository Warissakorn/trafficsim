#include "los.hpp"
#include <algorithm>

namespace trafficsim {
std::optional<char> losLetter(double delay, const std::string& controlType, const LosPack& pack) {
    const auto it = pack.bounds.find(controlType);
    if (it == pack.bounds.end()) return std::nullopt;
    for (std::size_t i = 0; i < it->second.size(); ++i)
        if (delay <= it->second[i]) return static_cast<char>('A' + i);
    return 'F';
}
LosGroups losGroups(const std::vector<LosInput>& sections, const LosPack& pack) {
    LosGroups out;
    struct Sum { double vehicles{}, weighted{}; };
    const auto add = [&](std::vector<LosGroup>& groups, std::vector<Sum>& sums, const std::string& name, const LosInput& s) {
        auto it = std::find_if(groups.begin(), groups.end(), [&](const auto& g) { return g.name == name && g.controlType == *s.controlType; });
        if (it == groups.end()) { groups.push_back({name, *s.controlType, 0, {}, {}}); sums.emplace_back(); it = groups.end() - 1; }
        auto& sum = sums[static_cast<std::size_t>(it - groups.begin())];
        if (s.delay) { sum.vehicles += s.vehicles; sum.weighted += s.vehicles * *s.delay; }
    };
    std::vector<Sum> approachSums, intersectionSums;
    for (const auto& s : sections) {
        if (!s.controlType) continue;
        add(out.approaches, approachSums, s.approach, s);
        add(out.intersections, intersectionSums, *s.controlType, s);
    }
    const auto finish = [&](std::vector<LosGroup>& groups, const std::vector<Sum>& sums) {
        for (std::size_t i = 0; i < groups.size(); ++i) {
            groups[i].vehicles = sums[i].vehicles;
            if (sums[i].vehicles > 0) {
                groups[i].delay = sums[i].weighted / sums[i].vehicles;
                groups[i].los = losLetter(*groups[i].delay, groups[i].controlType, pack);
            }
        }
    };
    finish(out.approaches, approachSums); finish(out.intersections, intersectionSums);
    return out;
}
}
