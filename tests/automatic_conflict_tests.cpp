#include "test.hpp"
#include "../tools/t_junction_network.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/model/network/right_of_way.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
using namespace trafficsim;
// M3.2.4c (D68): conflict areas the drawing implies, Vissim's automatic areas. A crossing nobody has
// set is passive; a merge shows the drawing-order rule it already runs with. Derived, never stored.
namespace {
fixture::TJunction bare(DrivingSide side = DrivingSide::right) {
    fixture::TJunctionOptions o; o.authoredControls = false; o.side = side;
    return fixture::tJunction(o);
}
const PriorityDefaults kDefaults{3, 7};
std::vector<AutomaticConflict> of(const std::vector<AutomaticConflict>& all, ConflictKind kind) {
    std::vector<AutomaticConflict> r;
    for (const auto& a : all) if (a.kind == kind) r.push_back(a);
    return r;
}
bool touches(const AutomaticConflict& a, const std::string& object) {
    for (const auto* s : {&a.first, &a.second}) if (s->path.linkId == object || s->path.connectorId == object) return true;
    return false;
}
}
TEST(automatic_conflict, the_bare_t_junction_implies_one_passive_crossing_and_two_merges) {
    for (const auto side : {DrivingSide::right, DrivingSide::left}) {
        const auto t = bare(side);
        CHECK(t.document.network.rightOfWay.empty()); // the forcing: nothing is authored
        const auto all = automaticConflicts(t.document.network);
        const auto crossings = of(all, ConflictKind::crossing), merges = of(all, ConflictKind::merge);
        CHECK(crossings.size() == 1);
        // The crossing turn over the near major stream -- not the diverge of the two turns, and
        // not a turn against the Links its own mouths lie on.
        CHECK(touches(crossings.front(), t.crossingTurn) && touches(crossings.front(), t.eastbound));
        CHECK(crossings.front().priority == ConflictPriority::undetermined);
        CHECK(merges.size() == 2);
        for (const auto& m : merges) CHECK(m.priority == ConflictPriority::firstYields && !m.mergeSection.empty());
    }
}
TEST(automatic_conflict, a_derived_merge_is_exactly_what_a_take_over_stores) {
    auto d = bare().document;
    const auto shown = of(automaticConflicts(d.network), ConflictKind::merge);
    for (const auto& m : shown) {
        const auto id = authorAutomaticConflict(d, m, kDefaults);
        const auto& a = *std::find_if(d.network.rightOfWay.conflictAreas.begin(), d.network.rightOfWay.conflictAreas.end(),
                                      [&](const auto& x) { return x.id == id; });
        CHECK(a.first.path == m.first.path && a.second.path == m.second.path);
        test::near(a.first.entryStation, m.first.entryStation, 0); test::near(a.first.exitStation, m.first.exitStation, 0);
        test::near(a.second.entryStation, m.second.entryStation, 0); test::near(a.second.exitStation, m.second.exitStation, 0);
        CHECK(a.priority == m.priority);
    }
    CHECK(of(automaticConflicts(d.network), ConflictKind::merge).empty()); // taken over: no longer automatic
    validateDocument(d);
}
TEST(automatic_conflict, a_click_authors_what_add_crossing_areas_would_and_delete_restores_passive) {
    auto clicked = bare().document;
    const auto passive = of(automaticConflicts(clicked.network), ConflictKind::crossing).front();
    const auto id = authorAutomaticConflict(clicked, passive, kDefaults);
    validateDocument(clicked);
    auto added = bare();
    addCrossingAreas(added.document, added.crossingTurn, added.eastbound, added.crossingTurn, kDefaults);
    const auto& a = clicked.network.rightOfWay.conflictAreas.front();
    const auto& b = added.document.network.rightOfWay.conflictAreas.front();
    // The same pair, extents, lines and rule; the Connector gives way to the Link, as the fixture's.
    const auto sideOf = [](const ConflictArea& x, bool connector) { return (x.first.path.connectorId.empty() != connector) ? x.first : x.second; };
    for (const bool connector : {true, false}) {
        CHECK(sideOf(a, connector).path == sideOf(b, connector).path);
        test::near(sideOf(a, connector).entryStation, sideOf(b, connector).entryStation, 1e-12);
        test::near(sideOf(a, connector).exitStation, sideOf(b, connector).exitStation, 1e-12);
    }
    const auto yielding = [](const ConflictArea& x) { return (x.priority == ConflictPriority::firstYields ? x.first : x.second).path; };
    CHECK(yielding(a) == yielding(b) && !yielding(a).connectorId.empty());
    const auto station = [](const ProjectDocument& d, const std::string& line) {
        for (const auto& w : d.network.rightOfWay.waitingLines) if (w.id == line) return w.point.station;
        return -1.0;
    };
    test::near(station(clicked, sideOf(a, true).waitingLineId), station(added.document, sideOf(b, true).waitingLineId), 1e-12);
    CHECK(clicked.network.rightOfWay.priorityRules.size() == 1 && clicked.network.rightOfWay.priorityRules.front().conflictAreaId == id);
    CHECK(of(automaticConflicts(clicked.network), ConflictKind::crossing).empty()); // authored: no longer passive
    test::throws([&] { authorAutomaticConflict(clicked, passive, kDefaults); }, "EDIT_NO_CROSSING"); // a stale click
    // Delete: passive again, and nothing of it is left behind.
    removeConflictArea(clicked, id);
    CHECK(clicked.network.rightOfWay.empty());
    CHECK(of(automaticConflicts(clicked.network), ConflictKind::crossing) == std::vector{passive});
}
TEST(automatic_conflict, a_lanes_second_area_shares_its_line_moved_before_the_first_area) {
    // One road crossed by two others: its lane meets two areas (D63).
    ProjectDocument d;
    const auto road = addLink(d, {{0, 0}, {200, 0}}, 1, 3.5);
    addLink(d, {{80, -60}, {80, 60}}, 1, 3.5);
    addLink(d, {{120, -60}, {120, 60}}, 1, 3.5);
    const auto passive = of(automaticConflicts(d.network), ConflictKind::crossing);
    CHECK(passive.size() == 2);
    // Author the far one first, then the near one: the road's line must end up before the near one.
    const auto onRoad = [&](const AutomaticConflict& a) { return a.first.path.linkId == road ? a.first : a.second; };
    const auto near = onRoad(passive[0]).entryStation < onRoad(passive[1]).entryStation ? passive[0] : passive[1];
    const auto far = near.key == passive[0].key ? passive[1] : passive[0];
    authorAutomaticConflict(d, far, kDefaults);
    authorAutomaticConflict(d, near, kDefaults);
    validateDocument(d);
    std::vector<std::string> roadLines;
    for (const auto& a : d.network.rightOfWay.conflictAreas)
        for (const auto* s : {&a.first, &a.second}) if (s->path.linkId == road) roadLines.push_back(s->waitingLineId);
    CHECK(roadLines.size() == 2 && roadLines[0] == roadLines[1]); // one line for the lane
    const auto& line = *std::find_if(d.network.rightOfWay.waitingLines.begin(), d.network.rightOfWay.waitingLines.end(),
                                     [&](const auto& w) { return w.id == roadLines[0]; });
    test::near(line.point.station, onRoad(near).entryStation - 1, 1e-9);
}
TEST(automatic_conflict, levels_apart_do_not_conflict_and_order_does_not_matter) {
    ProjectDocument d;
    addLink(d, {{0, 0}, {200, 0}}, 1, 3.5);
    const auto bridge = addLink(d, {{100, -60}, {100, 60}}, 1, 3.5);
    CHECK(of(automaticConflicts(d.network), ConflictKind::crossing).size() == 1); // the forcing: at grade they cross
    editableLink(d, bridge).level = 1;
    CHECK(automaticConflicts(d.network).empty());
    // The same drawing listed in another order derives the same areas.
    const auto t = bare();
    auto shuffled = t.document.network;
    std::reverse(shuffled.links.begin(), shuffled.links.end());
    std::reverse(shuffled.connectors.begin(), shuffled.connectors.end());
    CHECK(automaticConflicts(shuffled) == automaticConflicts(t.document.network));
}
TEST(automatic_conflict, passive_areas_change_nothing_at_run) {
    // Derived, never stored: a bare drawing compiles to no zone, as before automatic areas existed.
    const auto t = bare();
    CHECK(!automaticConflicts(t.document.network).empty()); // the forcing: there is something to show
    const auto s = compileDocument(t.document, test::root() / "data").scenario;
    CHECK(s.conflictZones.empty());
    CHECK(!documentJson(t.document)["network"].contains("rightOfWay"));
}
