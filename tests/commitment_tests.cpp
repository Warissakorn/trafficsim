#include "test.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/routes.hpp"
#include <algorithm>
using namespace trafficsim;
// M3.2.8a (docs/M3_8_CONTRACT.md §1, D69): a driver who cannot stop at its line at its type's
// maximum deceleration goes instead of being clamped there -- unless the area is physically taken.
// Hand-built scenarios with test-owned states: a crossing (area 98..102 on both roads, the minor
// line at 90; the major route starts 50 m earlier) and a derived-rule merge.
namespace {
Scenario crossing() {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 60; s.timeStep = 0.1;
    s.segments = {{"eastUp", 50, {"east"}}, {"east", 200, {}}, {"north", 200, {}}};
    s.routes = {{"majorRoute", {"eastUp", "east"}}, {"minorRoute", {"north"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.conflictZones = {{"zone", {{"east"}, 98, 102}, {{"north"}, 98, 102}, 90, 3, 10}};
    return s;
}
Scenario merge() { // core_tests' shape: the minor waits at 100, the conflict point is the join at 150
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 60; s.timeStep = 0.1;
    s.segments = {{"major", 150, {"merged"}}, {"minor", 100, {"merged"}}, {"merged", 150, {}}};
    s.routes = {{"majorRoute", {"major", "merged"}}, {"minorRoute", {"minor", "merged"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.priorityRules = {{"give-way", "minor", 100, "major", 150, 3, 10}};
    return s;
}
test::Placement on(std::uint64_t id, const char* route, double distance, double speed) { return {id, route, distance, speed}; }
const Vehicle& vehicle(const SimState& s, std::uint64_t id) {
    return *std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
}
const VehicleType& car(const SimState& s) { return s.scenario->vehicleTypes[s.vehicles.front().typeIndex]; }
ZoneState zoneOf(const SimState& s) {
    return summarizeZones(*s.scenario, *s.index, s.vehicles, resolveRefs(*s.scenario, s.vehicles, *s.index)).front();
}
// What a run from `s` did: the minor front's furthest distance while the major is still short of
// the area, clamps per vehicle, and whether both fronts were ever over the area at once.
struct Outcome { double minorBeforeMajor{}; int majorClamps{}, minorClamps{}; bool bothInside{}; bool minorCrossed{}; };
Outcome runFrom(SimState s, double seconds) {
    Outcome o;
    const double length = car(s).length;
    for (int i = 0; i < seconds / s.scenario->timeStep; ++i) {
        s = stepSimulation(s);
        for (const auto& e : s.events)
            if (const auto* c = std::get_if<SafetyClampEvent>(&e)) ++(c->vehicleId == 1 ? o.majorClamps : o.minorClamps);
        bool major = false, minor = false;
        for (const auto& v : s.vehicles) {
            const double station = v.id == 1 ? v.distance - 50 : v.distance;
            (v.id == 1 ? major : minor) = (v.id == 1 ? major : minor) || (station > 98 && station - length < 102);
            if (v.id == 2 && v.distance > 90) o.minorCrossed = true;
            if (v.id == 2 && std::any_of(s.vehicles.begin(), s.vehicles.end(), [](const auto& m) { return m.id == 1 && m.distance - 50 < 98; }))
                o.minorBeforeMajor = std::max(o.minorBeforeMajor, v.distance);
        }
        o.bothInside = o.bothInside || (major && minor);
    }
    return o;
}
}
TEST(commitment, a_driver_who_cannot_stop_goes_through_a_closing_gap) {
    // Major 25 m short of entry at 10 m/s: 2.5 s, inside gapTime 3, outside headway 10. Minor 5 m
    // short of its line at 10 m/s needs 6.25 m to stop at 8 m/s².
    const auto s = test::withVehicles(crossing(), {on(1, "majorRoute", 50 + 98 - 25, 10), on(2, "minorRoute", 85, 10)});
    const auto zone = zoneOf(s);
    CHECK(zone.majorBlocks && !zone.majorInside);          // the forcing: the gap is closed by anticipation alone
    CHECK(committed(10, 5, car(s)));                       // and the minor cannot stop before its line
    const auto o = runFrom(s, 8);
    CHECK(o.minorCrossed);
    CHECK(o.minorClamps == 0 && o.majorClamps == 0);       // nobody is halted by the emergency clamp
    CHECK(!o.bothInside);                                  // the major waits at entry for the admitted minor
}
TEST(commitment, a_driver_who_can_stop_still_waits) {
    // The same major; the minor is 30 m out and stops within 6.25 m.
    const auto s = test::withVehicles(crossing(), {on(1, "majorRoute", 50 + 98 - 25, 10), on(2, "minorRoute", 60, 10)});
    CHECK(zoneOf(s).majorBlocks);                          // the forcing
    CHECK(!committed(10, 30, car(s)));
    const auto o = runFrom(s, 8);
    CHECK(o.minorBeforeMajor > 0 && o.minorBeforeMajor <= 90 + 1e-9); // held at its line while the major passes
    CHECK(o.minorClamps == 0 && o.majorClamps == 0);
    CHECK(!o.bothInside);
}
TEST(commitment, occupancy_still_holds_a_committed_driver) {
    // A major vehicle standing inside the area; the minor is 0.5 m short of its line at 10 m/s.
    const auto s = test::withVehicles(crossing(), {on(1, "majorRoute", 50 + 100, 0), on(2, "minorRoute", 89.5, 10)});
    CHECK(zoneOf(s).majorInside);                          // the forcing: the area is physically taken
    CHECK(committed(10, 0.5, car(s)));
    // The zone itself holds it -- not only the swept check, which would also cap it.
    const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
    const auto zones = summarizeZones(*s.scenario, *s.index, s.vehicles, refs);
    CHECK(zoneHold(*s.scenario, *s.index, zones, s.vehicles.back(), refs.back(), std::nullopt).has_value());
    const auto next = stepSimulation(s);
    CHECK(vehicle(next, 2).distance <= 90 + 1e-9);         // it does not enter a taken area...
    CHECK(std::any_of(next.events.begin(), next.events.end(), [](const auto& e) {
        const auto* c = std::get_if<SafetyClampEvent>(&e); return c && c->vehicleId == 2; })); // ...and the clamp is counted
}
TEST(commitment, equality_can_stop) {
    // 4 m/s, 1 m short: 16 = 2 * 8 * 1 exactly, the boundary.
    const auto major = on(1, "majorRoute", 50 + 98 - 25, 10);
    const auto held = [&](double distance) {
        const auto s = test::withVehicles(crossing(), {major, on(2, "minorRoute", distance, 4)});
        const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
        const auto zones = summarizeZones(*s.scenario, *s.index, s.vehicles, refs);
        CHECK(zones.front().majorBlocks);                  // the forcing, each time
        return zoneHold(*s.scenario, *s.index, zones, s.vehicles.back(), refs.back(), std::nullopt).has_value();
    };
    CHECK(car(test::withVehicles(crossing(), {major})).maxDeceleration == 8); // the boundary below assumes it
    CHECK(held(89));                                       // exactly able to stop: held
    CHECK(!held(89.25));                                   // 0.75 m short: committed
}
TEST(commitment, a_derived_merge_rule_lets_a_committed_driver_go_but_not_onto_a_taken_join) {
    // Major 25 m short of the join at 10 m/s: 2.5 s, inside gapTime 3.
    const auto approaching = on(1, "majorRoute", 125, 10);
    const auto go = test::withVehicles(merge(), {approaching, on(2, "minorRoute", 95, 10)});
    CHECK(committed(10, 5, car(go)));                      // the forcing: cannot stop before 100
    auto s = go;
    int clamps = 0;
    for (int i = 0; i < 20; ++i) {
        s = stepSimulation(s);
        for (const auto& e : s.events) clamps += std::holds_alternative<SafetyClampEvent>(e);
    }
    CHECK(vehicle(s, 2).distance > 100 && clamps == 0);
    // The same major, the minor 30 m out: it can stop, so the rule holds it.
    auto wait = test::withVehicles(merge(), {approaching, on(2, "minorRoute", 70, 10)});
    CHECK(!committed(10, 30, car(wait)));
    for (int i = 0; i < 20; ++i) wait = stepSimulation(wait);
    CHECK(vehicle(wait, 2).distance <= 100 + 1e-9);
    // A major standing across the join still holds the committed driver: it is its leader.
    const auto taken = test::withVehicles(merge(), {on(1, "majorRoute", 151, 0), on(2, "minorRoute", 99.5, 10)});
    CHECK(committed(10, 0.5, car(taken)));
    CHECK(vehicle(stepSimulation(taken), 2).distance <= 100 + 1e-9);
}
TEST(commitment, is_read_off_the_snapshot_so_a_copied_state_replays_exactly) {
    auto s = crossing();
    s.inputs = {{"majorIn", "majorRoute", "car", 800, 0, 50}, {"minorIn", "minorRoute", "car", 500, 0, 50}};
    auto state = createSimulation(s, 11);
    for (int i = 0; i < 200; ++i) state = stepSimulation(state);
    auto copy = state;
    for (int i = 0; i < 300; ++i) { state = stepSimulation(state); copy = stepSimulation(copy); }
    CHECK(state.vehicles == copy.vehicles); CHECK(state.events == copy.events);
}
