#include "simulation.hpp"
#include "conflicts.hpp"
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
    sort(scenario.conflictZones);
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
// `id`, when asked for, receives the leader's vehicle id -- only admission needs it (M3.2.3c).
std::optional<Leader> closestVehicle(const Vehicle& vehicle, const std::vector<RoutePart>& parts,
                                     const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                                     std::uint64_t* id = nullptr) {
    std::optional<Leader> nearest;
    for (const auto& part : parts) {
        if (part.start + part.length < vehicle.distance) continue;
        // Parts ascend by start and every span's rear is at least 0, so no span on this part or
        // any later one can have a gap strictly below the nearest already found: stop, and the
        // same leader is chosen as by the full scan.
        if (nearest && part.start - vehicle.distance >= nearest->gap) break;
        // Only this segment's spans; the segmentId comparison the full scan did is now implicit.
        for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i) {
            const auto& span = spans[buckets.items[i]];
            if (span.vehicleId == vehicle.id ||
                part.start + span.front < vehicle.distance - 1e-9) continue;
            const double gap = part.start + span.rear - vehicle.distance;
            if (!nearest || gap < nearest->gap) {
                nearest = Leader{gap, span.speed};
                if (id) *id = span.vehicleId;
            }
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
    // A vehicle's inputIndex is a position in BOTH vectors, so a hand-built state that does not
    // hold one entry per scenario input must say so here rather than index past the end later.
    if (state.inputs.size() != state.scenario->inputs.size())
        throw std::invalid_argument("state.inputs must be parallel to scenario.inputs");
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
    // Where the arrivals will start. The survivors kept the order last tick's rebuild wrote
    // them in, so everything before this point is already sorted by id.
    const std::size_t survivors = vehicles.size();
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
        const auto& route = scenario.routes[pending.routeIndex];
        if (!attemptedSources.insert(route.segmentIds.front()).second) continue;
        const auto& type = scenario.vehicleTypes[pending.typeIndex];
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
        next.inputs[pending.inputIndex].queue.erase(next.inputs[pending.inputIndex].queue.begin());
        // Events still carry the route's NAME: they are the run's output, read by the evaluator
        // and by every frozen fixture, and a slot would mean nothing outside this Scenario.
        events.emplace_back(DepartedEvent{state.time, vehicle.id, route.id,
                                         vehicle.scheduledTime, vehicle.desiredSpeed});
    }
    // Two sorted runs, not an unsorted list: the survivors in id order, then this tick's
    // arrivals, which were appended in scheduled-time order. Sorting the short tail and merging
    // is linear where sorting the whole fleet again is not, and ids are unique, so the result is
    // the same total order std::sort produced. A hand-built initial state need not be ordered,
    // so the prefix is checked rather than assumed; after the first tick the rebuild below is
    // what guarantees it.
    const auto byId = [](const auto& a, const auto& b) { return a.id < b.id; };
    const auto arrivals = vehicles.begin() + static_cast<std::ptrdiff_t>(survivors);
    std::sort(arrivals, vehicles.end(), byId);
    if (std::is_sorted(vehicles.begin(), arrivals, byId))
        std::inplace_merge(vehicles.begin(), arrivals, vehicles.end(), byId);
    else std::sort(vehicles.begin(), vehicles.end(), byId);
    // Resolved once per tick rather than roughly six times per vehicle.
    const auto refs = resolveRefs(scenario, vehicles, index);
    const auto spans = occupiedSpans(scenario, vehicles, index, refs); // Everyone sees the SAME pre-step state.
    const auto buckets = bucketSpans(spans, scenario.segments.size());
    // Signal colour depends only on the tick's time, so it is the same for every vehicle.
    std::vector<SignalColor> headColors;
    headColors.reserve(scenario.signalHeads.size());
    for (std::size_t h = 0; h < scenario.signalHeads.size(); ++h)
        headColors.push_back(signalColorAt(scenario.signalPrograms[index.programOfHead[h]], state.time));
    // Conflict zones (M3.2.3a), read from the same snapshot. Empty -- and free -- without one.
    const auto zones = summarizeZones(scenario, index, vehicles, refs);
    // Stop service (M3.2.5), from the same snapshot and only when some zone is a Stop.
    // Kept out of the per-vehicle loop entirely when no zone is a Stop: that loop is the engine's
    // hot path, and a branch per vehicle there measured +1% of stepSimulation.
    auto service = index.stopZones ? refreshStops(scenario, index, vehicles, refs, state.stopService, state.tick)
                                   : std::vector<StopService>{};
    std::vector<const StopService*> stopOf(service.empty() ? 0 : vehicles.size(), nullptr);
    for (std::size_t v = 0, s = 0; v < stopOf.size() && s < service.size(); ++v) {
        while (s < service.size() && service[s].vehicleId < vehicles[v].id) ++s;
        if (s < service.size() && service[s].vehicleId == vehicles[v].id) stopOf[v] = &service[s];
    }
    // Phase 1: every vehicle's candidate move, from the snapshot alone. Nothing is published
    // until phase 2 has seen them all (contract §4, steps 1-3 and 6).
    struct Move { double distance{}, speed{}, acceleration{}; FollowingMode mode{}; bool clamped{}; };
    std::vector<Move> moves(vehicles.size());
    std::vector<std::optional<VehicleLeader>> leaders(zones.empty() ? 0 : vehicles.size()); // phase 2 only
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        const auto& vehicle = vehicles[v];
        const auto& type = scenario.vehicleTypes[refs[v].type];
        const auto& behaviour = scenario.behaviours[refs[v].behaviour];
        const auto& parts = index.parts[refs[v].route];
        std::uint64_t leaderId = 0;
        auto leader = closestVehicle(vehicle, parts, spans, buckets, zones.empty() ? nullptr : &leaderId);
        const auto vehicleLeader = leader; // receiving space is about vehicles, not stop lines
        if (leader && !zones.empty()) leaders[v] = VehicleLeader{leader->gap, leader->speed, leaderId};
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
            // M3.2.8a: a driver who cannot stop at the line goes unless the point is occupied.
            const bool goes = committed(vehicle.speed, std::max(0.0, gap), type);
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
                } else if (goes) continue; // only occupancy holds it; a front clipped at a join
                // reads reach 0 here, but it is then on the minor's next segment, a car-following leader
                else if (reach <= rule.headway) giveWay = true;
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
        // Conflict zones hold a vehicle by the same stop-line mechanism once more.
        if (const auto hold = index.routeZones[refs[v].route].empty() ? std::nullopt
                              : zoneHold(scenario, index, zones, vehicle, refs[v], vehicleLeader,
                                         stopOf.empty() ? nullptr : stopOf[v], state.tick)) {
            allowedDistance = std::min(allowedDistance, *hold);
            if (!leader || *hold < leader->gap) leader = Leader{*hold, 0};
        }
        const auto following = followingAcceleration(vehicle.speed, vehicle.desiredSpeed,
                                                       vehicle.driverFactor, type, behaviour, leader);
        const auto motion = integrate(vehicle.speed, following.acceleration, dt);
        auto& move = moves[v];
        move = {motion.distance, std::min(vehicle.desiredSpeed, motion.speed), following.acceleration, following.mode, false};
        if (move.distance > allowedDistance) {
            move.distance = allowedDistance; move.speed = 0; move.acceleration = -vehicle.speed / dt;
            move.clamped = true;
        }
    }
    // A Stop finishes the stop the model only approaches (M3.2.5): from below walking pace, so at
    // most kStoppedSpeed/dt of ordinary braking -- not an emergency clamp.
    for (std::size_t v = 0; v < stopOf.size(); ++v)
        if (restsAtStop(stopOf[v], state.tick)) moves[v] = {0, 0, -vehicles[v].speed / dt, FollowingMode::braking, false};
    // Phase 2: requests resolved across all candidates at once -- the swept check and shared
    // receiving space. A cap only ever shortens a move.
    if (!zones.empty()) {
        std::vector<double> distances(moves.size());
        for (std::size_t v = 0; v < moves.size(); ++v) distances[v] = moves[v].distance;
        for (const auto& cap : resolveRequests(scenario, index, vehicles, refs, distances, leaders)) {
            auto& move = moves[cap.vehicle];
            if (move.distance <= cap.distance) continue;
            move.distance = cap.distance; move.speed = 0;
            move.acceleration = -vehicles[cap.vehicle].speed / dt; move.clamped = true;
        }
    }
    // Phase 3: publish, in vehicle order, exactly the events one pass always emitted.
    next.vehicles.clear();
    next.vehicles.reserve(vehicles.size()); // At most one survivor per vehicle; arrivals already in.
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        const auto& vehicle = vehicles[v];
        const auto& parts = index.parts[refs[v].route];
        const auto& move = moves[v];
        if (move.clamped) events.emplace_back(SafetyClampEvent{time, vehicle.id});
        auto moved = vehicle;
        moved.distance += move.distance; moved.speed = move.speed;
        moved.acceleration = move.acceleration; moved.mode = move.mode;
        for (std::size_t i = 1; i < parts.size(); ++i)
            if (vehicle.distance < parts[i].start && moved.distance >= parts[i].start)
                events.emplace_back(SegmentEnteredEvent{time, vehicle.id, parts[i].segmentId});
        const double routeLength = parts.back().start + parts.back().length;
        if (moved.distance >= routeLength) {
            ++next.completed;
            events.emplace_back(ArrivedEvent{time, vehicle.id, scenario.routes[vehicle.routeIndex].id,
                time - vehicle.enteredTime,
                vehicle.enteredTime - vehicle.scheduledTime, routeLength / vehicle.desiredSpeed});
        } else {
            next.vehicles.push_back(std::move(moved));
            const auto location = locateOnParts(parts, moved);
            events.emplace_back(MovedEvent{time, vehicle.id, location.segmentId, location.position, move.speed, move.acceleration});
        }
    }
    for (const auto& head : scenario.signalHeads) {
        const auto& program = detail::byId(scenario.signalPrograms, head.programId);
        const auto color = signalColorAt(program, time);
        if (color != signalColorAt(program, state.time)) events.emplace_back(SignalEvent{time, head.id, color});
    }
    next.tick = tick; next.time = time;
    next.stopService = std::move(service);
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
