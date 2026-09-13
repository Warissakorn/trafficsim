#include "network_commands.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace trafficsim {
Link& editableLink(ProjectDocument& d, const std::string& id) {
    for (auto& l : d.network.links) if (l.id == id) return l;
    throw std::invalid_argument("EDIT_UNKNOWN_LINK");
}
namespace {
void reanchor(ProjectDocument& d) {
    for (auto& c : d.network.connectors) {
        const auto from = laneGeometry(editableLink(d, c.from.linkId), c.from.laneId, d.network.drivingSide).back();
        const auto to = laneGeometry(editableLink(d, c.to.linkId), c.to.laneId, d.network.drivingSide).front();
        const auto a = c.geometry.front(), b = c.geometry.back();
        const auto total = polylineLength(c.geometry);
        double length = 0;
        const auto old = c.geometry;
        for (std::size_t i = 0; i < old.size(); ++i) {
            if (i) length += std::hypot(old[i].x - old[i-1].x, old[i].y - old[i-1].y);
            const double t = length / total;
            c.geometry[i] = {old[i].x + (from.x-a.x)*(1-t) + (to.x-b.x)*t,
                             old[i].y + (from.y-a.y)*(1-t) + (to.y-b.y)*t};
        }
    }
}
void removeRoutes(ProjectDocument& d, const std::set<std::string>& segments) {
    if (d.definition.is_null()) return;
    std::set<std::string> routes;
    auto& rs = d.definition["routes"];
    for (const auto& r : rs) for (const auto& segment : r.at("segmentIds"))
        if (segments.contains(segment.get<std::string>())) routes.insert(r.at("id").get<std::string>());
    std::erase_if(rs.get_ref<Json::array_t&>(), [&](const auto& r) { return routes.contains(r.at("id").template get<std::string>()); });
    auto& inputs = d.definition["inputs"].get_ref<Json::array_t&>();
    std::erase_if(inputs, [&](const auto& i) { return routes.contains(i.at("routeId").template get<std::string>()); });
}
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
    editableLink(d, id).geometry = geometry; reanchor(d);
}
void changeLanes(ProjectDocument& d, const std::string& id, const std::vector<double>& widths) {
    if (widths.empty() || widths.size() > 12) throw std::invalid_argument("EDIT_LANES");
    auto& l = editableLink(d, id);
    std::set<std::string> removed;
    for (std::size_t i = widths.size(); i < l.lanes.size(); ++i) removed.insert(l.lanes[i].id);
    for (const auto& c : d.network.connectors)
        if (removed.contains(c.from.laneId) || removed.contains(c.to.laneId)) throw std::invalid_argument("EDIT_REFERENCED_LANE");
    for (const auto& h : d.network.signalHeads)
        if (removed.contains(h.lane.laneId)) throw std::invalid_argument("EDIT_REFERENCED_LANE");
    if (!d.definition.is_null()) for (const auto& r : d.definition.at("routes")) for (const auto& s : r.at("segmentIds"))
        if (removed.contains(s.get<std::string>())) throw std::invalid_argument("EDIT_REFERENCED_LANE");
    while (l.lanes.size() < widths.size()) l.lanes.push_back({allocateId(d, "lane"), widths[l.lanes.size()]});
    l.lanes.resize(widths.size());
    for (std::size_t i = 0; i < widths.size(); ++i) l.lanes[i].width = widths[i];
    reanchor(d);
}
void deleteLink(ProjectDocument& d, const std::string& id) {
    const auto l = editableLink(d, id);
    std::set<std::string> removed;
    for (const auto& lane : l.lanes) removed.insert(lane.id);
    for (const auto& c : d.network.connectors) if (c.from.linkId == id || c.to.linkId == id) removed.insert(c.id);
    std::erase_if(d.network.connectors, [&](const auto& c) { return removed.contains(c.id); });
    std::erase_if(d.network.signalHeads, [&](const auto& h) { return h.lane.linkId == id; });
    std::erase_if(d.network.links, [&](const auto& link) { return link.id == id; });
    removeRoutes(d, removed);
}
void changeDrivingSide(ProjectDocument& d, DrivingSide side) { d.network.drivingSide = side; reanchor(d); }
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
    std::vector<double> widths;
    for (const auto& lane : original.lanes) widths.push_back(lane.width);
    changeLanes(d, created, widths); return created;
}
}
