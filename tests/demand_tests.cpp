#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
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
    // A route names objects (M1.26), and an object that does not exist is refused where the
    // route is stored -- not later, by a compile that would report a segment nobody authored.
    test::throws([&]{h.execute("bad route",[](auto& d){putRoute(d,{"",{"ghost"}});});},"EDIT_UNKNOWN_OBJECT");
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
// Hard rule 2: same scenario, same seed, same build, same trajectory. Sectioning a lane changes
// how many segments a route is made of and therefore how RoutePart offsets are laid out, which is
// exactly the kind of change that silently perturbs a run.
TEST(demand, deterministic_replay_survives_lane_sectioning) {
    const auto build=[](double station) {
        History h;h.reset();std::string route;
        h.execute("roads",[](auto& d){
            addLink(d,{{0,0},{100,0}},1,3.5);addLink(d,{{120,40},{200,40}},1,3.5);});
        h.execute("turn",[&](auto& d){
            const auto& a=d.network.links[0];const auto& b=d.network.links[1];
            const auto id=addConnector(d,{a.id,a.lanes.front().id,station},{b.id,b.lanes.front().id,0});
            route=putRoute(d,{"",{a.lanes.front().id,id,b.lanes.front().id}});
            putInput(d,{"",route,"car",900,0,60});});
        return compileDocument(h.document(),test::root()/"data").scenario;
    };
    const auto sectioned=build(40), whole=build(100);
    // The forcing: the two networks really are compiled differently. Without this, the streams
    // below could match simply because sectioning never happened.
    CHECK(sectioned.segments.size()==whole.segments.size()+1);
    CHECK(std::any_of(sectioned.segments.begin(),sectioned.segments.end(),
                      [](const auto& s){return s.id.find("/sec-")!=std::string::npos;}));
    CHECK(std::none_of(whole.segments.begin(),whole.segments.end(),
                       [](const auto& s){return s.id.find("/sec-")!=std::string::npos;}));
    const auto stream=[](const Scenario& s,std::uint32_t seed) {
        std::vector<SimEvent> result;
        runSimulation(s,seed,[&](const auto& e){result.push_back(e);},true);
        return result;
    };
    // Replay: the same seed on the same build reproduces the run exactly, sections and all.
    CHECK(stream(sectioned,42)==stream(sectioned,42));
    CHECK(stream(sectioned,7)==stream(sectioned,7));
    // And a route that turns off at 40 m of a 100 m lane is genuinely a shorter drive than one
    // that turns off at the end: if sectioning did not shorten the travel, these would agree.
    CHECK(stream(sectioned,42)!=stream(whole,42));
    const auto reachEnd=[&](const Scenario& s){return runSimulation(s,42).completed;};
    CHECK(reachEnd(sectioned)>0);CHECK(reachEnd(whole)>0);
    CHECK(reachEnd(sectioned)>=reachEnd(whole)); // The shorter route cannot deliver fewer.
}
// Hard rule 5: the two numbers a derived priority rule is given are content, not code. Changing
// the gap time must be a data edit, not a recompile.
TEST(demand, priority_defaults_come_from_the_data_catalog_and_do_not_break_portability) {
    std::ifstream file(test::root()/"data/scenarios/crossing.json");Json j;file>>j;
    const auto d=parseDocument(j);
    const auto resolved=resolveCatalogs(*d.definition,test::root()/"data");
    // The forcing: the values are the ones in data/priority-rules/default.json, not struct
    // defaults -- PriorityDefaults is zero-initialised, so reading the file is the only way here.
    test::near(resolved.priorityDefaults.gapTime,3.0,1e-12);
    test::near(resolved.priorityDefaults.headway,7.0,1e-12);
    CHECK(PriorityDefaults{}.gapTime==0);
    // And a document with its own catalogs still resolves against a directory that has none: the
    // numbers are needed only where a rule is derived, so their absence is not a load failure.
    auto portable=*d.definition;
    portable.externalVehicleTypes=false;portable.externalBehaviours=false;
    portable.vehicleTypes=resolved.vehicleTypes;portable.behaviours=resolved.behaviours;
    const auto bare=resolveCatalogs(portable,test::root()/"missing-catalog");
    CHECK(bare.priorityDefaults==PriorityDefaults{});
    CHECK(!bare.vehicleTypes.empty());
}
