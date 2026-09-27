#include "connector_surface.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace trafficsim {
namespace {
Point sub(Point a,Point b) { return {a.x-b.x,a.y-b.y}; }
Point add(Point a,Point b) { return {a.x+b.x,a.y+b.y}; }
Point mul(Point a,double s) { return {a.x*s,a.y*s}; }
double dot(Point a,Point b) { return a.x*b.x+a.y*b.y; }
double cross(Point a,Point b) { return a.x*b.y-a.y*b.x; }
double norm(Point a) { return std::hypot(a.x,a.y); }
Point mid(Point a,Point b) { return {std::midpoint(a.x,b.x),std::midpoint(a.y,b.y)}; }
bool same(Point a,Point b) { return norm(sub(a,b))<1e-9; }
void append(std::vector<Point>& out,Point p) { if(out.empty() || !same(out.back(),p))out.push_back(p); }
std::optional<Point> intersection(Point a,Point u,Point b,Point v) {
    const double divisor=cross(u,v);
    if(std::abs(divisor)<1e-8)return {};
    const auto p=add(a,mul(u,cross(sub(b,a),v)/divisor));
    if(!std::isfinite(p.x) || !std::isfinite(p.y))return {};
    return p;
}
// Where the line through `a` along `u` crosses the segment p-q, if it does.
std::optional<Point> onSegment(Point a,Point u,Point p,Point q) {
    const auto v=sub(q,p);const double d=cross(v,u);
    if(std::abs(d)<1e-12)return {};
    const double t=cross(sub(a,p),u)/d;
    if(t<-1e-9 || t>1+1e-9)return {};
    return add(p,mul(v,std::clamp(t,0.,1.)));
}
// Move one end of a boundary onto `target`, fading the shift out over the half of its length
// nearest that end, so each end is bent independently and the far half is left alone.
void bend(std::vector<Point>& g,Point target,bool start) {
    if(g.size()<2)return;
    const auto delta=sub(target,start?g.front():g.back());
    const double half=polylineLength(g)/2;
    double along=0;
    for(std::size_t k=0;k<g.size();++k) {
        const std::size_t i=start?k:g.size()-1-k;
        if(k)along+=norm(sub(g[i],g[start?i-1:i+1]));
        if(along>=half)break;
        const double x=1-along/half,w=x*x*(3-2*x);
        g[i]=add(g[i],mul(delta,w));
    }
}
struct Edge { std::vector<Point> geometry; Point at,along; };
// The Link lane range an end is attached to, and its boundaries' lines at the attachment station.
struct Range { const Link* link{}; std::size_t first{}; int count{}; double station{}; };
std::optional<Range> attachedRange(const Network& n,const Connector& c,bool start) {
    const auto& ref=start?c.from:c.to;
    const int count=start?c.fromLaneCount:c.toLaneCount;
    const auto link=std::find_if(n.links.begin(),n.links.end(),[&](const auto& l){return l.id==ref.linkId;});
    if(link==n.links.end())return {};
    const auto lane=std::find_if(link->lanes.begin(),link->lanes.end(),[&](const auto& l){return l.id==ref.laneId;});
    if(lane==link->lanes.end() || std::distance(lane,link->lanes.end())<count)return {};
    return Range{&*link,static_cast<std::size_t>(std::distance(link->lanes.begin(),lane)),count,attachmentStation(n,ref,start)};
}
Edge rangeEdge(const Network& n,const Range& r,std::size_t i,bool start) {
    auto g=laneBoundaryGeometry(*r.link,i,n.drivingSide);
    const double at=matchedStation(r.link->geometry,g,r.station);
    const auto p=pointAlong(g,at),u=directionAlong(g,at,start);
    return Edge{std::move(g),p,u};
}
std::optional<ConnectorMouth> mouth(const Network& n,const Connector& c,
                                  const std::vector<std::vector<Point>>& rails,bool start) {
    const auto range=attachedRange(n,c,start);
    if(!range)return {};
    const auto first=range->first;const int count=range->count;
    const auto edge=[&](std::size_t i){return rangeEdge(n,*range,i,start);};
    const std::array<Edge,2> edges{edge(first),edge(first+count)};
    const Point centre=mid(edges[0].at,edges[1].at);
    const auto widths=connectorLaneWidths(n,c);
    const auto& w=start?widths.source:widths.target;
    const double width=std::accumulate(w.begin(),w.end(),0.);
    if(width<=1e-9)return {};
    const auto u=directionAlong(c.geometry,start?0:polylineLength(c.geometry),!start);
    Point normal{-u.y,u.x};
    const auto end=[&](const auto& g){return start?g.front():g.back();};
    // Keep boundary indices attached to the body's actual two rails, even on a reversed
    // arrival. Pairing with the Link edges below is allowed to swap, lane order is not.
    if(dot(normal,sub(end(rails.back()),end(rails.front())))<0)normal=mul(normal,-1);
    const std::array<Point,2> own{add(centre,mul(normal,-width/2)),add(centre,mul(normal,width/2))};
    std::array<Point,2> cuts{};int pairing=-1;double best=std::numeric_limits<double>::max();
    for(int swap=0;swap<2;++swap) {
        const auto a=intersection(own[0],u,edges[swap].at,edges[swap].along);
        const auto b=intersection(own[1],u,edges[1-swap].at,edges[1-swap].along);
        if(!a || !b)continue;
        const double reach=norm(sub(*a,centre))+norm(sub(*b,centre));
        // Choosing the shorter pairing folds the angle at 90 degrees. At equal width the
        // shoulder is W/2*tan(min(theta,180-theta)/2), bounded by W/2.
        if(reach<best-1e-9){best=reach;cuts={*a,*b};pairing=swap;}
    }
    if(pairing<0 || best>4*std::max(width,norm(sub(edges[1].at,edges[0].at))))return {};
    const Point outward=mul(u,start?-1:1);
    const int near=dot(sub(cuts[0],centre),outward)<=dot(sub(cuts[1],centre),outward)+1e-9?0:1;
    const int far=1-near,farEdge=far==0?pairing:1-pairing;
    const auto& g=edges[farEdge].geometry;
    const Point projection=pointAlong(g,stationOfClosestPoint(g,centre));
    ConnectorMouth result{{cuts[near],centre,projection,cuts[far]},near==0,{}};
    // cuts[0] is the first rail's end and lies on edges[pairing]: past 90 degrees the rails meet
    // the range in reverse lane order, and the interior boundaries must follow them.
    const std::array<Point,3> cap{result.points[0],result.points[1],result.points[2]};
    result.boundaries.push_back(cuts[0]);
    double offset=0;std::size_t nextLane=0;
    for(int j=1;j<count;++j) {
        const auto b=edge(first+static_cast<std::size_t>(pairing==0?j:count-j));
        // D76: built as P1/P4 are -- the Connector's own divider line, offset from the first rail's
        // edge by the widths of the lanes before it, runs on along the end direction to meet the
        // Link boundary's line. A surplus lane has no width here, so it adds no offset.
        while(nextLane<w.size() && w[nextLane]<=0)++nextLane;
        if(nextLane<w.size())offset+=w[nextLane++];
        std::optional<Point> hit=intersection(add(own[0],mul(normal,offset)),u,b.at,b.along);
        if(hit && norm(sub(*hit,centre))>4*std::max(width,norm(sub(edges[1].at,edges[0].at))))hit.reset();
        for(int s=0;s<2 && !hit;++s)hit=onSegment(b.at,b.along,cap[s],cap[s+1]);
        // A curved Link can bend a boundary's local tangent away from the cap; the nearest cap
        // vertex keeps the divider on the mouth rather than dropping it.
        if(!hit)hit=*std::min_element(cap.begin(),cap.end(),[&](Point x,Point y){
            return norm(sub(x,b.at))<norm(sub(y,b.at));});
        result.boundaries.push_back(*hit);
    }
    result.boundaries.push_back(cuts[1]);
    return result;
}
void cap(std::vector<Point>& ring,const std::optional<ConnectorMouth>& m,bool forward) {
    if(!m)return;
    const bool ordered=m->firstBoundaryNear==forward;
    for(int i=0;i<4;++i)append(ring,m->points[ordered?i:3-i]);
}
std::vector<Point> outline(const std::vector<std::vector<Point>>& rails,
                           const std::optional<ConnectorMouth>& source,const std::optional<ConnectorMouth>& target) {
    std::vector<Point> ring;
    for(auto p:rails.front())append(ring,p);
    cap(ring,target,true);
    for(auto it=rails.back().rbegin();it!=rails.back().rend();++it)append(ring,*it);
    cap(ring,source,false);
    if(ring.size()>1 && same(ring.front(),ring.back()))ring.pop_back();
    return ring;
}
// Return the fraction on a segment where another segment crosses it.
std::optional<double> crossing(Point a,Point b,Point p,Point q) {
    const auto u=sub(b,a),v=sub(q,p);const double d=cross(u,v);
    if(std::abs(d)<1e-12)return {};
    const double t=cross(sub(p,a),v)/d,s=cross(sub(p,a),u)/d;
    if(t<0 || t>1 || s<0 || s>1)return {};
    return t;
}
bool simple(const std::vector<Point>& ring) {
    for(std::size_t i=0;i<ring.size();++i)for(std::size_t j=i+2;j<ring.size();++j) {
        if(i==0 && j+1==ring.size())continue;
        const auto t=crossing(ring[i],ring[(i+1)%ring.size()],ring[j],ring[(j+1)%ring.size()]);
        if(t && *t>1e-9 && *t<1-1e-9)return false;
    }
    return true;
}
bool contains(const std::vector<Point>& ring,Point p) {
    bool inside=false;
    for(std::size_t i=0,j=ring.size()-1;i<ring.size();j=i++) {
        const auto a=ring[j],b=ring[i],v=sub(b,a),w=sub(p,a);
        if(std::abs(cross(v,w))<1e-8 && dot(w,v)>=0 && dot(w,v)<=dot(v,v))return true;
        if((a.y>p.y)!=(b.y>p.y) && p.x<(b.x-a.x)*(p.y-a.y)/(b.y-a.y)+a.x)inside=!inside;
    }
    return inside;
}
void clippedMarking(std::vector<ConnectorMarking>& result,const ConnectorMarking& marking,
                    const std::vector<Point>& ring) {
    std::vector<Point> run;
    const auto flush=[&] { if(run.size()>1)result.push_back({run,marking.edge,marking.type});run.clear(); };
    for(std::size_t i=1;i<marking.geometry.size();++i) {
        const auto a=marking.geometry[i-1],b=marking.geometry[i],v=sub(b,a);
        std::vector<double> cuts{0,1};
        for(std::size_t j=0;j<ring.size();++j)
            if(auto t=crossing(a,b,ring[j],ring[(j+1)%ring.size()]))cuts.push_back(*t);
        std::sort(cuts.begin(),cuts.end());
        for(std::size_t j=1;j<cuts.size();++j) {
            if(cuts[j]-cuts[j-1]<1e-10)continue;
            if(contains(ring,add(a,mul(v,(cuts[j-1]+cuts[j])/2)))) {
                append(run,add(a,mul(v,cuts[j-1])));append(run,add(a,mul(v,cuts[j])));
            } else flush();
        }
    }
    flush();
}
}
ConnectorSurface connectorSurface(const Network& n,const Connector& c) {
    const auto original=connectorBoundaries(n,c);
    auto rails=original;
    ConnectorSurface result;
    result.source=mouth(n,c,rails,true);result.target=mouth(n,c,rails,false);
    const auto widths=connectorLaneWidths(n,c);
    const auto apply=[&](const std::optional<ConnectorMouth>& m,bool start) {
        if(!m)return;
        // The rails bend onto P1/P4 exactly as the dividers bend onto their points, so the body
        // stays one shape and no divider runs outside a rail that moved only its last vertex.
        bend(rails.front(),m->points[m->firstBoundaryNear?0:3],start);
        bend(rails.back(),m->points[m->firstBoundaryNear?3:0],start);
        // Each interior boundary ends on the Link boundary it belongs to: the one after as many
        // lanes as have width at this end. A surplus lane (width 0) adds none, so a taper closes
        // onto its neighbour's point, which for an outermost surplus lane is P1 or P4 (D73).
        const auto& w=start?widths.source:widths.target;
        std::size_t lanes=0;
        for(std::size_t k=1;k+1<rails.size();++k) {
            if(w[k-1]>0)++lanes;
            bend(rails[k],m->boundaries[std::min(lanes,m->boundaries.size()-1)],start);
        }
    };
    apply(result.source,true);apply(result.target,false);
    result.outline=outline(rails,result.source,result.target);
    // A short/bent Connector can run out of room before a local edge intersection. Do not
    // silently trim P2/P3 away or draw a folded polygon: retain the established surface.
    if(!simple(result.outline)) {
        result.source.reset();result.target.reset();rails=original;
        result.outline=trimSelfIntersections(outline(rails,{},{}));
    }
    // A divider bent onto its mouth point ends ON the cap, and may reach it across the P2-P3
    // notch, which is Link surface: clipping would cut that last stretch off. Only the legacy
    // cap, where dividers are not placed, still needs clipping to keep them on the surface.
    const bool placed=result.source || result.target;
    for(const auto& marking:connectorMarkings(c,rails)) {
        if(marking.edge || placed)result.markings.push_back(marking);
        else clippedMarking(result.markings,marking,result.outline);
    }
    return result;
}
std::optional<Point> connectorRangeCentre(const Network& n,const Connector& c,bool start) {
    const auto range=attachedRange(n,c,start);
    if(!range)return {};
    return mid(rangeEdge(n,*range,range->first,start).at,
               rangeEdge(n,*range,range->first+static_cast<std::size_t>(range->count),start).at);
}
std::vector<Point> connectorGrips(const Network& n,const Connector& c) {
    auto grips=connectorCentreline(n,c);
    if(grips.empty())return grips;
    if(const auto p=connectorRangeCentre(n,c,true))grips.front()=*p;
    if(const auto p=connectorRangeCentre(n,c,false))grips.back()=*p;
    return grips;
}
}
