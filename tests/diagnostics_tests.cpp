#include "test.hpp"
#include "../src/project/diagnostics.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include <fstream>
#include <set>
using namespace trafficsim;
namespace {
// Two approaches feeding one exit lane. Valid authoring, but the M0 engine cannot run the merge.
ProjectDocument merging() {
    ProjectDocument d;
    d.network.links = {
        {"west", {{-80,0},{-10,0}}, {{"west-1",3.5}}},
        {"south", {{0,-80},{0,-10}}, {{"south-1",3.5}}},
        {"exit", {{20,20},{20,100}}, {{"exit-1",3.5}}}};
    d.nextId = 9;
    addConnector(d, {"west","west-1"}, {"exit","exit-1"});
    addConnector(d, {"south","south-1"}, {"exit","exit-1"});
    return d;
}
ProjectDocument crossing() {
    std::ifstream file(test::root()/"data/scenarios/crossing.json"); Json j; file >> j; return parseDocument(j);
}
const Diagnostic* find(const std::vector<Diagnostic>& rows, const std::string& code) {
    for (const auto& row : rows) if (row.code == code) return &row;
    return nullptr;
}
}
TEST(diagnostics, paths_resolve_to_object_ids) {
    const auto d = merging();
    const auto& n = d.network;
    CHECK(objectIdForPath(n, "links[0]") == "west");
    CHECK(objectIdForPath(n, "links[0].geometry") == "west");
    CHECK(objectIdForPath(n, "links[2].lanes[0].width") == "exit-1");
    CHECK(objectIdForPath(n, "connectors[1].from") == d.network.connectors[1].id);
    // Network-wide and malformed paths must name nothing rather than guess.
    CHECK(objectIdForPath(n, "drivingSide").empty());
    CHECK(objectIdForPath(n, "id").empty());
    CHECK(objectIdForPath(n, "links[99].geometry").empty());
    CHECK(objectIdForPath(n, "links[2].lanes[7].width").empty());
    CHECK(objectIdForPath(n, "links[abc]").empty());
    CHECK(objectIdForPath(n, "links[]").empty());
    CHECK(objectIdForPath(n, "links[3").empty());
    // A lane is reached through its link; a connector is its own selectable object.
    CHECK(selectableFor(n, "exit-1") == "exit");
    CHECK(selectableFor(n, "west") == "west");
    CHECK(selectableFor(n, d.network.connectors[0].id) == d.network.connectors[0].id);
    CHECK(selectableFor(n, "no-such-object").empty());
    CHECK(selectableFor(n, "").empty());
}
TEST(diagnostics, draft_issues_carry_object_ids) {
    auto d = merging();
    d.network.links[0].lanes[0].width = -1;
    d.network.links[1].id = "west";                       // duplicate id
    d.network.signalHeads.push_back({"head", {"exit","exit-1"}, 1e9, "program"});
    const auto rows = networkDiagnostics(d.network);
    const auto* width = find(rows, "INVALID_WIDTH");
    CHECK(width && width->objectId == "west-1" && width->selectId == "west");
    const auto* duplicate = find(rows, "DUPLICATE_ID");
    CHECK(duplicate && !duplicate->objectId.empty());
    const auto* position = find(rows, "INVALID_POSITION");
    CHECK(position && position->objectId == "head" && position->selectId == "head");
    for (const auto& row : rows) CHECK(row.severity == DiagnosticSeverity::draft);
}
TEST(diagnostics, empty_network_is_a_draft_not_an_error) {
    CHECK(!blocksDraft("EMPTY_NETWORK"));
    CHECK(blocksDraft("INVALID_WIDTH"));
    validateDocument(ProjectDocument{});                  // a blank document must stay editable
    const auto rows = documentDiagnostics(ProjectDocument{});
    const auto* empty = find(rows, "EMPTY_NETWORK");
    CHECK(empty && empty->severity == DiagnosticSeverity::runtime);
}
TEST(diagnostics, runtime_issues_are_informational_not_blocking) {
    auto d = merging();
    const auto rows = documentDiagnostics(d);
    const auto* merge = find(rows, "UNSUPPORTED_MERGE");
    CHECK(merge && merge->severity == DiagnosticSeverity::runtime);
    CHECK(merge->objectId == "exit-1" && merge->selectId == "exit");
    // The whole point of the separation: authoring the merge is still a legal, committable edit.
    CHECK(validateNetwork(d.network).empty());
    History history; history.reset(ProjectDocument{});
    CHECK(history.execute("test", [](auto& doc) { doc = merging(); }));
    CHECK(find(documentDiagnostics(history.document()), "UNSUPPORTED_MERGE"));
}
TEST(diagnostics, invalid_network_skips_the_runtime_pass_without_throwing) {
    auto d = merging();
    d.network.connectors[0].from.laneId = "ghost";        // laneGeometry would throw on this
    const auto rows = runtimeDiagnostics(d.network, {});
    CHECK(rows.size() == 1 && rows[0].code == "EDIT_RUNTIME_SKIPPED");
    CHECK(rows[0].severity == DiagnosticSeverity::runtime);
    CHECK(find(documentDiagnostics(d), "UNKNOWN_LANE"));
    CHECK(find(documentDiagnostics(d), "EDIT_RUNTIME_SKIPPED"));
}
TEST(diagnostics, missing_or_broken_definition_is_reported_not_thrown) {
    auto d = merging();
    CHECK(!d.definition);
    CHECK(find(documentDiagnostics(d), "EDIT_NO_DEFINITION"));
    d.definition.emplace(); d.definition->timeStep = -1; // Invalid typed demand is diagnosed.
    const auto rows = documentDiagnostics(d);
    CHECK(!rows.empty() && !find(rows, "EDIT_NO_DEFINITION"));
    CHECK(rows.back().severity == DiagnosticSeverity::runtime);
}
TEST(diagnostics, catalog_backed_references_are_withheld_not_blamed_on_the_drawing) {
    // The M0 fixture's inputs name vehicle types that live in data/, not in the document, so
    // the editor must say it cannot judge them rather than report them as unknown.
    const auto rows = documentDiagnostics(crossing());
    CHECK(!find(rows, "UNKNOWN_VEHICLE_TYPE"));
    CHECK(!find(rows, "UNKNOWN_BEHAVIOUR"));
    const auto* catalog = find(rows, "EDIT_NO_CATALOG");
    CHECK(catalog && catalog->severity == DiagnosticSeverity::runtime);
    CHECK(rows.size() == 1);
    // Topology itself is clean, and that is judged without any catalog.
    CHECK(!find(rows, "UNSUPPORTED_MERGE"));
}
TEST(diagnostics, every_emitted_code_has_a_translation) {
    // Derived from the validators, not from a hand-kept list, so it cannot rot.
    std::set<std::string> codes{"EDIT_NO_DEFINITION", "EDIT_RUNTIME_SKIPPED", "EDIT_NO_CATALOG"};
    auto broken = merging();
    broken.network.links[0].lanes[0].width = -1;
    broken.network.links[0].geometry = {{0,0},{0,0}};
    broken.network.links[1].id = "  ";
    broken.network.links[2].lanes.clear();
    broken.network.connectors[0].to.laneId = "ghost";
    broken.network.signalHeads.push_back({"head", {"west","west-1"}, -5, " "});
    broken.network.drivingSide = static_cast<DrivingSide>(7);
    for (const auto& row : networkDiagnostics(broken.network)) codes.insert(row.code);
    for (const auto& row : documentDiagnostics(ProjectDocument{})) codes.insert(row.code);
    auto runtime = crossing();
    auto definition = *runtime.definition;
    definition.timeStep = 5; definition.duration = 0.3;
    definition.routes.push_back({"cycle", {"north-1", "north-1"}});
    definition.inputs.push_back({"bad", "ghost-route", "ghost-type", -1, 10, 1});
    definition.signalPrograms.push_back({"blank", 0, {}});
    for (const auto& row : runtimeDiagnostics(runtime.network, definition)) codes.insert(row.code);
    for (const auto& row : documentDiagnostics(merging())) codes.insert(row.code);
    CHECK(codes.size() > 12);
    for (const auto* language : {"en", "th"}) {
        std::ifstream file(test::root()/"data/locales"/(std::string(language) + ".json"));
        Json locale; file >> locale;
        for (const auto& code : codes) {
            CHECK(locale.contains(code));
            CHECK(locale.at(code).is_string() && !locale.at(code).get<std::string>().empty());
        }
    }
}
// A turn no vehicle could take is still a legal drawing: it is reported, never blocked.
TEST(diagnostics, a_tight_connector_is_advised_without_blocking_run) {
    auto d=merging();
    const auto id=d.network.connectors.front().id;
    CHECK(connectorShapeIssues(d.network).empty());
    // The forcing: bend this connector into a hairpin far tighter than its 3.5 m lane.
    auto& c=editableConnector(d,id);
    const auto a=c.geometry.front(),b=c.geometry.back();
    c.geometry={a,{a.x+1,a.y+1},{a.x,a.y+2},{a.x-1,a.y+1},{(a.x+b.x)/2,(a.y+b.y)/2},b};
    c.laneBlend.clear();
    validateDocument(d);
    const auto rows=documentDiagnostics(d);
    int advised=0;
    for(const auto& row:rows)if(row.code=="TIGHT_CONNECTOR_RADIUS"){++advised;CHECK(row.selectId==id);}
    CHECK(advised==1);
    // Neither the draft nor the run is refused because of it.
    auto definition=test::straight();definition.routes.clear();definition.inputs.clear();
    test::throws([&]{compileScenario(d.network,definition);},"UNSUPPORTED_MERGE");
    d.network.connectors.erase(d.network.connectors.begin()+1);
    CHECK(connectorShapeIssues(d.network).size()==1);
    CHECK(compileScenario(d.network,definition).segments.size()>0);
}
