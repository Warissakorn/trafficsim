#include "simulation.hpp"
#include "detail.hpp"
#include "following.hpp"
#include "routes.hpp"
#include "validate.hpp"
#include <cmath>
#include <limits>
#include <cstdint>
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
// Spans grouped by segment, flat (CSR) so grouping costs three allocations, not one per segment.
// items keeps each segment's spans in their original relative order, which is what makes the
// strictly-less-than tie-break below select exactly the same span as a full scan would.
struct SpanBuckets {
    std::vector<std::uint32_t> start, items;
};
SpanBuckets bucketSpans(const std::vector<OccupiedSpan>& spans, std::size_t segmentCount) {
    SpanBuckets buckets;
    buckets.start.assign(segmentCount + 1, 0);
    for (const auto& span : spans) ++buckets.start[span.segmentIndex + 1];
    for (std::size_t i = 0; i < segmentCount; ++i) buckets.start[i + 1] += buckets.start[i];
    buckets.items.resize(spans.size());
    auto cursor = buckets.start;
    for (std::uint32_t i = 0; i < spans.size(); ++i) buckets.items[cursor[spans[i].segmentIndex]++] = i;
    return buckets;
}
std::optional<Leader> closestVehicle(const Vehicle& vehicle, const std::vector<RoutePart>& parts,
                                     const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets) {
    std::optional<Leader> nearest;
    for (const auto& part : parts) {
        if (part.start + part.length < vehicle.distance) continue;
        // Only this segment's spans; the segmentId comparison the full scan did is now implicit.
        for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i) {
            const auto& span = spans[buckets.items[i]];
            if (span.vehicleId == vehicle.id ||
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
    // Built from the canonical scenario, so part order matches the sorted routes.
    state.index = std::make_shared<const ScenarioIndex>(buildScenarioIndex(*state.scenario));
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
    // States built by createSimulation always carry an index; tolerate a hand-built one.
    const auto indexOwner = state.index ? state.index
                                        : std::make_shared<const ScenarioIndex>(buildScenarioIndex(scenario));
    const auto& index = *indexOwner;
    SimState next = state; // Value copy of state; scenario alone is shared and const.
    next.index = indexOwner;
    next.events.clear();
    detail::generateArrivals(next); // At START of tick, before insertion or movement.
    const auto tick = state.tick + 1;
    const double time = static_cast<double>(tick) * dt;
    auto& events = next.events;
    // The state copy above already deep-copied the vehicle list, and next.vehicles is rebuilt from
    // scratch below, so take that buffer as this tick's working copy rather than copying the list a
    // second time. Nothing reads next.vehicles between here and the rebuild.
    auto vehicles = std::move(next.vehicles);
    next.vehicles.clear();
    std::vector<PendingVehicle> candidates;
    for (const auto& input : next.inputs)
        if (!input.queue.empty()) candidates.push_back(input.queue.front());
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
        return a.scheduledTime == b.scheduledTime ? a.id < b.id : a.scheduledTime < b.scheduledTime;
    });
    std::set<std::string> attemptedSources;
    // Built at most once per tick rather than per candidate, and only once a candidate actually
    // survives the source filter - most ticks have no arrival at all and must stay free.
    // Insertions append to both, reproducing exactly what a full rebuild over the grown vehicle
    // list would have produced.
    std::vector<VehicleRefs> candidateRefs;
    std::vector<OccupiedSpan> candidateSpans;
    SpanBuckets candidateBuckets;
    bool spansBuilt = false;
    for (const auto& pending : candidates) {
        const auto& route = detail::byId(scenario.routes, pending.routeId);
        if (!attemptedSources.insert(route.segmentIds.front()).second) continue;
        const auto& type = detail::byId(scenario.vehicleTypes, pending.vehicleTypeId);
        const auto& behaviour = detail::byId(scenario.behaviours, type.behaviourId);
        if (!spansBuilt) {
            candidateRefs = resolveRefs(scenario, vehicles, index);
            candidateSpans = occupiedSpans(scenario, vehicles, index, candidateRefs);
            candidateBuckets = bucketSpans(candidateSpans, scenario.segments.size());
            spansBuilt = true;
        }
        Vehicle vehicle;
        static_cast<PendingVehicle&>(vehicle) = pending;
        vehicle.enteredTime = state.time;
        const auto leader = closestVehicle(vehicle, partsFor(index, scenario, route), candidateSpans,
                                           candidateBuckets);
        if (leader && leader->gap < behaviour.standstillDistance) continue;
        const VehicleRefs inserted{static_cast<std::size_t>(&route - scenario.routes.data()),
                                   static_cast<std::size_t>(&type - scenario.vehicleTypes.data()),
                                   static_cast<std::size_t>(&behaviour - scenario.behaviours.data())};
        vehicles.push_back(vehicle);
        candidateRefs.push_back(inserted);
        appendVehicleSpans(candidateSpans, scenario, index, vehicle, inserted);
        candidateBuckets = bucketSpans(candidateSpans, scenario.segments.size());
        for (auto& input : next.inputs)
            if (input.id == pending.inputId) { input.queue.erase(input.queue.begin()); break; }
        events.emplace_back(DepartedEvent{state.time, vehicle.id, vehicle.routeId,
                                         vehicle.scheduledTime, vehicle.desiredSpeed});
    }
    std::sort(vehicles.begin(), vehicles.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
    // Resolved once per tick rather than roughly six times per vehicle.
    const auto refs = resolveRefs(scenario, vehicles, index);
    const auto spans = occupiedSpans(scenario, vehicles, index, refs); // Everyone sees the SAME pre-step state.
    const auto buckets = bucketSpans(spans, scenario.segments.size());
    // Signal colour depends only on the tick's time, so it is the same for every vehicle.
    std::vector<SignalColor> headColors;
    headColors.reserve(scenario.signalHeads.size());
    for (std::size_t h = 0; h < scenario.signalHeads.size(); ++h)
        headColors.push_back(signalColorAt(scenario.signalPrograms[index.programOfHead[h]], state.time));
    next.vehicles.clear();
    next.vehicles.reserve(vehicles.size()); // At most one survivor per vehicle; arrivals already in.
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        const auto& vehicle = vehicles[v];
        const auto& type = scenario.vehicleTypes[refs[v].type];
        const auto& behaviour = scenario.behaviours[refs[v].behaviour];
        const auto& parts = index.parts[refs[v].route];
        auto leader = closestVehicle(vehicle, parts, spans, buckets);
        double allowedDistance = leader ? std::max(0.0, leader->gap - behaviour.standstillDistance) :
                                          std::numeric_limits<double>::infinity();
        for (const auto& routeHead : index.routeHeads[refs[v].route]) {
            const auto& head = scenario.signalHeads[routeHead.headIndex];
            const double gap = routeHead.partStart + head.position - vehicle.distance;
            if (gap < -1e-9) continue;
            if (headColors[routeHead.headIndex] == SignalColor::green) continue;
            allowedDistance = std::min(allowedDistance, std::max(0.0, gap));
            if (!leader || gap < leader->gap) leader = Leader{gap, 0};
        }
        // Priority rules, after the signal heads and by the same mechanism: a vehicle that must
        // give way is held at its stop line exactly as a red head holds one. Car-following past
        // the merge already works without any of this, because spans are bucketed by GLOBAL
        // segment index, so two vehicles see each other the moment they share a segment. What a
        // rule adds is seeing the major approach BEFORE entering it, which is not on this
        // vehicle's own route and so is invisible to closestVehicle.
        for (const auto& routeRule : index.routeRules[refs[v].route]) {
            const auto& rule = scenario.priorityRules[routeRule.ruleIndex];
            const double gap = routeRule.partStart + rule.yieldPosition - vehicle.distance;
            if (gap < -1e-9) continue; // Already across the stop line; the decision was taken.
            const auto conflict = index.conflictSegmentOfRule[routeRule.ruleIndex];
            if (conflict == SIZE_MAX) continue; // Rejected by validation; never read out of bounds.
            bool giveWay = false;
            for (auto i = buckets.start[conflict]; i < buckets.start[conflict + 1]; ++i) {
                const auto& span = spans[buckets.items[i]];
                if (span.vehicleId == vehicle.id) continue;
                // Positions are along the conflict segment, so this is a distance on the major
                // approach: positive means its front is still short of the conflict point.
                const double reach = rule.conflictPosition - span.front;
                if (reach < 0) {
                    // Its front is past the point; it still blocks while its rear is not.
                    if (span.rear <= rule.conflictPosition) giveWay = true;
                } else if (reach <= rule.headway) giveWay = true;
                // Arriving too soon. A stopped major vehicle further off than the headway does
                // NOT block, which is what stops a standing queue from deadlocking the minor
                // approach forever rather than releasing it into a gap that genuinely exists.
                else if (span.speed > 0 && reach / span.speed < rule.gapTime) giveWay = true;
                if (giveWay) break;
            }
            if (!giveWay) continue;
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
            const auto location = locateOnParts(parts, moved);
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
