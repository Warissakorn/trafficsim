#include "network.hpp"
#include <cmath>

namespace trafficsim {
std::string signalGroupProgramId(const std::string& controllerId, int groupNumber) {
    return controllerId + "#" + std::to_string(groupNumber);
}
std::string headProgramId(const NetworkSignalHead& head) {
    return head.controllerId.empty() ? head.programId : signalGroupProgramId(head.controllerId, head.groupNumber);
}
// Where a Signal head can stand: a Link lane or a Connector path, the width being the lane's so
// the stop line spans exactly the carriageway it holds. One routine serves the canvas pick, the
// drawn bar and the Ctrl-drag copy, so a head placed by any of them lands on the same station.
std::optional<HeadSlot> headSlot(const Network& n, const NetworkSignalHead& head) {
    if (head.connectorId.empty()) {
        for (const auto& l : n.links) if (l.id == head.lane.linkId)
            for (const auto& lane : l.lanes) if (lane.id == head.lane.laneId)
                return HeadSlot{head.lane, {}, laneGeometry(l, lane.id, n.drivingSide), lane.width, l.level};
        return {};
    }
    for (const auto& c : n.connectors) for (const auto& path : connectorPaths(n, c)) if (path.id == head.connectorId) {
        double width = 3.5;
        for (const auto& l : n.links) for (const auto& lane : l.lanes) if (lane.id == path.from.laneId) width = lane.width;
        return HeadSlot{{}, path.id, path.geometry, width, c.level};
    }
    return {};
}

std::optional<HeadPlacement> nearestHeadSlot(const Network& n, Point target, int level) {
    std::optional<HeadPlacement> best;
    double bestDistance = 1e300;
    const auto consider = [&](HeadSlot slot) {
        if (slot.geometry.size() < 2) return;
        const double station = stationOfClosestPoint(slot.geometry, target);
        const auto at = pointAlong(slot.geometry, station);
        const double distance = std::hypot(at.x - target.x, at.y - target.y);
        if (distance <= slot.width / 2 + .01 && distance < bestDistance) {
            bestDistance = distance;
            best = HeadPlacement{std::move(slot), station};
        }
    };
    for (const auto& l : n.links) if (l.level == level) for (const auto& lane : l.lanes)
        consider({{l.id, lane.id}, {}, laneGeometry(l, lane.id, n.drivingSide), lane.width, l.level});
    for (const auto& c : n.connectors) if (c.level == level) for (const auto& path : connectorPaths(n, c)) {
        double width = 3.5;
        for (const auto& l : n.links) for (const auto& lane : l.lanes) if (lane.id == path.from.laneId) width = lane.width;
        consider({{}, path.id, path.geometry, width, c.level});
    }
    return best;
}
}
