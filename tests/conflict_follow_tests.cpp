#include "test.hpp"
#include "../tools/t_junction_network.hpp"
#include "../src/commands/history.hpp"
#include "../src/model/network/right_of_way.hpp"
#include <algorithm>
using namespace trafficsim;
// D86: an authored conflict area follows the overlap it was set on when a Link or Connector is
// edited, and falls away with its rule, control and line when the overlap does (Vissim's rule).
namespace {
const ConflictArea& areaOf(const ProjectDocument& d, const std::string& id) {
    for (const auto& a : d.network.rightOfWay.conflictAreas) if (a.id == id) return a;
    throw std::runtime_error("no area " + id);
}
bool hasArea(const ProjectDocument& d, const std::string& id) {
    const auto& areas = d.network.rightOfWay.conflictAreas;
    return std::any_of(areas.begin(), areas.end(), [&](const auto& a) { return a.id == id; });
}
double lineStation(const ProjectDocument& d, const std::string& id) {
    for (const auto& w : d.network.rightOfWay.waitingLines) if (w.id == id) return w.point.station;
    throw std::runtime_error("no line " + id);
}
const Link& linkOf(const ProjectDocument& d, const std::string& id) {
    for (const auto& l : d.network.links) if (l.id == id) return l;
    throw std::runtime_error("no link " + id);
}
std::vector<Point> shifted(const std::vector<Point>& g, double dx, double dy) {
    auto r = g;
    for (auto& p : r) { p.x += dx; p.y += dy; }
    return r;
}
// The single overlap of the area's pair on the current drawing.
SurfaceOverlap overlapOf(const ProjectDocument& d, const ConflictArea& a) {
    const auto pieces = surfaceOverlaps(d.network, a.first.path, a.second.path);
    if (pieces.size() != 1 || pieces.front().status != SurfaceOverlap::Status::overlap) throw std::runtime_error("not one overlap");
    return pieces.front();
}
std::vector<std::string> issuesOf(const ProjectDocument& d) {
    std::vector<std::string> codes;
    for (const auto& i : resolveRightOfWay(d.network, runtimeSections(d.network), {5, 7}).issues) codes.push_back(i.code + "@" + i.path);
    return codes;
}
void followsItsOverlap(const ProjectDocument& d, const std::string& id) {
    const auto& a = areaOf(d, id);
    const auto o = overlapOf(d, a);
    test::near(a.first.entryStation, o.first.from, 1e-9); test::near(a.first.exitStation, o.first.to, 1e-9);
    test::near(a.second.entryStation, o.second.from, 1e-9); test::near(a.second.exitStation, o.second.to, 1e-9);
    const auto issues = issuesOf(d);
    if (!issues.empty()) throw std::runtime_error("resolver: " + issues.front());
}
}
TEST(conflict_follow, a_crossing_follows_a_link_moved_along_itself) {
    auto t = fixture::tJunction();
    History h; h.reset(t.document);
    const auto before = areaOf(h.document(), t.crossingArea);
    const auto geometry = shifted(linkOf(h.document(), t.eastbound).geometry, 5, 0);
    CHECK(h.execute("move", [&](auto& d) { changeGeometry(d, t.eastbound, geometry); }));
    CHECK(linkOf(h.document(), t.eastbound).geometry == geometry); // the forcing: the Link moved
    followsItsOverlap(h.document(), t.crossingArea);
    const auto& after = areaOf(h.document(), t.crossingArea);
    const auto& onLink = after.first.path.connectorId.empty() ? after.first : after.second;
    const auto& was = before.first.path.connectorId.empty() ? before.first : before.second;
    test::near(onLink.entryStation, was.entryStation - 5, 1e-6); // the reference now starts 5 m later
    // The line keeps its distance to the area it stands before.
    test::near(onLink.entryStation - lineStation(h.document(), onLink.waitingLineId),
               was.entryStation - lineStation(t.document, was.waitingLineId), 1e-6);
    CHECK(after.priority == before.priority && after.name == before.name);
}
TEST(conflict_follow, a_crossing_follows_a_link_moved_across_the_turn) {
    auto t = fixture::tJunction();
    History h; h.reset(t.document);
    const auto before = areaOf(h.document(), t.crossingArea);
    const auto geometry = shifted(linkOf(h.document(), t.eastbound).geometry, 0, -1.5);
    CHECK(h.execute("move", [&](auto& d) { changeGeometry(d, t.eastbound, geometry); }));
    CHECK(linkOf(h.document(), t.eastbound).geometry == geometry);
    followsItsOverlap(h.document(), t.crossingArea);
    const auto& after = areaOf(h.document(), t.crossingArea);
    const auto& onTurn = after.first.path.connectorId.empty() ? after.second : after.first;
    const auto& was = before.first.path.connectorId.empty() ? before.second : before.first;
    CHECK(std::abs(onTurn.entryStation - was.entryStation) > 0.1); // it moved along the turn
    const auto automatic = automaticConflicts(h.document().network);
    CHECK(std::none_of(automatic.begin(), automatic.end(), [&](const auto& x) { return x.kind == ConflictKind::crossing; }));
}
TEST(conflict_follow, an_area_whose_overlap_is_gone_leaves_with_its_rule_control_and_line) {
    auto t = fixture::tJunction();
    History h; h.reset(t.document);
    const auto a = areaOf(h.document(), t.crossingArea);
    const auto& row0 = h.document().network.rightOfWay;
    CHECK(std::any_of(row0.stopControls.begin(), row0.stopControls.end(), [&](const auto& c) {
        return std::find(c.conflictAreaIds.begin(), c.conflictAreaIds.end(), a.id) != c.conflictAreaIds.end(); }));
    // Eastbound moved 30 m north of the turn's crossing: it no longer meets the crossing turn.
    const auto geometry = shifted(linkOf(h.document(), t.eastbound).geometry, 0, -30);
    CHECK(h.execute("move", [&](auto& d) { changeGeometry(d, t.eastbound, geometry); }));
    CHECK(surfaceOverlaps(h.document().network, a.first.path, a.second.path).front().status == SurfaceOverlap::Status::none);
    const auto& row = h.document().network.rightOfWay;
    CHECK(!hasArea(h.document(), a.id));
    CHECK(std::none_of(row.priorityRules.begin(), row.priorityRules.end(), [&](const auto& r) { return r.conflictAreaId == a.id; }));
    for (const auto& c : row.stopControls) CHECK(std::find(c.conflictAreaIds.begin(), c.conflictAreaIds.end(), a.id) == c.conflictAreaIds.end());
    for (const auto* s : {&a.first, &a.second}) {
        const bool used = std::any_of(row.conflictAreas.begin(), row.conflictAreas.end(), [&](const auto& x) {
            return x.first.waitingLineId == s->waitingLineId || x.second.waitingLineId == s->waitingLineId; });
        const bool kept = std::any_of(row.waitingLines.begin(), row.waitingLines.end(), [&](const auto& w) { return w.id == s->waitingLineId; });
        CHECK(used == kept);
    }
    h.undo();
    CHECK(h.document() == t.document);
}
TEST(conflict_follow, a_taken_over_merge_follows_its_join) {
    auto t = fixture::tJunction();
    History h; h.reset(t.document);
    CHECK(issuesOf(h.document()).empty());
    // Westbound 4 m longer upstream: every station on it grows by 4, the join included.
    auto geometry = linkOf(h.document(), t.westbound).geometry;
    geometry.front().x += 4;
    CHECK(h.execute("lengthen", [&](auto& d) { changeGeometry(d, t.westbound, geometry); }));
    CHECK(linkOf(h.document(), t.westbound).geometry == geometry);
    const auto issues = issuesOf(h.document());
    if (!issues.empty()) throw std::runtime_error("resolver: " + issues.front());
    CHECK(hasArea(h.document(), t.crossingMerge) && hasArea(h.document(), t.nearMerge));
}
TEST(conflict_follow, a_second_add_crossing_does_not_double_the_area) {
    auto t = fixture::tJunction();
    const auto count = t.document.network.rightOfWay.conflictAreas.size();
    test::throws([&] { addCrossingAreas(t.document, t.crossingTurn, t.eastbound, t.crossingTurn, {5, 7}); }, "EDIT_CROSSING_EXISTS");
    CHECK(t.document.network.rightOfWay.conflictAreas.size() == count);
}
TEST(conflict_follow, an_edit_that_leaves_the_drawing_alone_leaves_the_areas_alone) {
    auto t = fixture::tJunction();
    History h; h.reset(t.document);
    CHECK(h.execute("rename", [&](auto& d) { editableLink(d, t.eastbound).name = "Renamed"; }));
    CHECK(h.document().network.rightOfWay == t.document.network.rightOfWay);
}
