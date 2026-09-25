#include "test.hpp"
#include "../src/core/following.hpp"
#include "../src/core/routes.hpp"
#include <algorithm>
#include <limits>

using namespace trafficsim;
namespace {
std::vector<SimEvent> events(const Scenario& s, std::uint32_t seed) {
    std::vector<SimEvent> result;
    runSimulation(s, seed, [&](const auto& event) { result.push_back(event); });
    return result;
}
Scenario signalScenario() {
    auto s = test::straight();
    s.signalPrograms = {{"p", 0, {{30, SignalColor::red}, {90, SignalColor::green}}}};
    s.signalHeads = {{"h", "road", 60, "p"}};
    return s;
}
}
TEST(core, deterministic_replay) {
    const auto s = test::demo().scenario;
    const auto reference = events(s, 42);
    CHECK(events(s, 42) == reference);
    CHECK(events(s, 43) != reference);
}
TEST(core, canonical_order) {
    auto s = test::demo().scenario;
    const auto reference = events(s, 7);
    std::reverse(s.segments.begin(), s.segments.end()); std::reverse(s.routes.begin(), s.routes.end());
    std::reverse(s.inputs.begin(), s.inputs.end()); std::reverse(s.signalHeads.begin(), s.signalHeads.end());
    std::reverse(s.signalPrograms.begin(), s.signalPrograms.end());
    CHECK(events(s, 7) == reference);
}
TEST(core, pure_steps_and_owned_snapshot) {
    auto s = test::straight();
    const auto initial = createSimulation(s, 7);
    const auto before = checkpointJson(initial);
    s.inputs[0].vehiclesPerHour = 0;
    CHECK(initial.scenario->inputs[0].vehiclesPerHour == 900);
    CHECK(checkpointJson(stepSimulation(initial)) == checkpointJson(stepSimulation(initial)));
    CHECK(checkpointJson(initial) == before);
    const auto control = events(test::straight(), 2);
    auto editable = test::straight();
    std::vector<SimEvent> result;
    runSimulation(editable, 2, [&](const auto& event) { result.push_back(event); editable.duration = 1; });
    CHECK(result == control);
}
TEST(core, streaming_equals_stepping_and_final_tick) {
    auto s = test::straight(); s.duration = 60;
    auto state = createSimulation(s, 0);
    auto all = state.events;
    while (state.time < s.duration) {
        state = stepSimulation(state);
        all.insert(all.end(), state.events.begin(), state.events.end());
    }
    CHECK(all == events(s, 0)); CHECK(state.tick == 600); CHECK(state.time == 60);
    CHECK(checkpointJson(stepSimulation(state)) == checkpointJson(state));
    test::throws([&] { stepSimulation(state, 0.2); }, "timeStep");
}
TEST(core, final_subinterval_demand) {
    auto s = test::straight(); s.duration = 1; s.inputs[0].endTime = 1;
    auto state = createSimulation(s, 1);
    state.tick = 9; state.time = 0.9; state.inputs = {{0.95, {}}};
    state = stepSimulation(state);
    CHECK(!state.inputs[0].queue.empty()); CHECK(state.inputs[0].queue[0].scheduledTime == 0.95);
    CHECK(state.vehicles.empty()); CHECK(state.nextVehicleId > 1);
}
TEST(core, acceleration_and_stop_integration) {
    auto s = test::straight(); s.segments[0].length = 2000;
    auto state = test::withVehicles(s, {test::vehicle(1, 0)});
    for (int i = 0; i < 300; ++i) {
        const auto previous = state.vehicles[0]; state = stepSimulation(state);
        CHECK(state.vehicles[0].distance >= previous.distance);
        CHECK(state.vehicles[0].speed >= previous.speed - 1e-10);
        CHECK(state.vehicles[0].speed <= 15);
    }
    test::near(state.vehicles[0].speed, 15, 1e-4);
    const auto stop = integrate(1, -8, 0.5); CHECK(stop.speed == 0); CHECK(stop.distance == 0.0625);
}
TEST(core, short_connectors) {
    auto s = test::straight();
    s.segments = {{"road", 10, {"connector"}}, {"connector", 0.1, {"exit"}}, {"exit", 100, {}}};
    s.routes = {{"route", {"road", "connector", "exit"}}};
    const auto state = stepSimulation(test::withVehicles(s, {test::vehicle(1, 9.5, 15)}));
    CHECK(state.vehicles[0].distance == 11);
    std::vector<std::string> crossed;
    for (const auto& e : state.events) if (const auto* part = std::get_if<SegmentEnteredEvent>(&e)) crossed.push_back(part->segmentId);
    CHECK(crossed == std::vector<std::string>({"connector", "exit"}));
    const auto location = locateVehicle(*state.scenario, state.vehicles[0]);
    CHECK(location.segmentId == "exit"); test::near(location.position, 0.9);
}
TEST(core, upstream_tail_and_gap) {
    auto s = test::straight();
    s.segments = {{"road", 100, {"connector"}}, {"connector", 2, {"exit"}}, {"exit", 100, {}}};
    s.routes = {{"route", {"road", "connector", "exit"}}};
    auto state = test::withVehicles(s, {test::vehicle(1, 102.5), test::vehicle(2, 95, 15)});
    const auto spans = occupiedSpans(s, state.vehicles);
    CHECK(std::any_of(spans.begin(), spans.end(),
        [&](const auto& x) { return x.vehicleId == 1 && s.segments[x.segmentIndex].id == "road"; }));
    for (int i = 0; i < 80; ++i) {
        state = stepSimulation(state);
        if (state.vehicles.size() == 2) CHECK(state.vehicles[0].distance - 4.5 - state.vehicles[1].distance >= 2 - 1e-8);
    }
}
TEST(core, half_open_signals) {
    const SignalProgram p{"p", 2, {{3, SignalColor::red}, {5, SignalColor::green}}};
    CHECK(signalColorAt(p, 0) == SignalColor::red); CHECK(signalColorAt(p, 1) == SignalColor::green);
    CHECK(signalColorAt(p, 6) == SignalColor::red); CHECK(signalColorAt(p, 14) == SignalColor::red);
}
TEST(core, red_queue_and_green_discharge) {
    auto state = test::withVehicles(signalScenario(), {test::vehicle(1, 0), test::vehicle(2, -10)});
    while (state.time < 30) {
        state = stepSimulation(state);
        for (const auto& v : state.vehicles) CHECK(v.distance <= 60 + 1e-8);
    }
    CHECK(state.vehicles[0].speed < 0.1); CHECK(test::finish(state).completed == 2);
    state = stepSimulation(test::withVehicles(signalScenario(), {test::vehicle(1, 61, 10)}));
    CHECK(state.vehicles[0].speed > 10);
}
TEST(core, leader_gap_and_nearby_red) {
    auto s = signalScenario(); s.timeStep = 0.5; s.signalHeads[0].position = 5;
    const auto state = stepSimulation(test::withVehicles(s, {test::vehicle(1, 10.5), test::vehicle(2, 0, 15)}));
    CHECK(state.vehicles[1].distance <= 4);
}
TEST(core, signal_visible_through_connector) {
    auto s = signalScenario();
    s.segments = {{"road", 10, {"connector"}}, {"connector", 0.1, {"exit"}}, {"exit", 100, {}}};
    s.routes = {{"route", {"road", "connector", "exit"}}}; s.signalHeads[0].segmentId = "exit"; s.signalHeads[0].position = 0;
    const auto state = stepSimulation(test::withVehicles(s, {test::vehicle(1, 9.8, 15)}));
    CHECK(state.vehicles[0].distance <= 10.1); CHECK(state.vehicles[0].speed == 0);
    CHECK(std::any_of(state.events.begin(), state.events.end(), [](const auto& e) { return std::holds_alternative<SafetyClampEvent>(e); }));
}
TEST(core, blocked_demand_conservation_and_nonoverlap) {
    auto s = signalScenario(); s.duration = 60; s.segments[0].length = 30;
    s.inputs[0].vehiclesPerHour = 1800; s.signalPrograms[0].phases = {{60, SignalColor::red}}; s.signalHeads[0].position = 20;
    auto state = createSimulation(s, 3);
    while (state.time < 60) {
        state = stepSimulation(state);
        auto vehicles = state.vehicles;
        std::sort(vehicles.begin(), vehicles.end(), [](const auto& a, const auto& b) { return a.distance > b.distance; });
        for (std::size_t i = 1; i < vehicles.size(); ++i) CHECK(vehicles[i - 1].distance - 4.5 - vehicles[i].distance >= 2 - 1e-8);
        for (const auto& v : vehicles) CHECK(v.distance <= 20);
        CHECK(state.completed + state.vehicles.size() + pendingCount(state) == state.nextVehicleId - 1);
    }
    CHECK(pendingCount(state) > 0); CHECK(state.completed == 0);
}
TEST(core, zero_demand_and_null_summary) {
    auto s = test::straight(); s.inputs[0].vehiclesPerHour = 0;
    const auto state = runSimulation(s, 42);
    CHECK(state.vehicles.empty()); CHECK(state.nextVehicleId == 1); CHECK(!state.inputs[0].nextArrival);
    CHECK(!SummaryAccumulator{}.summary().meanDelay);
}
TEST(core, input_validation) {
    for (const double dt : {0.0, -1.0, 1.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
        auto s = test::straight(); s.timeStep = dt;
        test::throws([&] { createSimulation(s, 1); });
    }
    auto s = test::straight(); s.duration = 1.05; s.inputs.clear();
    test::throws([&] { createSimulation(s, 1); }, "OFF_TIME_GRID");
    s = test::straight(); s.routes[0].segmentIds.push_back("missing");
    test::throws([&] { createSimulation(s, 1); }, "DISCONNECTED_ROUTE");
    s.routes[0].segmentIds = {"road", "road"};
    test::throws([&] { createSimulation(s, 1); }, "UNSUPPORTED_ROUTE_CYCLE");
    s = test::straight(); s.segments = {{"road", 10, {"exit"}}, {"other", 10, {"exit"}}, {"exit", 10, {}}};
    test::throws([&] { createSimulation(s, 1); }, "UNSUPPORTED_MERGE");
    s.segments = {{"road", 10, {"exit"}}, {"exit", 10, {}}}; s.routes = {{"route", {"exit"}}};
    test::throws([&] { createSimulation(s, 1); }, "UNSUPPORTED_INTERNAL_INPUT");
}
TEST(core, invalid_control_and_parameter_ranges) {
    auto s = signalScenario(); s.signalPrograms[0].offset = 0.05;
    test::throws([&] { createSimulation(s, 1); }, "OFF_TIME_GRID");
    s = test::straight(); s.vehicleTypes[0].desiredSpeed = {20, 10};
    s.inputs[0].routeId = "bad"; s.inputs[0].vehiclesPerHour = -1; s.inputs[0].endTime = 121;
    s.signalHeads = {{"head", "road", 999, "bad"}};
    const auto issues = validateScenario(s);
    for (const auto* code : {"INVALID_RANGE", "UNKNOWN_ROUTE", "INVALID_NUMBER", "INVALID_INTERVAL", "UNKNOWN_SIGNAL_PROGRAM", "INVALID_POSITION"})
        CHECK(std::any_of(issues.begin(), issues.end(), [&](const auto& issue) { return issue.code == code; }));
}
namespace {
// Two approaches feeding one place: the shape the engine refused outright before M3.1. The minor
// approach waits at the end of its own segment; the conflict point is where the major approach
// reaches the merge.
Scenario mergeScenario(double gapTime, double headway, bool withRule = true) {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 120; s.timeStep = 0.1;
    s.segments = {{"major", 150, {"merged"}}, {"minor", 100, {"merged"}}, {"merged", 150, {}}};
    s.routes = {{"majorRoute", {"major", "merged"}}, {"minorRoute", {"minor", "merged"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    if (withRule) s.priorityRules = {{"give-way", "minor", 100, "major", 150, gapTime, headway}};
    return s;
}
test::Placement on(std::uint64_t id, const char* route, double distance, double speed) {
    return {id, route, distance, speed};
}
double distanceOf(const SimState& state, std::uint64_t id) {
    for (const auto& v : state.vehicles) if (v.id == id) return v.distance;
    return -1; // Left the network.
}
}
// M3.1. Merge arbitration by gap time and headway -- the two numbers an engineer tunes. Without
// this the engine had no answer at a merge at all, which is why it refused to run one.
TEST(core, a_minor_approach_gives_way_and_then_goes) {
    // A major vehicle 90 m short of the conflict point at 10 m/s -- nine seconds away, inside a
    // thirty-second gap time, so it blocks for long enough to be worth measuring. A one-second
    // block is NOT enough: from 95 m at rest, an unyielding vehicle would not have reached the
    // stop line within it anyway, and the assertion would hold whether the clamp existed or not.
    auto blocked = test::withVehicles(mergeScenario(30, 10), {on(1, "minorRoute", 95, 0),
                                                              on(2, "majorRoute", 60, 10)});
    // The forcing: the major vehicle really is approaching the conflict point during the wait. If
    // it had already left, the minor vehicle standing still would prove nothing about yielding.
    CHECK(distanceOf(blocked, 2) > 0);
    bool majorWasApproaching = false;
    for (int i = 0; i < 60; ++i) {
        blocked = stepSimulation(blocked);
        // Held at the stop line, never across it, for as long as the major vehicle is short of
        // the conflict point.
        if (distanceOf(blocked, 2) > 0 && distanceOf(blocked, 2) < 150) {
            majorWasApproaching = true;
            CHECK(distanceOf(blocked, 1) <= 100 + 1e-9);
        }
    }
    // Six seconds of blocking really did happen, and six seconds is comfortably longer than the
    // minor vehicle needs to cross 5 m from rest -- so the assertion above had something to catch.
    CHECK(majorWasApproaching);
    // With the major approach empty, the same vehicle from the same place crosses freely.
    auto clear = test::withVehicles(mergeScenario(3, 10), {on(1, "minorRoute", 95, 0)});
    for (int i = 0; i < 60; ++i) clear = stepSimulation(clear);
    CHECK(distanceOf(clear, 1) > 100);
    // The negative that matters: the rule is READ, not ignored. A zero gap time and headway with
    // the identical blocking traffic must let the vehicle out, and a long one must hold it.
    auto permissive = test::withVehicles(mergeScenario(0, 0), {on(1, "minorRoute", 95, 0),
                                                               on(2, "majorRoute", 140, 10)});
    auto strict = test::withVehicles(mergeScenario(30, 10), {on(1, "minorRoute", 95, 0),
                                                             on(2, "majorRoute", 60, 10)});
    for (int i = 0; i < 60; ++i) { permissive = stepSimulation(permissive); strict = stepSimulation(strict); }
    CHECK(distanceOf(permissive, 1) > 100);
    CHECK(distanceOf(strict, 1) <= 100 + 1e-9);
}
// A stopped major vehicle far from the conflict point must not hold the minor approach for ever:
// a queue that is not moving is a gap, and treating it as a block is a deadlock.
TEST(core, a_standing_queue_upstream_does_not_deadlock_the_minor_approach) {
    auto state = test::withVehicles(mergeScenario(3, 10), {on(1, "minorRoute", 95, 0),
                                                           on(2, "majorRoute", 60, 0)});
    // The forcing: the major vehicle is genuinely stopped and genuinely still on the segment.
    CHECK(distanceOf(state, 2) == 60);
    for (int i = 0; i < 80; ++i) state = stepSimulation(state);
    CHECK(distanceOf(state, 1) > 100);
}
// The guard is loosened by construction, never by removal.
TEST(core, a_merge_without_a_priority_rule_is_still_rejected) {
    test::throws([&] { assertValidScenario(mergeScenario(3, 10, false)); }, "UNSUPPORTED_MERGE");
    // And a rule pointing somewhere else does not count as arbitration of this merge.
    auto elsewhere = mergeScenario(3, 10, false);
    elsewhere.priorityRules = {{"wrong", "minor", 100, "merged", 10, 3, 10}};
    test::throws([&] { assertValidScenario(elsewhere); }, "UNSUPPORTED_MERGE");
    // A segment cannot give way to itself.
    auto itself = mergeScenario(3, 10, false);
    itself.priorityRules = {{"self", "minor", 100, "minor", 10, 3, 10}};
    test::throws([&] { assertValidScenario(itself); }, "INVALID_RANGE");
    // The arbitrated merge is accepted -- the forcing for all three negatives above.
    CHECK(validateScenario(mergeScenario(3, 10)).empty());
}
TEST(core, a_priority_rule_is_validated_against_the_segments_it_names) {
    auto unknown = mergeScenario(3, 10);
    unknown.priorityRules[0].conflictSegmentId = "nowhere";
    test::throws([&] { assertValidScenario(unknown); }, "UNKNOWN_SEGMENT");
    auto offSegment = mergeScenario(3, 10);
    offSegment.priorityRules[0].yieldPosition = 500;
    test::throws([&] { assertValidScenario(offSegment); }, "INVALID_POSITION");
    auto negative = mergeScenario(3, 10);
    negative.priorityRules[0].gapTime = -1;
    test::throws([&] { assertValidScenario(negative); }, "INVALID_NUMBER");
}
// Hard rule 2 again, now with a merge in the network.
TEST(core, the_same_seed_produces_the_same_merge) {
    auto s = mergeScenario(3, 10);
    s.inputs = {{"majorIn", "majorRoute", "car", 600, 0, 60}, {"minorIn", "minorRoute", "car", 600, 0, 60}};
    const auto reference = events(s, 42);
    CHECK(events(s, 42) == reference);
    CHECK(events(s, 43) != reference);
    // The forcing: vehicles actually got through the merge, so the stream is not an empty run.
    CHECK(runSimulation(s, 42).completed > 0);
    // And a tighter gap time is a different run, which is what makes the rule observable at all.
    auto tighter = s; tighter.priorityRules[0].gapTime = 12;
    CHECK(events(tighter, 42) != reference);
}
// M3.2.2c. A stop line may stand on the approach before the segment that gives way -- a waiting
// line on the Link before a Connector -- as far back as every route onto that segment must come.
TEST(core, a_stop_line_may_stand_on_the_one_approach_before_the_yielding_segment) {
    auto s = mergeScenario(30, 10);
    s.segments = {{"major", 150, {"merged"}}, {"approach", 80, {"minor"}}, {"minor", 100, {"merged"}}, {"merged", 150, {}}};
    s.routes = {{"majorRoute", {"major", "merged"}}, {"minorRoute", {"approach", "minor", "merged"}}};
    s.priorityRules[0].yieldPosition = -30; // approach station 50: route distance 50
    CHECK(validateScenario(s).empty());
    const auto refusesPosition = [](const Scenario& x) {
        const auto issues = validateScenario(x);
        return std::any_of(issues.begin(), issues.end(), [](const auto& i) { return i.code == "INVALID_POSITION"; });
    };
    auto past = s; past.priorityRules[0].yieldPosition = -80.5; // beyond the start of the approach
    CHECK(refusesPosition(past));
    // A second way onto the yielding segment: a vehicle arriving by it never crosses the line.
    auto bypass = s; bypass.segments.push_back({"side", 40, {"minor"}});
    CHECK(refusesPosition(bypass));
    auto onSegment = bypass; onSegment.priorityRules[0].yieldPosition = 0;
    CHECK(!refusesPosition(onSegment)); // the forcing: it is the position that is refused, not the network
    // It holds: the minor vehicle stays short of the line while the major one approaches...
    auto held = test::withVehicles(s, {on(1, "minorRoute", 30, 0), on(2, "majorRoute", 60, 10)});
    // ...where the same vehicle with the line at the old place, the yielding segment's end, is
    // already past it in the same time -- so the check below had something to catch.
    auto late = s; late.priorityRules[0].yieldPosition = 100;
    auto free = test::withVehicles(late, {on(1, "minorRoute", 30, 0), on(2, "majorRoute", 60, 10)});
    bool majorWasApproaching = false;
    for (int i = 0; i < 60; ++i) {
        held = stepSimulation(held); free = stepSimulation(free);
        if (distanceOf(held, 2) > 0 && distanceOf(held, 2) < 150) {
            majorWasApproaching = true;
            CHECK(distanceOf(held, 1) <= 50 + 1e-9);
        }
    }
    CHECK(majorWasApproaching);
    CHECK(distanceOf(free, 1) > 50);
}
