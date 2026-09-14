#include "connector_commands.hpp"
#include "detail.hpp"
#include "network_commands.hpp"
#include <algorithm>
#include <cmath>

namespace trafficsim {
Connector& editableConnector(ProjectDocument& d, const std::string& id) {
    for (auto& c : d.network.connectors) if (c.id == id) return c;
    throw std::invalid_argument("EDIT_UNKNOWN_CONNECTOR");
}
namespace {
void uniqueConnection(const ProjectDocument& d, const LaneReference& from, const LaneReference& to,
                      const std::string& except = {}) {
    for (const auto& c : d.network.connectors)
        if (c.id != except && c.from == from && c.to == to) throw std::invalid_argument("DUPLICATE_CONNECTION");
}
void reanchor(ProjectDocument& d, Connector& c) {
    const auto from = laneGeometry(editableLink(d, c.from.linkId), c.from.laneId, d.network.drivingSide).back();
    const auto to = laneGeometry(editableLink(d, c.to.linkId), c.to.laneId, d.network.drivingSide).front();
    const double total = polylineLength(c.geometry);
    if (!std::isfinite(total) || total <= 0) throw std::invalid_argument("INVALID_GEOMETRY");
    const auto old = c.geometry;
    const auto a = old.front(), b = old.back();
    double length = 0;
    for (std::size_t i = 1; i+1 < old.size(); ++i) {
        length += std::hypot(old[i].x-old[i-1].x, old[i].y-old[i-1].y);
        const double t = length/total;
        c.geometry[i] = {old[i].x+(from.x-a.x)*(1-t)+(to.x-b.x)*t,
                         old[i].y+(from.y-a.y)*(1-t)+(to.y-b.y)*t};
    }
    c.geometry.front() = from; c.geometry.back() = to;
}
}
void reanchorConnectors(ProjectDocument& d) { for (auto& c : d.network.connectors) reanchor(d, c); }
std::string addConnector(ProjectDocument& d, const LaneReference& from, const LaneReference& to) {
    uniqueConnection(d, from, to);
    auto geometry = connectorCurve(d.network, from, to);
    const auto id = allocateId(d, "connector");
    d.network.connectors.push_back({id, from, to, std::move(geometry)});
    return id;
}
bool connectorReferenced(const ProjectDocument& d,const Connector& c) {
    std::set<std::string> ids;
    for(int i=0;i<std::max(c.fromLaneCount,c.toLaneCount);++i)ids.insert(connectorPathId(c,i));
    if(d.definition)for(const auto& r:d.definition->routes)for(const auto& s:r.segmentIds)if(ids.contains(s))return true;
    for(const auto& h:d.network.signalHeads)if(ids.contains(h.connectorId))return true;
    return false;
}
std::string addConnectorRange(ProjectDocument& d,const LaneReference& from,const LaneReference& to,int fromCount,int toCount) {
    const auto id=addConnector(d,from,to);
    auto& c=editableConnector(d,id);c.fromLaneCount=fromCount;c.toLaneCount=toCount;
    c.level=editableLink(d,from.linkId).level;c.displayType=editableLink(d,from.linkId).displayType;
    (void)connectorPaths(d.network,c);return id;
}
void changeConnectorRange(ProjectDocument& d,const std::string& id,int fromCount,int toCount) {
    auto& c=editableConnector(d,id);if(c.fromLaneCount==fromCount && c.toLaneCount==toCount)return;
    if(connectorReferenced(d,c))throw std::invalid_argument("EDIT_REFERENCED_CONNECTOR");
    c.fromLaneCount=fromCount;c.toLaneCount=toCount;(void)connectorPaths(d.network,c);
}
void changeConnectorGeometry(ProjectDocument& d, const std::string& id, const std::vector<Point>& geometry) {
    auto& c = editableConnector(d, id);
    if (geometry.size() < 2) throw std::invalid_argument("INVALID_GEOMETRY");
    if (geometry.front() != c.geometry.front() || geometry.back() != c.geometry.back())
        throw std::invalid_argument("EDIT_CONNECTOR_ENDPOINTS");
    c.geometry = geometry;
}
void changeConnectorEndpoints(ProjectDocument& d, const std::string& id, LaneReference from, LaneReference to) {
    auto& c = editableConnector(d, id);
    if (c.from == from && c.to == to) return;
    if(connectorReferenced(d,c))throw std::invalid_argument("EDIT_REFERENCED_CONNECTOR");
    uniqueConnection(d, from, to, id);
    c.from = from; c.to = to;
    reanchor(d, c);
}
void resetConnectorCurve(ProjectDocument& d, const std::string& id, bool straight) {
    auto& c = editableConnector(d, id);
    auto geometry = connectorCurve(d.network, c.from, c.to);
    if (straight) geometry = {geometry.front(), geometry.back()};
    c.geometry = std::move(geometry);
}
void deleteConnector(ProjectDocument& d, const std::string& id) {
    const auto c=editableConnector(d,id);std::set<std::string> paths;
    for(int i=0;i<std::max(c.fromLaneCount,c.toLaneCount);++i)paths.insert(connectorPathId(c,i));
    std::erase_if(d.network.connectors, [&](const auto& item) { return item.id == id; });
    std::erase_if(d.network.signalHeads,[&](const auto& h){return paths.contains(h.connectorId);});
    detail::removeRoutesUsingSegments(d, paths);
}
}
