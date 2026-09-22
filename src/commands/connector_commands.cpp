#include "connector_commands.hpp"
#include "detail.hpp"
#include "network_commands.hpp"
#include "../core/validate.hpp"
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
}
void anchorConnectors(ProjectDocument& d) {
    for (auto& c : d.network.connectors) anchorConnectorEnds(d.network, c);
}
void reanchorConnectors(ProjectDocument& d) {
    // A Connector keeps its own position, so a Link edit re-reads where each end has come to sit
    // rather than dragging it along. An end that is no longer on the lane it names has nothing to
    // connect: the Connector goes, with the routes and heads that named it, inside this same
    // transaction -- so one Undo brings both the Link edit and the Connector back.
    std::vector<std::string> lost;
    for (auto& c : d.network.connectors) if (!reanchorConnector(d.network, c)) lost.push_back(c.id);
    for (const auto& id : lost) deleteConnector(d, id);
}
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
    // A ROUTE is deliberately not a reference any more (M1.26): it names this Connector, not its
    // paths, so adding or removing a lane changes how many lanes the route expands to and
    // nothing else. A signal head does name a path, and still holds the range still.
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
void changeConnectorLanes(ProjectDocument& d,const std::string& id,
                          const std::vector<double>& widths,const std::vector<MarkingType>& markings) {
    auto& c=editableConnector(d,id);
    const auto paths=connectorPaths(d.network,c).size();
    const auto index=static_cast<std::size_t>(&c-d.network.connectors.data());
    // Empty is the documented "derive it from the Links" state, so clearing is always legal.
    // Anything else must describe every lane, or the drawing would silently mix an authored
    // width with a derived one and no field would say which lanes got which.
    if(!widths.empty() && widths.size()!=paths)throw std::invalid_argument("EDIT_LANES");
    // One per INTERIOR divider. paths-1 of them, and none at all on a single-lane Connector.
    if(!markings.empty() && markings.size()+1!=paths)throw std::invalid_argument("EDIT_LANES");
    for(const auto m:markings)if(!validMarking(m))throw std::invalid_argument("INVALID_MARKING");
    for(std::size_t i=0;i<widths.size();++i)if(!std::isfinite(widths[i]) || widths[i]<=0)
        throw ValidationError({{"INVALID_WIDTH",
            "connectors["+std::to_string(index)+"].laneWidths["+std::to_string(i)+"]"}});
    c.laneWidths=widths;c.laneMarkings=markings;
    // Proves the cross-section still builds with these numbers before the edit is committed.
    (void)connectorBoundaries(d.network,c);
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
    // The ends may move too: a Connector keeps its own position, which is only true if the author
    // can put that position anywhere -- including off the Link, where it is deleted below. What
    // they may NOT do is name a different lane this way; that is changeConnectorEndpoints, which
    // guards route topology. Here the reference follows the drawing, or the Connector goes.
    c.geometry = geometry;c.laneBlend.clear();
    if (!reanchorConnector(d.network, c)) deleteConnector(d, id);
}
void changeConnectorEndpoints(ProjectDocument& d, const std::string& id, LaneReference from, LaneReference to) {
    auto& c = editableConnector(d, id);
    if (c.from == from && c.to == to) return;
    if(connectorReferenced(d,c))throw std::invalid_argument("EDIT_REFERENCED_CONNECTOR");
    retargetConnector(d.network,c,std::move(from),std::move(to));
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
    const int current=static_cast<int>(c.geometry.size())-2;
    if(current==count)return;
    auto geometry=c.geometry;
    if(count>current) {
        // Raising the count must not cost the author a corner. Split the longest leg each time
        // and every existing point survives, so the Connector is drawn exactly where it was --
        // re-laying at even spacing instead cut a hand-placed corner by up to 1.00 m, measured.
        for(int added=current;added<count;++added) {
            std::size_t longest=1;double best=-1;
            for(std::size_t i=1;i<geometry.size();++i) {
                const double length=std::hypot(geometry[i].x-geometry[i-1].x,geometry[i].y-geometry[i-1].y);
                if(length>best){best=length;longest=i;}
            }
            geometry.insert(geometry.begin()+static_cast<std::ptrdiff_t>(longest),
                            {(geometry[longest].x+geometry[longest-1].x)/2,
                             (geometry[longest].y+geometry[longest-1].y)/2});
        }
    } else {
        // Lowering it has to give something up; spacing the points evenly along the shape that
        // is there gives up only the detail the lower count cannot hold, never the shape itself.
        const double length=polylineLength(c.geometry);
        if(!std::isfinite(length) || length<=0)throw std::invalid_argument("INVALID_GEOMETRY");
        geometry={c.geometry.front()};
        for(int i=1;i<=count;++i)geometry.push_back(pointAlong(c.geometry,length*i/(count+1)));
        geometry.push_back(c.geometry.back());
    }
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
