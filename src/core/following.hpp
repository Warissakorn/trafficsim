#pragma once
#include "types.hpp"

namespace trafficsim {
// `acceleration` is read by the w74 model only (docs/reference/W74.md §2); the prototype ignores
// it. Until the composition slice (BA28) fills it at every construction site it stays 0 (D130).
struct Leader { double gap{}, speed{}, acceleration{}; };
// `w74` is the state a w74 evaluation would store (W74.md §7); empty for the prototype.
struct FollowingResult { double acceleration{}; FollowingMode mode{}; std::optional<W74State> w74; };
struct Motion { double speed{}, distance{}; };
FollowingResult followingAcceleration(double speed, double desiredSpeed, double driverFactor,
    const VehicleType& type, const DriverBehaviour& behaviour, std::optional<Leader> leader = {});
Motion integrate(double speed, double acceleration, double dt);
// W74.md §6 (D133): what every consumer reads instead of a model's own fields.
double standstillGap(const DriverBehaviour&);
double desiredGap(const DriverBehaviour&, double driverFactor, double speed);
// The one dispatcher for every car-following evaluation, own motion and hypothetical alike: the
// prototype above, or w74Acceleration with the driver's traits and the previous tick's state.
// A hypothetical caller passes the vehicle's carried state and discards the result's.
FollowingResult follow(double speed, const PendingVehicle& driver, std::optional<W74State> previous,
    const VehicleType& type, const DriverBehaviour& behaviour, std::optional<Leader> leader = {});
}
