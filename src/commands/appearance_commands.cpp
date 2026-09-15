#include "appearance_commands.hpp"
#include "network_commands.hpp"
#include <algorithm>
#include <map>
#include <set>
namespace trafficsim {
void changeAppearance(ProjectDocument& d,const std::string& id,int level,const std::string& type) {
    if(level < -1000 || level > 1000 || type.empty())throw std::invalid_argument("EDIT_DISPLAY_VALUE");
    for(auto& l:d.network.links)if(l.id==id){l.level=level;l.displayType=type;return;}
    for(auto& c:d.network.connectors)if(c.id==id){c.level=level;c.displayType=type;return;}
    throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
std::vector<std::string> duplicateObjects(ProjectDocument& d,const std::vector<std::string>& ids,Point offset) {
    const auto source=d.network;std::map<std::string,std::string> links,lanes,paths;std::vector<std::string> created;
    const std::set<std::string> chosen(ids.begin(),ids.end());
    for(auto l:source.links)if(chosen.contains(l.id)) {
        const auto old=l.id;l.id=allocateId(d,"link");links[old]=l.id;
        for(auto& lane:l.lanes){const auto oldLane=lane.id;lane.id=allocateId(d,"lane");lanes[oldLane]=lane.id;}
        for(auto& p:l.geometry){p.x+=offset.x;p.y+=offset.y;}
        created.push_back(l.id);d.network.links.push_back(std::move(l));
    }
    if(created.empty())throw std::invalid_argument("EDIT_DUPLICATE_LINKS");
    for(auto c:source.connectors)if(links.contains(c.from.linkId) && links.contains(c.to.linkId)) {
        const auto original=c;c.id=allocateId(d,"connector");
        for(int i=0;i<std::max(c.fromLaneCount,c.toLaneCount);++i)paths[connectorPathId(original,i)]=connectorPathId(c,i);
        c.from={links.at(c.from.linkId),lanes.at(c.from.laneId),c.from.fraction};c.to={links.at(c.to.linkId),lanes.at(c.to.laneId),c.to.fraction};
        for(auto& p:c.geometry){p.x+=offset.x;p.y+=offset.y;}
        created.push_back(c.id);d.network.connectors.push_back(std::move(c));
    }
    for(auto h:source.signalHeads) {
        if(h.connectorId.empty() && links.contains(h.lane.linkId))h.lane={links.at(h.lane.linkId),lanes.at(h.lane.laneId)};
        else if(paths.contains(h.connectorId))h.connectorId=paths.at(h.connectorId);else continue;
        h.id=allocateId(d,"head");d.network.signalHeads.push_back(std::move(h));
    }
    return created; // Demand is deliberately not copied; it would double vehicle arrivals.
}
}
