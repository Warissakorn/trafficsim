#include "test.hpp"
#include "right_of_way_fixture.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/connector_commands.hpp"
using namespace trafficsim;
using namespace rowfixture;
// M3.2.2b: authored controls follow the objects they name (docs/M3_ACCEPTANCE.md A05) -- on a
// curved road and with both driving sides, because stations and lane offsets depend on both.
namespace {
// A curved two-lane Link X and a Link Z whose Connector arrives on X's first lane at station 110:
// one body merge, taken over so it carries a waiting line on X, one on Z's path, an area and a rule.
struct Merge { ProjectDocument d; std::string x, z, connector, section; };
Merge bodyMerge(DrivingSide side) {
    Merge m;
    changeDrivingSide(m.d, side);
    m.x = addLink(m.d, {{0, 0}, {50, 5}, {100, 20}, {150, 50}, {200, 90}}, 2, 3.5);
    const double sign = side == DrivingSide::left ? 1 : -1;
    m.z = addLink(m.d, {{20, sign * 60}, {80, sign * 45}}, 1, 3.5);
    const auto xLane = editableLink(m.d, m.x).lanes[0].id;
    m.connector = addConnector(m.d, {m.z, editableLink(m.d, m.z).lanes[0].id}, {m.x, xLane, 110.0});
    const auto table = runtimeSections(m.d.network);
    for (const auto& g : mergeGroups(m.d.network, table))
        if (g.incoming.size() == 2) m.section = g.section;
    if (m.section.empty()) throw std::runtime_error("fixture has no merge");
    takeOverMerge(m.d, m.section, kDefaults);
    validateDocument(m.d);
    return m;
}
const ConflictSide& linkSide(const ProjectDocument& d) {
    const auto& a = d.network.rightOfWay.conflictAreas.front();
    return a.first.path.connectorId.empty() ? a.first : a.second;
}
const Link& link(const ProjectDocument& d, const std::string& id) {
    for (const auto& l : d.network.links) if (l.id == id) return l;
    throw std::runtime_error("no link");
}
// Clean means: the area resolves, compiles its rule, and only waits for M3.2.3's runtime.
bool clean(const ProjectDocument& d) {
    const auto r = resolve(d);
    return count(r.issues, "UNSUPPORTED_CONFLICT_RUNTIME") == static_cast<int>(r.issues.size()) &&
           rulesWithPrefix(r, "right-of-way/") == static_cast<int>(d.network.rightOfWay.conflictAreas.size());
}
}
TEST(rightofway_lifecycle, deleting_an_owner_cascades_in_one_undoable_step) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        auto m = bodyMerge(side);
        CHECK(clean(m.d)); CHECK(m.d.network.rightOfWay.conflictAreas.size() == 1);
        for (const auto& target : {m.connector, m.x, m.z}) {
            History h; h.reset(m.d);
            CHECK(h.execute("delete", [&](ProjectDocument& d) { deleteObjects(d, {target}); }));
            CHECK(h.document().network.rightOfWay.empty()); // area, rule and both lines together
            h.undo(); CHECK(h.document() == m.d);
            h.redo(); CHECK(h.document().network.rightOfWay.empty());
        }
    }
}
TEST(rightofway_lifecycle, a_drag_that_detaches_the_connector_takes_its_controls) {
    auto m = bodyMerge(DrivingSide::left);
    History h; h.reset(m.d);
    CHECK(h.execute("drag", [&](ProjectDocument& d) { changeGeometry(d, m.x, {{0, 0}, {60, 0}}); }));
    // The forcing: shortening X past station 110 really removed the Connector.
    CHECK(std::none_of(h.document().network.connectors.begin(), h.document().network.connectors.end(),
                       [&](const auto& c) { return c.id == m.connector; }));
    CHECK(h.document().network.rightOfWay.empty());
    h.undo(); CHECK(h.document() == m.d);
}
TEST(rightofway_lifecycle, a_split_carries_controls_to_the_same_place) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        auto m = bodyMerge(side);
        const auto original = link(m.d, m.x).geometry;
        const auto before = linkSide(m.d);
        const auto world = pointAlong(original, before.entryStation);
        CHECK(before.entryStation > 60.1); // the forcing: the control lies downstream of the cut
        const auto downstream = splitLink(m.d, m.x, 60);
        validateDocument(m.d);
        const auto& after = linkSide(m.d);
        CHECK(after.path.linkId == downstream);
        CHECK(after.path.laneId == link(m.d, downstream).lanes[0].id);
        const auto moved = pointAlong(link(m.d, downstream).geometry, after.entryStation);
        test::near(moved.x, world.x, 1e-9); test::near(moved.y, world.y, 1e-9);
        // The Connector's arrival moved downstream too; its lane pair followed it.
        CHECK(clean(m.d));
    }
}
TEST(rightofway_lifecycle, a_split_through_a_control_is_refused_whole) {
    auto m = bodyMerge(DrivingSide::left);
    const auto& s = linkSide(m.d);
    const double cut = (s.entryStation + s.exitStation) / 2 - 0.3;
    CHECK(!(s.exitStation <= cut - 0.1 || s.entryStation >= cut + 0.1)); // the forcing: the span meets it
    CHECK(std::abs(cut - 110) > 0.1); // and it is not refused for the Connector attachment instead
    History h; h.reset(m.d);
    test::throws([&] { h.execute("split", [&](ProjectDocument& d) { splitLink(d, m.x, cut); }); }, "EDIT_SPLIT_CONTROL");
    CHECK(h.document() == m.d);
}
TEST(rightofway_lifecycle, a_copy_takes_controls_only_with_every_owner) {
    auto m = bodyMerge(DrivingSide::right);
    auto both = m.d;
    duplicateObjects(both, {m.x, m.z}, {0, 400});
    validateDocument(both);
    CHECK(both.network.rightOfWay.conflictAreas.size() == 2);
    CHECK(both.network.rightOfWay.waitingLines.size() == 4);
    CHECK(both.network.rightOfWay.priorityRules.size() == 2);
    const auto& copy = both.network.rightOfWay.conflictAreas.back();
    CHECK(copy.id != m.d.network.rightOfWay.conflictAreas.front().id);
    CHECK(clean(both)); // the copy resolves on its own new Links and Connector
    auto one = m.d;
    duplicateObjects(one, {m.x}, {0, 400}); // the Connector is not copied, so neither is the area
    validateDocument(one);
    CHECK(one.network.rightOfWay == m.d.network.rightOfWay);
}
TEST(rightofway_lifecycle, lane_edits_keep_identity_and_never_retarget_by_ordinal) {
    auto m = bodyMerge(DrivingSide::left);
    const auto ids = m.d.network.rightOfWay;
    const auto xLanes = link(m.d, m.x).lanes;
    // Retarget the arrival to X's second lane: the area still names the first, and says so.
    changeConnectorEndpoints(m.d, m.connector, m.d.network.connectors.back().from, {m.x, xLanes[1].id, 110.0});
    validateDocument(m.d);
    CHECK(m.d.network.rightOfWay == ids);
    CHECK(has(resolve(m.d).issues, "CONFLICT_UNRESOLVED_PATH"));
    // Remove the named lane, then add a lane back: a new id is not the lane the control names.
    resizeLinkLanes(m.d, m.x, 1, true);
    resizeLinkLanes(m.d, m.x, 2, true);
    validateDocument(m.d);
    CHECK(link(m.d, m.x).lanes[0].id != xLanes[0].id); // the forcing
    CHECK(m.d.network.rightOfWay == ids);
    CHECK(has(resolve(m.d).issues, "CONFLICT_UNRESOLVED_PATH"));
}
TEST(rightofway_lifecycle, a_shorter_link_reports_a_station_it_cannot_hold) {
    auto m = bodyMerge(DrivingSide::left);
    auto w = addLink(m.d, {{0, -200}, {100, -200}}, 1, 3.5);
    putWaitingLine(m.d, {"", "standalone", {{w, link(m.d, w).lanes[0].id, "", "", ""}, 90}});
    CHECK(clean(m.d));
    changeGeometry(m.d, w, {{0, -200}, {50, -200}});
    validateDocument(m.d);
    CHECK(has(resolve(m.d).issues, "CONFLICT_UNRESOLVED_PATH"));
    CHECK(m.d.network.rightOfWay.waitingLines.back().point.station == 90); // kept, never clamped
}
TEST(rightofway_lifecycle, reversing_a_link_a_control_names_is_refused) {
    ProjectDocument d;
    const auto w = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5);
    auto free = d;
    reverseLink(free, w); // the forcing: with nothing on it, this Link reverses
    putWaitingLine(d, {"", "", {{w, link(d, w).lanes[0].id, "", "", ""}, 40}});
    test::throws([&] { reverseLink(d, w); }, "EDIT_REFERENCED_LINK");
}
