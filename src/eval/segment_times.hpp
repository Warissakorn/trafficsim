#pragma once
#include "movement.hpp"
#include <map>

namespace trafficsim {
// M3.2.8c step 2: where along its route a movement's time goes. Per movement a vehicle arrived on,
// the mean time from its departure (insertion) to entering each runtime segment, plus its mean
// departure delay, travel time and free-flow time. A diagnostic fed by snapshots alone; it reads
// only events older than M3.2.8b, so it also measures an engine from before it. Not validated.
struct SegmentTime {
    std::string segmentId; std::uint64_t vehicles{}; double meanSinceDeparture{};
    bool operator==(const SegmentTime&) const = default;
};
struct SegmentTimeRow {
    std::string name; std::uint64_t vehicles{};
    double meanDepartureDelay{}, meanTravelTime{}, meanFreeFlowTime{};
    std::vector<SegmentTime> segments; // by meanSinceDeparture, then id
    bool operator==(const SegmentTimeRow&) const = default;
};
// One row per movement in the spec's order. Arrivals on no movement's route are `unassigned`;
// a vehicle with no DepartedEvent (placed by hand, not released by an input) is `undeparted`.
struct SegmentTimeReport { std::vector<SegmentTimeRow> rows; std::uint64_t unassigned{}, undeparted{}; };

class SegmentTimeAccumulator {
public:
    explicit SegmentTimeAccumulator(const EvaluationSpec& spec);
    // Once per state, like MovementAccumulator. A segment is timed when the vehicle's front
    // crosses its start; a vehicle's first segment and a lane change are never "entered".
    void observe(const SimState& state);
    SegmentTimeReport report() const;
private:
    struct Trip { double departed{}; std::vector<std::pair<std::string, double>> entered; };
    struct Sum { std::uint64_t vehicles{}; double sinceDeparture{}; };
    struct Movement { std::uint64_t vehicles{}; double departureDelay{}, travel{}, freeFlow{}; std::map<std::string, Sum> segments; };
    std::map<std::string, std::size_t> movementOfRoute_;
    std::vector<std::string> names_;
    std::vector<Movement> movements_;
    std::map<std::uint64_t, Trip> open_;
    std::uint64_t unassigned_{}, undeparted_{};
};
}
