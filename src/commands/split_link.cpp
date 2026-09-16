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
    // An attachment in the small continuity span cannot remain on either child link.
    // The station is already measured along this link's own geometry, so the cut compares directly.
    for(const auto& c:d.network.connectors)for(const auto& ref:{c.from,c.to})
        if(ref.linkId==id && ref.station && *ref.station>distance-.1 && *ref.station<distance+.1)
            throw std::invalid_argument("EDIT_SPLIT_ATTACHMENT");
    auto downstream = original;
    downstream.id = allocateId(d, "link");
    downstream.geometry = section(original.geometry, distance + 0.1, total);
    std::map<std::string, std::pair<std::string, std::string>> replacements;
    for (auto& lane : downstream.lanes) {
        const auto old = lane.id;
        lane.id = allocateId(d, "lane");
        replacements[old] = {allocateId(d, "connector"), lane.id};
    }
    if (pocket) {
        auto lanes=downstream.lanes;lanes.push_back({allocateId(d,"lane"),original.lanes.back().width});
        replaceLaneBundle(downstream,std::move(lanes),false);
    }
    editableLink(d, id).geometry = section(original.geometry, 0, distance - 0.1);
    d.network.links.push_back(downstream);
    // Stations carry across the cut by arithmetic alone: the upstream child's polyline is a
    // prefix of the original, and the downstream child starts at the far side of the span.
    for(auto& c:d.network.connectors)for(auto* ref:{&c.from,&c.to})if(ref->linkId==id) {
        if(!ref->station) {
            if(ref==&c.from)*ref={downstream.id,replacements.at(ref->laneId).second};
            continue;
        }
        if(*ref->station>=distance+.1) {
            ref->linkId=downstream.id;ref->laneId=replacements.at(ref->laneId).second;
            ref->station=*ref->station-(distance+.1);
        }
    }
    // Reanchor external connectors, then create explicit one-to-one continuity connectors.
    changeGeometry(d, id, editableLink(d, id).geometry);
    for (const auto& lane : original.lanes) {
        const auto& [connector, nextLane] = replacements.at(lane.id);
        const auto a = laneGeometry(editableLink(d, id), lane.id, d.network.drivingSide).back();
        const auto b = laneGeometry(downstream, nextLane, d.network.drivingSide).front();
        d.network.connectors.push_back({connector, {id, lane.id}, {downstream.id, nextLane}, {a, b},1,1,original.level,original.displayType});
    }
    // Classify by centreline station, then project the original world point onto the new
    // owning lane/span. At the first cut boundary the upstream lane owns the head; at
    // the second boundary the downstream lane owns it. Interior gap heads stay on the
    // explicit connector, never silently shifted to a lane endpoint.
    for(auto& h:d.network.signalHeads) if(h.connectorId.empty() && h.lane.linkId==id) {
        const auto oldLane=h.lane.laneId;
        const auto world=pointAlong(laneGeometry(original,oldLane,d.network.drivingSide),h.position);
        const double station=stationOfClosestPoint(original.geometry,world);
        const auto& [connector,nextLane]=replacements.at(oldLane);
        std::vector<Point> geometry;
        if(station<=distance-0.1) geometry=laneGeometry(editableLink(d,id),oldLane,d.network.drivingSide);
        else if(station>=distance+0.1) {
            h.lane={downstream.id,nextLane};geometry=laneGeometry(downstream,nextLane,d.network.drivingSide);
        } else {
            h.connectorId=connector;h.lane={};
            for(const auto& c:d.network.connectors)if(c.id==connector)geometry=c.geometry;
        }
        h.position=stationOfClosestPoint(geometry,world);
    }
    if (d.definition) for (auto& r : d.definition->routes) {
        std::vector<std::string> ids;
        for (const auto& s : r.segmentIds) {
            ids.push_back(s);
            const auto match = replacements.find(s);
            if (match != replacements.end()) { ids.push_back(match->second.first); ids.push_back(match->second.second); }
        }
        r.segmentIds = std::move(ids);
    }
    return downstream.id;
}
}
