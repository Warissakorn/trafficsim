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
TEST(equation, zero_points_and_reset_straight_use_the_authored_chord) {
    auto d=turn(0);const auto& c=d.network.connectors.front();
    const auto paths=connectorPaths(d.network,c);
    for(const auto& path:paths) {
        CHECK(!path.equation);CHECK(path.geometry.size()==2);
        test::near(connectorPathLength(path),polylineLength(path.geometry),1e-12);
        near(connectorPathPoint(path,connectorPathLength(path)*.5),
             {(path.geometry.front().x+path.geometry.back().x)/2,(path.geometry.front().y+path.geometry.back().y)/2});
    }
    auto curved=turn(19);const auto before=connectorPaths(curved.network,curved.network.connectors.front());
    resetConnectorCurve(curved,c.id,true);
    const auto straight=connectorPaths(curved.network,curved.network.connectors.front());
    CHECK(straight.front().geometry.size()==2);
    CHECK(connectorPathLength(before.front())>connectorPathLength(straight.front())+1);
}
