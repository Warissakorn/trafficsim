#include "following.hpp"
#include <algorithm>
#include <cmath>

namespace trafficsim {
// Reduced Wiedemann-inspired prototype, NOT W74/W99. See docs/SIMULATION.md.
FollowingResult followingAcceleration(double speed, double desiredSpeed, double driverFactor,
    const VehicleType& type, const DriverBehaviour& behaviour, std::optional<Leader> leader) {
    const double free = std::min(type.maxAcceleration, (desiredSpeed - speed) / behaviour.followingTime);
    double acceleration = free;
    auto mode = FollowingMode::free;
    if (leader) {
        const double desiredGap = behaviour.standstillDistance +
            (behaviour.additiveSafetyDistance + behaviour.multiplicativeSafetyDistance * driverFactor) *
            std::sqrt(std::max(0.0, speed));
        const double closing = speed - leader->speed;
        const double room = std::max(0.01, leader->gap - desiredGap);
        const auto following = [&] {
            return std::min(free, -closing / behaviour.followingTime + (leader->gap - desiredGap) /
                            (behaviour.followingTime * behaviour.followingTime));
        };
        if (leader->gap < desiredGap) {
            mode = FollowingMode::braking;
            acceleration = following();
        } else if (closing > behaviour.speedThreshold &&
                   room < closing * closing / (2 * type.comfortableDeceleration) +
                          speed * behaviour.followingTime) {
            mode = FollowingMode::approaching;
            acceleration = std::min(free, -(closing * closing) / (2 * room));
        } else if (leader->gap < desiredGap * 1.5) {
            mode = FollowingMode::following;
            acceleration = following();
        }
    }
    return {std::max(-type.maxDeceleration, acceleration), mode};
}
Motion integrate(double speed, double acceleration, double dt) {
    if (acceleration < 0 && speed + acceleration * dt < 0)
        return {0, -(speed * speed) / (2 * acceleration)};
    return {std::max(0.0, speed + acceleration * dt),
            std::max(0.0, speed * dt + 0.5 * acceleration * (dt * dt))};
}
}
