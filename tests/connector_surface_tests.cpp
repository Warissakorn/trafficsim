#include "test.hpp"
#include "connector_surface_fixture.hpp"
#include "../src/model/network/rotation.hpp"
#include <algorithm>
#include <numbers>
using namespace trafficsim;
namespace {
double distance(Point a,Point b) { return std::hypot(a.x-b.x,a.y-b.y); }
void near(Point a,Point b) { test::near(a.x,b.x);test::near(a.y,b.y); }
void included(const ConnectorSurface& surface,const ConnectorMouth& mouth) {
    for(auto p:mouth.points)
        CHECK(std::any_of(surface.outline.begin(),surface.outline.end(),[&](auto q){return distance(p,q)<1e-8;}));
}
}
TEST(mouths, four_point_45_degree_mouth_projects_onto_the_link_not_the_connector) {
    auto n=surface_fixture::arrival(45);const auto original=n;
    const auto drawing=connectorSurface(n,n.connectors.front());
    CHECK(drawing.target.has_value());
    const auto& p=drawing.target->points;
    near(p[0],{0,0});near(p[1],{2-2*std::sqrt(2.),2});
    near(p[2],{p[1].x,4});near(p[3],{4-4*std::sqrt(2.),4});
    test::near(distance(p[1],p[2]),2);
    test::near(distance(p[2],p[3]),2*std::tan(std::numbers::pi/8));
    included(drawing,*drawing.target);
    CHECK(n==original);
}
TEST(mouths, four_point_near_perpendicular_and_perpendicular_mouths_keep_the_shoulder) {
    for(double degrees:{89.,89.999,90.}) {
        const auto n=surface_fixture::arrival(degrees);
        const auto drawing=connectorSurface(n,n.connectors.front());
        CHECK(drawing.target.has_value());
        const auto& p=drawing.target->points;
        const double a=2*std::tan(degrees*std::numbers::pi/360);
        near(p[0],{0,0});near(p[1],{-a,2});near(p[2],{-a,4});near(p[3],{-2*a,4});
        CHECK(distance(p[2],p[3])<=distance(p[1],p[2])+1e-8);
        included(drawing,*drawing.target);
    }
}
TEST(mouths, four_point_mouth_keeps_one_construction_beyond_90_degrees) {
    // D79: no switch of Link edges at 90 degrees. Each Connector edge meets the Link edge on its
    // own side, so past 90 the shoulder keeps growing as W/2*tan(theta/2) -- beyond W/2.
    for(double degrees:{91.,120.,135.,150.})for(auto side:{DrivingSide::left,DrivingSide::right}) {
        auto n=surface_fixture::arrival(degrees);n.drivingSide=side;
        const auto drawing=connectorSurface(n,n.connectors.front());
        CHECK(drawing.target.has_value());
        const auto& p=drawing.target->points;
        test::near(p[1].x,p[2].x); // Projection remains vertical on the horizontal Link.
        test::near(distance(p[1],p[2]),2);
        test::near(distance(p[2],p[3]),2*std::tan(degrees*std::numbers::pi/360));
        CHECK(distance(p[2],p[3])>2);
        included(drawing,*drawing.target);
    }
    // A shoulder past the reach limit (four times the width) is unusable: the legacy cap stays.
    const auto n=surface_fixture::arrival(170);
    const auto drawing=connectorSurface(n,n.connectors.front());
    CHECK(!drawing.target);
    for(auto p:drawing.outline)CHECK(std::isfinite(p.x) && std::isfinite(p.y));
}
TEST(mouths, four_point_source_and_target_follow_rotation_and_reflection) {
    for(bool source:{false,true})for(bool mirror:{false,true})for(double rotation:{0.,37.,90.,180.,270.}) {
        auto n=surface_fixture::arrival(45);
        if(source) {
            auto& c=n.connectors.front();std::swap(c.from,c.to);
            std::reverse(n.links[1].geometry.begin(),n.links[1].geometry.end());
            std::reverse(c.geometry.begin(),c.geometry.end());
        }
        const auto initial=connectorSurface(n,n.connectors.front());
        const auto m=source?initial.source:initial.target;CHECK(m.has_value());
        const auto transform=[&](Point p) { if(mirror)p.y=-p.y;return rotatePoint(p,{7,-3},rotation); };
        for(auto& l:n.links)for(auto& p:l.geometry)p=transform(p);
        for(auto& p:n.connectors.front().geometry)p=transform(p);
        const auto drawing=connectorSurface(n,n.connectors.front());
        const auto transformed=source?drawing.source:drawing.target;CHECK(transformed.has_value());
        for(int i=0;i<4;++i)near(transformed->points[i],transform(m->points[i]));
        included(drawing,*transformed);
    }
}
TEST(mouths, parallel_mouth_retains_the_existing_cap) {
    const auto n=surface_fixture::arrival(0);
    const auto drawing=connectorSurface(n,n.connectors.front());
    CHECK(!drawing.source);CHECK(!drawing.target);
    const auto boundaries=connectorBoundaries(n,n.connectors.front());
    CHECK(drawing.outline.front()==boundaries.front().front());
    for(auto p:drawing.outline)CHECK(std::isfinite(p.x) && std::isfinite(p.y));
}
TEST(mouths, four_point_mouth_sweep_follows_one_formula_at_every_angle) {
    int placed=0;
    for(int degrees=5;degrees<360;degrees+=5) {
        if(degrees==180)continue;
        const auto n=surface_fixture::arrival(degrees);
        const auto drawing=connectorSurface(n,n.connectors.front());
        const double theta=degrees<180?degrees:360-degrees; // arrival angle to the Link axis
        if(theta>=155){CHECK(!drawing.target);continue;} // past the reach limit
        CHECK(drawing.target.has_value());++placed;
        const auto& p=drawing.target->points;
        test::near(p[1].x,p[2].x);
        test::near(distance(p[1],p[2]),2);
        test::near(distance(p[2],p[3]),2*std::tan(theta*std::numbers::pi/360));
        included(drawing,*drawing.target);
    }
    CHECK(placed==60);
}
TEST(mouths, four_point_mouth_uses_the_attached_lane_range_centre) {
    auto n=surface_fixture::arrival(45);
    // Two 2 m lanes still make a 4 m range. Anchor the first path to its actual lane;
    // the mouth centre must stay halfway between the RANGE edges, not on that path.
    n.links[0].lanes={{"main-1",2},{"main-2",2}};
    n.links[1].lanes={{"feed-1",2},{"feed-2",2}};
    auto& c=n.connectors.front();c.fromLaneCount=c.toLaneCount=2;
    anchorConnectorEnds(n,c);
    const auto drawing=connectorSurface(n,c);CHECK(drawing.target.has_value());
    const auto& p=drawing.target->points;
    test::near(p[1].y,2);test::near(distance(p[1],p[2]),2);
    CHECK(distance(p[1],c.geometry.back())>.9);
    included(drawing,*drawing.target);
    for(const auto& m:drawing.markings)for(auto point:m.geometry)
        CHECK(std::isfinite(point.x) && std::isfinite(point.y));
}
TEST(mouths, four_point_mouth_handles_unequal_widths_without_claiming_the_half_width_bound) {
    auto n=surface_fixture::arrival(45);n.connectors.front().laneWidths={3};
    const auto drawing=connectorSurface(n,n.connectors.front());CHECK(drawing.target.has_value());
    const auto& p=drawing.target->points;
    test::near(p[1].x,p[2].x);test::near(distance(p[1],p[2]),2); // Link half-width, not 1.5.
    included(drawing,*drawing.target);
}
