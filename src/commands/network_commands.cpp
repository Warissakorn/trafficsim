#include "network_commands.hpp"
#include "connector_commands.hpp"
#include "detail.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace trafficsim {
Link& editableLink(ProjectDocument& d, const std::string& id) {
    for (auto& l : d.network.links) if (l.id == id) return l;
    throw std::invalid_argument("EDIT_UNKNOWN_LINK");
}
void detail::removeRoutesUsingSegments(ProjectDocument& d, const std::set<std::string>& segments) {
    if (!d.definition) return;
    std::set<std::string> routes;
    for (const auto& r : d.definition->routes) for (const auto& segment : r.segmentIds)
        if (segments.contains(segment)) routes.insert(r.id);
    std::erase_if(d.definition->routes, [&](const auto& r) { return routes.contains(r.id); });
    std::erase_if(d.definition->inputs, [&](const auto& i) { return routes.contains(i.routeId); });
}
std::string addLink(ProjectDocument& d, const std::vector<Point>& geometry, int lanes, double width) {
    if (lanes < 1 || lanes > 12 || !std::isfinite(width) || width <= 0) throw std::invalid_argument("EDIT_LANES");
    Link link{allocateId(d, "link"), geometry, {}};
    for (int i = 0; i < lanes; ++i) link.lanes.push_back({allocateId(d, "lane"), width});
    const auto id = link.id; d.network.links.push_back(std::move(link)); return id;
}
void changeGeometry(ProjectDocument& d, const std::string& id, const std::vector<Point>& geometry) {
    if (geometry.size() < 2 || !std::isfinite(polylineLength(geometry)) || polylineLength(geometry) <= 0)
        throw std::invalid_argument("INVALID_GEOMETRY");
    for (std::size_t i=0; i<geometry.size(); ++i)
        if (!std::isfinite(geometry[i].x) || !std::isfinite(geometry[i].y) || (i && geometry[i]==geometry[i-1]))
            throw std::invalid_argument("INVALID_GEOMETRY");
    editableLink(d, id).geometry = geometry; reanchorConnectors(d);
}
namespace {
void checkRemovedLanes(const ProjectDocument& d,const std::set<std::string>& removed) {
    // A connector whose range is already invalid cannot be reported as a lane reference; leave
    // that to validation, which names it properly, instead of throwing EDIT_LANE_RANGE here.
    for (const auto& c : d.network.connectors) {
        std::vector<ConnectorPath> paths;
        try { paths = connectorPaths(d.network, c); } catch (const std::exception&) { continue; }
        for (const auto& path : paths)
            if (removed.contains(path.from.laneId) || removed.contains(path.to.laneId))
                throw std::invalid_argument("EDIT_REFERENCED_LANE");
    }
    for (const auto& h : d.network.signalHeads)
        if (removed.contains(h.lane.laneId)) throw std::invalid_argument("EDIT_REFERENCED_LANE");
    if (d.definition) for (const auto& r : d.definition->routes) for (const auto& s : r.segmentIds)
        if (removed.contains(s)) throw std::invalid_argument("EDIT_REFERENCED_LANE");
}
}
void changeLanes(ProjectDocument& d, const std::string& id, const std::vector<double>& widths) {
    if(widths.empty() || widths.size()>12)
        throw std::invalid_argument("EDIT_LANES");
    auto& l=editableLink(d,id);std::set<std::string> removed;
    for(std::size_t i=0;i<widths.size();++i)if(!std::isfinite(widths[i]) || widths[i]<=0) {
        const auto index=static_cast<std::size_t>(&l-d.network.links.data());
        throw ValidationError({{"INVALID_WIDTH","links["+std::to_string(index)+"].lanes["+std::to_string(i)+"].width"}});
    }
    for(std::size_t i=widths.size();i<l.lanes.size();++i)removed.insert(l.lanes[i].id);
    checkRemovedLanes(d,removed);
    auto lanes=l.lanes;
    while(lanes.size()<widths.size())lanes.push_back({allocateId(d,"lane"),widths[lanes.size()]});
    lanes.resize(widths.size());
    for(std::size_t i=0;i<widths.size();++i)lanes[i].width=widths[i];
    replaceLaneBundle(l,std::move(lanes),false);anchorConnectors(d);
}
void resizeLinkLanes(ProjectDocument& d,const std::string& id,int count,bool leading) {
    if(count<1 || count>12)throw std::invalid_argument("EDIT_LANES");
    auto& link=editableLink(d,id);auto lanes=link.lanes;std::set<std::string> removed;
    const double width=(leading?lanes.front():lanes.back()).width;
    while(static_cast<int>(lanes.size())>count) {
        const auto at=leading?lanes.begin():lanes.end()-1;removed.insert(at->id);lanes.erase(at);
    }
    checkRemovedLanes(d,removed);
    while(static_cast<int>(lanes.size())<count)lanes.insert(leading?lanes.begin():lanes.end(),{allocateId(d,"lane"),width});
    replaceLaneBundle(link,std::move(lanes),leading);anchorConnectors(d);
}

void deleteLink(ProjectDocument& d, const std::string& id) {
    const auto l = editableLink(d, id);
    std::set<std::string> removed;
    for (const auto& lane : l.lanes) removed.insert(lane.id);
    for (const auto& c : d.network.connectors) if (c.from.linkId == id || c.to.linkId == id)
        for(int i=0;i<std::max(c.fromLaneCount,c.toLaneCount);++i)removed.insert(connectorPathId(c,i));
    std::erase_if(d.network.connectors, [&](const auto& c) { return removed.contains(c.id); });
    std::erase_if(d.network.signalHeads, [&](const auto& h) { return h.lane.linkId == id || removed.contains(h.connectorId); });
    std::erase_if(d.network.links, [&](const auto& link) { return link.id == id; });
    detail::removeRoutesUsingSegments(d, removed);
}
void changeDrivingSide(ProjectDocument& d, DrivingSide side) { d.network.drivingSide = side; anchorConnectors(d); }
std::string oppositeLink(ProjectDocument& d, const std::string& id, double gap) {
    if (!std::isfinite(gap) || gap < 0) throw std::invalid_argument("EDIT_GAP");
    const auto original = editableLink(d, id);
    double width = 0;
    for (const auto& lane : original.lanes) width += lane.width;
    auto helper = original;
    // Offset the opposite centreline to the median side of the current carriageway.
    helper.lanes = {{"other", 2 * width + 2 * gap}, {"offset", width}};
    auto geometry = laneGeometry(helper, "offset", d.network.drivingSide);
    std::reverse(geometry.begin(), geometry.end());
    const auto created = addLink(d, geometry, static_cast<int>(original.lanes.size()), original.lanes.front().width);
    auto& other=editableLink(d,created);
    // This new road is already positioned by its final total width; do not anchor an
    // edge while assigning unequal widths, which would change the requested median gap.
    for(std::size_t i=0;i<other.lanes.size();++i)other.lanes[i].width=original.lanes[i].width;
    other.level=original.level;other.displayType=original.displayType;return created;
}
}
