#pragma once
#include "types.hpp"
#include <functional>
#include <utility>

namespace trafficsim {
SimState createSimulation(const Scenario& scenario, std::uint32_t seed);
SimState stepSimulation(const SimState& state);
SimState stepSimulation(const SimState& state, double dt);
// The same step, taking the previous state over instead of copying it: `s = stepSimulation(std::move(s))`.
SimState stepSimulation(SimState&& state);
SimState stepSimulation(SimState&& state, double dt);
using EventSink = std::function<void(const SimEvent&)>;
// Synchronous streaming callback; no retained trajectory history or I/O in core.
SimState runSimulation(const Scenario& scenario, std::uint32_t seed,
                       const EventSink& sink = {}, bool includeMovementEvents = true);
std::uint64_t totalTicks(const Scenario& scenario);
SignalColor signalColorAt(const SignalProgram& program, double time);
}
