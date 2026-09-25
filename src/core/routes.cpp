#include "routes.hpp"
#include "conflicts.hpp"
#include "detail.hpp"
#include <cstdint>

namespace trafficsim {
namespace {
// Shared by both overloads so the cached and uncached paths cannot diverge.
VehicleLocation locate(const std::vector<RoutePart>& parts, const Vehicle& vehicle) {
    for (const auto& part : parts)
        if (vehicle.distance < part.start + part.length)
            return {part.segmentId, std::clamp(vehicle.distance - part.start, 0.0, part.length)};
    return {parts.back().segmentId, parts.back().length};
}
void appendSpans(std::vector<OccupiedSpan>& spans, const std::vector<RoutePart>& parts,
                 const Vehicle& vehicle, double length) {
    // Parts are contiguous and ordered by station, so the ones a vehicle covers are a single
    // run: from the first whose end is past the rear bumper, through the last whose start the
    // front has reached. A corridor route crosses twenty-five segments and a vehicle occupies
    // one or two, so the scan this replaces read the whole route for every vehicle every tick.
    // Both comparisons are written exactly as the scan wrote them -- the same subtraction, the
    // same operand order -- so the same parts match in the same order and the spans are
    // identical, which is what replay depends on.
    const double rear = vehicle.distance - length;
    const auto first = std::lower_bound(parts.begin(), parts.end(), rear,
        [](const RoutePart& part, double at) { return !(at < part.start + part.length); });
    for (auto part = first; part != parts.end() && vehicle.distance >= part->start; ++part)
        spans.push_back({vehicle.id, part->segmentIndex,
            std::max(0.0, rear - part->start),
            std::min(part->length, vehicle.distance - part->start), vehicle.speed});
}
}
std::vector<RoutePart> routeParts(const Scenario& scenario, const Route& route) {
    double start = 0;
    std::vector<RoutePart> parts;
    for (const auto& id : route.segmentIds) {
        const auto& segment = detail::byId(scenario.segments, id);
        // Segments are contiguous, so the segment's own address gives its index.
        parts.push_back({id, static_cast<std::size_t>(&segment - scenario.segments.data()),
                         start, segment.length});
        start += segment.length;
    }
    return parts;
}
ScenarioIndex buildScenarioIndex(const Scenario& scenario) {
    ScenarioIndex index;
    index.parts.reserve(scenario.routes.size());
    for (const auto& route : scenario.routes) index.parts.push_back(routeParts(scenario, route));
    index.programOfHead.reserve(scenario.signalHeads.size());
    for (const auto& head : scenario.signalHeads) {
        const auto& program = detail::byId(scenario.signalPrograms, head.programId);
        index.programOfHead.push_back(static_cast<std::size_t>(&program - scenario.signalPrograms.data()));
    }
    index.routeHeads.resize(scenario.routes.size());
    for (std::size_t r = 0; r < scenario.routes.size(); ++r)
        // signalHeads order is preserved, and only the first matching part is recorded, so the
        // per-vehicle find_if this replaces sees an identical sequence of heads and stations.
        for (std::size_t h = 0; h < scenario.signalHeads.size(); ++h) {
            const auto& parts = index.parts[r];
            const auto part = std::find_if(parts.begin(), parts.end(),
                [&](const auto& item) { return item.segmentId == scenario.signalHeads[h].segmentId; });
            if (part != parts.end()) index.routeHeads[r].push_back({h, part->start});
        }
    // Rule incidence, resolved once per scenario for the same reason heads are: the per-vehicle
    // loop must not search by id once per tick.
    index.conflictSegmentOfRule.reserve(scenario.priorityRules.size());
    for (const auto& rule : scenario.priorityRules) {
        const auto found = std::find_if(scenario.segments.begin(), scenario.segments.end(),
            [&](const auto& s) { return s.id == rule.conflictSegmentId; });
        index.conflictSegmentOfRule.push_back(found == scenario.segments.end() ? SIZE_MAX :
            static_cast<std::size_t>(found - scenario.segments.begin()));
    }
    index.behaviourOfType.reserve(scenario.vehicleTypes.size());
    for (const auto& type : scenario.vehicleTypes) {
        const auto& behaviour = detail::byId(scenario.behaviours, type.behaviourId);
        index.behaviourOfType.push_back(static_cast<std::size_t>(&behaviour - scenario.behaviours.data()));
    }
    // Through byId, so a slot names the element the id named -- its first occurrence.
    index.routeOfInput.reserve(scenario.inputs.size());
    index.typeOfInput.reserve(scenario.inputs.size());
    for (const auto& input : scenario.inputs) {
        const auto& route = detail::byId(scenario.routes, input.routeId);
        const auto& type = detail::byId(scenario.vehicleTypes, input.vehicleTypeId);
        index.routeOfInput.push_back(static_cast<std::uint32_t>(&route - scenario.routes.data()));
        index.typeOfInput.push_back(static_cast<std::uint32_t>(&type - scenario.vehicleTypes.data()));
    }
    index.routeRules.resize(scenario.routes.size());
    for (std::size_t r = 0; r < scenario.routes.size(); ++r)
        for (std::size_t k = 0; k < scenario.priorityRules.size(); ++k) {
            const auto& parts = index.parts[r];
            const auto part = std::find_if(parts.begin(), parts.end(),
                [&](const auto& item) { return item.segmentId == scenario.priorityRules[k].yieldSegmentId; });
            if (part != parts.end()) index.routeRules[r].push_back({k, part->start});
        }
    // Zone incidence (M3.2.3a/b): every route through either side, in route distances, so a
    // major vehicle is seen on its whole approach and a chain is followed across section cuts.
    index.routeZones.reserve(scenario.routes.size());
    for (std::size_t r = 0; r < scenario.routes.size(); ++r) index.routeZones.push_back(zoneIncidence(scenario, index.parts[r]));
    index.stopZones = std::any_of(scenario.conflictZones.begin(), scenario.conflictZones.end(),
                                  [](const auto& z) { return z.control == ZoneControl::stop; });
    return index;
}
const std::vector<RoutePart>& partsFor(const ScenarioIndex& index, const Scenario& scenario, const Route& route) {
    const auto offset = static_cast<std::size_t>(&route - scenario.routes.data());
    if (offset >= index.parts.size()) throw std::logic_error("Route does not belong to this scenario");
    return index.parts[offset];
}
VehicleLocation locateOnParts(const std::vector<RoutePart>& parts, const Vehicle& vehicle) {
    return locate(parts, vehicle);
}
std::vector<VehicleRefs> resolveRefs(const Scenario& scenario, const std::vector<Vehicle>& vehicles,
                                     const ScenarioIndex& index) {
    (void)scenario; // A vehicle carries its own slots; only the behaviour is still derived.
    std::vector<VehicleRefs> refs;
    refs.reserve(vehicles.size());
    for (const auto& vehicle : vehicles)
        refs.push_back({vehicle.routeIndex, vehicle.typeIndex, index.behaviourOfType[vehicle.typeIndex]});
    return refs;
}
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles,
                                        const ScenarioIndex& index, const std::vector<VehicleRefs>& refs) {
    std::vector<OccupiedSpan> spans;
    // Most vehicles sit inside one segment, so this is the whole answer for most ticks and a
    // couple of growth steps for the rest. The vector is rebuilt from empty every tick.
    spans.reserve(vehicles.size());
    for (std::size_t i = 0; i < vehicles.size(); ++i)
        appendSpans(spans, index.parts[refs[i].route], vehicles[i],
                    scenario.vehicleTypes[refs[i].type].length);
    return spans;
}
void appendVehicleSpans(std::vector<OccupiedSpan>& spans, const Scenario& scenario,
                        const ScenarioIndex& index, const Vehicle& vehicle, const VehicleRefs& refs) {
    appendSpans(spans, index.parts[refs.route], vehicle, scenario.vehicleTypes[refs.type].length);
}
VehicleLocation locateVehicle(const Scenario& scenario, const Vehicle& vehicle) {
    return locate(routeParts(scenario, scenario.routes[vehicle.routeIndex]), vehicle);
}
VehicleLocation locateVehicle(const Scenario& scenario, const Vehicle& vehicle, const ScenarioIndex& index) {
    return locate(partsFor(index, scenario, scenario.routes[vehicle.routeIndex]), vehicle);
}
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles) {
    std::vector<OccupiedSpan> spans;
    for (const auto& vehicle : vehicles)
        appendSpans(spans, routeParts(scenario, scenario.routes[vehicle.routeIndex]), vehicle,
                    scenario.vehicleTypes[vehicle.typeIndex].length);
    return spans;
}
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles,
                                        const ScenarioIndex& index) {
    std::vector<OccupiedSpan> spans;
    for (const auto& vehicle : vehicles)
        appendSpans(spans, partsFor(index, scenario, scenario.routes[vehicle.routeIndex]), vehicle,
                    scenario.vehicleTypes[vehicle.typeIndex].length);
    return spans;
}
}
