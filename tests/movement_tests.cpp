#include "test.hpp"
#include "../src/eval/movement.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include <fstream>
using namespace trafficsim;
// M2.5. The analytic cases pin the measurement definitions by hand; the engine cases pin that
// the engine feeds them what they expect; the four-leg case is M2's done-condition.
namespace {
constexpr double kmh = 1 / 3.6;
EvaluationSpec oneLane() {
    EvaluationSpec spec;
    spec.movementNames = {"in → out"};
    spec.movementOfRoute = {{"route", 0}};
    spec.counters = {{"approach", {{"road", 100}}}}; // the head "h" stop line
    spec.queue = {5 * kmh, 10 * kmh, 20};
    return spec;
}
Scenario redRoad(double red) {
    auto s = test::straight(); s.duration = 60;
    s.signalPrograms = {{"p", 0, {{600, SignalColor::green}}}};
    if (red > 0) s.signalPrograms[0].phases.insert(s.signalPrograms[0].phases.begin(), {red, SignalColor::red});
    s.signalHeads = {{"h", "road", 100, "p"}};
    return s;
}
MovementReport observeRun(SimState state, const EvaluationSpec& spec) {
    MovementAccumulator m(spec); m.observe(state);
    while (state.tick < totalTicks(*state.scenario)) { state = stepSimulation(state); m.observe(state); }
    return m.report(state);
}
Json committedFourLeg() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j; return j;
}
}
TEST(movement, queue_length_by_hand) {
    // Three stopped 4.5 m cars, fronts 1, 7.5 and 14 m before the line: the queue reaches the
    // last one's rear, 14 + 4.5 = 18.5 m.
    CHECK(queueLength({{7.5, 4.5, true}, {1, 4.5, true}, {14, 4.5, true}}, 20) == 18.5);
    // A clear gap over maxGap ends it: 5.5 -> 30 leaves 24.5 m of open road.
    CHECK(queueLength({{1, 4.5, true}, {30, 4.5, true}}, 20) == 5.5);
    // The first vehicle's gap is measured from the line itself.
    CHECK(queueLength({{25, 4.5, true}}, 20) == 0);
    // A moving vehicle ends the queue even with queued ones behind it.
    CHECK(queueLength({{1, 4.5, true}, {7.5, 4.5, false}, {14, 4.5, true}}, 20) == 5.5);
    // A vehicle whose front has passed the line is not in this queue.
    CHECK(queueLength({{-2, 4.5, true}, {3, 4.5, true}}, 20) == 7.5);
    CHECK(queueLength({}, 20) == 0);
}
TEST(movement, queue_state_has_hysteresis) {
    // Vissim's queue conditions: join below 5 km/h, leave only above 10 km/h.
    const auto s = redRoad(600);
    const auto measure = [&](double speed) {
        auto state = test::withVehicles(s, {test::vehicle(1, 99, speed)});
        state.events.clear();
        return state;
    };
    const auto spec = oneLane();
    MovementAccumulator m(spec);
    m.observe(measure(7 * kmh));   // never slow enough to join
    CHECK(m.report(measure(0)).queues[0].maxLength == 0);
    m.observe(measure(4 * kmh));   // joins
    m.observe(measure(8 * kmh));   // between the thresholds: stays
    m.observe(measure(11 * kmh));  // leaves
    const auto r = m.report(measure(0));
    CHECK(r.queues[0].maxLength == 5.5);
    // Four observations, two of them queued at 5.5 m.
    test::near(r.queues[0].meanLength, 2 * 5.5 / 4);
}
TEST(movement, delay_by_hand_and_it_adds_up_to_the_run_summary) {
    auto spec = oneLane();
    spec.movementNames.push_back("other");
    spec.movementOfRoute["route/lane-1"] = 1;
    auto state = test::withVehicles(test::straight(), {});
    state.events = {ArrivedEvent{1, 1, "route", 30, 2, 20}, ArrivedEvent{2, 2, "route", 25, 0, 20},
                    ArrivedEvent{3, 3, "elsewhere", 40, 0, 20}, SafetyClampEvent{3, 4}};
    MovementAccumulator m(spec); m.observe(state);
    const auto r = m.report(state);
    // (30 + 2 - 20) and (25 + 0 - 20): 12 and 5 s, mean 8.5; travel (30 + 25) / 2.
    CHECK(r.movements[0].vehicles == 2); test::near(*r.movements[0].meanDelay, 8.5);
    test::near(*r.movements[0].meanTravelTime, 27.5);
    CHECK(r.movements[1].vehicles == 0); CHECK(!r.movements[1].meanDelay);
    // A trip on a route no movement names is counted, never silently dropped.
    CHECK(r.unassigned == 1); CHECK(r.completed == 3); CHECK(r.safetyClamps == 1);
    test::near(*r.meanDelay, (12 + 5 + 20) / 3.0);
    CHECK(tripDelay(ArrivedEvent{0, 0, "", 10, 0, 20}) == 0); // faster than free flow is not negative
}
TEST(movement, a_standing_queue_measured_from_the_engine) {
    // Red for the whole run: four placed cars drive up and stop. Their stopping positions come
    // from car-following, so the bound is physical rather than exact: at least four lengths plus
    // three standstill gaps, and no more slack than a metre per vehicle beyond that.
    const auto s = redRoad(600);
    auto end = test::withVehicles(s, {test::vehicle(1, 60), test::vehicle(2, 45), test::vehicle(3, 30), test::vehicle(4, 15)});
    const auto r = observeRun(end, oneLane());
    const auto final = test::finish(end);
    CHECK(final.vehicles.size() == 4);
    // All four are in queue state (below 5 km/h) by the end; car-following creeps them together.
    for (const auto& v : final.vehicles) CHECK(v.speed < 5 * kmh);
    MovementAccumulator last(oneLane()); last.observe(final);
    const double standing = last.report(final).queues[0].maxLength;
    const double tight = 4 * 4.5 + 3 * 2;
    CHECK(standing >= tight); CHECK(standing <= tight + 4 * 1.0 + 1);
    // While it forms, the queue is longer than when it has compressed: the maximum says so.
    CHECK(r.queues[0].maxLength >= standing);
    CHECK(r.pending == 0); CHECK(r.active == 4); CHECK(r.movements[0].vehicles == 0);
}
TEST(movement, delay_responds_to_red_time) {
    auto spec = oneLane();
    auto run = [&](double red) {
        auto s = redRoad(red); s.duration = 200; s.inputs[0].endTime = 120;
        return observeRun(createSimulation(s, 42), spec);
    };
    const auto green = run(0), red60 = run(60);
    CHECK(green.movements[0].vehicles > 10); CHECK(red60.movements[0].vehicles == green.movements[0].vehicles);
    // Unimpeded, a trip still carries its entry acceleration: a vehicle enters from standstill,
    // and v/(2a) = 15/(2 x 2.5) = 3 s of it is delay against the desired-speed free-flow time.
    CHECK(*green.movements[0].meanDelay > 2.5); CHECK(*green.movements[0].meanDelay < 4.5);
    CHECK(*red60.movements[0].meanDelay > *green.movements[0].meanDelay + 10);
    CHECK(green.queues[0].maxLength == 0); CHECK(red60.queues[0].maxLength > 20);
    // Exact replay per seed (hard rule 2).
    CHECK(run(60) == red60);
}
TEST(movement, four_leg_movement_table) {
    const auto d = parseDocument(committedFourLeg());
    const auto data = test::root() / "data";
    const auto snapshot = compileDocument(d, data);
    const auto spec = evaluationSpec(d, snapshot, data);
    CHECK(spec.movementNames.size() == 12); CHECK(spec.counters.size() == 4);
    CHECK(spec.movementNames.front() == "West approach → East exit");
    CHECK(spec.counters.front().name == "West approach, right-turn pocket");
    CHECK(spec.counters.front().lines.size() == 3);
    CHECK(spec.movementOfRoute.size() == snapshot.scenario.routes.size()); // every lane route mapped
    const auto r = observeRun(createSimulation(snapshot.scenario, 42), spec);
    for (const auto& m : r.movements) { CHECK(m.vehicles > 0); CHECK(*m.meanDelay > 0); }
    for (const auto& q : r.queues) CHECK(q.maxLength > 0);
    CHECK(r.unassigned == 0);
    std::uint64_t sum = 0; for (const auto& m : r.movements) sum += m.vehicles;
    CHECK(sum == r.completed);
    CHECK(observeRun(createSimulation(snapshot.scenario, 42), spec) == r);
    const auto csv = movementCsv(r);
    CHECK(csv.starts_with("# TrafficSim - not yet validated. Simulated movement delay, not HCM control delay"));
    CHECK(movementJson(r)["validated"] == false);
}
TEST(movement, queue_definition_is_data) {
    const auto q = loadQueueDefinition(test::root() / "data");
    test::near(q.beginSpeed, 5 / 3.6); test::near(q.endSpeed, 10 / 3.6); CHECK(q.maxGap == 20);
    // The forcing first: this directory really has no queue-counter file.
    const auto empty = test::root() / "tests";
    CHECK(!std::filesystem::exists(empty / "evaluation" / "queue-counter.json"));
    test::throws([&] { loadQueueDefinition(empty); }, "EDIT_CATALOG_READ");
}
