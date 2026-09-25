#include "test.hpp"
#include "right_of_way_fixture.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/project/run.hpp"
#include "../src/core/conflicts.hpp"
using namespace trafficsim;
using namespace rowfixture;
// M3.2.3a: an authored crossing compiles to a core ConflictZone and runs; what the first slice
// cannot run stays refused by name (docs/M3_ACCEPTANCE.md, M3.2.3 rows at the model seam).
namespace {
struct Crossing { ProjectDocument d; std::string major, minor, area, minorRoute; };
std::string lane(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes[0].id;
    throw std::runtime_error("no link");
}
// Major Link along x, minor Link along y, crossing at (100, 0); the minor side gives way and
// waits at station 90. Both carry a route and a vehicle input.
Crossing crossing(double gapTime) {
    Crossing c;
    c.major = addLink(c.d, {{0, 0}, {200, 0}}, 1, 3.5);
    c.minor = addLink(c.d, {{100, -100}, {100, 100}}, 1, 3.5);
    const ControlPathRef pMajor{c.major, lane(c.d, c.major), "", "", ""}, pMinor{c.minor, lane(c.d, c.minor), "", "", ""};
    const auto o = surfaceOverlap(c.d.network, pMinor, pMajor);
    const auto wMinor = putWaitingLine(c.d, {"", "", {pMinor, 90}});
    const auto wMajor = putWaitingLine(c.d, {"", "", {pMajor, 90}});
    c.area = putConflictArea(c.d, {"", "", ConflictKind::crossing, {pMinor, o.first.from, o.first.to, wMinor},
                                   {pMajor, o.second.from, o.second.to, wMajor}, ConflictPriority::firstYields});
    putPriorityRule(c.d, {"", "", c.area, gapTime, 5});
    const auto majorRoute = putRoute(c.d, {"", {c.major}});
    c.minorRoute = putRoute(c.d, {"", {c.minor}});
    putInput(c.d, {"", majorRoute, "car", 900, 0, 540, {}});
    putInput(c.d, {"", c.minorRoute, "car", 300, 0, 540, {}});
    changeRunSettings(c.d, 600, 0.1);
    validateDocument(c.d);
    return c;
}
double minorTravelTime(const Scenario& s) {
    double total = 0; int n = 0;
    runSimulation(s, 42, [&](const SimEvent& e) {
        const auto* a = std::get_if<ArrivedEvent>(&e);
        if (!a) return;
        // Only the minor road: its routes start on the minor side's segment.
        for (const auto& r : s.routes)
            if (r.id == a->routeId && r.segmentIds.front() == s.conflictZones.front().minor.segmentIds.front()) { total += a->travelTime; ++n; }
    });
    return n ? total / n : 0;
}
}
TEST(rightofway_runtime, an_isolated_crossing_compiles_to_one_zone_and_runs) {
    const auto c = crossing(3);
    const auto r = resolve(c.d);
    CHECK(r.issues.empty()); // M3.2.2c reported UNSUPPORTED_CONFLICT_RUNTIME here
    const auto s = compileDocument(c.d, test::root() / "data").scenario;
    CHECK(s.conflictZones.size() == 1);
    const auto& zone = s.conflictZones.front();
    CHECK(zone.minor.segmentIds == std::vector{lane(c.d, c.minor)}); CHECK(zone.major.segmentIds == std::vector{lane(c.d, c.major)});
    // A straight one-lane Link: lane stations are reference stations, the overlap is 98.25..101.75.
    test::near(zone.waitPosition, 90, 1e-9);
    test::near(zone.minor.entry, 98.25, 1e-9); test::near(zone.minor.exit, 101.75, 1e-9);
    test::near(zone.major.entry, 98.25, 1e-9); test::near(zone.major.exit, 101.75, 1e-9);
    test::near(zone.gapTime, 3, 0); test::near(zone.headway, 5, 0);
    const auto end = runSimulation(s, 42);
    CHECK(end.completed > 100);
}
TEST(rightofway_runtime, minor_road_delay_responds_to_gap_time) {
    // The M3 done-condition's quantity, compared between two runs of one network; no magnitude
    // is claimed, only the direction a longer required gap must push it.
    const auto shortGap = minorTravelTime(compileDocument(crossing(1).d, test::root() / "data").scenario);
    const auto longGap = minorTravelTime(compileDocument(crossing(6).d, test::root() / "data").scenario);
    CHECK(shortGap > 0); // the forcing: minor vehicles really completed
    CHECK(longGap > shortGap);
}
TEST(rightofway_runtime, an_area_over_a_section_cut_runs_as_one_chain) {
    // A Connector leaves the major Link inside the area, cutting its lane there.
    auto span = crossing(3);
    const auto exit = addLink(span.d, {{120, -40}, {200, -40}}, 1, 3.5);
    addConnector(span.d, {span.major, lane(span.d, span.major), 100.0}, {exit, lane(span.d, exit)});
    validateDocument(span.d);
    const auto table = runtimeSections(span.d.network);
    CHECK(sectionForStation(table, lane(span.d, span.major), 99).end == 100); // the forcing: cut inside
    const auto r = resolve(span.d);
    CHECK(r.issues.empty()); // M3.2.3a refused this as UNSUPPORTED_CONFLICT_SPAN
    const auto s = compileDocument(span.d, test::root() / "data").scenario;
    CHECK(s.conflictZones.size() == 1);
    CHECK(s.conflictZones.front().major.segmentIds.size() == 2); // both sections of the major lane
    CHECK(runSimulation(s, 42).completed > 100);
}
TEST(rightofway_runtime, two_areas_on_one_link_both_run) {
    auto group = crossing(3);
    const auto other = addLink(group.d, {{0, 50}, {200, 50}}, 1, 3.5);
    const ControlPathRef pOther{other, lane(group.d, other), "", "", ""}, pMinor{group.minor, lane(group.d, group.minor), "", "", ""};
    const auto o = surfaceOverlap(group.d.network, pMinor, pOther);
    const auto wOther = putWaitingLine(group.d, {"", "", {pOther, 90}});
    const auto wMinor = putWaitingLine(group.d, {"", "", {pMinor, 140}});
    const auto second = putConflictArea(group.d, {"", "", ConflictKind::crossing, {pMinor, o.first.from, o.first.to, wMinor},
                                                  {pOther, o.second.from, o.second.to, wOther}, ConflictPriority::firstYields});
    putPriorityRule(group.d, {"", "", second, 3, 5});
    putInput(group.d, {"", putRoute(group.d, {"", {other}}), "car", 600, 0, 540, {}});
    validateDocument(group.d);
    CHECK(resolve(group.d).issues.empty()); // M3.2.3a refused this as UNSUPPORTED_CONFLICT_GROUP
    const auto s = compileDocument(group.d, test::root() / "data").scenario;
    CHECK(s.conflictZones.size() == 2);
    CHECK(runSimulation(s, 42).completed > 100);
    // Undetermined priority is still a draft: no zone, the specific reason named.
    auto open = crossing(3);
    open.d.network.rightOfWay.conflictAreas[0].priority = ConflictPriority::undetermined;
    CHECK(has(resolve(open.d).issues, "CONFLICT_UNDETERMINED")); CHECK(resolve(open.d).zones.empty());
}
TEST(rightofway_runtime, a_taken_over_merge_runs_on_the_solver_with_the_fallbacks_numbers) {
    // A body merge loaded on both roads, so priority binds (the reversal below is the forcing).
    const auto data = test::root() / "data";
    ProjectDocument d;
    const auto x = addLink(d, {{0, 0}, {300, 0}}, 1, 3.5);
    const auto z = addLink(d, {{0, 60}, {100, 20}}, 1, 3.5);
    const auto join = addConnector(d, {z, lane(d, z)}, {x, lane(d, x), 150.0});
    putInput(d, {"", putRoute(d, {"", {x}}), "car", 700, 0, 540, {}});
    putInput(d, {"", putRoute(d, {"", {z, join, x}}), "car", 400, 0, 540, {}});
    changeRunSettings(d, 600, 0.1);
    validateDocument(d);
    const auto fallback = compileDocument(d, data).scenario;
    CHECK(fallback.priorityRules.size() == 1); CHECK(fallback.conflictZones.empty());
    const auto& derived = fallback.priorityRules.front();
    const auto events = [](const Scenario& s) {
        std::vector<SimEvent> out;
        runSimulation(s, 42, [&](const SimEvent& e) { out.push_back(e); });
        return out;
    };
    // Taken over with the catalog's own two numbers: the same order, line and thresholds...
    takeOverMerge(d, mergeAt(d, 2), fallback.priorityDefaults);
    validateDocument(d);
    const auto taken = compileDocument(d, data).scenario;
    CHECK(taken.priorityRules.empty()); CHECK(taken.conflictZones.size() == 1);
    const auto& zone = taken.conflictZones.front();
    CHECK(zone.minor.segmentIds.front() == derived.yieldSegmentId);
    CHECK(zone.major.segmentIds.back() == derived.conflictSegmentId);
    test::near(zone.waitPosition, derived.yieldPosition, 1e-9);
    test::near(zone.gapTime, derived.gapTime, 0); test::near(zone.headway, derived.headway, 0);
    // ...run on the admission solver (D59), which also holds the major side for an admitted minor.
    const auto run = runSimulation(taken, 42);
    CHECK(run.completed > 100);
    const auto reference = events(taken);
    d.network.rightOfWay.conflictAreas[0].priority = ConflictPriority::secondYields;
    CHECK(events(compileDocument(d, data).scenario) != reference); // reversing it is observable
}
TEST(rightofway_runtime, a_taken_over_three_way_merge_runs_without_a_hold_cycle) {
    // Each middle path is minor at one zone and major at another; the order is total, so the
    // waits-for graph between zones is acyclic and the group runs (D59).
    auto t = threeWayMerge();
    std::vector<std::string> connectors;
    for (const auto& c : t.d.network.connectors) connectors.push_back(c.id);
    for (const auto& id : connectors) {
        const auto& c = *std::find_if(t.d.network.connectors.begin(), t.d.network.connectors.end(), [&](const auto& k) { return k.id == id; });
        putInput(t.d, {"", putRoute(t.d, {"", {c.from.linkId, c.id, t.x}}), "car", 300, 0, 540, {}});
    }
    changeRunSettings(t.d, 600, 0.1);
    const auto fallback = runSimulation(compileDocument(t.d, test::root() / "data").scenario, 42);
    takeOverMerge(t.d, mergeAt(t.d, 3), kDefaults);
    validateDocument(t.d);
    const auto s = compileDocument(t.d, test::root() / "data").scenario; // no CONFLICT_HOLD_CYCLE
    CHECK(s.conflictZones.size() == 3);
    const auto end = runSimulation(s, 42);
    // The three streams keep moving: all demand served, nobody left stuck at the join.
    CHECK(fallback.completed > 100); CHECK(fallback.vehicles.empty()); // the forcing: it all clears
    CHECK(end.completed == fallback.completed); CHECK(end.vehicles.empty());
}
// M3.2.5a: Stop and Yield at the minor waiting line, through the document (A18/A19 at the seam).
TEST(rightofway_runtime, a_yield_control_runs_exactly_as_no_control_and_a_stop_makes_every_minor_vehicle_stop) {
    auto c = crossing(3);
    const auto data = test::root() / "data";
    const auto stream = [&](const Scenario& s) {
        std::vector<SimEvent> out;
        runSimulation(s, 42, [&](const SimEvent& e) { out.push_back(e); });
        return out;
    };
    const auto plain = stream(compileDocument(c.d, data).scenario);
    setAreaControl(c.d, c.area, StopMode::yield); validateDocument(c.d);
    const auto yielding = compileDocument(c.d, data).scenario;
    CHECK(yielding.conflictZones.front().control == ZoneControl::yield);
    CHECK(stream(yielding) == plain); // Yield is the gap test the area already runs
    setAreaControl(c.d, c.area, StopMode::stop); validateDocument(c.d);
    const auto stopping = compileDocument(c.d, data).scenario;
    CHECK(stopping.conflictZones.front().control == ZoneControl::stop);
    // Every minor vehicle that crosses the line stood still at it first.
    const auto& zone = stopping.conflictZones.front();
    std::map<std::uint64_t, bool> stood, crossed;
    int minorVehicles = 0;
    auto state = createSimulation(stopping, 42);
    while (state.tick < totalTicks(stopping)) {
        for (const auto& v : state.vehicles) {
            // Slots index the canonical scenario the state holds, not `stopping`'s order.
            if (state.scenario->routes[v.routeIndex].segmentIds.front() != zone.minor.segmentIds.front()) continue;
            if (v.distance > zone.waitPosition + 1e-9) { if (!crossed[v.id]) { crossed[v.id] = true; ++minorVehicles; } continue; }
            if (v.speed == 0 && zone.waitPosition - v.distance <= stopLineReach(stopping.behaviours.front(), v.driverFactor)) stood[v.id] = true;
        }
        state = stepSimulation(state);
    }
    CHECK(minorVehicles > 20); // the forcing: the Stop is exercised by real demand
    for (const auto& [id, went] : crossed) if (went) CHECK(stood[id]);
    CHECK(stream(stopping) != plain);
}
