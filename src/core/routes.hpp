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
// Each vehicle's route/type slots and its effectiveBehaviour at this snapshot (M3.3.2b). Called
// again wherever a tick rebuilds references -- after source-zero routing and accepted lane
// changes -- so a new road's behaviour is used from the next snapshot that sees the front there.
std::vector<VehicleRefs> resolveRefs(const Scenario& scenario, const std::vector<Vehicle>& vehicles,
                                     const ScenarioIndex& index);
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
// Spans grouped by segment, flat (CSR) so grouping costs three allocations, not one per segment.
// items keeps each segment's spans in their original relative order, which is what makes a
// strictly-less-than tie-break select exactly the same span as a full scan would.
struct SpanBuckets {
    std::vector<std::uint32_t> start, items;
};
SpanBuckets bucketSpans(const std::vector<OccupiedSpan>& spans, std::size_t segmentCount);
}
