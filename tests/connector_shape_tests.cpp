#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <fstream>
using namespace trafficsim;
// Split out of connector_tests.cpp, which passed the 500-line guard (hard rule 6). That file keeps
// connector TOPOLOGY -- creation, references, retargeting, deletion, history. This one keeps its
// SHAPE: how wide the carriageway is, where its markings fall, how tight a bend it will draw, and
// the wedge at its mouth. The measuring helpers below belong with the assertions that use them.
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
// How wide the lane is square to the road, rather than along the cross-section, which is the lane
// width by construction whatever angle that cross-section sits at and so cannot fail.
double perpendicular(const std::vector<Point>& edge,Point p) {
    double best=1e300;
    for(std::size_t i=1;i<edge.size();++i) {
        const double dx=edge[i].x-edge[i-1].x,dy=edge[i].y-edge[i-1].y,square=dx*dx+dy*dy;
        const double t=square>0?std::clamp(((p.x-edge[i-1].x)*dx+(p.y-edge[i-1].y)*dy)/square,0.,1.):0.;
        best=std::min(best,std::hypot(p.x-edge[i-1].x-t*dx,p.y-edge[i-1].y-t*dy));
    }
    return best;
}
// A Connector's ends are square to its own axis: no cut on the Link's cross-section, no wedge.
// How far off one straight line across the road the worst boundary end is (still zero -- the ends
// are all offsets of the same spine vertex), and how far that line is from square to the spine.
double mouthLine(const std::vector<std::vector<Point>>& boundaries,bool start) {
    const auto at=[&](std::size_t i){return start?boundaries[i].front():boundaries[i].back();};
    const auto a=at(0),b=at(boundaries.size()-1);
    const double span=std::hypot(b.x-a.x,b.y-a.y);
    double worst=0;
    for(std::size_t i=0;i<boundaries.size();++i)
        worst=std::max(worst,std::abs((b.x-a.x)*(at(i).y-a.y)-(b.y-a.y)*(at(i).x-a.x))/std::max(span,1e-12));
    return worst;
}
// `mouthSquareness` lived here until M1.18: it measured the cosine between the mouth line and the
// boundaries' own end legs, and asserting it zero was asserting a square cut across the ribbon.
// The mouth is cut on the Link's cross-section now, so that measure is deliberately non-zero and
// has been replaced by `mouthFlush` in `connector_mouth_tests.cpp`, which measures the thing that
// matters instead: how far the ends are from the Link's own cross-section line.
// How far a boundary end stands from the Link lane edge it attaches to. Since M1.18 the mouth is
// cut on the Link's cross-section, so a boundary end lands ALONG that cut at width/cos(arrival)
// rather than on the Link's own lane edge: this distance is the spread, not an error.
double stepTo(Point end,Point edge){return std::hypot(end.x-edge.x,end.y-edge.y);}
// An edge continued straight off both of its ends, so a point near a mouth still has a leg of the
// far edge to measure against. Needed because the mouth slide leaves the two edges ending at
// different stations: measured against the edge as stored, a point past its end would report the
// distance to that endpoint instead of to the road, which reads as a lane several metres wide.
std::vector<Point> extended(const std::vector<Point>& edge) {
    const auto lead=[](Point p,Point q){return Point{p.x+(p.x-q.x)*50,p.y+(p.y-q.y)*50};};
    std::vector<Point> result{lead(edge.front(),edge[1])};
    result.insert(result.end(),edge.begin(),edge.end());
    result.push_back(lead(edge.back(),edge[edge.size()-2]));
    return result;
}
// The carriageway between two boundaries at one sample, square to the road, with the far edge
// extended so a mouth sample has something to measure against. Correct wherever the two edges are
// parallel; where the far one leans -- a lane tapering closed -- use `mouthWidth` below instead.
double squareWidth(const std::vector<Point>& a,const std::vector<Point>& b,std::size_t j) {
    return perpendicular(extended(b),a[j]);
}
// The same width at a mouth where the FAR edge leans -- a lane tapering closed converges on its
// neighbour, so a perpendicular dropped onto it reads short by construction (2.945 m of a 3 m
// lane here) and always did. Resolving the two ends' separation onto the ribbon's own normal
// instead removes the mouth's spread without asking the far edge to be parallel.
double mouthWidth(const std::vector<Point>& a,const std::vector<Point>& b,bool start) {
    const Point p=start?a.front():a.back(),q=start?a[1]:a[a.size()-2];
    const Point along{start?q.x-p.x:p.x-q.x,start?q.y-p.y:p.y-q.y};
    const double length=std::hypot(along.x,along.y);
    if(length<1e-12)return 0;
    const Point end=start?b.front():b.back();
    return std::abs((end.x-p.x)*(-along.y/length)+(end.y-p.y)*(along.x/length));
}
// Measured square to the road. The two end samples are excluded when `body`: a mouth cut on the
// link is a wedge on purpose, so its corner is nearer the far edge than a full width, and
// measuring it square would forbid the very thing Vissim draws.
double narrowest(const std::vector<std::vector<Point>>& boundaries,bool body=false) {
    double best=1e300;const auto& front=boundaries.front();
    for(std::size_t j=body?1:0;j+(body?1:0)<front.size();++j)
        best=std::min(best,perpendicular(boundaries.back(),front[j]));
    return best;
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
        // The continuing lane is 3 m where it leaves and 3.5 m where it arrives, exactly, and the
        // width in between to within a millimetre of the miter on a tapering neighbour -- never
        // the 2.6 m pinch that came of shrinking every lane together.
        const bool end=j==0 || j+1==weights.size();
        // One measure now, end to end: the separation of the two edges at the same index. Through
        // the body that is the cross-section square to the ribbon; at a mouth it is the cut on the
        // Link, and the lane there is the LINK's own lane, exactly -- 3 m where it leaves, 3.5 m
        // where it arrives. The mouth is no longer the looser of the two readings, it is the exact
        // one, which is what putting the mouth on the Link's own lane boundaries bought.
        test::near(apart(boundaries[0],boundaries[1],j),3+.5*weights[j],end?1e-9:1e-3);
        (void)end;
        const double wedge=apart(boundaries[1],boundaries[2],j);
        CHECK(wedge<previous);previous=wedge;
    }
    test::near(previous,0,1e-9);
    // Square to the road as well as along the cross-section: the along measure is the lane width
    // by construction, so on its own it cannot tell a cross-section that has been left behind by
    // the curve from one that follows it.
    for(const auto& p:boundaries.front())CHECK(perpendicular(boundaries[1],p)>2.9);
    // Each mouth is one straight line across the road. Since M1.18 that line is the Link's own
    // cross-section rather than a square cut across the ribbon, so it is deliberately NOT square
    // to the Connector's axis any more; `connector_mouth_tests.cpp` asserts it lies on the Link.
    test::near(mouthLine(boundaries,true),0,1e-9);
    test::near(mouthLine(boundaries,false),0,1e-9);
    const auto meets=[&](Point a,Point b){CHECK(stepTo(a,b)<.25);};
    for(std::size_t i=0;i<3;++i)meets(boundaries[i].front(),laneBoundaryGeometry(in,i,d.network.drivingSide).back());
    meets(boundaries[0].back(),laneBoundaryGeometry(out,0,d.network.drivingSide).front());
    meets(boundaries[1].back(),laneBoundaryGeometry(out,1,d.network.drivingSide).front());
    test::near(boundaries[2].back().x,boundaries[1].back().x,1e-9);
    test::near(boundaries[2].back().y,boundaries[1].back().y,1e-9);
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
        // Exact where the links fix it, and close to the straight interpolation in between,
        // where each lane's two edges converge at their own rate. The 8 cm allowed at a corner
        // is the miter (6.3 cm measured): along the cross-section a mitered corner reads wide,
        // exactly as a Link's own edges do at a bend. Square to the road it is the lane width,
        // which is what the perpendicular check above measures.
        // The body is untouched by M1.18 and is asserted exactly as it always was: close to the
        // straight interpolation, each lane's two edges converging at their own rate, within the
        // 8 cm the miter costs at a corner (6.3 cm measured). The two mouths are the samples the
        // mouth slide moves, and there the distance is measured ALONG the Link's cross-section, so
        // it reads the lane width over the cosine of the arrival: 3.118 m for this 3 m lane and
        // 4.118 m for the 4 m one, a 3-4% spread on this near-square pair of joints. Bounded here
        // rather than pinned, because on a 90-degree turn between two five-point edges no
        // index-for-index measure is exact once the two ends sit at different stations;
        // `connector_mouth_tests.cpp` pins width and flushness to 1e-9 on fixtures where it is.
        if(j==0 || j+1==both[0].size()) {
            CHECK(apart(both[0],both[1],j)>=3+pairWeights[j]-1e-9);
            CHECK(apart(both[0],both[1],j)<=(3+pairWeights[j])*1.05);
            CHECK(apart(both[1],both[2],j)>=4-pairWeights[j]-1e-9);
            CHECK(apart(both[1],both[2],j)<=(4-pairWeights[j])*1.05);
        } else {
            test::near(apart(both[0],both[1],j),3+pairWeights[j],8e-2);   // in-1 3 m into out-1 4 m
            test::near(apart(both[1],both[2],j),4-pairWeights[j],8e-2);   // in-2 4 m into out-2 3 m
        }
    }
    CHECK(connectorMarkings(wide.network,editableConnector(wide,pair))[1].geometry.size()==both[1].size());
    auto plain=roads();const auto single=addConnector(plain,{"in","in-2"},{"other","other-1"});
    CHECK(connectorMarkings(plain.network,editableConnector(plain,single)).size()==2);
    // A diverge is the mirror image: the lane that opens out starts at nothing and grows.
    auto open=roads();const auto out2=addConnectorRange(open,{"other","other-1"},{"out","out-1"},1,2);
    const auto opening=connectorBoundaries(open.network,editableConnector(open,out2));
    // The closed end stays exactly closed -- a lane tapered to nothing leaves ON its neighbour,
    // and the mouth slide is made to keep it there rather than prise it back open by millimetres.
    test::near(apart(opening[1],opening[2],0),0,1e-9);
    // ... and opens to the Link's own 3 m lane at the far end, exactly. This used to be bounded
    // rather than pinned (2.8 to 3.2, on readings of 2.945 and 2.868 m) because the mouth was cut
    // square across the ribbon and then slid: the two ends of the lane came to rest at different
    // stations and no index-for-index measure of it was exact. The mouth is now built ON the
    // Link's own lane boundaries, so the measure along the cut is the Link's lane, to 1e-9.
    test::near(apart(opening[1],opening[2],opening[1].size()-1),3,1e-5);
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
// The cross-section has to turn with the road. Interpolating between the two mouths does not: on a
// reverse curve they are parallel, so it stood still while the path swung away and the lane was
// drawn at its own width times the cosine of that angle.
TEST(connectors, a_drawn_lane_keeps_its_width_square_to_the_road) {
    struct Shape { const char* name; std::vector<Point> geometry; double least; };
    // Each is the same 3.5 m lane; `least` is what it measured before the cross-section followed
    // the road -- 1.06 m on the reverse curve, and 0.36 m on the turn if the two end corrections
    // are wrapped independently instead of the second against the first.
    for(const auto& shape:std::vector<Shape>{
            {"reverse curve",{{10,-12},{70,-12}},1.1},
            {"gentle reverse",{{20,-12},{80,-12}},2.5},
            {"quarter turn",{{20,20},{20,80}},0.4},
            {"u-turn",{{0,-10},{-60,-10}},3.4}}) {
        ProjectDocument d;d.network.drivingSide=DrivingSide::left;
        d.network.links={{"a",{{-60,0},{0,0}},{{"a1",3.5}}},{"b",shape.geometry,{{"b1",3.5}}}};
        const auto id=addConnector(d,{"a","a1"},{"b","b1"});
        const auto boundaries=connectorBoundaries(d.network,editableConnector(d,id));
        // The forcing: the shape really does bend, so a cross-section that ignored it would show.
        const auto& g=editableConnector(d,id).geometry;
        CHECK(minimumRadius(g)<40);
        CHECK(boundaries.size()==2);
        // Each mouth is still ONE straight line across the road, and that line is the Link's own
        // cross-section rather than a square cut across the ribbon, so it is not square to the
        // Connector's axis. `connector_mouth_tests.cpp` asserts it lies on the Link to 1e-9; here
        // the point is that the lane it cuts is the Link's own 3.5 m lane, measured along the cut.
        test::near(mouthLine(boundaries,true),0,1e-9);
        test::near(mouthLine(boundaries,false),0,1e-9);
        // 1e-5, not 1e-9: the offset each boundary leaves the mouth at is a fixed point solved
        // against the end leg it itself produces. The residue is 2 microns on a 3.5 m lane.
        test::near(apart(boundaries[0],boundaries[1],0),3.5,1e-5);
        test::near(apart(boundaries[0],boundaries[1],boundaries[0].size()-1),3.5,1e-5);
        // And it does not merely lie ON the Link's cross-section, it lies on the Link's own lane
        // EDGES: the spread along the cut that M1.18 left -- 3.5 cm on the quarter turn, 25.6 cm
        // on the reverse curve -- is gone, because the mouth is now built from those edges.
        for(std::size_t i=0;i<2;++i)
            CHECK(stepTo(boundaries[i].front(),laneBoundaryGeometry(d.network.links[0],i,DrivingSide::left).back())<1e-5);
        const double least=narrowest(boundaries,true);
        CHECK(least>shape.least);      // beats what the mouth-to-mouth cross-section drew
        CHECK(least>.9*3.5);           // and is the lane the links actually give it
    }
}
// The shape the owner reported: draw a Connector, then move the Link it arrives at. Vissim moves
// the one poly point attached to that Link; the ribbon must still be a road afterwards, not the
// sliver the mouth-to-mouth cross-section drew once the curve no longer left the lane straight.
TEST(connectors, a_moved_link_leaves_the_connector_its_width) {
    for(const double degrees:{30.,60.,90.}) {
        ProjectDocument d;d.network.drivingSide=DrivingSide::left;
        d.network.links={{"a",{{-60,0},{0,0}},{{"a1",3.5}}},{"b",{{20,0},{80,0}},{{"b1",3.5}}}};
        const auto id=addConnector(d,{"a","a1"},{"b","b1"});
        const auto drawn=editableConnector(d,id).geometry;
        const double radians=degrees*std::numbers::pi/180;
        changeGeometry(d,"b",{{20,0},{20+60*std::cos(radians),60*std::sin(radians)}});
        const auto& moved=editableConnector(d,id);
        // The forcing: only the attached point moved, and it really did move a long way.
        CHECK(moved.geometry.size()==drawn.size());
        for(std::size_t i=0;i+1<drawn.size();++i) {
            test::near(moved.geometry[i].x,drawn[i].x,1e-12);test::near(moved.geometry[i].y,drawn[i].y,1e-12);
        }
        // ... and the Connector now arrives across the lane rather than along it, which is the
        // state that used to fold the ribbon flat.
        const auto& b=*std::find_if(d.network.links.begin(),d.network.links.end(),
                                    [](const auto& l){return l.id=="b";});
        const auto lane=laneGeometry(b,"b1",DrivingSide::left);
        const Point arrival{moved.geometry.back().x-moved.geometry[moved.geometry.size()-2].x,
                            moved.geometry.back().y-moved.geometry[moved.geometry.size()-2].y};
        const Point along{lane[1].x-lane[0].x,lane[1].y-lane[0].y};
        const double between=std::abs(std::atan2(arrival.x*along.y-arrival.y*along.x,
                                                 arrival.x*along.x+arrival.y*along.y));
        CHECK(between>degrees*std::numbers::pi/180*.8);
        const auto boundaries=connectorBoundaries(d.network,moved);
        const auto fit=connectorMouthFit(d.network,moved);
        // The mouth is one straight line at every arrival angle, now cut on the Link's own
        // cross-section rather than square across the ribbon (M1.18).
        test::near(mouthLine(boundaries,false),0,1e-9);
        // At 30 and 60 degrees the mouth reaches its Link exactly, and the ribbon it cuts is still
        // the 3.5 m lane square to the road. At 90 the Connector arrives straight across the lane,
        // where a cut on the cross-section lies along the ribbon itself and no finite slide meets
        // it: the fit reports what is left standing off, in metres, rather than pretending.
        if(degrees<90) {
            test::near(fit.target.residual,0,1e-9);
            // Along the cut, which is where the Connector meets the Link: its lane there is the
            // Link's own 3.5 m lane. Square to the Connector it reads that times the cosine of the
            // arrival -- 3.03 m at 30 degrees, 1.75 m at 60 -- which is what a road crossing
            // another at an angle measures, and is why that is no longer what is asserted.
            // 5e-3: where the slide is longer than the boundary's own end leg it follows the leg
            // AFTER that one too, so the landing point leaves the straight line the offset was
            // solved against. 2.8 mm on a 3.5 m lane at 60 degrees, and it is a fixed point --
            // measured unchanged at 8, 16 and 32 passes, so it is the geometry, not convergence.
            test::near(apart(boundaries[0],boundaries[1],boundaries[0].size()-1),3.5,5e-3);
        } else {
            CHECK(fit.target.residual>1.);   // the forcing: this really is the unreachable case ...
            CHECK(fit.target.residual<2.);   // ... and it is bounded, not running away
        }
        for(std::size_t i=0;i<2;++i)
            // The remaining distance to the Link's own lane edge is the SPREAD along the cut,
            // which grows as the arrival steepens: 0.27 m at 30 degrees, 1.75 m at 60, and 8.93 m
            // at 90, where the Connector arrives straight across the lane and a cut on the
            // cross-section is nearly parallel to the ribbon. That spread is the trade the owner
            // took for a flush mouth; `connectorMouthFit` reports what it could not reach.
            CHECK(stepTo(boundaries[i].back(),laneBoundaryGeometry(b,i,DrivingSide::left).front())<9.);
        // At 30 and 60 degrees there is no spread left at all: the mouth is ON the Link's own lane
        // edges, not merely on the line through them. 0.27 m and 1.75 m stood there before.
        if(degrees<90)for(std::size_t i=0;i<2;++i)
            CHECK(stepTo(boundaries[i].back(),laneBoundaryGeometry(b,i,DrivingSide::left).front())<5e-3);
        // The BODY is still a 3.5 m lane square to the road, at every interior sample: the
        // interpolated cross-section drew 1.96 m at 60 degrees and 0.46 m at 90. The two mouth
        // samples are excluded from this measure on purpose -- a mouth is a cut on the Link's
        // cross-section, so square to the Connector it reads 3.5 m times the cosine of the
        // arrival, and the exact measure of it is the one along the cut, asserted above.
        // The middle of the body is the full 3.5 m square to the road, exactly -- the interpolated
        // cross-section drew 1.96 m at 60 degrees and 0.46 m at 90 there. The samples either side
        // lie inside a mouth's transition zone, part-way through opening out from the Link's cut
        // to the Connector's own width, and read up to 3.1 cm narrow square to the road at 90
        // degrees: the opening out, not a pinch, so bounded here rather than pinned.
        test::near(squareWidth(boundaries[0],boundaries[1],boundaries[0].size()/2),3.5,1e-9);
        for(std::size_t j=1;j+1<boundaries[0].size();++j)
            CHECK(squareWidth(boundaries[0],boundaries[1],j)>3.45);
        test::near(apart(boundaries[0],boundaries[1],0),3.5,5e-3);   // the source mouth, on its Link
    }
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
// M1.12.2 was booked as a defect: "a 2->2 Connector through a sharp bend bulges to 8.698 m of a
// 7.000 m width, 24% over". Measured three ways, that is a MEASUREMENT artifact, not a bulge:
//
//   along the cross-section at the mitered vertex : 9.9403 m  (+42%)
//   perpendicular, point to the far polyline      : 7.0425 m  (+0.6%)
//   projected across the leg the vertex lies on   : 7.000000 m (exact)
//
// A mitered corner's diagonal IS width/cos(phi/2) -- that is what the intersection of two offset
// legs is, and it is what a road painted round a kink actually measures corner to corner. The
// carriageway square to the road is untouched. `bends_keep_their_full_carriageway_width` already
// asserted both halves of this deliberately, and the 8e-2 tolerance above already says so in as
// many words. So offsetGeometry is NOT to be "fixed": network_tests pins the miter to 1e-9 and
// changing it would pinch every bend in the editor, which is the defect the miter was added to fix.
//
// What WAS missing is an upper bound: width was only ever checked from below (`least > .9*3.5`),
// which is how a claim of 24% over went unchallenged for a session. This is that bound.
TEST(connectors, a_bent_connector_holds_its_width_square_to_the_road_from_both_sides) {
    Network n; n.drivingSide=DrivingSide::left;
    n.links={{"a",{{0,0},{60,0}},{{"a1",3.5},{"a2",3.5}},0,"default",0,""},
             {"b",{{90,60},{90,140}},{{"b1",3.5},{"b2",3.5}},0,"default",0,""}};
    Connector c{"c",{"a","a1",60},{"b","b1",0},{},2,2,0,"default",{},"",{},{}};
    c.geometry=connectorCurve(n,c.from,c.to,3);
    c.geometry[2]={95,10}; // Bent hard by hand, the way an author dragging a poly point would.
    n.connectors={c};
    const auto b=connectorBoundaries(n,c);
    const auto& spine=b[1];
    // The forcing: this really is a hard bend, and it really does read wide ALONG the cross
    // section. Without this the exact widths below would be measuring a gentle curve and would
    // prove nothing about the case that was reported.
    double worstAlong=0;
    for(std::size_t j=0;j<spine.size();++j)
        worstAlong=std::max(worstAlong,apart(b.front(),b.back(),j));
    CHECK(worstAlong>8.5);
    // And the bend really is hard: 90.47 degrees of deflection measured, which is what makes
    // 1/cos(phi/2) = 1.4200 and so 7.000 m read as 9.94 m along the cross-section.
    double sharpest=0;
    for(std::size_t i=1;i+1<spine.size();++i) {
        const double a1=std::atan2(spine[i].y-spine[i-1].y,spine[i].x-spine[i-1].x);
        const double a2=std::atan2(spine[i+1].y-spine[i].y,spine[i+1].x-spine[i].x);
        double turn=std::abs(a2-a1);
        while(turn>std::acos(-1.))turn=2*std::acos(-1.)-turn;
        sharpest=std::max(sharpest,turn);
    }
    CHECK(sharpest>1.5); // radians, i.e. sharper than 85 degrees
    // Square to the road, on every leg of the body, the carriageway is its full 7 m -- bounded
    // from ABOVE as well as below, which is the check that was missing.
    // Interior legs only. Kept from when the wedge mouth (M1.17, since reverted) moved the end
    // vertices; the ends are square cuts again, so this exclusion is now only conservative.
    std::size_t measured=0;
    for(std::size_t i=2;i+2<spine.size();++i) {
        const double dx=spine[i].x-spine[i-1].x,dy=spine[i].y-spine[i-1].y;
        const double length=std::hypot(dx,dy);
        if(length<=0)continue;
        const Point across{-dy/length,dx/length};
        for(std::size_t j=i-1;j<=i;++j) {
            const double width=std::abs((b.front()[j].x-b.back()[j].x)*across.x+
                                        (b.front()[j].y-b.back()[j].y)*across.y);
            // No bulge, and exactly so. This is the assertion M1.12.2 was really about, and the
            // one that was missing: width had only ever been bounded from below.
            test::near(width,7.0,1e-9);
            ++measured;
        }
    }
    CHECK(measured>0); // The loop really ran rather than skipping every leg.
    // The same both ways round: neither edge is measured as the privileged one.
    CHECK(narrowest(b,true)>7.0-1e-2);
}
// The oblique-arrival sweep that lived here moved to `connector_mouth_tests.cpp` with M1.18: what
// it was really about -- where a mouth lands and whether the ribbon folds getting there -- is that
// file's subject now, and it was also what pushed this one past the 500-line guard.
