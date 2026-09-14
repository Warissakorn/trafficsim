#include "routes.hpp"
#include "detail.hpp"

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
    for (const auto& part : parts)
        if (vehicle.distance >= part.start && vehicle.distance - length < part.start + part.length)
            spans.push_back({vehicle.id, part.segmentId, part.segmentIndex,
                std::max(0.0, vehicle.distance - length - part.start),
                std::min(part.length, vehicle.distance - part.start), vehicle.speed});
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
std::vector<VehicleRefs> resolveRefs(const Scenario& scenario, const std::vector<Vehicle>& vehicles) {
    std::vector<VehicleRefs> refs;
    refs.reserve(vehicles.size());
    for (const auto& vehicle : vehicles) {
        const auto& route = detail::byId(scenario.routes, vehicle.routeId);
        const auto& type = detail::byId(scenario.vehicleTypes, vehicle.vehicleTypeId);
        const auto& behaviour = detail::byId(scenario.behaviours, type.behaviourId);
        refs.push_back({static_cast<std::size_t>(&route - scenario.routes.data()),
                        static_cast<std::size_t>(&type - scenario.vehicleTypes.data()),
                        static_cast<std::size_t>(&behaviour - scenario.behaviours.data())});
    }
    return refs;
}
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles,
                                        const ScenarioIndex& index, const std::vector<VehicleRefs>& refs) {
    std::vector<OccupiedSpan> spans;
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
    return locate(routeParts(scenario, detail::byId(scenario.routes, vehicle.routeId)), vehicle);
}
VehicleLocation locateVehicle(const Scenario& scenario, const Vehicle& vehicle, const ScenarioIndex& index) {
    return locate(partsFor(index, scenario, detail::byId(scenario.routes, vehicle.routeId)), vehicle);
}
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles) {
    std::vector<OccupiedSpan> spans;
    for (const auto& vehicle : vehicles)
        appendSpans(spans, routeParts(scenario, detail::byId(scenario.routes, vehicle.routeId)), vehicle,
                    detail::byId(scenario.vehicleTypes, vehicle.vehicleTypeId).length);
    return spans;
}
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles,
                                        const ScenarioIndex& index) {
    std::vector<OccupiedSpan> spans;
    for (const auto& vehicle : vehicles)
        appendSpans(spans, partsFor(index, scenario, detail::byId(scenario.routes, vehicle.routeId)), vehicle,
                    detail::byId(scenario.vehicleTypes, vehicle.vehicleTypeId).length);
    return spans;
}
}
