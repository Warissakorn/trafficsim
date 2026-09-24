#include "test.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/project/diagnostics.hpp"
#include <algorithm>
#include <limits>
using namespace trafficsim;

namespace {
ProjectDocument roads(DrivingSide side=DrivingSide::left) {
    ProjectDocument d;d.network.drivingSide=side;
    d.network.links={{"a",{{0,0},{40,0}},{{"a1",3},{"a2",4}}},
                     {"b",{{60,0},{100,0}},{{"b1",3},{"b2",4}}}};
    return d;
}
ProjectDocument connected() {
    auto d=roads();addConnectorRange(d,{"a","a1"},{"b","b1"},2,2);return d;
}
void sameGeometry(const std::vector<Point>& a,const std::vector<Point>& b) {
    CHECK(a.size()==b.size());
    for(std::size_t i=0;i<a.size();++i) {
        test::near(a[i].x,b[i].x,1e-9);test::near(a[i].y,b[i].y,1e-9);
    }
}
bool hasIssue(const Network& n,const std::string& code,const std::string& path) {
    const auto issues=validateNetwork(n);
    return std::any_of(issues.begin(),issues.end(),[&](const auto& i){return i.code==code && i.path==path;});
}
}
TEST(authoring, imported_connector_cross_sections_cannot_bypass_commands) {
    const auto good=documentJson(connected());
    for(const auto widths:{Json::array({3}),Json::array({3,4,5})}) {
        auto j=good;j["network"]["connectors"][0]["laneWidths"]=widths;
        test::throws([&]{parseDocument(j);},"EDIT_LANES");
    }
    for(double width:{-1.,0.,std::numeric_limits<double>::infinity(),
                      std::numeric_limits<double>::quiet_NaN()}) {
        auto d=connected();d.network.connectors[0].laneWidths={3,width};
        CHECK(hasIssue(d.network,"INVALID_WIDTH","connectors[0].laneWidths[1]"));
        test::throws([&]{validateDocument(d);},"INVALID_WIDTH");
    }
    auto j=good;j["network"]["connectors"][0]["laneWidths"]={3,-1};
    test::throws([&]{parseDocument(j);},"INVALID_WIDTH");
    j=good;j["network"]["connectors"][0]["laneMarkings"]={"solid","dashed"};
    test::throws([&]{parseDocument(j);},"EDIT_LANES");
    j=good;j["network"]["connectors"][0]["laneMarkings"]={"typo"};
    test::throws([&]{parseDocument(j);},"INVALID_MARKING");
    auto d=connected();d.network.connectors[0].laneMarkings={static_cast<MarkingType>(99)};
    CHECK(hasIssue(d.network,"INVALID_MARKING","connectors[0].laneMarkings[0]"));
    // Direct model changes through History are checked too, with the entire failed edit undone.
    History h;h.reset(connected());const auto before=h.document();
    test::throws([&]{h.execute("bad",[](auto& m){m.network.connectors[0].laneWidths={-2,3};});},"INVALID_WIDTH");
    CHECK(h.document()==before);CHECK(!h.canUndo());CHECK(!h.dirty());
}
TEST(authoring, schema_eight_roundtrip_and_legacy_marking_defaults) {
    auto d=connected();
    changeLinkMarkings(d,"a",{MarkingType::none,MarkingType::doubleLine,MarkingType::dashed});
    changeConnectorLanes(d,d.network.connectors[0].id,{3,4},{MarkingType::doubleLine});
    auto j=documentJson(d);CHECK(j["schemaVersion"]==11);
    CHECK(j["network"]["links"][0]["boundaryMarkings"]==Json::array({"none","double","dashed"}));
    CHECK(parseDocument(Json::parse(j.dump()))==d);
    const auto old=connected();
    for(int version=1;version<=6;++version) {
        auto legacy=documentJson(old);legacy["schemaVersion"]=version;
        for(auto& link:legacy["network"]["links"])link.erase("boundaryMarkings");
        CHECK(parseDocument(Json::parse(legacy.dump()))==old);
    }
    for(const auto invalid:{Json(nullptr),Json("solid"),Json::array({"solid"}),Json::array({"solid",3,"solid"})}) {
        j=documentJson(d);j["network"]["links"][0]["boundaryMarkings"]=invalid;
        test::throws([&]{parseDocument(j);});
    }
}
TEST(authoring, retargeting_to_a_narrower_range_drops_stale_lane_properties) {
    auto d=connected();const auto id=d.network.connectors[0].id;
    changeConnectorLanes(d,id,{4,5},{MarkingType::doubleLine});
    History h;h.reset(d);
    h.execute("narrow",[&](auto& m){changeConnectorEndpoints(m,id,{"a","a2"},{"b","b2"});});
    const auto& c=h.document().network.connectors[0];
    CHECK(c.fromLaneCount==1 && c.toLaneCount==1);
    CHECK(c.laneWidths.empty() && c.laneMarkings.empty());validateDocument(h.document());
    h.undo();CHECK(h.document()==d);
}
TEST(authoring, schema_seven_rejects_unsupported_spec_fields_before_data_is_lost) {
    const auto good=documentJson(connected());
    for(const auto* key:{"behavior","laneChange","curve","priority","conflict","signal","routing","mouth","visual"}) {
        auto j=good;j["network"]["connectors"][0][key]=Json::object();
        test::throws([&]{parseDocument(j);},"EDIT_UNSUPPORTED_FIELD");
    }
    for(const auto* key:{"behavior","type","drivingSide","detectors","parking","crossSection","startNodeId"}) {
        auto j=good;j["network"]["links"][0][key]=Json::object();
        test::throws([&]{parseDocument(j);},"EDIT_UNSUPPORTED_FIELD");
    }
    auto j=good;j["network"]["links"][0]["geometry"][0]["z"]=3;
    test::throws([&]{parseDocument(j);},"EDIT_UNSUPPORTED_FIELD");
    j=good;j["network"]["links"][0]["lanes"][0]["type"]="bus";
    test::throws([&]{parseDocument(j);},"EDIT_UNSUPPORTED_FIELD");
    j=good;j["network"]["detectors"]=Json::array();
    test::throws([&]{parseDocument(j);},"EDIT_UNSUPPORTED_FIELD");
}
TEST(authoring, default_and_authored_markings_share_one_render_contract) {
    for(auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=connected();d.network.drivingSide=side;anchorConnectors(d);
        const auto defaults=linkMarkings(d.network.links[0],side);
        CHECK(defaults.size()==3);CHECK(defaults.front().type==MarkingType::solid);
        CHECK(defaults[1].type==MarkingType::dashed);CHECK(defaults.back().type==MarkingType::solid);
        changeLinkMarkings(d,"a",{MarkingType::none,MarkingType::doubleLine,MarkingType::dashed});
        const auto marks=linkMarkings(d.network.links[0],side);
        const auto strokes=markingStrokes(marks);CHECK(strokes.size()==3);
        CHECK(strokes[0].type==MarkingType::solid);CHECK(strokes[1].type==MarkingType::solid);
        CHECK(strokes[2].type==MarkingType::dashed);CHECK(strokes[2].edge);
        test::near(std::hypot(strokes[0].geometry[0].x-strokes[1].geometry[0].x,
                             strokes[0].geometry[0].y-strokes[1].geometry[0].y),.15);
        const auto geometry=d.network.connectors[0].geometry;
        const auto boundaries=connectorBoundaries(d.network,d.network.connectors[0]);
        changeConnectorLanes(d,d.network.connectors[0].id,{},{MarkingType::none});
        CHECK(markingStrokes(connectorMarkings(d.network,d.network.connectors[0])).size()==2);
        changeConnectorLanes(d,d.network.connectors[0].id,{},{MarkingType::doubleLine});
        CHECK(markingStrokes(connectorMarkings(d.network,d.network.connectors[0])).size()==4);
        CHECK(d.network.connectors[0].geometry==geometry);
        CHECK(connectorBoundaries(d.network,d.network.connectors[0])==boundaries);
    }
}
TEST(authoring, lane_resize_split_copy_and_opposite_preserve_markings) {
    for(bool leading:{false,true}) {
        auto d=roads();changeLinkMarkings(d,"a",{MarkingType::none,MarkingType::doubleLine,MarkingType::dashed});
        History h;h.reset(d);
        h.execute("grow",[&](auto& m){resizeLinkLanes(m,"a",4,leading);});
        const auto& grown=h.document().network.links[0].boundaryMarkings;CHECK(grown.size()==5);
        const std::size_t start=leading?2:0;
        CHECK(grown[start]==MarkingType::none);CHECK(grown[start+1]==MarkingType::doubleLine);
        CHECK(grown[start+2]==MarkingType::dashed);
        h.execute("shrink",[&](auto& m){resizeLinkLanes(m,"a",2,leading);});
        CHECK(h.document().network==d.network);h.undo();h.undo();CHECK(h.document()==d);
        const auto id=splitLink(d,"a",20,true);
        CHECK(editableLink(d,id).boundaryMarkings.size()==4);
        CHECK(editableLink(d,id).boundaryMarkings[1]==MarkingType::doubleLine);
        const auto copy=duplicateObjects(d,{"a"},{0,30}).front();
        CHECK(editableLink(d,copy).boundaryMarkings==editableLink(d,"a").boundaryMarkings);
        const auto opposite=oppositeLink(d,copy,2);
        CHECK(editableLink(d,opposite).boundaryMarkings==editableLink(d,copy).boundaryMarkings);
        CHECK(parseDocument(Json::parse(documentJson(d).dump()))==d);
    }
}
TEST(authoring, link_point_commands_are_undoable_and_keep_reference_stations) {
    auto d=connected();d.network.links[0].geometry={{0,0},{10,0},{40,0}};
    d.network.connectors[0].from.station=30;anchorConnectors(d);
    History h;h.reset(d);const auto before=h.document();
    CHECK(!h.execute("existing",[](auto& m){insertLinkPoint(m,"a",10);}));
    CHECK(!h.canUndo());CHECK(!h.dirty());
    h.execute("midpoint",[](auto& m){addLinkIntermediatePoint(m,"a");});
    CHECK(h.document().network.links[0].geometry==std::vector<Point>({{0,0},{10,0},{25,0},{40,0}}));
    CHECK(h.document().network.connectors==d.network.connectors);
    h.undo();CHECK(h.document()==before);CHECK(h.canRedo());
    for(double station:{-1.,0.,40.,41.,std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::quiet_NaN()}) {
        test::throws([&]{h.execute("invalid",[&](auto& m){insertLinkPoint(m,"a",station);});},"EDIT_LINK_STATION");
        CHECK(h.document()==before);CHECK(h.canRedo());CHECK(!h.dirty());
    }
    h.redo();h.execute("straight",[](auto& m){straightenLink(m,"a");});
    CHECK(h.document().network.links[0].geometry==std::vector<Point>({{0,0},{40,0}}));
    CHECK(h.document().network.connectors==d.network.connectors);
    h.undo();CHECK(h.document().network.links[0].geometry.size()==4);
}
TEST(authoring, reversing_unreferenced_links_keeps_named_lanes_on_the_same_road) {
    for(auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);auto& link=d.network.links[0];
        link.geometry={{0,0},{30,0},{40,10}};link.laneOffset=2.5;
        changeLinkMarkings(d,"a",{MarkingType::none,MarkingType::doubleLine,MarkingType::dashed});
        const auto original=link;History h;h.reset(d);
        h.execute("reverse",[](auto& m){reverseLink(m,"a");});
        const auto& reversed=h.document().network.links[0];
        for(const auto& lane:original.lanes) {
            auto expected=laneGeometry(original,lane.id,side);std::reverse(expected.begin(),expected.end());
            sameGeometry(laneGeometry(reversed,lane.id,side),expected);
        }
        CHECK(reversed.lanes.front().id=="a2");CHECK(reversed.laneOffset==-2.5);
        CHECK(reversed.boundaryMarkings.front()==MarkingType::dashed);
        CHECK(reversed.boundaryMarkings.back()==MarkingType::none);
        h.execute("reverse again",[](auto& m){reverseLink(m,"a");});
        CHECK(h.document().network==d.network);h.undo();h.undo();CHECK(h.document()==d);
    }
}
TEST(authoring, reversing_a_referenced_link_rejects_without_touching_history) {
    for(int kind=0;kind<2;++kind) {
        auto d=roads();
        if(kind==0)addConnector(d,{"a","a1"},{"b","b1"});
        if(kind==1) {
            const auto program=putProgram(d,{"",0,{{10,SignalColor::green}}});
            putSignalHead(d,{"",{"a","a1"},10,program,{}});
        }
        History h;h.reset(d);const auto before=h.document();
        test::throws([&]{h.execute("reverse",[](auto& m){reverseLink(m,"a");});},"EDIT_REFERENCED_LINK");
        CHECK(h.document()==before);CHECK(!h.canUndo());CHECK(!h.dirty());
    }
    // A ROUTE no longer holds the Link (M1.26): it names the Link itself, not its lanes, so
    // reversing the lane order leaves it saying the same thing.
    auto d=roads();const auto route=putRoute(d,{"",{"a"}});
    History h;h.reset(d);
    h.execute("reverse",[](auto& m){reverseLink(m,"a");});
    CHECK(h.document().definition->routes.size()==1);
    CHECK(h.document().definition->routes.front().id==route);
    CHECK(h.document().network.links.front().lanes.front().id=="a2");
}
TEST(authoring, short_connectors_are_selectable_advisories_and_still_compile) {
    auto d=roads();d.network.links[1].geometry={{44,0},{100,0}};
    const auto id=addConnector(d,{"a","a1"},{"b","b1"});
    CHECK(polylineLength(d.network.connectors[0].geometry)<5);
    const auto rows=documentDiagnostics(d);
    bool found=false;
    for(const auto& r:rows)if(r.code=="WARN_SHORT_CONNECTOR") {
        CHECK(r.severity==DiagnosticSeverity::advisory);CHECK(r.selectId==id);found=true;
    }
    CHECK(found);validateDocument(d);
    auto definition=test::straight();definition.routes.clear();definition.inputs.clear();
    CHECK(!compileScenario(d.network,definition).segments.empty());
    CHECK(parseDocument(Json::parse(documentJson(d).dump()))==d);
    d.network.links[1].geometry={{45,0},{100,0}};anchorConnectors(d);
    CHECK(polylineLength(d.network.connectors[0].geometry)>=5);
    for(const auto& issue:connectorShapeIssues(d.network))CHECK(issue.code!="WARN_SHORT_CONNECTOR");
}
