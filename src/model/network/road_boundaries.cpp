#include "network.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace trafficsim {
namespace {
// Use the lane's segment parameter, rather than boundary arclength, so a cross-section
// remains aligned at body attachments on a curved link.
Point edgeAt(const Network& n,const LaneReference& ref,int boundary,bool outgoing) {
    for(const auto& l:n.links)if(l.id==ref.linkId) {
        const auto lane=laneGeometry(l,ref.laneId,n.drivingSide);
        const auto edge=laneBoundaryGeometry(l,static_cast<std::size_t>(boundary),n.drivingSide);
        double remaining=polylineLength(lane)*ref.fraction.value_or(outgoing?1.:0.);
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
}
std::vector<std::vector<Point>> connectorBoundaries(const Network& n,const Connector& c) {
    const auto paths=connectorPaths(n,c);const auto weights=connectorBlendWeights(c);
    const auto endEdge=[&](const LaneReference& ref,int count,double boundary,bool outgoing) {
        for(const auto& l:n.links)if(l.id==ref.linkId) {
            const auto first=std::find_if(l.lanes.begin(),l.lanes.end(),[&](const auto& lane){return lane.id==ref.laneId;});
            const int index=static_cast<int>(std::distance(l.lanes.begin(),first));
            const double at=boundary*count;const int lower=std::min(count,static_cast<int>(at));
            const auto edge=[&](int number) {
                auto leftRef=ref,rightRef=ref;
                leftRef.laneId=(first+std::max(0,number-1))->id;
                rightRef.laneId=(first+std::min(count-1,number))->id;
                const auto a=edgeAt(n,leftRef,index+number,outgoing),b=edgeAt(n,rightRef,index+number,outgoing);
                return Point{(a.x+b.x)/2,(a.y+b.y)/2};
            };
            const auto a=edge(lower),b=edge(std::min(count,lower+1));
            return Point{a.x+(b.x-a.x)*(at-lower),a.y+(b.y-a.y)*(at-lower)};
        }
        throw std::invalid_argument("UNKNOWN_LANE");
    };
    std::vector<std::vector<Point>> result;
    for(std::size_t boundary=0;boundary<=paths.size();++boundary) {
        std::vector<Point> shape;
        if(boundary>0 && boundary<paths.size()) {
            shape=paths[boundary-1].geometry;
            for(std::size_t j=0;j<shape.size();++j) {
                shape[j].x=(shape[j].x+paths[boundary].geometry[j].x)/2;
                shape[j].y=(shape[j].y+paths[boundary].geometry[j].y)/2;
            }
        } else {
            const auto& path=boundary==0?paths.front():paths.back();double width=0;
            for(const auto& l:n.links)for(const auto& lane:l.lanes)
                if(lane.id==path.from.laneId || lane.id==path.to.laneId)width+=lane.width;
            shape=offsetGeometry(path.geometry,width/4*(boundary==0?1.:-1.)*(n.drivingSide==DrivingSide::left?1.:-1.));
        }
        const double fraction=static_cast<double>(boundary)/paths.size();
        const auto a=endEdge(c.from,c.fromLaneCount,fraction,true),b=endEdge(c.to,c.toLaneCount,fraction,false);
        const auto oldA=shape.front(),oldB=shape.back();
        for(std::size_t j=0;j<shape.size();++j) {
            const double t=weights[j];
            shape[j].x+=(a.x-oldA.x)*(1-t)+(b.x-oldB.x)*t;
            shape[j].y+=(a.y-oldA.y)*(1-t)+(b.y-oldB.y)*t;
        }
        shape.front()=a;shape.back()=b;result.push_back(std::move(shape));
    }
    return result;
}
}
