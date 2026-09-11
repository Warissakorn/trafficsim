#include "routes.hpp"
#include "detail.hpp"

namespace trafficsim {
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
VehicleLocation locateVehicle(const Scenario& scenario, const Vehicle& vehicle) {
    const auto parts = routeParts(scenario, detail::byId(scenario.routes, vehicle.routeId));
    for (const auto& part : parts)
        if (vehicle.distance < part.start + part.length)
            return {part.segmentId, std::clamp(vehicle.distance - part.start, 0.0, part.length)};
    return {parts.back().segmentId, parts.back().length};
}
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles) {
    std::vector<OccupiedSpan> spans;
    for (const auto& vehicle : vehicles) {
        const double length = detail::byId(scenario.vehicleTypes, vehicle.vehicleTypeId).length;
        for (const auto& part : routeParts(scenario, detail::byId(scenario.routes, vehicle.routeId))) {
            if (vehicle.distance >= part.start && vehicle.distance - length < part.start + part.length)
                spans.push_back({vehicle.id, part.segmentId,
                    std::max(0.0, vehicle.distance - length - part.start),
                    std::min(part.length, vehicle.distance - part.start), vehicle.speed});
        }
    }
    return spans;
}
}
