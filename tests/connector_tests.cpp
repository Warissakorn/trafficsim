#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include <cmath>
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
        CHECK(json["schemaVersion"]==5);
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
    // Reanchoring is a similarity transform of the endpoint chord, so the curve's shape is
    // carried rigidly: every interior point keeps its position relative to the chord.
    const auto shape=[](const Connector& c){
        std::vector<Point> local;
        const double vx=c.geometry.back().x-c.geometry.front().x,vy=c.geometry.back().y-c.geometry.front().y;
        const double chord=vx*vx+vy*vy;
        for(std::size_t i=1;i+1<c.geometry.size();++i) {
            const double px=c.geometry[i].x-c.geometry.front().x,py=c.geometry[i].y-c.geometry.front().y;
            local.push_back({(px*vx+py*vy)/chord,(py*vx-px*vy)/chord});
        }
        return local;
    };
    const auto before_shape=shape(old),after_shape=shape(moved);
    CHECK(before_shape.size()==after_shape.size());
    for(std::size_t i=0;i<before_shape.size();++i) {
        test::near(after_shape[i].x,before_shape[i].x);test::near(after_shape[i].y,before_shape[i].y);
    }
    CHECK(moved.from==old.from && moved.to==old.to);CHECK(moved.geometry.size()==old.geometry.size());anchored(h.document());
    // Path independence is the point: returning the link to where it started must restore the
    // curve exactly, not leave it deformed by however many edits took it there.
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
namespace {
// Counts the places a polyline crosses itself, which is what an offset does round a bend
// tighter than the offset, and what a drawn line must never do.
int crossings(const std::vector<Point>& p) {
    int found=0;
    for(std::size_t i=0;i+1<p.size();++i)for(std::size_t j=i+2;j+1<p.size();++j) {
        const double rx=p[i+1].x-p[i].x,ry=p[i+1].y-p[i].y,sx=p[j+1].x-p[j].x,sy=p[j+1].y-p[j].y;
        const double denominator=rx*sy-ry*sx;
        if(std::abs(denominator)<1e-12)continue;
        const double t=((p[j].x-p[i].x)*sy-(p[j].y-p[i].y)*sx)/denominator;
        const double u=((p[j].x-p[i].x)*ry-(p[j].y-p[i].y)*rx)/denominator;
        if(t>=0 && t<=1 && u>=0 && u<=1)++found;
    }
    return found;
}
double apart(const std::vector<Point>& a,const std::vector<Point>& b,std::size_t i) {
    return std::hypot(b[i].x-a[i].x,b[i].y-a[i].y);
}
double minimumRadius(const std::vector<Point>& g) {
    double radius=1e300;
    for(std::size_t i=1;i+1<g.size();++i) {
        const double ab=std::hypot(g[i].x-g[i-1].x,g[i].y-g[i-1].y);
        const double bd=std::hypot(g[i+1].x-g[i].x,g[i+1].y-g[i].y);
        const double ad=std::hypot(g[i+1].x-g[i-1].x,g[i+1].y-g[i-1].y);
        const double area=std::abs((g[i].x-g[i-1].x)*(g[i+1].y-g[i-1].y)-(g[i].y-g[i-1].y)*(g[i+1].x-g[i-1].x))/2;
        if(area>1e-12)radius=std::min(radius,ab*bd*ad/(4*area));
    }
    return radius;
}
}
// A Connector between ends with different lane counts carries lanes, not a ribbon that shrinks.
// The lane that continues keeps the width its own links give it from end to end, and the lane the
// far end has no room for is the one that closes, like a merge taper.
TEST(connectors, the_lane_that_continues_keeps_its_width_and_the_extra_one_tapers_in) {
    auto d=roads();
    const auto id=addConnectorRange(d,{"in","in-1"},{"other","other-1"},2,1);
    const auto& c=editableConnector(d,id);
    // The forcing: two lanes into one, three boundaries, and source lanes of two different widths.
    CHECK(c.fromLaneCount==2);CHECK(c.toLaneCount==1);
    const auto boundaries=connectorBoundaries(d.network,c);
    CHECK(boundaries.size()==3);
    const auto& in=editableLink(d,"in");const auto& out=editableLink(d,"other");
    CHECK(in.lanes[0].width==3);CHECK(in.lanes[1].width==4);CHECK(out.lanes[0].width==3.5);
    const auto weights=connectorBlendWeights(c);
    double previous=1e300;
    for(std::size_t j=0;j<weights.size();++j) {
        // The continuing lane is 3 m where it leaves and 3.5 m where it arrives, and exactly the
        // width in between -- never the 2.6 m pinch that came of shrinking every lane together.
        test::near(apart(boundaries[0],boundaries[1],j),3+.5*weights[j],1e-9);
        const double wedge=apart(boundaries[1],boundaries[2],j);
        CHECK(wedge<previous);previous=wedge;
    }
    test::near(previous,0,1e-9);
    // Both mouths still meet their link's own lane edges exactly; the single target lane has two.
    const auto meets=[&](Point a,Point b){test::near(a.x,b.x,1e-9);test::near(a.y,b.y,1e-9);};
    for(std::size_t i=0;i<3;++i)meets(boundaries[i].front(),laneBoundaryGeometry(in,i,d.network.drivingSide).back());
    meets(boundaries[0].back(),laneBoundaryGeometry(out,0,d.network.drivingSide).front());
    meets(boundaries[1].back(),laneBoundaryGeometry(out,1,d.network.drivingSide).front());
    meets(boundaries[2].back(),boundaries[1].back());
    // The divider is a lane edge for its whole length, so it arrives on the edge of the lane the
    // two merge into -- not part way down the middle of it, where the traffic is.
    const auto markings=connectorMarkings(d.network,c);
    CHECK(markings.size()==3);
    CHECK(markings.front().edge);CHECK(markings.back().edge);CHECK(!markings[1].edge);
    CHECK(markings[1].geometry.size()==boundaries[1].size());
    const auto target=laneAttachment(d.network,c.to,false);
    for(const auto& p:markings[1].geometry)CHECK(std::hypot(p.x-target.x,p.y-target.y)>out.lanes[0].width/2-1e-9);
    // Two into two keeps both lanes and a full-length divider; one into one has no divider at all.
    auto wide=roads();const auto pair=addConnectorRange(wide,{"in","in-1"},{"out","out-1"},2,2);
    const auto both=connectorBoundaries(wide.network,editableConnector(wide,pair));
    const auto pairWeights=connectorBlendWeights(editableConnector(wide,pair));
    for(std::size_t j=0;j<both[0].size();++j) {
        test::near(apart(both[0],both[1],j),3+pairWeights[j],1e-9);   // in-1 3 m into out-1 4 m
        test::near(apart(both[1],both[2],j),4-pairWeights[j],1e-9);   // in-2 4 m into out-2 3 m
    }
    CHECK(connectorMarkings(wide.network,editableConnector(wide,pair))[1].geometry.size()==both[1].size());
    auto plain=roads();const auto single=addConnector(plain,{"in","in-2"},{"other","other-1"});
    CHECK(connectorMarkings(plain.network,editableConnector(plain,single)).size()==2);
    // A diverge is the mirror image: the lane that opens out starts at nothing and grows.
    auto open=roads();const auto out2=addConnectorRange(open,{"other","other-1"},{"out","out-1"},1,2);
    const auto opening=connectorBoundaries(open.network,editableConnector(open,out2));
    test::near(apart(opening[1],opening[2],0),0,1e-9);
    test::near(apart(opening[1],opening[2],opening[0].size()-1),3,1e-9);
}
// A reverse curve leaves both ends parallel, so the turn between the two tangents says nothing
// about how hard the road bends. Reading each end against the chord is what catches it.
TEST(connectors, a_reverse_curve_bends_no_harder_than_the_lane_it_carries) {
    auto d=roads();
    d.network.links.push_back({"back",{{10,-12},{90,-12}},{{"back-1",3.5}}});
    const auto id=addConnector(d,{"in","in-1"},{"back","back-1"});
    const auto& g=editableConnector(d,id).geometry;
    // The forcing: the two ends really do point the same way, which is what used to hide the bend.
    const Point entry{g[1].x-g[0].x,g[1].y-g[0].y},exit{g.back().x-g[g.size()-2].x,g.back().y-g[g.size()-2].y};
    const double along=(entry.x*exit.x+entry.y*exit.y)/std::hypot(entry.x,entry.y)/std::hypot(exit.x,exit.y);
    CHECK(along>.99);
    // Reading the turn between the tangents alone made this the gentlest case there is, and drew
    // it at 0.182 of its chord -- a kink. Read against the chord it opens to 0.218, and clears the
    // 3 m lane it carries. A cubic cannot do better on a chord this short, so this is the measure.
    const double chord=std::hypot(g.back().x-g.front().x,g.back().y-g.front().y);
    CHECK(minimumRadius(g)>.2*chord);
    CHECK(minimumRadius(g)>editableLink(d,"in").lanes.front().width);
    CHECK(connectorShapeIssues(d.network).empty());
}
// An offset of a bend tighter than the offset loops back on itself. The surface is filled
// correctly either way, but the line drawn round it must not double back.
TEST(connectors, a_drawn_edge_never_doubles_back_on_a_tight_bend) {
    std::vector<Point> arc{{0,-10}};
    for(int i=0;i<=6;++i)arc.push_back({3*std::sin(std::numbers::pi*i/12),3-3*std::cos(std::numbers::pi*i/12)});
    arc.push_back({arc.back().x-8,arc.back().y+8}); // A quarter turn of radius 3 between two straights.
    const Link link{"tight",arc,{{"tight-1",3.5},{"tight-2",3.5}}};
    const auto inner=laneBoundaryGeometry(link,0,DrivingSide::left);
    CHECK(crossings(inner)==1); // The forcing: this edge really does cross itself today.
    CHECK(crossings(trimSelfIntersections(inner))==0);
    CHECK(trimSelfIntersections(inner).size()<inner.size());
    // A boundary with no loop is returned unchanged, point for point.
    const auto outer=laneBoundaryGeometry(link,2,DrivingSide::left);
    CHECK(crossings(outer)==0);CHECK(trimSelfIntersections(outer)==outer);
}
// The default curve stands for a circular arc. It used to use the same control reach for every
// turn, which is only right for a gentle one, and drew a U-turn at half the radius it needs.
TEST(connectors, a_u_turn_gets_the_radius_its_own_lanes_need) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);
        const auto reverse=oppositeLink(d,"in",2);
        const auto lane=editableLink(d,reverse).lanes.front();
        const auto id=addConnector(d,{"in","in-1"},{reverse,lane.id});
        const auto& g=editableConnector(d,id).geometry;
        // The forcing: the two ends really do face opposite ways.
        const auto entry=Point{g[1].x-g[0].x,g[1].y-g[0].y};
        const auto exit=Point{g.back().x-g[g.size()-2].x,g.back().y-g[g.size()-2].y};
        CHECK(entry.x*exit.x+entry.y*exit.y<0);
        double radius=1e300;
        for(std::size_t i=1;i+1<g.size();++i) {
            const double ab=std::hypot(g[i].x-g[i-1].x,g[i].y-g[i-1].y);
            const double bd=std::hypot(g[i+1].x-g[i].x,g[i+1].y-g[i].y);
            const double ad=std::hypot(g[i+1].x-g[i-1].x,g[i+1].y-g[i-1].y);
            const double area=std::abs((g[i].x-g[i-1].x)*(g[i+1].y-g[i-1].y)-(g[i].y-g[i-1].y)*(g[i+1].x-g[i-1].x))/2;
            if(area>1e-12)radius=std::min(radius,ab*bd*ad/(4*area));
        }
        // A U-turn stands for a half circle, whose radius is half the chord. The old fixed
        // reach gave 0.176 of the chord here -- 2.29 m against a 3 m lane, tight enough for the
        // ribbon's own edge to cross itself. The arc-based reach gives 0.454.
        const double chord=std::hypot(g.back().x-g.front().x,g.back().y-g.front().y);
        CHECK(radius>.4*chord);
        CHECK(radius>lane.width);
        CHECK(connectorShapeIssues(d.network).empty());
        // No turn at all is still the degenerate case the old constant was built for: aligned
        // tangents put every control point on the chord, so the curve is the straight line.
        // Same widths in the same order, so in-1 and out-1 are one straight line apart.
        auto aligned=roads(side);aligned.network.links[1].geometry={{80,0},{160,0}};
        aligned.network.links[1].lanes={{"out-1",3},{"out-2",4}};
        const auto gentle=addConnector(aligned,{"in","in-1"},{"out","out-1"});
        const auto& g2=editableConnector(aligned,gentle).geometry;
        const auto first=g2.front(),last=g2.back();
        const double span=std::hypot(last.x-first.x,last.y-first.y);
        CHECK(span>1);
        for(const auto& p:g2) {
            const double off=std::abs((last.x-first.x)*(p.y-first.y)-(last.y-first.y)*(p.x-first.x))/span;
            test::near(off,0,1e-9);
        }
    }
}
