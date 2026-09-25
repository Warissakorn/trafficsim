#include "movement.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace trafficsim {
namespace { constexpr auto npos = std::numeric_limits<std::size_t>::max(); }
double tripDelay(const ArrivedEvent& a) {
    return std::max(0.0, a.travelTime + a.departureDelay - a.freeFlowTime);
}
double queueLength(std::vector<QueuedVehicle> vehicles, double maxGap) {
    std::sort(vehicles.begin(), vehicles.end(), [](const auto& a, const auto& b) {
        return a.upstream < b.upstream;
    });
    // Walk upstream from the line; the queue ends at the first moving vehicle or open gap.
    double end = 0, length = 0;
    for (const auto& v : vehicles) {
        if (v.upstream < 0) continue; // front already past the line
        if (!v.queued || v.upstream - end > maxGap) break;
        end = v.upstream + v.length; length = end;
    }
    return length;
}
MovementAccumulator::MovementAccumulator(EvaluationSpec spec)
    : spec_(std::move(spec)), count_(spec_.movementNames.size()),
      delay_(spec_.movementNames.size()), travel_(spec_.movementNames.size()),
      queueSum_(spec_.counters.size()), queueMax_(spec_.counters.size()) {}
void MovementAccumulator::bind(const SimState& state) {
    // Slots are resolved once per scenario; a run never swaps its scenario.
    if (bound_ == state.scenario.get()) return;
    bound_ = state.scenario.get();
    const auto& s = *state.scenario;
    movementOfSlot_.assign(s.routes.size(), npos);
    for (std::size_t r = 0; r < s.routes.size(); ++r)
        if (const auto it = spec_.movementOfRoute.find(s.routes[r].id); it != spec_.movementOfRoute.end())
            movementOfSlot_[r] = it->second;
    // The first part of each route on the line's segment, exactly as the index finds a head's.
    lines_.clear();
    for (std::size_t c = 0; c < spec_.counters.size(); ++c)
        for (const auto& line : spec_.counters[c].lines) {
            BoundLine bound{c, std::vector<double>(s.routes.size(), std::nan(""))};
            for (std::size_t r = 0; r < s.routes.size(); ++r)
                for (const auto& part : state.index->parts[r])
                    if (part.segmentId == line.segmentId) { bound.atRoute[r] = part.start + line.position; break; }
            lines_.push_back(std::move(bound));
        }
}
void MovementAccumulator::observe(const SimState& state) {
    bind(state);
    const auto& s = *state.scenario;
    std::map<std::string, std::size_t> slotOfRoute;
    for (const auto& event : state.events) {
        summary_.add(event);
        const auto* arrived = std::get_if<ArrivedEvent>(&event);
        if (!arrived) continue;
        if (slotOfRoute.empty())
            for (std::size_t r = 0; r < s.routes.size(); ++r) slotOfRoute[s.routes[r].id] = r;
        const auto slot = slotOfRoute.find(arrived->routeId);
        const auto m = slot == slotOfRoute.end() ? npos : movementOfSlot_[slot->second];
        if (m == npos) { ++unassigned_; continue; }
        ++count_[m]; delay_[m] += tripDelay(*arrived); travel_[m] += arrived->travelTime;
    }
    // Queue state with hysteresis, keyed by vehicle id; vehicles that left are dropped.
    std::map<std::uint64_t, bool> queued;
    for (const auto& v : state.vehicles) {
        const auto before = queued_.find(v.id);
        const bool was = before != queued_.end() && before->second;
        queued[v.id] = was ? v.speed <= spec_.queue.endSpeed : v.speed < spec_.queue.beginSpeed;
    }
    queued_ = std::move(queued);
    // Per line, the vehicles whose route crosses it; an approach is the max over its lines.
    std::vector<double> length(spec_.counters.size());
    for (const auto& line : lines_) {
        std::vector<QueuedVehicle> behind;
        for (const auto& v : state.vehicles) {
            const double at = line.atRoute[v.routeIndex];
            if (std::isnan(at)) continue;
            behind.push_back({at - v.distance, s.vehicleTypes[v.typeIndex].length, queued_[v.id]});
        }
        length[line.counter] = std::max(length[line.counter], queueLength(std::move(behind), spec_.queue.maxGap));
    }
    for (std::size_t c = 0; c < length.size(); ++c) {
        queueSum_[c] += length[c]; queueMax_[c] = std::max(queueMax_[c], length[c]);
    }
    ++observed_;
}
MovementReport MovementAccumulator::report(const SimState& end) const {
    MovementReport r;
    for (std::size_t m = 0; m < spec_.movementNames.size(); ++m) {
        MovementRow row{spec_.movementNames[m], count_[m], {}, {}};
        if (count_[m]) {
            row.meanDelay = delay_[m] / static_cast<double>(count_[m]);
            row.meanTravelTime = travel_[m] / static_cast<double>(count_[m]);
        }
        r.movements.push_back(row);
    }
    for (std::size_t c = 0; c < spec_.counters.size(); ++c)
        r.queues.push_back({spec_.counters[c].name,
                            observed_ ? queueSum_[c] / static_cast<double>(observed_) : 0.0, queueMax_[c]});
    const auto summary = summary_.summary();
    r.completed = summary.completed; r.safetyClamps = summary.safetyClamps; r.meanDelay = summary.meanDelay;
    r.unassigned = unassigned_;
    r.pending = pendingCount(end); r.active = end.vehicles.size(); r.time = end.time;
    return r;
}
}
