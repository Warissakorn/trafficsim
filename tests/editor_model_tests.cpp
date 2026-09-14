#include "test.hpp"
#include "../src/commands/network_commands.hpp"
#include <fstream>
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
    j["schemaVersion"]=2;test::throws([&]{parseDocument(j);},"EDIT_VERSION");
    j=documentJson(d);j["background"]["metresPerPixel"]=-1;test::throws([&]{parseDocument(j);},"EDIT_BACKGROUND_INVALID");
    j=documentJson(d);j["nextId"]=-1;test::throws([&]{parseDocument(j);},"EDIT_ID_LIMIT");
}
TEST(editor, referenced_edits_reanchor_and_undo) {
    History h;h.reset(sample());const auto before=documentJson(h.document());
    h.execute("move",[](auto& d){changeGeometry(d,"west",{{-160,10},{-10,10}});});
    test::near(h.document().network.connectors[0].geometry.front().y,10);
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

TEST(editor, invalid_geometry_and_signal_split_are_atomic) {
    History h;h.reset(sample());const auto before=documentJson(h.document());
    for (const std::vector<Point>& geometry : std::vector<std::vector<Point>>{{},{{0,0}},{{0,0},{0,0}},{{0,0},{NAN,0}}}) {
        test::throws([&]{h.execute("bad geometry",[&](auto& d){changeGeometry(d,"west",geometry);});});
        CHECK(documentJson(h.document())==before);CHECK(!h.canUndo());
    }
    test::throws([&]{h.execute("split signal",[](auto& d){splitLink(d,"west",50);});},"EDIT_SPLIT_SIGNAL");
    CHECK(documentJson(h.document())==before);
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
TEST(editor, signal_bearing_link_split_is_still_rejected) {
    // M1.3.1 guard: preserving control stationing through a split needs a policy that does not
    // exist yet, so the command must refuse rather than silently move the head.
    History h;h.reset(sample());const auto before=documentJson(h.document());
    for (const auto* id : {"west","south"}) {
        test::throws([&]{h.execute("split",[&](auto& d){splitLink(d,id,50);});},"EDIT_SPLIT_SIGNAL");
        test::throws([&]{h.execute("pocket",[&](auto& d){splitLink(d,id,50,true);});},"EDIT_SPLIT_SIGNAL");
    }
    CHECK(documentJson(h.document())==before);CHECK(!h.canUndo());
}
