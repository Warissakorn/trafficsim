#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/core/following.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/routes.hpp"
#include "../src/eval/lane_changes.hpp"
#include "../src/project/document.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <fstream>
#include <map>
using namespace trafficsim;
// M3.2.8c, D95 (docs/reference/M3_8_CONTRACT.md §2 "Discretionary lane changes", A47-A54): a vehicle on a
// full route changes to an adjacent full route by choice, on a hand-built two-lane road.
namespace {
using test::on;
// Two lanes of one 200 m Link, both full routes to the same exit, with discretionary spans both
// ways and no dead end. The behaviour has D95's two fields; nothing else differs from the default.
Scenario road(double threshold = 0.5, double accepted = 1) {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 120; s.timeStep = 0.1;
    s.segments = {{"l1", 200, {}}, {"l2", 200, {}}};
    s.routes = {{"L1", {"l1"}}, {"L2", {"l2"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    for (auto& b : s.behaviours) { b.discretionaryLaneChangeThreshold = threshold; b.acceptedDecelerationTrailingVehicle = accepted; }
    s.laneChanges = {{"L1", "L2", 0, 200, 0, 200}, {"L2", "L1", 0, 200, 0, 200}};
    return s;
}
const Vehicle* find(const SimState& s, std::uint64_t id) {
    const auto it = std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
    return it == s.vehicles.end() ? nullptr : &*it;
}
const std::string& routeOf(const SimState& s, std::uint64_t id) { return s.scenario->routes[find(s, id)->routeIndex].id; }
bool changed(const SimState& s, std::uint64_t id) {
    return std::any_of(s.events.begin(), s.events.end(), [&](const auto& e) {
        const auto* c = std::get_if<LaneChangeEvent>(&e); return c && c->vehicleId == id; });
}
const VehicleType& typeOf(const SimState& s, std::uint64_t id) { return s.scenario->vehicleTypes[find(s, id)->typeIndex]; }
const DriverBehaviour& behaviourOf(const SimState& s, std::uint64_t id) {
    return s.scenario->behaviours[s.index->behaviourOfType[find(s, id)->typeIndex]];
}
// A47's road: a slow vehicle ahead on L1 and a faster one 15.5 m behind it; L2 empty.
std::vector<test::Placement> overtaking() { return {on(1, "L1", 60, 3), on(2, "L1", 40, 12)}; }
// What the follower gains by changing: free on L2 against following vehicle 1 on L1.
double gainOf(const SimState& s) {
    const auto& v = *find(s, 2);
    const auto here = followingAcceleration(v.speed, v.desiredSpeed, v.driverFactor, typeOf(s, 2), behaviourOf(s, 2),
                                            Leader{60 - typeOf(s, 1).length - 40, 3});
    const auto there = followingAcceleration(v.speed, v.desiredSpeed, v.driverFactor, typeOf(s, 2), behaviourOf(s, 2));
    return there.acceleration - here.acceleration;
}
std::vector<LaneChange> decide(const SimState& s, const std::vector<StopService>& service = {}) {
    const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
    const auto spans = occupiedSpans(*s.scenario, s.vehicles, *s.index, refs);
    const auto buckets = bucketSpans(spans, s.scenario->segments.size());
    return decideLaneChanges(*s.scenario, *s.index, s.vehicles, refs, spans, buckets, service, s.tick);
}
}
TEST(discretionary, a_follower_overtakes_a_slow_vehicle_on_the_empty_lane) { // A47
    auto s = test::withVehicles(road(), overtaking());
    // The forcing: it is following the slow vehicle, and L2 is better by more than the threshold.
    const auto& v = *find(s, 2);
    CHECK(followingAcceleration(v.speed, v.desiredSpeed, v.driverFactor, typeOf(s, 2), behaviourOf(s, 2),
                                Leader{60 - typeOf(s, 1).length - 40, 3}).mode != FollowingMode::free);
    CHECK(gainOf(s) >= 0.5);
    CHECK(s.index->discretionary && s.index->remainingOfRoute[v.routeIndex] == 0);
    s = stepSimulation(s);
    CHECK(changed(s, 2) && routeOf(s, 2) == "L2" && !changed(s, 1));
    int clamps = 0;
    std::map<std::uint64_t, std::string> arrivedOn;
    while (!s.vehicles.empty() && s.tick < totalTicks(*s.scenario)) {
        s = stepSimulation(s);
        for (const auto& e : s.events) {
            clamps += std::holds_alternative<SafetyClampEvent>(e);
            if (const auto* a = std::get_if<ArrivedEvent>(&e)) arrivedOn[a->vehicleId] = a->routeId;
        }
    }
    CHECK(clamps == 0 && arrivedOn[2] == "L2" && arrivedOn[1] == "L1");
}
TEST(discretionary, the_trailing_vehicle_is_protected_more_strictly_than_for_a_mandatory_change) { // A48
    // A vehicle behind the target place on L2, at a gap where it would brake between the accepted
    // (1 m/s²) and the comfortable (2 m/s²) deceleration behind the changer.
    const auto probe = test::withVehicles(road(), overtaking());
    const auto& car = typeOf(probe, 2);
    const auto& behaviour = behaviourOf(probe, 2);
    std::optional<double> place;
    for (double gap = 1; gap < 40 && !place; gap += 0.05) {
        const double a = followingAcceleration(14, 15, 0.5, car, behaviour, Leader{gap, 12}).acceleration;
        if (a < -1.2 && a > -car.comfortableDeceleration + 0.2 &&
            integrate(14, a, probe.scenario->timeStep).distance <= gap - behaviour.standstillDistance)
            place = 40 - car.length - gap;
    }
    CHECK(place.has_value());
    auto placed = overtaking();
    placed.push_back(on(3, "L2", *place, 14));
    auto s = test::withVehicles(road(), placed);
    // The forcing: §2's mandatory rules 3-4 accept this placement -- the follower brakes no harder
    // than it comfortably can and its move fits -- and the gain alone would change.
    const double gap = 40 - car.length - *place;
    const double a = followingAcceleration(14, 15, 0.5, car, behaviour, Leader{gap, 12}).acceleration;
    CHECK(a >= -car.comfortableDeceleration && a < -*behaviour.acceptedDecelerationTrailingVehicle);
    CHECK(gainOf(s) >= 0.5);
    CHECK(decide(s).empty());
    // A laxer accepted deceleration than the follower's braking lets the same change through.
    auto lax = test::withVehicles(road(0.5, -a + 0.1), placed);
    const auto changes = decide(lax);
    CHECK(changes.size() == 1 && lax.vehicles[changes.front().vehicle].id == 2);
}
TEST(discretionary, a_gain_below_the_threshold_changes_nothing) { // A49
    const auto probe = test::withVehicles(road(), overtaking());
    const double gain = gainOf(probe);
    CHECK(gain > 0); // the forcing: L2 is better
    auto s = test::withVehicles(road(gain + 0.1), overtaking());
    CHECK(decide(s).empty());
    s = stepSimulation(s);
    CHECK(!changed(s, 2) && routeOf(s, 2) == "L1");
}
TEST(discretionary, without_the_threshold_no_vehicle_changes_by_choice) { // A51, in-test
    auto scenario = road();
    for (auto& b : scenario.behaviours) { b.discretionaryLaneChangeThreshold.reset(); b.acceptedDecelerationTrailingVehicle.reset(); }
    auto s = test::withVehicles(scenario, overtaking());
    CHECK(!s.index->discretionaryOfRoute[find(s, 2)->routeIndex].empty()); // the forcing: the spans are there
    CHECK(!s.index->discretionary);
    s = stepSimulation(s);
    CHECK(!changed(s, 2));
}
TEST(discretionary, validation_takes_both_fields_or_neither_and_full_to_full_spans) {
    CHECK(validateScenario(road()).empty());
    auto one = road();
    one.behaviours.front().acceptedDecelerationTrailingVehicle.reset();
    const auto a = validateScenario(one);
    CHECK(std::any_of(a.begin(), a.end(), [](const auto& i) {
        return i.code == "INCOMPLETE_DISCRETIONARY_BEHAVIOUR" && i.path == "behaviours[0].acceptedDecelerationTrailingVehicle"; }));
    auto other = road();
    other.behaviours.front().discretionaryLaneChangeThreshold.reset();
    const auto b = validateScenario(other);
    CHECK(std::any_of(b.begin(), b.end(), [](const auto& i) {
        return i.code == "INCOMPLETE_DISCRETIONARY_BEHAVIOUR" && i.path == "behaviours[0].discretionaryLaneChangeThreshold"; }));
    auto zero = road(0);
    const auto c = validateScenario(zero);
    CHECK(std::any_of(c.begin(), c.end(), [](const auto& i) {
        return i.code == "INVALID_NUMBER" && i.path == "behaviours[0].discretionaryLaneChangeThreshold"; }));
}
TEST(discretionary, a_copied_state_replays_exactly) { // A52, with and without D101's hold
  for (const std::optional<double> hold : {std::optional<double>{}, std::optional<double>{3.0}}) {
    auto scenario = road();
    for (auto& b : scenario.behaviours) b.discretionaryLaneChangeHoldTime = hold;
    scenario.inputs = {{"in1", "L1", "car", 1100, 0, 240}, {"in2", "L2", "car", 700, 0, 240}};
    scenario.duration = 300;
    auto s = createSimulation(scenario, 42);
    int changes = 0;
    // Branch in the tick right after the first change by choice.
    for (int i = 0; i < 2400 && changes == 0; ++i) {
        s = stepSimulation(s);
        changes += static_cast<int>(std::count_if(s.events.begin(), s.events.end(),
                                                  [](const auto& e) { return std::holds_alternative<LaneChangeEvent>(e); }));
    }
    CHECK(changes > 0); // the forcing: discretionary changes are under way when the state is branched
    CHECK(std::any_of(s.vehicles.begin(), s.vehicles.end(), [](const auto& v) { return v.lastLaneChange.has_value(); }));
    auto copy = s;
    for (int i = 0; i < 300; ++i) {
        s = stepSimulation(s); copy = stepSimulation(copy);
        CHECK(s.vehicles == copy.vehicles && s.events == copy.events);
    }
  }
}
TEST(discretionary, a_steady_two_lane_stream_never_changes_back_within_three_seconds) { // A53
    auto scenario = road();
    scenario.inputs = {{"in1", "L1", "car", 1100, 0, 240}, {"in2", "L2", "car", 700, 0, 240}};
    scenario.duration = 300;
    auto s = createSimulation(scenario, 42);
    std::map<std::uint64_t, double> last;
    int changes = 0, repeats = 0, clamps = 0;
    while (s.tick < totalTicks(*s.scenario)) {
        s = stepSimulation(s);
        for (const auto& e : s.events) {
            clamps += std::holds_alternative<SafetyClampEvent>(e);
            const auto* c = std::get_if<LaneChangeEvent>(&e);
            if (!c) continue;
            ++changes;
            if (const auto it = last.find(c->vehicleId); it != last.end() && c->time - it->second < 3 - 1e-9) ++repeats;
            last[c->vehicleId] = c->time;
        }
    }
    CHECK(changes > 0); // the forcing: the stream does change lanes by choice
    CHECK(repeats == 0);
    CHECK(clamps == 0);
}
TEST(discretionary, the_diagnostic_sorts_repeats_within_three_seconds_by_kind) { // A53's counter
    auto scenario = road();
    scenario.segments.push_back({"l3", 200, {}});
    scenario.routes.push_back({"L3", {"l3"}});
    scenario.laneChanges.push_back({"L2", "L3", 0, 200, 0, 200});
    auto s = test::withVehicles(scenario, {on(1, "L1", 50, 10), on(2, "L1", 80, 10)});
    LaneChangeAccumulator changes(EvaluationSpec{{"through"}, {{"L1", 0}, {"L2", 0}, {"L3", 0}}, {}, {}});
    changes.observe(s);
    const auto feed = [&](std::vector<LaneChangeEvent> events) {
        s.events.clear();
        for (auto& e : events) s.events.emplace_back(std::move(e));
        changes.observe(s);
    };
    feed({{1.0, 1, "L1", "L2"}, {1.0, 2, "L1", "L2"}});
    feed({{2.0, 1, "L2", "L1"}, {2.5, 2, "L2", "L3"}}); // back within 1 s; onward within 1.5 s
    feed({{6.0, 1, "L1", "L2"}});                       // 4 s later: not a repeat, but a return
    feed({{17.0, 1, "L2", "L1"}});                      // 11 s later: neither
    const auto row = changes.report().rows[1];          // "unfinished": nobody arrived
    CHECK(row.discretionaryChanges == 6 && row.changes == 0);
    CHECK(row.quickRepeats == 2 && row.quickBack == 1 && row.quickOnward == 1 && row.quickAfterMandatory == 0);
    // D101's A53: straight back within 10 s -- 1 s and 4 s count; onward and 11 s do not.
    CHECK(row.returns == 2);
}
// D101 (A56-A58): a hold after a vehicle's last change, of either kind, delays discretionary changes only.
namespace {
// A47 mirrored onto L2, so changing BACK to L1 is what pays; the follower changed from L1 at tick 0.
SimState justChanged(std::optional<double> hold, std::uint64_t tick) {
    auto scenario = road();
    for (auto& b : scenario.behaviours) b.discretionaryLaneChangeHoldTime = hold;
    auto s = test::withVehicles(scenario, {on(1, "L2", 60, 3), on(2, "L2", 40, 12)});
    std::uint32_t l1 = 0;
    while (s.scenario->routes[l1].id != "L1") ++l1;
    for (auto& v : s.vehicles) if (v.id == 2) v.lastLaneChange = LastLaneChange{0, l1};
    s.tick = tick; s.time = static_cast<double>(tick) * s.scenario->timeStep;
    return s;
}
}
TEST(discretionary, a_hold_delays_the_change_back_until_it_has_passed) { // A56
    // The forcing: with no hold, the follower changes back to L1 at once, 1 s after its change.
    const auto free = decide(justChanged(std::nullopt, 10));
    CHECK(free.size() == 1);
    CHECK(decide(justChanged(3.0, 10)).empty());  // 1 s into a 3 s hold
    CHECK(decide(justChanged(3.0, 29)).empty());  // 2.9 s
    const auto after = decide(justChanged(3.0, 30)); // 3 s: the hold has passed
    CHECK(after.size() == 1 && after.front().route == free.front().route);
}
TEST(discretionary, a_hold_never_delays_a_mandatory_change) { // A57
    auto scenario = road();
    for (auto& b : scenario.behaviours) b.discretionaryLaneChangeHoldTime = 60.0;
    scenario.segments.push_back({"l0", 200, {}});
    scenario.routes.push_back({"S", {"l0"}});
    scenario.routeDeadEnds = {{"S", 150}};
    scenario.laneChanges.push_back({"S", "L1", 0, 150, 0, 150});
    auto s = test::withVehicles(scenario, {on(2, "S", 100, 10)});
    std::uint32_t l1 = 0;
    while (s.scenario->routes[l1].id != "L1") ++l1;
    s.vehicles.front().lastLaneChange = LastLaneChange{0, l1};
    s.tick = 10;
    // The forcing: the stub vehicle's last change is 1 s old, well inside the 60 s hold.
    CHECK(s.index->remainingOfRoute[s.vehicles.front().routeIndex] > 0);
    CHECK(static_cast<double>(s.tick - s.vehicles.front().lastLaneChange->tick) * s.scenario->timeStep < 60);
    const auto changes = decide(s);
    CHECK(changes.size() == 1 && s.scenario->routes[changes.front().route].id == "L1");
}
TEST(discretionary, without_a_hold_the_record_is_never_read) { // A58
    auto scenario = road();
    scenario.inputs = {{"in1", "L1", "car", 1100, 0, 240}, {"in2", "L2", "car", 700, 0, 240}};
    scenario.duration = 240;
    auto s = createSimulation(scenario, 42), blind = s;
    int changes = 0;
    while (s.tick < totalTicks(*s.scenario)) {
        // `blind` forgets every record before each step; with no hold that must change nothing.
        for (auto& v : blind.vehicles) v.lastLaneChange.reset();
        s = stepSimulation(s); blind = stepSimulation(blind);
        CHECK(s.events == blind.events);
        changes += static_cast<int>(std::count_if(s.events.begin(), s.events.end(),
                                                  [](const auto& e) { return std::holds_alternative<LaneChangeEvent>(e); }));
    }
    CHECK(changes > 0); // the forcing: there were changes whose records the blind run dropped
}
TEST(discretionary, no_change_inside_a_conflict_area_or_while_serving_a_stop) { // A54
    auto scenario = road();
    scenario.segments.push_back({"cross", 200, {}});
    scenario.routes.push_back({"crossRoute", {"cross"}});
    scenario.conflictZones = {{"zone", {{"l2"}, 35, 45}, {{"cross"}, 98, 102}, 90, 3, 10}};
    auto s = test::withVehicles(scenario, overtaking());
    // The forcing: only the target route crosses the area, and the gain alone would change.
    CHECK(s.index->routeZones[find(s, 2)->routeIndex].empty() && gainOf(s) >= 0.5);
    CHECK(decide(s).empty());
    auto served = test::withVehicles(road(), overtaking());
    CHECK(decide(served).size() == 1); // the forcing: unserved, it changes
    CHECK(decide(served, {{2, 100, 0}}).empty());
}
TEST(discretionary, the_four_leg_and_m2_6_spans_join_full_routes_ending_on_one_link) { // A50
    for (const auto* name : {"four-leg-signalised", "m2.6-study-template"}) {
        std::ifstream file(test::root() / "data/projects" / (std::string(name) + ".traffic.json"));
        Json j; file >> j;
        const auto document = parseDocument(j);
        const auto s = compileDocument(document, test::root() / "data").scenario;
        CHECK(validateScenario(s).empty());
        std::map<std::string, bool> stub;
        for (const auto& r : s.routes) stub[r.id] = false;
        for (const auto& d : s.routeDeadEnds) stub[d.routeId] = true;
        const auto table = runtimeSections(document.network);
        const auto lastLink = [&](const std::string& route) {
            for (const auto& r : s.routes) if (r.id == route)
                for (const auto& section : table.sections) if (section.id == r.segmentIds.back()) return section.linkId;
            return std::string{};
        };
        std::size_t discretionary = 0;
        for (const auto& span : s.laneChanges) {
            if (stub[span.fromRouteId]) continue;
            ++discretionary;
            CHECK(!stub[span.toRouteId]); // never from a full route into a stub
            CHECK(!lastLink(span.fromRouteId).empty() && lastLink(span.fromRouteId) == lastLink(span.toRouteId));
            // Both ways: every discretionary span has its mirror.
            CHECK(std::any_of(s.laneChanges.begin(), s.laneChanges.end(), [&](const auto& x) {
                return x.fromRouteId == span.toRouteId && x.toRouteId == span.fromRouteId; }));
        }
        CHECK(discretionary > 0); // the forcing: each drawing has two-lane full routes
    }
}
