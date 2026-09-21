#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/project/diagnostics.hpp"
#include <numbers>
using namespace trafficsim;
namespace {
ProjectDocument turn(double degrees, DrivingSide side=DrivingSide::left, int lanes=1) {
    ProjectDocument d;d.network.drivingSide=side;
    const double a=degrees*std::numbers::pi/180;
    std::vector<Lane> source,target;
    for(int i=0;i<lanes;++i){source.push_back({"a"+std::to_string(i),3.5});target.push_back({"b"+std::to_string(i),3.5});}
    d.network.links={{"a",{{-60,-30},{-30,-30}},source},
        {"b",{{-60*std::cos(a),-60*std::sin(a)},{60*std::cos(a),60*std::sin(a)}},target}};
    addConnectorRange(d,{"a","a0"},{"b","b0",60},lanes,lanes);
    CHECK(validateNetwork(d.network).empty());return d;
}
double dot(Point a,Point b){return a.x*b.x+a.y*b.y;}
Point step(Point a,Point b){const double l=std::hypot(b.x-a.x,b.y-a.y);return {(b.x-a.x)/l,(b.y-a.y)/l};}
}
TEST(lifecycle, retarget_across_the_road_rebuilds_the_turn_and_undo_restores_authored_points) {
    for(auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=turn(0,side);auto& c=d.network.connectors.front();
        c.geometry={c.geometry.front(),{-20,-25},{-15,-10},{-5,-3},c.geometry.back()};
        const auto old=c;History h;h.reset(d);
        // Move the destination past the old last control point, to the opposite side.
        auto to=c.to;to.station=20;
        const auto target=laneAttachment(d.network,to,false);
        CHECK(dot(step(old.geometry[old.geometry.size()-2],target),Point{1,0})<0);
        h.execute("retarget",[&](auto& doc){changeConnectorEndpoints(doc,old.id,old.from,to);});
        const auto moved=h.document().network.connectors.front();
        CHECK(moved.geometry.size()==old.geometry.size());
        const auto tangents=connectorTangents(d.network,moved.from,moved.to);
        CHECK(dot(step(moved.geometry[moved.geometry.size()-2],moved.geometry.back()),tangents.second)>0.5);
        CHECK(validateNetwork(h.document().network).empty());
        const auto saved=documentJson(h.document());CHECK(documentJson(parseDocument(saved))==saved);
        h.undo();CHECK(h.document()==d);h.redo();CHECK(documentJson(h.document())==saved);
    }
}
TEST(lifecycle, perpendicular_authored_mouth_never_collapses_to_a_needle) {
    for(auto side:{DrivingSide::left,DrivingSide::right})for(int lanes:{1,2,3})
    for(double angle:{-100.,-91.,-90.,-89.,-80.,80.,89.,90.,91.,100.,180.}) {
        auto d=turn(0,side,lanes);auto& c=d.network.connectors.front();
        const auto b=c.geometry.back();const double a=angle*std::numbers::pi/180;
        const Point direction{std::cos(a),std::sin(a)};
        c.geometry={c.geometry.front(),{b.x-direction.x*30,b.y-direction.y*30},
            {b.x-direction.x*15,b.y-direction.y*15},b};
        CHECK(validateNetwork(d.network).empty());
        CHECK(dot(direction,Point{1,0})<.2); // forces an ill-conditioned or reversed arrival
        const auto edges=connectorBoundaries(d.network,c);
        const auto p=edges.front().back(),q=edges.back().back();
        const double perpendicular=std::abs((q.x-p.x)*direction.y-(q.y-p.y)*direction.x);
        // A square fallback keeps a full-width mouth; a clipped needle cannot pass this.
        CHECK(perpendicular>=lanes*3.5*.95);
        CHECK(std::hypot(q.x-p.x,q.y-p.y)<lanes*3.5*2);
        const auto fit=connectorMouthFit(d.network,c);
        CHECK(fit.target.squareFallback);
        const auto diagnostics=documentDiagnostics(d);
        bool advisory=false;
        for(const auto& issue:diagnostics)if(issue.code=="WARN_CONNECTOR_ALIGNMENT") {
            CHECK(issue.selectId==c.id);CHECK(issue.severity==DiagnosticSeverity::advisory);advisory=true;
        }
        CHECK(advisory);validateDocument(d);
    }
}
TEST(lifecycle, generated_turns_keep_forward_end_legs_on_both_driving_sides) {
    for(auto side:{DrivingSide::left,DrivingSide::right})for(int lanes:{1,2,3})
    for(int degrees=0;degrees<360;degrees+=15) {
        const auto d=turn(degrees,side,lanes);const auto& c=d.network.connectors.front();
        for(const auto& path:connectorPaths(d.network,c)) {
            const auto [entry,exit]=connectorTangents(d.network,path.from,path.to);
            const auto& g=path.geometry;
            CHECK(dot(step(g[0],g[1]),entry)>0);
            CHECK(dot(step(g[g.size()-2],g.back()),exit)>0);
        }
        const auto boundaries=connectorBoundaries(d.network,c);
        for(const auto& edge:boundaries)for(const auto p:edge)CHECK(std::isfinite(p.x) && std::isfinite(p.y));
        const auto encoded=documentJson(d);CHECK(documentJson(parseDocument(encoded))==encoded);
    }
}
TEST(lifecycle, attachment_at_a_corner_uses_the_travel_side_instead_of_an_averaged_zero_tangent) {
    const std::vector<Point> cusp{{0,0},{20,0},{0,0}};
    CHECK(directionAlong(cusp,20,true)==Point{1,0});
    CHECK(directionAlong(cusp,20,false)==Point{-1,0});
    ProjectDocument d;d.network.links={{"a",cusp,{{"a1",3.5}}},
        {"b",{{30,20},{50,20}},{{"b1",3.5}}}};
    addConnector(d,{"a","a1",20},{"b","b1"});
    CHECK(validateNetwork(d.network).empty());
    CHECK(!connectorBoundaries(d.network,d.network.connectors.front()).empty());
}
TEST(lifecycle, collapsed_offset_lanes_are_rejected_before_the_editor_samples_them) {
    ProjectDocument d;d.network.links={{"a",{{0,0},{1,0},{1,1}},{{"a1",3.5}}}};
    CHECK(validateNetwork(d.network).empty());History h;h.reset(d);h.markSaved();
    auto collapsed=d;collapsed.network.links.front().laneOffset=1;
    CHECK(polylineLength(laneGeometry(collapsed.network.links.front(),"a1",DrivingSide::left))==0);
    CHECK(!validateNetwork(collapsed.network).empty());
    test::throws([&]{h.execute("collapse",[](auto& doc){doc.network.links.front().laneOffset=1;});},"INVALID_GEOMETRY");
    CHECK(h.document()==d);CHECK(!h.canUndo());CHECK(!h.dirty());
    test::throws([&]{parseDocument(documentJson(collapsed));},"INVALID_GEOMETRY");
}
TEST(lifecycle, every_one_to_three_lane_range_transition_roundtrips_and_undoes_atomically) {
    for(auto side:{DrivingSide::left,DrivingSide::right})for(bool leading:{false,true})
    for(int from=1;from<=3;++from)for(int to=1;to<=3;++to) {
        auto d=turn(45,side,3);const auto id=d.network.connectors.front().id;
        changeConnectorLanes(d,id,{3,4,5},{MarkingType::solid,MarkingType::doubleLine});
        History h;h.reset(d);h.markSaved();
        const bool changed=h.execute("range",[&](auto& doc){changeConnectorRange(doc,id,from,to,leading);});
        CHECK(changed==(from!=3 || to!=3));
        const auto after=h.document();const auto& c=after.network.connectors.front();
        CHECK(c.fromLaneCount==from && c.toLaneCount==to);
        for(const auto& path:connectorPaths(after.network,c)) {
            CHECK(path.geometry.front()==laneAttachment(after.network,path.from,true));
            CHECK(path.geometry.back()==laneAttachment(after.network,path.to,false));
        }
        if(std::max(from,to)<3){CHECK(c.laneWidths.empty());CHECK(c.laneMarkings.empty());}
        CHECK(connectorBoundaries(after.network,c).size()==static_cast<std::size_t>(std::max(from,to))+1);
        CHECK(parseDocument(documentJson(after))==after);
        for(int bad:{0,13}) {
            test::throws([&]{h.execute("invalid range",[&](auto& doc){changeConnectorRange(doc,id,bad,to,leading);});});
            CHECK(h.document()==after);
        }
        if(changed){h.undo();CHECK(h.document()==d);CHECK(!h.dirty());h.redo();CHECK(h.document()==after);}
    }
}
TEST(lifecycle, retarget_keeps_accepted_dense_imports_editable) {
    auto d=turn(0);auto& c=d.network.connectors.front();const auto curve=c.geometry;
    c.geometry={curve.front()};
    for(int i=1;i<64;++i)c.geometry.push_back(pointAlong(curve,polylineLength(curve)*i/64));
    c.geometry.push_back(curve.back());CHECK(c.geometry.size()>42);
    const auto loaded=parseDocument(documentJson(d));CHECK(loaded==d);
    auto to=c.to;to.station=20;changeConnectorEndpoints(d,c.id,c.from,to);
    CHECK(c.geometry.size()==65);CHECK(validateNetwork(d.network).empty());
    CHECK(c.geometry.back()==laneAttachment(d.network,to,false));
}
