#include "test.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include <cmath>
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
const Connector& connector(const ProjectDocument& d,const std::string& id) {
    for(const auto& c:d.network.connectors)if(c.id==id)return c;
    throw std::invalid_argument("EDIT_UNKNOWN_CONNECTOR");
}
double at(const ProjectDocument& d,const std::string& link,double fraction) {
    for(const auto& l:d.network.links)if(l.id==link)return fraction*polylineLength(l.geometry);
    throw std::invalid_argument("UNKNOWN_LINK");
}
void same(const std::vector<Point>& a,const std::vector<Point>& b) {
    CHECK(a.size()==b.size());
    for(std::size_t i=0;i<a.size();++i){test::near(a[i].x,b[i].x,1e-9);test::near(a[i].y,b[i].y,1e-9);}
}
// The carriageway between two boundaries at one sample, measured square to the road. Straight
// hypot is the same thing everywhere except at the two mouths, which are cut on the Link's
// cross-section (M1.18) and so read 1/cos(arrival) wide ALONG the cut -- by construction, not by
// error. Projecting onto the ribbon's own normal is the measure that can actually fail.
double across(const std::vector<Point>& a,const std::vector<Point>& b,std::size_t j) {
    const std::size_t k=j+1<a.size()?j:j-1;
    const Point along{a[k+1].x-a[k].x,a[k+1].y-a[k].y};
    const double length=std::hypot(along.x,along.y);
    if(length<1e-12)return std::hypot(b[j].x-a[j].x,b[j].y-a[j].y);
    return std::abs((b[j].x-a[j].x)*(-along.y/length)+(b[j].y-a[j].y)*(along.x/length));
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
// M1.12.1. A Connector's lane widths were derived from the Links each end joins, so a widening
// taper had to be authored on the links instead. Now the Connector can carry its own.
TEST(attachments, a_connector_carries_its_own_lane_widths) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},2,2);
        // The forcing: with nothing authored, the widths really do come from the Links, and the
        // two ends really do differ -- so an authored width has something to override.
        const auto derived=connectorLaneWidths(d.network,connector(d,id));
        test::near(derived.source[0],3,1e-9);test::near(derived.target[0],4,1e-9);
        const auto before=connectorBoundaries(d.network,connector(d,id));
        History h;h.reset(d);
        h.execute("widths",[&](auto& m){changeConnectorLanes(m,id,{5.5,5.5},{});});
        const auto authored=connectorLaneWidths(h.document().network,connector(h.document(),id));
        test::near(authored.source[0],5.5,1e-9);test::near(authored.target[0],5.5,1e-9);
        test::near(authored.source[1],5.5,1e-9);
        // It reaches the drawing, and EXACTLY -- measured square to the road rather than along
        // the cross-section. Along it this reads 5.529 m, which is the mitered corner's diagonal
        // (width/cos(phi/2)) and not a width error: see M1.12.2, where the reported 24% bulge was
        // measured that way and turned out to be exactly this. Square to the road it is 5.5 m.
        const auto after=connectorBoundaries(h.document().network,connector(h.document(),id));
        // The BODY only. Each mouth is now built from the Link's own lane boundaries, so an
        // authored width does not reach it and a sample inside a mouth's transition zone is
        // part-way between the two (5.473 m at the sample next to the mouth here). That is
        // M1.12.3: a lane laid at a width the Link does not have cannot also have its middle and
        // its edges on the Link's lane. The width the author typed is the width of the road it
        // owns, and the Link's is the width where the two roads meet.
        std::size_t measured=0;
        for(std::size_t i=after[1].size()/2;i<=after[1].size()/2;++i) {
            const double dx=after[1][i].x-after[1][i-1].x,dy=after[1][i].y-after[1][i-1].y;
            const double length=std::hypot(dx,dy);
            if(length<=0)continue;
            const Point across{-dy/length,dx/length};
            {
                const std::size_t j=i;
                // 5e-4: the offset each boundary leaves a mouth at is a fixed point solved
                // against the end leg it itself produces, and 0.17 mm of that solve is still
                // showing at the middle of a Connector this short. `an_authored_width_is_exact_
                // where_the_connector_is_straight` carries the exact case.
                test::near(std::abs((after[1][j].x-after[0][j].x)*across.x+
                                    (after[1][j].y-after[0][j].y)*across.y),5.5,5e-4);
                ++measured;
            }
        }
        CHECK(measured>0); // The loop above really ran, rather than skipping every leg.
        // One undo entry, and it restores the derived cross-section exactly.
        h.undo();
        const auto restored=connectorBoundaries(h.document().network,connector(h.document(),id));
        CHECK(restored.size()==before.size());
        for(std::size_t i=0;i<restored.size();++i)for(std::size_t j=0;j<restored[i].size();++j) {
            test::near(restored[i][j].x,before[i][j].x,1e-12);
            test::near(restored[i][j].y,before[i][j].y,1e-12);
        }
    }
}
// The exact width, on a Connector with no bend in it, so the miter (M1.12.2) is not in the way --
// and the exact statement of which width rules where: the author's through the body, the Link's at
// each mouth. Both Links carry the same lane widths here, so the Connector's lane middles are
// collinear and its default curve is exactly straight.
TEST(attachments, an_authored_width_is_exact_where_the_connector_is_straight) {
    ProjectDocument d;
    d.network.links={{"a",{{0,0},{100,0}},{{"a1",3},{"a2",3}}},
                     {"b",{{180,0},{280,0}},{{"b1",3},{"b2",3}}}};
    const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},2,2);
    History h;h.reset(d);
    // The forcing: the Connector really is straight, so every cross-section is square to it and
    // the miter contributes nothing. Without this the equality below would be measuring luck.
    const auto& geometry=connector(h.document(),id).geometry;
    CHECK(geometry.size()>2);   // ... and it really has a body between its two mouths to measure
    for(std::size_t j=1;j+1<geometry.size();++j) {
        const double cross=(geometry[j].x-geometry[j-1].x)*(geometry[j+1].y-geometry[j].y)-
                           (geometry[j].y-geometry[j-1].y)*(geometry[j+1].x-geometry[j].x);
        test::near(cross,0,1e-9);
    }
    h.execute("widths",[&](auto& m){changeConnectorLanes(m,id,{5.5,6.25},{});});
    const auto b=connectorBoundaries(h.document().network,connector(h.document(),id));
    // Through the body, square to the road, the authored widths are exact.
    const std::size_t middle=b[0].size()/2;
    test::near(across(b[0],b[1],middle),5.5,1e-9);
    test::near(across(b[1],b[2],middle),6.25,1e-9);
    // At each mouth it is the LINK's 3 m lane, exactly, because that is where the two roads meet
    // and a lane laid at 5.5 m cannot both start on the Link's lane middle and end on its edges
    // (M1.12.3). An author who wants 5.5 m at the joint widens the Link's lane.
    // Measured ALONG the cut -- the separation of the two edges at the same index -- because that
    // is what the mouth is: one straight line across the road, on the Link's own cross-section.
    const auto apart=[](const std::vector<Point>& x,const std::vector<Point>& y,std::size_t j) {
        return std::hypot(x[j].x-y[j].x,x[j].y-y[j].y);
    };
    for(const std::size_t j:{std::size_t{0},b[0].size()-1}) {
        test::near(apart(b[0],b[1],j),3,1e-9);
        test::near(apart(b[1],b[2],j),3,1e-9);
    }
}
// The no-regression assertion this milestone turns on: a Connector that was never given a width
// must draw exactly what it drew before the field existed.
TEST(attachments, a_connector_without_authored_widths_is_unchanged) {
    auto d=roads();const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},3,3);
    // The forcing: the field exists and is empty, which is the state every older file loads in.
    CHECK(connector(d,id).laneWidths.empty());CHECK(connector(d,id).laneMarkings.empty());
    const auto boundaries=connectorBoundaries(d.network,connector(d,id));
    const auto markings=connectorMarkings(d.network,connector(d,id));
    // Interior dividers dashed, outer edges solid -- the derived rule, untouched.
    CHECK(markings.front().edge);CHECK(markings.front().type==MarkingType::solid);
    CHECK(markings.back().edge);CHECK(markings.back().type==MarkingType::solid);
    for(std::size_t i=1;i+1<markings.size();++i) {
        CHECK(!markings[i].edge);CHECK(markings[i].type==MarkingType::dashed);
    }
    // And each lane still measures the width its own Link lane gives it.
    const auto widths=connectorLaneWidths(d.network,connector(d,id));
    test::near(widths.source[0],3,1e-9);test::near(widths.source[1],4,1e-9);
    test::near(widths.source[2],3.5,1e-9);
    CHECK(boundaries.size()==4);
}
TEST(attachments, a_connector_carries_its_own_marking_types) {
    auto d=roads();const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},3,3);
    History h;h.reset(d);
    h.execute("markings",[&](auto& m){
        changeConnectorLanes(m,id,{},{MarkingType::solid,MarkingType::dashed});});
    const auto markings=connectorMarkings(h.document().network,connector(h.document(),id));
    // The forcing: there really are two interior dividers to distinguish, and they now differ --
    // which they cannot under the derived rule, where every interior divider is dashed.
    CHECK(markings.size()==4);
    CHECK(markings[1].type==MarkingType::solid);CHECK(!markings[1].edge);
    CHECK(markings[2].type==MarkingType::dashed);CHECK(!markings[2].edge);
    // The outer edges stay solid edges: they are the edge of the carriageway, not a divider.
    CHECK(markings.front().edge && markings.front().type==MarkingType::solid);
    CHECK(markings.back().edge && markings.back().type==MarkingType::solid);
    h.undo();
    for(std::size_t i=1;i+1<markings.size();++i)
        CHECK(connectorMarkings(h.document().network,connector(h.document(),id))[i].type==MarkingType::dashed);
}
TEST(attachments, connector_lane_widths_round_trip_and_older_files_still_open) {
    auto d=roads();const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},2,2);
    History h;h.reset(d);
    h.execute("lanes",[&](auto& m){changeConnectorLanes(m,id,{4.25,4.75},{MarkingType::solid});});
    const auto json=documentJson(h.document());
    CHECK(json["schemaVersion"]==8);
    CHECK(json["network"]["connectors"][0]["laneWidths"][1]==4.75);
    CHECK(json["network"]["connectors"][0]["laneMarkings"][0]=="solid");
    // Round trip, exactly.
    CHECK(documentJson(parseDocument(Json::parse(json.dump())))==json);
    CHECK(parseDocument(json)==h.document());
    // THE TEST M1.18'S REVERT MAKES MANDATORY: a file written by the previous build -- schema 5,
    // no laneWidths, no laneMarkings -- must still open, and must draw exactly what it drew.
    // M1.18 changed what stored data meant with no migration and was reverted for it; its
    // verification measured 120 of 120 drawn vertices unchanged, which was true and beside the
    // point, because it never opened a file written by the previous build.
    auto old=documentJson(d);old["schemaVersion"]=5;
    for(auto& c:old["network"]["connectors"]){c.erase("laneWidths");c.erase("laneMarkings");}
    const auto reopened=parseDocument(old);
    // The forcing: the old file genuinely lacks the keys, so this exercises the absent path.
    CHECK(!old["network"]["connectors"][0].contains("laneWidths"));
    CHECK(reopened.network.connectors[0].laneWidths.empty());
    CHECK(reopened==d);
    const auto thenBoundaries=connectorBoundaries(d.network,connector(d,id));
    const auto nowBoundaries=connectorBoundaries(reopened.network,connector(reopened,id));
    for(std::size_t i=0;i<nowBoundaries.size();++i)for(std::size_t j=0;j<nowBoundaries[i].size();++j) {
        test::near(nowBoundaries[i][j].x,thenBoundaries[i][j].x,1e-12);
        test::near(nowBoundaries[i][j].y,thenBoundaries[i][j].y,1e-12);
    }
    // And a malformed value is rejected by name rather than surfacing as a parser error.
    auto bad=json;bad["network"]["connectors"][0]["laneWidths"][0]="wide";
    test::throws([&]{parseDocument(bad);},"INVALID_WIDTH");
    bad=json;bad["network"]["connectors"][0]["laneMarkings"][0]="stripey";
    test::throws([&]{parseDocument(bad);},"INVALID_MARKING");
}
TEST(attachments, connector_lane_widths_are_validated_and_dropped_when_the_range_resizes) {
    auto d=roads();const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},2,2);
    History h;h.reset(d);
    // A partial list is rejected: no field would say which lanes were authored.
    test::throws([&]{h.execute("partial",[&](auto& m){changeConnectorLanes(m,id,{4},{});});},"EDIT_LANES");
    test::throws([&]{h.execute("bad marking count",[&](auto& m){
        changeConnectorLanes(m,id,{},{MarkingType::solid,MarkingType::solid});});},"EDIT_LANES");
    test::throws([&]{h.execute("zero",[&](auto& m){changeConnectorLanes(m,id,{4,0},{});});},"INVALID_WIDTH");
    test::throws([&]{h.execute("nan",[&](auto& m){
        changeConnectorLanes(m,id,{4,std::numeric_limits<double>::quiet_NaN()},{});});},"INVALID_WIDTH");
    CHECK(connector(h.document(),id).laneWidths.empty());
    h.execute("widths",[&](auto& m){changeConnectorLanes(m,id,{4,4},{MarkingType::solid});});
    // The forcing: the widths really are there before the resize.
    CHECK(connector(h.document(),id).laneWidths.size()==2);
    h.execute("resize",[&](auto& m){changeConnectorRange(m,id,3,3);});
    // Dropped, not padded: an entry the author never typed is not a width they chose.
    CHECK(connector(h.document(),id).laneWidths.empty());
    CHECK(connector(h.document(),id).laneMarkings.empty());
    h.undo();CHECK(connector(h.document(),id).laneWidths.size()==2);
    // Clearing back to derived is always legal, whatever the lane count.
    h.execute("clear",[&](auto& m){changeConnectorLanes(m,id,{},{});});
    CHECK(connector(h.document(),id).laneWidths.empty());
}
