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
std::vector<RouteZone> zoneIncidence(const Scenario& scenario, const std::vector<RoutePart>& parts) {
    std::vector<RouteZone> result;
    for (std::size_t z = 0; z < scenario.conflictZones.size(); ++z)
        for (const auto role : {ZoneRole::major, ZoneRole::minor}) {
            const auto& zone = scenario.conflictZones[z];
            const auto& chain = sideOf(zone, role).segmentIds;
            std::size_t p = 0, k = 0;
            for (; p < parts.size(); ++p) {
                const auto at = std::find(chain.begin(), chain.end(), parts[p].segmentId);
                if (at != chain.end()) { k = static_cast<std::size_t>(at - chain.begin()); break; }
            }
            if (p == parts.size()) continue;
            const auto& side = sideOf(zone, role);
            RouteZone rz{z, role, k == 0 ? parts[p].start + side.entry : parts[p].start, 0, 0, 0, k > 0};
            // Follow the chain for as long as the route does; turning off frees the area there.
            std::size_t q = p, c = k;
            while (q + 1 < parts.size() && c + 1 < chain.size() && parts[q + 1].segmentId == chain[c + 1]) { ++q; ++c; }
            rz.exitAt = c + 1 == chain.size() ? parts[q].start + side.exit : parts[q].start + parts[q].length;
            rz.waitAt = parts[p].start + zone.waitPosition;
            rz.clearAt = rz.exitAt;
            result.push_back(rz);
        }
    // Chain the minor zones with nowhere to wait between them, in the order the route meets them.
    double room = 0;
    for (const auto& type : scenario.vehicleTypes) {
        const auto behaviour = std::find_if(scenario.behaviours.begin(), scenario.behaviours.end(),
                                            [&](const auto& b) { return b.id == type.behaviourId; });
        room = std::max(room, type.length + (behaviour == scenario.behaviours.end() ? 0 : behaviour->standstillDistance));
    }
    std::vector<RouteZone*> minors;
    for (auto& rz : result) if (rz.role == ZoneRole::minor) minors.push_back(&rz);
    std::stable_sort(minors.begin(), minors.end(), [](const auto* a, const auto* b) { return a->waitAt < b->waitAt; });
    for (std::size_t i = 0; i < minors.size();) {
        std::size_t j = i + 1;
        double exit = minors[i]->exitAt;
        while (j < minors.size() && minors[j]->waitAt - exit < room) { exit = std::max(exit, minors[j]->exitAt); ++j; }
        for (std::size_t m = i; m < j; ++m) { minors[m]->waitAt = minors[i]->waitAt; minors[m]->clearAt = exit; }
        i = j;
    }
    return result;
}
std::vector<ZoneState> summarizeZones(const Scenario& scenario, const ScenarioIndex& index,
                                      const std::vector<Vehicle>& vehicles, const std::vector<VehicleRefs>& refs) {
    std::vector<ZoneState> zones(scenario.conflictZones.size());
    if (zones.empty()) return zones;
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        const auto& vehicle = vehicles[v];
        const double rear = vehicle.distance - scenario.vehicleTypes[refs[v].type].length;
        for (const auto& rz : index.routeZones[refs[v].route]) {
            const auto& zone = scenario.conflictZones[rz.zoneIndex];
            auto& state = zones[rz.zoneIndex];
            if (rz.role == ZoneRole::minor) {
                if (vehicle.distance - rz.waitAt > kPast && rear < rz.exitAt) state.holders.push_back(vehicle.id);
                continue;
            }
            const double d = rz.entryAt - vehicle.distance; // positive: still short of the area
            if (d < 0) { if (rear < rz.exitAt) state.majorBlocks = true; }
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
        const auto& state = zones[rz.zoneIndex];
        double gap;
        if (rz.role == ZoneRole::minor) {
            gap = rz.waitAt - vehicle.distance;
            if (gap < -kPast) continue; // past the line: it holds the crossing, never revoked
            // Receiving space: a standing leader must leave room for the whole vehicle past the
            // exit -- the chain's last exit -- or it would be admitted only to stop inside.
            const bool exitTaken = leader && leader->speed < kStoppedSpeed &&
                vehicle.distance + leader->gap - rz.clearAt < type.length + behaviour.standstillDistance;
            if (!state.majorBlocks && !exitTaken) continue;
        } else {
            gap = rz.entryAt - vehicle.distance;
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
        const double from = vehicles[v].distance, to = from + moves[v];
        for (const auto& rz : index.routeZones[refs[v].route]) {
            if (rz.role == ZoneRole::minor) {
                // Crossing the line this tick: a new request, not yet a grant. A chain shares
                // one line, so the request is made to every zone of it at once.
                if (from - rz.waitAt <= kPast && to - rz.waitAt > kPast)
                    commits.push_back({rz.zoneIndex, v, std::max(0.0, rz.waitAt - from)});
            } else if (to > rz.entryAt && from - length < rz.exitAt) {
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
