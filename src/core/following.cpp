#include "following.hpp"
#include "w74.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trafficsim {
// Reduced Wiedemann-inspired prototype, NOT W74/W99. See docs/reference/SIMULATION.md.
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
    // A moving merge leader can leave less than the standstill gap at the shared segment's
    // start. A stopped follower must wait for room, not accelerate into the zero hard cap.
    // Moving followers keep their existing braking/clamp behaviour (D105).
    if (speed == 0 && leader && leader->gap <= behaviour.standstillDistance)
        acceleration = std::min(0.0, acceleration);
    return {std::max(-type.maxDeceleration, acceleration), mode};
}
double standstillGap(const DriverBehaviour& b) { return b.w74 ? b.w74->ax : b.standstillDistance; }
double desiredGap(const DriverBehaviour& b, double driverFactor, double speed) {
    if (b.w74) return b.w74->ax + (b.w74->bxAdd + b.w74->bxMult * driverFactor) * std::sqrt(std::max(speed, 0.1));
    return b.standstillDistance + (b.additiveSafetyDistance + b.multiplicativeSafetyDistance * driverFactor) *
           std::sqrt(std::max(0.0, speed));
}
FollowingResult follow(double speed, const PendingVehicle& driver, std::optional<W74State> previous,
    const VehicleType& type, const DriverBehaviour& behaviour, std::optional<Leader> leader) {
    if (!behaviour.w74)
        return followingAcceleration(speed, driver.desiredSpeed, driver.driverFactor, type, behaviour, leader);
    // Every vehicle of a scenario holding w74 carries traits (D137); one without is a broken state.
    if (!driver.w74Traits) throw std::logic_error("w74 behaviour without driver traits");
    const auto r = w74Acceleration(speed, driver.desiredSpeed, type, *behaviour.w74, *driver.w74Traits, previous, leader);
    return {r.acceleration, r.mode, r.state};
}
Motion integrate(double speed, double acceleration, double dt) {
    if (acceleration < 0 && speed + acceleration * dt < 0)
        return {0, -(speed * speed) / (2 * acceleration)};
    return {std::max(0.0, speed + acceleration * dt),
            std::max(0.0, speed * dt + 0.5 * acceleration * (dt * dt))};
}
}
