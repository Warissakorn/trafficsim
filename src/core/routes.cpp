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
            spans.push_back({vehicle.id, part.segmentId,
                std::max(0.0, vehicle.distance - length - part.start),
                std::min(part.length, vehicle.distance - part.start), vehicle.speed});
}
}
std::vector<RoutePart> routeParts(const Scenario& scenario, const Route& route) {
    double start = 0;
    std::vector<RoutePart> parts;
    for (const auto& id : route.segmentIds) {
        const double length = detail::byId(scenario.segments, id).length;
        parts.push_back({id, start, length});
        start += length;
    }
    return parts;
}
ScenarioIndex buildScenarioIndex(const Scenario& scenario) {
    ScenarioIndex index;
    index.parts.reserve(scenario.routes.size());
    for (const auto& route : scenario.routes) index.parts.push_back(routeParts(scenario, route));
    return index;
}
const std::vector<RoutePart>& partsFor(const ScenarioIndex& index, const Scenario& scenario, const Route& route) {
    const auto offset = static_cast<std::size_t>(&route - scenario.routes.data());
    if (offset >= index.parts.size()) throw std::logic_error("Route does not belong to this scenario");
    return index.parts[offset];
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
