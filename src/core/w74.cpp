#include "w74.hpp"
#include <algorithm>
#include <cmath>

namespace trafficsim {
W74Thresholds w74Thresholds(double speed, double gap, const VehicleType& type,
    const W74Parameters& p, const W74Traits& z) {
    W74Thresholds t{};
    t.ax = p.ax;
    t.bx = (p.bxAdd + p.bxMult * z.zBx) * std::sqrt(std::max(speed, 0.1));
    t.abx = t.ax + t.bx;
    t.ex = p.exAdd + p.exMult * z.zEx;
    t.sdx = t.ax + t.ex * t.bx;
    t.cx = p.cxAdd + p.cxMult * z.zCx;
    t.sdv = gap > t.ax ? ((gap - t.ax) / t.cx) * ((gap - t.ax) / t.cx) : 0;
    t.cldv = t.sdv * t.ex * t.ex;
    t.opdv = -t.cldv * (p.opdvAdd + p.opdvMult * z.zOp);
    t.dmax = std::max(p.dMax, speed * speed / (2 * type.comfortableDeceleration));
    t.bNull = p.bNullAdd + p.bNullMult * z.zOsc;
    t.bMax = p.bMaxAdd + p.bMaxMult * std::max(0.0, p.bMaxSpeedRoot - std::sqrt(std::max(0.0, speed)));
    return t;
}
namespace {
W74Regime classify(double gap, double dv, const W74Thresholds& t) {
    if (gap <= t.abx) return W74Regime::emergency;
    if (gap < t.sdx) {
        if (dv > t.cldv) return W74Regime::approaching;
        if (dv > t.opdv) return W74Regime::following;
        return W74Regime::free;
    }
    if (dv > t.sdv && gap < t.dmax) return W74Regime::approaching;
    return W74Regime::free;
}
// §7: hysteresis on the previous tick, overridden at standstill (a stopped driver can only close up).
std::int8_t followingSign(double speed, double dv, std::optional<W74State> previous) {
    if (speed == 0) return 1;
    if (!previous) return dv > 0 ? -1 : 1;
    switch (previous->regime) {
    case W74Regime::free: return 1;
    case W74Regime::following: return previous->sign;
    case W74Regime::approaching: case W74Regime::emergency: return -1;
    }
    return 1;
}
double freeAcceleration(double speed, double desiredSpeed, const W74Thresholds& t,
                        std::optional<double> gap) {
    if (speed >= desiredSpeed) return 0;
    if (gap && *gap <= 2 * t.abx) return std::min(t.bNull, t.bMax * (*gap - t.abx) / t.abx);
    return t.bMax;
}
FollowingMode modeOf(W74Regime regime) {
    switch (regime) {
    case W74Regime::approaching: return FollowingMode::approaching;
    case W74Regime::following: return FollowingMode::following;
    case W74Regime::emergency: return FollowingMode::braking;
    case W74Regime::free: break;
    }
    return FollowingMode::free;
}
}
W74Result w74Acceleration(double speed, double desiredSpeed, const VehicleType& type,
    const W74Parameters& p, const W74Traits& z, std::optional<W74State> previous,
    std::optional<Leader> obstacle) {
    const auto t = w74Thresholds(speed, obstacle ? obstacle->gap : 0, type, p, z);
    W74State state{W74Regime::free, 0};
    double a{};
    if (!obstacle) {
        a = freeAcceleration(speed, desiredSpeed, t, {});
    } else {
        const double g = obstacle->gap, dv = speed - obstacle->speed, aL = obstacle->acceleration;
        state.regime = classify(g, dv, t);
        switch (state.regime) {
        case W74Regime::free: a = freeAcceleration(speed, desiredSpeed, t, g); break;
        case W74Regime::approaching:
            a = std::max(-type.comfortableDeceleration,
                         0.5 * dv * dv / (t.abx - g) + p.leaderAccelerationWeight * aL);
            break;
        case W74Regime::following:
            state.sign = followingSign(speed, dv, previous);
            a = state.sign * t.bNull;
            break;
        case W74Regime::emergency:
            a = g > t.ax ? 0.5 * dv * dv / (t.ax - g) + p.emergencyLeaderWeight * aL +
                           p.bMinAdd * (t.abx - g) / t.bx
                         : -type.maxDeceleration;
            break;
        }
    }
    a = std::clamp(a, -type.maxDeceleration, type.maxAcceleration);
    // D105, as in the prototype: a stopped vehicle within the standstill gap waits for room.
    if (speed == 0 && obstacle && obstacle->gap <= t.ax) a = std::min(0.0, a);
    return {a, state, modeOf(state.regime)};
}
}
