#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
using namespace trafficsim;
namespace {
ProjectDocument roads() {
    ProjectDocument d;d.network.drivingSide=DrivingSide::left;
    d.network.links={{"in",{{-80,0},{0,0}},{{"in-1",3.5}}},{"out",{{30,30},{30,100}},{{"out-1",3.5}}}};
    return d;
}
// How far a point is from a polyline, which is what "the road goes through here" means when the
// road carries several samples per stored point.
double away(const std::vector<Point>& road,Point p) {
    double best=1e300;
    for(std::size_t i=1;i<road.size();++i) {
        const double dx=road[i].x-road[i-1].x,dy=road[i].y-road[i-1].y,square=dx*dx+dy*dy;
        const double t=square>0?std::clamp(((p.x-road[i-1].x)*dx+(p.y-road[i-1].y)*dy)/square,0.,1.):0.;
        best=std::min(best,std::hypot(p.x-road[i-1].x-t*dx,p.y-road[i-1].y-t*dy));
    }
    return best;
}
// The sharpest corner in a polyline, in degrees. A chain of stored points turns in a few big
// steps; the road through them turns in many small ones, which is what smooth means here.
double sharpest(const std::vector<Point>& p) {
    double worst=0;
    for(std::size_t i=1;i+1<p.size();++i) {
        const Point a{p[i].x-p[i-1].x,p[i].y-p[i-1].y},b{p[i+1].x-p[i].x,p[i+1].y-p[i].y};
        if(std::hypot(a.x,a.y)<1e-12 || std::hypot(b.x,b.y)<1e-12)continue;
        worst=std::max(worst,std::abs(std::atan2(a.x*b.y-a.y*b.x,a.x*b.x+a.y*b.y))*180/std::numbers::pi);
    }
    return worst;
}
double apart(const std::vector<Point>& a,const std::vector<Point>& b) {
    double worst=0;for(const auto& p:a)worst=std::max(worst,away(b,p));return worst;
}
}
// Vissim stores a Connector as a curve through a few intermediate points; the dialog's
// Intermediate points field is how many. What is stored is those points, and the road is drawn
// through every one of them.
TEST(spline, a_connector_stores_a_few_points_and_the_road_goes_through_each) {
    auto d=roads();const auto id=addConnector(d,{"in","in-1"},{"out","out-1"});
    const auto& c=editableConnector(d,id);
    // The forcing: the stored polyline really is short, so a road equal to it would be a chain
    // of four straight legs and this test could not tell the two apart.
    CHECK(c.geometry.size()==static_cast<std::size_t>(kDefaultIntermediatePoints)+2);
    std::vector<std::size_t> indices;
    const auto road=connectorRoad(d.network,c,&indices);
    CHECK(road.size()>4*c.geometry.size());
    CHECK(indices.size()==c.geometry.size());
    for(std::size_t i=0;i<c.geometry.size();++i) {
        // Through the point, not merely near it: the sample the index names is the point itself.
        test::near(road[indices[i]].x,c.geometry[i].x,1e-9);
        test::near(road[indices[i]].y,c.geometry[i].y,1e-9);
    }
    CHECK(road.front()==c.geometry.front());CHECK(road.back()==c.geometry.back());
    // And the road is smooth where the stored points are corners: the control polygon turns in
    // steps of tens of degrees, the road in steps of a few.
    CHECK(sharpest(c.geometry)>10);
    CHECK(sharpest(road)<5);
    // It leaves and arrives along its lanes, which is what clamping the two end tangents buys.
    const auto [entry,exit]=connectorTangents(d.network,c.from,c.to);
    const Point leaves{road[1].x-road[0].x,road[1].y-road[0].y};
    const Point arrives{road.back().x-road[road.size()-2].x,road.back().y-road[road.size()-2].y};
    for(const auto& pair:std::vector<std::pair<Point,Point>>{{leaves,entry},{arrives,exit}}) {
        const auto& [step,lane]=pair;
        const double length=std::hypot(step.x,step.y);
        // The first and last sampled steps are chords of the curve, not its derivative, so
        // tangency is read to within the 1.6 degrees that chord cuts off, not to zero.
        CHECK(std::abs(step.x*lane.y-step.y*lane.x)/length<.03);
    }
}
// Dragging one point is the whole point of storing few of them: the road follows it, and stays
// a road rather than gaining the kink every sample of a baked curve used to gain.
TEST(spline, a_dragged_point_bends_the_road_without_kinking_it) {
    auto d=roads();const auto id=addConnector(d,{"in","in-1"},{"out","out-1"});
    const auto before=connectorRoad(d.network,editableConnector(d,id));
    auto moved=editableConnector(d,id).geometry;
    const Point target{moved[2].x+6,moved[2].y-9};
    moved[2]=target;
    History h;h.reset(d);
    h.execute("drag",[&](auto& m){changeConnectorGeometry(m,id,moved);});
    const auto& c=h.document().network.connectors[0];
    const auto after=connectorRoad(h.document().network,c);
    // The forcing: the drag really moved the point, and the road really moved with it.
    CHECK(away(before,target)>1);
    test::near(away(after,target),0,1e-9);
    // And no corner anywhere: a stored point is a place the road passes through, not a hinge.
    // And no hinge where the point is: the author's own polygon turns 98.1 degrees there, the
    // road 32.9, spread over the samples either side of it. Sampling the polygon instead would
    // put the full 98.1 into one corner, which is the kink a baked curve used to gain.
    CHECK(sharpest(after)<sharpest(moved)/2.5);
    CHECK(after.size()==before.size());
    // The far parts of the road barely move, because only one span's tangents changed.
    CHECK(away(after,before.front())<1e-9);CHECK(away(after,before.back())<1e-9);
}
// Vissim's Intermediate points spin box re-lays the shape the author has; it does not throw it
// away for the default curve. Raising the count and lowering it again returns the same road.
TEST(spline, changing_the_point_count_re_lays_the_shape_it_already_has) {
    auto d=roads();const auto id=addConnector(d,{"in","in-1"},{"out","out-1"});
    auto bent=editableConnector(d,id).geometry;bent[2]={bent[2].x+5,bent[2].y-7};
    changeConnectorGeometry(d,id,bent);
    const auto original=connectorRoad(d.network,editableConnector(d,id));
    // The forcing: the shape is the author's, not the default one, so a reset would show.
    CHECK(apart(original,connectorRoad(d.network,[&]{auto c=editableConnector(d,id);
        c.geometry=connectorCurve(d.network,c.from,c.to);return c;}()))>1);
    resampleConnectorPoints(d,id,7);
    CHECK(editableConnector(d,id).geometry.size()==9);
    const auto richer=connectorRoad(d.network,editableConnector(d,id));
    // The author's bend survives the change: laying it out again with 7 points holds the road
    // to 0.49 m, where Reset curve -- which is what discarding the shape would mean -- is 6.61 m
    // away from it. Points at equal spacing along the road are not the points the old count had,
    // so the two are close rather than identical, exactly as Vissim re-fairs it.
    const auto reset=connectorRoad(d.network,[&]{auto c=editableConnector(d,id);
        c.geometry=connectorCurve(d.network,c.from,c.to);return c;}());
    CHECK(apart(richer,original)<.6);
    CHECK(apart(reset,original)>10*apart(richer,original));
    resampleConnectorPoints(d,id,kDefaultIntermediatePoints);
    CHECK(editableConnector(d,id).geometry.size()==static_cast<std::size_t>(kDefaultIntermediatePoints)+2);
    // Each change re-lays the shape rather than reproducing it, so a round trip drifts: 0.88 m
    // on a 55.6 m road, 1.6 per cent of its length, against the 6.61 m a reset would cost.
    CHECK(apart(connectorRoad(d.network,editableConnector(d,id)),original)<1);
    // Zero points is a legal count and leaves one span between the two attachments.
    resampleConnectorPoints(d,id,0);CHECK(editableConnector(d,id).geometry.size()==2);
    for(const int bad:{-1,41})test::throws([&]{resampleConnectorPoints(d,id,bad);},"EDIT_CONNECTOR_POINTS");
}
// A straightened Connector is two points and a straight road. Sampling it must add nothing.
TEST(spline, a_straightened_connector_is_left_exactly_as_it_is) {
    auto d=roads();const auto id=addConnector(d,{"in","in-1"},{"out","out-1"});
    resetConnectorCurve(d,id,true);
    const auto& c=editableConnector(d,id);
    CHECK(c.geometry.size()==2);
    std::vector<std::size_t> indices;
    const auto road=connectorRoad(d.network,c,&indices);
    CHECK(road==c.geometry);CHECK((indices==std::vector<std::size_t>{0,1}));
}
// The owner's picture: a Connector drawn where two links nearly touch, turning back on itself.
// The arc reach that shapes the default curve runs away as the turn approaches 180 degrees, and
// the curve left the junction altogether -- 11.0 times its own chord, measured.
TEST(spline, a_hairpin_between_touching_links_stays_inside_its_own_junction) {
    ProjectDocument d;d.network.drivingSide=DrivingSide::left;
    d.network.links={{"m",{{-60,60},{60,-60}},{{"m1",3.5}}},{"v",{{0,-80},{0,80}},{{"v1",3.5},{"v2",3.5}}}};
    const auto id=addConnector(d,{"m","m1",polylineLength(d.network.links[0].geometry)/2-2},
                                 {"v","v1",polylineLength(d.network.links[1].geometry)/2+2});
    const auto& c=editableConnector(d,id);
    const auto road=connectorRoad(d.network,c);
    const auto [entry,exit]=connectorTangents(d.network,c.from,c.to);
    // The forcing: this really is a hairpin -- the two lanes turn 135 degrees apart -- and the
    // two attachments really are less than a metre from each other.
    const double turn=std::abs(std::atan2(entry.x*exit.y-entry.y*exit.x,entry.x*exit.x+entry.y*exit.y));
    CHECK(turn>2);
    const double chord=std::hypot(road.back().x-road.front().x,road.back().y-road.front().y);
    CHECK(chord<1);
    // The road stays in the junction: 1.9 times its chord, against 11.0 before the reach was
    // held at its 120-degree value.
    CHECK(polylineLength(road)/chord<3);
    // It is still an undrivable turn for a 3.5 m lane, and it says so rather than quietly
    // drawing a sliver: this is the one advisory that stands between the author and a fold.
    const auto issues=connectorShapeIssues(d.network);
    CHECK(issues.size()==1);CHECK(issues.front().code=="TIGHT_CONNECTOR_RADIUS");
}
