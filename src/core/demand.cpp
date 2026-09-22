#include "detail.hpp"
#include <cmath>
#include <numbers>

namespace trafficsim::detail {
double random(std::uint32_t& state) {
    // Unsigned shifts and overflow intentionally match xorshift32 in the TS baseline.
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (static_cast<double>(state) + 0.5) / 4294967296.0;
}
void initializeInputs(SimState& state) {
    for (const auto& input : state.scenario->inputs) {
        InputState current{std::nullopt, {}};
        if (input.vehiclesPerHour != 0) {
            const double arrival = input.startTime - std::log(random(state.randomState)) *
                                                    3600 / input.vehiclesPerHour;
            if (arrival < input.endTime) current.nextArrival = arrival;
        }
        state.inputs.push_back(std::move(current));
    }
}
void generateArrivals(SimState& state) {
    // state.inputs is parallel to scenario.inputs, so the input is the one at this position --
    // no id lookup, and the slot a released vehicle carries is this index.
    for (std::size_t i = 0; i < state.inputs.size(); ++i) {
        auto& current = state.inputs[i];
        const auto& input = state.scenario->inputs[i];
        // Resolved once per scenario in the index, not once per input per tick.
        const auto routeIndex = state.index->routeOfInput[i];
        const auto typeIndex = state.index->typeOfInput[i];
        const auto& type = state.scenario->vehicleTypes[typeIndex];
        while (current.nextArrival && *current.nextArrival <= state.time) {
            const double desiredSpeed = type.desiredSpeed.min + random(state.randomState) *
                                       (type.desiredSpeed.max - type.desiredSpeed.min);
            // Separate statements specify draw order; C++ operand evaluation order must
            // never choose which draw goes to the logarithm versus cosine.
            const double radiusDraw = random(state.randomState);
            const double angleDraw = random(state.randomState);
            const double gaussian = std::sqrt(-2 * std::log(radiusDraw)) *
                                    std::cos(2 * std::numbers::pi * angleDraw);
            current.queue.push_back({state.nextVehicleId++, static_cast<std::uint32_t>(i),
                routeIndex, typeIndex, *current.nextArrival, desiredSpeed,
                std::clamp(0.5 + 0.15 * gaussian, 0.0, 1.0)});
            const double arrival = *current.nextArrival - std::log(random(state.randomState)) *
                                                         3600 / input.vehiclesPerHour;
            current.nextArrival = arrival < input.endTime ? std::optional(arrival) : std::nullopt;
        }
    }
}
}
