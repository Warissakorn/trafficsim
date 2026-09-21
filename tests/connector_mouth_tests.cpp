#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
using namespace trafficsim;
// Split out of connector_shape_tests.cpp, which passed the 500-line guard (hard rule 6). That file
// keeps a Connector's BODY -- how wide the carriageway is, where its markings fall, how tight a
// bend it will draw. This one keeps its MOUTHS: since M1.18 each end is cut on the Link's own
// cross-section, and the assertions that this is exactly what it does belong together.
//
// Why the fixtures here are straight and constant-width. A mouth slide moves the two edges of a
// lane to different stations along their own curves, so on a curved or width-changing ribbon no
// index-for-index measure of the width is exact, and the shape tests bound it instead. Here the
// geometry is chosen so an exact measure exists, and everything is pinned to 1e-9.
namespace {
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
// A point on one of the Link's own lane edges, at the station a Connector attaches to it. Two of
// these give the Link's cross-section there, which is the line every mouth must now land on.
Point edgePoint(const Link& link,std::size_t boundary,DrivingSide side,double station) {
    const auto edge=laneBoundaryGeometry(link,boundary,side);
    return pointAlong(edge,matchedStation(link.geometry,edge,station));
}
// The worst perpendicular distance from a boundary end to the Link's cross-section at the
// attachment. Zero means the mouth is flush with the Link: on its line, at its angle. This is the
// measure `mouthSquareness` was replaced by -- that one asserted a square cut across the ribbon,
// which is precisely what M1.18 stopped drawing.
double mouthFlush(const std::vector<std::vector<Point>>& boundaries,Point a,Point b,bool start) {
    const double span=std::hypot(b.x-a.x,b.y-a.y);
    double worst=0;
    for(const auto& edge:boundaries) {
        const Point p=start?edge.front():edge.back();
        worst=std::max(worst,std::abs((b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x))/std::max(span,1e-12));
    }
    return worst;
}
// The carriageway between two boundaries at a mouth, square to the road. Along the mouth itself
// the reading is width/cos(arrival) -- that is what a flush mouth IS, so it cannot fail and proves
// nothing. Resolved onto the ribbon's own normal it is the width, which can.
double mouthWidth(const std::vector<Point>& a,const std::vector<Point>& b,bool start) {
    const Point p=start?a.front():a.back(),q=start?a[1]:a[a.size()-2];
    const Point along{start?q.x-p.x:p.x-q.x,start?q.y-p.y:p.y-q.y};
    const double length=std::hypot(along.x,along.y);
    if(length<1e-12)return 0;
    const Point end=start?b.front():b.back();
    return std::abs((end.x-p.x)*(-along.y/length)+(end.y-p.y)*(along.x/length));
}
// A two-lane Link crossed by a two-lane Connector arriving on its BODY at `arrival` radians, the
// case the owner reported. The Connector's spine is straight, so an exact width measure exists.
Network obliqueArrival(double heading,double arrival,int spans=4,double spacing=9) {
    Network n;n.id="mouth-sweep";n.drivingSide=DrivingSide::left;
    const Link main{"main",{{-60*std::cos(heading),-60*std::sin(heading)},
                            {60*std::cos(heading),60*std::sin(heading)}},{{"main-1",3.5},{"main-2",3.5}}};
    const auto lane=laneGeometry(main,"main-1",n.drivingSide);
    const auto meet=pointAlong(lane,polylineLength(lane)/2);
    std::vector<Point> spine;
    for(int i=spans;i>=0;--i)spine.push_back({meet.x-i*spacing*std::cos(arrival),meet.y-i*spacing*std::sin(arrival)});
    // The first lane is offset 1.75 m from the two-lane reference. Build the feed around
    // the attachment, not on it; the old fixture had a disconnected source geometry.
    const Point end{spine.front().x+1.75*std::sin(arrival),spine.front().y-1.75*std::cos(arrival)};
    n.links={main,{"feed",{{end.x-25*std::cos(arrival),end.y-25*std::sin(arrival)},
                           end},{{"feed-1",3.5},{"feed-2",3.5}}}};
    n.connectors={Connector{"c",{"feed","feed-1",{}},{"main","main-1",60.},spine,2,2}};
    return n;
}
}
// The milestone's own assertion. Every lane of the Connector meets the Link it is assigned to: the
// mouth lies ON the Link's cross-section, exactly, at every arrival angle -- and the ribbon it cuts
// is still the full lane the Links give it.
TEST(mouths, a_mouth_lands_on_the_links_cross_section_at_every_arrival_angle) {
    int oblique=0,aligned=0,matched=0,mirrored=0;
    // Every heading of the Link crossed with every heading of the arrival: the two are independent
    // and it is their difference that conditions the cut, so neither may be fixed.
    for(int linkDegrees=0;linkDegrees<360;linkDegrees+=30)
    for(int arrivalDegrees=0;arrivalDegrees<360;arrivalDegrees+=15) {
        const double heading=linkDegrees*std::numbers::pi/180;
        const double arrival=arrivalDegrees*std::numbers::pi/180;
        const auto n=obliqueArrival(heading,arrival);
        const auto issues=validateNetwork(n);
        if(!issues.empty())throw std::runtime_error("Invalid angle fixture "+std::to_string(linkDegrees)+"/"+
            std::to_string(arrivalDegrees)+": "+issues.front().code+" "+issues.front().path);
        const auto& c=n.connectors.front();
        const auto boundaries=connectorBoundaries(n,c);
        const auto fit=connectorMouthFit(n,c);
        const auto& main=n.links.front();
        const Point a=edgePoint(main,0,n.drivingSide,60.),b=edgePoint(main,2,n.drivingSide,60.);
        // The forcing, in two halves. This really is a BODY attachment, not the ordinary joint at
        // a Link's end face, and the cross-section really is a line of some length to land on.
        CHECK(!attachedAtLinkEnd(n,c.to,false));
        CHECK(std::hypot(b.x-a.x,b.y-a.y)>6.9);
        // How oblique: |cos| between the arrival and the Link's own axis. 0 is a right-angle
        // crossing, 1 is parallel. Counted so the sweep cannot quietly stop covering either.
        const double along=std::abs(std::cos(arrival-heading));
        if(along<std::cos(60*std::numbers::pi/180))++oblique;
        if(fit.target.residual<1e-9) {
            ++aligned;
            // Flush: every boundary end on the Link's own cross-section, to 1e-9.
            test::near(mouthFlush(boundaries,a,b,false),0,1e-9);
            // ... and, where the mouth's two OUTER edges land on the Link's two outer lane
            // edges, the whole cross-section agrees with the Link's: every interior divider on
            // the Link's divider, and so the middle of every Connector lane on the middle of the
            // Link lane it feeds. That is the milestone's own assertion, and it is not the same
            // statement as the one it is conditioned on -- the outer edges agreeing says nothing
            // about how the lanes between them are divided, which is exactly what went wrong
            // before: the mouth was flush and the right span, and its lanes were still spread
            // 1/cos(arrival) apart with their middles beside the Link's.
            //
            // Where they do not land on it -- a Connector arriving from the far side, whose lane
            // order runs opposite the Link's, or one so oblique that the compression floor holds
            // it back -- no ribbon can carry the Link's lane order without turning over between
            // its two ends, so the Connector keeps its own cross-section and only the flushness
            // above holds. Both are counted, so neither can quietly stop being exercised.
            const bool onLink=std::hypot(boundaries.front().back().x-a.x,boundaries.front().back().y-a.y)<1e-3 &&
                              std::hypot(boundaries.back().back().x-b.x,boundaries.back().back().y-b.y)<1e-3;
            if(onLink) {
                ++matched;
                for(std::size_t i=0;i+1<boundaries.size();++i) {
                    test::near(apart(boundaries[i],boundaries[i+1],boundaries[i].size()-1),3.5,5e-3);
                    const auto middle=laneAttachment(n,{"main",main.lanes[i].id,60.},false);
                    const Point cut{(boundaries[i].back().x+boundaries[i+1].back().x)/2,
                                    (boundaries[i].back().y+boundaries[i+1].back().y)/2};
                    test::near(std::hypot(cut.x-middle.x,cut.y-middle.y),0,5e-3);
                }
            } else ++mirrored;
        }
        // Folding is what killed M1.17's wedge. A slide along each boundary cannot change which
        // side of its neighbour it is on, so this holds whether or not the mouth reached the Link.
        std::vector<Point> ring(boundaries.front());
        ring.insert(ring.end(),boundaries.back().rbegin(),boundaries.back().rend());
        CHECK(crossings(ring)==0);
        CHECK(trimSelfIntersections(ring).size()==ring.size());
    }
    CHECK(oblique>0);   // the sweep really reached the steep arrivals this test is about ...
    CHECK(aligned>0);   // ... and really did align on the ordinary ones, rather than skipping all
    CHECK(matched>0);   // ... and the lane-for-lane case above really was exercised ...
    CHECK(mirrored>0);  // ... as was the far-side arrival it cannot hold for
}
// A Connector running along its Link is the one case where nothing needs correcting, and it must
// come out bit for bit as the plain offset drew it. This is what keeps M1.18 from disturbing the
// ordinary end-to-end joint, which is most of every network.
TEST(mouths, a_parallel_arrival_is_left_exactly_where_the_offset_put_it) {
    Network n;n.drivingSide=DrivingSide::left;
    n.links={{"a",{{0,0},{50,0}},{{"a1",3.5},{"a2",3.5}}},{"b",{{70,0},{120,0}},{{"b1",3.5},{"b2",3.5}}}};
    Connector c{"c",{"a","a1",{}},{"b","b1",{}},{},2,2,0,"default",{},"",{},{}};
    c.geometry=connectorCurve(n,c.from,c.to,3);
    n.connectors={c};
    const auto fit=connectorMouthFit(n,c);
    // The forcing: the Connector really does leave and arrive along the Links, so a cut on their
    // cross-sections is the square end already drawn and the slide must be exactly zero.
    const Point leaving{c.geometry[1].x-c.geometry[0].x,c.geometry[1].y-c.geometry[0].y};
    test::near(leaving.y,0,1e-12);
    for(const double s:fit.source.shift)test::near(s,0,1e-12);
    for(const double s:fit.target.shift)test::near(s,0,1e-12);
    // ... and so every boundary is the Link's own lane edge, continued, to the last bit.
    const auto boundaries=connectorBoundaries(n,c);
    for(std::size_t i=0;i<boundaries.size();++i) {
        const auto edge=laneBoundaryGeometry(n.links.front(),i,n.drivingSide);
        test::near(boundaries[i].front().x,edge.back().x,1e-12);
        test::near(boundaries[i].front().y,edge.back().y,1e-12);
    }
}
// Where the arrival is too steep for the room the Connector has, as much of the correction as fits
// is applied and the rest is REPORTED, in metres. The owner asked for that rather than a mouth
// that silently stops short: a number can be looked at, a look cannot.
TEST(mouths, a_connector_too_short_for_its_arrival_reports_what_it_could_not_reach) {
    // 70 degrees off the Link's axis, on a spine of four 1.5 m spans -- deliberately far too short
    // to carry the slide that a mouth this oblique needs.
    const double arrival=70*std::numbers::pi/180;
    const auto cramped=obliqueArrival(0,arrival,4,1.5);
    const auto& c=cramped.connectors.front();
    const auto fit=connectorMouthFit(cramped,c);
    const double length=polylineLength(c.geometry);
    // The forcing: the Connector really is shorter than the correction wants, and the two
    // transition zones really were held back to fit inside it rather than overlapping.
    CHECK(length<6.1);
    CHECK(fit.source.zone+fit.target.zone<=length+1e-9);
    CHECK(fit.target.residual>0.5);      // it really did fall short ...
    CHECK(fit.target.residual<6.);       // ... by a bounded amount, not without limit
    // Falling short is not failing: the ribbon is still a ribbon, and still does not fold.
    const auto boundaries=connectorBoundaries(cramped,c);
    std::vector<Point> ring(boundaries.front());
    ring.insert(ring.end(),boundaries.back().rbegin(),boundaries.back().rend());
    CHECK(crossings(ring)==0);
    // The same Connector with room to work reaches its Link exactly, which is what proves the
    // shortfall above was the length and not the angle.
    const auto roomy=obliqueArrival(0,arrival,4,20);
    const auto roomyFit=connectorMouthFit(roomy,roomy.connectors.front());
    test::near(roomyFit.target.residual,0,1e-9);
}
// As the arrival turns to face along the Link's cross-section, the slide that would reach it runs
// away to infinity. `offsetGeometry` already clamps the same runaway at a corner (kMiterLimit);
// this is the mouth's version of that clamp, and without it a mouth becomes a spike.
TEST(mouths, an_arrival_along_the_cross_section_is_clamped_rather_than_spiking) {
    // 89 degrees: the Connector arrives very nearly straight across the Link, where the
    // cross-section is nearly parallel to the ribbon itself.
    const double arrival=89*std::numbers::pi/180;
    const auto n=obliqueArrival(0,arrival,4,20);
    const auto& c=n.connectors.front();
    const auto fit=connectorMouthFit(n,c);
    const auto boundaries=connectorBoundaries(n,c);
    // The forcing: this really is the ill-conditioned direction, so an unclamped solve would run
    // away here rather than merely being large.
    CHECK(std::abs(std::cos(arrival))<0.02);
    CHECK(fit.target.residual>0.);       // it really is clamped short of the Link ...
    for(const double s:fit.target.shift)CHECK(std::isfinite(s));
    // ... and the mouth stays a mouth: finite, bounded by the limit, and not folded.
    double widest=0;
    for(const auto& edge:boundaries)
        widest=std::max(widest,std::hypot(edge.back().x-boundaries.front().back().x,
                                          edge.back().y-boundaries.front().back().y));
    CHECK(widest<30.);                   // a bounded spread, where an unclamped solve is unbounded
    CHECK(apart(boundaries.front(),boundaries.back(),boundaries.front().size()-1)>6.9);
    std::vector<Point> ring(boundaries.front());
    ring.insert(ring.end(),boundaries.back().rbegin(),boundaries.back().rend());
    CHECK(crossings(ring)==0);
}
