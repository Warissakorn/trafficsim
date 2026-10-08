#pragma once
#include "following.hpp"
#include <optional>
#include <string>
#include <vector>

namespace trafficsim {
// Wiedemann-74-structured car-following (M3.3.3a, D129). The contract, including every equation,
// equality side and the sign hysteresis, is docs/reference/W74.md. Not wired into the tick yet
// (D135): a behaviour can be `w74` in a file (schema 25), but Run refuses one in use (D136).

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

// Contract §5: zBx is the driver's existing driverFactor; the other four are hashed from
// (seed, vehicle id) with integer arithmetic only, so every compiler gives the same bits and the
// run's random stream is never touched.
W74Traits w74Traits(std::uint32_t seed, std::uint64_t vehicleId, double driverFactor);

// The 18 W74 keys, in contract §5 order: the one list the codec, the validator and the serializer
// read, so a key name exists once (hard rule 3).
struct W74Key { const char* name; double W74Parameters::* member; };
const std::vector<W74Key>& w74ParameterKeys();
// §5 ranges and finiteness: INVALID_BEHAVIOUR_PARAMETER at `<path>.<key>`.
std::vector<ValidationIssue> w74ParameterIssues(const W74Parameters& parameters, const std::string& path);
}
