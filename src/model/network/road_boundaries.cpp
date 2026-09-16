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
double wrap(double angle) {
    while(angle>std::numbers::pi)angle-=2*std::numbers::pi;
    while(angle<-std::numbers::pi)angle+=2*std::numbers::pi;
    return angle;
}
double laneWidthOf(const Network& n,const LaneReference& ref) {
    for(const auto& l:n.links)if(l.id==ref.linkId)
        for(const auto& lane:l.lanes)if(lane.id==ref.laneId)return lane.width;
    throw std::invalid_argument("UNKNOWN_LANE");
}
}
std::vector<std::vector<Point>> connectorBoundaries(const Network& n,const Connector& c) {
    const auto paths=connectorPaths(n,c);
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
    // Only the source mouth is consulted, and only for which way lane order runs across the road.
    const auto from=endCross(n,c.from,c.fromLaneCount,true);
    // Hang the cross-section on the last lane that is a real lane at both ends, and step out from
    // there in both directions. A lane added at the leading edge then cannot move the far edge,
    // and a lane that tapers is placed against its neighbour rather than on its own driving line,
    // which converges onto the lane it merges into and is no longer where that lane's edge is.
    std::size_t anchorLane=0;
    for(std::size_t i=0;i<count;++i)if(source[i]>0 && target[i]>0)anchorLane=i;
    // A constant offset from the axis, square to it at every point and at both ends, is what a
    // road is: the width belongs to the Connector, not to the links it happens to join. Cutting
    // the ends on the links' cross-sections instead drew a slanted wedge at the joint wherever
    // the curve did not leave the lane straight; interpolating that cut through the body was
    // worse still, at 1.06 m of a 3.50 m lane on a reverse curve. Vissim squares the ends to the
    // connector and lets the joint overlap the link, and so does this.
    const auto& spine=paths[anchorLane].geometry;
    // How far along the taper each sample is. The stored blend weights belong to the author's
    // poly points, which are far fewer than the road's samples, so the road's own arclength is
    // what the cross-section is read against.
    std::vector<double> weights(spine.size());
    {
        const double length=polylineLength(spine);double station=0;
        for(std::size_t j=1;j<spine.size();++j) {
            station+=std::hypot(spine[j].x-spine[j-1].x,spine[j].y-spine[j-1].y);
            weights[j]=length>0?station/length:0;
        }
    }
    const double entry=std::atan2(from.y,from.x);
    // Which way a normal points is a convention; which way lane order runs is not. Take the
    // source mouth's word for it once, for the whole body, or the lanes come out mirrored.
    const auto raw=[&](std::size_t j) {
        const auto a=spine[j?j-1:0],b=spine[std::min(j+1,spine.size()-1)];
        return std::atan2(b.y-a.y,b.x-a.x)+std::numbers::pi/2;
    };
    const double sign=std::abs(wrap(entry-raw(0)))>std::numbers::pi/2?-1.:1.;
    // Each boundary is the axis offset by the lanes stacked up to it, mitered at every corner by
    // the same function a Link's own edges use -- so a lane is its full width square to the road
    // at every point, through a bend and past a poly point the author has dragged.
    std::vector<std::vector<double>> offsets(count+1,std::vector<double>(spine.size()));
    for(std::size_t j=0;j<spine.size();++j) {
        const double t=weights[j];
        const auto width=[&](std::size_t i){return source[i]+(target[i]-source[i])*t;};
        offsets[anchorLane][j]=-width(anchorLane)/2;
        for(std::size_t i=anchorLane;i-->0;)offsets[i][j]=offsets[i+1][j]-width(i);
        for(std::size_t i=anchorLane;i<count;++i)offsets[i+1][j]=offsets[i][j]+width(i);
        for(std::size_t i=0;i<=count;++i)offsets[i][j]*=sign;
    }
    std::vector<std::vector<Point>> result;
    for(std::size_t i=0;i<=count;++i)result.push_back(offsetGeometry(spine,offsets[i]));
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
    // One grip per point the author stored, never one per sample of the road between them.
    std::vector<std::size_t> indices;
    (void)connectorRoad(n,c,&indices);
    std::vector<Point> result;
    for(const auto i:indices)
        result.push_back({(boundaries.front()[i].x+boundaries.back()[i].x)/2,
                          (boundaries.front()[i].y+boundaries.back()[i].y)/2});
    return result;
}
}
