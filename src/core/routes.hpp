#pragma once
#include "types.hpp"

namespace trafficsim {
struct VehicleLocation { std::string segmentId; double position{}; };
// The segment is carried as an index into Scenario::segments, not as a string: a span is
// rebuilt for every vehicle on every tick, and copying the id was the single largest
// remaining cost in the step loop. Resolve the name via scenario.segments[segmentIndex].id.
struct OccupiedSpan {
    std::uint64_t vehicleId{}; std::size_t segmentIndex{};
    double rear{}, front{}, speed{};
};
std::vector<RoutePart> routeParts(const Scenario& scenario, const Route& route);
// Resolved once per run; parts[i] corresponds to scenario.routes[i].
ScenarioIndex buildScenarioIndex(const Scenario& scenario);
// O(1): routes live in a contiguous vector, so the route's own address gives its index.
const std::vector<RoutePart>& partsFor(const ScenarioIndex& index, const Scenario& scenario, const Route& route);
// Callers that already hold the route's parts avoid a route lookup entirely.
VehicleLocation locateOnParts(const std::vector<RoutePart>& parts, const Vehicle& vehicle);
std::vector<VehicleRefs> resolveRefs(const Scenario& scenario, const std::vector<Vehicle>& vehicles);
VehicleLocation locateVehicle(const Scenario& scenario, const Vehicle& vehicle);
VehicleLocation locateVehicle(const Scenario& scenario, const Vehicle& vehicle, const ScenarioIndex& index);
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles);
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles,
                                        const ScenarioIndex& index);
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles,
                                        const ScenarioIndex& index, const std::vector<VehicleRefs>& refs);
// Appends exactly the spans occupiedSpans would have produced for this one vehicle, so a caller
// that adds vehicles one at a time ends up with a byte-identical span list.
void appendVehicleSpans(std::vector<OccupiedSpan>& spans, const Scenario& scenario,
                        const ScenarioIndex& index, const Vehicle& vehicle, const VehicleRefs& refs);
}
