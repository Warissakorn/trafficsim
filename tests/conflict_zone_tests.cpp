#include "test.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/routes.hpp"
#include <algorithm>

using namespace trafficsim;
// M3.2.3a: the crossing admission runtime, rows A09-A14, A16, A17 and A25 of
// docs/M3_ACCEPTANCE.md, on hand-built scenarios. The minor approach "north" crosses the major
// "east"; the area is 98..102 on both; the minor waiting line is at 90.
namespace {
Scenario crossing(double gapTime = 3, double headway = 10) {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 300; s.timeStep = 0.1;
    s.segments = {{"eastUp", 50, {"east"}}, {"east", 200, {}}, {"north", 200, {}}};
    s.routes = {{"majorRoute", {"eastUp", "east"}}, {"minorRoute", {"north"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.conflictZones = {{"zone", {{"east"}, 98, 102}, {{"north"}, 98, 102}, 90, gapTime, headway}};
    return s;
}
test::Placement on(std::uint64_t id, const char* route, double distance, double speed) { return {id, route, distance, speed}; }
const Vehicle* find(const SimState& s, std::uint64_t id) {
    for (const auto& v : s.vehicles) if (v.id == id) return &v;
    return nullptr; // left the network
}
double at(const SimState& s, std::uint64_t id) { const auto* v = find(s, id); return v ? v->distance : 1e9; }
ZoneState zoneOf(const SimState& s) {
    return summarizeZones(*s.scenario, *s.index, s.vehicles, resolveRefs(*s.scenario, s.vehicles, *s.index)).front();
}
// The invariant the whole slice exists for, checked over each tick's SWEPT interval: a side's
// vehicle touches the area between two snapshots when its front ends past entry while its rear
// started short of exit. Both sides may never do so in the same tick.
bool sweptOverlap(const SimState& before, const SimState& after) {
    const auto& sc = *before.scenario;
    bool touched[2] = {false, false};
    for (const auto& v : before.vehicles) {
        const double length = sc.vehicleTypes[v.typeIndex].length;
        const bool major = sc.routes[v.routeIndex].id == "majorRoute";
        const double offset = major ? 50 : 0; // the side's segment start on the route
        const double from = v.distance - offset, to = at(after, v.id) - offset;
        if (to > 98 + 1e-6 && from - length < 102 - 1e-6) touched[major ? 0 : 1] = true;
    }
    return touched[0] && touched[1];
}
std::vector<SimEvent> events(const Scenario& s, std::uint32_t seed) {
    std::vector<SimEvent> result;
    runSimulation(s, seed, [&](const auto& event) { result.push_back(event); });
    return result;
}
}
TEST(conflict_zone, a09_thresholds_are_exact) {
    // Major front-to-entry distance d = 98 - (distance - 50). Headway 10, gap time 3.
    const auto blocks = [](double d, double speed) {
        return zoneOf(test::withVehicles(crossing(), {on(1, "majorRoute", 50 + 98 - d, speed)})).majorBlocks;
    };
    CHECK(blocks(10, 0));              // at the headway: blocks even standing
    CHECK(!blocks(10.5, 0));           // beyond it and standing: a gap, not a block
    CHECK(blocks(29.5, 10));           // 2.95 s away: inside the gap time
    CHECK(!blocks(30, 10));            // exactly 3 s: equality passes the time threshold
    CHECK(!blocks(40, 10));
    CHECK(blocks(-7, 0));              // inside the area: rear at 100.5
    CHECK(!blocks(-9, 0));             // rear (4.5 m back) past the exit: gone
}
TEST(conflict_zone, a10_a_major_vehicle_before_the_section_cut_is_seen) {
    const auto place = [](Scenario sc) {
        return test::withVehicles(sc, {on(1, "majorRoute", 40, 40), on(2, "minorRoute", 89.9, 5)});
    };
    auto s = place(crossing());
    CHECK(locateVehicle(*s.scenario, s.vehicles.front(), *s.index).segmentId == "eastUp"); // the forcing
    auto open = crossing(); open.conflictZones.clear();
    CHECK(at(stepSimulation(place(open)), 2) > 90); // and unheld, the minor crosses this tick
    CHECK(zoneOf(s).majorBlocks); // 108 m at 40 m/s: 2.7 s, seen from the segment before
    CHECK(at(stepSimulation(s), 2) <= 90 + 1e-9);
}
TEST(conflict_zone, a11_same_tick_requests_replay_identically_in_any_input_order) {
    auto s = crossing();
    s.inputs = {{"majorIn", "majorRoute", "car", 700, 0, 200}, {"minorIn", "minorRoute", "car", 700, 0, 200}};
    const auto reference = events(s, 7);
    auto shuffled = s;
    std::reverse(shuffled.segments.begin(), shuffled.segments.end());
    std::reverse(shuffled.routes.begin(), shuffled.routes.end());
    std::reverse(shuffled.inputs.begin(), shuffled.inputs.end());
    shuffled.conflictZones.insert(shuffled.conflictZones.begin(), {"another", {{"eastUp"}, 1, 2}, {{"north"}, 1, 2}, 0, 3, 10});
    auto one = s; one.conflictZones.push_back({"another", {{"eastUp"}, 1, 2}, {{"north"}, 1, 2}, 0, 3, 10});
    CHECK(events(shuffled, 7) == events(one, 7));
    CHECK(events(s, 7) == reference);
    CHECK(runSimulation(s, 7).completed > 0); // the forcing: traffic really met at the area
}
TEST(conflict_zone, a12_a_sink_too_close_for_the_longest_vehicle_is_refused) {
    CHECK(validateScenario(crossing()).empty()); // the forcing: the base case is valid
    auto close = crossing();
    close.segments[2].length = 105; // 3 m past the exit: shorter than any vehicle
    const auto issues = validateScenario(close);
    CHECK(std::any_of(issues.begin(), issues.end(), [](const auto& i) { return i.code == "CONFLICT_SINK_TOO_CLOSE"; }));
    auto late = crossing();
    late.routes[1].segmentIds = {"north"};
    late.conflictZones[0].waitPosition = 99; // after the entry
    CHECK(!validateScenario(late).empty());
}
TEST(conflict_zone, a13_a_fast_vehicle_never_overlaps_across_a_tick) {
    // Half-second ticks: at 15 m/s a front moves 7.5 m a tick, past the whole 4 m area, and the
    // two vehicles are timed to reach it together.
    const auto run = [](Scenario sc, bool& overlapped) {
        sc.timeStep = 0.5;
        auto s = test::withVehicles(sc, {on(1, "minorRoute", 70, 15), on(2, "majorRoute", 120, 15)});
        overlapped = false;
        for (int i = 0; i < 60; ++i) { const auto next = stepSimulation(s); overlapped |= sweptOverlap(s, next); s = next; }
        return s;
    };
    bool overlapped = false;
    auto open = crossing(1, 2); open.conflictZones.clear();
    run(open, overlapped);
    CHECK(overlapped); // the forcing: unprotected, they meet inside the area within one tick
    const auto end = run(crossing(1, 2), overlapped);
    CHECK(!overlapped);
    CHECK(!find(end, 1)); CHECK(!find(end, 2)); // and both got through
}
TEST(conflict_zone, a14_a_standing_queue_past_the_exit_holds_the_minor_at_its_line) {
    auto s = crossing();
    s.signalPrograms = {{"p", 0, {{20, SignalColor::red}, {280, SignalColor::green}}}};
    s.signalHeads = {{"h", "north", 112, "p"}};
    auto state = test::withVehicles(s, {on(1, "minorRoute", 111, 0), on(2, "minorRoute", 80, 8)});
    // The forcing: without the queued leader the same vehicle is past the line in 3 s.
    auto alone = test::withVehicles(s, {on(2, "minorRoute", 80, 8)});
    for (int i = 0; i < 30; ++i) alone = stepSimulation(alone);
    CHECK(at(alone, 2) > 90);
    for (int i = 0; i < 400; ++i) {
        state = stepSimulation(state);
        if (state.time < 19) CHECK(at(state, 2) <= 90 + 1e-9);
    }
    CHECK(at(state, 2) > 102); // released once the queue moved off
}
TEST(conflict_zone, a16_an_admitted_minor_keeps_its_grant_and_the_major_waits) {
    // Minor already past its line; a major arrives and must wait at the entry until it clears.
    auto s = test::withVehicles(crossing(), {on(1, "minorRoute", 91, 0), on(2, "majorRoute", 120, 12)});
    CHECK(zoneOf(s).holders.size() == 1); // the forcing: the minor holds the crossing
    auto free = test::withVehicles([] { auto x = crossing(); x.conflictZones.clear(); return x; }(),
                                   {on(2, "majorRoute", 120, 12)});
    bool waited = false;
    for (int i = 0; i < 200; ++i) {
        s = stepSimulation(s); free = stepSimulation(free);
        const bool minorInside = find(s, 1) && at(s, 1) - 4.5 < 102;
        if (minorInside) CHECK(at(s, 2) <= 50 + 98 + 1e-9);
        if (minorInside && at(free, 2) > 50 + 102) waited = true;
    }
    CHECK(waited); // without the zone the major vehicle would have been through already
    CHECK(!find(s, 1)); CHECK(at(s, 2) > 150); // both finish: nobody deleted, nobody stuck
}
TEST(conflict_zone, a17_same_side_vehicles_follow_through_together) {
    auto s = test::withVehicles(crossing(), {on(1, "minorRoute", 95, 8), on(2, "minorRoute", 88, 8)});
    bool together = false;
    for (int i = 0; i < 40; ++i) {
        s = stepSimulation(s);
        if (find(s, 1) && at(s, 1) > 98 && at(s, 1) - 4.5 < 102 && at(s, 2) > 90) together = true;
    }
    CHECK(together); // the follower crossed its line while the leader was still in the area
}
TEST(conflict_zone, a25_a_copied_state_replays_exactly) {
    auto s = crossing();
    s.inputs = {{"majorIn", "majorRoute", "car", 800, 0, 200}, {"minorIn", "minorRoute", "car", 500, 0, 200}};
    auto state = createSimulation(s, 11);
    for (int i = 0; i < 600; ++i) state = stepSimulation(state);
    auto copy = state;
    for (int i = 0; i < 600; ++i) { state = stepSimulation(state); copy = stepSimulation(copy); }
    CHECK(state.vehicles == copy.vehicles); CHECK(state.events == copy.events);
    CHECK(state.completed == copy.completed);
}
TEST(conflict_zone, congested_demand_never_overlaps_and_the_minor_road_waits) {
    auto s = crossing();
    s.inputs = {{"majorIn", "majorRoute", "car", 900, 0, 240}, {"minorIn", "minorRoute", "car", 400, 0, 240}};
    auto state = createSimulation(s, 3);
    std::uint64_t clamps = 0;
    while (state.tick < totalTicks(*state.scenario)) {
        const auto next = stepSimulation(state);
        CHECK(!sweptOverlap(state, next));
        for (const auto& e : next.events) if (std::holds_alternative<SafetyClampEvent>(e)) ++clamps;
        state = next;
    }
    CHECK(state.completed > 0);
    CHECK(clamps > 0); // the forcing: vehicles really were held at the line
    // A zone costs the minor road time: the same demand with no zone is faster through.
    auto open = s; open.conflictZones.clear();
    const auto delay = [](const Scenario& x) {
        double total = 0; int n = 0;
        runSimulation(x, 3, [&](const SimEvent& e) {
            if (const auto* a = std::get_if<ArrivedEvent>(&e); a && a->routeId == "minorRoute") { total += a->travelTime; ++n; }
        });
        return total / n;
    };
    CHECK(delay(s) > delay(open));
}
TEST(conflict_zone, a13_a_same_tick_request_is_capped_by_the_swept_check) {
    // A gap time shorter than one tick and a waiting line 1 m before the area: from the snapshot
    // nothing blocks, yet within this half-second tick both fronts reach the area.
    auto sc = crossing(0.2, 0.5); sc.timeStep = 0.5; sc.conflictZones[0].waitPosition = 97;
    const auto s = test::withVehicles(sc, {on(1, "minorRoute", 96, 10), on(2, "majorRoute", 50 + 93, 15)});
    CHECK(!zoneOf(s).majorBlocks); // the forcing: the snapshot admits it (5 m at 15 m/s = 0.33 s)
    const auto next = stepSimulation(s);
    CHECK(at(next, 2) - 50 > 98); // the major vehicle really is in the area by the end of the tick
    CHECK(!sweptOverlap(s, next));
    CHECK(at(next, 1) <= 97 + 1e-9); // the request was capped at its line, before publishing
}
