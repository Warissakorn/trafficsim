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
double away(const std::vector<Point>& line,Point p) {
    double best=1e300;
    for(std::size_t i=1;i<line.size();++i) {
        const double dx=line[i].x-line[i-1].x,dy=line[i].y-line[i-1].y,square=dx*dx+dy*dy;
        const double t=square>0?std::clamp(((p.x-line[i-1].x)*dx+(p.y-line[i-1].y)*dy)/square,0.,1.):0.;
        best=std::min(best,std::hypot(p.x-line[i-1].x-t*dx,p.y-line[i-1].y-t*dy));
    }
    return best;
}
double apart(const std::vector<Point>& a,const std::vector<Point>& b) {
    double worst=0;for(const auto& p:a)worst=std::max(worst,away(b,p));return worst;
}
// How far a polyline's corners are from the smooth arc they stand in for: the worst gap between
// the chord of each span and the arc it cuts across.
double chordSag(const std::vector<Point>& points,const std::vector<Point>& arc) {
    double worst=0;for(const auto& p:arc)worst=std::max(worst,away(points,p));return worst;
}
}
// Vissim draws a Connector straight between its intermediate points and does not smooth them:
// a Connector with two of them is three straight legs with a corner at each point, which is what
// the owner's Vissim screenshot shows. Ours is the same polyline a Link is.
TEST(points, a_connector_is_drawn_straight_between_its_intermediate_points) {
    auto d=roads();const auto id=addConnector(d,{"in","in-1"},{"out","out-1"});
    resampleConnectorPoints(d,id,2);
    const auto& c=editableConnector(d,id);
    // The forcing: two intermediate points, so four points and three legs, and the turn is a real
    // one -- a straight Connector would satisfy any claim about straight legs for free.
    CHECK(c.geometry.size()==4);
    const auto paths=connectorPaths(d.network,c);
    CHECK(paths.size()==1);
    // What is drawn and what is compiled is the stored polyline itself, corner for corner.
    CHECK(paths.front().geometry==c.geometry);
    const auto boundaries=connectorBoundaries(d.network,c);
    for(const auto& boundary:boundaries)CHECK(boundary.size()==c.geometry.size());
    // Each boundary is straight over each leg, because a leg is straight: its middle sample is
    // on the line between its ends, to machine precision, and there are no samples in between.
    for(const auto& boundary:boundaries)for(std::size_t i=1;i+1<boundary.size();++i) {
        const Point a{boundary[i].x-boundary[i-1].x,boundary[i].y-boundary[i-1].y};
        const Point b{boundary[i+1].x-boundary[i].x,boundary[i+1].y-boundary[i].y};
        const double turn=std::abs(std::atan2(a.x*b.y-a.y*b.x,a.x*b.x+a.y*b.y));
        CHECK(turn>0.05);   // and a real corner at every point, mitered, not rounded away
    }
    // Grips are the stored points and nothing else: one per corner, plus the two ends.
    CHECK(connectorCentreline(d.network,c).size()==c.geometry.size());
}
// What the count buys on a default Connector is how closely the polygon follows the turn, which
// is Vissim's own reason for the field: more points, a shape nearer the arc, more corners to drag.
TEST(points, a_default_connector_with_more_points_follows_the_turn_more_closely) {
    auto d=roads();const auto id=addConnector(d,{"in","in-1"},{"out","out-1"});
    const auto& c=editableConnector(d,id);
    const auto arc=connectorCurve(d.network,c.from,c.to,39);
    double previous=1e300;
    for(const int count:{1,2,3,7,15}) {
        const auto points=connectorCurve(d.network,c.from,c.to,count);
        // The forcing: the count really is the number of intermediate points, not of samples.
        CHECK(points.size()==static_cast<std::size_t>(count)+2);
        const double sag=chordSag(points,arc);
        CHECK(sag<previous);previous=sag;
    }
    // A 39-point reading of the same arc is what "the turn" means here, and 15 points is within
    // a tenth of a metre of it on a quarter turn of this size, against 2.29 m at one point.
    CHECK(previous<.1);
    // The default is the owner's 3, and 0 leaves one straight leg between the two attachments.
    CHECK(c.geometry.size()==static_cast<std::size_t>(kDefaultIntermediatePoints)+2);
    resampleConnectorPoints(d,id,0);CHECK(editableConnector(d,id).geometry.size()==2);
    for(const int bad:{-1,41})test::throws([&]{resampleConnectorPoints(d,id,bad);},"EDIT_CONNECTOR_POINTS");
    test::throws([&]{(void)connectorCurve(d.network,c.from,c.to,41);},"EDIT_CONNECTOR_POINTS");
}
// Vissim's field re-lays the shape the author has; it is not a Reset curve with extra steps.
TEST(points, changing_the_count_re_lays_the_shape_the_author_bent) {
    auto d=roads();const auto id=addConnector(d,{"in","in-1"},{"out","out-1"});
    auto bent=editableConnector(d,id).geometry;bent[2]={bent[2].x+5,bent[2].y-7};
    changeConnectorGeometry(d,id,bent);
    const auto original=editableConnector(d,id).geometry;
    const auto reset=connectorCurve(d.network,editableConnector(d,id).from,editableConnector(d,id).to);
    // The forcing: the shape really is the author's, 5.9 m away from the one a reset would give.
    CHECK(apart(reset,original)>5);
    resampleConnectorPoints(d,id,7);
    CHECK(editableConnector(d,id).geometry.size()==9);
    const auto richer=editableConnector(d,id).geometry;
    // Raising the count keeps the author's line exactly, both ways: the new points are added by
    // splitting the longest leg, so every point that was there is still there. Re-laying at even
    // spacing instead cut the bent corner by 1.00 m.
    CHECK(apart(richer,original)<1e-12);
    CHECK(apart(original,richer)<1e-12);
    // Going back down gives up the detail three points cannot hold, never the shape: the round
    // trip leaves every point on the author's own line, where a reset is 5.58 m away from it.
    resampleConnectorPoints(d,id,kDefaultIntermediatePoints);
    const auto returned=editableConnector(d,id).geometry;
    CHECK(returned.size()==original.size());
    CHECK(apart(returned,original)<1e-9);
    CHECK(apart(reset,original)>5);
    CHECK(apart(reset,original)>4*apart(returned,original));
}
// The owner's picture: a Connector drawn where two links nearly touch, turning back on itself.
// The arc reach that lays the default points runs away as the turn approaches 180 degrees, and
// the shape left the junction altogether -- 11.0 times its own chord, measured.
TEST(points, a_hairpin_between_touching_links_stays_inside_its_own_junction) {
    ProjectDocument d;d.network.drivingSide=DrivingSide::left;
    d.network.links={{"m",{{-60,60},{60,-60}},{{"m1",3.5}}},{"v",{{0,-80},{0,80}},{{"v1",3.5},{"v2",3.5}}}};
    const auto id=addConnector(d,{"m","m1",polylineLength(d.network.links[0].geometry)/2-2},
                                 {"v","v1",polylineLength(d.network.links[1].geometry)/2+2});
    const auto& c=editableConnector(d,id);
    const auto [entry,exit]=connectorTangents(d.network,c.from,c.to);
    // The forcing: this really is a hairpin -- the two lanes turn 135 degrees apart -- and the
    // two attachments really are less than a metre from each other.
    const double turn=std::abs(std::atan2(entry.x*exit.y-entry.y*exit.x,entry.x*exit.x+entry.y*exit.y));
    CHECK(turn>2);
    const double chord=std::hypot(c.geometry.back().x-c.geometry.front().x,
                                  c.geometry.back().y-c.geometry.front().y);
    CHECK(chord<1);
    // It stays in the junction: 1.9 times its chord, against 11.0 before the reach was held at
    // its 120-degree value.
    CHECK(polylineLength(c.geometry)/chord<3);
    // It is still an undrivable turn for a 3.5 m lane, and it says so rather than quietly
    // drawing a sliver: this is the one advisory that stands between the author and a fold.
    const auto issues=connectorShapeIssues(d.network);
    CHECK(issues.size()==2);CHECK(issues[0].code=="WARN_SHORT_CONNECTOR");
    CHECK(issues[1].code=="TIGHT_CONNECTOR_RADIUS");
}
