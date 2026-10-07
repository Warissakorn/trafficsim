#pragma once
#include "../core/types.hpp"
#include <map>

namespace trafficsim {
// Observer-owned previous positions; never reconstruct motion from display traces.
struct DischargePosition {std::size_t route{},type{};double distance{};};
using DischargePositions=std::map<std::uint64_t,DischargePosition>;
struct DischargeMotion {
    std::optional<DischargePosition> start,end;
    std::vector<std::pair<DischargePosition,DischargePosition>> remaps;
    bool source{},ambiguous{};
};
// Replay start-of-tick remaps using unique engine span mappings. End-of-tick
// routing preserves distance. Missing/contradictory terminal/type evidence is
// ambiguous; callers must not infer passage from it. Inputs are immutable.
// `pending` holds the previous snapshot's queued vehicles plus upcomingArrivals (D125); a
// departure uses its type only when route, scheduled time and desired speed match exactly.
using DischargePending=std::map<std::uint64_t,PendingVehicle>;
std::map<std::uint64_t,DischargeMotion> dischargeMotions(
    const SimState&,const DischargePositions&,const DischargePending& pending);
}
