#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace trafficsim {
// M5.5 (D134, docs/reference/LOS.md): a level-of-service pack is content (rule 5) -- per control
// type, the upper delay bounds of A-E in seconds; above the last is F. Applied to simulated
// section delay only: NOT HCM control delay, not validated (rule 4).
struct LosPack {
    std::string id;
    std::map<std::string, std::array<double, 5>> bounds; // "signalised", "unsignalised"
    bool operator==(const LosPack&) const = default;
};
// The first bound the delay does not exceed (equal takes the better letter), else 'F'. No letter
// for a control type the pack does not have.
std::optional<char> losLetter(double delay, const std::string& controlType, const LosPack&);
// One section's figures, from a single run or a batch mean.
struct LosInput {
    std::string approach; std::optional<std::string> controlType;
    double vehicles{}; std::optional<double> delay;
};
// A volume-weighted group: an approach (start Link and control type) or an intersection (one
// control type). Sections without a type or a delay are left out; no vehicles means no delay.
struct LosGroup {
    std::string name, controlType; double vehicles{};
    std::optional<double> delay; std::optional<char> los;
    bool operator==(const LosGroup&) const = default;
};
struct LosGroups { std::vector<LosGroup> approaches, intersections; };
// Groups in first-appearance order of the sections.
LosGroups losGroups(const std::vector<LosInput>&, const LosPack&);
}
