#include "test.hpp"
#include "../src/core/following.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/routes.hpp"
#include "../src/eval/lane_changes.hpp"
#include "../src/project/run.hpp"
#include "../tools/four_leg_network.hpp"
#include <algorithm>
using namespace trafficsim;
// M3.2.8b (docs/M3_8_CONTRACT.md §2, A27-A33): mandatory lane changes, on hand-built scenarios.
// One 200 m Link of three lanes: "a" | "b" | "c". The movement leaves from lane b only, so a
// vehicle on a or c is on a stub that must change to b before its dead end at 200.
namespace {
Scenario lanes() {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 120; s.timeStep = 0.1;
    s.segments = {{"a", 200, {}}, {"b", 200, {"turn"}}, {"c", 200, {}}, {"turn", 50, {}}};
    s.routes = {{"full", {"b", "turn"}}, {"stubA", {"a"}}, {"stubC", {"c"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.laneChanges = {{"stubA", "full", 0, 200, 0, 200}, {"stubC", "full", 0, 200, 0, 200}};
    s.routeDeadEnds = {{"stubA", 200}, {"stubC", 200}};
    return s;
}
test::Placement on(std::uint64_t id, const char* route, double distance, double speed) { return {id, route, distance, speed}; }
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
TEST(lanechange, validation_refuses_a_span_from_a_full_route_and_an_unreachable_dead_end) {
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
    CHECK(decideLaneChanges(*s.scenario, *s.index, s.vehicles, refs, spans, buckets, {}).empty());
    const auto holds = courtesyHolds(*s.scenario, *s.index, s.vehicles, refs, spans, buckets);
    CHECK(std::count_if(holds.begin(), holds.end(), [](double h) { return std::isfinite(h); }) == 1);
    CHECK(!std::isfinite(holds[1])); // not the nearest (vehicle 10)
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
    while (!s.vehicles.empty() && s.tick < totalTicks(*s.scenario)) { s = stepSimulation(s); changes.observe(s); }
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
