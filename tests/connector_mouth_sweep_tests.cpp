#include "test.hpp"
#include "../src/model/network/connector_surface.hpp"
#include <algorithm>
#include <numbers>
#include <string>
using namespace trafficsim;
// M3.2.9b/e: across arrival angles at both ends, lane counts, offset ranges, unequal lane widths and
// both driving sides, every interior divider ends on the Link lane boundary it belongs to.
namespace {
double distance(Point a,Point b) { return std::hypot(a.x-b.x,a.y-b.y); }
// Off the boundary's LINE at its nearest station: at a Link's end the mouth may sit on the
// edge's continuation past the last vertex, which is where P1 and P4 are defined to be.
double toLine(const std::vector<Point>& g,Point p) {
    const double at=stationOfClosestPoint(g,p);const auto q=pointAlong(g,at),u=directionAlong(g,at,true);
    return std::abs((p.x-q.x)*u.y-(p.y-q.y)*u.x);
}
Point heading(double degrees) {
    const double r=degrees*std::numbers::pi/180;return {std::cos(r),std::sin(r)};
}
struct Case { double target,source; int from,to,offset; DrivingSide side; };
std::string name(const Case& k) {
    return "target "+std::to_string(int(k.target))+" source "+std::to_string(int(k.source))+" lanes "+
        std::to_string(k.from)+"->"+std::to_string(k.to)+" offset "+std::to_string(k.offset)+
        (k.side==DrivingSide::left?" left":" right");
}
Network build(const Case& k) {
    const double widths[2]={3.5,3.0};
    Network n;n.id="sweep";n.drivingSide=k.side;
    Link main;main.id="main";main.geometry={{-100,0},{100,0}};
    for(int i=0;i<=k.to;++i)main.lanes.push_back({"m"+std::to_string(i),widths[i%2]});
    const Point u=heading(k.target),start{-30*u.x,-30*u.y},v=heading(k.target+k.source);
    Link feed;feed.id="feed";feed.geometry={{start.x-40*v.x,start.y-40*v.y},start};
    for(int i=0;i<=k.from;++i)feed.lanes.push_back({"f"+std::to_string(i),widths[(i+1)%2]});
    n.links={main,feed};
    Connector c;c.id="c";c.fromLaneCount=k.from;c.toLaneCount=k.to;
    c.from={"feed","f"+std::to_string(k.offset),{}};c.to={"main","m"+std::to_string(k.offset),100};
    for(int i=0;i<=4;++i)c.geometry.push_back({start.x+i*7.5*u.x,start.y+i*7.5*u.y});
    n.connectors={c};anchorConnectorEnds(n,n.connectors.front());
    return n;
}
// Every point the mouth hands the dividers lies on a boundary of the attached lane range, and
// every painted divider ends on one of those points.
int check(const Network& n,const ConnectorSurface& s,bool start,const std::string& label) {
    const auto& m=start?s.source:s.target;
    if(!m)return 0;
    const auto& c=n.connectors.front();
    const auto& ref=start?c.from:c.to;const int count=start?c.fromLaneCount:c.toLaneCount;
    const auto& link=*std::find_if(n.links.begin(),n.links.end(),[&](const auto& l){return l.id==ref.linkId;});
    int first=0;while(link.lanes[static_cast<std::size_t>(first)].id!=ref.laneId)++first;
    std::vector<std::vector<Point>> edges;
    for(int b=first;b<=first+count;++b)edges.push_back(laneBoundaryGeometry(link,static_cast<std::size_t>(b),n.drivingSide));
    if(m->boundaries.size()!=static_cast<std::size_t>(count+1))throw std::runtime_error(label+": boundary count");
    for(std::size_t j=0;j<m->boundaries.size();++j) {
        // In order: the j-th point is on the j-th boundary counted from the first rail's edge.
        const double onFirst=toLine(edges[j],m->boundaries[j]);
        if(onFirst>1e-2)throw std::runtime_error(label+": mouth point off its Link boundary");
    }
    // D76: and on the Connector's own divider line, as P1/P4 are on its edge lines -- offset from
    // the first rail's end by the widths of the lanes before it, square to the end direction.
    const auto axis=connectorCentreline(n,c);
    const auto u=directionAlong(axis,start?0:polylineLength(axis),!start);
    const auto widths=connectorLaneWidths(n,c);const auto& w=start?widths.source:widths.target;
    double offset=0;std::size_t lane=0;
    for(std::size_t j=1;j+1<m->boundaries.size();++j) {
        while(lane<w.size() && w[lane]<=0)++lane;
        offset+=w[lane++];
        const Point d{m->boundaries[j].x-m->boundaries[0].x,m->boundaries[j].y-m->boundaries[0].y};
        const double lateral=std::abs(d.x*u.y-d.y*u.x);
        if(std::abs(lateral-offset)>1e-2)
            throw std::runtime_error(label+": divider point "+std::to_string(lateral)+" m across, not "+std::to_string(offset));
    }
    int dividers=0;
    for(const auto& marking:s.markings) {
        if(marking.edge)continue;
        const Point end=start?marking.geometry.front():marking.geometry.back();
        double best=1e300;for(auto p:m->boundaries)best=std::min(best,distance(p,end));
        if(best>1e-2)throw std::runtime_error(label+": divider ends "+std::to_string(best)+" m off the mouth");
        ++dividers;
    }
    return dividers;
}
}
TEST(mouth_sweep, every_divider_ends_on_its_link_boundary) {
    int cases=0,mouths=0,dividers=0;
    for(double target:{30.,45.,60.,90.,120.,135.,150.})for(double source:{0.,30.,60.,90.,-45.,135.})
    for(int from=1;from<=3;++from)for(int to=1;to<=3;++to)for(int offset=0;offset<2;++offset)
    for(auto side:{DrivingSide::right,DrivingSide::left}) {
        const Case k{target,source,from,to,offset,side};
        const auto n=build(k);const auto s=connectorSurface(n,n.connectors.front());++cases;
        for(bool start:{true,false}) {
            const auto found=check(n,s,start,name(k)+(start?" source":" target"));
            if((start?s.source:s.target))++mouths;
            dividers+=found;
        }
    }
    // The forcing: the sweep really exercised mouths and dividers, not only fallbacks.
    CHECK(cases==1512);CHECK(mouths>2500);CHECK(dividers>2000);
}
TEST(mouth_sweep, a_dropped_lane_closes_onto_the_edge_of_its_side) {
    // 3 -> 2 at 90 degrees: the two dividers end on the one Link divider and on the edge point of
    // the side the dropped lane is on, never in the middle of the lane that continues.
    for(auto side:{LaneSide::left,LaneSide::right}) {
        auto n=build({90,0,3,2,0,DrivingSide::right});
        auto& c=n.connectors.front();c.laneChangeSide=side;anchorConnectorEnds(n,c);
        const auto s=connectorSurface(n,c);CHECK(s.target.has_value());
        const auto& b=s.target->boundaries;CHECK(b.size()==3);
        std::vector<Point> ends;
        for(const auto& m:s.markings)if(!m.edge)ends.push_back(m.geometry.back());
        CHECK(ends.size()==2);
        const auto on=[&](Point p){double best=1e300;for(auto q:b)best=std::min(best,distance(p,q));return best;};
        for(auto e:ends)if(on(e)>1e-2)throw std::runtime_error("divider end "+std::to_string(on(e))+" m off: ("+
            std::to_string(e.x)+","+std::to_string(e.y)+") mouth "+std::to_string(b[0].x)+","+std::to_string(b[0].y)+" / "+
            std::to_string(b[1].x)+","+std::to_string(b[1].y)+" / "+std::to_string(b[2].x)+","+std::to_string(b[2].y));
        // One divider sits on the Link's divider, the other on an outer edge point.
        const bool middle=distance(ends[0],b[1])<1e-2 || distance(ends[1],b[1])<1e-2;
        const bool edge=distance(ends[0],b[0])<1e-2 || distance(ends[1],b[0])<1e-2 ||
                        distance(ends[0],b[2])<1e-2 || distance(ends[1],b[2])<1e-2;
        CHECK(middle);CHECK(edge);
    }
}
TEST(mouth_sweep, a_divider_runs_straight_on_to_its_link_boundary) {
    // A Connector arriving along +y, square onto a Link along x, two lanes. Its divider keeps the
    // Connector's direction to the Link, so across the Connector (x) it stands one Connector lane
    // from P1, and along it (y) one Link lane: a point the P1-P2 cap diagonal does not pass through.
    auto n=build({90,0,2,2,0,DrivingSide::right});
    const auto& c=n.connectors.front();const auto s=connectorSurface(n,c);
    CHECK(s.target.has_value());
    const auto& b=s.target->boundaries;CHECK(b.size()==3);
    const double first=connectorLaneWidths(n,c).target[0];
    const auto axis=connectorCentreline(n,c);
    const auto u=directionAlong(axis,polylineLength(axis),true);
    const Point d{b[1].x-b[0].x,b[1].y-b[0].y};
    test::near(std::abs(d.x*u.y-d.y*u.x),first);
    test::near(std::abs(d.y),3.5);

}
TEST(mouth_sweep, an_end_grip_sits_on_the_middle_of_its_link_lanes) {
    // M3.2.9f (D77). The main Link runs along x and the target end attaches at x = 0, so the middle
    // of its lane range is (0, midway between the range's two boundaries), at every arrival angle.
    int steep=0;
    for(double target:{30.,45.,60.,75.,80.,90.,100.,105.,120.,135.,150.})for(int from=1;from<=3;++from)
    for(int to=1;to<=3;++to)for(auto side:{DrivingSide::right,DrivingSide::left}) {
        const Case k{target,0,from,to,0,side};
        const auto n=build(k);const auto& c=n.connectors.front();
        const auto& main=n.links.front();
        const auto a=laneBoundaryGeometry(main,0,side),b=laneBoundaryGeometry(main,static_cast<std::size_t>(to),side);
        const Point expected{0,(pointAlong(a,100).y+pointAlong(b,100).y)/2};
        const auto grip=connectorGrips(n,c).back();
        if(distance(grip,expected)>1e-6)throw std::runtime_error(name(k)+": end grip "+std::to_string(distance(grip,expected))+" m off the lane range");
        // The forcing: steep arrivals use the same centred axis without a square fallback.
        if(target>=90)++steep;
    }
    CHECK(steep>0);
}
