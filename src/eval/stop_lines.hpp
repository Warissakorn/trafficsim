#pragma once
#include "movement.hpp"
#include <map>

namespace trafficsim {
// M3.2.8c step 3: how each signal head's stop line discharges. Per head, the vehicles that
// crossed it: how long each stood (below the queue's beginSpeed) upstream of it during red and
// during green, and through how many green ends it stood. Per green: how many crossed, the mean
// gap between them, and how many were left standing at its end. Amber is not green (D36).
// Fed by snapshots alone; it reads only SignalEvent, positions and speeds, all older than
// M3.2.8b, so it also measures an engine from before it. Not validated.
struct StopLineRow {
    std::string headId;
    std::uint64_t crossed{}, greens{};
    double meanStandRed{}, meanStandGreen{};   // s per crossing vehicle
    double heldShare{}, meanHeld{};            // crossing vehicles held through >= 1 green end; mean ends
    double meanDischarged{}, meanHeadway{};    // per green; s between crossings within one green
    double meanResidual{};                     // vehicles standing upstream at a green's end
    bool operator==(const StopLineRow&) const = default;
};
struct StopLineReport { std::vector<StopLineRow> rows; }; // in Scenario::signalHeads order

class StopLineAccumulator {
public:
    explicit StopLineAccumulator(const EvaluationSpec& spec);
    // Once per state: after createSimulation (its SignalEvents set every head's first colour)
    // and after every stepSimulation.
    void observe(const SimState& state);
    StopLineReport report() const;
private:
    struct Tally { double red{}, green{}; std::uint64_t held{}; };
    struct Head {
        std::string id; std::vector<double> atRoute; // NaN: the route misses this head
        bool green{};
        std::uint64_t crossed{}, heldAny{}, held{}, greens{}, greenEnds{}, discharged{}, residual{}, gaps{};
        double red{}, standGreen{}, gapSum{}, lastCrossing{-1};
    };
    void bind(const SimState& state);
    double beginSpeed_{};
    const Scenario* bound_{};
    std::vector<Head> heads_;
    std::map<std::pair<std::uint64_t, std::size_t>, Tally> open_; // (vehicle, head) while upstream
};
}
