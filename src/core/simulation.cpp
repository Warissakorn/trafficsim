#include "simulation.hpp"
#include "detail.hpp"
#include "following.hpp"
#include "routes.hpp"
#include "validate.hpp"
#include <cmath>
#include <limits>
#include <set>

namespace trafficsim {
namespace {
Scenario canonicalScenario(Scenario scenario) {
    const auto sort = [](auto& items) {
        std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
    };
    sort(scenario.segments);
    for (auto& segment : scenario.segments) std::sort(segment.next.begin(), segment.next.end());
    sort(scenario.routes); sort(scenario.vehicleTypes); sort(scenario.behaviours);
    sort(scenario.inputs); sort(scenario.signalPrograms); sort(scenario.signalHeads);
    return scenario;
}
std::optional<Leader> closestVehicle(const Vehicle& vehicle, const std::vector<RoutePart>& parts,
                                     const std::vector<OccupiedSpan>& spans) {
    std::optional<Leader> nearest;
    for (const auto& part : parts) {
        if (part.start + part.length < vehicle.distance) continue;
        for (const auto& span : spans) {
            if (span.vehicleId == vehicle.id || span.segmentId != part.segmentId ||
                part.start + span.front < vehicle.distance - 1e-9) continue;
            const double gap = part.start + span.rear - vehicle.distance;
            if (!nearest || gap < nearest->gap) nearest = Leader{gap, span.speed};
        }
    }
    return nearest;
}
}
std::uint64_t totalTicks(const Scenario& scenario) {
    return static_cast<std::uint64_t>(std::round(scenario.duration / scenario.timeStep));
}
SimState createSimulation(const Scenario& scenario, std::uint32_t seed) {
    assertValidScenario(scenario);
    SimState state;
    state.scenario = std::make_shared<const Scenario>(canonicalScenario(scenario));
    state.seed = seed;
    state.randomState = seed == 0 ? 0x6d2b79f5U : seed;
    detail::initializeInputs(state);
    for (const auto& head : state.scenario->signalHeads)
        state.events.emplace_back(SignalEvent{0, head.id,
            signalColorAt(detail::byId(state.scenario->signalPrograms, head.programId), 0)});
    return state;
}
SimState stepSimulation(const SimState& state) {
    if (!state.scenario) throw std::invalid_argument("Simulation must be initialized");
    return stepSimulation(state, state.scenario->timeStep);
}
SimState stepSimulation(const SimState& state, double dt) {
    if (!state.scenario) throw std::invalid_argument("Simulation must be initialized");
    if (dt != state.scenario->timeStep) throw std::invalid_argument("dt must equal scenario.timeStep");
    if (state.tick >= totalTicks(*state.scenario)) return state;
    const auto& scenario = *state.scenario;
    SimState next = state; // Value copy of state; scenario alone is shared and const.
    next.events.clear();
    detail::generateArrivals(next); // At START of tick, before insertion or movement.
    const auto tick = state.tick + 1;
    const double time = static_cast<double>(tick) * dt;
    auto& events = next.events;
    auto vehicles = state.vehicles;
    std::vector<PendingVehicle> candidates;
    for (const auto& input : next.inputs)
        if (!input.queue.empty()) candidates.push_back(input.queue.front());
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
        return a.scheduledTime == b.scheduledTime ? a.id < b.id : a.scheduledTime < b.scheduledTime;
    });
    std::set<std::string> attemptedSources;
    for (const auto& pending : candidates) {
        const auto& route = detail::byId(scenario.routes, pending.routeId);
        if (!attemptedSources.insert(route.segmentIds.front()).second) continue;
        const auto& type = detail::byId(scenario.vehicleTypes, pending.vehicleTypeId);
        const auto& behaviour = detail::byId(scenario.behaviours, type.behaviourId);
        Vehicle vehicle;
        static_cast<PendingVehicle&>(vehicle) = pending;
        vehicle.enteredTime = state.time;
        const auto leader = closestVehicle(vehicle, routeParts(scenario, route), occupiedSpans(scenario, vehicles));
        if (leader && leader->gap < behaviour.standstillDistance) continue;
        vehicles.push_back(vehicle);
        for (auto& input : next.inputs)
            if (input.id == pending.inputId) { input.queue.erase(input.queue.begin()); break; }
        events.emplace_back(DepartedEvent{state.time, vehicle.id, vehicle.routeId,
                                         vehicle.scheduledTime, vehicle.desiredSpeed});
    }
    std::sort(vehicles.begin(), vehicles.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
    const auto spans = occupiedSpans(scenario, vehicles); // Everyone sees the SAME pre-step state.
    next.vehicles.clear();
    for (const auto& vehicle : vehicles) {
        const auto& type = detail::byId(scenario.vehicleTypes, vehicle.vehicleTypeId);
        const auto& behaviour = detail::byId(scenario.behaviours, type.behaviourId);
        const auto parts = routeParts(scenario, detail::byId(scenario.routes, vehicle.routeId));
        auto leader = closestVehicle(vehicle, parts, spans);
        double allowedDistance = leader ? std::max(0.0, leader->gap - behaviour.standstillDistance) :
                                          std::numeric_limits<double>::infinity();
        for (const auto& head : scenario.signalHeads) {
            const auto part = std::find_if(parts.begin(), parts.end(),
                [&](const auto& item) { return item.segmentId == head.segmentId; });
            if (part == parts.end()) continue;
            const double gap = part->start + head.position - vehicle.distance;
            if (gap < -1e-9) continue;
            if (signalColorAt(detail::byId(scenario.signalPrograms, head.programId), state.time) == SignalColor::green)
                continue;
            allowedDistance = std::min(allowedDistance, std::max(0.0, gap));
            if (!leader || gap < leader->gap) leader = Leader{gap, 0};
        }
        const auto following = followingAcceleration(vehicle.speed, vehicle.desiredSpeed,
                                                       vehicle.driverFactor, type, behaviour, leader);
        const auto motion = integrate(vehicle.speed, following.acceleration, dt);
        double speed = std::min(vehicle.desiredSpeed, motion.speed);
        double distance = motion.distance;
        double acceleration = following.acceleration;
        if (distance > allowedDistance) {
            distance = allowedDistance; speed = 0; acceleration = -vehicle.speed / dt;
            events.emplace_back(SafetyClampEvent{time, vehicle.id});
        }
        auto moved = vehicle;
        moved.distance += distance; moved.speed = speed;
        moved.acceleration = acceleration; moved.mode = following.mode;
        for (std::size_t i = 1; i < parts.size(); ++i)
            if (vehicle.distance < parts[i].start && moved.distance >= parts[i].start)
                events.emplace_back(SegmentEnteredEvent{time, vehicle.id, parts[i].segmentId});
        const double routeLength = parts.back().start + parts.back().length;
        if (moved.distance >= routeLength) {
            ++next.completed;
            events.emplace_back(ArrivedEvent{time, vehicle.id, vehicle.routeId, time - vehicle.enteredTime,
                vehicle.enteredTime - vehicle.scheduledTime, routeLength / vehicle.desiredSpeed});
        } else {
            next.vehicles.push_back(moved);
            const auto location = locateVehicle(scenario, moved);
            events.emplace_back(MovedEvent{time, vehicle.id, location.segmentId, location.position, speed, acceleration});
        }
    }
    for (const auto& head : scenario.signalHeads) {
        const auto& program = detail::byId(scenario.signalPrograms, head.programId);
        const auto color = signalColorAt(program, time);
        if (color != signalColorAt(program, state.time)) events.emplace_back(SignalEvent{time, head.id, color});
    }
    next.tick = tick; next.time = time;
    // Retain demand due in the last subinterval, even though it cannot enter this run.
    if (tick == totalTicks(scenario)) detail::generateArrivals(next);
    return next;
}
SimState runSimulation(const Scenario& scenario, std::uint32_t seed,
                       const EventSink& sink, bool includeMovementEvents) {
    auto state = createSimulation(scenario, seed);
    const auto emit = [&] {
        if (sink) for (const auto& event : state.events)
            if (includeMovementEvents || !std::holds_alternative<MovedEvent>(event)) sink(event);
    };
    emit();
    while (state.tick < totalTicks(*state.scenario)) { state = stepSimulation(state); emit(); }
    return state;
}
}
