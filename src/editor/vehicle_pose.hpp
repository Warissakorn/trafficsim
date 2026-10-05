#pragma once
#include "../model/network/network.hpp"
#include <map>

namespace trafficsim {
struct VehiclePose { Point front, heading; };
// Display only: distance is the front-bumper station on the WHOLE route. The rear
// sample at distance-length supplies a chord heading, not an axle or a rigid-body
// rear position. Before route entry it extends the first tangent backwards.
// Missing geometry/empty routes or a directionless pose return no drawable pose.
// Invalid distance/length throws INVALID_VEHICLE_POSE. No simulation state is stored.
std::optional<VehiclePose> vehiclePose(const std::vector<RoutePart>& parts,
    double distance, double length,
    const std::map<std::string,std::vector<Point>>& geometry,
    const std::map<std::string,ConnectorEquation>& equations);
}
