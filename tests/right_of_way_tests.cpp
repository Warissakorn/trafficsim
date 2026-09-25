#include "test.hpp"
#include "../tools/four_leg_network.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/model/network/right_of_way.hpp"
#include "../src/model/network/diagnostics.hpp"
#include "right_of_way_fixture.hpp"
#include <algorithm>
using namespace trafficsim;
using namespace rowfixture;
// M3.2.2: authored right-of-way controls at the file/model seam (docs/M3_ACCEPTANCE.md A01-A08).
// Since M3.2.3b a complete authored merge group runs on its compiled rules (D58).
TEST(rightofway, a01_no_controls_compile_exactly_as_before) {
    const auto d = fixture::fourLegIntersection().document;
    CHECK(d.network.rightOfWay.empty());
    const auto table = runtimeSections(d.network);
    const auto r = resolveRightOfWay(d.network, table, kDefaults);
    CHECK(r.issues.empty());
    CHECK(r.rules == derivedPriorityRules(table, kDefaults));
    CHECK(!documentJson(d)["network"].contains("rightOfWay")); // a project without controls saves as before
}
TEST(rightofway, a02_taken_over_merge_round_trips_with_no_derived_state) {
    auto d = fixture::fourLegIntersection().document;
    const auto section = mergeAt(d, 2);
    const auto areas = takeOverMerge(d, section, kDefaults);
    CHECK(areas.size() == 1);
    validateDocument(d);
    const auto file = documentJson(d);
    CHECK(file["schemaVersion"] == 15);
    const auto back = parseDocument(Json::parse(file.dump()));
    CHECK(back.network.rightOfWay == d.network.rightOfWay);
    CHECK(documentJson(back) == file);
    // Authored values only: no section id, route slot or compiled rule reaches the file.
    const auto text = file["network"]["rightOfWay"].dump();
    CHECK(text.find("/sec-") == std::string::npos); CHECK(text.find("give-way") == std::string::npos);
    CHECK(text.find("\"segment") == std::string::npos);
}
TEST(rightofway, a03_bad_controls_are_refused_and_change_nothing) {
    History h; h.reset(fixture::fourLegIntersection().document);
    const auto before = h.document();
    const auto area = [&] {
        auto d = h.document(); takeOverMerge(d, mergeAt(d, 2), kDefaults); return d.network.rightOfWay;
    }();
    CHECK(area.conflictAreas.size() == 1); // the forcing below starts from a well-formed control
    CHECK(rightOfWayStructuralIssues([&] { auto n = before.network; n.rightOfWay = area; return n; }()).empty());
    // Each case: the structural check names the intended code, and the edit is refused whole.
    const auto refused = [&](auto change, const std::string& code) {
        auto n = before.network; n.rightOfWay = area; change(n.rightOfWay);
        CHECK(has(rightOfWayStructuralIssues(n), code));
        test::throws([&] { h.execute("edit", [&](ProjectDocument& d) { d.network.rightOfWay = area; change(d.network.rightOfWay); }); }, code);
        CHECK(h.document() == before);
    };
    refused([](RightOfWay& r) { r.waitingLines[0].point.station = std::nan(""); }, "INVALID_POSITION");
    refused([](RightOfWay& r) { r.conflictAreas[0].id = r.waitingLines[0].id; }, "DUPLICATE_ID");
    refused([](RightOfWay& r) { r.conflictAreas[0].first.path.connectorId = "no-such-connector"; }, "UNKNOWN_CONTROL_OWNER");
    refused([](RightOfWay& r) { r.conflictAreas[0].first.waitingLineId = "no-such-line"; }, "UNKNOWN_WAITING_LINE");
    refused([](RightOfWay& r) { r.priorityRules[0].conflictAreaId = "no-such-area"; }, "UNKNOWN_CONFLICT_AREA");
    refused([](RightOfWay& r) { r.priorityRules[0].gapTime = 0; }, "INVALID_PRIORITY_RULE");
    refused([](RightOfWay& r) { r.priorityRules.push_back(r.priorityRules[0]); r.priorityRules[1].id = "second-rule"; }, "DUPLICATE_PRIORITY_RULE");
    refused([](RightOfWay& r) { std::swap(r.conflictAreas[0].first.entryStation, r.conflictAreas[0].first.exitStation); }, "INVALID_CONFLICT_EXTENT");
    refused([](RightOfWay& r) { r.conflictAreas[0].second.path.linkId = "x"; }, "INVALID_CONTROL_PATH"); // both kinds set
    // The file side: an unknown key, a bad enum, and the key in a schema that predates it.
    auto d = before; d.network.rightOfWay = area;
    auto file = documentJson(d);
    auto unknown = file; unknown["network"]["rightOfWay"]["conflictAreas"][0]["colour"] = "red";
    test::throws([&] { parseDocument(unknown); }, "");
    auto badEnum = file; badEnum["network"]["rightOfWay"]["conflictAreas"][0]["priority"] = "bothGo";
    test::throws([&] { parseDocument(badEnum); }, "INVALID_ENUM");
    auto older = file; older["schemaVersion"] = 13;
    test::throws([&] { parseDocument(older); }, "");
    auto future = file; future["schemaVersion"] = 16;
    test::throws([&] { parseDocument(future); }, "EDIT_VERSION");
    CHECK(parseDocument(file).network.rightOfWay == area);
}
TEST(rightofway, a04_drafts_save_but_run_is_refused_by_name) {
    auto d = fixture::fourLegIntersection().document;
    takeOverMerge(d, mergeAt(d, 2), kDefaults);
    auto& a = d.network.rightOfWay.conflictAreas[0];
    a.priority = ConflictPriority::undetermined;
    a.second.path.fromLaneId = "no-such-lane"; // a stale lane pair: a draft, not a load error
    validateDocument(d);
    const auto back = parseDocument(Json::parse(documentJson(d).dump()));
    CHECK(back.network.rightOfWay == d.network.rightOfWay);
    const auto r = resolve(d);
    CHECK(has(r.issues, "CONFLICT_UNDETERMINED")); CHECK(has(r.issues, "CONFLICT_UNRESOLVED_PATH"));
    test::throws([&] { compileScenario(d.network, ScenarioDefinition{.priorityDefaults = kDefaults}); }, "");
    const auto rows = runtimeDiagnostics(d.network, ScenarioDefinition{.priorityDefaults = kDefaults});
    CHECK(std::any_of(rows.begin(), rows.end(), [&](const auto& row) {
        return row.code == "CONFLICT_UNDETERMINED" && row.objectId == a.id && row.severity == DiagnosticSeverity::runtime; }));
}
TEST(rightofway, a06_the_effective_order_must_be_total_and_acyclic) {
    auto t = threeWayMerge();
    const auto section = mergeAt(t.d, 3);
    CHECK(resolve(t.d).issues.empty()); // the legacy drawing order is accepted
    const auto areas = takeOverMerge(t.d, section, kDefaults);
    CHECK(areas.size() == 3);
    auto r = resolve(t.d);
    // A valid total order runs (M3.2.3b): no issue, and the group's own three rules.
    CHECK(r.issues.empty());
    CHECK(r.zones.size() == 3); CHECK(rulesWithPrefix(r, "give-way/") == 0);
    // A three-way cycle: areas are (1 yields 0), (2 yields 0), (2 yields 1). Reverse the second.
    auto cycle = t.d;
    cycle.network.rightOfWay.conflictAreas[1].priority = ConflictPriority::secondYields;
    r = resolve(cycle);
    CHECK(count(r.issues, "CONFLICT_PRIORITY_CYCLE") == 3); CHECK(r.zones.empty());
    // A two-way cycle: a second area on one pair, reversed.
    auto twoWay = t.d;
    auto copy = twoWay.network.rightOfWay.conflictAreas[0];
    copy.id = "reverse"; copy.priority = ConflictPriority::secondYields;
    putConflictArea(twoWay, copy); putPriorityRule(twoWay, {"", "", "reverse", 3, 7});
    validateDocument(twoWay);
    r = resolve(twoWay);
    CHECK(has(r.issues, "CONFLICT_PRIORITY_CYCLE")); CHECK(has(r.issues, "CONFLICT_DUPLICATE_PAIR"));
    // A missing pair leaves two paths incomparable: blocked, and nothing is guessed for it.
    auto missing = t.d;
    deleteConflictArea(missing, missing.network.rightOfWay.conflictAreas[2].id);
    r = resolve(missing);
    CHECK(has(r.issues, "CONFLICT_GROUP_INCOMPLETE")); CHECK(r.zones.empty());
    CHECK(rulesWithPrefix(r, "give-way/") == 0);
}
TEST(rightofway, a07_reversing_a_merge_replaces_its_whole_fallback) {
    auto d = fixture::fourLegIntersection().document;
    const auto table = runtimeSections(d.network);
    const auto derived = derivedPriorityRules(table, kDefaults);
    const auto section = mergeAt(d, 2);
    takeOverMerge(d, section, kDefaults);
    auto r = resolve(d);
    CHECK(rulesWithPrefix(r, "give-way/") == static_cast<int>(derived.size()) - 1);
    CHECK(r.zones.size() == 1);
    const auto taken = r.zones.front();
    // Until the author changes it, the take-over carries the fallback's order and numbers; since
    // M3.2.3c it runs on the admission solver instead of the rule (D59).
    const auto fallback = std::find_if(derived.begin(), derived.end(), [&](const auto& x) {
        return x.yieldSegmentId == taken.minor.segmentIds.front() && x.conflictSegmentId == taken.major.segmentIds.back(); });
    CHECK(fallback != derived.end());
    test::near(taken.waitPosition, fallback->yieldPosition, 1e-9);
    test::near(taken.major.exit, fallback->conflictPosition, 1e-9);
    const auto pair = [&](const PriorityRule& x) {
        return (x.yieldSegmentId == fallback->yieldSegmentId && x.conflictSegmentId == fallback->conflictSegmentId) ||
               (x.yieldSegmentId == fallback->conflictSegmentId && x.conflictSegmentId == fallback->yieldSegmentId); };
    CHECK(std::count_if(r.rules.begin(), r.rules.end(), pair) == 0); // no fallback rule beside it
    // Reverse it: exactly one zone for the pair, pointing the other way; no hidden reciprocal.
    d.network.rightOfWay.conflictAreas[0].priority = ConflictPriority::secondYields;
    r = resolve(d);
    CHECK(r.zones.size() == 1); CHECK(std::count_if(r.rules.begin(), r.rules.end(), pair) == 0);
    CHECK(r.zones.front().minor.segmentIds.front() == fallback->conflictSegmentId);
    CHECK(r.zones.front().major.segmentIds.back() == fallback->yieldSegmentId);
    // Deleting the rule blocks the group; it does not hand it back to the fallback.
    deletePriorityRule(d, d.network.rightOfWay.priorityRules[0].id);
    r = resolve(d);
    CHECK(has(r.issues, "CONFLICT_RULE_MISSING")); CHECK(std::count_if(r.rules.begin(), r.rules.end(), pair) == 0);
    CHECK(r.zones.empty());
    // Handing it back is its own action, and restores the fallback exactly.
    restoreAutomaticPriority(d, section);
    CHECK(d.network.rightOfWay.empty());
    CHECK(resolve(d).rules == derived);
}
TEST(rightofway, a08_an_explicit_group_is_portable_without_defaults) {
    auto t = threeWayMerge();
    const PriorityDefaults missing{0, 0};
    // The forcing: with no defaults the automatic merge is reported.
    CHECK(!priorityDefaultsIssues(t.d.network, missing).empty());
    takeOverMerge(t.d, mergeAt(t.d, 3), kDefaults);
    CHECK(priorityDefaultsIssues(t.d.network, missing).empty());
    // A second, automatic merge beside it still needs the catalog.
    auto both = fixture::fourLegIntersection().document;
    CHECK(!priorityDefaultsIssues(both.network, missing).empty());
    takeOverMerge(both, mergeAt(both, 2), kDefaults);
    CHECK(!priorityDefaultsIssues(both.network, missing).empty());
}
TEST(rightofway, commands_are_one_undoable_step) {
    History h; h.reset(threeWayMerge().d);
    const auto before = h.document();
    const auto section = mergeAt(before, 3);
    CHECK(h.execute("takeOver", [&](ProjectDocument& d) { takeOverMerge(d, section, kDefaults); }));
    const auto after = h.document();
    CHECK(after.network.rightOfWay.conflictAreas.size() == 3);
    test::throws([&] { h.execute("again", [&](ProjectDocument& d) { takeOverMerge(d, section, kDefaults); }); }, "EDIT_ALREADY_EXPLICIT");
    test::throws([&] { h.execute("line", [&](ProjectDocument& d) {
        deleteWaitingLine(d, d.network.rightOfWay.waitingLines[0].id); }); }, "EDIT_REFERENCED");
    CHECK(h.document() == after);
    h.undo(); CHECK(h.document() == before);
    h.redo(); CHECK(h.document() == after);
}
TEST(rightofway, a_taken_over_lane_two_of_a_curved_range_compiles_to_the_fallback) {
    // Scrutiny finding 1 (D55): the waiting line was placed on the Connector's stored polyline,
    // which is exact only for the path that IS that polyline.
    ProjectDocument d;
    const auto x = addLink(d, {{0, 0}, {100, 0}}, 2, 3.5);
    const auto p = addLink(d, {{-100, 60}, {-20, 40}}, 1, 3.5);
    const auto r = addLink(d, {{-100, -60}, {-20, -40}}, 2, 3.5);
    const auto lane = [&](const std::string& link, int k) { return editableLink(d, link).lanes[k].id; };
    addConnector(d, {p, lane(p, 0)}, {x, lane(x, 1)});                    // drawn first: priority
    const auto range = addConnectorRange(d, {r, lane(r, 0)}, {x, lane(x, 0)}, 2, 2);
    const auto table = runtimeSections(d.network);
    const auto derived = derivedPriorityRules(table, kDefaults);
    const auto yielding = connectorPathId(d.network.connectors.back(), 1);
    CHECK(d.network.connectors.back().id == range);
    const auto fallback = std::find_if(derived.begin(), derived.end(), [&](const auto& x) { return x.yieldSegmentId == yielding; });
    CHECK(fallback != derived.end());
    // The forcing: on this path the old base-polyline placement really is off.
    const auto& base = d.network.connectors.back().geometry;
    const auto path = std::find_if(table.paths.begin(), table.paths.end(), [&](const auto& x) { return x.id == yielding; });
    const double old = matchedStation(base, path->geometry, polylineLength(base) - 1);
    CHECK(std::abs(old - fallback->yieldPosition) > 1e-6);
    // The consequence: the take-over now compiles to the fallback's rule.
    std::string section;
    for (const auto& g : mergeGroups(d.network, table))
        if (std::find(g.incoming.begin(), g.incoming.end(), yielding) != g.incoming.end()) section = g.section;
    takeOverMerge(d, section, kDefaults);
    const auto res = resolve(d);
    const auto* taken = zoneYielding(res, yielding);
    CHECK(taken != nullptr);
    test::near(taken->waitPosition, fallback->yieldPosition, 1e-9);
    test::near(taken->major.exit, fallback->conflictPosition, 1e-9);
}
TEST(rightofway, the_resolver_never_throws_on_a_reference_it_cannot_locate) {
    // Scrutiny finding 2: buildScenario must not throw, and locate calls geometry that can.
    auto d = fixture::fourLegIntersection().document;
    takeOverMerge(d, mergeAt(d, 2), kDefaults);
    const RuntimeSections empty;
    const auto& somePath = d.network.links.front();
    // The forcing: this table really does make the geometry helper throw.
    test::throws([&] { sectionForStation(empty, somePath.lanes.front().id, 0); }, "UNKNOWN_LANE");
    auto probe = d; // a side on a Link lane is what reaches sectionForStation
    probe.network.rightOfWay.conflictAreas.front().first.path = {somePath.id, somePath.lanes.front().id, "", "", ""};
    probe.network.rightOfWay.conflictAreas.front().first.entryStation = 1;
    probe.network.rightOfWay.conflictAreas.front().first.exitStation = 2;
    const auto r = resolveRightOfWay(probe.network, empty, kDefaults); // must not throw
    CHECK(has(r.issues, "CONFLICT_UNRESOLVED_PATH"));
}
TEST(rightofway, a_waiting_line_past_its_path_end_is_reported_not_clamped) {
    auto d = fixture::fourLegIntersection().document;
    takeOverMerge(d, mergeAt(d, 2), kDefaults);
    CHECK(!has(resolve(d).issues, "CONFLICT_UNRESOLVED_PATH"));
    d.network.rightOfWay.waitingLines.front().point.station = 1e4;
    validateDocument(d);
    CHECK(has(resolve(d).issues, "CONFLICT_UNRESOLVED_PATH"));
    CHECK(d.network.rightOfWay.waitingLines.front().point.station == 1e4);
}
TEST(rightofway, a_short_upstream_section_is_taken_over_into_its_own_group) {
    // Scrutiny finding 4: a section shorter than 1 m put the entry into the previous section.
    ProjectDocument d;
    const auto x = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5);
    const auto y = addLink(d, {{60, 30}, {100, 30}}, 1, 3.5);
    const auto z = addLink(d, {{0, 40}, {40, 20}}, 1, 3.5);
    const auto lane = [&](const std::string& link) { return editableLink(d, link).lanes[0].id; };
    addConnector(d, {x, lane(x), 50.0}, {y, lane(y)});
    addConnector(d, {z, lane(z)}, {x, lane(x), 50.6});
    const auto table = runtimeSections(d.network);
    std::string section;
    for (const auto& g : mergeGroups(d.network, table))
        if (g.incoming.size() == 2) section = g.section;
    CHECK(!section.empty());
    const auto upstream = std::find_if(table.sections.begin(), table.sections.end(),
        [&](const auto& s) { return s.laneId == lane(x) && s.end > 50 && s.end < 51; });
    CHECK(upstream != table.sections.end()); CHECK(upstream->end - upstream->start < 1); // the forcing
    takeOverMerge(d, section, kDefaults);
    const auto r = resolve(d);
    CHECK(!has(r.issues, "CONFLICT_MERGE_TOPOLOGY")); CHECK(!has(r.issues, "CONFLICT_UNRESOLVED_PATH"));
    CHECK(r.zones.size() == 1);
}
