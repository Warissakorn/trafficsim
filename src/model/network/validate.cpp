#include "network.hpp"
#include "../../core/validate.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <utility>
#include <tuple>

namespace trafficsim {
std::vector<ValidationIssue> validateNetwork(const Network& network) {
    std::vector<ValidationIssue> issues;
    const auto add = [&](const std::string& code, const std::string& path) { issues.push_back({code, path}); };
    std::set<std::string> ids;
    const auto id = [&](const std::string& value, const std::string& path) {
        if (value.find_first_not_of(" \t\n\r") == std::string::npos) add("INVALID_ID", path);
        else if (!ids.insert(value).second) add("DUPLICATE_ID", path);
    };
    const auto geometry = [&](const std::vector<Point>& points, const std::string& path) {
        bool valid = points.size() >= 2 && std::isfinite(polylineLength(points)) && polylineLength(points) > 0;
        for (std::size_t i = 0; i < points.size(); ++i)
            if (!std::isfinite(points[i].x) || !std::isfinite(points[i].y) ||
                (i > 0 && points[i] == points[i - 1])) valid = false;
        if (!valid) add("INVALID_GEOMETRY", path);
    };
    const auto resolve = [&](const LaneReference& ref, const std::string& path) -> const Link* {
        for (const auto& link : network.links)
            if (link.id == ref.linkId && std::any_of(link.lanes.begin(), link.lanes.end(),
                [&](const auto& lane) { return lane.id == ref.laneId; })) {
                // A station is bounded by the link it names, so this check follows the lookup.
                if(ref.station && (!std::isfinite(*ref.station) || *ref.station<0 ||
                                   *ref.station>polylineLength(link.geometry))) {
                    add("EDIT_CONNECTOR_POSITION",path+".station");return nullptr;
                }
                return &link;
            }
        add("UNKNOWN_LANE", path);
        return nullptr;
    };
    id(network.id, "id");
    const bool validSide = network.drivingSide == DrivingSide::left || network.drivingSide == DrivingSide::right;
    if (!validSide) add("INVALID_DRIVING_SIDE", "drivingSide");
    if (network.links.empty()) add("EMPTY_NETWORK", "links");
    for (std::size_t i = 0; i < network.links.size(); ++i) {
        const auto& link = network.links[i];
        const auto p = "links[" + std::to_string(i) + "]";
        id(link.id, p + ".id"); geometry(link.geometry, p + ".geometry");
        if(link.level < -1000 || link.level > 1000 || link.displayType.empty())add("EDIT_DISPLAY_VALUE",p+".displayType");
        if(!std::isfinite(link.laneOffset))add("INVALID_GEOMETRY",p+".laneOffset");
        if (link.lanes.empty()) add("NO_LANES", p + ".lanes");
        if(link.lanes.size()>12)add("EDIT_LANES",p+".lanes");
        if(!link.boundaryMarkings.empty() && link.boundaryMarkings.size()!=link.lanes.size()+1)
            add("EDIT_LANES",p+".boundaryMarkings");
        for(std::size_t j=0;j<link.boundaryMarkings.size();++j)
            if(!validMarking(link.boundaryMarkings[j]))add("INVALID_MARKING",p+".boundaryMarkings["+std::to_string(j)+"]");
        for (std::size_t j = 0; j < link.lanes.size(); ++j) {
            const auto q = p + ".lanes[" + std::to_string(j) + "]";
            id(link.lanes[j].id, q + ".id");
            if (!std::isfinite(link.lanes[j].width) || link.lanes[j].width <= 0) add("INVALID_WIDTH", q + ".width");
        }
    }
    std::set<std::tuple<std::string,double,std::string,double>> connections;
    for (std::size_t i = 0; i < network.connectors.size(); ++i) {
        const auto& c = network.connectors[i];
        const auto p = "connectors[" + std::to_string(i) + "]";
        id(c.id, p + ".id"); geometry(c.geometry, p + ".geometry");
        const auto count=static_cast<std::size_t>(std::max(0,std::max(c.fromLaneCount,c.toLaneCount)));
        if(!c.laneWidths.empty() && c.laneWidths.size()!=count)add("EDIT_LANES",p+".laneWidths");
        if(!c.laneMarkings.empty() && c.laneMarkings.size()+1!=count)add("EDIT_LANES",p+".laneMarkings");
        for(std::size_t j=0;j<c.laneWidths.size();++j)
            if(!std::isfinite(c.laneWidths[j]) || c.laneWidths[j]<=0)
                add("INVALID_WIDTH",p+".laneWidths["+std::to_string(j)+"]");
        for(std::size_t j=0;j<c.laneMarkings.size();++j)
            if(!validMarking(c.laneMarkings[j]))add("INVALID_MARKING",p+".laneMarkings["+std::to_string(j)+"]");
        if(c.level < -1000 || c.level > 1000 || c.displayType.empty())add("EDIT_DISPLAY_VALUE",p+".displayType");
        const auto* from = resolve(c.from, p + ".from"); const auto* to = resolve(c.to, p + ".to");
        if(from && to && validSide) try {
            for(const auto& path:connectorPaths(network,c)) {
                if(path.id!=c.id)id(path.id,p+".id");
                geometry(path.geometry,p+".geometry");
                if(!connections.emplace(path.from.laneId,attachmentStation(network,path.from,true),
                                        path.to.laneId,attachmentStation(network,path.to,false)).second)
                    add("DUPLICATE_CONNECTION",p);
            }
        }catch(const std::exception&){add("EDIT_LANE_RANGE",p);}
        const auto check = [&](const Link* link, const LaneReference& ref, bool end) {
            if (!link || link->geometry.size() < 2 || c.geometry.empty() || !validSide) return;
            const auto expected = laneAttachment(network,ref,end);
            const auto endpoint = end ? c.geometry.front() : c.geometry.back();
            if (std::hypot(endpoint.x - expected.x, endpoint.y - expected.y) > 0.01)
                add("DISCONNECTED_GEOMETRY", p + (end ? ".from" : ".to"));
        };
        check(from, c.from, true); check(to, c.to, false);
    }
    for (std::size_t i = 0; i < network.signalHeads.size(); ++i) {
        const auto& head = network.signalHeads[i];
        const auto p = "signalHeads[" + std::to_string(i) + "]";
        id(head.id, p + ".id");
        double length=-1;
        if (head.connectorId.empty()) {
            const auto* link = resolve(head.lane, p + ".lane");
            if(link && validSide) length=polylineLength(laneGeometry(*link,head.lane.laneId,network.drivingSide));
        } else {
            for(const auto& c:network.connectors)try {
                for(const auto& path:connectorPaths(network,c))if(path.id==head.connectorId)length=polylineLength(path.geometry);
            }catch(const std::exception&){ /* Range errors are already reported above. */ }
            if(length<0) add("UNKNOWN_SEGMENT",p+".connectorId");
            if(!head.lane.linkId.empty() || !head.lane.laneId.empty())add("EDIT_HEAD_REFERENCE",p+".lane");
        }
        if (head.programId.find_first_not_of(" \t\r\n") == std::string::npos) add("INVALID_ID", p + ".programId");
        if (!std::isfinite(head.position) || head.position < 0 || (length>=0 && head.position > length))
            add("INVALID_POSITION", p + ".position");
    }
    return issues;
}
void assertValidNetwork(const Network& network) {
    auto issues = validateNetwork(network);
    if (!issues.empty()) throw ValidationError(std::move(issues));
}
}
