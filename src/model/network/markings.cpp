#include "network.hpp"
#include <stdexcept>

namespace trafficsim {
bool validMarking(MarkingType m) {
    return m==MarkingType::solid || m==MarkingType::dashed ||
           m==MarkingType::none || m==MarkingType::doubleLine;
}
const char* markingName(MarkingType m) {
    switch(m) {
    case MarkingType::solid: return "solid";
    case MarkingType::dashed: return "dashed";
    case MarkingType::none: return "none";
    case MarkingType::doubleLine: return "double";
    }
    throw std::invalid_argument("INVALID_MARKING");
}
MarkingType markingFromName(const std::string& name) {
    if(name=="solid")return MarkingType::solid;
    if(name=="dashed")return MarkingType::dashed;
    if(name=="none")return MarkingType::none;
    if(name=="double")return MarkingType::doubleLine;
    throw std::invalid_argument("INVALID_MARKING");
}
std::vector<ConnectorMarking> linkMarkings(const Link& link,DrivingSide side) {
    if(!link.boundaryMarkings.empty() && link.boundaryMarkings.size()!=link.lanes.size()+1)
        throw std::invalid_argument("EDIT_LANES");
    std::vector<ConnectorMarking> result;
    for(std::size_t i=0;i<=link.lanes.size();++i) {
        const bool edge=i==0 || i==link.lanes.size();
        const auto type=link.boundaryMarkings.empty() ?
            (edge?MarkingType::solid:MarkingType::dashed):link.boundaryMarkings[i];
        if(!validMarking(type))throw std::invalid_argument("INVALID_MARKING");
        result.push_back({trimSelfIntersections(laneBoundaryGeometry(link,i,side)),edge,type});
    }
    return result;
}
std::vector<ConnectorMarking> markingStrokes(const std::vector<ConnectorMarking>& markings) {
    std::vector<ConnectorMarking> result;
    for(const auto& m:markings) {
        if(!validMarking(m.type))throw std::invalid_argument("INVALID_MARKING");
        if(m.type==MarkingType::none)continue;
        if(m.type==MarkingType::doubleLine) {
            for(double offset:{-.075,.075})
                result.push_back({trimSelfIntersections(offsetGeometry(m.geometry,offset)),m.edge,MarkingType::solid});
        } else result.push_back(m);
    }
    return result;
}
}
