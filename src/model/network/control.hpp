#pragma once
// M3.2.2: authored right-of-way controls (docs/M3_CONTRACT.md §1). Values only -- they belong to
// the project network and are persisted with it. Everything derived from them (runtime segments,
// route incidence, the compiled core PriorityRule) is computed, never stored.
//
// Waiting lines, conflict areas, their priority rules and (M3.2.5) Stop/Yield controls. Queue
// counters are M3.2.6 and are not here yet.
#include <optional>
#include <string>
#include <vector>

namespace trafficsim {
// One lane-level path: a Link lane when `connectorId` is empty, otherwise the Connector lane
// that joins `fromLaneId` to `toLaneId`. Never a section id, a `/lane-k` path id or a scenario
// slot -- those are derived and change when the drawing does.
struct ControlPathRef {
    std::string linkId, laneId;
    std::string connectorId, fromLaneId, toLaneId;
    bool operator==(const ControlPathRef&) const = default;
};
// Link stations are metres on the Link's reference polyline, Connector stations metres on its
// stored (first-lane) polyline -- the coordinates the author's other attachments already use.
struct ControlPoint {
    ControlPathRef path; double station{};
    bool operator==(const ControlPoint&) const = default;
};
struct WaitingLine {
    std::string id, name; ControlPoint point;
    bool operator==(const WaitingLine&) const = default;
};
enum class ConflictKind { crossing, merge };
enum class ConflictPriority { firstYields, secondYields, undetermined };
struct ConflictSide {
    ControlPathRef path; double entryStation{}, exitStation{}; std::string waitingLineId;
    bool operator==(const ConflictSide&) const = default;
};
struct ConflictArea {
    std::string id, name; ConflictKind kind{ConflictKind::merge};
    ConflictSide first, second; ConflictPriority priority{ConflictPriority::undetermined};
    bool operator==(const ConflictArea&) const = default;
};
// At most one per area. Direction comes from the area, never a second copy here.
struct AuthoredPriorityRule {
    std::string id, name, conflictAreaId; double gapTime{}, headway{};
    bool operator==(const AuthoredPriorityRule&) const = default;
};
// M3.2.5 (contract §1, §5): what a driver must do at one waiting line before crossing the areas
// it controls. `yield` is the gap test alone; `stop` also means a full stop at the line. At most
// one control per line, and every area it names gives way at exactly that line.
enum class StopMode { stop, yield };
struct StopControl {
    std::string id, name, waitingLineId; StopMode mode{StopMode::stop};
    std::vector<std::string> conflictAreaIds;
    bool operator==(const StopControl&) const = default;
};
struct RightOfWay {
    std::vector<WaitingLine> waitingLines;
    std::vector<ConflictArea> conflictAreas;
    std::vector<AuthoredPriorityRule> priorityRules;
    std::vector<StopControl> stopControls; // schema 15
    bool empty() const { return waitingLines.empty() && conflictAreas.empty() && priorityRules.empty() && stopControls.empty(); }
    bool operator==(const RightOfWay&) const = default;
};
// M3.2.6b (contract §1). Where a queue is measured from: an existing Signal head or waiting line
// (`referenceId`), or an explicit ControlPoint -- exactly one of the two. The place is then stored
// once, by whatever owns it, so moving the head moves the counter's line with it.
struct MeasurementLine {
    std::string referenceId; std::optional<ControlPoint> point;
    bool operator==(const MeasurementLine&) const = default;
};
// A queue counter the author placed, independent of signal heads. Its row replaces the counter
// derived from a Link's heads when it measures any of them (no duplicate approach rows, A23).
struct AuthoredQueueCounter {
    std::string id, name; std::vector<MeasurementLine> lines;
    bool operator==(const AuthoredQueueCounter&) const = default;
};
const char* conflictKindName(ConflictKind);
const char* conflictPriorityName(ConflictPriority);
// Throw std::invalid_argument("INVALID_ENUM") on anything else, so a bad file fails to load.
ConflictKind conflictKindFromName(const std::string&);
ConflictPriority conflictPriorityFromName(const std::string&);
const char* stopModeName(StopMode);
StopMode stopModeFromName(const std::string&);
}
