#pragma once
#include "movement.hpp"
#include <array>
#include <map>
#include <optional>

namespace trafficsim {
// M3.2.8c (D92): why a stub vehicle still waits at its dead end, decided once per wait from the
// snapshot that starts it. A wait is a run of snapshots in which waitingAtDeadEnd holds -- the
// engine's own test, as LaneChangeAccumulator counts it, so the two reports' seconds agree.
// A diagnostic fed by snapshots alone; the engine does not know it exists. Not validated.
// Causes, tested in this order:
// - noMovingApproach: on this route the vehicle never moved (at walking pace or more) inside a span
//   and inside its look-ahead (deadEndGoverns), so cooperative braking, which helps a moving
//   changer, never applied to it.
// - outsideSpan: no span target at its place (laneChangeTargetOf is null).
// - targetStanding: at its mapped place on the target route, a vehicle alongside or the nearest
//   one behind is below walking pace -- the target lane stands (a red or a queue).
// - movingStream: otherwise; the target lane moves and nobody fell in behind at the place.
// Another vehicle is placed on the target route by its front only: the part of its own route
// holding its front, then the target's part on the same segment. A body straddling a segment
// boundary is seen on its front's segment alone.
enum class WaitCause : std::size_t { noMovingApproach, outsideSpan, targetStanding, movingStream };
inline constexpr std::size_t kWaitCauses = 4;
const char* waitCauseName(WaitCause);
struct WaitCauseRow {
    std::string name;
    std::array<std::uint64_t, kWaitCauses> waits{};
    std::array<double, kWaitCauses> seconds{};
    bool operator==(const WaitCauseRow&) const = default;
};
// Movements in the spec's order, then "unfinished" and "unassigned", as LaneChangeReport.
struct WaitCauseReport { std::vector<WaitCauseRow> rows; };

class DeadEndWaitAccumulator {
public:
    explicit DeadEndWaitAccumulator(const EvaluationSpec& spec);
    // Once per state: after createSimulation and after every stepSimulation, like the others.
    void observe(const SimState& state);
    WaitCauseReport report() const;
private:
    struct Tally {
        std::uint32_t route{};
        bool unhelped{true};                // no moving approach yet on this route
        std::optional<WaitCause> waiting; // the current wait's cause
        std::array<std::uint64_t, kWaitCauses> waits{};
        std::array<double, kWaitCauses> seconds{};
    };
    std::map<std::string, std::size_t> movementOfRoute_;
    std::vector<WaitCauseRow> rows_; // movements, then unassigned
    std::map<std::uint64_t, Tally> open_;
};
// The cause of a wait starting at vehicles[v] in this snapshot; `unhelped`: no moving approach yet.
WaitCause waitCause(const SimState& state, std::size_t v, bool unhelped);
}
