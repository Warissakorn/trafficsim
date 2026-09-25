#include "test.hpp"
#include "right_of_way_fixture.hpp"
#include "../src/commands/connector_commands.hpp"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <regex>
#include <set>
using namespace trafficsim;
using namespace rowfixture;
// M3.2.4: the commands and geometry the conflict-area editor stands on (docs/M3_ACCEPTANCE.md A24).
namespace {
std::string lane(const ProjectDocument& d, const std::string& link, std::size_t k = 0) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes[k].id;
    throw std::runtime_error("no link");
}
struct Pair { ProjectDocument d; std::string a, b; };
// A two-lane Link along x crossed by a one-lane Link along y at x = 50.
Pair crossingLinks() {
    Pair p;
    p.a = addLink(p.d, {{0, 0}, {100, 0}}, 2, 3.5);
    p.b = addLink(p.d, {{50, -60}, {50, 60}}, 1, 3.5);
    return p;
}
}
TEST(rightofway_editor, crossing_areas_expand_to_every_crossing_lane_pair) {
    auto p = crossingLinks();
    History h; h.reset(p.d);
    std::vector<std::string> areas;
    CHECK(h.execute("add", [&](ProjectDocument& d) { areas = addCrossingAreas(d, p.a, p.b, p.b, kDefaults); }));
    CHECK(areas.size() == 2); // both lanes of A cross B's one lane
    const auto& row = h.document().network.rightOfWay;
    CHECK(row.conflictAreas.size() == 2); CHECK(row.waitingLines.size() == 4); CHECK(row.priorityRules.size() == 2);
    for (const auto& a : row.conflictAreas) {
        CHECK(a.kind == ConflictKind::crossing);
        CHECK(a.priority == ConflictPriority::secondYields); // B, the second, gives way
        const auto o = surfaceOverlap(h.document().network, a.first.path, a.second.path);
        test::near(a.first.entryStation, o.first.from, 1e-12); test::near(a.second.exitStation, o.second.to, 1e-12);
        const auto line = std::find_if(row.waitingLines.begin(), row.waitingLines.end(), [&](const auto& w) { return w.id == a.second.waitingLineId; });
        test::near(line->point.station, o.second.from - 1, 1e-12);
    }
    for (const auto& r : row.priorityRules) { test::near(r.gapTime, kDefaults.gapTime, 0); test::near(r.headway, kDefaults.headway, 0); }
    // What the gesture makes runs: nothing reported, one zone per area.
    const auto r = resolve(h.document());
    CHECK(r.issues.empty()); CHECK(r.zones.size() == 2);
    h.undo(); CHECK(h.document() == p.d); // one step
}
TEST(rightofway_editor, a_crossing_gesture_that_cannot_apply_changes_nothing) {
    auto p = crossingLinks();
    const auto parallel = addLink(p.d, {{0, 20}, {100, 20}}, 1, 3.5);
    History h; h.reset(p.d);
    // The forcing: the parallel Link really lies clear of A's surface.
    CHECK(surfaceOverlap(p.d.network, {p.a, lane(p.d, p.a), "", "", ""}, {parallel, lane(p.d, parallel), "", "", ""}).status ==
          SurfaceOverlap::Status::none);
    test::throws([&] { h.execute("x", [&](ProjectDocument& d) { addCrossingAreas(d, p.a, parallel, p.a, kDefaults); }); }, "EDIT_NO_CROSSING");
    test::throws([&] { h.execute("x", [&](ProjectDocument& d) { addCrossingAreas(d, p.a, p.a, p.a, kDefaults); }); }, "EDIT_SAME_OBJECT");
    test::throws([&] { h.execute("x", [&](ProjectDocument& d) { addCrossingAreas(d, p.a, "nothing", p.a, kDefaults); }); }, "EDIT_UNKNOWN_OBJECT");
    test::throws([&] { h.execute("x", [&](ProjectDocument& d) { addCrossingAreas(d, p.a, p.b, p.a, {0, 0}); }); }, "EDIT_NO_PRIORITY_DEFAULTS");
    CHECK(h.document() == p.d);
}
TEST(rightofway_editor, one_step_sets_priority_and_the_rules_numbers) {
    auto p = crossingLinks();
    const auto id = addCrossingAreas(p.d, p.a, p.b, p.b, kDefaults).front();
    History h; h.reset(p.d);
    CHECK(h.execute("set", [&](ProjectDocument& d) { setConflictControl(d, id, "school crossing", ConflictPriority::firstYields, 4.5, 12); }));
    const auto& row = h.document().network.rightOfWay;
    CHECK(row.conflictAreas.front().name == "school crossing");
    CHECK(row.conflictAreas.front().priority == ConflictPriority::firstYields);
    CHECK(row.priorityRules.front().gapTime == 4.5); CHECK(row.priorityRules.front().headway == 12);
    // A rule deleted earlier is created again, so a decided area never stays unruled by this path.
    auto noRule = h.document();
    deletePriorityRule(noRule, noRule.network.rightOfWay.priorityRules.front().id);
    setConflictControl(noRule, id, "", ConflictPriority::secondYields, 3, 7);
    CHECK(std::count_if(noRule.network.rightOfWay.priorityRules.begin(), noRule.network.rightOfWay.priorityRules.end(),
                        [&](const auto& r) { return r.conflictAreaId == id; }) == 1);
    // A number the structure refuses is refused whole.
    test::throws([&] { h.execute("bad", [&](ProjectDocument& d) { setConflictControl(d, id, "", ConflictPriority::firstYields, 0, 12); }); },
                 "INVALID_PRIORITY_RULE");
}
TEST(rightofway_editor, take_over_and_restore_are_named_actions_on_the_objects) {
    auto t = threeWayMerge();
    const auto connector = t.d.network.connectors[1].id;
    History h; h.reset(t.d);
    CHECK(h.execute("take", [&](ProjectDocument& d) { CHECK(takeOverMergesOf(d, connector, kDefaults).size() == 3); }));
    test::throws([&] { h.execute("again", [&](ProjectDocument& d) { takeOverMergesOf(d, connector, kDefaults); }); }, "EDIT_NO_MERGE");
    const auto area = h.document().network.rightOfWay.conflictAreas.front().id;
    CHECK(h.execute("restore", [&](ProjectDocument& d) { restoreAutomaticPriorityOf(d, area); }));
    CHECK(h.document().network.rightOfWay.empty());
    // A crossing has no automatic priority to go back to.
    auto p = crossingLinks();
    const auto crossing = addCrossingAreas(p.d, p.a, p.b, p.b, kDefaults).front();
    test::throws([&] { restoreAutomaticPriorityOf(p.d, crossing); }, "EDIT_NO_MERGE");
}
TEST(rightofway_editor, deleting_an_area_keeps_lines_other_areas_still_use) {
    auto t = threeWayMerge();
    takeOverMerge(t.d, mergeAt(t.d, 3), kDefaults);
    const auto before = t.d.network.rightOfWay.waitingLines.size();
    CHECK(before == 3); // the forcing: three areas share three lines
    removeConflictArea(t.d, t.d.network.rightOfWay.conflictAreas.front().id);
    CHECK(t.d.network.rightOfWay.conflictAreas.size() == 2);
    CHECK(t.d.network.rightOfWay.waitingLines.size() == 3); // every line still serves another area
    auto p = crossingLinks();
    const auto id = addCrossingAreas(p.d, p.a, p.b, p.b, kDefaults).front();
    removeConflictArea(p.d, id);
    CHECK(p.d.network.rightOfWay.waitingLines.size() == 2); // this area's own two lines went with it
    validateDocument(p.d);
}
TEST(rightofway_editor, the_drawn_area_and_line_are_where_the_runtime_puts_them) {
    auto p = crossingLinks();
    addCrossingAreas(p.d, p.a, p.b, p.b, kDefaults);
    const auto& a = p.d.network.rightOfWay.conflictAreas.back(); // A's second lane
    const auto outline = conflictSideOutline(p.d.network, a.second);
    CHECK(outline.size() == 4);
    double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
    for (const auto& q : outline) { minX = std::min(minX, q.x); maxX = std::max(maxX, q.x); minY = std::min(minY, q.y); maxY = std::max(maxY, q.y); }
    test::near(minX, 48.25, 1e-9); test::near(maxX, 51.75, 1e-9); // B's own lane width
    // B's extent is exactly A's second lane band across it.
    const auto& linkA = p.d.network.links.front();
    const auto band = laneGeometry(linkA, linkA.lanes[1].id, p.d.network.drivingSide).front().y;
    test::near((minY + maxY) / 2, band, 1e-9); test::near(maxY - minY, 3.5, 1e-9);
    const auto line = std::find_if(p.d.network.rightOfWay.waitingLines.begin(), p.d.network.rightOfWay.waitingLines.end(),
                                   [&](const auto& w) { return w.id == a.second.waitingLineId; });
    const auto bar = waitingLineBar(p.d.network, line->point);
    CHECK(bar.has_value());
    test::near(bar->first.y, minY - 1, 1e-9); test::near(bar->second.y, minY - 1, 1e-9); // 1 m short of entry
    CHECK(std::abs(std::abs(bar->first.x - bar->second.x) - 3.5) < 1e-9);
    // A reference that no longer resolves draws nothing rather than a guess.
    auto stale = a.second; stale.path.laneId = "gone";
    CHECK(conflictSideOutline(p.d.network, stale).empty());
}
TEST(rightofway_editor, every_right_of_way_code_has_english_and_thai_text) {
    // Scraped from the sources that emit them, so a new code cannot ship untranslated.
    std::set<std::string> codes;
    const std::regex code("\"((?:CONFLICT|EDIT_NO_CROSSING|EDIT_NO_MERGE|EDIT_SAME_OBJECT|EDIT_ALREADY_EXPLICIT|EDIT_SPLIT_CONTROL|"
                          "UNKNOWN_CONTROL|INVALID_CONTROL|UNKNOWN_WAITING|UNKNOWN_CONFLICT|DUPLICATE_PRIORITY|INVALID_PRIORITY|"
                          "INVALID_CONFLICT|DUPLICATE_STOP|INVALID_STOP|EDIT_UNDETERMINED)[A-Z_]*)\"");
    for (const auto* source : {"src/model/network/right_of_way.cpp", "src/core/validate.cpp", "src/core/conflicts.cpp",
                               "src/commands/conflict_authoring.cpp", "src/commands/right_of_way_commands.cpp"}) {
        std::ifstream file(test::root() / source);
        const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        for (std::sregex_iterator it(text.begin(), text.end(), code), end; it != end; ++it) codes.insert((*it)[1]);
    }
    CHECK(codes.size() > 20); // the forcing: the scrape found the codes
    for (const auto* language : {"en", "th"}) {
        std::ifstream file(test::root() / "data/locales" / (std::string(language) + ".json"));
        Json locale; file >> locale;
        for (const auto& c : codes) {
            if (!locale.contains(c)) std::printf("missing %s in %s\n", c.c_str(), language);
            CHECK(locale.contains(c) && !locale.at(c).get<std::string>().empty());
        }
    }
}
TEST(rightofway_editor, a_click_cycles_priority_through_three_states_in_one_step_each) {
    auto p = crossingLinks();
    const auto id = addCrossingAreas(p.d, p.a, p.b, p.b, kDefaults).front();
    setConflictControl(p.d, id, "school", ConflictPriority::secondYields, 4.5, 12);
    History h; h.reset(p.d);
    const auto& row = h.document().network.rightOfWay;
    std::vector<ConflictPriority> seen;
    for (int i = 0; i < 3; ++i) {
        CHECK(h.execute("cycle", [&](ProjectDocument& d) { seen.push_back(cycleConflictPriority(d, id, kDefaults)); }));
        CHECK(row.conflictAreas.front().priority == seen.back());
        CHECK(row.conflictAreas.front().name == "school"); // the rest of the control is kept
        CHECK(row.priorityRules.front().gapTime == 4.5); CHECK(row.priorityRules.front().headway == 12);
    }
    CHECK(seen == (std::vector{ConflictPriority::undetermined, ConflictPriority::firstYields, ConflictPriority::secondYields}));
    h.undo(); CHECK(h.document().network.rightOfWay.conflictAreas.front().priority == ConflictPriority::firstYields); // one step each
    const auto before = h.document();
    test::throws([&] { h.execute("x", [&](ProjectDocument& d) { cycleConflictPriority(d, "nothing", kDefaults); }); }, "EDIT_UNKNOWN_OBJECT");
    CHECK(h.document() == before);
}
TEST(rightofway_editor, a_waiting_line_moves_along_the_polyline_its_bar_is_drawn_on) {
    auto p = crossingLinks();
    addCrossingAreas(p.d, p.a, p.b, p.b, kDefaults);
    // A Connector path too: a line on one is measured on the Connector's base polyline.
    const auto c = addLink(p.d, {{130, 0}, {200, 0}}, 1, 3.5);
    addConnector(p.d, {p.a, lane(p.d, p.a, 0)}, {c, lane(p.d, c)});
    const auto& connector = p.d.network.connectors.front();
    const auto paths = connectorPaths(p.d.network, connector);
    CHECK(!paths.empty());
    const ControlPathRef onConnector{"", "", connector.id, paths.front().from.laneId, paths.front().to.laneId};
    const auto line = p.d.network.rightOfWay.waitingLines.front(); // on A's first lane, offset from A's reference polyline
    for (const auto& point : {line.point, ControlPoint{onConnector, 10}}) {
        const auto polyline = controlPathPolyline(p.d.network, point.path);
        CHECK(polyline.size() >= 2);
        const auto bar = waitingLineBar(p.d.network, point);
        CHECK(bar.has_value());
        const auto mid = Point{(bar->first.x + bar->second.x) / 2, (bar->first.y + bar->second.y) / 2};
        const auto on = pointAlong(polyline, point.station);
        // The bar crosses the polyline's normal at that station: the offset is purely lateral.
        const auto ahead = pointAlong(polyline, point.station + .5);
        const double along = ((mid.x - on.x) * (ahead.x - on.x) + (mid.y - on.y) * (ahead.y - on.y)) / .5;
        test::near(along, 0, 1e-6);
    }
    CHECK(controlPathPolyline(p.d.network, {p.a, "gone", "", "", ""}).empty());
    History h; h.reset(p.d);
    CHECK(h.execute("move", [&](ProjectDocument& d) { moveWaitingLine(d, line.id, line.point.station - 5); }));
    test::near(h.document().network.rightOfWay.waitingLines.front().point.station, line.point.station - 5, 1e-12);
    // Past its area's entry: kept, and reported by the resolver rather than refused.
    CHECK(h.execute("past", [&](ProjectDocument& d) { moveWaitingLine(d, line.id, 70); }));
    const auto r = resolve(h.document());
    CHECK(std::any_of(r.issues.begin(), r.issues.end(), [](const auto& i) { return i.code == "CONFLICT_WAITING_LINE_AFTER_ENTRY"; }));
    test::throws([&] { h.execute("x", [&](ProjectDocument& d) { moveWaitingLine(d, "nothing", 1); }); }, "EDIT_UNKNOWN_OBJECT");
}
