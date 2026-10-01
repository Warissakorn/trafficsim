#pragma once
#include "test.hpp"
// M3.2.8b/c tests: one 200 m Link of three lanes, "a" | "b" | "c". The movement leaves from lane b
// only, so a vehicle on a or c is on a stub that must change to b before its dead end at 200.
namespace test {
inline trafficsim::Scenario lanes() {
    const auto source = demo().scenario;
    trafficsim::Scenario s;
    s.duration = 120; s.timeStep = 0.1;
    s.segments = {{"a", 200, {}}, {"b", 200, {"turn"}}, {"c", 200, {}}, {"turn", 50, {}}};
    s.routes = {{"full", {"b", "turn"}}, {"stubA", {"a"}}, {"stubC", {"c"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.laneChanges = {{"stubA", "full", 0, 200, 0, 200}, {"stubC", "full", 0, 200, 0, 200}};
    s.routeDeadEnds = {{"stubA", 200}, {"stubC", 200}};
    return s;
}
inline Placement on(std::uint64_t id, const char* route, double distance, double speed) { return {id, route, distance, speed}; }
}
