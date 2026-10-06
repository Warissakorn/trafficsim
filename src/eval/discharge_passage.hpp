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
std::map<std::uint64_t,DischargeMotion> dischargeMotions(
    const SimState&,const DischargePositions&,const std::map<std::uint64_t,std::size_t>& pending);
}
