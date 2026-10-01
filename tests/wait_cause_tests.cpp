#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/eval/dead_end_waits.hpp"
#include "../src/eval/lane_changes.hpp"
#include <algorithm>
#include <numeric>
using namespace trafficsim;
// M3.2.8c (D92): the wait-cause diagnostic classifies each dead-end wait by the snapshot that
// starts it, on the three-lane road of the lane-change tests.
namespace {
using test::lanes; using test::on;
std::size_t slot(const SimState& s, std::uint64_t id) {
    const auto it = std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
    return static_cast<std::size_t>(it - s.vehicles.begin());
}
bool waiting(const SimState& s, std::uint64_t id) {
    const auto v = slot(s, id);
    if (v == s.vehicles.size()) return false;
    const auto& x = s.vehicles[v];
    return waitingAtDeadEnd(*s.index, x.routeIndex, x, s.scenario->behaviours[s.index->behaviourOfType[x.typeIndex]]);
}
// The snapshot in which vehicle `id` first waits at its dead end; the run's end if never.
SimState firstWait(SimState s, std::uint64_t id) {
    while (!waiting(s, id) && s.tick < totalTicks(*s.scenario)) s = stepSimulation(s);
    return s;
}
// Speeds of the lane-b vehicles whose front lies in [lo, hi].
std::vector<double> speedsOnB(const SimState& s, double lo, double hi) {
    std::vector<double> speeds;
    for (const auto& v : s.vehicles)
        if (s.scenario->routes[v.routeIndex].id == "full" && v.distance >= lo && v.distance <= hi) speeds.push_back(v.speed);
    return speeds;
}
struct Both { WaitCauseReport causes; LaneChangeReport changes; };
Both diagnose(SimState s) {
    const EvaluationSpec spec{{"through"}, {{"full", 0}}, {}, {}};
    DeadEndWaitAccumulator causes(spec);
    LaneChangeAccumulator changes(spec);
    causes.observe(s); changes.observe(s);
    while (s.tick < totalTicks(*s.scenario)) { s = stepSimulation(s); causes.observe(s); changes.observe(s); }
    return {causes.report(), changes.report()};
}
std::uint64_t waits(const WaitCauseRow& row, WaitCause c) { return row.waits[static_cast<std::size_t>(c)]; }
double total(const WaitCauseReport& r) {
    double sum = 0;
    for (const auto& row : r.rows) sum += std::accumulate(row.seconds.begin(), row.seconds.end(), 0.0);
    return sum;
}
double total(const LaneChangeReport& r) {
    double sum = 0;
    for (const auto& row : r.rows) sum += row.waitSeconds;
    return sum;
}
// A standing queue on lane b, held by a red at 199 for `red` seconds, from 198 back 7 m a vehicle.
Scenario heldB(double red, std::vector<test::Placement>& placed, std::size_t queue) {
    auto s = lanes();
    s.signalPrograms = {{"b-red", 0, {{red, SignalColor::red}, {120 - red, SignalColor::green}}}};
    s.signalHeads = {{"head-b", "b", 199, "b-red"}};
    for (std::uint64_t k = 0; k < queue; ++k) placed.push_back(on(10 + k, "full", 198 - 7 * static_cast<double>(k), 0));
    return s;
}
}
TEST(lanechange, a_wait_beside_a_standing_target_lane_is_target_standing) {
    std::vector<test::Placement> placed{on(1, "stubA", 100, 10)};
    const auto s = test::withVehicles(heldB(40, placed, 15), placed);
    // The forcing: it moved to its dead end and waits there, beside a lane b that stands.
    const auto start = firstWait(s, 1);
    CHECK(waiting(start, 1));
    const auto beside = speedsOnB(start, 180, 200);
    CHECK(!beside.empty() && std::all_of(beside.begin(), beside.end(), [](double v) { return v < kWaitingSpeed; }));
    const auto r = diagnose(s);
    CHECK(waits(r.causes.rows[0], WaitCause::targetStanding) == 1);
    CHECK(waits(r.causes.rows[0], WaitCause::noMovingApproach) == 0 && waits(r.causes.rows[0], WaitCause::movingStream) == 0);
    test::near(total(r.causes), total(r.changes)); // the same waits as --lane-changes
}
TEST(lanechange, a_wait_with_no_moving_approach_is_no_moving_approach) {
    // Already standing at its dead end when first observed, beside a lane b held for 110 s.
    std::vector<test::Placement> placed{on(1, "stubA", 197, 0)};
    const auto s = test::withVehicles(heldB(110, placed, 28), placed);
    CHECK(waiting(s, 1)); // the forcing: it waits in the very first snapshot, never having moved
    const auto r = diagnose(s);
    CHECK(waits(r.causes.rows[0], WaitCause::noMovingApproach) + waits(r.causes.rows[1], WaitCause::noMovingApproach) == 1);
    CHECK(waits(r.causes.rows[0], WaitCause::targetStanding) + waits(r.causes.rows[1], WaitCause::targetStanding) == 0);
    test::near(total(r.causes), total(r.changes));
}
TEST(lanechange, standing_in_its_own_lanes_queue_first_does_not_make_it_no_moving_approach) {
    // Lane a has its own red at 150 for 30 s; lane b stands at a red for 110 s. Every M2.6 wait
    // stood in its own queue first (D92), so that alone does not separate one wait from another.
    std::vector<test::Placement> placed{on(1, "stubA", 60, 10)};
    auto scenario = heldB(110, placed, 28);
    scenario.signalPrograms.push_back({"a-red", 0, {{30, SignalColor::red}, {90, SignalColor::green}}});
    scenario.signalHeads.push_back({"head-a", "a", 150, "a-red"});
    const auto s = test::withVehicles(scenario, placed);
    // The forcing: it stands at lane a's red, short of its dead end and not waiting there ...
    auto atRed = s;
    while (atRed.time < 25) atRed = stepSimulation(atRed);
    const auto& stood = atRed.vehicles[slot(atRed, 1)];
    CHECK(stood.speed < kWaitingSpeed && stood.distance < 155 && !waiting(atRed, 1));
    // ... and later, after driving up to its dead end, waits there beside the standing lane b.
    CHECK(waiting(firstWait(atRed, 1), 1));
    const auto r = diagnose(s);
    CHECK(waits(r.causes.rows[0], WaitCause::targetStanding) + waits(r.causes.rows[1], WaitCause::targetStanding) == 1);
    CHECK(waits(r.causes.rows[0], WaitCause::noMovingApproach) + waits(r.causes.rows[1], WaitCause::noMovingApproach) == 0);
    test::near(total(r.causes), total(r.changes));
}
TEST(lanechange, a_wait_beside_a_moving_stream_is_moving_stream) {
    // A36's road without cooperative braking: a 10 m/s stream with 7.5 m gaps refuses the change.
    auto scenario = lanes();
    for (auto& b : scenario.behaviours) b.maxDecelerationCooperativeBraking.reset();
    std::vector<test::Placement> placed{on(1, "stubA", 120, 10)};
    for (std::uint64_t k = 0; k < 15; ++k) placed.push_back(on(10 + k, "full", 190 - 12 * static_cast<double>(k), 10));
    const auto s = test::withVehicles(scenario, placed);
    // The forcing: it waits, and at that moment lane b alongside and behind its place is moving.
    const auto start = firstWait(s, 1);
    CHECK(waiting(start, 1));
    const auto& at = start.vehicles[slot(start, 1)];
    const auto behind = speedsOnB(start, at.distance - 30, at.distance + 5);
    CHECK(!behind.empty() && std::all_of(behind.begin(), behind.end(), [](double v) { return v >= kWaitingSpeed; }));
    const auto r = diagnose(s);
    CHECK(waits(r.causes.rows[0], WaitCause::movingStream) + waits(r.causes.rows[1], WaitCause::movingStream) >= 1);
    CHECK(waits(r.causes.rows[0], WaitCause::noMovingApproach) + waits(r.causes.rows[1], WaitCause::noMovingApproach) == 0);
    test::near(total(r.causes), total(r.changes));
}
