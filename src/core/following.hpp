#pragma once
#include "types.hpp"

namespace trafficsim {
// `acceleration` is read by the w74 model only (docs/reference/W74.md §2); the prototype ignores
// it. Until the composition slice (BA28) fills it at every construction site it stays 0 (D130).
struct Leader { double gap{}, speed{}, acceleration{}; };
struct FollowingResult { double acceleration{}; FollowingMode mode{}; };
struct Motion { double speed{}, distance{}; };
FollowingResult followingAcceleration(double speed, double desiredSpeed, double driverFactor,
    const VehicleType& type, const DriverBehaviour& behaviour, std::optional<Leader> leader = {});
Motion integrate(double speed, double acceleration, double dt);
}
