#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/core/following.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/routes.hpp"
#include "../src/eval/lane_changes.hpp"
#include "../src/eval/segment_times.hpp"
#include "../src/project/run.hpp"
#include "../tools/four_leg_network.hpp"
#include <algorithm>
using namespace trafficsim;
// M3.2.8b (docs/M3_8_CONTRACT.md §2, A27-A35) and M3.2.8c cooperative braking (A36-A39): mandatory
// lane changes, on hand-built scenarios.
namespace {
using test::lanes; using test::on;
const Vehicle* find(const SimState& s, std::uint64_t id) {
    const auto it = std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
    return it == s.vehicles.end() ? nullptr : &*it;
}
const std::string& routeOf(const SimState& s, std::uint64_t id) { return s.scenario->routes[find(s, id)->routeIndex].id; }
struct Tally { int changes{}, clamps{}; std::vector<std::string> arrivedOn; };
void count(const SimState& s, Tally& t) {
    for (const auto& e : s.events) {
        t.changes += std::holds_alternative<LaneChangeEvent>(e);
        t.clamps += std::holds_alternative<SafetyClampEvent>(e);
        if (const auto* a = std::get_if<ArrivedEvent>(&e)) t.arrivedOn.push_back(a->routeId);
    }
}
bool changed(const SimState& s, std::uint64_t id) {
    return std::any_of(s.events.begin(), s.events.end(), [&](const auto& e) {
        const auto* c = std::get_if<LaneChangeEvent>(&e); return c && c->vehicleId == id; });
}
}
TEST(lanechange, a_stub_vehicle_on_an_empty_road_changes_at_once_and_arrives_on_the_full_chain) { // A27
    auto s = test::withVehicles(lanes(), {on(1, "stubA", 20, 10)});
    CHECK(s.index->remainingOfRoute[find(s, 1)->routeIndex] == 1); // the forcing: it is on a stub
    s = stepSimulation(s);
    CHECK(changed(s, 1) && routeOf(s, 1) == "full");
    Tally t;
    while (!s.vehicles.empty() && s.tick < totalTicks(*s.scenario)) { s = stepSimulation(s); count(s, t); }
    CHECK(t.clamps == 0 && t.arrivedOn == std::vector<std::string>{"full"});
}
TEST(lanechange, display_trace_captures_remap_and_cannot_change_traffic) {
    const auto before=test::withVehicles(lanes(),{on(1,"stubA",20,10)});
    auto traced=stepSimulation(before);
    CHECK(changed(traced,1)); // forcing: the record is from a real accepted engine change
    const auto* old=find(before,1);const auto* moved=find(traced,1);
    CHECK(old->laneChangeTrace.empty() && moved->laneChangeTrace.size()==1);
    const auto record=moved->laneChangeTrace.front();
    CHECK(record.fromRoute==old->routeIndex && record.toRoute==moved->routeIndex);
    CHECK(record.fromDistance==20 && record.toDistance==20 && record.speed==10);
    auto erased=traced;
    for(int tick=0;tick<150;++tick) {
        for(auto& vehicle:erased.vehicles)vehicle.laneChangeTrace.clear();
        traced=stepSimulation(traced);erased=stepSimulation(erased);
        CHECK(traced.events==erased.events && traced.randomState==erased.randomState);
        auto traffic=traced.vehicles;
        for(auto& vehicle:traffic)vehicle.laneChangeTrace.clear();
        auto without=erased.vehicles;
        for(auto& vehicle:without)vehicle.laneChangeTrace.clear();
        CHECK(traffic==without && traced.inputs==erased.inputs && traced.completed==erased.completed);
    }
    CHECK(before.vehicles.front().laneChangeTrace.empty()); // copied state remains independent
}
TEST(lanechange, a_vehicle_alongside_refuses_the_change_until_it_clears) { // A28
    auto s = test::withVehicles(lanes(), {on(1, "stubA", 30, 8), on(2, "full", 32, 14)});
    CHECK(find(s, 2)->distance - 4.5 < find(s, 1)->distance); // the forcing: it overlaps the target place
    s = stepSimulation(s);
    CHECK(!changed(s, 1) && routeOf(s, 1) == "stubA");
    Tally t;
    for (int i = 0; i < 100 && routeOf(s, 1) == "stubA"; ++i) { s = stepSimulation(s); count(s, t); }
    CHECK(t.changes == 1 && routeOf(s, 1) == "full");
    CHECK(find(s, 2)->distance - 4.5 - find(s, 1)->distance >= 2 - 1e-9); // behind it, standstill kept
}
TEST(lanechange, a_fast_follower_close_behind_refuses_the_change) { // A29
    const auto start = test::withVehicles(lanes(), {on(1, "stubA", 50, 5), on(2, "full", 40, 15)});
    // The forcing: the follower would have to brake harder than it comfortably can.
    const auto& car = start.scenario->vehicleTypes[find(start, 2)->typeIndex];
    const auto& behaviour = start.scenario->behaviours.front();
    const double gap = 50 - 4.5 - 40;
    CHECK(followingAcceleration(15, 15, 0.5, car, behaviour, Leader{gap, 5}).acceleration < -car.comfortableDeceleration);
    auto s = stepSimulation(start);
    CHECK(!changed(s, 1));
    Tally t;
    for (int i = 0; i < 200 && routeOf(s, 1) == "stubA"; ++i) { s = stepSimulation(s); count(s, t); }
    CHECK(routeOf(s, 1) == "full" && t.clamps == 0);
    CHECK(find(s, 1)->distance < find(s, 2)->distance); // it went in behind the follower that passed
}
TEST(lanechange, a_blocked_target_holds_the_vehicle_at_its_dead_end_without_a_clamp) { // A30
    auto scenario = lanes();
    scenario.signalPrograms = {{"red-then-green", 0, {{40, SignalColor::red}, {80, SignalColor::green}}}};
    scenario.signalHeads = {{"head", "b", 199, "red-then-green"}};
    std::vector<test::Placement> queue{on(1, "stubA", 100, 10)};
    for (std::uint64_t k = 0; k < 15; ++k) queue.push_back(on(10 + k, "full", 198 - 7 * static_cast<double>(k), 0));
    auto s = test::withVehicles(scenario, queue);
    Tally t;
    for (int i = 0; i < 380; ++i) { s = stepSimulation(s); count(s, t); }
    // The forcing: the queue is still standing, with no room anywhere alongside the dead end.
    CHECK(find(s, 10) && find(s, 10)->speed == 0 && t.changes == 0);
    CHECK(find(s, 1)->distance > 190 && find(s, 1)->distance <= 200 + 1e-9 && find(s, 1)->speed < 0.5);
    while (!s.vehicles.empty() && s.tick < totalTicks(*s.scenario)) { s = stepSimulation(s); count(s, t); }
    CHECK(t.clamps == 0 && t.changes == 1);
    CHECK(std::count(t.arrivedOn.begin(), t.arrivedOn.end(), "stubA") == 0 && t.arrivedOn.size() == 16);
}
TEST(lanechange, two_vehicles_aiming_at_one_gap_in_a_tick_the_lower_id_goes) { // A31
    const auto s = stepSimulation(test::withVehicles(lanes(), {on(1, "stubA", 50, 10), on(2, "stubC", 50, 10)}));
    CHECK(changed(s, 1) && !changed(s, 2));
    CHECK(routeOf(s, 1) == "full" && routeOf(s, 2) == "stubC");
}
TEST(lanechange, no_change_inside_a_conflict_area_on_the_target_route) { // A32
    auto scenario = lanes();
    scenario.segments.push_back({"cross", 200, {}});
    scenario.routes.push_back({"crossRoute", {"cross"}});
    scenario.conflictZones = {{"zone", {{"b"}, 100, 110}, {{"cross"}, 98, 102}, 90, 3, 10}};
    auto s = test::withVehicles(scenario, {on(1, "stubA", 105, 5)});
    const auto& zones = s.index->routeZones[find(s, 1)->routeIndex];
    CHECK(zones.empty()); // the forcing: only the target route crosses the area
    s = stepSimulation(s);
    CHECK(!changed(s, 1));
    std::optional<double> at;
    for (int i = 0; i < 100 && !at; ++i) {
        s = stepSimulation(s);
        for (const auto& e : s.events) if (std::holds_alternative<LaneChangeEvent>(e)) at = find(s, 1)->distance;
    }
    CHECK(at && *at - 4.5 > 110 - 1e-9);
}
TEST(lanechange, a_copied_state_replays_exactly_and_congested_demand_is_conserved) { // A33
    auto scenario = lanes();
    scenario.inputs = {{"inA", "stubA", "car", 700, 0, 60}, {"inB", "full", "car", 700, 0, 60},
                       {"inC", "stubC", "car", 700, 0, 60}};
    scenario.signalPrograms = {{"cycle", 0, {{30, SignalColor::green}, {30, SignalColor::red}}}};
    scenario.signalHeads = {{"head", "b", 199, "cycle"}};
    scenario.duration = 300;
    auto s = createSimulation(scenario, 42);
    Tally t;
    for (int i = 0; i < 400; ++i) { s = stepSimulation(s); count(s, t); }
    CHECK(t.changes > 0); // the forcing: changes are under way when the state is branched
    auto copy = s;
    for (int i = 0; i < 300; ++i) {
        s = stepSimulation(s); copy = stepSimulation(copy);
        CHECK(s.vehicles == copy.vehicles && s.events == copy.events);
        count(s, t);
    }
    while (s.tick < totalTicks(*s.scenario)) { s = stepSimulation(s); count(s, t); }
    std::size_t pending = 0;
    for (const auto& input : s.inputs) pending += input.queue.size();
    CHECK(s.completed + s.vehicles.size() + pending == s.nextVehicleId - 1);
    CHECK(s.vehicles.empty() && pending == 0 && s.completed > 0);
    CHECK(std::count(t.arrivedOn.begin(), t.arrivedOn.end(), "full") == static_cast<long>(s.completed));
}
TEST(lanechange, validation_refuses_a_span_from_a_full_route_into_a_stub_and_an_unreachable_dead_end) {
    auto fromFull = lanes();
    fromFull.laneChanges.push_back({"full", "stubA", 0, 200, 0, 200});
    const auto a = validateScenario(fromFull);
    CHECK(std::any_of(a.begin(), a.end(), [](const auto& i) { return i.code == "INVALID_RANGE" && i.path == "laneChanges[2].fromRouteId"; }));
    auto stranded = lanes();
    stranded.laneChanges.pop_back(); // stubC keeps its dead end but loses its only span
    const auto b = validateScenario(stranded);
    CHECK(std::any_of(b.begin(), b.end(), [](const auto& i) { return i.code == "LANE_CHANGE_DEAD_END_UNREACHABLE"; }));
    CHECK(validateScenario(lanes()).empty());
}
TEST(lanechange, a_waiting_vehicle_is_let_in_by_the_stream_it_waits_beside) { // A35, cooperation
    // A stream on the target lane at 10 m/s with 7.5 m gaps: never room to change into unaided.
    std::vector<test::Placement> placed{on(1, "stubA", 198, 0)};
    for (std::uint64_t k = 0; k < 15; ++k) placed.push_back(on(10 + k, "full", 190 - 12 * static_cast<double>(k), 10));
    auto s = test::withVehicles(lanes(), placed);
    const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
    const auto spans = occupiedSpans(*s.scenario, s.vehicles, *s.index, refs);
    const auto buckets = bucketSpans(spans, s.scenario->segments.size());
    // The forcing: unaided, the change is refused. The nearest vehicles behind the target place
    // are too close to stop there comfortably; one further back is asked to hold back.
    CHECK(decideLaneChanges(*s.scenario, *s.index, s.vehicles, refs, spans, buckets, {}, s.tick).empty());
    const auto holds = courtesyHolds(*s.scenario, *s.index, s.vehicles, refs, spans, buckets);
    CHECK(std::count_if(holds.begin(), holds.end(), [](const auto& h) { return std::isfinite(h.gap) && !h.moving; }) == 1);
    CHECK(!std::isfinite(holds[1].gap)); // not the nearest (vehicle 10)
    Tally t;
    double changedAt = -1;
    for (int i = 0; i < 300 && changedAt < 0; ++i) {
        s = stepSimulation(s); count(s, t);
        if (changed(s, 1)) changedAt = s.time;
    }
    // It goes in the middle of the stream -- the last stream vehicle is still behind it -- and
    // nobody brakes beyond the model or is clamped.
    CHECK(changedAt > 0 && find(s, 24) && find(s, 24)->distance < find(s, 1)->distance);
    CHECK(t.clamps == 0);
}
// M3.2.8c (contract §2, "Cooperative braking", A36-A39): a changer still moving is let in before it
// stops, by a target-lane vehicle braking at no more than maxDecelerationCooperativeBraking.
namespace {
Scenario cooperative(bool set) {
    auto s = lanes();
    for (auto& b : s.behaviours) b.maxDecelerationCooperativeBraking = set ? std::optional<double>(3) : std::nullopt;
    return s;
}
// A changer at 10 m/s, 80 m short of its dead end, alongside a 10 m/s stream with 7.5 m gaps.
SimState approaching(bool set, double at = 120) {
    std::vector<test::Placement> placed{on(1, "stubA", at, 10)};
    for (std::uint64_t k = 0; k < 15; ++k) placed.push_back(on(10 + k, "full", 190 - 12 * static_cast<double>(k), 10));
    return test::withVehicles(cooperative(set), placed);
}
std::vector<CourtesyHold> holdsOf(const SimState& s) {
    const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
    const auto spans = occupiedSpans(*s.scenario, s.vehicles, *s.index, refs);
    return courtesyHolds(*s.scenario, *s.index, s.vehicles, refs, spans, bucketSpans(spans, s.scenario->segments.size()));
}
bool anyMoving(const std::vector<CourtesyHold>& holds) {
    return std::any_of(holds.begin(), holds.end(), [](const auto& h) { return h.moving; });
}
// Steps until vehicle 1 leaves the stub (or 60 s pass); its slowest speed while still on the stub.
struct Approach { double slowest{1e9}; bool changedLanes{}, movingHold{}; double hardestStream{}; Tally t; };
Approach drive(SimState s) {
    Approach a;
    for (int i = 0; i < 600 && find(s, 1) && routeOf(s, 1) == "stubA"; ++i) {
        a.slowest = std::min(a.slowest, find(s, 1)->speed);
        a.movingHold = a.movingHold || anyMoving(holdsOf(s));
        s = stepSimulation(s); count(s, a.t);
        for (const auto& v : s.vehicles) if (v.id >= 10) a.hardestStream = std::min(a.hardestStream, v.acceleration);
    }
    a.changedLanes = find(s, 1) && routeOf(s, 1) == "full";
    return a;
}
}
TEST(lanechange, a_moving_changer_is_let_in_before_it_stops) { // A36
    // The forcing: without the parameter, the same vehicle comes to a stand at its dead end.
    const auto off = drive(approaching(false));
    CHECK(off.slowest < 0.5);
    const auto with = drive(approaching(true));
    CHECK(with.changedLanes && with.slowest >= 0.5 && with.t.clamps == 0);
}
TEST(lanechange, a_cooperative_helper_never_brakes_beyond_its_maximum) { // A37
    const auto with = drive(approaching(true));
    CHECK(with.movingHold); // the forcing: somebody really braked cooperatively
    CHECK(with.hardestStream >= -3 - 1e-9 && with.t.clamps == 0);
}
TEST(lanechange, no_cooperative_braking_outside_the_look_ahead) { // A38
    const auto s = approaching(true, 40);
    // The forcing: it is on a stub, and the stream refuses its change.
    const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
    const auto spans = occupiedSpans(*s.scenario, s.vehicles, *s.index, refs);
    CHECK(s.index->remainingOfRoute[find(s, 1)->routeIndex] == 1);
    CHECK(decideLaneChanges(*s.scenario, *s.index, s.vehicles, refs, spans,
                            bucketSpans(spans, s.scenario->segments.size()), {}, s.tick).empty());
    // 160 m from its dead end at 10 m/s: far outside the 25 m it needs to stop comfortably.
    CHECK(!anyMoving(holdsOf(s)));
}
TEST(lanechange, without_the_parameter_only_the_waiting_rule_runs_and_replay_is_exact) { // A39
    CHECK(!drive(approaching(false)).movingHold);
    auto a = approaching(true);
    for (int i = 0; i < 50; ++i) a = stepSimulation(a);
    auto b = a; // a copied state
    for (int i = 0; i < 150; ++i) { a = stepSimulation(a); b = stepSimulation(b); }
    CHECK(a.vehicles == b.vehicles && a.events == b.events && a.time == b.time);
}
TEST(lanechange, the_four_leg_turns_compile_to_a_stub_per_other_lane) { // A34
    const auto built = fixture::fourLegIntersection();
    const auto s = compileDocument(built.document, test::root() / "data").scenario;
    const auto length = [&](const std::string& route, std::size_t upTo) {
        double at = 0;
        for (const auto& r : s.routes) if (r.id == route)
            for (std::size_t k = 0; k < upTo && k < r.segmentIds.size(); ++k)
                for (const auto& seg : s.segments) if (seg.id == r.segmentIds[k]) at += seg.length;
        return at;
    };
    const auto deadEnd = [&](const std::string& route) {
        for (const auto& d : s.routeDeadEnds) if (d.routeId == route) return d.at;
        return -1.0;
    };
    const auto spansOf = [&](const std::string& route) {
        std::vector<LaneChangeSpan> out;
        for (const auto& x : s.laneChanges) if (x.fromRouteId == route) out.push_back(x);
        std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.fromStart < b.fromStart; });
        return out;
    };
    // West left: the kerb lane reaches it (full); the inner lane is a stub through the taper to the
    // pocket's middle lane, with a span on each Link and its dead end where the turn leaves.
    const auto left = built.routes[1];
    CHECK(deadEnd(left + "/lane-1") < 0);
    const auto leftSpans = spansOf(left + "/lane-2");
    CHECK(leftSpans.size() == 2);
    test::near(leftSpans[0].fromStart, 0); test::near(leftSpans[0].fromEnd, length(left + "/lane-1", 1));
    test::near(deadEnd(left + "/lane-2"), length(left + "/lane-1", 3));
    test::near(leftSpans[1].fromEnd, deadEnd(left + "/lane-2"));
    for (const auto& x : leftSpans) { CHECK(x.toRouteId == left + "/lane-1"); test::near(x.toStart, x.fromStart); test::near(x.toEnd, x.fromEnd); }
    // West right: the kerb lane is the stub, and must change before the pocket entry.
    const auto right = built.routes[2];
    const auto rightSpans = spansOf(right + "/lane-1");
    CHECK(rightSpans.size() == 1 && rightSpans[0].toRouteId == right + "/lane-2");
    test::near(deadEnd(right + "/lane-1"), length(right + "/lane-2", 1));
    // Through reaches from both lanes: no stub.
    CHECK(deadEnd(built.routes[0] + "/lane-1") < 0 && deadEnd(built.routes[0] + "/lane-2") < 0);
}
// M3.2.8c step 1: the lane-change diagnostic reads where a change happened and how long a stub
// vehicle waited at its dead end, and books both on the movement the vehicle arrived on.
namespace {
LaneChangeReport diagnose(SimState s) {
    LaneChangeAccumulator changes(EvaluationSpec{{"through"}, {{"full", 0}}, {}, {}});
    changes.observe(s);
    while (s.tick < totalTicks(*s.scenario)) { s = stepSimulation(s); changes.observe(s); }
    return changes.report();
}
}
TEST(lanechange, the_diagnostic_records_an_early_change_and_no_wait) { // A27's road
    const auto s = test::withVehicles(lanes(), {on(1, "stubA", 20, 10)});
    CHECK(s.index->remainingOfRoute[find(s, 1)->routeIndex] == 1); // the forcing: it is on a stub
    const auto r = diagnose(s);
    CHECK(r.rows.size() == 3 && r.rows[0].name == "through" && r.rows[1].name == "unfinished");
    CHECK(r.rows[0].changes == 1 && r.rows[0].changedVehicles == 1 && r.rows[0].beforeDeadEnd.size() == 1);
    test::near(r.rows[0].beforeDeadEnd[0], 180); // changed where it stood, 180 m before the dead end
    CHECK(r.rows[0].waitingVehicles == 0 && r.rows[0].waitSeconds == 0);
    CHECK(r.rows[1].changes == 0 && r.rows[2].changes == 0);
}
TEST(lanechange, the_diagnostic_records_a_dead_end_wait_and_the_late_change) { // A30's road
    auto scenario = lanes();
    scenario.signalPrograms = {{"red-then-green", 0, {{40, SignalColor::red}, {80, SignalColor::green}}}};
    scenario.signalHeads = {{"head", "b", 199, "red-then-green"}};
    std::vector<test::Placement> queue{on(1, "stubA", 100, 10)};
    for (std::uint64_t k = 0; k < 15; ++k) queue.push_back(on(10 + k, "full", 198 - 7 * static_cast<double>(k), 0));
    auto s = test::withVehicles(scenario, queue);
    auto probe = s;
    for (int i = 0; i < 380; ++i) probe = stepSimulation(probe);
    const auto& waiting = *find(probe, 1);
    // The forcing: the engine's own test says it is waiting at its dead end.
    CHECK(waitingAtDeadEnd(*probe.index, waiting.routeIndex, waiting,
                           probe.scenario->behaviours[probe.index->behaviourOfType[waiting.typeIndex]]));
    const auto r = diagnose(s);
    const auto& row = r.rows[0];
    CHECK(row.changes == 1 && row.waitingVehicles == 1);
    CHECK(row.waitSeconds > 10 && row.longestWait > 10 && row.longestWait <= row.waitSeconds + 1e-9);
    CHECK(row.beforeDeadEnd.size() == 1 && row.beforeDeadEnd[0] >= 0 && row.beforeDeadEnd[0] < 10);
    CHECK(r.rows[1].waitingVehicles == 0 && r.rows[2].waitingVehicles == 0);
}
TEST(lanechange, the_diagnostic_places_every_change_of_an_inserted_vehicle) {
    // The diagnostic reads a change's place off the previous snapshot. That holds because a change
    // needs the rear inside a span, and an arrival enters with its rear behind the route's start,
    // so no vehicle changes in the tick it is inserted, although insertion comes first in a tick.
    auto scenario = lanes();
    scenario.inputs = {{"inA", "stubA", "car", 600, 0, 110}}; // seed 42 first arrives near 56 s
    const auto s = createSimulation(scenario, 42);
    std::uint64_t changes = 0; bool sameTick = false;
    for (auto probe = s; probe.tick < totalTicks(*s.scenario); ) {
        probe = stepSimulation(probe);
        for (const auto& e : probe.events) {
            changes += std::holds_alternative<LaneChangeEvent>(e);
            if (const auto* d = std::get_if<DepartedEvent>(&e)) sameTick = sameTick || changed(probe, d->vehicleId);
        }
    }
    CHECK(changes >= 3); // the forcing: inserted stub vehicles do change
    CHECK(!sameTick);
    // Late arrivals are still on the road at the end, so their changes are "unfinished": pool the rows.
    LaneChangeRow row;
    for (const auto& r : diagnose(s).rows) {
        row.changes += r.changes; row.unplaced += r.unplaced;
        row.beforeDeadEnd.insert(row.beforeDeadEnd.end(), r.beforeDeadEnd.begin(), r.beforeDeadEnd.end());
        row.atDistance.insert(row.atDistance.end(), r.atDistance.begin(), r.atDistance.end());
    }
    CHECK(row.changes == changes && row.unplaced == 0);
    CHECK(row.beforeDeadEnd.size() == changes && row.atDistance.size() == changes);
    const auto& types = s.scenario->vehicleTypes;
    const double car = std::find_if(types.begin(), types.end(), [](const auto& v) { return v.id == "car"; })->length;
    for (std::size_t k = 0; k < changes; ++k) {
        CHECK(row.atDistance[k] >= car - 1e-9); // its rear had cleared the route's start
        test::near(row.atDistance[k] + row.beforeDeadEnd[k], 200);
    }
}
// M3.2.8c step 2: the per-segment timing, on released vehicles (a DepartedEvent is its clock).
namespace {
SegmentTimeReport timeSegments(const Scenario& scenario) {
    auto s = createSimulation(scenario, 42);
    SegmentTimeAccumulator times(EvaluationSpec{{"through"}, {{"full", 0}}, {}, {}});
    times.observe(s);
    while (s.tick < totalTicks(*s.scenario)) { s = stepSimulation(s); times.observe(s); }
    return times.report();
}
// Every arrival's own event, for the figures the timing must reproduce.
std::vector<ArrivedEvent> arrivedEvents(const Scenario& scenario) {
    std::vector<ArrivedEvent> out;
    for (auto s = createSimulation(scenario, 42); s.tick < totalTicks(*s.scenario); ) {
        s = stepSimulation(s);
        for (const auto& e : s.events) if (const auto* a = std::get_if<ArrivedEvent>(&e)) out.push_back(*a);
    }
    return out;
}
std::uint64_t arrivals(const Scenario& scenario) { return arrivedEvents(scenario).size(); }
}
TEST(lanechange, segment_times_clock_each_segment_from_departure) {
    auto scenario = lanes();
    scenario.inputs = {{"inB", "full", "car", 600, 0, 110}}; // seed 42 first arrives near 56 s
    const auto n = arrivals(scenario);
    CHECK(n >= 3); // the forcing: released vehicles reach the end
    const auto r = timeSegments(scenario);
    const auto& row = r.rows.at(0);
    CHECK(row.vehicles == n && r.undeparted == 0 && r.unassigned == 0);
    CHECK(row.segments.size() == 1 && row.segments[0].segmentId == "turn" && row.segments[0].vehicles == n);
    const auto& types = scenario.vehicleTypes;
    const auto& car = *std::find_if(types.begin(), types.end(), [](const auto& v) { return v.id == "car"; });
    // 200 m of "b" from a standing start: no faster than at top speed, and not far slower.
    CHECK(row.segments[0].meanSinceDeparture >= 200 / car.desiredSpeed.max);
    CHECK(row.segments[0].meanSinceDeparture <= 200 / car.desiredSpeed.min + 10);
    // The means are the arrivals' own figures (bunched arrivals do wait to be inserted).
    double delay = 0, travel = 0;
    for (const auto& a : arrivedEvents(scenario)) { delay += a.departureDelay; travel += a.travelTime; }
    test::near(row.meanDepartureDelay, delay / static_cast<double>(n));
    test::near(row.meanTravelTime, travel / static_cast<double>(n));
    CHECK(row.meanTravelTime > row.segments[0].meanSinceDeparture);
}
TEST(lanechange, segment_times_book_a_stub_vehicle_on_the_chain_it_finished) {
    auto scenario = lanes();
    scenario.inputs = {{"inA", "stubA", "car", 600, 0, 110}};
    const auto n = arrivals(scenario);
    CHECK(n >= 3); // the forcing: stub vehicles change and arrive
    const auto r = timeSegments(scenario);
    const auto& row = r.rows.at(0);
    CHECK(row.vehicles == n && r.undeparted == 0 && r.unassigned == 0);
    // Its first segment ("a") is never entered and the change emits no entry: "turn" is the one.
    CHECK(row.segments.size() == 1 && row.segments[0].segmentId == "turn" && row.segments[0].vehicles == n);
}
