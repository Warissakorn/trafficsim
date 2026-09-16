#include "network.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace trafficsim {
namespace {
// Use the lane's segment parameter, rather than boundary arclength, so a cross-section
// remains aligned at body attachments on a curved link.
Point edgeAt(const Network& n,const LaneReference& ref,int boundary,bool outgoing) {
    for(const auto& l:n.links)if(l.id==ref.linkId) {
        const auto lane=laneGeometry(l,ref.laneId,n.drivingSide);
        const auto edge=laneBoundaryGeometry(l,static_cast<std::size_t>(boundary),n.drivingSide);
        double remaining=matchedStation(l.geometry,lane,attachmentStation(n,ref,outgoing));
        for(std::size_t i=1;i<lane.size();++i) {
            const double length=std::hypot(lane[i].x-lane[i-1].x,lane[i].y-lane[i-1].y);
            if(length>0 && remaining<=length) {
                const double t=remaining/length;
                return {edge[i-1].x+(edge[i].x-edge[i-1].x)*t,edge[i-1].y+(edge[i].y-edge[i-1].y)*t};
            }
            remaining-=length;
        }
        return edge.back();
    }
    throw std::invalid_argument("UNKNOWN_LANE");
}
// The direction across the road at the end a reference attaches to: taken from the link's own
// lane edges, so the Connector's mouth meets the link flush instead of being pulled onto it.
Point endCross(const Network& n,const LaneReference& ref,int count,bool outgoing) {
    for(const auto& l:n.links)if(l.id==ref.linkId) {
        const auto first=std::find_if(l.lanes.begin(),l.lanes.end(),[&](const auto& lane){return lane.id==ref.laneId;});
        if(first==l.lanes.end() || std::distance(first,l.lanes.end())<count)throw std::invalid_argument("EDIT_LANE_RANGE");
        const int index=static_cast<int>(std::distance(l.lanes.begin(),first));
        auto last=ref;last.laneId=(first+count-1)->id;
        const auto a=edgeAt(n,ref,index,outgoing),b=edgeAt(n,last,index+count,outgoing);
        const double span=std::hypot(b.x-a.x,b.y-a.y);
        if(!std::isfinite(span) || span<=0)throw std::invalid_argument("INVALID_GEOMETRY");
        return {(b.x-a.x)/span,(b.y-a.y)/span};
    }
    throw std::invalid_argument("UNKNOWN_LANE");
}
double laneWidthOf(const Network& n,const LaneReference& ref) {
    for(const auto& l:n.links)if(l.id==ref.linkId)
        for(const auto& lane:l.lanes)if(lane.id==ref.laneId)return lane.width;
    throw std::invalid_argument("UNKNOWN_LANE");
}
}
std::vector<std::vector<Point>> connectorBoundaries(const Network& n,const Connector& c) {
    const auto paths=connectorPaths(n,c);const auto weights=connectorBlendWeights(c);
    // A Connector carries lanes, not a ribbon that shrinks. Each lane keeps the width its link
    // gives it from end to end; a lane the other end has no room for is the one that tapers,
    // closing onto its neighbour like a merge taper. Where two paths share a lane at one end,
    // the second of them is the surplus one, so its width there is zero.
    const std::size_t count=paths.size();
    std::vector<double> source(count),target(count);
    for(std::size_t i=0;i<count;++i) {
        source[i]=i && paths[i].from.laneId==paths[i-1].from.laneId?0:laneWidthOf(n,paths[i].from);
        target[i]=i && paths[i].to.laneId==paths[i-1].to.laneId?0:laneWidthOf(n,paths[i].to);
    }
    const auto from=endCross(n,c.from,c.fromLaneCount,true),to=endCross(n,c.to,c.toLaneCount,false);
    // Turn the cross-section from one mouth to the other rather than averaging the two vectors:
    // on a U-turn they are opposite, and their average is nothing at all.
    const double a0=std::atan2(from.y,from.x);
    double sweep=std::atan2(to.y,to.x)-a0;
    while(sweep>std::numbers::pi)sweep-=2*std::numbers::pi;
    while(sweep<-std::numbers::pi)sweep+=2*std::numbers::pi;
    if(std::abs(std::abs(sweep)-std::numbers::pi)<1e-9) {
        // Half a turn either way lands on the same line, so take the way the road itself turns.
        const auto& g=paths.front().geometry;
        const double sx=g[1].x-g.front().x,sy=g[1].y-g.front().y;
        const double ex=g.back().x-g[g.size()-2].x,ey=g.back().y-g[g.size()-2].y;
        sweep=std::copysign(std::numbers::pi,sx*ey-sy*ex==0?sweep:sx*ey-sy*ex);
    }
    // Hang the cross-section on the last lane that is a real lane at both ends, and step out from
    // there in both directions. A lane added at the leading edge then cannot move the far edge,
    // and a lane that tapers is placed against its neighbour rather than on its own driving line,
    // which converges onto the lane it merges into and is no longer where that lane's edge is.
    std::size_t anchorLane=0;
    for(std::size_t i=0;i<count;++i)if(source[i]>0 && target[i]>0)anchorLane=i;
    std::vector<std::vector<Point>> result(count+1);
    for(std::size_t j=0;j<paths.front().geometry.size();++j) {
        const double t=weights[j],angle=a0+sweep*t;
        const Point across{std::cos(angle),std::sin(angle)};
        const auto width=[&](std::size_t i){return source[i]+(target[i]-source[i])*t;};
        const auto anchor=paths[anchorLane].geometry[j];
        std::vector<Point> edges(count+1);
        edges[anchorLane]={anchor.x-across.x*width(anchorLane)/2,anchor.y-across.y*width(anchorLane)/2};
        for(std::size_t i=anchorLane;i-->0;)
            edges[i]={edges[i+1].x-across.x*width(i),edges[i+1].y-across.y*width(i)};
        for(std::size_t i=anchorLane;i<count;++i)
            edges[i+1]={edges[i].x+across.x*width(i),edges[i].y+across.y*width(i)};
        for(std::size_t i=0;i<=count;++i)result[i].push_back(edges[i]);
    }
    return result;
}
std::vector<ConnectorMarking> connectorMarkings(const Network& n,const Connector& c) {
    const auto boundaries=connectorBoundaries(n,c);
    std::vector<ConnectorMarking> result;
    result.push_back({trimSelfIntersections(boundaries.front()),true});
    for(std::size_t i=1;i+1<boundaries.size();++i) {
        // Every interior boundary now sits on a real lane edge for its whole length, because the
        // cross-section is built from lane widths. The one case with nothing to divide is a lane
        // with no width anywhere -- a range drawn onto lanes that are not there.
        double widest=0;
        for(std::size_t j=0;j<boundaries[i].size();++j)
            widest=std::max(widest,std::hypot(boundaries[i+1][j].x-boundaries[i][j].x,
                                              boundaries[i+1][j].y-boundaries[i][j].y));
        if(widest>1e-9)result.push_back({trimSelfIntersections(boundaries[i]),false});
    }
    if(boundaries.size()>1)result.push_back({trimSelfIntersections(boundaries.back()),true});
    return result;
}
std::vector<Point> connectorCentreline(const Network& n,const Connector& c) {
    const auto boundaries=connectorBoundaries(n,c);
    std::vector<Point> result;
    for(std::size_t i=0;i<c.geometry.size();++i)
        result.push_back({(boundaries.front()[i].x+boundaries.back()[i].x)/2,
                          (boundaries.front()[i].y+boundaries.back()[i].y)/2});
    return result;
}
}
