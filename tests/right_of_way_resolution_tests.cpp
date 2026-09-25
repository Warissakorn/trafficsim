#include "test.hpp"
#include "right_of_way_fixture.hpp"
#include "../src/commands/connector_commands.hpp"
#include <cmath>
using namespace trafficsim;
using namespace rowfixture;
// M3.2.2c: a waiting line on a preceding Link, and crossing coverage (docs/M3_CONTRACT.md §1).
// Every authored area is still Run-blocked by UNSUPPORTED_CONFLICT_RUNTIME until M3.2.3.
namespace {
const Link& link(const ProjectDocument& d, const std::string& id) {
    for (const auto& l : d.network.links) if (l.id == id) return l;
    throw std::runtime_error("no link");
}
std::string lane(const ProjectDocument& d, const std::string& id, std::size_t k = 0) { return link(d, id).lanes[k].id; }
ControlPathRef onLane(const ProjectDocument& d, const std::string& id, std::size_t k = 0) { return {id, lane(d, id, k), "", "", ""}; }
bool onlyRuntimeMarker(const RightOfWayResolution& r) {
    return count(r.issues, "UNSUPPORTED_CONFLICT_RUNTIME") == static_cast<int>(r.issues.size());
}
bool coreRefuses(const ProjectDocument& d, const std::string& code) {
    return has(validateScenario(buildScenario(d.network, ScenarioDefinition{.priorityDefaults = kDefaults})), code);
}
// A taken-over three-way merge, with the yielding side of one area moved onto the Link before
// its Connector, `before` metres short of that Link's end.
struct Upstream { Three t; std::string approach, yielding, line; double length{}; };
Upstream lineOnApproach(double before) {
    Upstream u{threeWayMerge(), {}, {}, {}, 0};
    takeOverMerge(u.t.d, mergeAt(u.t.d, 3), kDefaults);
    const auto& area = u.t.d.network.rightOfWay.conflictAreas.back(); // the last-drawn path yields
    CHECK(area.priority == ConflictPriority::firstYields);
    const auto& connector = *std::find_if(u.t.d.network.connectors.begin(), u.t.d.network.connectors.end(),
                                          [&](const auto& c) { return c.id == area.first.path.connectorId; });
    u.approach = connector.from.linkId;
    u.yielding = connectorPathId(connector, 0);
    u.line = area.first.waitingLineId;
    u.length = polylineLength(link(u.t.d, u.approach).geometry);
    auto line = *std::find_if(u.t.d.network.rightOfWay.waitingLines.begin(), u.t.d.network.rightOfWay.waitingLines.end(),
                              [&](const auto& w) { return w.id == u.line; });
    line.point = {onLane(u.t.d, u.approach), u.length - before};
    putWaitingLine(u.t.d, line);
    validateDocument(u.t.d);
    return u;
}
}
TEST(rightofway_resolution, a_waiting_line_on_the_preceding_link_compiles_before_the_connector) {
    const auto u = lineOnApproach(10);
    // The forcing: the line really is on another object than the side it serves.
    CHECK(u.t.d.network.rightOfWay.conflictAreas.back().first.path.connectorId != "");
    const auto r = resolve(u.t.d);
    CHECK(onlyRuntimeMarker(r)); // M3.2.2b reported CONFLICT_WAITING_LINE_UNSUPPORTED here
    const auto rules = std::count_if(r.rules.begin(), r.rules.end(), [&](const auto& x) {
        // A straight one-lane approach: its lane stations are its reference stations.
        return x.yieldSegmentId == u.yielding && std::abs(x.yieldPosition + 10) < 1e-9; });
    CHECK(rules == 2); // it yields to both earlier paths, and waits at the same line for each
    CHECK(!coreRefuses(u.t.d, "INVALID_POSITION")); CHECK(!coreRefuses(u.t.d, "UNSUPPORTED_MERGE"));
}
TEST(rightofway_resolution, a_line_a_route_can_go_round_is_refused_by_name) {
    auto u = lineOnApproach(60); // approach station ~25 of ~85
    auto& d = u.t.d;
    // A Connector joining the approach between the line and its end: a second way onto it.
    const auto w = addLink(d, {{-120, -120}, {-80, -80}}, 1, 3.5);
    const auto join = addConnector(d, {w, lane(d, w)}, {u.approach, lane(d, u.approach), u.length - 30});
    // And one leaving before the join: a diverge takes nobody round the line.
    const auto v = addLink(d, {{-60, -80}, {-40, -120}}, 1, 3.5);
    addConnector(d, {u.approach, lane(d, u.approach), u.length - 45}, {v, lane(d, v)});
    validateDocument(d);
    const auto line = *std::find_if(d.network.rightOfWay.waitingLines.begin(), d.network.rightOfWay.waitingLines.end(),
                                    [&](const auto& x) { return x.id == u.line; });
    CHECK(line.point.station < u.length - 45); // the forcing: the line is upstream of both
    CHECK(has(resolve(d).issues, "CONFLICT_WAITING_LINE_BYPASSED"));
    CHECK(rulesWithPrefix(resolve(d), "right-of-way/") == 0); // nothing is compiled for the group
    // Past the join the line is on every route again.
    auto after = d;
    auto moved = line; moved.point.station = u.length - 10; putWaitingLine(after, moved);
    CHECK(onlyRuntimeMarker(resolve(after)));
    // Without the join, the diverge alone does not stop the original line resolving.
    deleteObjects(d, {join});
    validateDocument(d);
    const auto r = resolve(d);
    CHECK(onlyRuntimeMarker(r));
    CHECK(std::any_of(r.rules.begin(), r.rules.end(), [&](const auto& x) {
        return x.yieldSegmentId == u.yielding && std::abs(x.yieldPosition + 60) < 1e-9; }));
    CHECK(!coreRefuses(d, "INVALID_POSITION"));
    // A line nowhere upstream of the side is named as such.
    auto elsewhere = d;
    moved.point = {onLane(d, u.t.x), 50};
    putWaitingLine(elsewhere, moved);
    CHECK(has(resolve(elsewhere).issues, "CONFLICT_WAITING_LINE_NOT_UPSTREAM"));
}
TEST(rightofway_resolution, a_line_earlier_on_the_same_lane_compiles_back_along_it) {
    // The short-section merge from right_of_way_tests.cpp: X's own lane is cut at 50 and 50.6.
    ProjectDocument d;
    const auto x = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5);
    const auto y = addLink(d, {{60, 30}, {100, 30}}, 1, 3.5);
    const auto z = addLink(d, {{0, 40}, {40, 20}}, 1, 3.5);
    addConnector(d, {x, lane(d, x), 50.0}, {y, lane(d, y)});
    addConnector(d, {z, lane(d, z)}, {x, lane(d, x), 50.6});
    takeOverMerge(d, mergeAt(d, 2), kDefaults);
    auto& area = d.network.rightOfWay.conflictAreas.front();
    const auto& xSide = area.first.path.linkId == x ? area.first : area.second;
    // X yields instead, waiting 30 m back, in the section before the one it yields from.
    area.priority = &xSide == &area.first ? ConflictPriority::firstYields : ConflictPriority::secondYields;
    auto line = *std::find_if(d.network.rightOfWay.waitingLines.begin(), d.network.rightOfWay.waitingLines.end(),
                              [&](const auto& w) { return w.id == xSide.waitingLineId; });
    line.point.station = 20;
    putWaitingLine(d, line);
    validateDocument(d);
    const auto table = runtimeSections(d.network);
    const auto& minor = sectionForStation(table, lane(d, x), 50.3);
    CHECK(minor.start == 50); // the forcing: the line lies in an earlier section than the side
    const auto r = resolve(d);
    CHECK(onlyRuntimeMarker(r));
    const auto rule = std::find_if(r.rules.begin(), r.rules.end(), [&](const auto& k) { return k.yieldSegmentId == minor.id; });
    CHECK(rule != r.rules.end());
    // M3.2.2b compiled this to the end of the short section, 0.6 m, without a word.
    test::near(rule->yieldPosition, -30, 1e-9);
}
namespace {
// Link A (optionally curved, several lanes) crossed by a straight one-lane Link B; one crossing
// area on A's last lane and B, authored from the measured overlap.
struct Cross { ProjectDocument d; std::string a, b, area; ControlPathRef pa, pb; };
Cross crossing(DrivingSide side, const std::vector<Point>& shape, int lanes) {
    Cross c;
    changeDrivingSide(c.d, side);
    c.a = addLink(c.d, shape, lanes, 3.5);
    c.b = addLink(c.d, {{50, -60}, {50, 60}}, 1, 3.5);
    c.pa = onLane(c.d, c.a, static_cast<std::size_t>(lanes - 1)); c.pb = onLane(c.d, c.b);
    const auto o = surfaceOverlap(c.d.network, c.pa, c.pb);
    if (o.status != SurfaceOverlap::Status::overlap) throw std::runtime_error("fixture does not cross");
    const auto wa = putWaitingLine(c.d, {"", "", {c.pa, o.first.from - 5}});
    const auto wb = putWaitingLine(c.d, {"", "", {c.pb, o.second.from - 5}});
    c.area = putConflictArea(c.d, {"", "", ConflictKind::crossing, {c.pa, o.first.from, o.first.to, wa},
                                   {c.pb, o.second.from, o.second.to, wb}, ConflictPriority::firstYields});
    putPriorityRule(c.d, {"", "", c.area, 3, 7});
    validateDocument(c.d);
    return c;
}
// An independent reading of "A's cross-section at station s meets B's surface": sample the
// cross-section and test each point against B's lane polygon by ray casting.
bool meets(const ProjectDocument& d, const Cross& c, double s) {
    const auto& a = link(d, c.a);
    const auto k = a.lanes.size() - 1;
    const auto l = laneBoundaryGeometry(a, k, d.network.drivingSide), r = laneBoundaryGeometry(a, k + 1, d.network.drivingSide);
    const auto p = pointAlong(l, matchedStation(a.geometry, l, s)), q = pointAlong(r, matchedStation(a.geometry, r, s));
    const auto& b = link(d, c.b);
    auto polygon = laneBoundaryGeometry(b, 0, d.network.drivingSide);
    const auto other = laneBoundaryGeometry(b, 1, d.network.drivingSide);
    polygon.insert(polygon.end(), other.rbegin(), other.rend());
    for (int i = 1; i < 1000; ++i) {
        const Point x{p.x + (q.x - p.x) * i / 1000, p.y + (q.y - p.y) * i / 1000};
        bool inside = false;
        for (std::size_t j = 0, m = polygon.size() - 1; j < polygon.size(); m = j++)
            if ((polygon[j].y > x.y) != (polygon[m].y > x.y) &&
                x.x < (polygon[m].x - polygon[j].x) * (x.y - polygon[j].y) / (polygon[m].y - polygon[j].y) + polygon[j].x)
                inside = !inside;
        if (inside) return true;
    }
    return false;
}
}
TEST(rightofway_resolution, a_crossing_area_must_cover_the_real_overlap) {
    // Straight and square: the overlap is known by hand, 48.25..51.75 on both.
    auto c = crossing(DrivingSide::left, {{0, 0}, {100, 0}}, 1);
    const auto o = surfaceOverlap(c.d.network, c.pa, c.pb);
    test::near(o.first.from, 48.25, 1e-9); test::near(o.first.to, 51.75, 1e-9);
    test::near(o.second.from, 58.25, 1e-9); test::near(o.second.to, 61.75, 1e-9);
    CHECK(onlyRuntimeMarker(resolve(c.d)));
    // An extent that stops short of the overlap is named, on the side that is short.
    auto shortExit = c.d;
    shortExit.network.rightOfWay.conflictAreas[0].first.exitStation = 51;
    CHECK(51 < o.first.to); // the forcing
    const auto r = resolve(shortExit);
    CHECK(std::any_of(r.issues.begin(), r.issues.end(), [](const auto& i) {
        return i.code == "CONFLICT_EXTENT_UNCOVERED" && i.path == "rightOfWay.conflictAreas[0].first"; }));
    CHECK(count(r.issues, "CONFLICT_EXTENT_UNCOVERED") == 1);
    // A larger area than the overlap is the author's choice, not an error.
    auto larger = c.d;
    larger.network.rightOfWay.conflictAreas[0].second.exitStation = 70;
    CHECK(onlyRuntimeMarker(resolve(larger)));
    // Move B off A: the stored numbers stay, and the area no longer protects anything.
    auto apart = c.d;
    changeGeometry(apart, c.b, {{150, -60}, {150, 60}});
    validateDocument(apart);
    CHECK(surfaceOverlap(apart.network, c.pa, c.pb).status == SurfaceOverlap::Status::none);
    CHECK(has(resolve(apart).issues, "CONFLICT_NO_OVERLAP"));
    CHECK(apart.network.rightOfWay == c.d.network.rightOfWay);
    // Reshape A to cross B twice: which crossing the area meant would be a guess.
    auto twice = c.d;
    changeGeometry(twice, c.a, {{0, -20}, {80, -20}, {80, 20}, {0, 20}});
    validateDocument(twice);
    CHECK(surfaceOverlap(twice.network, c.pa, c.pb).status == SurfaceOverlap::Status::unsupported);
    CHECK(has(resolve(twice).issues, "CONFLICT_GEOMETRY_UNSUPPORTED"));
}
TEST(rightofway_resolution, the_overlap_is_measured_on_curved_lanes_for_both_driving_sides) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        auto c = crossing(side, {{0, -30}, {30, -20}, {60, 5}, {90, 40}}, 2);
        const auto o = surfaceOverlap(c.d.network, c.pa, c.pb);
        // Just outside the interval A's cross-section misses B; just inside it meets B.
        CHECK(o.first.to - o.first.from > 1); // the forcing: a real crossing, not a touch
        CHECK(!meets(c.d, c, o.first.from - 0.05)); CHECK(meets(c.d, c, o.first.from + 0.05));
        CHECK(meets(c.d, c, o.first.to - 0.05)); CHECK(!meets(c.d, c, o.first.to + 0.05));
        CHECK(onlyRuntimeMarker(resolve(c.d)));
    }
}
