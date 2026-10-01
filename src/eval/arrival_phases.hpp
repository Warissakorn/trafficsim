#pragma once
#include "movement.hpp"
#include <map>

namespace trafficsim {
// M3.2.8c step 4: when in the signal cycle vehicles reach a segment and first stop on it. Per
// movement a vehicle arrived on and per segment, two histograms of the phase in the cycle
// (simulation time mod the cycle, offsets not folded in): when its front entered the segment (a
// SegmentEnteredEvent, or its departure for the route's first segment; a lane change is never an
// entry) and when it first stood there: below the queue's beginSpeed, once it has reached its
// endSpeed (vehicles enter at rest, so without that hysteresis every entry Link would record each
// insertion; one that queues straight from insertion is never counted). The cycle is the sum of a
// program's phase durations; when the programs disagree, or there is none, the report has cycle 0
// and no rows.
// Fed by snapshots alone; it reads only events older than M3.2.8b, so it also measures an engine
// from before it. Not validated.
struct ArrivalPhaseRow {
    std::string movement, segmentId;
    std::uint64_t entered{}, stopped{};
    std::vector<std::uint64_t> enteredAt, firstStopAt; // one count per binWidth of the cycle
    bool operator==(const ArrivalPhaseRow&) const = default;
};
// Rows in the spec's movement order, then by segment id; only pairs that saw a vehicle.
struct ArrivalPhaseReport { double cycle{}, binWidth{}; std::vector<ArrivalPhaseRow> rows; std::uint64_t unassigned{}; };

class ArrivalPhaseAccumulator {
public:
    explicit ArrivalPhaseAccumulator(const EvaluationSpec& spec, double binWidth = 10);
    // Once per state: after createSimulation and after every stepSimulation.
    void observe(const SimState& state);
    ArrivalPhaseReport report() const;
private:
    struct Trip { bool moved{}; std::vector<std::pair<std::string, double>> entered; std::map<std::string, double> stopped; };
    struct Row { std::uint64_t entered{}, stopped{}; std::vector<std::uint64_t> enteredAt, firstStopAt; };
    std::size_t bin(double time) const;
    std::map<std::string, std::size_t> movementOfRoute_;
    std::vector<std::string> names_;
    double beginSpeed_{}, endSpeed_{}, binWidth_{}, cycle_{-1};
    std::map<std::uint64_t, Trip> open_;
    std::map<std::pair<std::size_t, std::string>, Row> rows_;
    std::uint64_t unassigned_{};
};
}
