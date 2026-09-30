#include "segment_times.hpp"
#include <algorithm>

namespace trafficsim {
SegmentTimeAccumulator::SegmentTimeAccumulator(const EvaluationSpec& spec)
    : movementOfRoute_(spec.movementOfRoute), names_(spec.movementNames), movements_(spec.movementNames.size()) {}
void SegmentTimeAccumulator::observe(const SimState& state) {
    for (const auto& event : state.events) {
        if (const auto* departed = std::get_if<DepartedEvent>(&event)) {
            open_[departed->vehicleId].departed = departed->time;
        } else if (const auto* entered = std::get_if<SegmentEnteredEvent>(&event)) {
            if (auto trip = open_.find(entered->vehicleId); trip != open_.end())
                trip->second.entered.emplace_back(entered->segmentId, entered->time - trip->second.departed);
        } else if (const auto* arrived = std::get_if<ArrivedEvent>(&event)) {
            const auto trip = open_.find(arrived->vehicleId);
            if (trip == open_.end()) { ++undeparted_; continue; }
            const auto m = movementOfRoute_.find(arrived->routeId);
            if (m == movementOfRoute_.end()) ++unassigned_;
            else {
                auto& row = movements_[m->second];
                ++row.vehicles; row.departureDelay += arrived->departureDelay;
                row.travel += arrived->travelTime; row.freeFlow += arrived->freeFlowTime;
                for (const auto& [segment, since] : trip->second.entered) {
                    auto& sum = row.segments[segment];
                    ++sum.vehicles; sum.sinceDeparture += since;
                }
            }
            open_.erase(trip);
        }
    }
}
SegmentTimeReport SegmentTimeAccumulator::report() const {
    SegmentTimeReport r{{}, unassigned_, undeparted_};
    for (std::size_t m = 0; m < movements_.size(); ++m) {
        const auto& row = movements_[m];
        const double n = row.vehicles ? static_cast<double>(row.vehicles) : 1.0;
        SegmentTimeRow out{names_[m], row.vehicles, row.departureDelay / n, row.travel / n, row.freeFlow / n, {}};
        for (const auto& [segment, sum] : row.segments)
            out.segments.push_back({segment, sum.vehicles, sum.sinceDeparture / static_cast<double>(sum.vehicles)});
        std::sort(out.segments.begin(), out.segments.end(), [](const auto& a, const auto& b) {
            return a.meanSinceDeparture != b.meanSinceDeparture ? a.meanSinceDeparture < b.meanSinceDeparture
                                                                : a.segmentId < b.segmentId;
        });
        r.rows.push_back(std::move(out));
    }
    return r;
}
}
