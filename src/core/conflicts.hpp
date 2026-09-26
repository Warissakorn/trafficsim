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
// The room a vehicle needs to wait in: the longest type plus its standstill distance. Less than
// this between two zones means a vehicle waiting at the second still occupies the first.
double waitingRoom(const Scenario&);
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
// A Stop zone (M3.2.5) also holds a minor vehicle at its line until `service` says it has served
// that line by `tick`, whatever the gap.
std::optional<double> zoneHold(const Scenario&, const ScenarioIndex&, const std::vector<ZoneState>&,
                               const Vehicle&, const VehicleRefs&, const std::optional<Leader>& leader,
                               const StopService* service = nullptr, std::uint64_t tick = 0);
// M3.2.5, contract §5: the stop-service table for the snapshot at the start of `tick`, from the
// previous one. Each vehicle keeps at most one entry, for the nearest Stop line it has not passed:
// carried over while it is the same line, created when the vehicle has come to the line -- slower
// than walking pace (kStoppedSpeed), front within stopLineReach of it. A queued vehicle behind a
// leader stands a vehicle length further back, so it has not reached the line. Sorted by id.
//
// The reduced car-following model never brings a vehicle to exactly zero behind a line: it crawls
// ever slower towards its standstill gap. So the Stop itself finishes the stop -- restsAtStop --
// and the vehicle stands at zero speed for the tick that entry was made in and one whole tick
// more before it has served the line (the one-tick minimum of §5; no dwell parameter is added).
// How far short of a line a vehicle at walking pace is still "at" it: the gap car-following keeps
// behind a standing obstacle at kStoppedSpeed, plus kLineReach.
double stopLineReach(const DriverBehaviour&, double driverFactor);
// Held at rest by its Stop this tick: service began but has not yet run a whole tick at zero.
inline bool restsAtStop(const StopService* s, std::uint64_t tick) { return s && s->since + 1 >= tick; }
std::vector<StopService> refreshStops(const Scenario&, const ScenarioIndex&, const std::vector<Vehicle>&,
                                      const std::vector<VehicleRefs>&, const std::vector<StopService>& previous,
                                      std::uint64_t tick);
// Phase 2, after every candidate move is known and before any is published. A minor vehicle
// crossing its waiting line this tick is a request, and it is capped back at the line when:
//  - a major vehicle's front reaches that zone's area within the same tick (the swept check); or
//  - it and other requests of this tick wait on the same standing leader, which leaves room for
//    fewer of them than asked -- lower vehicle ids are served first (receiving space, M3.2.3c).
// `moves` is each vehicle's candidate distance, `leaders` its snapshot vehicle leader.
struct ZoneCap { std::size_t vehicle{}; double distance{}; };
struct VehicleLeader { double gap{}, speed{}; std::uint64_t vehicleId{}; };
std::vector<ZoneCap> resolveRequests(const Scenario&, const ScenarioIndex&, const std::vector<Vehicle>&,
                                     const std::vector<VehicleRefs>&, const std::vector<double>& moves,
                                     const std::vector<std::optional<VehicleLeader>>& leaders);
}
