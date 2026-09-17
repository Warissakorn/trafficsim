#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <numbers>
#include <fstream>
using namespace trafficsim;
namespace {
ProjectDocument roads(DrivingSide side = DrivingSide::left) {
    ProjectDocument d; d.network.drivingSide=side;
    d.network.links={
        {"in",{{-80,0},{0,0}},{{"in-1",3},{"in-2",4}}},
        {"out",{{30,30},{30,100}},{{"out-1",4},{"out-2",3}}},
        {"other",{{40,0},{100,0}},{{"other-1",3.5}}}};
    return d;
}
ProjectDocument crossing() {
    std::ifstream file(test::root()/"data/scenarios/crossing.json");Json j;file>>j;return parseDocument(j);
}
void anchored(ProjectDocument d) {
    CHECK(validateNetwork(d.network).empty());
    for (const auto& c : d.network.connectors) {
        CHECK(c.geometry.front()==laneGeometry(editableLink(d,c.from.linkId),c.from.laneId,d.network.drivingSide).back());
        CHECK(c.geometry.back()==laneGeometry(editableLink(d,c.to.linkId),c.to.laneId,d.network.drivingSide).front());
    }
}
}
TEST(connectors, create_turn_straight_and_uturn_on_both_driving_sides) {
    for (const auto side : {DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);
        const auto turn=addConnector(d,{"in","in-1"},{"out","out-2"});
        CHECK(editableConnector(d,turn).geometry.size()>2);
        const auto& g=editableConnector(d,turn).geometry;
        CHECK(g[1].x>g.front().x);CHECK(g.back().y>g[g.size()-2].y);
        addConnector(d,{"in","in-2"},{"other","other-1"});
        const auto reverse=oppositeLink(d,"in",2);
        addConnector(d,{"in","in-1"},{reverse,editableLink(d,reverse).lanes[0].id});
        anchored(d);validateDocument(d);
        const auto json=documentJson(d);
        CHECK(documentJson(parseDocument(Json::parse(json.dump())))==json);
        CHECK(json["schemaVersion"]==6);
    }
}
TEST(connectors, invalid_creation_preserves_ids_revision_savepoint_and_redo) {
    History h;h.reset(roads());std::string id;
    h.execute("create",[&](auto& d){id=addConnector(d,{"in","in-1"},{"out","out-1"});});h.markSaved();
    h.execute("straight",[&](auto& d){resetConnectorCurve(d,id,true);});h.undo();
    const auto before=documentJson(h.document());
    for (const auto& refs : std::vector<std::pair<LaneReference,LaneReference>>{
        {{"in","in-1"},{"out","out-1"}},{{"in","missing"},{"out","out-2"}},
        {{"missing","in-1"},{"out","out-2"}},{{"in","in-2"},{"in","out-2"}}}) {
        test::throws([&]{h.execute("invalid",[&](auto& d){addConnector(d,refs.first,refs.second);});});
        CHECK(documentJson(h.document())==before);CHECK(h.canRedo());CHECK(!h.dirty());
    }
    test::throws([&]{h.execute("coincident",[](auto& d){
        d.network.links.push_back({"touch",{{0,2},{10,2}},{{"touch-lane",3}}});
        addConnector(d,{"in","in-1"},{"touch","touch-lane"});
    });},"EDIT_CONNECTOR_GAP");
    CHECK(documentJson(h.document())==before);
    h.redo();CHECK(h.document().network.connectors[0].geometry.size()==2);
}
TEST(connectors, interior_edits_lock_endpoints_and_rollback_invalid_shapes) {
    auto d=roads();const auto id=addConnector(d,{"in","in-1"},{"out","out-1"});
    History h;h.reset(d);const auto before=documentJson(d);const auto original=d.network.connectors[0].geometry;
    const std::vector<Point> shape{original.front(),{10,4},{18,18},original.back()};
    h.execute("reshape",[&](auto& m){changeConnectorGeometry(m,id,shape);});
    CHECK(h.document().network.connectors[0].geometry==shape);CHECK(documentJson(d)==before);
    h.undo();CHECK(documentJson(h.document())==before);h.redo();
    const auto edited=documentJson(h.document());
    for (const auto& bad : std::vector<std::vector<Point>>{
        {},{shape.front()},{{999,0},shape.back()},{shape.front(),{NAN,0},shape.back()},
        {shape.front(),shape.front(),shape.back()},{shape.front(),{20,999}}}) {
        test::throws([&]{h.execute("bad",[&](auto& m){changeConnectorGeometry(m,id,bad);});});
        CHECK(documentJson(h.document())==edited);
    }
}
TEST(connectors, reanchor_preserves_points_and_lane_references) {
    auto d=roads();const auto id=addConnector(d,{"in","in-1"},{"out","out-2"});
    auto& c=editableConnector(d,id);c.geometry={c.geometry.front(),{10,9},{22,16},c.geometry.back()};
    const auto old=c;History h;h.reset(d);const auto before=documentJson(d);
    h.execute("move",[](auto& m){changeGeometry(m,"in",{{-80,10},{0,10}});});
    const auto moved=h.document().network.connectors[0];
    // Vissim moves the one poly point attached to the Link that moved. Every other point the
    // author placed stays exactly where it was, and the far end does not budge either.
    CHECK(moved.geometry.front()!=old.geometry.front());
    for(std::size_t i=1;i<old.geometry.size();++i) {
        test::near(moved.geometry[i].x,old.geometry[i].x,1e-12);
        test::near(moved.geometry[i].y,old.geometry[i].y,1e-12);
    }
    // And it lands on the lane it is attached to, not merely somewhere near it.
    const auto& link=*std::find_if(h.document().network.links.begin(),h.document().network.links.end(),
                                   [](const auto& l){return l.id=="in";});
    const auto lane=laneGeometry(link,"in-1",h.document().network.drivingSide);
    test::near(moved.geometry.front().x,lane.back().x,1e-12);
    test::near(moved.geometry.front().y,lane.back().y,1e-12);
    CHECK(moved.from==old.from && moved.to==old.to);CHECK(moved.geometry.size()==old.geometry.size());anchored(h.document());
    // Path independence follows for free: the point returns to where the lane puts it, and no
    // other point was ever touched, however many edits took the Link away and back.
    h.execute("away",[](auto& m){changeGeometry(m,"in",{{-90,44},{-7,-3}});});
    h.execute("back",[](auto& m){changeGeometry(m,"in",{{-80,10},{0,10}});});
    const auto returned=h.document().network.connectors[0];
    CHECK(returned.geometry.size()==moved.geometry.size());
    for(std::size_t i=0;i<returned.geometry.size();++i) {
        test::near(returned.geometry[i].x,moved.geometry[i].x);test::near(returned.geometry[i].y,moved.geometry[i].y);
    }
    h.undo();h.undo();
    h.execute("widths",[](auto& m){changeLanes(m,"out",{5,2});});anchored(h.document());
    h.execute("right",[](auto& m){changeDrivingSide(m,DrivingSide::right);});anchored(h.document());
    h.undo();h.undo();h.undo();CHECK(documentJson(h.document())==before);
}
TEST(connectors, retarget_preserves_shape_and_rejects_duplicates_or_missing_lanes) {
    auto d=roads();const auto id=addConnector(d,{"in","in-1"},{"out","out-1"});
    addConnector(d,{"in","in-2"},{"other","other-1"});
    const auto points=d.network.connectors[0].geometry.size();History h;h.reset(d);
    h.execute("retarget",[&](auto& m){changeConnectorEndpoints(m,id,{"in","in-1"},{"out","out-2"});});
    CHECK(h.document().network.connectors[0].to.laneId=="out-2");
    CHECK(h.document().network.connectors[0].geometry.size()==points);anchored(h.document());
    const auto before=documentJson(h.document());
    test::throws([&]{h.execute("duplicate",[&](auto& m){changeConnectorEndpoints(m,id,{"in","in-2"},{"other","other-1"});});},"DUPLICATE_CONNECTION");
    test::throws([&]{h.execute("missing",[&](auto& m){changeConnectorEndpoints(m,id,{"in","in-1"},{"out","missing"});});});
    CHECK(documentJson(h.document())==before);
    CHECK(!h.execute("same",[&](auto& m){changeConnectorEndpoints(m,id,{"in","in-1"},{"out","out-2"});}));
    h.undo();CHECK(documentJson(h.document())==documentJson(d));
    // Callers may pass the connector's own references when swapping directions.
    h.execute("swap",[&](auto& m){auto& c=editableConnector(m,id);changeConnectorEndpoints(m,id,c.to,c.from);});
    CHECK(h.document().network.connectors[0].from==d.network.connectors[0].to);
    CHECK(h.document().network.connectors[0].to==d.network.connectors[0].from);anchored(h.document());
}
TEST(connectors, referenced_connector_can_reshape_but_cannot_retarget) {
    auto d=crossing();History h;h.reset(d);const auto before=documentJson(d);
    test::throws([&]{h.execute("retarget",[](auto& m){changeConnectorEndpoints(m,"west-east",{"west","west-1"},{"north","north-1"});});},"EDIT_REFERENCED_CONNECTOR");
    CHECK(documentJson(h.document())==before);CHECK(!h.canUndo());
    h.execute("reshape",[](auto& m){changeConnectorGeometry(m,"west-east",{{-10,0},{0,3},{10,0}});});
    CHECK(documentJson(h.document())["definition"]==documentJson(d)["definition"]);anchored(h.document());
    h.undo();CHECK(documentJson(h.document())==before);
}
TEST(connectors, deletion_and_undo_preserve_routes_inputs_and_heads) {
    History h;h.reset(crossing());const auto before=documentJson(h.document());
    h.execute("delete",[](auto& d){deleteConnector(d,"west-east");});
    CHECK(h.document().network.links.size()==4);CHECK(h.document().network.connectors.size()==1);
    CHECK(h.document().network.signalHeads.size()==2);
    CHECK(h.document().definition->routes.size()==1);CHECK(h.document().definition->inputs.size()==1);
    CHECK(h.document().definition->routes[0].id==before["definition"]["routes"][1]["id"].get<std::string>());
    h.undo();CHECK(documentJson(h.document())==before);
    h.redo();CHECK(h.document().network.connectors.size()==1);
}
TEST(connectors, authoring_merge_does_not_enable_unsupported_runtime_merging) {
    auto d=roads();addConnector(d,{"in","in-1"},{"out","out-1"});
    addConnector(d,{"in","in-2"},{"out","out-1"});validateDocument(d);
    auto definition=test::straight();definition.routes.clear();definition.inputs.clear();
    test::throws([&]{compileScenario(d.network,definition);},"UNSUPPORTED_MERGE");
}
TEST(connectors, moving_an_end_narrows_the_range_to_the_lanes_that_are_there) {
    auto d=roads();
    // The forcing: "other" really is narrower than the two lanes this connector carries.
    CHECK(editableLink(d,"out").lanes.size()==2);CHECK(editableLink(d,"other").lanes.size()==1);
    const auto id=addConnectorRange(d,{"in","in-1"},{"out","out-1"},2,2);
    History h;h.reset(d);
    h.execute("narrow",[&](auto& m){auto c=editableConnector(m,id);changeConnectorEndpoints(m,id,c.from,{"other","other-1"});});
    const auto narrowed=h.document().network.connectors[0];
    CHECK(narrowed.to.linkId=="other");CHECK(narrowed.toLaneCount==1);CHECK(narrowed.fromLaneCount==2);
    CHECK(validateNetwork(h.document().network).empty());
    // A wider link is not an instruction to carry more lanes: the range stays where it was put.
    h.execute("back",[&](auto& m){auto c=editableConnector(m,id);changeConnectorEndpoints(m,id,c.from,{"out","out-1"});});
    CHECK(h.document().network.connectors[0].toLaneCount==1);
    const auto before=documentJson(h.document());
    test::throws([&]{h.execute("missing",[&](auto& m){auto c=editableConnector(m,id);changeConnectorEndpoints(m,id,c.from,{"out","absent"});});});
    CHECK(documentJson(h.document())==before);
    h.undo();h.undo();CHECK(documentJson(h.document())==documentJson(d));
}
TEST(connectors, grips_ride_the_middle_of_the_whole_width) {
    auto d=roads();const auto id=addConnectorRange(d,{"in","in-1"},{"out","out-1"},2,2);
    const auto& c=d.network.connectors[0];
    const auto centre=connectorCentreline(d.network,c);
    const auto boundaries=connectorBoundaries(d.network,c);
    CHECK(centre.size()==c.geometry.size());
    // The stored polyline is the first lane's path, so it is an edge of the ribbon, not its middle.
    CHECK(std::hypot(centre.front().x-c.geometry.front().x,centre.front().y-c.geometry.front().y)>1);
    for(std::size_t i=0;i<centre.size();++i) {
        test::near(centre[i].x,(boundaries.front()[i].x+boundaries.back()[i].x)/2,1e-9);
        test::near(centre[i].y,(boundaries.front()[i].y+boundaries.back()[i].y)/2,1e-9);
    }
    CHECK(editableConnector(d,id).geometry==c.geometry);
}
