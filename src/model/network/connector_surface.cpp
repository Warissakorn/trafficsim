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
struct Edge { std::vector<Point> geometry; Point at,along; };
std::optional<ConnectorMouth> mouth(const Network& n,const Connector& c,
                                  const std::vector<std::vector<Point>>& rails,bool start) {
    const auto& ref=start?c.from:c.to;
    const int count=start?c.fromLaneCount:c.toLaneCount;
    const auto link=std::find_if(n.links.begin(),n.links.end(),[&](const auto& l){return l.id==ref.linkId;});
    if(link==n.links.end())return {};
    const auto lane=std::find_if(link->lanes.begin(),link->lanes.end(),[&](const auto& l){return l.id==ref.laneId;});
    if(lane==link->lanes.end() || std::distance(lane,link->lanes.end())<count)return {};
    const auto first=static_cast<std::size_t>(std::distance(link->lanes.begin(),lane));
    const double station=attachmentStation(n,ref,start);
    const auto edge=[&](std::size_t i) {
        auto g=laneBoundaryGeometry(*link,i,n.drivingSide);
        const double at=matchedStation(link->geometry,g,station);
        const auto p=pointAlong(g,at),u=directionAlong(g,at,start);
        return Edge{std::move(g),p,u};
    };
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
    return ConnectorMouth{{cuts[near],centre,projection,cuts[far]},near==0};
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
    const auto apply=[&](const std::optional<ConnectorMouth>& m,bool start) {
        if(!m)return;
        auto& a=start?rails.front().front():rails.front().back();
        auto& b=start?rails.back().front():rails.back().back();
        a=m->points[m->firstBoundaryNear?0:3];b=m->points[m->firstBoundaryNear?3:0];
    };
    apply(result.source,true);apply(result.target,false);
    result.outline=outline(rails,result.source,result.target);
    // A short/bent Connector can run out of room before a local edge intersection. Do not
    // silently trim P2/P3 away or draw a folded polygon: retain the established surface.
    if(!simple(result.outline)) {
        result.source.reset();result.target.reset();rails=original;
        result.outline=trimSelfIntersections(outline(rails,{},{}));
    }
    result.markings.push_back({trimSelfIntersections(rails.front()),true,MarkingType::solid});
    for(const auto& marking:connectorMarkings(n,c))if(!marking.edge)
        clippedMarking(result.markings,marking,result.outline);
    result.markings.push_back({trimSelfIntersections(rails.back()),true,MarkingType::solid});
    return result;
}
}
