#include "network_commands.hpp"
#include <algorithm>
#include <cmath>
#include <map>

namespace trafficsim {
namespace {
std::vector<Point> section(const std::vector<Point>& geometry, double start, double end) {
    std::vector<Point> result{pointAlong(geometry, start)};
    double distance = 0;
    for (std::size_t i = 1; i < geometry.size(); ++i) {
        distance += std::hypot(geometry[i].x - geometry[i-1].x, geometry[i].y - geometry[i-1].y);
        if (distance > start && distance < end) result.push_back(geometry[i]);
    }
    result.push_back(pointAlong(geometry, end)); return result;
}
}
std::string splitLink(ProjectDocument& d, const std::string& id, double distance, bool pocket) {
    const auto original = editableLink(d, id);
    const auto total = polylineLength(original.geometry);
    // A 0.2 m connector span keeps runtime segments strictly positive, even on a straight road.
    if (!std::isfinite(distance) || distance <= 0.2 || distance >= total - 0.2)
        throw std::invalid_argument("EDIT_SPLIT_RANGE");
    if (pocket && original.lanes.size() >= 12) throw std::invalid_argument("EDIT_LANES");
    // Moving an existing head at a split requires a stationing policy; never silently shift it.
    for (const auto& h : d.network.signalHeads) if (h.lane.linkId == id)
        throw std::invalid_argument("EDIT_SPLIT_SIGNAL");
    auto downstream = original;
    downstream.id = allocateId(d, "link");
    downstream.geometry = section(original.geometry, distance + 0.1, total);
    std::map<std::string, std::pair<std::string, std::string>> replacements;
    for (auto& lane : downstream.lanes) {
        const auto old = lane.id;
        lane.id = allocateId(d, "lane");
        replacements[old] = {allocateId(d, "connector"), lane.id};
    }
    if (pocket) downstream.lanes.push_back({allocateId(d, "lane"), original.lanes.back().width});
    editableLink(d, id).geometry = section(original.geometry, 0, distance - 0.1);
    d.network.links.push_back(downstream);
    for (auto& c : d.network.connectors) if (c.from.linkId == id) {
        c.from = {downstream.id, replacements.at(c.from.laneId).second};
    }
    // Reanchor external connectors, then create explicit one-to-one continuity connectors.
    changeGeometry(d, id, editableLink(d, id).geometry);
    for (const auto& lane : original.lanes) {
        const auto& [connector, nextLane] = replacements.at(lane.id);
        const auto a = laneGeometry(editableLink(d, id), lane.id, d.network.drivingSide).back();
        const auto b = laneGeometry(downstream, nextLane, d.network.drivingSide).front();
        d.network.connectors.push_back({connector, {id, lane.id}, {downstream.id, nextLane}, {a, b}});
    }
    if (!d.definition.is_null()) for (auto& r : d.definition["routes"]) {
        Json ids = Json::array();
        for (const auto& s : r.at("segmentIds")) {
            ids.push_back(s);
            const auto match = replacements.find(s.get<std::string>());
            if (match != replacements.end()) { ids.push_back(match->second.first); ids.push_back(match->second.second); }
        }
        r["segmentIds"] = std::move(ids);
    }
    return downstream.id;
}
}
