#pragma once
#include "types.hpp"

namespace trafficsim {
struct Leader { double gap{}, speed{}; };
struct FollowingResult { double acceleration{}; FollowingMode mode{}; };
struct Motion { double speed{}, distance{}; };
FollowingResult followingAcceleration(double speed, double desiredSpeed, double driverFactor,
    const VehicleType& type, const DriverBehaviour& behaviour, std::optional<Leader> leader = {});
Motion integrate(double speed, double acceleration, double dt);
}
