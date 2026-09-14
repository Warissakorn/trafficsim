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
    state.tick = 9; state.time = 0.9; state.inputs = {{"input", 0.95, {}}};
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
