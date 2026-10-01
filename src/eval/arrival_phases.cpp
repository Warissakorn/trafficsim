#include "arrival_phases.hpp"
#include <algorithm>
#include <cmath>

namespace trafficsim {
namespace {
double cycleOf(const Scenario& s) {
    double cycle = 0;
    for (const auto& program : s.signalPrograms) {
        double sum = 0;
        for (const auto& phase : program.phases) sum += phase.duration;
        if (cycle > 0 && std::abs(sum - cycle) > 1e-9) return 0;
        cycle = sum;
    }
    return cycle;
}
}
ArrivalPhaseAccumulator::ArrivalPhaseAccumulator(const EvaluationSpec& spec, double binWidth)
    : movementOfRoute_(spec.movementOfRoute), names_(spec.movementNames), beginSpeed_(spec.queue.beginSpeed), endSpeed_(spec.queue.endSpeed),
      binWidth_(binWidth) {}
std::size_t ArrivalPhaseAccumulator::bin(double time) const {
    const auto bins = static_cast<std::size_t>(std::ceil(cycle_ / binWidth_ - 1e-9));
    const auto b = static_cast<std::size_t>(std::fmod(time, cycle_) / binWidth_);
    return b < bins ? b : bins - 1;
}
void ArrivalPhaseAccumulator::observe(const SimState& state) {
    if (cycle_ < 0) cycle_ = cycleOf(*state.scenario);
    if (cycle_ <= 0) return;
    const auto bins = static_cast<std::size_t>(std::ceil(cycle_ / binWidth_ - 1e-9));
    for (const auto& event : state.events) {
        if (const auto* departed = std::get_if<DepartedEvent>(&event)) {
            for (const auto& route : state.scenario->routes)
                if (route.id == departed->routeId) { open_[departed->vehicleId].entered.emplace_back(route.segmentIds.front(), departed->time); break; }
        } else if (const auto* entered = std::get_if<SegmentEnteredEvent>(&event)) {
            open_[entered->vehicleId].entered.emplace_back(entered->segmentId, entered->time);
        } else if (const auto* arrived = std::get_if<ArrivedEvent>(&event)) {
            const auto trip = open_.find(arrived->vehicleId);
            const auto m = movementOfRoute_.find(arrived->routeId);
            if (m == movementOfRoute_.end()) ++unassigned_;
            else if (trip != open_.end()) {
                const auto row = [&](const std::string& segment) -> Row& {
                    auto& r = rows_[{m->second, segment}];
                    if (r.enteredAt.empty()) { r.enteredAt.assign(bins, 0); r.firstStopAt.assign(bins, 0); }
                    return r;
                };
                for (const auto& [segment, time] : trip->second.entered) { auto& r = row(segment); ++r.entered; ++r.enteredAt[bin(time)]; }
                for (const auto& [segment, time] : trip->second.stopped) { auto& r = row(segment); ++r.stopped; ++r.firstStopAt[bin(time)]; }
            }
            if (trip != open_.end()) open_.erase(trip);
        }
    }
    for (const auto& v : state.vehicles) {
        auto& trip = open_[v.id];
        if (v.speed >= endSpeed_) trip.moved = true;
        if (!trip.moved || v.speed >= beginSpeed_) continue;
        // The part its front is on, as the engine locates a vehicle.
        const auto& parts = state.index->parts[v.routeIndex];
        const auto part = std::find_if(parts.begin(), parts.end(), [&](const auto& p) { return v.distance < p.start + p.length; });
        trip.stopped.emplace((part == parts.end() ? parts.back() : *part).segmentId, state.time); // keeps the first
    }
}
ArrivalPhaseReport ArrivalPhaseAccumulator::report() const {
    ArrivalPhaseReport r{cycle_ > 0 ? cycle_ : 0, binWidth_, {}, unassigned_};
    for (const auto& [key, row] : rows_)
        r.rows.push_back({names_[key.first], key.second, row.entered, row.stopped, row.enteredAt, row.firstStopAt});
    return r;
}
}
