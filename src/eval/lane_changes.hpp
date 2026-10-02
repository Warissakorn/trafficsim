#pragma once
#include "movement.hpp"
#include <map>
#include <optional>

namespace trafficsim {
// M3.2.8c, step 1: where mandatory lane changes happen and how long stub vehicles wait at their
// dead ends, per movement (docs/NEXT.md; M3_8_CONTRACT.md §2). A diagnostic, fed like
// MovementAccumulator by snapshots alone, so the engine does not know it exists. Not validated.
struct LaneChangeRow {
    std::string name;
    std::uint64_t changes{}, changedVehicles{};
    // Per change, from the pre-step snapshot it was decided on: metres from the vehicle's front to
    // its stub's dead end, and its front's distance along the stub route (from the network edge,
    // where every stub starts). A change needs the rear inside a span, so none happens in the
    // insertion tick and every change has a previous snapshot; `unplaced` counts any that did not.
    std::vector<double> beforeDeadEnd, atDistance;
    std::uint64_t unplaced{};
    // Waiting is the engine's own test (waitingAtDeadEnd); seconds are whole ticks observed.
    std::uint64_t waitingVehicles{};
    double waitSeconds{}, longestWait{};
    // D95: changes from a FULL route (by choice), which the counts and positions above leave out,
    // and those of them made within 3 s of the same vehicle's previous change, of either kind (A53).
    std::uint64_t discretionaryChanges{}, quickRepeats{};
    // The repeats by kind, summing to quickRepeats: back to the route the previous change left
    // (A to B to A), right after a mandatory change, or onward to a third route.
    std::uint64_t quickBack{}, quickAfterMandatory{}, quickOnward{};
    // A53 as D101 rewrote it: changes by choice straight back to the route the previous change
    // left, within 10 s of it -- a window longer than any hold, so a hold cannot pass it by itself.
    std::uint64_t returns{};
    bool operator==(const LaneChangeRow&) const = default;
};
// One row per movement in the spec's order, then "unfinished" for vehicles still in the network
// and "unassigned" for arrivals on a route no movement owns. A vehicle is counted on the movement
// it arrives on, so a stub's changes land on the full chain it finished.
struct LaneChangeReport { std::vector<LaneChangeRow> rows; };

class LaneChangeAccumulator {
public:
    explicit LaneChangeAccumulator(const EvaluationSpec& spec);
    // Once per state: after createSimulation and after every stepSimulation, like the others.
    void observe(const SimState& state);
    LaneChangeReport report() const;
private:
    struct Tally { std::uint64_t changes{}, unplaced{}; std::vector<double> before, at; double wait{}, run{}, longest{};
                   std::uint64_t discretionary{}, quickRepeats{}, quickBack{}, quickAfterMandatory{}, quickOnward{}, returns{};
                   std::optional<double> lastChange; std::string lastFrom; bool lastMandatory{}; };
    void bind(const SimState& state);
    const Scenario* bound_{};
    std::map<std::string, std::size_t> movementOfRoute_, slotOfRoute_;
    std::vector<LaneChangeRow> rows_; // movements, then unassigned
    std::map<std::uint64_t, Tally> open_;
    // The previous snapshot's route slot and distance per vehicle id: a change is decided on the
    // pre-step snapshot, so this is where it happened.
    std::map<std::uint64_t, std::pair<std::uint32_t, double>> previous_;
};
}
