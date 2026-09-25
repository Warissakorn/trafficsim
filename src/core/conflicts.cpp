#include "conflicts.hpp"
#include <algorithm>
#include <cmath>

namespace trafficsim {
namespace {
// Named numerical constants, not calibration (contract §1).
constexpr double kPast = 1e-9;          // m: a front this far beyond a line has crossed it
constexpr double kStoppedSpeed = 0.1;   // m/s: a leader slower than this is taken as standing
const ZoneSide& sideOf(const ConflictZone& z, ZoneRole role) { return role == ZoneRole::major ? z.major : z.minor; }
}
std::vector<ZoneState> summarizeZones(const Scenario& scenario, const ScenarioIndex& index,
                                      const std::vector<Vehicle>& vehicles, const std::vector<VehicleRefs>& refs) {
    std::vector<ZoneState> zones(scenario.conflictZones.size());
    if (zones.empty()) return zones;
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        const auto& vehicle = vehicles[v];
        const double length = scenario.vehicleTypes[refs[v].type].length;
        for (const auto& rz : index.routeZones[refs[v].route]) {
            const auto& zone = scenario.conflictZones[rz.zoneIndex];
            auto& state = zones[rz.zoneIndex];
            const double front = vehicle.distance - rz.partStart; // on the side's segment
            const auto& side = sideOf(zone, rz.role);
            if (rz.role == ZoneRole::minor) {
                if (front - zone.waitPosition > kPast && front - length < side.exit) state.holders.push_back(vehicle.id);
                continue;
            }
            const double d = side.entry - front; // positive: still short of the area
            if (d < 0) { if (front - length < side.exit) state.majorBlocks = true; }
            else if (d <= zone.headway) state.majorBlocks = true;
            // Equality satisfies the time threshold (contract §4); a standing vehicle beyond the
            // headway is a gap, not a block -- it cannot enter while a minor vehicle holds anyway.
            else if (vehicle.speed > 0 && d / vehicle.speed < zone.gapTime) state.majorBlocks = true;
        }
    }
    return zones;
}
std::optional<double> zoneHold(const Scenario& scenario, const ScenarioIndex& index,
                               const std::vector<ZoneState>& zones, const Vehicle& vehicle,
                               const VehicleRefs& refs, const std::optional<Leader>& leader) {
    std::optional<double> hold;
    const auto& type = scenario.vehicleTypes[refs.type];
    const auto& behaviour = scenario.behaviours[refs.behaviour];
    for (const auto& rz : index.routeZones[refs.route]) {
        const auto& zone = scenario.conflictZones[rz.zoneIndex];
        const auto& state = zones[rz.zoneIndex];
        double gap;
        if (rz.role == ZoneRole::minor) {
            gap = rz.partStart + zone.waitPosition - vehicle.distance;
            if (gap < -kPast) continue; // past the line: it holds the crossing, never revoked
            // Receiving space: a standing leader must leave room for the whole vehicle past the
            // exit, or the vehicle would be admitted only to stop inside the area.
            const bool exitTaken = leader && leader->speed < kStoppedSpeed &&
                vehicle.distance + leader->gap - (rz.partStart + zone.minor.exit) <
                    type.length + behaviour.standstillDistance;
            if (!state.majorBlocks && !exitTaken) continue;
        } else {
            gap = rz.partStart + zone.major.entry - vehicle.distance;
            if (gap < -kPast) continue;
            if (std::none_of(state.holders.begin(), state.holders.end(),
                             [&](auto id) { return id != vehicle.id; })) continue;
        }
        gap = std::max(0.0, gap);
        if (!hold || gap < *hold) hold = gap;
    }
    return hold;
}
std::vector<ZoneCap> sweptConflicts(const Scenario& scenario, const ScenarioIndex& index,
                                    const std::vector<Vehicle>& vehicles, const std::vector<VehicleRefs>& refs,
                                    const std::vector<double>& moves) {
    std::vector<ZoneCap> caps;
    if (scenario.conflictZones.empty()) return caps;
    std::vector<bool> majorSweeps(scenario.conflictZones.size(), false);
    struct Commit { std::size_t zone, vehicle; double distance; };
    std::vector<Commit> commits;
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        const double length = scenario.vehicleTypes[refs[v].type].length;
        for (const auto& rz : index.routeZones[refs[v].route]) {
            const auto& zone = scenario.conflictZones[rz.zoneIndex];
            const double from = vehicles[v].distance - rz.partStart, to = from + moves[v];
            if (rz.role == ZoneRole::minor) {
                // Crossing the line this tick: a new request, not yet a grant.
                if (from - zone.waitPosition <= kPast && to - zone.waitPosition > kPast)
                    commits.push_back({rz.zoneIndex, v, std::max(0.0, zone.waitPosition - from)});
            } else if (to > zone.major.entry && from - length < zone.major.exit) {
                // Its front reaches the area at some instant of the tick while its rear is not
                // yet clear: the whole swept interval, not just where the tick ends.
                majorSweeps[rz.zoneIndex] = true;
            }
        }
    }
    for (const auto& c : commits)
        if (majorSweeps[c.zone]) caps.push_back({c.vehicle, c.distance});
    return caps;
}
}
