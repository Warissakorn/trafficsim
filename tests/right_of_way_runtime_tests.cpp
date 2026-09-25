#include "test.hpp"
#include "right_of_way_fixture.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/project/run.hpp"
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
            if (r.id == a->routeId && r.segmentIds.front() == s.conflictZones.front().minor.segmentId) { total += a->travelTime; ++n; }
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
    CHECK(zone.minor.segmentId == lane(c.d, c.minor)); CHECK(zone.major.segmentId == lane(c.d, c.major));
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
TEST(rightofway_runtime, what_the_first_slice_cannot_run_is_refused_by_name) {
    // An area over a section cut: a Connector leaves the major Link inside the area.
    auto span = crossing(3);
    const auto exit = addLink(span.d, {{120, -40}, {200, -40}}, 1, 3.5);
    addConnector(span.d, {span.major, lane(span.d, span.major), 100.0}, {exit, lane(span.d, exit)});
    validateDocument(span.d);
    const auto table = runtimeSections(span.d.network);
    CHECK(sectionForStation(table, lane(span.d, span.major), 99).end == 100); // the forcing: cut inside
    CHECK(has(resolve(span.d).issues, "UNSUPPORTED_CONFLICT_SPAN"));
    CHECK(resolve(span.d).zones.empty());
    test::throws([&] { compileDocument(span.d, test::root() / "data"); }, "UNSUPPORTED_CONFLICT_SPAN");
    // Two areas on the minor Link: a connected group needs atomic admission (M3.2.3b).
    auto group = crossing(3);
    const auto other = addLink(group.d, {{0, 50}, {200, 50}}, 1, 3.5);
    const ControlPathRef pOther{other, lane(group.d, other), "", "", ""}, pMinor{group.minor, lane(group.d, group.minor), "", "", ""};
    const auto o = surfaceOverlap(group.d.network, pMinor, pOther);
    const auto wOther = putWaitingLine(group.d, {"", "", {pOther, 90}});
    const auto wMinor = putWaitingLine(group.d, {"", "", {pMinor, 140}});
    const auto second = putConflictArea(group.d, {"", "", ConflictKind::crossing, {pMinor, o.first.from, o.first.to, wMinor},
                                                  {pOther, o.second.from, o.second.to, wOther}, ConflictPriority::firstYields});
    putPriorityRule(group.d, {"", "", second, 3, 5});
    validateDocument(group.d);
    const auto r = resolve(group.d);
    CHECK(count(r.issues, "UNSUPPORTED_CONFLICT_GROUP") == 2); CHECK(r.zones.empty());
    // Undetermined priority is still a draft: no zone, the specific reason named.
    auto open = crossing(3);
    open.d.network.rightOfWay.conflictAreas[0].priority = ConflictPriority::undetermined;
    CHECK(has(resolve(open.d).issues, "CONFLICT_UNDETERMINED")); CHECK(resolve(open.d).zones.empty());
}
