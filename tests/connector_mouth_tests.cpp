#include "test.hpp"
#include "connector_surface_fixture.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/model/network/rotation.hpp"
#include <algorithm>
#include <numbers>
using namespace trafficsim;
namespace {
double distance(Point a,Point b) { return std::hypot(a.x-b.x,a.y-b.y); }
void near(Point a,Point b) { test::near(a.x,b.x);test::near(a.y,b.y); }
Network arrival(double angle,bool source,DrivingSide side) {
    auto n=surface_fixture::arrival(angle);n.drivingSide=side;
    if(source) {
        auto& c=n.connectors.front();std::swap(c.from,c.to);
        std::reverse(c.geometry.begin(),c.geometry.end());
        // Reverse BOTH roads as well as the connector, preserving the directed angle.
        for(auto& l:n.links)std::reverse(l.geometry.begin(),l.geometry.end());
        c.from.station=polylineLength(n.links[0].geometry)-*c.from.station;
    }
    return n;
}
void included(const ConnectorSurface& s,const ConnectorMouth& m) {
    for(auto p:m.points)CHECK(std::any_of(s.outline.begin(),s.outline.end(),[&](auto q){return distance(p,q)<1e-7;}));
}
}
TEST(mouths, both_ends_keep_index_order_across_90_and_the_former_reach_limit) {
    int tested=0;
    for(bool source:{false,true})for(auto side:{DrivingSide::left,DrivingSide::right})
    for(double angle:{5.,30.,60.,75.5,75.6,89.999,90.,90.001,120.,150.,151.,151.1,170.,179.,179.9})
    for(double rotate:{0.,37.,173.}) {
        auto n=arrival(angle,source,side);
        for(auto& l:n.links)for(auto& p:l.geometry)p=rotatePoint(p,{7,-3},rotate);
        for(auto& p:n.connectors.front().geometry)p=rotatePoint(p,{7,-3},rotate);
        const auto& c=n.connectors.front();const auto s=connectorSurface(n,c);
        const auto& m=source?s.source:s.target;CHECK(m);
        test::near(distance(m->points[1],m->points[2]),2,1e-6);
        test::near(distance(m->points[2],m->points[3]),2*std::tan(angle*std::numbers::pi/360),1e-6);
        const auto axis=connectorCentreline(n,c);
        const auto u=directionAlong(axis,source?0:polylineLength(axis),!source);
        const Point delta{m->boundaries.back().x-m->boundaries.front().x,
                          m->boundaries.back().y-m->boundaries.front().y};
        // Signed lateral order, not min(distance to either side): catches the source flip.
        test::near(delta.x*(-u.y)+delta.y*u.x,side==DrivingSide::left?-4.:4.,1e-6);
        included(s,*m);
        CHECK(connectorBoundaries(n,c)==s.boundaries);
        for(std::size_t j=0;j<axis.size();++j)
            near(axis[j],{(s.boundaries.front()[j].x+s.boundaries.back()[j].x)/2,
                          (s.boundaries.front()[j].y+s.boundaries.back()[j].y)/2});
        ++tested;
    }
    CHECK(tested==180);
}
TEST(mouths, undefined_parallel_pairs_do_not_erase_the_other_mouth_or_fake_a_square_cap) {
    auto n=surface_fixture::arrival(45);auto& c=n.connectors.front();
    // The feed is exactly parallel to the first leg. Different widths make its two
    // corresponding edge lines distinct parallels: no intersection exists.
    c.laneWidths={3};
    const auto s=connectorSurface(n,c);
    CHECK(!s.source);CHECK(s.target);CHECK(s.outline.empty());CHECK(!s.markings.empty());
    near(s.boundaries.front().back(),s.target->boundaries.front());
    near(s.boundaries.back().back(),s.target->boundaries.back());
    for(bool source:{false,true}) {
        auto reverse=arrival(180,source,DrivingSide::left);
        const auto drawing=connectorSurface(reverse,reverse.connectors.front());
        CHECK(!(source?drawing.source:drawing.target));
        CHECK(drawing.outline.empty());
        for(const auto& b:drawing.boundaries)for(auto p:b)CHECK(std::isfinite(p.x)&&std::isfinite(p.y));
    }
}
TEST(mouths, a_fold_is_reported_without_discarding_computed_mouth_points) {
    bool forced=false;
    for(double angle:{150.,170.,179.}) {
        auto n=surface_fixture::arrival(angle);auto& c=n.connectors.front();
        const auto s=connectorSurface(n,c);CHECK(s.source);CHECK(s.target);
        included(s,*s.source);included(s,*s.target);
        if(s.selfIntersecting) {
            forced=true;
            const auto issues=connectorShapeIssues(n);
            CHECK(std::any_of(issues.begin(),issues.end(),[](const auto& i){return i.code=="WARN_CONNECTOR_ALIGNMENT";}));
        }
    }
    CHECK(forced);
}
TEST(mouths, the_reference_axis_centres_all_lanes_before_edges_are_constructed) {
    for(auto side:{DrivingSide::left,DrivingSide::right})for(int from:{1,2,3})for(int to:{1,2,3}) {
        auto n=surface_fixture::arrival(60);n.drivingSide=side;
        n.links[0].lanes={{"main-1",2},{"main-2",3},{"main-3",4}};
        n.links[1].lanes={{"feed-1",4},{"feed-2",3},{"feed-3",2}};
        auto& c=n.connectors.front();c.fromLaneCount=from;c.toLaneCount=to;anchorConnectorEnds(n,c);
        c.geometry[2].x+=7;c.geometry[2].y-=9; // actual bent, unequal-width, tapering body
        const auto original=c.geometry;
        const auto axis=connectorCentreline(n,c),grips=connectorGrips(n,c);
        const auto body=connectorBodyBoundaries(n,c);
        CHECK(axis==grips);CHECK(c.geometry==original);
        near(axis.front(),*connectorRangeCentre(n,c,true));near(axis.back(),*connectorRangeCentre(n,c,false));
        for(std::size_t j=0;j<axis.size();++j)
            near(axis[j],{(body.front()[j].x+body.back()[j].x)/2,(body.front()[j].y+body.back()[j].y)/2});
        // Authored widths change the offsets, not the controlling centre axis.
        c.laneWidths=std::vector<double>(std::max(from,to),5.5);
        CHECK(connectorCentreline(n,c)==axis);
    }
}
TEST(mouths, dragging_the_centre_grip_hits_the_pointer_after_commit_and_reopen) {
    for(auto side:{DrivingSide::left,DrivingSide::right})for(double angle:{30.,60.,90.,120.,150.,170.}) {
        ProjectDocument d;d.network=surface_fixture::arrival(angle);d.network.drivingSide=side;
        d.network.links[0].lanes={{"main-1",3},{"main-2",4}};
        d.network.links[1].lanes={{"feed-1",4},{"feed-2",3}};
        auto& c=d.network.connectors.front();c.fromLaneCount=c.toLaneCount=2;anchorConnectorEnds(d.network,c);
        const auto id=c.id;const auto before=c.geometry;
        const auto grip=connectorGrips(d.network,c)[2];const Point target{grip.x+5,grip.y+5};
        const auto geometry=connectorGeometryWithGrip(d.network,c,2,target);
        History h;h.reset(d);h.execute("move grip",[&](auto& edit){changeConnectorGeometry(edit,id,geometry);});
        const auto after=h.document();near(connectorGrips(after.network,after.network.connectors.front())[2],target);
        for(std::size_t i=0;i<before.size();++i)if(i!=2)CHECK(after.network.connectors.front().geometry[i]==before[i]);
        const auto reopened=parseDocument(Json::parse(documentJson(after).dump()));
        near(connectorGrips(reopened.network,reopened.network.connectors.front())[2],target);
        h.undo();CHECK(h.document()==d);h.redo();CHECK(h.document()==after);
    }
}
