#include "test.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include <fstream>
#include <cmath>
#include <limits>
using namespace trafficsim;
namespace {
ProjectDocument sample() {
    std::ifstream file(test::root()/"data/scenarios/crossing.json");Json j;file>>j;return parseDocument(j);
}
}
TEST(editor, history_saved_branch_and_atomic_failure) {
    History h;h.reset();const auto empty=documentJson(h.document());
    std::string id;
    h.execute("draw",[&](auto& d){id=addLink(d,{{0,0},{100,0}},2,3.5);});
    CHECK(h.dirty());h.markSaved();const auto saved=documentJson(h.document());const auto revision=h.revision();
    h.execute("move",[&](auto& d){changeGeometry(d,id,{{0,0},{100,10}});});
    h.undo();CHECK(!h.dirty());CHECK(h.revision()==revision);CHECK(documentJson(h.document())==saved);
    h.redo();CHECK(h.dirty());h.undo();
    test::throws([&]{h.execute("bad",[&](auto& d){changeLanes(d,id,{-1});});});
    CHECK(!h.dirty());CHECK(h.canRedo());CHECK(documentJson(h.document())==saved);
    h.execute("branch",[&](auto& d){changeLanes(d,id,{3,4,3});});CHECK(!h.canRedo());
    h.undo();h.undo();CHECK(documentJson(h.document())==empty);
}
TEST(editor, document_roundtrip_and_version_guards) {
    auto d=sample();d.background.x=12.75;d.background.rotation=32;d.background.metresPerPixel=0.07;
    auto j=documentJson(d);CHECK(documentJson(parseDocument(Json::parse(j.dump())))==j);
    CHECK(documentJson(parseDocument(documentJson(ProjectDocument{})))==documentJson(ProjectDocument{}));
    j["schemaVersion"]=999;test::throws([&]{parseDocument(j);},"EDIT_VERSION");
    j=documentJson(d);j["background"]["metresPerPixel"]=-1;test::throws([&]{parseDocument(j);},"EDIT_BACKGROUND_INVALID");
    j=documentJson(d);j["nextId"]=-1;test::throws([&]{parseDocument(j);},"EDIT_ID_LIMIT");
}
TEST(editor, referenced_edits_keep_the_connector_in_place_and_undo) {
    History h;h.reset(sample());const auto before=documentJson(h.document());
    const auto placed=h.document().network.connectors;
    h.execute("move",[](auto& d){changeGeometry(d,"west",{{-160,10},{-10,10}});});
    // A Connector keeps its own position: the Link moved 10 m across lanes 3.5 m wide, and the
    // Connectors that hung off it had their ends left in mid-air. They go, with the routes that
    // named them, in the same transaction. They used to be dragged to y=10 with the Link, which
    // moved a shape the author had placed by hand.
    CHECK(!placed.empty());
    CHECK(h.document().network.connectors.size()<placed.size());
    h.undo();CHECK(documentJson(h.document())==before);
    h.execute("delete",[](auto& d){deleteLink(d,"west");});
    CHECK(h.document().network.links.size()==3);CHECK(h.document().network.signalHeads.size()==1);
    CHECK(h.document().network.signalHeads.front().id=="south-head");
    CHECK(h.document().network.connectors.size()==1);CHECK(h.document().definition->routes.size()==1);
    CHECK(h.document().definition->inputs.size()==1);h.undo();CHECK(documentJson(h.document())==before);
}
TEST(editor, split_remaps_routes_and_preserves_old_snapshot) {
    History h;h.reset(sample());const auto old=documentJson(h.document());std::string downstream;
    h.execute("split",[&](auto& d){downstream=splitLink(d,"east",70);});
    CHECK(h.document().network.links.size()==5);CHECK(h.document().network.connectors.size()==3);
    const auto& ids=h.document().definition->routes[0].segmentIds;
    CHECK(ids.size()==5);CHECK(h.document().network.connectors[0].to.linkId=="east");
    const auto& n=h.document().network;CHECK(validateNetwork(n).empty());
    h.undo();CHECK(documentJson(h.document())==old);h.redo();CHECK(h.document().network.links.back().id==downstream);
}
TEST(editor, turn_pocket_and_opposite_both_driving_sides) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset();std::string road,opposite;
        h.execute("draw",[&](auto& d){d.network.drivingSide=side;road=addLink(d,{{0,0},{100,0}},2,3.5);});
        h.execute("opposite",[&](auto& d){opposite=oppositeLink(d,road,2);});
        const auto& reverse=h.document().network.links.back();
        test::near(reverse.geometry.front().x,100);test::near(reverse.geometry.front().y,side==DrivingSide::left?-9:9);
        h.execute("pocket",[&](auto& d){splitLink(d,road,60,true);});
        CHECK(h.document().network.links.back().lanes.size()==3);
        CHECK(h.document().network.connectors.size()==2);CHECK(validateNetwork(h.document().network).empty());
        CHECK(!h.document().definition.has_value());
    }
}
TEST(editor, lane_removal_does_not_dangle_references) {
    History h;auto d=sample();
    changeLanes(d,"west",{3.5,3.5});
    d.network.signalHeads.push_back({"extra",{"west",d.network.links[0].lanes.back().id},10,"east-west-program"});
    h.reset(d);const auto before=documentJson(d);
    test::throws([&]{h.execute("remove",[](auto& model){changeLanes(model,"west",{3.5});});},"EDIT_REFERENCED_LANE");
    CHECK(documentJson(h.document())==before);CHECK(!h.canUndo());
}
TEST(editor, ids_skip_imported_collisions_and_image_is_shared) {
    ProjectDocument d;addLink(d,{{0,0},{100,0}},1,3.5);d.nextId=1;
    const auto id=addLink(d,{{0,10},{100,10}},1,3.5);CHECK(id!="link-1");validateDocument(d);
    d.background.pngBase64=std::make_shared<const std::string>(100000,'a');
    History h;h.reset(d);h.execute("move",[&](auto& m){changeGeometry(m,id,{{0,10},{100,20}});});
    CHECK(h.document().background.pngBase64==d.background.pngBase64);h.undo();CHECK(h.document().background.pngBase64==d.background.pngBase64);
}

TEST(editor, invalid_geometry_is_atomic) {
    History h;h.reset(sample());const auto before=documentJson(h.document());
    for (const std::vector<Point>& geometry : std::vector<std::vector<Point>>{{},{{0,0}},{{0,0},{0,0}},{{0,0},{NAN,0}}}) {
        test::throws([&]{h.execute("bad geometry",[&](auto& d){changeGeometry(d,"west",geometry);});});
        CHECK(documentJson(h.document())==before);CHECK(!h.canUndo());
    }
}

TEST(editor, delete_objects_is_one_undoable_transaction) {
    History h;h.reset(sample());const auto before=documentJson(h.document());const auto revision=h.revision();
    CHECK(h.execute("delete selection",[](auto& d){deleteObjects(d,{"west","south-north"});}));
    CHECK(h.revision()!=revision);
    const auto& n=h.document().network;
    CHECK(n.links.size()==3);                                  // west removed, the other three remain
    CHECK(n.connectors.empty());                               // west-east cascaded, south-north named
    CHECK(n.signalHeads.size()==1);                            // west-head cascaded with its link
    CHECK(h.document().definition->routes.empty());       // both routes used a removed segment
    CHECK(h.document().definition->inputs.empty());
    h.undo();
    // One Undo, everything back: links, connectors, heads, routes and their vehicle inputs.
    CHECK(documentJson(h.document())==before);CHECK(!h.canUndo());
}
TEST(editor, delete_objects_skips_cascaded_ids_and_rejects_unknown) {
    History h;h.reset(sample());const auto before=documentJson(h.document());
    // "west-east" belongs to "west" and is already gone by the time its turn comes.
    CHECK(h.execute("delete selection",[](auto& d){deleteObjects(d,{"west-east","west"});}));
    CHECK(h.document().network.connectors.size()==1);
    h.undo();CHECK(documentJson(h.document())==before);
    for (const auto& ids : std::vector<std::vector<std::string>>{{"ghost"},{"west","ghost"},{"west-1"}}) {
        test::throws([&]{h.execute("delete selection",[&](auto& d){deleteObjects(d,ids);});},"EDIT_UNKNOWN_OBJECT");
        CHECK(documentJson(h.document())==before);CHECK(!h.canUndo());
    }
}
TEST(editor, signal_bearing_split_preserves_control_and_routes) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const bool pocket:{false,true}) {
        History h;auto d=sample();d.network.drivingSide=side;
        d.network.signalHeads={{"up",{"west","west-1"},20,"east-west-program"},
            {"span",{"west","west-1"},50,"east-west-program"},
            {"down",{"west","west-1"},100,"east-west-program"}};
        Link west;for(const auto& l:d.network.links)if(l.id=="west")west=l;
        const auto spanWorld=pointAlong(laneGeometry(west,"west-1",side),50);
        h.reset(d);const auto before=documentJson(h.document());
        std::string downstream;h.execute("split",[&](auto& doc){downstream=splitLink(doc,"west",50,pocket);});
        const auto& n=h.document().network;
        CHECK(n.signalHeads[0].lane.linkId=="west");test::near(n.signalHeads[0].position,20);
        CHECK(!n.signalHeads[1].connectorId.empty());CHECK(n.signalHeads[1].lane.laneId.empty());
        CHECK(n.signalHeads[2].lane.linkId==downstream);
        // Stations are re-projected onto the new owner, never carried over: the downstream
        // link now starts 50.1 along the original, so the head authored at 100 sits at 49.9.
        test::near(n.signalHeads[2].position,49.9);
        // Without a pocket no lane moves, so the spanning head must keep its exact world
        // point on the 0.2 m gap connector. A pocket widens the downstream link and shifts
        // its lanes, so there only the station is preserved.
        if(!pocket) {
            test::near(n.signalHeads[1].position,0.1);
            std::vector<Point> span;
            for(const auto& c:n.connectors)if(c.id==n.signalHeads[1].connectorId)span=c.geometry;
            const auto at=pointAlong(span,n.signalHeads[1].position);
            test::near(at.x,spanWorld.x);test::near(at.y,spanWorld.y);
        }
        CHECK(validateNetwork(n).empty());
        const auto& ids=h.document().definition->routes.front().segmentIds;
        CHECK(ids.size()==5);CHECK(ids[1]==n.signalHeads[1].connectorId);
        for(const auto& head:n.signalHeads)CHECK(head.programId=="east-west-program");
        CHECK(documentJson(parseDocument(Json::parse(documentJson(h.document()).dump())))==documentJson(h.document()));
        h.undo();CHECK(documentJson(h.document())==before);h.redo();CHECK(h.document().network.links.size()==5);
    }
}
TEST(editor, a_name_belongs_to_the_object_and_outlives_save_undo_and_copy) {
    History h;h.reset();std::string link,other,connector,head;
    h.execute("draw",[&](auto& d){
        link=addLink(d,{{0,0},{100,0}},2,3.5);other=addLink(d,{{130,0},{230,0}},2,3.5);
        connector=addConnector(d,{link,d.network.links[0].lanes[0].id},{other,d.network.links[1].lanes[0].id});
        head=putSignalHead(d,{"",{link,d.network.links[0].lanes[0].id},50,putProgram(d,{"",0,{{10,SignalColor::green}}}),{}});
    });
    // The forcing: nothing carries a name until one is typed, so an empty name below is a
    // rename that happened, not a default that was never touched.
    const auto& n=h.document().network;
    CHECK(n.links[0].name.empty());CHECK(n.connectors[0].name.empty());CHECK(n.signalHeads[0].name.empty());
    const auto unnamed=h.document();
    h.execute("name",[&](auto& d){
        renameObject(d,link,"Sukhumvit inbound");renameObject(d,connector,"NB left turn");renameObject(d,head,"Head A1");
    });
    CHECK(n.links[0].name=="Sukhumvit inbound");CHECK(n.connectors[0].name=="NB left turn");CHECK(n.signalHeads[0].name=="Head A1");
    // A name is not a key: the other link may carry the same one, and neither id moves.
    h.execute("same",[&](auto& d){renameObject(d,other,"Sukhumvit inbound");});
    CHECK(n.links[1].name==n.links[0].name);CHECK(n.links[1].id!=n.links[0].id);
    CHECK(parseDocument(Json::parse(documentJson(h.document()).dump()))==h.document());
    const auto named=h.document();
    h.execute("copy",[&](auto& d){duplicateObjects(d,{link},{0,60});});
    CHECK(h.document().network.links.back().name=="Sukhumvit inbound");
    h.undo();CHECK(h.document()==named);
    h.undo();h.undo();CHECK(h.document()==unnamed);h.redo();h.redo();CHECK(h.document()==named);
    test::throws([&]{h.execute("unknown",[&](auto& d){renameObject(d,"link-nope","x");});},"EDIT_UNKNOWN_OBJECT");
    test::throws([&]{h.execute("long",[&](auto& d){renameObject(d,link,std::string(201,'x'));});},"EDIT_NAME_LENGTH");
    CHECK(h.document()==named);
    h.execute("clear",[&](auto& d){renameObject(d,link,std::string(200,'x'));renameObject(d,connector,"");});
    CHECK(n.links[0].name.size()==200);CHECK(n.connectors[0].name.empty());
}
TEST(editor, a_group_move_carries_a_whole_junction_rigidly) {
    History h;h.reset();std::string a,b,connector,head;
    h.execute("draw",[&](auto& d){
        a=addLink(d,{{0,0},{100,0}},2,3.5);b=addLink(d,{{130,0},{230,0}},2,3.5);
        connector=addConnector(d,{a,d.network.links[0].lanes[0].id},{b,d.network.links[1].lanes[0].id});
        head=putSignalHead(d,{"",{a,d.network.links[0].lanes[0].id},50,putProgram(d,{"",0,{{10,SignalColor::green}}}),{}});
    });
    const auto& n=h.document().network;
    // The forcing: the Connector has points of its own between its two attachments, so a move
    // that only reanchored the ends would visibly leave them behind.
    CHECK(n.connectors[0].geometry.size()>2);
    const auto before=h.document();const Point delta{25,-40};
    const auto shifted=[&](const std::vector<Point>& was,const std::vector<Point>& now) {
        CHECK(was.size()==now.size());
        for(std::size_t i=0;i<was.size();++i){test::near(now[i].x,was[i].x+delta.x,1e-9);test::near(now[i].y,was[i].y+delta.y,1e-9);}
    };
    h.execute("move",[&](auto& d){translateObjects(d,{a,b,head},delta);});
    shifted(before.network.links[0].geometry,n.links[0].geometry);
    shifted(before.network.links[1].geometry,n.links[1].geometry);
    // Both ends moved, so the junction moved: every point the author placed keeps its place
    // inside it, rather than the ends being dragged onto a shape that stayed behind.
    shifted(before.network.connectors[0].geometry,n.connectors[0].geometry);
    // A head rides a station, so it needs no moving and must not have been moved.
    CHECK(n.signalHeads[0].position==before.network.signalHeads[0].position);
    CHECK(validateNetwork(n).empty());
    CHECK(parseDocument(Json::parse(documentJson(h.document()).dump()))==h.document());
    h.undo();CHECK(h.document()==before);h.redo();
    const auto together=h.document();
    // One end moving is not the junction moving: the Connector keeps its own position, so the
    // Link slides out from under its end. Ten metres across a lane leaves nothing to connect to,
    // and a Connector with an end off its Link goes -- in the same transaction, so one Undo
    // brings back the Link edit and the Connector together.
    h.execute("one end",[&](auto& d){translateObjects(d,{a},{0,10});});
    CHECK(n.connectors.empty());
    h.undo();CHECK(h.document()==together);
    // A Connector picked out on its own is moved because the author said so -- and may be moved
    // right off its Links, at which point it goes the same way.
    h.execute("connector",[&](auto& d){translateObjects(d,{connector},{0,40});});
    CHECK(n.connectors.empty());
    h.undo();CHECK(h.document()==together);
    // A head rides a station and carries no geometry, so a selection of nothing else is a gesture
    // with no meaning rather than a move of zero objects.
    test::throws([&]{h.execute("no geometry",[&](auto& d){translateObjects(d,{head},delta);});},"EDIT_MOVE_TARGET");
    CHECK(h.document()==together);
    test::throws([&]{h.execute("nan",[&](auto& d){translateObjects(d,{a},{std::nan(""),0});});},"INVALID_GEOMETRY");
    CHECK(h.document()==together);
}
TEST(editor, route_continuations_are_the_rule_the_dialog_applied) {
    const auto d=sample();const auto& n=d.network;
    // The forcing: this network really does have the two-Connector crossing the rest relies on.
    CHECK(n.links.size()==4);CHECK(n.connectors.size()==2);
    const auto starts=routeContinuations(n,{});
    // Every lane and every Connector path may start a route; nothing else may.
    CHECK(starts.size()==n.links.size()+n.connectors.size());
    for(const auto& l:n.links)CHECK(std::find(starts.begin(),starts.end(),l.lanes.front().id)!=starts.end());
    const auto afterWest=routeContinuations(n,{"west-1"});
    CHECK(afterWest==std::vector<std::string>({"west-east"}));
    CHECK(routeContinuations(n,{"west-1","west-east"})==std::vector<std::string>({"east-1"}));
    // The end of the road leads nowhere, and a segment already in the route is never offered
    // again -- a route that revisited one would loop.
    CHECK(routeContinuations(n,{"west-1","west-east","east-1"}).empty());
    CHECK(std::find(afterWest.begin(),afterWest.end(),"west-1")==afterWest.end());
}
TEST(editor, route_chain_to_a_clicked_destination_or_nothing) {
    const auto d=sample();const auto& n=d.network;
    // One click on the far side of the junction authors the whole crossing.
    CHECK(routeChainTo(n,{"west-1"},"east-1")==std::vector<std::string>({"west-east","east-1"}));
    // From nothing, the chain includes the segment clicked itself.
    CHECK(routeChainTo(n,{},"west-east")==std::vector<std::string>({"west-east"}));
    // The two arms never meet, so there is no chain -- and the click must be refused rather
    // than resolved to the nearest thing that does connect.
    CHECK(routeChainTo(n,{"west-1"},"north-1").empty());
    CHECK(routeChainTo(n,{"west-1"},"no-such-lane").empty());
    CHECK(routeChainTo(n,{"west-1"},"").empty());
    // A target already in the route is not a destination: it would close a loop.
    CHECK(routeChainTo(n,{"west-1","west-east","east-1"},"west-1").empty());
}
TEST(editor, route_geometry_draws_the_compiled_length) {
    const auto d=sample();const auto& n=d.network;
    const std::vector<std::string> route{"west-1","west-east","east-1"};
    const auto drawn=routeGeometry(n,route);
    CHECK(drawn.size()>=3);
    double parts=0;
    const auto table=runtimeSections(n);
    for(const auto& id:expandRouteSegments(table,route)) {
        for(const auto& s:table.sections)if(s.id==id)parts+=s.end-s.start;
        for(const auto& p:table.paths)if(p.id==id)parts+=polylineLength(p.geometry);
    }
    // The drawing follows the road the route travels, so it is as long as the compiled chain.
    test::near(polylineLength(drawn),parts,1e-6);
    CHECK(routeGeometry(n,{"no-such-lane"}).empty());
    // Both driving sides draw: lane geometry is offset from the reference line either way.
    auto mirrored=d.network;mirrored.drivingSide=DrivingSide::right;
    const auto other=routeGeometry(mirrored,route);
    CHECK(other.size()==drawn.size());
    test::near(polylineLength(other),polylineLength(drawn),1e-6);
}
