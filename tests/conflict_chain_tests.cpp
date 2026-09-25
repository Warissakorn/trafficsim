#include "test.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/routes.hpp"
#include <algorithm>

using namespace trafficsim;
// M3.2.3b: sides over section cuts, chains of zones admitted atomically (A15), and the route
// topologies the solver refuses by name. Hand-built scenarios; the minor road runs n1 -> n2.
namespace {
test::Placement on(std::uint64_t id, const char* route, double distance, double speed) { return {id, route, distance, speed}; }
double at(const SimState& s, std::uint64_t id) {
    for (const auto& v : s.vehicles) if (v.id == id) return v.distance;
    return 1e9;
}
bool refuses(const Scenario& s, const std::string& code) {
    const auto issues = validateScenario(s);
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code; });
}
Scenario base() {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 120; s.timeStep = 0.1;
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    return s;
}
// Two major roads crossing one minor road: "a" at minor 50..54 and "b" at `bAt`..`bAt`+4, with
// B's own waiting line 3 m before it. At bAt = 60 there is no room to wait between them.
Scenario twoCrossings(double bAt) {
    auto s = base();
    s.segments = {{"n1", 200, {}}, {"a", 200, {}}, {"b", 200, {}}};
    s.routes = {{"minorRoute", {"n1"}}, {"aRoute", {"a"}}, {"bRoute", {"b"}}};
    s.conflictZones = {{"za", {{"a"}, 98, 102}, {{"n1"}, 50, 54}, 45, 3, 10},
                       {"zb", {{"b"}, 98, 102}, {{"n1"}, bAt, bAt + 4}, bAt - 3, 3, 10}};
    return s;
}
}
TEST(conflict_chain, a15_zones_with_no_room_between_them_admit_together) {
    // Only B's major road is busy: a vehicle 8 m from B's entry at 10 m/s.
    const auto run = [](double bAt) {
        auto s = test::withVehicles(twoCrossings(bAt), {on(1, "minorRoute", 40, 8), on(2, "bRoute", 90, 10)});
        double furthest = 0;
        for (int i = 0; i < 12; ++i) { s = stepSimulation(s); furthest = std::max(furthest, at(s, 1)); }
        return furthest;
    };
    // The forcing: with room to wait between them, the vehicle passes A's line and stops at B's.
    const double apart = run(90);
    CHECK(apart > 45); CHECK(apart <= 87 + 1e-9);
    // Chained: it waits at A's line, because stopping at B's would leave its tail inside A.
    CHECK(run(60) <= 45 + 1e-9);
    // Chained zones share the first line and the last exit in the route's incidence.
    const auto s = createSimulation(twoCrossings(60), 1);
    std::size_t minor = 0;
    while (s.scenario->routes[minor].id != "minorRoute") ++minor; // routes are sorted by id
    const auto& zones = s.index->routeZones[minor];
    CHECK(zones.size() == 2);
    for (const auto& rz : zones) { test::near(rz.waitAt, 45, 1e-12); test::near(rz.clearAt, 64, 1e-12); }
}
TEST(conflict_chain, a_chain_never_overlaps_under_demand) {
    auto s = twoCrossings(60);
    s.duration = 300;
    s.inputs = {{"m", "minorRoute", "car", 400, 0, 240}, {"a", "aRoute", "car", 600, 0, 240}, {"b", "bRoute", "car", 600, 0, 240}};
    auto state = createSimulation(s, 5);
    const auto inside = [&](const SimState& st, const char* route, double entry, double exit) {
        for (const auto& v : st.vehicles)
            if (st.scenario->routes[v.routeIndex].id == route && v.distance > entry && v.distance - 4.5 < exit) return true;
        return false;
    };
    bool minorCrossed = false;
    while (state.tick < totalTicks(*state.scenario)) {
        state = stepSimulation(state);
        CHECK(!(inside(state, "minorRoute", 50, 54) && inside(state, "aRoute", 98, 102)));
        CHECK(!(inside(state, "minorRoute", 60, 64) && inside(state, "bRoute", 98, 102)));
        minorCrossed |= inside(state, "minorRoute", 60, 64);
    }
    CHECK(minorCrossed); // the forcing: the minor road really used both areas
}
TEST(conflict_chain, a_side_over_a_section_cut_is_one_area) {
    auto s = base();
    s.segments = {{"e1", 100, {"e2", "off"}}, {"e2", 100, {}}, {"off", 100, {}}, {"n1", 200, {}}};
    s.routes = {{"through", {"e1", "e2"}}, {"turn", {"e1", "off"}}, {"minorRoute", {"n1"}}};
    s.conflictZones = {{"z", {{"e1", "e2"}, 97, 3}, {{"n1"}, 98, 102}, 90, 3, 10}};
    CHECK(validateScenario(s).empty());
    const auto blocks = [&](const char* route) {
        const auto st = test::withVehicles(s, {on(1, route, 106, 0)});
        return summarizeZones(*st.scenario, *st.index, st.vehicles, resolveRefs(*st.scenario, st.vehicles, *st.index)).front().majorBlocks;
    };
    // Front at 106: on e2 its rear (101.5) is still inside the area, which ends at e2 station 3.
    CHECK(blocks("through"));
    // The same position after turning off at the cut: it left the area at e1's end.
    CHECK(!blocks("turn"));
}
TEST(conflict_chain, routes_the_solver_cannot_serve_are_refused_by_name) {
    auto s = base();
    s.segments = {{"n0", 100, {"n1"}}, {"n1", 100, {"n2"}}, {"n2", 200, {}}, {"a", 200, {}}};
    s.routes = {{"minorRoute", {"n0", "n1", "n2"}}, {"aRoute", {"a"}}};
    s.conflictZones = {{"z", {{"a"}, 98, 102}, {{"n1", "n2"}, 97, 3}, 90, 3, 10}};
    CHECK(validateScenario(s).empty()); // the forcing: the base case is valid
    // A route that meets the minor chain part way would be inside without passing the line.
    auto joins = s;
    joins.segments.push_back({"side", 50, {"n2"}});
    joins.routes.push_back({"sideRoute", {"side", "n2"}});
    CHECK(refuses(joins, "CONFLICT_ROUTE_JOINS_INSIDE"));
    // A route minor at one zone and major at another could deadlock with its mirror image.
    auto mixed = s;
    mixed.segments.push_back({"c", 200, {}});
    mixed.conflictZones.push_back({"y", {{"n2"}, 150, 154}, {{"c"}, 98, 102}, 90, 3, 10});
    mixed.routes.push_back({"cRoute", {"c"}});
    CHECK(refuses(mixed, "CONFLICT_MIXED_ROLES"));
    // A chain whose segments do not follow one another is not a side.
    auto broken = s; broken.conflictZones[0].minor.segmentIds = {"n2", "n1"};
    CHECK(refuses(broken, "INVALID_RANGE"));
}
