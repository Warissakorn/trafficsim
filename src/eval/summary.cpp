#include "summary.hpp"
#include <algorithm>

namespace trafficsim {
void SummaryAccumulator::add(const SimEvent& event) {
    if (std::holds_alternative<SafetyClampEvent>(event)) ++clamps_;
    if (const auto* arrived = std::get_if<ArrivedEvent>(&event)) {
        ++completed_;
        travel_ += arrived->travelTime;
        delay_ += std::max(0.0, arrived->travelTime + arrived->departureDelay - arrived->freeFlowTime);
    }
}
RunSummary SummaryAccumulator::summary() const {
    RunSummary result{completed_, clamps_, {}, {}};
    if (completed_) {
        result.meanTravelTime = travel_ / static_cast<double>(completed_);
        result.meanDelay = delay_ / static_cast<double>(completed_);
    }
    return result;
}
std::size_t pendingCount(const SimState& state) {
    std::size_t count = 0;
    for (const auto& input : state.inputs) count += input.queue.size();
    return count;
}
}
