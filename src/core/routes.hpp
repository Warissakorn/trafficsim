#pragma once
#include "types.hpp"

namespace trafficsim {
struct VehicleLocation { std::string segmentId; double position{}; };
struct OccupiedSpan {
    std::uint64_t vehicleId{}; std::string segmentId; double rear{}, front{}, speed{};
};
std::vector<RoutePart> routeParts(const Scenario& scenario, const Route& route);
// Resolved once per run; parts[i] corresponds to scenario.routes[i].
ScenarioIndex buildScenarioIndex(const Scenario& scenario);
// O(1): routes live in a contiguous vector, so the route's own address gives its index.
const std::vector<RoutePart>& partsFor(const ScenarioIndex& index, const Scenario& scenario, const Route& route);
VehicleLocation locateVehicle(const Scenario& scenario, const Vehicle& vehicle);
VehicleLocation locateVehicle(const Scenario& scenario, const Vehicle& vehicle, const ScenarioIndex& index);
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles);
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles,
                                        const ScenarioIndex& index);
}
