#pragma once
#include "vehicle_pose.hpp"

namespace trafficsim {
struct AxleVehiclePose {
    Point front, rearAxle, frontAxle, rearBumper, heading;
};
// Prescribed FRONT BUMPER track; rear axle is the rigid body's reference point.
// Rear velocity is parallel to the body (no lateral slip). This is display-only:
// the traffic engine still measures front-bumper route distance and scalar length.
// Initial heading defaults to the first tangent. A continuing guide can supply
// its previous rolling heading instead; axles remain rigid across that boundary.
// A fixed spatial RK4 solution, independent of ticks/paint order, is interpolated
// on read. Split at every section/Link vertex; missing/disconnected paths draw none.
class RearAxlePath {
public:
    RearAxlePath(const std::vector<RoutePart>& parts, const VehicleType& type,
        const std::map<std::string,std::vector<Point>>& geometry,
        const std::map<std::string,ConnectorEquation>& equations, double maxStep=.1,
        std::optional<Point> initialHeading={});
    std::optional<AxleVehiclePose> pose(double distance, Point front) const;
private:
    struct Sample { double station, angle; };
    VehicleAxles axles_;
    std::vector<Sample> samples_;
};
}
