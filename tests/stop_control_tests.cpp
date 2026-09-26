#include "test.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/routes.hpp"
#include <algorithm>

using namespace trafficsim;
// M3.2.5a: Stop and Yield at a conflict zone's waiting line, rows A18-A20 of
// docs/M3_ACCEPTANCE.md, on hand-built scenarios. The minor approach "north" crosses the major
// "east"; the area is 98..102 on both; the minor waiting line is at 90.
namespace {
Scenario crossing(ZoneControl control) {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 300; s.timeStep = 0.1;
    s.segments = {{"eastUp", 50, {"east"}}, {"east", 200, {}}, {"north", 200, {}}};
    s.routes = {{"majorRoute", {"eastUp", "east"}}, {"minorRoute", {"north"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.conflictZones = {{"zone", {{"east"}, 98, 102}, {{"north"}, 98, 102}, 90, 3, 10, control}};
    return s;
}
test::Placement on(std::uint64_t id, const char* route, double distance, double speed) { return {id, route, distance, speed}; }
const Vehicle* find(const SimState& s, std::uint64_t id) {
    for (const auto& v : s.vehicles) if (v.id == id) return &v;
    return nullptr;
}
// Standing at the line: speed zero, front within the reach a Stop accepts.
bool atLine(const SimState& s, const Vehicle& v) {
    return v.speed == 0 && v.distance <= 90 + 1e-9 && 90 - v.distance <= stopLineReach(s.scenario->behaviours.front(), v.driverFactor);
}
// Each minor vehicle's story up to the tick it crosses the line: how many ticks it began standing
// at the line, and whether it ever stood still anywhere before crossing.
struct Story { int ticksAtLine{}; bool crossed{}; double slowest{1e9}; };
std::map<std::uint64_t, Story> run(SimState s, int ticks) {
    std::map<std::uint64_t, Story> stories;
    for (int t = 0; t < ticks; ++t) {
        for (const auto& v : s.vehicles) {
            if (s.scenario->routes[v.routeIndex].id != "minorRoute") continue;
            auto& story = stories[v.id];
            if (story.crossed) continue;
            if (v.distance > 90 + 1e-9) { story.crossed = true; continue; }
            story.slowest = std::min(story.slowest, v.speed);
            if (atLine(s, v)) ++story.ticksAtLine;
        }
        s = stepSimulation(s);
    }
    return stories;
}
}
TEST(stop_control, a18_stop_serves_the_line_and_yield_does_not_dwell) {
    // The same vehicle, the same empty major road.
    const auto yield = run(test::withVehicles(crossing(ZoneControl::yield), {on(1, "minorRoute", 40, 10)}), 300).at(1);
    const auto stop = run(test::withVehicles(crossing(ZoneControl::stop), {on(1, "minorRoute", 40, 10)}), 300).at(1);
    CHECK(yield.crossed && stop.crossed);
    CHECK(yield.slowest > 0 && yield.ticksAtLine == 0); // no mandatory zero-speed dwell
    // Stop: standing at the line at the start of two ticks -- the tick it arrived and one whole
    // tick more -- before it may cross.
    CHECK(stop.slowest == 0); CHECK(stop.ticksAtLine >= 2);
}
TEST(stop_control, a19_every_queued_vehicle_serves_the_line_once_and_service_survives_waiting) {
    // Three standing in a queue behind the line: each is served at the line, not at the queue tail.
    auto s = test::withVehicles(crossing(ZoneControl::stop), {on(1, "minorRoute", 87, 0), on(2, "minorRoute", 79, 0), on(3, "minorRoute", 71, 0)});
    // The forcing: the queued ones stand still but are not at the line.
    CHECK(atLine(s, s.vehicles[0])); CHECK(!atLine(s, s.vehicles[1])); CHECK(!atLine(s, s.vehicles[2]));
    const auto first = stepSimulation(s);
    CHECK(first.stopService.size() == 1 && first.stopService.front().vehicleId == 1); // the tail holds no service
    const auto stories = run(s, 600);
    for (std::uint64_t id = 1; id <= 3; ++id) { CHECK(stories.at(id).crossed); CHECK(stories.at(id).ticksAtLine >= 2); }
    // Passing clears it: nothing is left once everyone is through.
    auto after = s; for (int t = 0; t < 600; ++t) after = stepSimulation(after);
    CHECK(after.stopService.empty());
    // Served, then held by a major vehicle: the service stands through the wait; nothing re-arms.
    auto wait = test::withVehicles(crossing(ZoneControl::stop), {on(1, "minorRoute", 87, 0)});
    for (int t = 0; t < 3; ++t) wait = stepSimulation(wait);
    CHECK(wait.stopService.size() == 1);
    const auto since = wait.stopService.front().since;
    CHECK(since < wait.tick); // served
    auto blocked = test::withVehicles(crossing(ZoneControl::stop), {on(1, "minorRoute", find(wait, 1)->distance, 0), on(2, "majorRoute", 120, 12)});
    blocked.stopService = wait.stopService; blocked.tick = wait.tick; blocked.time = wait.time;
    CHECK(zoneHold(*blocked.scenario, *blocked.index,
                   summarizeZones(*blocked.scenario, *blocked.index, blocked.vehicles, resolveRefs(*blocked.scenario, blocked.vehicles, *blocked.index)),
                   blocked.vehicles.front(), resolveRefs(*blocked.scenario, blocked.vehicles, *blocked.index).front(), std::nullopt,
                   &blocked.stopService.front(), blocked.tick).has_value()); // the gap holds it, not the Stop
    for (int t = 0; t < 30; ++t) blocked = stepSimulation(blocked);
    CHECK(!blocked.stopService.empty() && blocked.stopService.front().since == since);
}
TEST(stop_control, a19_a_copied_state_replays_and_a_new_run_starts_unserved) {
    auto s = crossing(ZoneControl::stop);
    s.inputs = {{"minorIn", "minorRoute", "car", 900, 0, 200}, {"majorIn", "majorRoute", "car", 400, 0, 200}};
    auto state = createSimulation(s, 11);
    for (int t = 0; t < 2000 && state.stopService.empty(); ++t) state = stepSimulation(state);
    CHECK(!state.stopService.empty()); // the forcing: service is live at the copy point
    auto copy = state;
    for (int t = 0; t < 200; ++t) { state = stepSimulation(state); copy = stepSimulation(copy); }
    CHECK(state.vehicles == copy.vehicles); CHECK(state.stopService == copy.stopService);
    CHECK(createSimulation(s, 11).stopService.empty()); // reset clears service
}
TEST(stop_control, a20_a_green_head_at_the_line_neither_serves_the_stop_nor_erases_occupancy) {
    auto s = crossing(ZoneControl::stop);
    s.signalPrograms = {{"green", 0, {{300, SignalColor::green}}}, {"red", 0, {{300, SignalColor::red}}}};
    s.signalHeads = {{"h", "north", 90, "green"}};
    const auto green = run(test::withVehicles(s, {on(1, "minorRoute", 40, 10)}), 300).at(1);
    CHECK(green.crossed && green.ticksAtLine >= 2); // green is that head's permission only
    // Red holds a vehicle that has served the Stop.
    s.signalHeads = {{"h", "north", 90, "red"}};
    auto red = test::withVehicles(s, {on(1, "minorRoute", 40, 10)});
    CHECK(run(red, 600).at(1).ticksAtLine >= 2);
    for (int t = 0; t < 600; ++t) red = stepSimulation(red);
    CHECK(find(red, 1) && find(red, 1)->distance <= 90 + 1e-9);                            // still held
    CHECK(red.stopService.size() == 1 && red.stopService.front().since + 1 < red.tick); // though served
    // Green with the area occupied: a Yield vehicle still waits for the major vehicle to clear.
    auto y = crossing(ZoneControl::yield);
    y.signalPrograms = {{"green", 0, {{300, SignalColor::green}}}};
    y.signalHeads = {{"h", "north", 90, "green"}};
    auto occupied = test::withVehicles(y, {on(1, "minorRoute", 85, 5), on(2, "majorRoute", 50 + 100, 0)});
    // The forcing: without the standing major vehicle the minor one crosses this second.
    auto clear = test::withVehicles(y, {on(1, "minorRoute", 85, 5)});
    for (int t = 0; t < 10; ++t) { occupied = stepSimulation(occupied); clear = stepSimulation(clear); }
    CHECK(find(clear, 1)->distance > 90);
    CHECK(find(occupied, 1)->distance <= 90 + 1e-9);
}
