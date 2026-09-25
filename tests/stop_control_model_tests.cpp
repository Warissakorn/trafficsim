#include "test.hpp"
#include "right_of_way_fixture.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/project/run.hpp"
using namespace trafficsim;
using namespace rowfixture;
// M3.2.5a: the Stop/Yield control at the file/model seam -- the gesture, schema 15, validation and
// the lifecycle that keeps a control from outliving its line or its areas.
namespace {
struct Pair { ProjectDocument d; std::string a, b, area; };
Pair crossingLinks() {
    Pair p;
    p.a = addLink(p.d, {{0, 0}, {100, 0}}, 1, 3.5);
    p.b = addLink(p.d, {{50, -60}, {50, 60}}, 1, 3.5);
    p.area = addCrossingAreas(p.d, p.a, p.b, p.b, kDefaults).front(); // B gives way
    return p;
}
const StopControl* controlOf(const ProjectDocument& d, const std::string& area) {
    for (const auto& c : d.network.rightOfWay.stopControls)
        if (std::find(c.conflictAreaIds.begin(), c.conflictAreaIds.end(), area) != c.conflictAreaIds.end()) return &c;
    return nullptr;
}
}
TEST(stop_control_model, the_gesture_puts_the_control_on_the_line_the_area_gives_way_at) {
    auto p = crossingLinks();
    History h; h.reset(p.d);
    CHECK(h.execute("stop", [&](ProjectDocument& d) { setAreaControl(d, p.area, StopMode::stop); }));
    const auto* c = controlOf(h.document(), p.area);
    CHECK(c && c->mode == StopMode::stop);
    CHECK(c->waitingLineId == h.document().network.rightOfWay.conflictAreas.front().second.waitingLineId);
    CHECK(resolve(h.document()).zones.front().control == ZoneControl::stop);
    CHECK(h.execute("yield", [&](ProjectDocument& d) { setAreaControl(d, p.area, StopMode::yield); }));
    CHECK(h.document().network.rightOfWay.stopControls.size() == 1 && controlOf(h.document(), p.area)->mode == StopMode::yield);
    CHECK(resolve(h.document()).zones.front().control == ZoneControl::yield);
    CHECK(h.execute("none", [&](ProjectDocument& d) { setAreaControl(d, p.area, std::nullopt); }));
    CHECK(h.document().network.rightOfWay.stopControls.empty());
    h.undo(); h.undo(); CHECK(controlOf(h.document(), p.area)->mode == StopMode::stop); // one step each
    // No side gives way yet: nothing to stop at.
    auto undecided = p.d; setConflictControl(undecided, p.area, "", ConflictPriority::undetermined, 3, 7);
    test::throws([&] { setAreaControl(undecided, p.area, StopMode::stop); }, "EDIT_UNDETERMINED_PRIORITY");
    // Turning the priority round moves the wait to the other line, so the control is cleared.
    auto turned = h.document();
    setConflictControl(turned, p.area, "", ConflictPriority::firstYields, 3, 7);
    CHECK(turned.network.rightOfWay.stopControls.empty());
}
TEST(stop_control_model, one_line_has_one_mode_for_every_area_it_controls) {
    auto t = threeWayMerge();
    takeOverMerge(t.d, mergeAt(t.d, 3), kDefaults);
    const auto& areas = t.d.network.rightOfWay.conflictAreas;
    // The forcing: two areas give way at the same line (the last incoming path yields to both).
    std::vector<std::string> sharing;
    const auto lineOf = [](const ConflictArea& a) { return (a.priority == ConflictPriority::firstYields ? a.first : a.second).waitingLineId; };
    for (const auto& a : areas) for (const auto& b : areas) if (a.id < b.id && lineOf(a) == lineOf(b)) sharing = {a.id, b.id};
    CHECK(sharing.size() == 2);
    setAreaControl(t.d, sharing[0], StopMode::stop);
    setAreaControl(t.d, sharing[1], StopMode::yield);
    CHECK(t.d.network.rightOfWay.stopControls.size() == 1);
    CHECK(controlOf(t.d, sharing[0]) == controlOf(t.d, sharing[1]) && controlOf(t.d, sharing[0])->mode == StopMode::yield);
    validateDocument(t.d);
    // Turning one area round takes only that area off the line's control.
    const auto& a = *std::find_if(areas.begin(), areas.end(), [&](const auto& x) { return x.id == sharing[0]; });
    setConflictControl(t.d, sharing[0], "", a.priority == ConflictPriority::firstYields ? ConflictPriority::secondYields
                                                                                          : ConflictPriority::firstYields, 3, 7);
    CHECK(!controlOf(t.d, sharing[0]) && controlOf(t.d, sharing[1]));
}
TEST(stop_control_model, schema_15_round_trips_and_an_older_or_malformed_file_is_refused) {
    auto p = crossingLinks();
    setAreaControl(p.d, p.area, StopMode::stop);
    auto file = documentJson(p.d);
    CHECK(file["schemaVersion"] == 15);
    CHECK(file["network"]["rightOfWay"]["stopControls"][0]["mode"] == "stop");
    CHECK(parseDocument(file).network == p.d.network);
    auto older = file; older["schemaVersion"] = 14;
    test::throws([&] { parseDocument(older); }, "");
    auto badMode = file; badMode["network"]["rightOfWay"]["stopControls"][0]["mode"] = "halt";
    test::throws([&] { parseDocument(badMode); }, "INVALID_ENUM");
    // Without a control the file keeps exactly the keys it had before.
    auto none = crossingLinks();
    CHECK(!documentJson(none.d)["network"]["rightOfWay"].contains("stopControls"));
}
TEST(stop_control_model, structural_and_runtime_checks_name_the_control) {
    auto p = crossingLinks();
    setAreaControl(p.d, p.area, StopMode::stop);
    const auto& row = p.d.network.rightOfWay;
    const auto& area = row.conflictAreas.front();
    auto twice = p.d; twice.network.rightOfWay.stopControls.push_back({"stop-x", "", area.second.waitingLineId, StopMode::yield, {}});
    const auto twiceIssues = rightOfWayStructuralIssues(twice.network);
    CHECK(has(twiceIssues, "DUPLICATE_STOP_CONTROL")); CHECK(has(twiceIssues, "INVALID_STOP_CONTROL"));
    auto unknown = p.d; unknown.network.rightOfWay.stopControls.front().conflictAreaIds.push_back("nothing");
    CHECK(has(rightOfWayStructuralIssues(unknown.network), "UNKNOWN_CONFLICT_AREA"));
    CHECK(rightOfWayStructuralIssues(p.d.network).empty());
    // The line the major side waits at is not where this area gives way: Run refuses it.
    auto wrong = p.d; wrong.network.rightOfWay.stopControls.front().waitingLineId = area.first.waitingLineId;
    CHECK(rightOfWayStructuralIssues(wrong.network).empty());
    CHECK(has(resolve(wrong).issues, "CONFLICT_STOP_LINE_MISMATCH"));
    CHECK(resolve(p.d).issues.empty());
}
TEST(stop_control_model, a_control_goes_with_its_areas_and_is_copied_with_them) {
    auto p = crossingLinks();
    setAreaControl(p.d, p.area, StopMode::stop);
    const auto line = controlOf(p.d, p.area)->waitingLineId;
    test::throws([&] { auto d = p.d; deleteWaitingLine(d, line); }, "EDIT_REFERENCED");
    auto removed = p.d; removeConflictArea(removed, p.area);
    CHECK(removed.network.rightOfWay.stopControls.empty());
    auto roadGone = p.d; deleteLink(roadGone, p.b);
    CHECK(roadGone.network.rightOfWay.empty()); validateDocument(roadGone);
    // Copying both roads copies the area and its Stop, onto the copy's own line.
    auto copied = p.d;
    duplicateObjects(copied, {p.a, p.b}, {0, 200});
    CHECK(copied.network.rightOfWay.stopControls.size() == 2);
    const auto& copy = copied.network.rightOfWay.stopControls.back();
    CHECK(copy.mode == StopMode::stop && copy.waitingLineId != line);
    CHECK(controlOf(copied, copy.conflictAreaIds.front()) == &copy);
    validateDocument(copied);
    CHECK(resolve(copied).issues.empty());
}
TEST(stop_control_model, a_stop_on_a_two_lane_crossing_halts_every_minor_vehicle_before_the_first_area) {
    // D63: the minor lane's one line stands before the FIRST area it meets, so a Stop there never
    // halts a vehicle inside the near lane's area.
    ProjectDocument d;
    const auto major = addLink(d, {{0, 0}, {200, 0}}, 2, 3.5);
    const auto minor = addLink(d, {{100, -100}, {100, 100}}, 1, 3.5);
    const auto areas = addCrossingAreas(d, major, minor, minor, kDefaults);
    CHECK(areas.size() == 2);
    setAreaControl(d, areas.front(), StopMode::stop);
    CHECK(d.network.rightOfWay.stopControls.size() == 1 && d.network.rightOfWay.stopControls.front().conflictAreaIds.size() == 2);
    putInput(d, {"", putRoute(d, {"", {major}}), "car", 900, 0, 540, {}});
    putInput(d, {"", putRoute(d, {"", {minor}}), "car", 300, 0, 540, {}});
    changeRunSettings(d, 600, 0.1);
    validateDocument(d);
    const auto s = compileDocument(d, test::root() / "data").scenario;
    CHECK(s.conflictZones.size() == 2);
    for (const auto& z : s.conflictZones) CHECK(z.control == ZoneControl::stop);
    const auto& near = *std::min_element(s.conflictZones.begin(), s.conflictZones.end(),
                                         [](const auto& a, const auto& b) { return a.minor.entry < b.minor.entry; });
    CHECK(near.waitPosition < near.minor.entry); // the forcing: the shared line is short of both areas
    std::map<std::uint64_t, bool> stood, crossed;
    auto state = createSimulation(s, 42);
    while (state.tick < totalTicks(s)) {
        for (const auto& v : state.vehicles) {
            // Slots index the canonical scenario the state holds.
            if (state.scenario->routes[v.routeIndex].segmentIds.front() != near.minor.segmentIds.front()) continue;
            if (v.distance > near.waitPosition + 1e-9) { crossed[v.id] = true; continue; }
            if (v.speed == 0 && near.waitPosition - v.distance <= stopLineReach(s.behaviours.front(), v.driverFactor)) stood[v.id] = true;
        }
        state = stepSimulation(state);
    }
    CHECK(crossed.size() > 20);
    for (const auto& [id, went] : crossed) CHECK(stood[id]);
}
