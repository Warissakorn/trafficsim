#pragma once
#include "summary.hpp"
#include <map>

namespace trafficsim {
// Movement evaluation for one run (M2.5). Fed only by SimState snapshots and their events, so the
// engine does not know it exists. NOT HCM control delay, NOT LOS, not validated (rule 4).
struct QueueDefinition {
    // Vissim's queue-counter conditions, in SI units: a vehicle joins the queue below
    // beginSpeed, leaves it above endSpeed, and a clear gap over maxGap ends the queue.
    double beginSpeed{}, endSpeed{}, maxGap{};
};
// One approach (M3.2.6b): the lines its queue is measured from, each metres along one runtime
// segment -- a head's stop line, a waiting line or a point the author placed; one per lane. The
// counter reports the longest queue behind any of them. Where a line came from does not matter
// here, so a head-derived counter and an authored one are the same measurement.
struct CounterLine { std::string segmentId; double position{}; bool operator==(const CounterLine&) const = default; };
struct QueueCounter { std::string name; std::vector<CounterLine> lines; };
struct EvaluationSpec {
    std::vector<std::string> movementNames;
    std::map<std::string, std::size_t> movementOfRoute; // runtime route id -> movement
    std::vector<QueueCounter> counters;
    QueueDefinition queue;
};
struct MovementRow {
    std::string name; std::uint64_t vehicles{};
    std::optional<double> meanDelay, meanTravelTime;
    bool operator==(const MovementRow&) const = default;
};
struct QueueRow {
    std::string name; double meanLength{}, maxLength{};
    bool operator==(const QueueRow&) const = default;
};
struct MovementReport {
    std::vector<MovementRow> movements;
    std::vector<QueueRow> queues;
    std::uint64_t completed{}, safetyClamps{}, unassigned{};
    std::optional<double> meanDelay;
    std::size_t pending{}, active{};
    double time{};
    bool operator==(const MovementReport&) const = default;
};
// Delay a completed trip contributes: the same term as the run summary, so movements add up.
double tripDelay(const ArrivedEvent& arrived);
// The queue behind one stop line: `upstream` is each vehicle's front distance before the line
// (negative = past it), sorted or not. Returns the length in metres to the last queued rear.
struct QueuedVehicle { double upstream{}, length{}; bool queued{}; };
double queueLength(std::vector<QueuedVehicle> vehicles, double maxGap);

class MovementAccumulator {
public:
    explicit MovementAccumulator(EvaluationSpec spec);
    // Once per state: after createSimulation and after every stepSimulation. Skipping one loses
    // that step's arrivals, as with SummaryAccumulator.
    void observe(const SimState& state);
    MovementReport report(const SimState& end) const;
private:
    void bind(const SimState& state);
    EvaluationSpec spec_;
    SummaryAccumulator summary_;
    const Scenario* bound_{};
    std::vector<std::size_t> movementOfSlot_;  // parallel to Scenario::routes; npos = none
    // Each counter line with its route distance on every route slot (NaN: the route misses it).
    struct BoundLine { std::size_t counter{}; std::vector<double> atRoute; };
    std::vector<BoundLine> lines_;
    std::vector<std::uint64_t> count_;
    std::vector<double> delay_, travel_;
    std::uint64_t unassigned_{}, observed_{};
    std::vector<double> queueSum_, queueMax_;
    std::map<std::uint64_t, bool> queued_;
};
}
