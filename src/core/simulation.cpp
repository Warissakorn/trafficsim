#include "simulation.hpp"
#include "conflicts.hpp"
#include "detail.hpp"
#include "following.hpp"
#include "lanes.hpp"
#include "routing.hpp"
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
    std::sort(scenario.laneChanges.begin(), scenario.laneChanges.end(), [](const auto& a, const auto& b) {
        if (a.fromRouteId != b.fromRouteId) return a.fromRouteId < b.fromRouteId;
        return a.fromStart != b.fromStart ? a.fromStart < b.fromStart : a.toRouteId < b.toRouteId;
    });
    std::sort(scenario.routeDeadEnds.begin(), scenario.routeDeadEnds.end(),
              [](const auto& a, const auto& b) { return a.routeId < b.routeId; });
    return scenario;
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
                nearest = Leader{gap, span.speed, span.acceleration};
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
    return stepSimulation(SimState(state), state.scenario->timeStep);
}
SimState stepSimulation(const SimState& state, double dt) { return stepSimulation(SimState(state), dt); }
SimState stepSimulation(SimState&& state) {
    if (!state.scenario) throw std::invalid_argument("Simulation must be initialized");
    const double dt = state.scenario->timeStep;
    return stepSimulation(std::move(state), dt);
}
// Takes the previous state over rather than copying it: a value copy re-copied every vehicle and
// last tick's events (strings included) only to discard them, 9% of an M2.6 hour (2026-10-01).
SimState stepSimulation(SimState&& state, double dt) {
    if (!state.scenario) throw std::invalid_argument("Simulation must be initialized");
    if (dt != state.scenario->timeStep) throw std::invalid_argument("dt must equal scenario.timeStep");
    // A vehicle's inputIndex is a position in BOTH vectors, so a hand-built state that does not
    // hold one entry per scenario input must say so here rather than index past the end later.
    if (state.inputs.size() != state.scenario->inputs.size())
        throw std::invalid_argument("state.inputs must be parallel to scenario.inputs");
    if (state.tick >= totalTicks(*state.scenario)) return std::move(state);
    const auto& scenario = *state.scenario;
    // States built by createSimulation always carry an index; tolerate a hand-built one.
    const auto indexOwner = state.index ? state.index
                                        : std::make_shared<const ScenarioIndex>(buildScenarioIndex(scenario));
    const auto& index = *indexOwner;
    // The previous tick's clock. next.tick, next.time and next.stopService keep the previous
    // tick's values until the end of this step writes the new ones, so they are read as such.
    const auto startTick = state.tick;
    const double startTime = state.time;
    SimState next = std::move(state); // scenario (shared, const) stays alive through next.
    next.index = indexOwner;
    next.events.clear();
    const auto& startService = next.stopService;
    detail::generateArrivals(next); // At START of tick, before insertion or movement.
    const auto tick = startTick + 1;
    const double time = static_cast<double>(tick) * dt;
    auto& events = next.events;
    // next.vehicles is rebuilt from scratch below, so take its buffer as this tick's working copy
    // rather than copying the list. Nothing reads next.vehicles between here and the rebuild.
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
        if (!spansBuilt) {
            candidateRefs = resolveRefs(scenario, vehicles, index);
            candidateSpans = occupiedSpans(scenario, vehicles, index, candidateRefs);
            candidateBuckets = bucketSpans(candidateSpans, scenario.segments.size());
            spansBuilt = true;
        }
        Vehicle vehicle;
        static_cast<PendingVehicle&>(vehicle) = pending;
        vehicle.enteredTime = startTime;
        // The entry segment's behaviour (M3.3.2b) -- the type's own without assignments, the very
        // slot byId found before. The pending record is only read: nothing is redrawn while it waits.
        const auto behaviourSlot = effectiveBehaviour(index, vehicle);
        const auto& behaviour = scenario.behaviours[behaviourSlot];
        const auto leader = closestVehicle(vehicle, partsFor(index, scenario, route), candidateSpans,
                                           candidateBuckets);
        if (leader && leader->gap < standstillGap(behaviour)) continue;
        // D108: a positive source gap can still be smaller than the first ordinary step.
        // Keep the sampled arrival pending until that step fits the existing buffer;
        // later lane-change/signal/conflict constraints still use the normal safety checks.
        if (leader) {
            // A vehicle entering has no w74 state: its first evaluation initialises one (W74.md §7).
            const auto first=follow(0,vehicle,std::nullopt,type,behaviour,leader);
            if (integrate(0,first.acceleration,dt).distance > leader->gap-standstillGap(behaviour)) continue;
        }
        const VehicleRefs inserted{static_cast<std::size_t>(&route - scenario.routes.data()),
                                   static_cast<std::size_t>(&type - scenario.vehicleTypes.data()),
                                   behaviourSlot};
        vehicles.push_back(vehicle);
        candidateRefs.push_back(inserted);
        appendVehicleSpans(candidateSpans, scenario, index, vehicle, inserted);
        candidateBuckets = bucketSpans(candidateSpans, scenario.segments.size());
        next.inputs[pending.inputIndex].queue.erase(next.inputs[pending.inputIndex].queue.begin());
        // Events still carry the route's NAME: they are the run's output, read by the evaluator
        // and by every frozen fixture, and a slot would mean nothing outside this Scenario.
        events.emplace_back(DepartedEvent{startTime, vehicle.id, route.id,
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
    // A station at the source is recognized before the first lane-change/motion decision.
    if(!scenario.routeDecisions.empty())for(auto& vehicle:vehicles)
        while(const auto* decision=nextRouteDecision(scenario,vehicle)) {
            if(vehicle.distance<decision->at-1e-9)break;
            applyRouteDecision(scenario,vehicle,*decision,startTime,next.randomState,events);
        }
    // Resolved once per tick rather than roughly six times per vehicle.
    auto refs = resolveRefs(scenario, vehicles, index);
    auto spans = occupiedSpans(scenario, vehicles, index, refs); // Everyone sees the SAME pre-step state.
    auto buckets = bucketSpans(spans, scenario.segments.size());
    // Mandatory lane changes (M3.2.8b), decided off that snapshot and applied before anything
    // moves; the rest of the tick then runs on the post-change snapshot, shared by every vehicle.
    // Free without a stub: the scenario then has no span at all.
    if (index.laneChanges) {
        const auto changes = decideLaneChanges(scenario, index, vehicles, refs, spans, buckets, startService, startTick);
        for (const auto& change : changes) {
            auto& vehicle = vehicles[change.vehicle];
            events.emplace_back(LaneChangeEvent{startTime, vehicle.id, scenario.routes[vehicle.routeIndex].id,
                                                scenario.routes[change.route].id});
            vehicle.lastLaneChange = LastLaneChange{startTick, vehicle.routeIndex};
            vehicle.laneChangeTrace.push_back({vehicle.routeIndex, change.route,
                vehicle.distance, change.distance, vehicle.speed});
            vehicle.routeIndex = change.route; vehicle.distance = change.distance;
        }
        if (!changes.empty()) {
            refs = resolveRefs(scenario, vehicles, index);
            spans = occupiedSpans(scenario, vehicles, index, refs);
            buckets = bucketSpans(spans, scenario.segments.size());
        }
    }
    // Cooperation, from the post-change snapshot: who holds back for a vehicle waiting to change.
    const auto courtesy = index.laneChanges ? courtesyHolds(scenario, index, vehicles, refs, spans, buckets)
                                            : std::vector<CourtesyHold>{};
    // Signal colour depends only on the tick's time, so it is the same for every vehicle.
    std::vector<SignalColor> headColors;
    headColors.reserve(scenario.signalHeads.size());
    for (std::size_t h = 0; h < scenario.signalHeads.size(); ++h)
        headColors.push_back(signalColorAt(scenario.signalPrograms[index.programOfHead[h]], startTime));
    // Conflict zones (M3.2.3a), read from the same snapshot. Empty -- and free -- without one.
    const auto zones = summarizeZones(scenario, index, vehicles, refs);
    // Stop service (M3.2.5), from the same snapshot and only when some zone is a Stop.
    // Kept out of the per-vehicle loop entirely when no zone is a Stop: that loop is the engine's
    // hot path, and a branch per vehicle there measured +1% of stepSimulation.
    auto service = index.stopZones ? refreshStops(scenario, index, vehicles, refs, startService, startTick)
                                   : std::vector<StopService>{};
    std::vector<const StopService*> stopOf(service.empty() ? 0 : vehicles.size(), nullptr);
    for (std::size_t v = 0, s = 0; v < stopOf.size() && s < service.size(); ++v) {
        while (s < service.size() && service[s].vehicleId < vehicles[v].id) ++s;
        if (s < service.size() && service[s].vehicleId == vehicles[v].id) stopOf[v] = &service[s];
    }
    // Phase 1: every vehicle's candidate move, from the snapshot alone. Nothing is published
    // until phase 2 has seen them all (contract §4, steps 1-3 and 6).
    struct Move { double distance{}, speed{}, acceleration{}; FollowingMode mode{}; bool clamped{}; std::optional<W74State> w74; };
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
        double allowedDistance = leader ? std::max(0.0, leader->gap - standstillGap(behaviour)) :
                                          std::numeric_limits<double>::infinity();
        for (const auto& routeHead : index.routeHeads[refs[v].route]) {
            const auto& head = scenario.signalHeads[routeHead.headIndex];
            const double gap = routeHead.partStart + head.position - vehicle.distance;
            if (gap < -1e-9) continue;
            const auto colour = headColors[routeHead.headIndex];
            if (colour == SignalColor::green) continue;
            // M4.2 (D147, AMBER.md): with an amber deceleration, a driver who cannot stop before the
            // line at it goes on amber, and one who cannot stop at all (the type's maxDeceleration,
            // as at a priority rule) goes on amber or red; equality and a standing vehicle stop.
            if (behaviour.amberDeceleration) {
                const double toLine = std::max(0.0, gap);
                if (committed(vehicle.speed, toLine, type) ||
                    (colour == SignalColor::amber && vehicle.speed * vehicle.speed > 2 * *behaviour.amberDeceleration * toLine)) continue;
            }
            allowedDistance = std::min(allowedDistance, std::max(0.0, gap));
            if (!leader || gap < leader->gap) leader = Leader{gap, 0, 0};
        }
        // A stub route's dead end holds its vehicle as a red head does, until it changes lanes.
        if (index.laneChanges)
            if (const double gap = std::max(0.0, index.deadEndOfRoute[refs[v].route] - vehicle.distance); std::isfinite(gap)) {
                allowedDistance = std::min(allowedDistance, gap);
                if (!leader || gap < leader->gap) leader = Leader{gap, 0, 0};
            }
        // Holding back for a waiting changer, as behind a standing vehicle -- even when a moving
        // leader is nearer, so it is a second obstacle rather than a replacement leader (below).
        // A moving changer (cooperative braking) is a virtual leader, so it caps nothing.
        const bool yields = !courtesy.empty() && std::isfinite(courtesy[v].gap);
        if (yields && !courtesy[v].moving)
            allowedDistance = std::min(allowedDistance, std::max(0.0, courtesy[v].gap - standstillGap(behaviour)));
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
            if (!leader || gap < leader->gap) leader = Leader{gap, 0, 0};
        }
        // Conflict zones hold a vehicle by the same stop-line mechanism once more.
        if (const auto hold = index.routeZones[refs[v].route].empty() ? std::nullopt
                              : zoneHold(scenario, index, zones, vehicle, refs[v], vehicleLeader,
                                         stopOf.empty() ? nullptr : stopOf[v], startTick)) {
            allowedDistance = std::min(allowedDistance, *hold);
            if (!leader || *hold < leader->gap) leader = Leader{*hold, 0, 0};
        }
        auto following = follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour, leader);
        if (yields) {
            // Both from the same previous state; the kept result's state is the one stored, and
            // on a tie the vehicle ahead is kept (strict <), W74.md §7.
            auto held = follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour,
                               Leader{courtesy[v].gap, courtesy[v].speed, courtesy[v].acceleration});
            // Cooperative braking never asks for more than the behaviour's maximum (M3.2.8c).
            if (courtesy[v].moving)
                held.acceleration = std::max(held.acceleration, -*behaviour.maxDecelerationCooperativeBraking);
            if (held.acceleration < following.acceleration) following = held;
        }
        const auto motion = integrate(vehicle.speed, following.acceleration, dt);
        auto& move = moves[v];
        move = {motion.distance, std::min(vehicle.desiredSpeed, motion.speed), following.acceleration, following.mode, false,
                following.w74};
        if (move.distance > allowedDistance) {
            move.distance = allowedDistance; move.speed = 0; move.acceleration = -vehicle.speed / dt;
            move.clamped = true;
        }
    }
    // A Stop finishes the stop the model only approaches (M3.2.5): from below walking pace, so at
    // most kStoppedSpeed/dt of ordinary braking -- not an emergency clamp.
    for (std::size_t v = 0; v < stopOf.size(); ++v)
        if (restsAtStop(stopOf[v], startTick)) moves[v] = {0, 0, -vehicles[v].speed / dt, FollowingMode::braking, false, moves[v].w74};
    // A newly recognized route starts at a tick boundary. Never move onto its unknown
    // suffix under the previous route's safety checks; keep speed, recognize at the line.
    if(!scenario.routeDecisions.empty())for(std::size_t v=0;v<vehicles.size();++v)
        if(const auto* decision=nextRouteDecision(scenario,vehicles[v]))
            moves[v].distance=std::min(moves[v].distance,std::max(0.,decision->at-vehicles[v].distance));
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
        auto& vehicle = vehicles[v];
        const auto& oldParts = index.parts[refs[v].route];
        const auto& move = moves[v];
        if (move.clamped) events.emplace_back(SafetyClampEvent{time, vehicle.id});
        // The working copy is not read after this vehicle is published, so its lists move rather
        // than copy; its scalars (id, times, desired speed) stay readable below. The pending decision
        // is found first, because it reads the passed-decision list.
        const auto* decision = scenario.routeDecisions.empty() ? nullptr : nextRouteDecision(scenario, vehicle);
        const double startDistance = vehicle.distance;
        auto moved = std::move(vehicle);
        moved.distance += move.distance; moved.speed = move.speed;
        moved.acceleration = move.acceleration; moved.mode = move.mode;
        moved.w74State = move.w74; // cleared on a prototype road; Stop rest and caps keep the regime
        for (std::size_t i = 1; i < oldParts.size(); ++i)
            if (startDistance < oldParts[i].start && moved.distance >= oldParts[i].start)
                events.emplace_back(SegmentEnteredEvent{time, moved.id, oldParts[i].segmentId});
        if (decision && moved.distance >= decision->at - 1e-9)
            applyRouteDecision(scenario, moved, *decision, time, next.randomState, events);
        const auto& parts=index.parts[moved.routeIndex];
        const double routeLength = parts.back().start + parts.back().length;
        // A stub's dead end may be its last metre; standing there is waiting, never arriving.
        const bool stub = index.laneChanges && std::isfinite(index.deadEndOfRoute[moved.routeIndex]);
        if (moved.distance >= routeLength && !stub) {
            ++next.completed;
            events.emplace_back(ArrivedEvent{time, vehicle.id, scenario.routes[moved.routeIndex].id,
                time - vehicle.enteredTime,
                vehicle.enteredTime - vehicle.scheduledTime, routeLength / vehicle.desiredSpeed});
        } else {
            const auto location = locateOnParts(parts, moved);
            next.vehicles.push_back(std::move(moved));
            events.emplace_back(MovedEvent{time, vehicle.id, location.segmentId, location.position, move.speed, move.acceleration});
        }
    }
    for (const auto& head : scenario.signalHeads) {
        const auto& program = detail::byId(scenario.signalPrograms, head.programId);
        const auto color = signalColorAt(program, time);
        if (color != signalColorAt(program, startTime)) events.emplace_back(SignalEvent{time, head.id, color});
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
    while (state.tick < totalTicks(*state.scenario)) { state = stepSimulation(std::move(state)); emit(); }
    return state;
}
}
