#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/model/network/right_of_way.hpp"
#include <algorithm>
using namespace trafficsim;
namespace {
ProjectDocument turn(int count) {
    ProjectDocument d;d.network.links={
        {"a",{{-80,0},{0,0}},{{"a1",3.5},{"a2",3.5},{"a3",3.5}}},
        {"b",{{30,30},{30,100}},{{"b1",3.5},{"b2",3.5},{"b3",3.5}}}};
    addConnectorRange(d,{"a","a1"},{"b","b1"},3,3);
    auto& c=d.network.connectors.front();c.geometry=connectorCurve(d.network,c.from,c.to,count);return d;
}
void near(Point a,Point b,double tolerance=1e-7){test::near(a.x,b.x,tolerance);test::near(a.y,b.y,tolerance);}
}
TEST(equation, parabola_has_exact_known_positions_tangents_and_arc_length) {
    // B(t)=(t,t²), with independent analytic integral of sqrt(1+4t²).
    const auto c=makeConnectorEquation({Point{0,0},{1./3,0},{2./3,1./3},{1,1}});
    test::near(c.arcStations.back(),std::sqrt(5.)/2+std::asinh(2.)/4,1e-11);
    for(int i=0;i<=100;++i){const double t=i/100.;near(equationPoint(c,t),{t,t*t},1e-12);
        near(equationDerivative(c,t),{1,2*t},1e-12);
        test::near(equationStation(c,t),t*std::sqrt(1+4*t*t)/2+std::asinh(2*t)/4,1e-11);
        test::near(equationParameter(c,equationStation(c,t)),t,1e-10);}
}
TEST(equation, station_is_distance_not_uniform_parameter_or_chord_distance) {
    const auto c=makeConnectorEquation({Point{0,0},{0,0},{10,0},{10,0}});
    test::near(c.arcStations.back(),10,1e-12);
    for(double s:{0.,.01,1.,3.,8.,9.99,10.})near(equationPoint(c,equationParameter(c,s)),{s,0},1e-9);
    test::near(equationClosestStation(c,{3,8}),3,1e-8);
    const auto parabola=makeConnectorEquation({Point{0,0},{1./3,0},{2./3,1./3},{1,1}});
    CHECK(parabola.arcStations.back()>std::sqrt(2.)+.05);
}
TEST(equation, closest_station_finds_global_minimum_including_loops_and_endpoints) {
    for(const auto controls:{std::array<Point,4>{Point{0,0},{30,80},{-30,80},{0,0}},
                             std::array<Point,4>{Point{0,0},{0,0},{20,20},{40,0}}}) {
        const auto c=makeConnectorEquation(controls);
        for(Point target:std::vector<Point>{{0,0},{10,25},{-15,40},{80,-30},{0,60}}) {
            const auto p=equationPoint(c,equationParameter(c,equationClosestStation(c,target)));
            const double best=std::hypot(p.x-target.x,p.y-target.y);
            for(int i=0;i<=2000;++i){const auto q=equationPoint(c,i/2000.);
                CHECK(best<=std::hypot(q.x-target.x,q.y-target.y)+1e-7);}
        }
    }
}
TEST(equation, drawing_point_count_and_interior_edits_do_not_change_runtime_motion) {
    auto d=turn(0);const auto& c=d.network.connectors.front();
    const auto first=connectorPaths(d.network,c);CHECK(first[0].equation);CHECK(first[0].geometry.size()==2);
    // Zero intermediate points draws a straight chord, but the runtime remains a real curve.
    const auto curved=connectorPathPoint(first[0],connectorPathLength(first[0])*.5);
    const auto chord=pointAlong(first[0].geometry,polylineLength(first[0].geometry)*.5);
    CHECK(std::hypot(curved.x-chord.x,curved.y-chord.y)>5);
    for(int count:{1,3,19,40}) {
        auto other=turn(count);auto& connector=other.network.connectors.front();
        auto paths=connectorPaths(other.network,connector);
        for(std::size_t k=0;k<paths.size();++k){test::near(connectorPathLength(paths[k]),connectorPathLength(first[k]),0);
            for(double fraction:{0.,.13,.5,.89,1.})near(connectorPathPoint(paths[k],connectorPathLength(paths[k])*fraction),
                connectorPathPoint(first[k],connectorPathLength(first[k])*fraction),0);}
        connector.geometry[1].x+=7;connector.geometry[1].y-=4;
        paths=connectorPaths(other.network,connector);
        for(std::size_t k=0;k<paths.size();++k)test::near(connectorPathLength(paths[k]),connectorPathLength(first[k]),0);
    }
}
TEST(equation, controls_heads_and_both_attachments_use_equation_stations) {
    auto d=turn(19);const auto& c=d.network.connectors.front();const auto table=runtimeSections(d.network);
    for(const auto& path:table.paths) {
        near(connectorPathPoint(path,0),laneAttachment(d.network,path.from,true));
        near(connectorPathPoint(path,connectorPathLength(path)),laneAttachment(d.network,path.to,false));
        const auto tangents=connectorTangents(d.network,path.from,path.to);
        auto a=connectorPathDirection(path,0),b=connectorPathDirection(path,connectorPathLength(path));
        near({a.x/std::hypot(a.x,a.y),a.y/std::hypot(a.x,a.y)},tangents.first);
        near({b.x/std::hypot(b.x,b.y),b.y/std::hypot(b.x,b.y)},tangents.second);
        double station=0;for(int j=1;j<=5;++j)station+=std::hypot(c.geometry[j].x-c.geometry[j-1].x,c.geometry[j].y-c.geometry[j-1].y);
        const ControlPoint point{{"","",c.id,path.from.laneId,path.to.laneId},station};
        const auto located=locateControlPoint(d.network,table,point);CHECK(located);
        test::near(located->position,equationStation(*path.equation,.25),1e-9);
        test::near(connectorAuthoringStation(c,path,located->position),station,1e-7);
        const auto target=equationPoint(*path.equation,.43);
        const auto placed=nearestHeadSlot(d.network,target,0);CHECK(placed);CHECK(placed->slot.connectorId==path.id);
        test::near(placed->station,equationStation(*path.equation,.43),1e-7);
    }
}
