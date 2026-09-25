#pragma once
// The M2.6 study template the owner asked for (2026-09-25): the four-leg drawing with right-turn
// pockets, the Thai left turn at all times, the owner's own timing plan, and the demand shaped the
// way a gate study is -- one routeless input per approach over four counted 15-minute intervals,
// split by a routing decision on its entry Link with per-interval turning counts (M2.1.1/M2.1.2).
//
// It is NOT the gate study (ROADMAP §M2): C1 and C2 need the owner to build a real study from
// blank without help. The VOLUMES ARE PLACEHOLDERS -- the four-leg fixture's round numbers,
// repeated in every interval -- to be replaced by the owner's counts in the input and decision
// dialogs. There is no aerial image; import and calibrate one before C4 is read against it.
//
// Committed as data/projects/m2.6-study-template.traffic.json; regenerate with
// trafficsim-m26-study, never by hand. `m26study` fails when the file and this builder differ.
#include "four_leg_network.hpp"

namespace trafficsim::fixture {
inline ProjectDocument m26StudyTemplate() {
    FourLegOptions options;
    options.leftBypass = 6;  // the left turn leaves the kerb lane 6 m before its stop line
    auto built = fourLegIntersection(options);
    auto& d = built.document;
    d.network.id = "m2.6-study-template";
    for (const auto& route : built.routes) deleteRoute(d, route);  // cascades the fixture inputs

    // The owner's timing plan, as drawn in network.traffic.json (2026-09-25): cycle 120 s, four
    // windows 0-24, 28-54, 58-79 and 83-116, 3 s amber, 1 s all-red. Which approach gets which
    // window is this template's guess -- the longest green to the busiest placeholder approach --
    // and the owner re-times each group in the dialog.
    auto controller = demand(d).signalControllers.at(0);
    controller.name = "Owner timing plan (2026-09-25)";
    // West, East, South, North.
    const std::array<std::array<double, 2>, 4> green{{{83, 116}, {28, 54}, {0, 24}, {58, 79}}};
    for (std::size_t k = 0; k < 4; ++k) {
        controller.groups[k].greenStart = green[k][0];
        controller.groups[k].greenEnd = green[k][1];
    }
    putSignalController(d, controller);

    const auto named = [&](const std::string& name) {
        for (const auto& l : d.network.links) if (l.name == name) return l.id;
        throw std::invalid_argument("UNKNOWN_LINK");
    };
    const std::array<const char*, 4> names{"West", "East", "South", "North"};
    const std::array<std::size_t, 4> through{1, 0, 3, 2}, left{3, 2, 0, 1}, right{2, 3, 1, 0};
    // PLACEHOLDER counts, veh/h per movement (through, left, right): the fixture's numbers.
    const std::array<std::array<double, 3>, 4> volume{{{500, 120, 100}, {450, 110, 90},
                                                      {250, 60, 50}, {220, 50, 40}}};
    constexpr double period = 900;
    constexpr int intervals = 4;
    std::vector<DecisionInterval> slots;
    for (int i = 0; i < intervals; ++i) slots.push_back({i * period, (i + 1) * period});
    for (std::size_t k = 0; k < 4; ++k) {
        const std::string name = names[k];
        const auto entry = named(name + " approach");
        RoutingDecision decision{"", name + " turning counts (PLACEHOLDER)", {}, entry, slots};
        const std::array<std::size_t, 3> to{through[k], left[k], right[k]};
        double total = 0;
        for (std::size_t m = 0; m < 3; ++m) {
            decision.routes.push_back({"", volume[k][m], named(std::string(names[to[m]]) + " exit"),
                                       std::vector<double>(intervals, volume[k][m])});
            total += volume[k][m];
        }
        putRoutingDecision(d, decision);
        VehicleInput input{"", "", "car", total, 0, intervals * period, {}};
        for (const auto& slot : slots) input.intervals.push_back({slot.startTime, slot.endTime, total});
        input.compositionId = "urban-mixed";
        input.linkId = entry;
        putInput(d, input);
    }
    changeRunSettings(d, intervals * period, 0.1);
    return d;
}
}
