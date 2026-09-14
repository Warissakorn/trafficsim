#include "network.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace trafficsim {
std::string connectorPathId(const Connector& c,int index) {
    return index==0?c.id:c.id+"/lane-"+std::to_string(index+1);
}
std::vector<ConnectorPath> connectorPaths(const Network& n,const Connector& c) {
    const auto range=[&](const LaneReference& ref,int count) {
        std::vector<LaneReference> result;
        if(count<1 || count>12)throw std::invalid_argument("EDIT_LANE_RANGE");
        for(const auto& l:n.links)if(l.id==ref.linkId) {
            const auto it=std::find_if(l.lanes.begin(),l.lanes.end(),[&](const auto& lane){return lane.id==ref.laneId;});
            if(it==l.lanes.end() || std::distance(it,l.lanes.end())<count)break;
            for(int i=0;i<count;++i)result.push_back({l.id,(it+i)->id});
            return result;
        }
        throw std::invalid_argument("EDIT_LANE_RANGE");
    };
    const auto from=range(c.from,c.fromLaneCount),to=range(c.to,c.toLaneCount);
    const auto geometry=[&](const LaneReference& ref) {
        for(const auto& l:n.links)if(l.id==ref.linkId)return laneGeometry(l,ref.laneId,n.drivingSide);
        throw std::invalid_argument("UNKNOWN_LANE");
    };
    const int count=std::max(c.fromLaneCount,c.toLaneCount);
    std::vector<ConnectorPath> result;
    for(int i=0;i<count;++i) {
        const int a=count==1?0:i*(c.fromLaneCount-1)/(count-1);
        const int b=count==1?0:i*(c.toLaneCount-1)/(count-1);
        auto shape=c.geometry;
        if(i && !shape.empty()) {
            const auto start=geometry(from[a]).back(),end=geometry(to[b]).front();
            const auto oldStart=shape.front(),oldEnd=shape.back();const double length=polylineLength(shape);
            double station=0;
            for(std::size_t j=1;j+1<shape.size();++j) {
                station+=std::hypot(c.geometry[j].x-c.geometry[j-1].x,c.geometry[j].y-c.geometry[j-1].y);
                const double t=length>0?station/length:0;
                shape[j].x+=(start.x-oldStart.x)*(1-t)+(end.x-oldEnd.x)*t;
                shape[j].y+=(start.y-oldStart.y)*(1-t)+(end.y-oldEnd.y)*t;
            }
            shape.front()=start;shape.back()=end;
        }
        result.push_back({connectorPathId(c,i),from[a],to[b],std::move(shape)});
    }
    return result;
}
}
