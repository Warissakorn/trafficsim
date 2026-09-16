#include "appearance_commands.hpp"
#include "network_commands.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <cmath>
namespace trafficsim {
void changeAppearance(ProjectDocument& d,const std::string& id,int level,const std::string& type) {
    if(level < -1000 || level > 1000 || type.empty())throw std::invalid_argument("EDIT_DISPLAY_VALUE");
    for(auto& l:d.network.links)if(l.id==id){l.level=level;l.displayType=type;return;}
    for(auto& c:d.network.connectors)if(c.id==id){c.level=level;c.displayType=type;return;}
    throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
namespace {
LaneReference dropLane(const Network& n,Point p,int count,int level) {
    double best=1e300;std::optional<LaneReference> result;
    for(const auto& l:n.links)if(l.level==level)for(std::size_t i=0;i<l.lanes.size();++i) {
        if(i+static_cast<std::size_t>(count)>l.lanes.size())continue;
        const auto g=laneGeometry(l,l.lanes[i].id,n.drivingSide);const auto station=stationOfClosestPoint(g,p);
        const auto at=pointAlong(g,station);const double distance=std::hypot(at.x-p.x,at.y-p.y);
        if(distance<=l.lanes[i].width/2+.01 && distance<best)
            {best=distance;result=LaneReference{l.id,l.lanes[i].id,matchedStation(g,l.geometry,station)};}
    }
    if(!result)throw std::invalid_argument("EDIT_COPY_TARGET");return *result;
}
int linkLevel(const Network& n,const std::string& id) {
    for(const auto& l:n.links)if(l.id==id)return l.level;
    throw std::invalid_argument("UNKNOWN_LANE");
}
void dropHead(const Network& n,NetworkSignalHead& head,Point target,int level) {
    double best=1e300;bool found=false;
    const auto consider=[&](const std::vector<Point>& g,double width,LaneReference lane,const std::string& connector) {
        const double station=stationOfClosestPoint(g,target);const auto at=pointAlong(g,station);
        const double distance=std::hypot(at.x-target.x,at.y-target.y);
        if(distance<=width/2+.01 && distance<best) {
            found=true;best=distance;head.lane=std::move(lane);head.connectorId=connector;head.position=station;
        }
    };
    for(const auto& l:n.links)if(l.level==level)for(const auto& lane:l.lanes)
        consider(laneGeometry(l,lane.id,n.drivingSide),lane.width,{l.id,lane.id},{});
    for(const auto& c:n.connectors)if(c.level==level)for(const auto& path:connectorPaths(n,c)) {
        double width=0;
        for(const auto& l:n.links)for(const auto& lane:l.lanes)if(lane.id==path.from.laneId)width=lane.width;
        consider(path.geometry,width,{},path.id);
    }
    if(!found)throw std::invalid_argument("EDIT_COPY_TARGET");
}
}
std::vector<std::string> duplicateObjects(ProjectDocument& d,const std::vector<std::string>& ids,Point offset) {
    if(!std::isfinite(offset.x) || !std::isfinite(offset.y))throw std::invalid_argument("INVALID_GEOMETRY");
    const auto source=d.network;std::map<std::string,std::string> links,lanes,paths;std::vector<std::string> created;
    const std::set<std::string> chosen(ids.begin(),ids.end());
    for(const auto& id:chosen) {
        bool found=false;
        for(const auto& l:source.links)found=found || l.id==id;
        for(const auto& c:source.connectors)found=found || c.id==id;
        for(const auto& h:source.signalHeads)found=found || h.id==id;
        if(!found)throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    }
    for(auto l:source.links)if(chosen.contains(l.id)) {
        const auto old=l.id;l.id=allocateId(d,"link");links[old]=l.id;
        for(auto& lane:l.lanes){const auto oldLane=lane.id;lane.id=allocateId(d,"lane");lanes[oldLane]=lane.id;}
        for(auto& p:l.geometry){p.x+=offset.x;p.y+=offset.y;}
        created.push_back(l.id);d.network.links.push_back(std::move(l));
    }
    for(auto c:source.connectors)if(chosen.contains(c.id) || (links.contains(c.from.linkId) && links.contains(c.to.linkId))) {
        const auto original=c;c.id=allocateId(d,"connector");
        for(int i=0;i<std::max(c.fromLaneCount,c.toLaneCount);++i)paths[connectorPathId(original,i)]=connectorPathId(c,i);
        const auto remap=[&](LaneReference ref,int count,bool outgoing) {
            // A duplicated link has the same geometry, so its stations transfer unchanged.
            if(links.contains(ref.linkId))return LaneReference{links.at(ref.linkId),lanes.at(ref.laneId),ref.station};
            const auto p=laneAttachment(source,ref,outgoing);
            return dropLane(d.network,{p.x+offset.x,p.y+offset.y},count,linkLevel(source,ref.linkId));
        };
        c.from=remap(c.from,c.fromLaneCount,true);c.to=remap(c.to,c.toLaneCount,false);
        for(auto& p:c.geometry){p.x+=offset.x;p.y+=offset.y;}
        // The cursor may be anywhere within the lane: attach precisely after the drop.
        const auto a=laneAttachment(d.network,c.from,true),b=laneAttachment(d.network,c.to,false);
        const auto oldA=c.geometry.front(),oldB=c.geometry.back();const auto weights=connectorBlendWeights(c);
        for(std::size_t i=0;i<c.geometry.size();++i) {
            c.geometry[i].x+=(a.x-oldA.x)*(1-weights[i])+(b.x-oldB.x)*weights[i];
            c.geometry[i].y+=(a.y-oldA.y)*(1-weights[i])+(b.y-oldB.y)*weights[i];
        }
        c.geometry.front()=a;c.geometry.back()=b;
        created.push_back(c.id);d.network.connectors.push_back(std::move(c));
    }
    for(auto h:source.signalHeads) {
        const bool selected=chosen.contains(h.id);
        if(h.connectorId.empty() && links.contains(h.lane.linkId))h.lane={links.at(h.lane.linkId),lanes.at(h.lane.laneId)};
        else if(paths.contains(h.connectorId))h.connectorId=paths.at(h.connectorId);
        else if(selected) {
            std::vector<Point> geometry;int level=0;
            if(h.connectorId.empty()) {
                for(const auto& l:source.links)if(l.id==h.lane.linkId){geometry=laneGeometry(l,h.lane.laneId,source.drivingSide);level=l.level;}
            } else for(const auto& c:source.connectors)for(const auto& path:connectorPaths(source,c))
                if(path.id==h.connectorId){geometry=path.geometry;level=c.level;}
            const auto p=pointAlong(geometry,h.position);dropHead(d.network,h,{p.x+offset.x,p.y+offset.y},level);
        } else continue;
        h.id=allocateId(d,"head");if(selected)created.push_back(h.id);d.network.signalHeads.push_back(std::move(h));
    }
    return created; // Demand is deliberately not copied; it would double vehicle arrivals.
}
}
