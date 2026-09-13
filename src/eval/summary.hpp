#pragma once
#include "../core/types.hpp"

namespace trafficsim {
struct RunSummary {
    std::uint64_t completed{}, safetyClamps{};
    std::optional<double> meanTravelTime, meanDelay;
};
// Incremental completed-trip diagnostic. This is NOT HCM control delay or LOS.
class SummaryAccumulator {
public:
    void add(const SimEvent& event);
    RunSummary summary() const;
private:
    std::uint64_t completed_{}, clamps_{};
    double travel_{}, delay_{};
};
std::size_t pendingCount(const SimState& state);
}
