#include "test.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include <limits>
using namespace trafficsim;
namespace {
ProjectDocument roads(DrivingSide side=DrivingSide::left) {
    ProjectDocument d;d.network.drivingSide=side;
    d.network.links={
        {"a",{{0,0},{30,0},{80,10}},{{"a1",3},{"a2",4},{"a3",3.5}}},
        {"b",{{100,20},{140,20},{160,60}},{{"b1",4},{"b2",3},{"b3",3.5}}}};
    return d;
}
double at(const ProjectDocument& d,const std::string& link,double fraction) {
    for(const auto& l:d.network.links)if(l.id==link)return fraction*polylineLength(l.geometry);
    throw std::invalid_argument("UNKNOWN_LINK");
}
void same(const std::vector<Point>& a,const std::vector<Point>& b) {
    CHECK(a.size()==b.size());
    for(std::size_t i=0;i<a.size();++i){test::near(a[i].x,b[i].x,1e-9);test::near(a[i].y,b[i].y,1e-9);}
}
}
TEST(attachments, both_link_edges_preserve_existing_curved_lanes_and_references) {
    for(auto side:{DrivingSide::left,DrivingSide::right})for(bool leading:{false,true}) {
        auto d=roads(side);addConnectorRange(d,{"a","a1",at(d,"a",.4)},{"b","b1",at(d,"b",.6)},3,3);
        const auto program=putProgram(d,{"",0,{{10,SignalColor::green}}});
        putSignalHead(d,{"",{"a","a2"},20,program,{}});
        History h;h.reset(d);const auto before=d.network.links.front();
        const auto fixed=laneBoundaryGeometry(before,leading?3:0,side);
        h.execute("grow",[&](auto& m){resizeLinkLanes(m,"a",5,leading);});
        const auto after=h.document().network.links.front();
        CHECK(after.geometry==before.geometry);
        for(const auto& lane:before.lanes)same(laneGeometry(before,lane.id,side),laneGeometry(after,lane.id,side));
        same(fixed,laneBoundaryGeometry(after,leading?5:0,side));
        CHECK(h.document().network.signalHeads==d.network.signalHeads);
        same(h.document().network.connectors.front().geometry,d.network.connectors.front().geometry);
        CHECK(parseDocument(Json::parse(documentJson(h.document()).dump()))==h.document());
        h.undo();CHECK(h.document()==d);h.redo();
        h.execute("shrink",[&](auto& m){resizeLinkLanes(m,"a",3,leading);});
        CHECK(h.document().network==d.network);
        const auto stable=h.document();
        test::throws([&]{h.execute("referenced",[&](auto& m){resizeLinkLanes(m,"a",2,leading);});},"EDIT_REFERENCED_LANE");
        CHECK(h.document()==stable);
    }
}
TEST(attachments, inspector_lane_count_and_turn_pocket_keep_opposite_edge) {
    auto d=roads();const auto before=d.network.links[0];
    changeLanes(d,"a",{3,4,3.5,3.5});
    for(const auto& lane:before.lanes)same(laneGeometry(before,lane.id,DrivingSide::left),laneGeometry(d.network.links[0],lane.id,DrivingSide::left));
    d=roads();d.network.links[0].geometry={{0,0},{100,0}};resizeLinkLanes(d,"a",4,true);
    const auto old=d.network.links[0];const auto id=splitLink(d,"a",50,true);
    const auto& downstream=editableLink(d,id);
    for(std::size_t i=0;i<old.lanes.size();++i) {
        const auto a=laneGeometry(old,old.lanes[i].id,DrivingSide::left);
        const auto b=laneGeometry(downstream,downstream.lanes[i].id,DrivingSide::left);
        test::near(a.back().y,b.back().y);
    }
}
TEST(attachments, connector_leading_edges_rebase_without_moving_surviving_paths) {
    for(auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);const auto id=addConnectorRange(d,{"a","a2",at(d,"a",.4)},{"b","b2",at(d,"b",.6)},1,1);
        const auto original=d.network.connectors[0];
        const auto fixedEdge=connectorBoundaries(d.network,original).back();
        changeConnectorRange(d,id,2,2,true);
        CHECK(d.network.connectors[0].from.laneId=="a1");CHECK(d.network.connectors[0].to.laneId=="b1");
        auto paths=connectorPaths(d.network,d.network.connectors[0]);same(paths[1].geometry,original.geometry);
        same(fixedEdge,connectorBoundaries(d.network,d.network.connectors[0]).back());
        // Reopen must keep the frozen weights; otherwise the old curve drifts during derivation.
        d=parseDocument(Json::parse(documentJson(d).dump()));
        paths=connectorPaths(d.network,d.network.connectors[0]);same(paths[1].geometry,original.geometry);
        changeConnectorRange(d,id,3,3,false);same(connectorPaths(d.network,d.network.connectors[0])[1].geometry,original.geometry);
        changeConnectorRange(d,id,2,2,true);same(connectorPaths(d.network,d.network.connectors[0])[0].geometry,original.geometry);
        const auto boundaries=connectorBoundaries(d.network,d.network.connectors[0]);CHECK(boundaries.size()==3);
        CHECK(validateNetwork(d.network).empty());
        History h;h.reset(d);const auto before=h.document();
        test::throws([&]{h.execute("past edge",[&](auto& m){changeConnectorRange(m,id,4,4,true);});},"EDIT_LANE_RANGE");CHECK(h.document()==before);
    }
}
TEST(attachments, markings_are_edges_and_boundaries_for_one_two_and_three_lanes) {
    for(auto side:{DrivingSide::left,DrivingSide::right})for(int count:{1,2,3}) {
        auto d=roads(side);d.network.links[0].geometry={{0,0},{50,0}};d.network.links[1].geometry={{70,0},{120,0}};
        changeLanes(d,"a",std::vector<double>(count,3.5));changeLanes(d,"b",std::vector<double>(count,3.5));
        const auto& l=d.network.links[0];
        for(int lane=0;lane<count;++lane) {
            const auto center=laneGeometry(l,l.lanes[lane].id,side);
            const auto a=laneBoundaryGeometry(l,lane,side),b=laneBoundaryGeometry(l,lane+1,side);
            test::near(std::abs(a[0].y-center[0].y),1.75);test::near(std::abs(b[0].y-center[0].y),1.75);
        }
        addConnectorRange(d,{"a","a1"},{"b","b1"},count,count);
        const auto boundaries=connectorBoundaries(d.network,d.network.connectors[0]);
        CHECK(boundaries.size()==static_cast<std::size_t>(count+1));
        for(int boundary=0;boundary<=count;++boundary) {
            same({boundaries[boundary].front()}, {laneBoundaryGeometry(l,boundary,side).back()});
            same({boundaries[boundary].back()}, {laneBoundaryGeometry(d.network.links[1],boundary,side).front()});
        }
    }
}
TEST(attachments, copy_connector_and_head_independently_and_reject_invalid_drops_atomically) {
    auto d=roads();d.network.links[0].geometry={{0,0},{100,0}};d.network.links[1].geometry={{0,25},{100,25}};
    const auto id=addConnectorRange(d,{"a","a2",30},{"b","b2",40},2,2);
    const auto program=putProgram(d,{"",0,{{10,SignalColor::green}}});
    const auto head=putSignalHead(d,{"",{"a","a2"},10,program,{}});
    const auto connectorHead=putSignalHead(d,{"",{},5,program,id});
    History h;h.reset(d);
    h.execute("copy",[&](auto& m){duplicateObjects(m,{id,head},{20,0});});
    const auto& copy=h.document();CHECK(copy.network.links.size()==2);CHECK(copy.network.connectors.size()==2);CHECK(copy.network.signalHeads.size()==4);
    CHECK(copy.network.connectors[0]==d.network.connectors[0]);
    test::near(copy.network.connectors[1].from.station.value(),50);
    test::near(copy.network.signalHeads[2].position,30);CHECK(copy.network.signalHeads[2].programId==program);
    CHECK(copy.network.signalHeads[3].connectorId==copy.network.connectors[1].id);
    CHECK(copy.network.signalHeads[3].id!=connectorHead);
    h.undo();CHECK(h.document()==d);h.redo();const auto before=h.document();
    test::throws([&]{h.execute("off-road",[&](auto& m){duplicateObjects(m,{"a",id,head},{0,500});});},"EDIT_COPY_TARGET");
    CHECK(h.document()==before);
    h.execute("delete head",[&](auto& m){deleteObjects(m,{head});});
    CHECK(h.document().network.signalHeads.size()==3);h.undo();CHECK(h.document()==before);
}
TEST(attachments, schema_three_migrates_and_rejects_bad_offsets_and_blend_weights) {
    auto d=roads();addConnector(d,{"a","a1"},{"b","b1"});auto j=documentJson(d);j["schemaVersion"]=3;
    for(auto& l:j["network"]["links"])l.erase("laneOffset");
    for(auto& c:j["network"]["connectors"])c.erase("laneBlend");
    CHECK(parseDocument(j)==d);
    auto bad=j;bad["network"]["links"][0]["laneOffset"]=nullptr;test::throws([&]{parseDocument(bad);},"laneOffset");
    bad=j;bad["network"]["connectors"][0]["laneBlend"]={0,1};test::throws([&]{parseDocument(bad);});
    bad=j;bad["network"]["connectors"][0]["laneBlend"]=Json::array();
    for(std::size_t i=0;i<d.network.connectors[0].geometry.size();++i)bad["network"]["connectors"][0]["laneBlend"].push_back(i==0?0.:2.);
    test::throws([&]{parseDocument(bad);});
}

TEST(attachments, opposite_road_keeps_requested_gap_after_asymmetric_lane_growth) {
    for(auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);d.network.links[0].geometry={{0,0},{100,0}};
        resizeLinkLanes(d,"a",4,true);const auto old=d.network.links[0];
        const auto id=oppositeLink(d,"a",5);const auto& other=editableLink(d,id);
        const auto median=laneBoundaryGeometry(old,old.lanes.size(),side);
        const auto opposite=laneBoundaryGeometry(other,other.lanes.size(),side);
        test::near(std::abs(median.front().y-opposite.back().y),5);
    }
}
TEST(attachments, grips_stay_on_the_bundle_centreline_after_one_sided_growth) {
    for(auto side:{DrivingSide::left,DrivingSide::right})for(bool leading:{false,true}) {
        History h;h.reset(roads(side));
        h.execute("grow",[&](auto& m){resizeLinkLanes(m,"a",5,leading);});
        const auto& link=h.document().network.links.front();
        // The forcing: growing one edge moves the reference polyline off the middle of the road.
        CHECK(link.laneOffset!=0);
        const auto centre=linkCentreline(link,side);
        CHECK(centre!=link.geometry);
        const auto left=laneBoundaryGeometry(link,0,side),right=laneBoundaryGeometry(link,link.lanes.size(),side);
        for(std::size_t i=0;i<centre.size();++i) {
            test::near(centre[i].x,(left[i].x+right[i].x)/2,1e-9);
            test::near(centre[i].y,(left[i].y+right[i].y)/2,1e-9);
        }
    }
}
