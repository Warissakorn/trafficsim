#pragma once
#include "movement.hpp"
#include <map>

namespace trafficsim {
// M3.2.8c, step 1: where mandatory lane changes happen and how long stub vehicles wait at their
// dead ends, per movement (docs/NEXT.md; M3_8_CONTRACT.md §2). A diagnostic, fed like
// MovementAccumulator by snapshots alone, so the engine does not know it exists. Not validated.
struct LaneChangeRow {
    std::string name;
    std::uint64_t changes{}, changedVehicles{};
    // Per change: metres from the vehicle's front to its stub's dead end at the change.
    std::vector<double> beforeDeadEnd;
    // Waiting is the engine's own test (waitingAtDeadEnd); seconds are whole ticks observed.
    std::uint64_t waitingVehicles{};
    double waitSeconds{}, longestWait{};
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
    struct Tally { std::uint64_t changes{}; std::vector<double> before; double wait{}, run{}, longest{}; };
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
