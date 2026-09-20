#include "network_commands.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace trafficsim {
void straightenLink(ProjectDocument& d,const std::string& id) {
    const auto& g=editableLink(d,id).geometry;
    changeGeometry(d,id,{g.front(),g.back()});
}
void insertLinkPoint(ProjectDocument& d,const std::string& id,double station) {
    auto geometry=editableLink(d,id).geometry;
    const double length=polylineLength(geometry);
    if(!std::isfinite(station) || station<=0 || station>=length)
        throw std::invalid_argument("EDIT_LINK_STATION");
    double start=0;
    for(std::size_t i=1;i<geometry.size();++i) {
        const auto a=geometry[i-1],b=geometry[i];
        const double end=start+std::hypot(b.x-a.x,b.y-a.y);
        // Existing vertices are a no-op, preserving revision, dirty state and redo history.
        if(station==end)return;
        if(station<end) {
            const double t=(station-start)/(end-start);
            const Point point{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t};
            if(point==a || point==b)return; // A sub-ulp insertion must not create duplicates.
            geometry.insert(geometry.begin()+static_cast<std::ptrdiff_t>(i),point);
            changeGeometry(d,id,geometry);return;
        }
        start=end;
    }
}
void addLinkIntermediatePoint(ProjectDocument& d,const std::string& id) {
    const auto& g=editableLink(d,id).geometry;
    double longest=0,station=0,start=0;
    for(std::size_t i=1;i<g.size();++i) {
        const double length=std::hypot(g[i].x-g[i-1].x,g[i].y-g[i-1].y);
        // First longest segment wins ties, independent of any unordered container.
        if(length>longest){longest=length;station=start+length/2;}
        start+=length;
    }
    insertLinkPoint(d,id,station);
}
void reverseLink(ProjectDocument& d,const std::string& id) {
    auto& link=editableLink(d,id);
    for(const auto& c:d.network.connectors)
        if(c.from.linkId==id || c.to.linkId==id)throw std::invalid_argument("EDIT_REFERENCED_LINK");
    for(const auto& head:d.network.signalHeads)
        if(head.lane.linkId==id)throw std::invalid_argument("EDIT_REFERENCED_LINK");
    std::set<std::string> lanes;
    for(const auto& lane:link.lanes)lanes.insert(lane.id);
    if(d.definition)for(const auto& route:d.definition->routes)for(const auto& segment:route.segmentIds)
        if(lanes.contains(segment))throw std::invalid_argument("EDIT_REFERENCED_LINK");
    std::reverse(link.geometry.begin(),link.geometry.end());
    std::reverse(link.lanes.begin(),link.lanes.end());
    std::reverse(link.boundaryMarkings.begin(),link.boundaryMarkings.end());
    link.laneOffset=-link.laneOffset;
}
void changeLinkMarkings(ProjectDocument& d,const std::string& id,const std::vector<MarkingType>& markings) {
    auto& link=editableLink(d,id);
    if(!markings.empty() && markings.size()!=link.lanes.size()+1)throw std::invalid_argument("EDIT_LANES");
    for(const auto m:markings)if(!validMarking(m))throw std::invalid_argument("INVALID_MARKING");
    link.boundaryMarkings=markings;
}
}
