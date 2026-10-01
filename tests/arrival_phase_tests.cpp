#include "test.hpp"
#include "../src/eval/arrival_phases.hpp"
#include <algorithm>
#include <cmath>
using namespace trafficsim;
// M3.2.8c step 4: the cycle phase at which vehicles enter a segment and first stand on it.
namespace {
constexpr double kmh = 1 / 3.6;
EvaluationSpec spec() {
    EvaluationSpec s;
    s.movementNames = {"in → out"};
    s.movementOfRoute = {{"route", 0}};
    s.queue = {5 * kmh, 10 * kmh, 20};
    return s;
}
// An `upLength` approach then a 150 m road whose head "h" stands 100 m in.
Scenario approach(double upLength, std::vector<SignalPhase> phases) {
    auto s = test::straight(); s.duration = 120;
    s.segments = {{"up", upLength, {"road"}}, {"road", 150, {}}}; s.routes = {{"route", {"up", "road"}}};
    s.signalPrograms = {{"p", 0, std::move(phases)}};
    s.signalHeads = {{"h", "road", 100, "p"}};
    return s;
}
ArrivalPhaseReport run(SimState state, std::vector<double>* entries = nullptr) {
    ArrivalPhaseAccumulator phases(spec()); phases.observe(state);
    while (state.tick < totalTicks(*state.scenario)) {
        state = stepSimulation(state); phases.observe(state);
        if (entries)
            for (const auto& e : state.events)
                if (const auto* in = std::get_if<SegmentEnteredEvent>(&e)) entries->push_back(in->time);
    }
    return phases.report();
}
ArrivalPhaseRow row(const ArrivalPhaseReport& r, const std::string& segment) {
    const auto it = std::find_if(r.rows.begin(), r.rows.end(), [&](const auto& x) { return x.segmentId == segment; });
    CHECK(it != r.rows.end());
    return *it;
}
}
TEST(movement, arrival_phase_a_queue_at_red_first_stops_in_the_red_bins) {
    auto s = approach(50, {{40, SignalColor::red}, {80, SignalColor::green}});
    auto state = test::withVehicles(s, {{1, "route", 45, 10}, {2, "route", 25, 10}, {3, "route", 5, 10}});
    auto probe = state;
    while (probe.time < 39) probe = stepSimulation(probe);
    // The forcing: all three have entered the road and stand at red.
    CHECK(probe.vehicles.size() == 3);
    for (const auto& v : probe.vehicles) CHECK(v.distance > 50 && v.speed < 5 * kmh);
    const auto r = run(state);
    CHECK(r.cycle == 120 && r.binWidth == 10 && r.unassigned == 0);
    const auto road = row(r, "road");
    CHECK(road.movement == "in → out" && road.entered == 3 && road.stopped == 3);
    CHECK(road.enteredAt.size() == 12 && road.enteredAt[0] == 3);
    std::uint64_t inRed = 0;
    for (std::size_t b = 0; b < 4; ++b) inRed += road.firstStopAt[b];
    CHECK(inRed == 3);
    // Placed by hand, they never departed, so their first segment is never entered; nor stood on.
    const auto up = std::find_if(r.rows.begin(), r.rows.end(), [](const auto& x) { return x.segmentId == "up"; });
    CHECK(up == r.rows.end());
}
TEST(movement, arrival_phase_wraps_the_cycle_and_does_not_count_a_start_from_rest) {
    auto s = approach(300, {{20, SignalColor::green}});
    std::vector<double> entries;
    const auto r = run(test::withVehicles(s, {{1, "route", 0, 0}}), &entries);
    // The forcing: the only entry happens after one whole 20 s cycle.
    CHECK(entries.size() == 1 && entries[0] > 20);
    CHECK(r.cycle == 20);
    const auto road = row(r, "road");
    CHECK(road.entered == 1 && road.enteredAt.size() == 2);
    CHECK(road.enteredAt[static_cast<std::size_t>(std::fmod(entries[0], 20) / 10)] == 1);
    CHECK(road.stopped == 0);
    // It started from rest on "up": not a stop before it has moved, and never an entry.
    CHECK(std::none_of(r.rows.begin(), r.rows.end(), [](const auto& x) { return x.segmentId == "up"; }));
}
TEST(movement, arrival_phase_needs_one_cycle_length) {
    auto s = approach(50, {{120, SignalColor::green}});
    s.signalPrograms.push_back({"q", 0, {{90, SignalColor::green}}});
    s.signalHeads.push_back({"k", "road", 120, "q"});
    const auto r = run(test::withVehicles(s, {{1, "route", 45, 10}}));
    CHECK(r.cycle == 0 && r.rows.empty());
}
TEST(movement, arrival_phase_counts_a_departure_as_entering_the_first_segment) {
    const auto s = approach(50, {{120, SignalColor::green}});
    const auto r = run(createSimulation(s, 42));
    const auto up = row(r, "up"), road = row(r, "road");
    CHECK(up.entered > 0 && up.entered == road.entered); // every departure that arrived, both times
    std::uint64_t early = 0;
    for (std::size_t b = 0; b < 7; ++b) early += up.enteredAt[b];
    CHECK(early == up.entered); // the input releases vehicles from 0 to 60 s only (60 s is bin 6)
}
