#include "lanes.hpp"
#include "conflicts.hpp"
#include "following.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

namespace trafficsim {
namespace {
constexpr auto kNone = std::numeric_limits<std::uint32_t>::max();
double routeLength(const Scenario& s, const Route& route, bool& known) {
    double length = 0;
    for (const auto& id : route.segmentIds) {
        const auto seg = std::find_if(s.segments.begin(), s.segments.end(), [&](const auto& x) { return x.id == id; });
        if (seg == s.segments.end()) { known = false; return 0; }
        length += seg->length;
    }
    return length;
}
std::size_t slotOf(const Scenario& s, const std::string& id) {
    const auto it = std::find_if(s.routes.begin(), s.routes.end(), [&](const auto& r) { return r.id == id; });
    return static_cast<std::size_t>(it - s.routes.begin());
}
// Distance along the target for a distance along the source, by the span's linear map.
double mapped(const RouteLaneChange& c, double at) {
    const double from = c.fromEnd - c.fromStart;
    return from > 0 ? c.toStart + (at - c.fromStart) * (c.toEnd - c.toStart) / from : c.toStart;
}
// Between a zone's waiting line (or entry, for a major route) and its exit, anywhere along
// [rear, front]: no lane change there, on either route (contract §2, rule 2).
bool inConflictArea(const ScenarioIndex& index, std::size_t route, double rear, double front) {
    for (const auto& rz : index.routeZones[route]) {
        const double lo = rz.role == ZoneRole::minor ? rz.waitAt : rz.entryAt;
        const double hi = rz.role == ZoneRole::minor ? rz.clearAt : rz.exitAt;
        if (front >= lo - 1e-9 && rear <= hi + 1e-9) return true;
    }
    return false;
}
// Who is around [atRear, at] on a target route: the nearest vehicle ahead, the nearest behind,
// and whether anything lies alongside. Over the snapshot's spans and any extra ones (the changes
// already accepted this tick).
struct Around { std::optional<Leader> leader, follower; std::uint64_t followerId{}; bool alongside{}; };
Around around(const ScenarioIndex& index, std::size_t target, double at, double atRear, std::uint64_t self,
              const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets, const std::vector<OccupiedSpan>& extra) {
    Around a;
    const auto consider = [&](const RoutePart& part, const OccupiedSpan& span) {
        if (span.vehicleId == self) return;
        const double otherFront = part.start + span.front, otherRear = part.start + span.rear;
        if (otherRear >= at) {
            if (!a.leader || otherRear - at < a.leader->gap) a.leader = Leader{otherRear - at, span.speed};
        } else if (otherFront <= atRear) {
            if (!a.follower || atRear - otherFront < a.follower->gap) {
                a.follower = Leader{atRear - otherFront, span.speed}; a.followerId = span.vehicleId;
            }
        } else a.alongside = true;
    };
    for (const auto& part : index.parts[target]) {
        for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i)
            consider(part, spans[buckets.items[i]]);
        for (const auto& span : extra) if (span.segmentIndex == part.segmentIndex) consider(part, span);
    }
    return a;
}
std::size_t slotOfId(const std::vector<Vehicle>& vehicles, std::uint64_t id) {
    const auto it = std::lower_bound(vehicles.begin(), vehicles.end(), id,
                                     [](const Vehicle& v, std::uint64_t at) { return v.id < at; });
    return it != vehicles.end() && it->id == id ? static_cast<std::size_t>(it - vehicles.begin()) : vehicles.size();
}
// The first target a vehicle on `route` with this front and rear may change to, in the index's
// order (fewest changes left, then the lower slot); null when it is inside no span.
const RouteLaneChange* targetOf(const ScenarioIndex& index, std::size_t route, double front, double rear) {
    for (const auto& change : index.laneChangesOfRoute[route]) {
        if (index.remainingOfRoute[change.target] >= index.remainingOfRoute[route]) break;
        if (rear >= change.fromStart - 1e-9 && front <= change.fromEnd + 1e-9) return &change;
    }
    return nullptr;
}
// Walking pace: a stub vehicle this slow, at its dead end, is waiting for a gap (cooperation).
constexpr double kWaitingSpeed = 0.5;
}
bool waitingAtDeadEnd(const ScenarioIndex& index, std::size_t route, const Vehicle& vehicle,
                      const DriverBehaviour& behaviour) {
    return index.remainingOfRoute[route] != 0 && vehicle.speed < kWaitingSpeed &&
           index.deadEndOfRoute[route] - vehicle.distance <= stopLineReach(behaviour, vehicle.driverFactor);
}
std::vector<std::uint32_t> laneChangesRemaining(const Scenario& s) {
    std::vector<std::uint32_t> remaining(s.routes.size(), 0);
    for (const auto& dead : s.routeDeadEnds) {
        const auto r = slotOf(s, dead.routeId);
        if (r < remaining.size()) remaining[r] = kNone;
    }
    // Bellman-Ford over the spans: a stub needs one change more than its best target. At most one
    // pass per route settles every chain of changes; iteration order is the scenario's.
    for (std::size_t pass = 0; pass < s.routes.size(); ++pass) {
        bool changed = false;
        for (const auto& span : s.laneChanges) {
            const auto from = slotOf(s, span.fromRouteId), to = slotOf(s, span.toRouteId);
            if (from >= remaining.size() || to >= remaining.size() || remaining[to] == kNone) continue;
            if (remaining[from] != 0 && remaining[to] + 1 < remaining[from]) { remaining[from] = remaining[to] + 1; changed = true; }
        }
        if (!changed) break;
    }
    return remaining;
}
std::vector<ValidationIssue> laneChangeIssues(const Scenario& s) {
    std::vector<ValidationIssue> issues;
    if (s.laneChanges.empty() && s.routeDeadEnds.empty()) return issues;
    std::map<std::string, double> lengths, deadEnds;
    for (const auto& route : s.routes) {
        bool known = true;
        const double length = routeLength(s, route, known);
        if (known) lengths[route.id] = length;
    }
    const auto finite = [](double v) { return std::isfinite(v) && v >= 0; };
    for (std::size_t i = 0; i < s.routeDeadEnds.size(); ++i) {
        const auto& dead = s.routeDeadEnds[i];
        const auto p = "routeDeadEnds[" + std::to_string(i) + "]";
        const auto length = lengths.find(dead.routeId);
        if (length == lengths.end()) { issues.push_back({"UNKNOWN_ROUTE", p + ".routeId"}); continue; }
        if (!deadEnds.try_emplace(dead.routeId, dead.at).second) issues.push_back({"DUPLICATE_ID", p + ".routeId"});
        if (!finite(dead.at) || dead.at > length->second + 1e-9) issues.push_back({"INVALID_POSITION", p + ".at"});
    }
    for (std::size_t i = 0; i < s.laneChanges.size(); ++i) {
        const auto& span = s.laneChanges[i];
        const auto p = "laneChanges[" + std::to_string(i) + "]";
        const auto from = lengths.find(span.fromRouteId), to = lengths.find(span.toRouteId);
        if (from == lengths.end()) issues.push_back({"UNKNOWN_ROUTE", p + ".fromRouteId"});
        if (to == lengths.end()) issues.push_back({"UNKNOWN_ROUTE", p + ".toRouteId"});
        if (from == lengths.end() || to == lengths.end()) continue;
        // Only a stub changes lanes (§2): a span from a full route would be a discretionary change.
        const auto dead = deadEnds.find(span.fromRouteId);
        if (span.fromRouteId == span.toRouteId || dead == deadEnds.end())
            issues.push_back({"INVALID_RANGE", p + ".fromRouteId"});
        if (!finite(span.fromStart) || !finite(span.fromEnd) || span.fromStart > span.fromEnd ||
            span.fromEnd > from->second + 1e-9 || (dead != deadEnds.end() && span.fromEnd > dead->second + 1e-9))
            issues.push_back({"INVALID_RANGE", p + ".from"});
        if (!finite(span.toStart) || !finite(span.toEnd) || span.toStart > span.toEnd || span.toEnd > to->second + 1e-9)
            issues.push_back({"INVALID_RANGE", p + ".to"});
    }
    if (!issues.empty()) return issues;
    // A stub no chain of changes leads off would hold its vehicles at the dead end for ever.
    const auto remaining = laneChangesRemaining(s);
    for (std::size_t i = 0; i < s.routeDeadEnds.size(); ++i)
        if (remaining[slotOf(s, s.routeDeadEnds[i].routeId)] == kNone)
            issues.push_back({"LANE_CHANGE_DEAD_END_UNREACHABLE", "routeDeadEnds[" + std::to_string(i) + "]"});
    return issues;
}
void indexLaneChanges(const Scenario& s, ScenarioIndex& index) {
    index.laneChanges = !s.laneChanges.empty();
    index.laneChangesOfRoute.assign(s.routes.size(), {});
    index.deadEndOfRoute.assign(s.routes.size(), std::numeric_limits<double>::infinity());
    index.remainingOfRoute = laneChangesRemaining(s);
    for (const auto& dead : s.routeDeadEnds)
        if (const auto r = slotOf(s, dead.routeId); r < s.routes.size()) index.deadEndOfRoute[r] = dead.at;
    for (const auto& span : s.laneChanges) {
        const auto from = slotOf(s, span.fromRouteId), to = slotOf(s, span.toRouteId);
        if (from >= s.routes.size() || to >= s.routes.size()) continue;
        index.laneChangesOfRoute[from].push_back({to, span.fromStart, span.fromEnd, span.toStart, span.toEnd});
    }
    // The order a vehicle tries its targets in: fewest changes left, then the lower slot (§2).
    for (auto& spans : index.laneChangesOfRoute)
        std::stable_sort(spans.begin(), spans.end(), [&](const auto& a, const auto& b) {
            const auto ra = index.remainingOfRoute[a.target], rb = index.remainingOfRoute[b.target];
            return ra != rb ? ra < rb : a.target < b.target;
        });
}
std::vector<LaneChange> decideLaneChanges(const Scenario& s, const ScenarioIndex& index,
                                          const std::vector<Vehicle>& vehicles, const std::vector<VehicleRefs>& refs,
                                          const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                                          const std::vector<StopService>& stopService) {
    std::vector<LaneChange> accepted;
    std::vector<OccupiedSpan> moved; // accepted changers at their new places, this tick
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        const auto route = refs[v].route;
        if (index.remainingOfRoute[route] == 0) continue;
        const auto& vehicle = vehicles[v];
        const auto& type = s.vehicleTypes[refs[v].type];
        const auto& behaviour = s.behaviours[refs[v].behaviour];
        const double front = vehicle.distance, rear = front - type.length;
        if (std::any_of(stopService.begin(), stopService.end(), [&](const auto& x) { return x.vehicleId == vehicle.id; })) continue;
        if (inConflictArea(index, route, rear, front)) continue;
        for (const auto& change : index.laneChangesOfRoute[route]) {
            if (index.remainingOfRoute[change.target] >= index.remainingOfRoute[route]) break;
            if (rear < change.fromStart - 1e-9 || front > change.fromEnd + 1e-9) continue;
            const double at = mapped(change, front), atRear = at - type.length;
            if (inConflictArea(index, change.target, atRear, at)) continue;
            // Nearest vehicle ahead of and behind the mapped place, over this tick's snapshot and
            // the changes already accepted; anything alongside refuses the change (rule 5).
            const auto near = around(index, change.target, at, atRear, vehicle.id, spans, buckets, moved);
            const auto& leader = near.leader;
            const auto& follower = near.follower;
            if (near.alongside) continue;
            // Forward safety (rule 3), by the car-following model itself: braking it would accept,
            // and this tick's move inside the room the leader leaves, so the change never clamps.
            if (leader) {
                const double a = followingAcceleration(vehicle.speed, vehicle.desiredSpeed, vehicle.driverFactor,
                                                       type, behaviour, leader).acceleration;
                if (a < -type.comfortableDeceleration ||
                    integrate(vehicle.speed, a, s.timeStep).distance > leader->gap - behaviour.standstillDistance) continue;
            }
            // Rearward safety (rule 4): the follower's own reaction to the changer ahead of it.
            if (follower) {
                const auto f = slotOfId(vehicles, near.followerId);
                if (f == vehicles.size()) continue; // never: every span is a vehicle of this snapshot
                const auto& fType = s.vehicleTypes[refs[f].type];
                const auto& fBehaviour = s.behaviours[refs[f].behaviour];
                const auto& other = vehicles[f];
                const double a = followingAcceleration(other.speed, other.desiredSpeed, other.driverFactor, fType,
                                                       fBehaviour, Leader{follower->gap, vehicle.speed}).acceleration;
                if (a < -fType.comfortableDeceleration ||
                    integrate(other.speed, a, s.timeStep).distance > follower->gap - fBehaviour.standstillDistance) continue;
            }
            accepted.push_back({v, static_cast<std::uint32_t>(change.target), at});
            auto placed = vehicle;
            placed.routeIndex = static_cast<std::uint32_t>(change.target); placed.distance = at;
            appendVehicleSpans(moved, s, index, placed, {change.target, refs[v].type, refs[v].behaviour});
            break;
        }
    }
    return accepted;
}
std::vector<CourtesyHold> courtesyHolds(const Scenario& s, const ScenarioIndex& index, const std::vector<Vehicle>& vehicles,
                                        const std::vector<VehicleRefs>& refs, const std::vector<OccupiedSpan>& spans,
                                        const SpanBuckets& buckets) {
    std::vector<CourtesyHold> holds(vehicles.size());
    // Without the parameter on any behaviour, only the D71 rule runs, exactly as before.
    const bool cooperative = std::any_of(s.behaviours.begin(), s.behaviours.end(),
                                         [](const auto& b) { return b.maxDecelerationCooperativeBraking.has_value(); });
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        const auto route = refs[v].route;
        if (index.remainingOfRoute[route] == 0) continue;
        const auto& vehicle = vehicles[v];
        const auto& type = s.vehicleTypes[refs[v].type];
        const auto& behaviour = s.behaviours[refs[v].behaviour];
        const bool waiting = waitingAtDeadEnd(index, route, vehicle, behaviour);
        if (!waiting) {
            // Cooperative braking: only inside the look-ahead, once its dead end, taken as a
            // standing obstacle, already governs its car-following (contract §2).
            if (!cooperative) continue;
            const double toDeadEnd = index.deadEndOfRoute[route] - vehicle.distance;
            if (followingAcceleration(vehicle.speed, vehicle.desiredSpeed, vehicle.driverFactor, type, behaviour,
                                      Leader{toDeadEnd, 0}).mode == FollowingMode::free) continue;
        }
        const auto* change = targetOf(index, route, vehicle.distance, vehicle.distance - type.length);
        if (!change) continue;
        const double at = mapped(*change, vehicle.distance), atRear = at - type.length;
        // Every vehicle behind the target place, nearest first. One alongside is not asked: it is
        // already past. A vehicle in two segments has two spans; its nearer one counts.
        std::vector<std::pair<double, std::uint64_t>> behind;
        for (const auto& part : index.parts[change->target])
            for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i) {
                const auto& span = spans[buckets.items[i]];
                if (span.vehicleId != vehicle.id && part.start + span.front <= atRear)
                    behind.emplace_back(atRear - (part.start + span.front), span.vehicleId);
            }
        std::sort(behind.begin(), behind.end());
        // The nearest that can hold back holds back; any nearer than it cannot, and passes first.
        for (const auto& [gap, id] : behind) {
            const auto f = slotOfId(vehicles, id);
            if (f == vehicles.size()) continue;
            const auto& other = vehicles[f];
            const auto& fType = s.vehicleTypes[refs[f].type];
            const auto& fBehaviour = s.behaviours[refs[f].behaviour];
            const double room = gap - fBehaviour.standstillDistance;
            if (room < 0) continue;
            CourtesyHold hold{gap, 0, false};
            if (waiting) {
                // Kinematic, not the model's commanded braking: once it holds back, its stopping
                // distance only shrinks, so the same vehicle keeps holding back tick after tick.
                if (other.speed * other.speed > 2 * fType.comfortableDeceleration * room ||
                    other.speed * s.timeStep > room) continue;
            } else {
                // Falls in behind the moving changer at no more than its cooperative deceleration.
                if (!fBehaviour.maxDecelerationCooperativeBraking) continue;
                const double closing = other.speed - vehicle.speed;
                if (closing > 0 && closing * closing > 2 * *fBehaviour.maxDecelerationCooperativeBraking * room) continue;
                hold = {gap, vehicle.speed, true};
            }
            // The nearest place counts; at equal gaps a waiting changer's standing one wins.
            if (hold.gap < holds[f].gap || (hold.gap == holds[f].gap && holds[f].moving)) holds[f] = hold;
            break;
        }
    }
    return holds;
}
}
