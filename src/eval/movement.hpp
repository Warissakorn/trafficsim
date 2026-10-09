#pragma once
#include "summary.hpp"
#include "sections.hpp"
#include "los.hpp"
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
struct QueueCounter { std::string name; std::vector<CounterLine> lines; };
struct EvaluationSpec {
    std::vector<std::string> movementNames;
    std::map<std::string, std::size_t> movementOfRoute; // runtime route id -> movement
    std::vector<QueueCounter> counters;
    QueueDefinition queue;
    // M5.3 (D132): rows count trips that END in [warmup, end] and queues average its ticks; no end
    // is the end of the run. Run totals in the report stay whole-run.
    double warmup{}; std::optional<double> end;
    std::vector<SectionSpec> sections; // M5.4 (D133), in authored order
    std::optional<LosPack> los; // M5.5 (D134): read only when a section has a control type
};
struct MovementRow {
    std::string name; std::uint64_t vehicles{};
    std::optional<double> meanDelay, meanTravelTime;
    std::uint64_t unfinished{}; // M5.3: active or pending on this movement when the report is taken
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
    std::uint64_t laneChanges{}; // M3.2.8b
    double warmup{}, evaluationEnd{}; // M5.3: the period the rows and queues describe
    std::vector<SectionRow> sections; // M5.4: one per authored section; empty without any
    std::optional<LosPack> los; // M5.5: the spec's pack, so output can letter the rows
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
    // Last observed queue state, sorted by vehicle id: a vector, not a map, because it is rebuilt
    // every tick and a node per vehicle was most of observe()'s cost.
    std::vector<std::pair<std::uint64_t, bool>> queued_;
    // One line's candidates, reused across lines and ticks so observe() allocates nothing here.
    std::vector<QueuedVehicle> behind_;
    SectionAccumulator sections_;
};
}
