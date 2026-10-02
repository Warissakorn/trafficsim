#pragma once
// D98: one run of a project with the D95 fields set on the compiled scenario, not in a catalog
// copy -- the index reads them from `scenario.behaviours` when the simulation is created, so this
// is the run a data directory carrying them would make. Shared by trafficsim-lane-change-sweep
// and `lanelab`. Development evidence only: no calibration, no M6 validity.
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include "../src/core/simulation.hpp"
#include <optional>

namespace trafficsim::sweep {
struct LaneChangeRun { MovementReport movements; LaneChangeReport changes; };
// `threshold` absent leaves the behaviours as the catalog compiled them.
inline LaneChangeRun runLaneChanges(const ProjectDocument& document, RunSnapshot snapshot, const std::filesystem::path& data,
                                    std::uint32_t seed, std::optional<double> threshold, double accepted = 1,
                                    std::optional<double> hold = std::nullopt) {
    if (threshold)
        for (auto& b : snapshot.scenario.behaviours) {
            b.discretionaryLaneChangeThreshold = threshold;
            b.acceptedDecelerationTrailingVehicle = accepted;
            b.discretionaryLaneChangeHoldTime = hold; // D101; absent, no hold
        }
    const auto spec = evaluationSpec(document, snapshot, data);
    MovementAccumulator m(spec);
    LaneChangeAccumulator c(spec);
    auto s = createSimulation(snapshot.scenario, seed);
    m.observe(s); c.observe(s);
    const auto ticks = totalTicks(snapshot.scenario);
    while (s.tick < ticks) { s = stepSimulation(std::move(s)); m.observe(s); c.observe(s); }
    return {m.report(s), c.report()};
}
}
