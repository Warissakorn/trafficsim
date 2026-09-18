#include "routes.hpp"
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
    for (const auto& part : parts)
        if (vehicle.distance >= part.start && vehicle.distance - length < part.start + part.length)
            spans.push_back({vehicle.id, part.segmentIndex,
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
    // Rule incidence, resolved once per scenario for the same reason heads are: the per-vehicle
    // loop must not search by id once per tick.
    index.conflictSegmentOfRule.reserve(scenario.priorityRules.size());
    for (const auto& rule : scenario.priorityRules) {
        const auto found = std::find_if(scenario.segments.begin(), scenario.segments.end(),
            [&](const auto& s) { return s.id == rule.conflictSegmentId; });
        index.conflictSegmentOfRule.push_back(found == scenario.segments.end() ? SIZE_MAX :
            static_cast<std::size_t>(found - scenario.segments.begin()));
    }
    // Sorting by (id, position) puts a repeated id's FIRST occurrence at the front of its equal
    // range, so the lower_bound below returns what byId's find_if returned.
    const auto buildTable = [](const auto& items) {
        std::vector<IdSlot> table;
        table.reserve(items.size());
        for (std::size_t i = 0; i < items.size(); ++i) table.push_back({items[i].id, i});
        std::sort(table.begin(), table.end(), [](const IdSlot& a, const IdSlot& b) {
            return a.id == b.id ? a.index < b.index : a.id < b.id;
        });
        return table;
    };
    index.routeOfId = buildTable(scenario.routes);
    index.typeOfId = buildTable(scenario.vehicleTypes);
    index.behaviourOfType.reserve(scenario.vehicleTypes.size());
    for (const auto& type : scenario.vehicleTypes) {
        const auto& behaviour = detail::byId(scenario.behaviours, type.behaviourId);
        index.behaviourOfType.push_back(static_cast<std::size_t>(&behaviour - scenario.behaviours.data()));
    }
    index.routeRules.resize(scenario.routes.size());
    for (std::size_t r = 0; r < scenario.routes.size(); ++r)
        for (std::size_t k = 0; k < scenario.priorityRules.size(); ++k) {
            const auto& parts = index.parts[r];
            const auto part = std::find_if(parts.begin(), parts.end(),
                [&](const auto& item) { return item.segmentId == scenario.priorityRules[k].yieldSegmentId; });
            if (part != parts.end()) index.routeRules[r].push_back({k, part->start});
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
namespace {
// Same miss as byId, so an unknown id still fails the same way and with the same message.
std::size_t lookup(const std::vector<IdSlot>& table, const std::string& id) {
    const auto found = std::lower_bound(table.begin(), table.end(), id,
        [](const IdSlot& slot, const std::string& key) { return slot.id < key; });
    if (found == table.end() || found->id != id) throw std::logic_error("Unknown runtime ID: " + id);
    return found->index;
}
}
std::vector<VehicleRefs> resolveRefs(const Scenario& scenario, const std::vector<Vehicle>& vehicles,
                                     const ScenarioIndex& index) {
    (void)scenario; // Every id this needs is already resolved in the index.
    std::vector<VehicleRefs> refs;
    refs.reserve(vehicles.size());
    for (const auto& vehicle : vehicles) {
        const auto route = lookup(index.routeOfId, vehicle.routeId);
        const auto type = lookup(index.typeOfId, vehicle.vehicleTypeId);
        refs.push_back({route, type, index.behaviourOfType[type]});
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
