#include "network.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace trafficsim {
std::string connectorPathId(const Connector& c,int index) {
    return index==0?c.id:c.id+"/lane-"+std::to_string(index+1);
}
std::vector<double> connectorBlendWeights(const Connector& c) {
    if(!c.laneBlend.empty()) {
        if(c.laneBlend.size()!=c.geometry.size() || c.laneBlend.front()!=0 || c.laneBlend.back()!=1)
            throw std::invalid_argument("INVALID_GEOMETRY");
        double previous=0;
        for(double t:c.laneBlend) {
            if(!std::isfinite(t) || t<previous || t>1)throw std::invalid_argument("INVALID_GEOMETRY");
            previous=t;
        }
        return c.laneBlend;
    }
    std::vector<double> result(c.geometry.size());const double length=polylineLength(c.geometry);
    double station=0;
    for(std::size_t i=1;i<result.size();++i) {
        station+=std::hypot(c.geometry[i].x-c.geometry[i-1].x,c.geometry[i].y-c.geometry[i-1].y);
        result[i]=length>0?station/length:0;
    }
    return result;
}
void resizeConnectorEdges(const Network& n,Connector& c,int fromCount,int toCount,bool leading) {
    auto resized=c;
    if(leading) {
        const auto shift=[&](LaneReference& ref,int delta) {
            for(const auto& l:n.links)if(l.id==ref.linkId) {
                const auto it=std::find_if(l.lanes.begin(),l.lanes.end(),[&](const auto& lane){return lane.id==ref.laneId;});
                const auto index=std::distance(l.lanes.begin(),it)-delta;
                if(it==l.lanes.end() || index<0 || index>=static_cast<int>(l.lanes.size()))break;
                ref.laneId=l.lanes[static_cast<std::size_t>(index)].id;return;
            }
            throw std::invalid_argument("EDIT_LANE_RANGE");
        };
        shift(resized.from,fromCount-c.fromLaneCount);shift(resized.to,toCount-c.toLaneCount);
        resized.laneBlend=connectorBlendWeights(c);
        const auto a=laneAttachment(n,resized.from,true),b=laneAttachment(n,resized.to,false);
        for(std::size_t j=0;j<c.geometry.size();++j) {
            const double t=resized.laneBlend[j];
            resized.geometry[j].x+=(a.x-c.geometry.front().x)*(1-t)+(b.x-c.geometry.back().x)*t;
            resized.geometry[j].y+=(a.y-c.geometry.front().y)*(1-t)+(b.y-c.geometry.back().y)*t;
        }
        resized.geometry.front()=a;resized.geometry.back()=b;
    }
    resized.fromLaneCount=fromCount;resized.toLaneCount=toCount;
    (void)connectorPaths(n,resized);c=std::move(resized);
}
std::vector<ConnectorPath> connectorPaths(const Network& n,const Connector& c) {
    const auto range=[&](const LaneReference& ref,int count) {
        std::vector<LaneReference> result;
        if(count<1 || count>12)throw std::invalid_argument("EDIT_LANE_RANGE");
        for(const auto& l:n.links)if(l.id==ref.linkId) {
            const auto it=std::find_if(l.lanes.begin(),l.lanes.end(),[&](const auto& lane){return lane.id==ref.laneId;});
            if(it==l.lanes.end() || std::distance(it,l.lanes.end())<count)break;
            for(int i=0;i<count;++i)result.push_back({l.id,(it+i)->id,ref.station});
            return result;
        }
        throw std::invalid_argument("EDIT_LANE_RANGE");
    };
    const auto from=range(c.from,c.fromLaneCount),to=range(c.to,c.toLaneCount);
    const int count=std::max(c.fromLaneCount,c.toLaneCount);
    const auto weights=connectorBlendWeights(c);
    std::vector<ConnectorPath> result;
    for(int i=0;i<count;++i) {
        const int a=count==1?0:i*(c.fromLaneCount-1)/(count-1);
        const int b=count==1?0:i*(c.toLaneCount-1)/(count-1);
        auto shape=c.geometry;
        if(i && !shape.empty()) {
            const auto start=laneAttachment(n,from[a],true),end=laneAttachment(n,to[b],false);
            const auto oldStart=shape.front(),oldEnd=shape.back();
            for(std::size_t j=1;j+1<shape.size();++j) {
                const double t=weights[j];
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
