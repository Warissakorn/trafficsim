#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <numbers>
#include <fstream>
#include <map>
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
    // On the middle of the lane it names, AT THE STATION it names -- not at the lane's end. A
    // Connector keeps its own position, so a Link edit moves the station it sits at rather than
    // dragging it to the Link's end.
    for (const auto& c : d.network.connectors) {
        const auto a=laneAttachment(d.network,c.from,true),b=laneAttachment(d.network,c.to,false);
        test::near(c.geometry.front().x,a.x,1e-9);test::near(c.geometry.front().y,a.y,1e-9);
        test::near(c.geometry.back().x,b.x,1e-9);test::near(c.geometry.back().y,b.y,1e-9);
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
        CHECK(json["schemaVersion"]==13);
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
TEST(connectors, reshaping_rolls_back_invalid_shapes_and_a_move_off_the_link_deletes) {
    auto d=roads();const auto id=addConnector(d,{"in","in-1"},{"out","out-1"});
    History h;h.reset(d);const auto before=documentJson(d);const auto original=d.network.connectors[0].geometry;
    const std::vector<Point> shape{original.front(),{10,4},{18,18},original.back()};
    h.execute("reshape",[&](auto& m){changeConnectorGeometry(m,id,shape);});
    CHECK(h.document().network.connectors[0].geometry==shape);CHECK(documentJson(d)==before);
    h.undo();CHECK(documentJson(h.document())==before);h.redo();
    const auto edited=documentJson(h.document());
    for (const auto& bad : std::vector<std::vector<Point>>{
        {},{shape.front()},{shape.front(),{NAN,0},shape.back()},
        {shape.front(),shape.front(),shape.back()}}) {
        test::throws([&]{h.execute("bad",[&](auto& m){changeConnectorGeometry(m,id,bad);});});
        CHECK(documentJson(h.document())==edited);
    }
    // Moving an END is not an invalid shape any more, it is how a Connector is moved off its Link.
    // A Connector keeps its own position, so the drawing is the author's to put where they like --
    // and one with an end off its Link has nothing to connect, so it goes. Undo brings it back.
    for (const auto& off : std::vector<std::vector<Point>>{
        {{999,0},shape.back()},{shape.front(),{20,999}}}) {
        h.execute("off",[&](auto& m){changeConnectorGeometry(m,id,off);});
        CHECK(h.document().network.connectors.empty());
        h.undo();CHECK(documentJson(h.document())==edited);
    }
    // An end moved WITHIN its lane is not a move off it: the Connector stays, and its station
    // follows the end. The forcing is that this really did move the end, by a metre along it.
    const std::vector<Point> along{{shape.front().x-1,shape.front().y},shape[1],shape[2],shape.back()};
    h.execute("along",[&](auto& m){changeConnectorGeometry(m,id,along);});
    CHECK(h.document().network.connectors.size()==1);
    CHECK(h.document().network.connectors[0].geometry.front()!=shape.front());
    CHECK(h.document().network.connectors[0].from.station.has_value());
    h.undo();CHECK(documentJson(h.document())==edited);
}
// A Connector keeps its own position. A Link edit does not reach into it and move it: the points
// the author placed stay where they were put, and what changes is which station of the Link the
// end now sits at -- or, when the Link has moved out from under it altogether, the Connector goes.
TEST(connectors, a_link_edit_leaves_every_point_where_the_author_put_it) {
    auto d=roads();const auto id=addConnector(d,{"in","in-1"},{"out","out-2"});
    auto& c=editableConnector(d,id);c.geometry={c.geometry.front(),{10,9},{22,16},c.geometry.back()};
    const auto old=c;History h;h.reset(d);const auto before=documentJson(d);
    // A Link stretched past the end the Connector sits on: the Connector holds, every point
    // included, and the end re-reads which station of the longer Link it is now standing at.
    h.execute("stretch",[](auto& m){changeGeometry(m,"in",{{-81,0},{1,0}});});
    const auto moved=h.document().network.connectors[0];
    CHECK(moved.geometry.size()==old.geometry.size());
    for(std::size_t i=0;i<old.geometry.size();++i) {
        test::near(moved.geometry[i].x,old.geometry[i].x,1e-9);
        test::near(moved.geometry[i].y,old.geometry[i].y,1e-9);
    }
    // The forcing: the Link really did move, so holding still is a decision and not an absence of
    // one -- and the attachment really did have to move along the Link to keep the end in place.
    CHECK(h.document().network.links.front().geometry!=d.network.links.front().geometry);
    CHECK(moved.from.station.has_value());
    test::near(*moved.from.station,81,1e-9);
    CHECK(moved.from.linkId==old.from.linkId && moved.from.laneId==old.from.laneId);
    CHECK(moved.to==old.to);anchored(h.document());
    h.undo();CHECK(documentJson(h.document())==before);
    // And a Link moved out from under the end takes the Connector with it: 10 m across a 3 m lane
    // leaves nothing to connect to, so the Connector is deleted in the same transaction.
    h.execute("away",[](auto& m){changeGeometry(m,"in",{{-80,10},{0,10}});});
    CHECK(h.document().network.connectors.empty());
    h.undo();CHECK(documentJson(h.document())==before);
    h.execute("widths",[](auto& m){changeLanes(m,"out",{5,2});});anchored(h.document());
    h.execute("right",[](auto& m){changeDrivingSide(m,DrivingSide::right);});anchored(h.document());
    h.undo();h.undo();CHECK(documentJson(h.document())==before);
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
TEST(connectors, retargeting_a_routed_connector_is_allowed_and_reports_the_route_it_breaks) {
    auto d=crossing();History h;h.reset(d);const auto before=documentJson(d);
    // The forcing: this route really is carried by the Connector about to be retargeted.
    const auto& route=h.document().definition->routes.front().segmentIds;
    CHECK(std::find(route.begin(),route.end(),"west-east")!=route.end());
    CHECK(routeLaneChains(h.document().network,route).size()==1);
    // M1.26: the edit goes through. Refusing it left an author unable to correct a Connector
    // they had already routed, which is the defect this milestone exists to remove.
    h.execute("retarget",[](auto& m){changeConnectorEndpoints(m,"west-east",{"west","west-1"},{"north","north-1"});});
    CHECK(h.document().definition->routes.size()==2);
    // The route is kept and REPORTED, not deleted: nothing can travel it until the author says
    // where it goes now, and Run refuses it by name rather than running a network without it.
    const auto issues=routeRuntimeIssues(h.document().network,*h.document().definition);
    CHECK(issues.size()==1);CHECK(issues.front().code=="UNSUPPORTED_ROUTE_TOPOLOGY");
    test::throws([&]{compileDocument(h.document(),test::root()/"data");},"UNSUPPORTED_ROUTE_TOPOLOGY");
    h.undo();CHECK(documentJson(h.document())==before);
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
// Two Connectors meeting at a lane's start. Before M2.0.1 (D35) that was refused outright as
// UNSUPPORTED_MERGE; now M3.1's derived rule orders them by drawing order -- but only with the
// numbers from data/priority-rules/. A merge never runs unarbitrated.
TEST(connectors, a_merge_at_a_lane_start_runs_only_arbitrated) {
    auto d=roads();const auto first=addConnector(d,{"in","in-1"},{"out","out-1"});
    const auto second=addConnector(d,{"in","in-2"},{"out","out-1"});validateDocument(d);
    auto definition=test::straight();definition.routes.clear();definition.inputs.clear();
    // Without the defaults a derived rule would have a zero gap time: refused, naming the yielder.
    test::throws([&]{compileScenario(d.network,definition);},"EDIT_NO_PRIORITY_DEFAULTS: connectors[1]");
    definition.priorityDefaults={2,10};
    auto scenario=compileScenario(d.network,definition);
    CHECK(scenario.priorityRules.size()==1);
    CHECK(scenario.priorityRules.front().yieldSegmentId.rfind(second,0)==0);
    CHECK(scenario.priorityRules.front().conflictSegmentId.rfind(first,0)==0);
    // The guard was narrowed by construction, never removed: take the rule away and it fires.
    scenario.priorityRules.clear();
    const auto issues=validateScenario(scenario);
    CHECK(std::any_of(issues.begin(),issues.end(),[](const auto& i){return i.code=="UNSUPPORTED_MERGE";}));
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
// Two Connectors arriving at the SAME station of one lane (CONNECTOR_PARITY_AUDIT.md §3.3).
//
// This used to be refused: runtimeSections measured the second cut against the boundary the first
// had just made at that very station -- against itself -- and Run was blocked for a pair nothing
// is physically wrong with. Now the second arrival reuses the first cut, both paths join the same
// section, and the merge is arbitrated in a strict order: the lane first, then the Connector drawn
// first, then the one drawn second. Without the last rule the two arrivals would each give way to
// the lane and never to each other, which is the second half §3.3 described.
TEST(connectors, two_connectors_arriving_at_one_station_share_one_cut) {
    auto d=roads();
    const auto first=addConnector(d,{"in","in-1"},{"out","out-1",35});
    // The forcing: ONE interior arrival runs, so what follows is about the second one.
    CHECK(connectorRuntimeIssues(d.network).empty());
    CHECK(runtimeSections(d.network).unsectionable.empty());
    const auto second=addConnector(d,{"in","in-2"},{"out","out-1",35});
    CHECK(d.network.connectors.size()==2);
    // Both really do arrive inside the BODY of out-1, at the same drawn metre, from different lanes.
    for(const auto& c:d.network.connectors) {
        CHECK(!attachedAtLinkEnd(d.network,c.to,false));
        CHECK(c.to.laneId=="out-1");CHECK(c.to.station.has_value());
        test::near(*c.to.station,35,1e-9);
    }
    CHECK(d.network.connectors[0].from.laneId!=d.network.connectors[1].from.laneId);
    const auto viaA=putRoute(d,{"viaA",{"in-1",first,"out-1"}});
    const auto viaB=putRoute(d,{"viaB",{"in-2",second,"out-1"}});
    putInput(d,{"",viaA,"car",600,0,60,{}});putInput(d,{"",viaB,"car",600,0,60,{}});
    validateDocument(d);
    const auto table=runtimeSections(d.network);
    CHECK(table.unsectionable.empty());
    // ONE cut at 35, not two: out-1 is two sections, and both paths join the one downstream.
    CHECK(std::count_if(table.sections.begin(),table.sections.end(),
                        [](const auto& s){return s.laneId=="out-1";})==2);
    const auto& joined=sectionStartingAt(table,"out-1",35);
    test::near(joined.start,35,1e-9);
    CHECK(table.pathNext.size()==2);
    CHECK(table.pathNext[0]==joined.id);CHECK(table.pathNext[1]==joined.id);
    CHECK(connectorRuntimeIssues(d.network).empty());
    // Three rules: each path gives way to the lane, and the second also to the first.
    const auto rules=derivedPriorityRules(table,{2,10});
    CHECK(rules.size()==3);
    const auto gives=[&](const std::string& minor,const std::string& major) {
        return std::any_of(rules.begin(),rules.end(),[&](const auto& r){
            return r.yieldSegmentId==minor&&r.conflictSegmentId==major;});
    };
    CHECK(gives(table.paths[0].id,"out-1"));CHECK(gives(table.paths[1].id,"out-1"));
    CHECK(gives(table.paths[1].id,table.paths[0].id));
    CHECK(!gives(table.paths[0].id,table.paths[1].id)); // A strict order, never a cycle.
    // And it runs: both routes deliver, and replay is exact.
    const auto snapshot=compileDocument(d,test::root()/"data");
    std::map<std::string,int> arrived;std::vector<ArrivedEvent> trips,again;
    runSimulation(snapshot.scenario,42,[&](const SimEvent& e){
        if(const auto* a=std::get_if<ArrivedEvent>(&e)){arrived[a->routeId]++;trips.push_back(*a);}},false);
    CHECK(arrived[viaA]>0);CHECK(arrived[viaB]>0);
    runSimulation(snapshot.scenario,42,[&](const SimEvent& e){
        if(const auto* a=std::get_if<ArrivedEvent>(&e))again.push_back(*a);},false);
    CHECK(trips==again);
}
// One 0.1 m off is a different drawn metre, not the same one, and the cut it would need is still
// too close to the first: that stays refused, so the reuse above cannot swallow it.
TEST(connectors, an_arrival_just_beside_an_existing_cut_is_still_refused) {
    auto d=roads();
    addConnector(d,{"in","in-1"},{"out","out-1",35});
    const auto second=addConnector(d,{"in","in-2"},{"out","out-1",35.1});
    // The forcing: the two stations really differ on the lane, by less than a section.
    CHECK(std::abs(*d.network.connectors[1].to.station-*d.network.connectors[0].to.station)>1e-6);
    CHECK(std::abs(*d.network.connectors[1].to.station-*d.network.connectors[0].to.station)<kMinSectionLength);
    CHECK(runtimeSections(d.network).unsectionable==std::vector<std::string>({second}));
}
