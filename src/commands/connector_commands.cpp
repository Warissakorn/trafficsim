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
// The similarity transform that carries the interior points lives in the model, beside the
// attachments it reads; commands only decide when a connector is re-anchored.
void reanchor(ProjectDocument& d, Connector& c) { reanchorConnector(d.network, c); }
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
void changeConnectorRange(ProjectDocument& d,const std::string& id,int fromCount,int toCount,bool leading) {
    auto& c=editableConnector(d,id);if(c.fromLaneCount==fromCount && c.toLaneCount==toCount)return;
    if(connectorReferenced(d,c))throw std::invalid_argument("EDIT_REFERENCED_CONNECTOR");
    resizeConnectorEdges(d.network,c,fromCount,toCount,leading);
}
void changeConnectorGeometry(ProjectDocument& d, const std::string& id, const std::vector<Point>& geometry) {
    auto& c = editableConnector(d, id);
    // Hold this to the same contract as changeGeometry: a command validates its own input
    // rather than relying on the surrounding transaction to catch it.
    if (geometry.size() < 2 || !std::isfinite(polylineLength(geometry)) || polylineLength(geometry) <= 0)
        throw std::invalid_argument("INVALID_GEOMETRY");
    for (std::size_t i = 0; i < geometry.size(); ++i)
        if (!std::isfinite(geometry[i].x) || !std::isfinite(geometry[i].y) || (i && geometry[i] == geometry[i-1]))
            throw std::invalid_argument("INVALID_GEOMETRY");
    if (geometry.front() != c.geometry.front() || geometry.back() != c.geometry.back())
        throw std::invalid_argument("EDIT_CONNECTOR_ENDPOINTS");
    c.geometry = geometry;c.laneBlend.clear();
}
void changeConnectorEndpoints(ProjectDocument& d, const std::string& id, LaneReference from, LaneReference to) {
    auto& c = editableConnector(d, id);
    if (c.from == from && c.to == to) return;
    if(connectorReferenced(d,c))throw std::invalid_argument("EDIT_REFERENCED_CONNECTOR");
    uniqueConnection(d, from, to, id);
    // The new end may have fewer lanes than the old one. Narrow the range to what is there
    // instead of rejecting the move; a wider link never widens the range on its own, because
    // how many lanes a connector carries is the author's decision, not the link's.
    auto moved = c; moved.from = from; moved.to = to;
    moved.fromLaneCount = std::max(1, std::min(c.fromLaneCount, lanesFromReference(d.network, from)));
    moved.toLaneCount = std::max(1, std::min(c.toLaneCount, lanesFromReference(d.network, to)));
    // Validate before mutating: an unknown lane must not leave a half-moved connector behind.
    (void)connectorPaths(d.network, moved);
    reanchorConnector(d.network, moved);
    c = std::move(moved);
}
void resetConnectorCurve(ProjectDocument& d, const std::string& id, bool straight) {
    auto& c = editableConnector(d, id);
    auto geometry = connectorCurve(d.network, c.from, c.to);
    if (straight) geometry = {geometry.front(), geometry.back()};
    c.geometry = std::move(geometry);c.laneBlend.clear();
}
void resampleConnectorPoints(ProjectDocument& d, const std::string& id, int count) {
    if(count<0 || count>40)throw std::invalid_argument("EDIT_CONNECTOR_POINTS");
    auto& c = editableConnector(d, id);
    if(static_cast<int>(c.geometry.size())-2==count)return;
    const auto road=connectorRoad(d.network,c);
    const double length=polylineLength(road);
    if(!std::isfinite(length) || length<=0)throw std::invalid_argument("INVALID_GEOMETRY");
    std::vector<Point> geometry{c.geometry.front()};
    for(int i=1;i<=count;++i)geometry.push_back(pointAlong(road,length*i/(count+1)));
    geometry.push_back(c.geometry.back());
    // The shape is only re-laid, never re-derived: a Connector the author has bent by hand keeps
    // its bend when the count goes up and gives up only the detail the lower count cannot hold.
    c.geometry=std::move(geometry);c.laneBlend.clear();
}
void deleteConnector(ProjectDocument& d, const std::string& id) {
    const auto c=editableConnector(d,id);std::set<std::string> paths;
    for(int i=0;i<std::max(c.fromLaneCount,c.toLaneCount);++i)paths.insert(connectorPathId(c,i));
    std::erase_if(d.network.connectors, [&](const auto& item) { return item.id == id; });
    std::erase_if(d.network.signalHeads,[&](const auto& h){return paths.contains(h.connectorId);});
    detail::removeRoutesUsingSegments(d, paths);
}
}
