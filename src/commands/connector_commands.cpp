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
// Carry the interior points with the similarity transform that maps the old endpoint chord
// onto the new one. Writing each point as the complex number z = (p-a)/(b-a) makes z an
// invariant of the curve's shape, so the transform depends only on where the endpoints are
// now, never on how they got there: moving a link away and back restores the curve exactly.
// A displacement blend relative to the current geometry cannot do this — it composes, so a
// round trip through two edits silently deformed hand-tuned curves.
void reanchor(ProjectDocument& d, Connector& c) {
    const auto from = laneGeometry(editableLink(d, c.from.linkId), c.from.laneId, d.network.drivingSide).back();
    const auto to = laneGeometry(editableLink(d, c.to.linkId), c.to.laneId, d.network.drivingSide).front();
    if (c.geometry.size() < 2) throw std::invalid_argument("INVALID_GEOMETRY");
    const auto old = c.geometry;
    const auto a = old.front(), b = old.back();
    const double vx = b.x-a.x, vy = b.y-a.y, chord = vx*vx + vy*vy;
    const double wx = to.x-from.x, wy = to.y-from.y;
    if (!std::isfinite(chord) || chord <= 0) throw std::invalid_argument("INVALID_GEOMETRY");
    for (std::size_t i = 1; i+1 < old.size(); ++i) {
        const double px = old[i].x-a.x, py = old[i].y-a.y;
        const double zr = (px*vx + py*vy)/chord, zi = (py*vx - px*vy)/chord;
        c.geometry[i] = {from.x + zr*wx - zi*wy, from.y + zr*wy + zi*wx};
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
    // Hold this to the same contract as changeGeometry: a command validates its own input
    // rather than relying on the surrounding transaction to catch it.
    if (geometry.size() < 2 || !std::isfinite(polylineLength(geometry)) || polylineLength(geometry) <= 0)
        throw std::invalid_argument("INVALID_GEOMETRY");
    for (std::size_t i = 0; i < geometry.size(); ++i)
        if (!std::isfinite(geometry[i].x) || !std::isfinite(geometry[i].y) || (i && geometry[i] == geometry[i-1]))
            throw std::invalid_argument("INVALID_GEOMETRY");
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
