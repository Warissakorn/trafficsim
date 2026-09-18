#include "test.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/display.hpp"
#include "../src/project/run.hpp"
using namespace trafficsim;
namespace {
ProjectDocument roads(DrivingSide side=DrivingSide::left) {
    ProjectDocument d;d.network.drivingSide=side;
    d.network.links={
        {"a",{{0,0},{40,0}},{{"a1",3},{"a2",4},{"a3",3.5}}},
        {"b",{{60,10},{120,10}},{{"b1",3.2},{"b2",3.7},{"b3",4}}}};
    return d;
}
// Every path begins and ends on the middle of the lane it names, AT THE STATION it names. Not
// at the lane's end: since a Connector keeps its own position, a Link edit moves the station the
// Connector sits at rather than dragging the Connector to the Link's end.
void anchored(const ProjectDocument& d) {
    CHECK(validateNetwork(d.network).empty());
    for(const auto& c:d.network.connectors)for(const auto& path:connectorPaths(d.network,c)) {
        const auto a=laneAttachment(d.network,path.from,true),b=laneAttachment(d.network,path.to,false);
        test::near(path.geometry.front().x,a.x,1e-9);test::near(path.geometry.front().y,a.y,1e-9);
        test::near(path.geometry.back().x,b.x,1e-9);test::near(path.geometry.back().y,b.y,1e-9);
    }
}
}
TEST(ranges, one_object_compiles_stable_lane_paths_on_both_driving_sides) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},3,3);
        CHECK(d.network.connectors.size()==1);const auto paths=connectorPaths(d.network,d.network.connectors.front());
        CHECK(paths.size()==3);CHECK(paths[0].id==id);CHECK(paths[2].id==id+"/lane-3");anchored(d);
        const auto route=putRoute(d,{"",{"a3",paths[2].id,"b3"}});putInput(d,{"",route,"car",600,0,60});
        validateDocument(d);const auto snapshot=compileDocument(d,test::root()/"data");
        CHECK(snapshot.scenario.segments.size()==9);CHECK(runSimulation(snapshot.scenario,42).completed>0);
        const auto json=documentJson(d);CHECK(documentJson(parseDocument(Json::parse(json.dump())))==json);
        changeGeometry(d,"a",{{0,0},{40,5}});anchored(d);
        changeDrivingSide(d,side==DrivingSide::left?DrivingSide::right:DrivingSide::left);anchored(d);
    }
}
TEST(ranges, shrinking_retargeting_duplicates_and_delete_are_atomic) {
    auto d=roads();const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},3,3);
    const auto route=putRoute(d,{"",{"a3",id+"/lane-3","b3"}});putInput(d,{"",route,"car",600,0,60});
    History h;h.reset(d);const auto before=documentJson(d);
    test::throws([&]{h.execute("shrink",[](auto& m){changeLanes(m,"a",{3,4});});},"EDIT_REFERENCED_LANE");
    test::throws([&]{h.execute("range",[&](auto& m){changeConnectorRange(m,id,2,2);});},"EDIT_REFERENCED_CONNECTOR");
    test::throws([&]{h.execute("duplicate",[](auto& m){addConnector(m,{"a","a3"},{"b","b3"});});},"DUPLICATE_CONNECTION");
    CHECK(documentJson(h.document())==before);CHECK(!h.canUndo());
    h.execute("delete",[&](auto& m){deleteConnector(m,id);});
    CHECK(h.document().definition->routes.empty());CHECK(h.document().definition->inputs.empty());
    h.undo();CHECK(documentJson(h.document())==before);
}
TEST(ranges, unequal_ranges_author_merges_without_weakening_runtime_guard) {
    auto d=roads();const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},3,2);
    const auto route=putRoute(d,{"",{"a1",id,"b1"}});putInput(d,{"",route,"car",600,0,60});
    validateDocument(d);anchored(d);
    test::throws([&]{compileDocument(d,test::root()/"data");},"UNSUPPORTED_MERGE");
}
TEST(ranges, duplication_preserves_internal_geometry_control_and_metadata_without_demand) {
    auto d=roads();const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},2,2);
    for(const auto& object:std::vector<std::string>{"a","b",id})changeAppearance(d,object,1,"ramp");
    const auto program=putProgram(d,{"",0,{{10,SignalColor::green}}});
    putSignalHead(d,{"",{"a","a1"},20,program,{}});
    const auto route=putRoute(d,{"",{"a2",id+"/lane-2","b2"}});putInput(d,{"",route,"car",600,0,60});
    History h;h.reset(d);const auto before=documentJson(d);
    h.execute("copy",[](auto& m){duplicateObjects(m,{"a","b"},{0,100});});
    const auto& copy=h.document();CHECK(copy.network.links.size()==4);CHECK(copy.network.connectors.size()==2);
    CHECK(copy.network.signalHeads.size()==2);CHECK(copy.definition->inputs.size()==1);anchored(copy);
    for(const auto& l:copy.network.links){CHECK(l.level==1);CHECK(l.displayType=="ramp");}
    for(const auto& c:copy.network.connectors){CHECK(c.level==1);CHECK(c.displayType=="ramp");}
    CHECK(copy.network.signalHeads.back().programId==program);
    h.undo();CHECK(documentJson(h.document())==before);
}
TEST(ranges, catalogs_are_content_and_schema_one_upgrades_without_losing_ids) {
    const auto catalog=loadDisplayCatalog(test::root()/"data");CHECK(catalog.levels.size()>=3);CHECK(catalog.types.size()>=3);
    auto d=roads();const auto id=addConnector(d,{"a","a1"},{"b","b1"});auto old=documentJson(d);old["schemaVersion"]=1;
    for(auto& l:old["network"]["links"]){l.erase("level");l.erase("displayType");}
    for(auto& c:old["network"]["connectors"])for(const auto* key:{"fromLaneCount","toLaneCount","level","displayType"})c.erase(key);
    const auto restored=parseDocument(old);CHECK(restored.network.connectors.front().id==id);
    CHECK(restored.network.connectors.front().fromLaneCount==1);CHECK(restored.network.links.front().level==0);
    CHECK(restored.network.links.front().displayType=="default");CHECK(documentJson(restored)["schemaVersion"]==6);
    old["schemaVersion"]=999;test::throws([&]{parseDocument(old);},"EDIT_VERSION");
}
