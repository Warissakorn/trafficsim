#pragma once
// M3.2.3a: the crossing admission runtime (docs/M3_CONTRACT.md §4). Everything here reads the
// immutable pre-step snapshot, so no vehicle's decision depends on the order vehicles are visited.
#include "following.hpp"

namespace trafficsim {
// Every zone this route meets, in route distances (M3.2.3b). Shared by the run index and by
// validation, so the two cannot disagree about where a route meets a chain. Minor zones with
// less than one vehicle (the longest type plus its standstill distance) between one's exit and
// the next one's line are chained: they share the first line and the last exit, so a vehicle is
// admitted to all of them at once or to none (A15).
std::vector<RouteZone> zoneIncidence(const Scenario&, const std::vector<RoutePart>& parts);
// One zone as the snapshot sees it at the start of a tick.
struct ZoneState {
    bool majorBlocks{};                  // a major vehicle is inside, within headway, or within gapTime
    std::vector<std::uint64_t> holders;  // minor vehicles past the waiting line, rear not yet clear
};
std::vector<ZoneState> summarizeZones(const Scenario&, const ScenarioIndex&, const std::vector<Vehicle>&,
                                      const std::vector<VehicleRefs>&);
// The nearest line a zone holds this vehicle at, as a distance ahead of its front; none when no
// zone holds it. A minor vehicle waits at its waiting line while the major side blocks or its
// receiving space past the exit is taken by a stopped leader; a major vehicle waits at the entry
// while another vehicle holds the crossing.
std::optional<double> zoneHold(const Scenario&, const ScenarioIndex&, const std::vector<ZoneState>&,
                               const Vehicle&, const VehicleRefs&, const std::optional<Leader>& leader);
// The swept check, after every candidate move is known and before any is published: a minor
// vehicle that would cross its waiting line in the same tick a major vehicle's front reaches the
// area is capped back at the line. `moves` is each vehicle's candidate distance this tick; the
// result is the vehicles to cap and the distance each may still travel.
struct ZoneCap { std::size_t vehicle{}; double distance{}; };
std::vector<ZoneCap> sweptConflicts(const Scenario&, const ScenarioIndex&, const std::vector<Vehicle>&,
                                    const std::vector<VehicleRefs>&, const std::vector<double>& moves);
}
