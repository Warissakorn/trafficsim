#include "test.hpp"
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/behaviour_library.hpp"
#include "../src/project/run.hpp"
#include <fstream>
using namespace trafficsim;
// M3.3.2a (D126): project-owned behaviour library, classes, link behaviour types, assignment.
// BA06-BA09 in docs/plans/DRIVING_BEHAVIOUR.md. Nothing here changes the engine.
namespace {
const std::string kDefault = "wiedemann-inspired-prototype";
// Owned catalogs from data/, two Links joined by a Connector, one routed input.
ProjectDocument owned() {
    ProjectDocument d; const auto a = addLink(d, {{0, 0}, {200, 0}}, 1, 3.5), b = addLink(d, {{220, 0}, {400, 0}}, 1, 3.5);
    addConnector(d, {a, d.network.links[0].lanes[0].id, {}}, {b, d.network.links[1].lanes[0].id, {}});
    const auto route = putRoute(d, {"route", {a, d.network.connectors[0].id, b}}); changeRunSettings(d, 60, .1);
    putDemandCatalog(d, resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data"));
    putInput(d, {"in", route, "car", 600, 0, 60});
    validateDocument(d);
    return d;
}
// The library this file exercises: a slow set, a class of heavy vehicles, an urban road type.
ProjectDocument library() {
    auto d = owned(); auto slow = d.definition->behaviours.front(); slow.id = "slow"; slow.followingTime = 1.5;
    putBehaviour(d, slow, "Slow");
    putVehicleClass(d, {"heavy", "Heavy", {"heavy-vehicle"}});
    putLinkBehaviourType(d, {"urban", "Urban", kDefault, {{"heavy", "slow"}}});
    assignBehaviourType(d, d.network.links[0].id, "urban");
    assignBehaviourType(d, d.network.connectors[0].id, "urban");
    validateDocument(d);
    return d;
}
History history(const ProjectDocument& d) { History h; h.reset(d); return h; }
// A rejected edit leaves the published document, its revision and the Undo stack untouched.
void rejected(History& h, const std::function<void(ProjectDocument&)>& change, const char* code) {
    const auto before = documentJson(h.document()); const auto revision = h.revision(); const bool undo = h.canUndo();
    test::throws([&] { h.execute("library", change); }, code);
    CHECK(documentJson(h.document()) == before); CHECK(h.revision() == revision); CHECK(h.canUndo() == undo);
}
}
TEST(behaviourlibrary, schema21_round_trips_and_legacy_files_keep_their_version) {
    const auto d = library(); const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 21);
    CHECK(j["network"]["links"][0]["behaviourType"] == "urban"); CHECK(!j["network"]["links"][1].contains("behaviourType"));
    CHECK(j["network"]["connectors"][0]["behaviourType"] == "urban");
    for (const auto& b : j["definition"]["behaviours"]) CHECK(b["model"] == "prototype");
    CHECK(j["definition"]["behaviours"][1]["name"] == "Slow");
    CHECK(j["definition"]["vehicleClasses"][0]["vehicleTypeIds"] == Json::array({"heavy-vehicle"}));
    CHECK(j["definition"]["linkBehaviourTypes"][0]["overrides"][0]["behaviourId"] == "slow");
    const auto reopened = parseDocument(Json::parse(j.dump()));
    CHECK(reopened == d); CHECK(documentJson(reopened) == j);
    // Without any library feature the file keeps its previous schema and carries no new key.
    const auto plain = documentJson(owned());
    CHECK(plain["schemaVersion"] == 18); CHECK(!plain["definition"]["behaviours"][0].contains("model"));
    CHECK(!plain["definition"].contains("vehicleClasses"));
}
TEST(behaviourlibrary, older_schemas_and_unknown_keys_are_refused_not_dropped) {
    const auto j = documentJson(library());
    auto old = j; old["schemaVersion"] = 20;
    CHECK(old["network"]["links"][0].contains("behaviourType"));
    test::throws([&] { parseDocument(old); }, "EDIT_UNSUPPORTED_FIELD");
    old["network"]["links"][0].erase("behaviourType"); old["network"]["connectors"][0].erase("behaviourType");
    test::throws([&] { parseDocument(old); }, "definition.vehicleClasses");
    old["definition"].erase("vehicleClasses"); old["definition"].erase("linkBehaviourTypes");
    test::throws([&] { parseDocument(old); }, "behaviours[0].model");
    for (const auto& [section, key] : std::vector<std::pair<std::string, std::string>>{
             {"behaviours", "followingTimeAlias"}, {"vehicleTypes", "behaviourClass"},
             {"vehicleClasses", "pcu"}, {"linkBehaviourTypes", "laneOverrides"}}) {
        auto future = j; future["definition"][section][0][key] = 1;
        CHECK(future["definition"][section][0].contains(key));
        test::throws([&] { parseDocument(future); }, "EDIT_UNSUPPORTED_FIELD");
    }
    auto override = j; override["definition"]["linkBehaviourTypes"][0]["overrides"][0]["lane"] = 1;
    test::throws([&] { parseDocument(override); }, "EDIT_UNSUPPORTED_FIELD");
    for (const Json model : {Json("w99"), Json(nullptr)}) {
        auto future = j; future["definition"]["behaviours"][0]["model"] = model;
        test::throws([&] { parseDocument(future); }, "UNSUPPORTED_BEHAVIOUR_MODEL");
    }
    auto missing = j; missing["definition"]["behaviours"][0].erase("model");
    test::throws([&] { parseDocument(missing); }, "UNSUPPORTED_BEHAVIOUR_MODEL");
    auto future = j; future["schemaVersion"] = 22; test::throws([&] { parseDocument(future); }, "EDIT_VERSION");
}
TEST(behaviourlibrary, invalid_entries_reject_load_and_edit_atomically_even_when_unused) {
    const auto j = documentJson(library());
    const auto refuse = [&](const std::function<void(Json&)>& change, const char* code) {
        auto file = j; change(file); test::throws([&] { parseDocument(file); }, code);
    };
    refuse([](Json& f) { f["definition"]["vehicleClasses"].push_back({{"id", "also"}, {"vehicleTypeIds", {"heavy-vehicle"}}}); }, "DUPLICATE_CLASS_MEMBERSHIP");
    refuse([](Json& f) { f["definition"]["vehicleClasses"][0]["vehicleTypeIds"].push_back("bus"); }, "UNKNOWN_VEHICLE_TYPE");
    refuse([](Json& f) { f["definition"]["vehicleClasses"][0]["id"] = " "; }, "INVALID_ID");
    refuse([](Json& f) { f["definition"]["linkBehaviourTypes"].push_back(f["definition"]["linkBehaviourTypes"][0]); }, "DUPLICATE_ID");
    // An unused behaviour type is still validated: it needs a default and real references.
    refuse([](Json& f) { f["definition"]["linkBehaviourTypes"].push_back({{"id", "unused"}, {"defaultBehaviourId", ""}, {"overrides", Json::array()}}); }, "MISSING_DEFAULT_BEHAVIOUR");
    refuse([](Json& f) { f["definition"]["linkBehaviourTypes"].push_back({{"id", "unused"}, {"defaultBehaviourId", "gone"}, {"overrides", Json::array()}}); }, "UNKNOWN_BEHAVIOUR");
    refuse([](Json& f) { f["definition"]["linkBehaviourTypes"][0]["overrides"].push_back({{"classId", "heavy"}, {"behaviourId", "slow"}}); }, "DUPLICATE_OVERRIDE");
    refuse([](Json& f) { f["definition"]["linkBehaviourTypes"][0]["overrides"][0]["classId"] = "bikes"; }, "UNKNOWN_VEHICLE_CLASS");
    refuse([](Json& f) { f["definition"]["linkBehaviourTypes"][0]["overrides"][0]["behaviourId"] = ""; }, "UNKNOWN_BEHAVIOUR");
    refuse([](Json& f) { f["network"]["links"][1]["behaviourType"] = "rural"; }, "UNKNOWN_BEHAVIOUR_TYPE");
    refuse([](Json& f) { f["network"]["links"][1]["behaviourType"] = ""; }, "UNKNOWN_BEHAVIOUR_TYPE");
    refuse([](Json& f) { f["definition"].erase("behaviours"); }, "EXTERNAL_BEHAVIOUR_CATALOG");
    auto h = history(library());
    rejected(h, [](auto& d) { putVehicleClass(d, {"cars", "Cars", {"car", "heavy-vehicle"}}); }, "DUPLICATE_CLASS_MEMBERSHIP");
    rejected(h, [](auto& d) { putLinkBehaviourType(d, {"rural", "Rural", "", {}}); }, "MISSING_DEFAULT_BEHAVIOUR");
    rejected(h, [](auto& d) { assignBehaviourType(d, d.network.links[1].id, "rural"); }, "UNKNOWN_BEHAVIOUR_TYPE");
    rejected(h, [](auto& d) { assignBehaviourType(d, "nowhere", "urban"); }, "EDIT_UNKNOWN_OBJECT");
    auto external = history(owned());
    rejected(external, [](auto& d) { d.definition->externalBehaviours = true; putVehicleClass(d, {"heavy", "", {}}); }, "EDIT_EXTERNAL_CATALOG");
}
TEST(behaviourlibrary, duplicate_is_independent_and_shared_edit_reaches_only_its_users) {
    auto h = history(library()); const auto before = documentJson(h.document());
    std::string copy;
    CHECK(h.execute("duplicate", [&](auto& d) { copy = duplicateBehaviour(d, "slow"); }));
    CHECK(copy == "slow-copy");
    const auto& def = *h.document().definition;
    CHECK(def.behaviourNames.at(copy) == "Slow");
    CHECK(behaviourUsers(h.document(), copy).empty());
    CHECK(behaviourUsers(h.document(), "slow").behaviourTypes == std::vector<std::string>({"urban"}));
    CHECK(behaviourTypeUsers(h.document(), "urban").roads.size() == 2); // the Link and the Connector
    // Editing the original changes what its users see; the copy keeps its own values.
    CHECK(h.execute("edit", [&](auto& d) { auto slow = d.definition->behaviours[1]; slow.followingTime = 2; putBehaviour(d, slow, "Slower"); }));
    const auto& after = *h.document().definition;
    const auto value = [&](const std::string& id) {
        return std::find_if(after.behaviours.begin(), after.behaviours.end(), [&](const auto& b) { return b.id == id; })->followingTime;
    };
    test::near(value("slow"), 2); test::near(value(copy), 1.5); CHECK(after.behaviourNames.at(copy) == "Slow");
    std::string second; h.execute("again", [&](auto& d) { second = duplicateBehaviour(d, "slow"); });
    CHECK(second == "slow-copy-2");
    std::string type, group;
    h.execute("type", [&](auto& d) { type = duplicateLinkBehaviourType(d, "urban"); group = duplicateVehicleClass(d, "heavy"); });
    const auto& types = h.document().definition->linkBehaviourTypes;
    CHECK(types.back().id == type && types.back().overrides == types.front().overrides);
    CHECK(h.document().definition->vehicleClasses.back().vehicleTypeIds.empty()); // a type is in one class
    while (h.canUndo()) h.undo();
    CHECK(documentJson(h.document()) == before);
}
TEST(behaviourlibrary, referenced_delete_needs_one_validated_reassignment_transaction) {
    auto h = history(library()); const auto before = documentJson(h.document());
    rejected(h, [](auto& d) { deleteBehaviour(d, "slow"); }, "EDIT_REFERENCED_BEHAVIOUR");
    rejected(h, [](auto& d) { deleteVehicleClass(d, "heavy"); }, "EDIT_REFERENCED_VEHICLE_CLASS");
    rejected(h, [](auto& d) { deleteLinkBehaviourType(d, "urban"); }, "EDIT_REFERENCED_BEHAVIOUR_TYPE");
    rejected(h, [](auto& d) { deleteBehaviour(d, "slow", std::string("missing")); }, "UNKNOWN_BEHAVIOUR");
    rejected(h, [](auto& d) { deleteBehaviour(d, "slow", std::string("slow")); }, "EDIT_INVALID_REPLACEMENT");
    rejected(h, [](auto& d) { deleteBehaviour(d, "gone"); }, "EDIT_UNKNOWN_BEHAVIOUR");
    // The default set is referenced by both vehicle types and by the road type's default.
    CHECK(behaviourUsers(h.document(), kDefault).vehicleTypes.size() == 2);
    const auto revision = h.revision();
    CHECK(h.execute("reassign", [](auto& d) { deleteBehaviour(d, kDefault, std::string("slow")); }));
    CHECK(h.revision() != revision);
    const auto& def = *h.document().definition;
    CHECK(def.behaviours.size() == 1 && def.behaviours[0].id == "slow");
    for (const auto& t : def.vehicleTypes) CHECK(t.behaviourId == "slow");
    CHECK(def.linkBehaviourTypes[0].defaultBehaviourId == "slow");
    h.undo(); CHECK(documentJson(h.document()) == before);
    // Road reassignment and class reassignment are each one step too.
    h.execute("rural", [](auto& d) { putLinkBehaviourType(d, {"rural", "", kDefault, {}}); putVehicleClass(d, {"cars", "", {"car"}}); });
    const auto withRural = documentJson(h.document());
    CHECK(h.execute("retire", [](auto& d) { deleteLinkBehaviourType(d, "urban", std::string("rural")); deleteVehicleClass(d, "heavy", std::string("cars")); }));
    for (const auto& l : h.document().network.links) CHECK(!l.behaviourTypeId || *l.behaviourTypeId == "rural");
    CHECK(h.document().network.connectors[0].behaviourTypeId == "rural");
    h.undo(); CHECK(documentJson(h.document()) == withRural);
    // An unreferenced entry deletes without a replacement.
    CHECK(h.execute("unused", [](auto& d) { deleteLinkBehaviourType(d, "rural"); }));
}
// M3.3.2b (D127) replaced the M3.3.2a Run refusal: an assigned road now compiles into selections.
TEST(behaviourlibrary, assigned_roads_compile_and_an_unassigned_library_changes_nothing) {
    const auto assigned = library();
    const auto compiled = compileDocument(assigned, test::root() / "data").scenario;
    CHECK(!compiled.segmentBehaviours.empty());
    const auto rows = runDiagnostics(assigned, test::root() / "data");
    CHECK(std::none_of(rows.begin(), rows.end(), [](const auto& r) { return r.severity == DiagnosticSeverity::runtime; }));
    auto unassigned = assigned;
    for (auto& l : unassigned.network.links) l.behaviourTypeId.reset();
    for (auto& c : unassigned.network.connectors) c.behaviourTypeId.reset();
    CHECK(usesBehaviourLibrary(unassigned)); // still schema 21: the library itself is kept
    CHECK(compileDocument(unassigned, test::root() / "data").scenario.segmentBehaviours.empty());
    unassigned.definition->behaviourNames.clear(); unassigned.definition->vehicleClasses.clear();
    unassigned.definition->linkBehaviourTypes.clear();
    auto withLibrary = unassigned; putVehicleClass(withLibrary, {"heavy", "", {"heavy-vehicle"}});
    putLinkBehaviourType(withLibrary, {"urban", "", kDefault, {}});
    CHECK(compileDocument(withLibrary, test::root() / "data").scenario == compileDocument(unassigned, test::root() / "data").scenario);
}
TEST(behaviourlibrary, every_new_code_is_translated_in_english_and_thai) {
    for (const char* language : {"en", "th"}) {
        std::ifstream file(test::root() / "data/locales" / (std::string(language) + ".json"));
        CHECK(file.is_open()); Json locale; file >> locale;
        for (const char* code : {"UNSUPPORTED_BEHAVIOUR_MODEL", "EXTERNAL_BEHAVIOUR_CATALOG", "DUPLICATE_CLASS_MEMBERSHIP",
                 "MISSING_DEFAULT_BEHAVIOUR", "UNKNOWN_VEHICLE_CLASS", "DUPLICATE_OVERRIDE", "UNKNOWN_BEHAVIOUR_TYPE",
                 "EDIT_EXTERNAL_CATALOG", "EDIT_UNKNOWN_BEHAVIOUR",
                 "EDIT_UNKNOWN_VEHICLE_CLASS", "EDIT_UNKNOWN_BEHAVIOUR_TYPE", "EDIT_REFERENCED_BEHAVIOUR",
                 "EDIT_REFERENCED_VEHICLE_CLASS", "EDIT_REFERENCED_BEHAVIOUR_TYPE", "EDIT_INVALID_REPLACEMENT"})
            CHECK(locale.contains(code) && locale.at(code).is_string() && !locale.at(code).get<std::string>().empty());
    }
}
// M3.3.2c (D128): the inspector and the compiler read one precedence.
TEST(behaviourlibrary, effective_road_behaviours_report_value_and_source) {
    const auto d = library(); const auto& def = *d.definition;
    const auto lookup = [](const std::vector<RoadBehaviour>& all, const std::string& type) {
        return *std::find_if(all.begin(), all.end(), [&](const auto& r) { return r.vehicleTypeId == type; });
    };
    const auto inherited = effectiveRoadBehaviours(def, std::nullopt);
    CHECK(lookup(inherited, "car") == RoadBehaviour{"car", kDefault, BehaviourSource::inherited});
    const auto urban = effectiveRoadBehaviours(def, std::string("urban"));
    CHECK(lookup(urban, "car") == RoadBehaviour{"car", kDefault, BehaviourSource::typeDefault});
    CHECK(lookup(urban, "heavy-vehicle") == RoadBehaviour{"heavy-vehicle", "slow", BehaviourSource::classOverride});
    test::throws([&] { effectiveRoadBehaviours(def, std::string("rural")); }, "UNKNOWN_BEHAVIOUR_TYPE");
    for (const auto& s : compileBehaviourAssignments(d.network, def))
        CHECK(s.behaviourId == lookup(urban, s.vehicleTypeId).behaviourId);
}
TEST(behaviourlibrary, editor_keys_are_translated_in_english_and_thai) {
    for (const char* language : {"en", "th"}) {
        std::ifstream file(test::root() / "data/locales" / (std::string(language) + ".json"));
        CHECK(file.is_open()); Json locale; file >> locale;
        for (const char* key : {"editorBehaviourLibrary", "editorBehaviourHelp", "editorBehaviourTabBehaviours",
                 "editorBehaviourTabClasses", "editorBehaviourTabTypes", "editorBehaviourDuplicate", "editorBehaviourUsedBy",
                 "editorBehaviourUnused", "editorBehaviourReplacement", "editorBehaviourMembers", "editorBehaviourDefault",
                 "editorBehaviourOverrides", "editorBehaviourClassColumn", "editorBehaviourInherit", "editorBehaviourType",
                 "editorApplyBehaviourType", "editorEffectiveBehaviour", "editorBehaviourSourceOverride",
                 "editorBehaviourSourceDefault", "editorBehaviourSourceInherited", "editorBehaviourMissing"})
            CHECK(locale.contains(key) && locale.at(key).is_string() && !locale.at(key).get<std::string>().empty());
        CHECK(locale.at("editorBehaviourUsedBy").get<std::string>().find("%1") != std::string::npos);
    }
}
