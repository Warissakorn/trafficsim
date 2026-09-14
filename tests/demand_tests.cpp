#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/run.hpp"
#include <fstream>
using namespace trafficsim;
TEST(demand, authors_runs_and_preserves_snapshot) {
    History h;h.reset();std::string route,input;
    h.execute("road",[](auto& d){addLink(d,{{0,0},{100,0}},1,3.5);});
    h.execute("route",[&](auto& d){route=putRoute(d,{"",{d.network.links.front().lanes.front().id}});});
    h.execute("input",[&](auto& d){input=putInput(d,{"",route,"car",600,0,60});});
    const auto snapshot=compileDocument(h.document(),test::root()/"data");
    CHECK(snapshot.revision==h.revision());CHECK(snapshot.scenario.routes.size()==1);
    const auto end=runSimulation(snapshot.scenario,42);
    CHECK(end.completed>0);
    const auto before=documentJson(h.document());h.markSaved();
    h.execute("delete",[&](auto& d){deleteRoute(d,route);});
    CHECK(h.document().definition->inputs.empty());CHECK(h.document().definition->routes.empty());
    CHECK(snapshot.scenario.inputs.front().id==input); // Detached snapshot survived the edit.
    h.undo();CHECK(documentJson(h.document())==before);CHECK(!h.dirty());
    CHECK(documentJson(parseDocument(Json::parse(before.dump())))==before);
}
TEST(demand, failed_edits_preserve_redo_and_allocator) {
    History h;h.reset();
    h.execute("road",[](auto& d){addLink(d,{{0,0},{100,0}},1,3.5);});h.undo();
    const auto before=documentJson(h.document());
    test::throws([&]{h.execute("bad route",[](auto& d){putRoute(d,{"",{"ghost"}});});},"UNKNOWN_SEGMENT");
    CHECK(documentJson(h.document())==before);CHECK(h.canRedo());CHECK(!h.canUndo());
}
TEST(demand, catalog_overrides_and_missing_catalog_are_explicit) {
    std::ifstream file(test::root()/"data/scenarios/crossing.json");Json j;file>>j;
    const auto d=parseDocument(j);CHECK(d.definition->externalVehicleTypes);
    const auto snapshot=compileDocument(d,test::root()/"data");
    CHECK(!snapshot.scenario.vehicleTypes.empty());
    auto explicitCatalog=*d.definition;
    explicitCatalog.externalVehicleTypes=false;explicitCatalog.externalBehaviours=false;
    explicitCatalog.vehicleTypes=snapshot.scenario.vehicleTypes;explicitCatalog.behaviours=snapshot.scenario.behaviours;
    auto portable=d;portable.definition=explicitCatalog;
    CHECK(compileDocument(portable,test::root()/"missing-catalog").scenario.inputs.size()==2);
    auto empty=explicitCatalog;empty.vehicleTypes.clear();
    CHECK(definitionJson(parseAuthoringDefinition(definitionJson(empty)))["vehicleTypes"].empty());
    test::throws([&]{compileDocument(d,test::root()/"missing-catalog");},"EDIT_CATALOG_READ");
}
TEST(demand, program_deletion_is_reference_safe_and_settings_validate_together) {
    std::ifstream file(test::root()/"data/scenarios/crossing.json");Json j;file>>j;
    History h;h.reset(parseDocument(j));const auto before=documentJson(h.document());
    test::throws([&]{h.execute("program",[](auto& d){deleteProgram(d,d.network.signalHeads.front().programId);});},"EDIT_REFERENCED_PROGRAM");
    test::throws([&]{h.execute("settings",[](auto& d){changeRunSettings(d,1,.1);});},"INVALID_INTERVAL");
    CHECK(documentJson(h.document())==before);CHECK(!h.canUndo());
}
