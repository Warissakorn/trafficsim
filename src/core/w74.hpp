#pragma once
#include "following.hpp"
#include <cstdint>
#include <optional>

namespace trafficsim {
// Wiedemann-74-structured car-following (M3.3.3a, D129). The contract, including every equation,
// equality side and the sign hysteresis, is docs/reference/W74.md. Not wired into the tick yet
// (D130): nothing outside tests calls these, and no behaviour can select the model.

// Every key is required data (hard rule 5); there are deliberately no defaults.
struct W74Parameters {
    double ax, bxAdd, bxMult, exAdd, exMult, cxAdd, cxMult, opdvAdd, opdvMult, dMax;
    double bMaxAdd, bMaxMult, bMaxSpeedRoot, bNullAdd, bNullMult, bMinAdd;
    double leaderAccelerationWeight, emergencyLeaderWeight;
    bool operator==(const W74Parameters&) const = default;
};
// Per-driver draws in [0, 1], fixed when the vehicle is generated (contract §5).
struct W74Traits {
    double zBx, zEx, zCx, zOp, zOsc;
    bool operator==(const W74Traits&) const = default;
};
enum class W74Regime : std::uint8_t { free, approaching, following, emergency };
// The previous tick's regime and the oscillation sign it used (0 outside following), §7.
struct W74State {
    W74Regime regime{};
    std::int8_t sign{};
    bool operator==(const W74State&) const = default;
};
// §3's values for one follower and obstacle, exposed so callers and tests read the same numbers
// the classification compares against.
struct W74Thresholds { double ax, bx, abx, ex, sdx, cx, sdv, cldv, opdv, dmax, bNull, bMax; };
W74Thresholds w74Thresholds(double speed, double gap, const VehicleType& type,
    const W74Parameters& parameters, const W74Traits& traits);

struct W74Result {
    double acceleration{};
    W74State state;     // what publish stores for this tick
    FollowingMode mode{};
};
// `previous` is empty when the vehicle enters w74 (§7). The result is bounded by the type's
// limits and D105 (§6); integration, caps and Stop rest remain the caller's.
W74Result w74Acceleration(double speed, double desiredSpeed, const VehicleType& type,
    const W74Parameters& parameters, const W74Traits& traits, std::optional<W74State> previous,
    std::optional<Leader> obstacle = {});
}
