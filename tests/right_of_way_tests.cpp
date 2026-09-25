#include "test.hpp"
#include "../tools/four_leg_network.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/model/network/right_of_way.hpp"
#include "../src/model/network/diagnostics.hpp"
#include <algorithm>
using namespace trafficsim;
// M3.2.2: authored right-of-way controls at the file/model seam (docs/M3_ACCEPTANCE.md A01-A08).
// Nothing here runs a new control: every authored area is Run-blocked until M3.2.3.
namespace {
const PriorityDefaults kDefaults{3, 7};
bool has(const std::vector<ValidationIssue>& issues, const std::string& code) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code; });
}
int count(const std::vector<ValidationIssue>& issues, const std::string& code) {
    return static_cast<int>(std::count_if(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code; }));
}
RightOfWayResolution resolve(const ProjectDocument& d, PriorityDefaults defaults = kDefaults) {
    return resolveRightOfWay(d.network, runtimeSections(d.network), defaults);
}
int rulesWithPrefix(const RightOfWayResolution& r, const std::string& prefix) {
    return static_cast<int>(std::count_if(r.rules.begin(), r.rules.end(),
                                          [&](const auto& x) { return x.id.rfind(prefix, 0) == 0; }));
}
// Three single-lane Links ending short of X, each joined to X's lane start: one merge of three.
struct Three { ProjectDocument d; std::string x; };
Three threeWayMerge() {
    Three t;
    t.x = addLink(t.d, {{0, 0}, {100, 0}}, 1, 3.5);
    const auto xLane = editableLink(t.d, t.x).lanes[0].id;
    for (const double y : {0.0, 40.0, -40.0}) {
        const auto a = addLink(t.d, {{-100, y}, {-20, y * 0.25}}, 1, 3.5);
        addConnector(t.d, {a, editableLink(t.d, a).lanes[0].id}, {t.x, xLane});
    }
    return t;
}
std::string mergeAt(const ProjectDocument& d, std::size_t members) {
    for (const auto& g : mergeGroups(d.network, runtimeSections(d.network)))
        if (g.incoming.size() == members) return g.section;
    throw std::runtime_error("no merge of that size");
}
}
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
    CHECK(file["schemaVersion"] == 14);
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
    auto future = file; future["schemaVersion"] = 15;
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
    CHECK(has(r.issues, "UNSUPPORTED_CONFLICT_RUNTIME"));
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
    // A valid total order: only the not-yet-runnable marker, and the group's own three rules.
    CHECK(count(r.issues, "UNSUPPORTED_CONFLICT_RUNTIME") == 3); CHECK(r.issues.size() == 3);
    CHECK(rulesWithPrefix(r, "right-of-way/") == 3); CHECK(rulesWithPrefix(r, "give-way/") == 0);
    // A three-way cycle: areas are (1 yields 0), (2 yields 0), (2 yields 1). Reverse the second.
    auto cycle = t.d;
    cycle.network.rightOfWay.conflictAreas[1].priority = ConflictPriority::secondYields;
    r = resolve(cycle);
    CHECK(count(r.issues, "CONFLICT_PRIORITY_CYCLE") == 3); CHECK(rulesWithPrefix(r, "right-of-way/") == 0);
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
    CHECK(has(r.issues, "CONFLICT_GROUP_INCOMPLETE")); CHECK(rulesWithPrefix(r, "right-of-way/") == 0);
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
    const auto taken = std::find_if(r.rules.begin(), r.rules.end(), [](const auto& x) { return x.id.rfind("right-of-way/", 0) == 0; });
    CHECK(taken != r.rules.end());
    // Until the author changes it, the take-over compiles to what the fallback ran.
    const auto fallback = std::find_if(derived.begin(), derived.end(), [&](const auto& x) {
        return x.yieldSegmentId == taken->yieldSegmentId && x.conflictSegmentId == taken->conflictSegmentId; });
    CHECK(fallback != derived.end());
    test::near(taken->yieldPosition, fallback->yieldPosition, 1e-9);
    test::near(taken->conflictPosition, fallback->conflictPosition, 1e-9);
    // Reverse it: exactly one rule for the pair, pointing the other way; no hidden reciprocal.
    d.network.rightOfWay.conflictAreas[0].priority = ConflictPriority::secondYields;
    r = resolve(d);
    const auto pair = [&](const PriorityRule& x) {
        return (x.yieldSegmentId == fallback->yieldSegmentId && x.conflictSegmentId == fallback->conflictSegmentId) ||
               (x.yieldSegmentId == fallback->conflictSegmentId && x.conflictSegmentId == fallback->yieldSegmentId); };
    CHECK(std::count_if(r.rules.begin(), r.rules.end(), pair) == 1);
    CHECK(std::any_of(r.rules.begin(), r.rules.end(), [&](const auto& x) {
        return x.yieldSegmentId == fallback->conflictSegmentId && x.conflictSegmentId == fallback->yieldSegmentId; }));
    // Deleting the rule blocks the group; it does not hand it back to the fallback.
    deletePriorityRule(d, d.network.rightOfWay.priorityRules[0].id);
    r = resolve(d);
    CHECK(has(r.issues, "CONFLICT_RULE_MISSING")); CHECK(std::count_if(r.rules.begin(), r.rules.end(), pair) == 0);
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
TEST(rightofway, until_m3_2_2b_owner_deletes_are_refused_and_lane_changes_leave_a_draft) {
    // Pins the interim behaviour D54 documents, so M3.2.2b changes it on purpose, not by accident.
    History h; h.reset(threeWayMerge().d);
    const auto section = mergeAt(h.document(), 3);
    h.execute("takeOver", [&](ProjectDocument& d) { takeOverMerge(d, section, kDefaults); });
    const auto& area = h.document().network.rightOfWay.conflictAreas.front();
    const auto connector = area.first.path.connectorId;
    CHECK(!connector.empty()); // the forcing: the area really names a Connector
    const auto before = h.document();
    test::throws([&] { h.execute("delete", [&](ProjectDocument& d) { deleteObjects(d, {connector}); }); }, "UNKNOWN_CONTROL_OWNER");
    CHECK(h.document() == before);
    // A lane id the pair no longer matches is kept, reported, and blocks Run -- never retargeted.
    auto stale = before; stale.network.rightOfWay.conflictAreas.front().first.path.toLaneId = "renamed";
    validateDocument(stale);
    CHECK(has(resolve(stale).issues, "CONFLICT_UNRESOLVED_PATH"));
}
