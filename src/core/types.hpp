#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace trafficsim {
// Runtime contract: metres, seconds, m/s and m/s². No UI or file-format types.
struct Segment { std::string id; double length{}; std::vector<std::string> next; };
struct Route {
    std::string id; std::vector<std::string> segmentIds;
    bool operator==(const Route&) const = default;
};
struct DriverBehaviour {
    std::string id;
    double standstillDistance{}, additiveSafetyDistance{}, multiplicativeSafetyDistance{};
    double followingTime{}, speedThreshold{};
    bool operator==(const DriverBehaviour&) const = default;
};
struct SpeedRange { double min{}, max{}; bool operator==(const SpeedRange&) const = default; };
struct VehicleType {
    std::string id;
    double length{}, width{};
    SpeedRange desiredSpeed;
    double maxAcceleration{}, comfortableDeceleration{}, maxDeceleration{};
    std::string behaviourId;
    bool operator==(const VehicleType&) const = default;
};
struct VehicleInput {
    std::string id, routeId, vehicleTypeId;
    double vehiclesPerHour{}, startTime{}, endTime{};
    bool operator==(const VehicleInput&) const = default;
};
enum class SignalColor { red, amber, green };
struct SignalPhase { double duration{}; SignalColor color{}; bool operator==(const SignalPhase&) const = default; };
struct SignalProgram {
    std::string id; double offset{}; std::vector<SignalPhase> phases;
    bool operator==(const SignalProgram&) const = default;
};
struct SignalHead { std::string id, segmentId; double position{}; std::string programId; };
// A minor approach giving way to a major one, at a point where two segments feed the same place.
// Vissim's priority rule: a stop line on the minor approach, a conflict marker on the major one,
// and the two numbers an engineer tunes -- a gap time in seconds and a headway in metres.
//
// This is a DETERMINISTIC THRESHOLD TEST, not a calibrated critical-gap distribution: a vehicle
// waits while any major vehicle is within `headway` of the conflict point or would reach it
// within `gapTime`, and goes otherwise. It makes a merge expressible and tunable. It does not
// make it validated -- see hard rule 4.
struct PriorityDefaults {
    double gapTime{}, headway{};
    bool operator==(const PriorityDefaults&) const = default;
};
struct PriorityRule {
    std::string id;
    std::string yieldSegmentId; double yieldPosition{};      // where the minor approach waits
    std::string conflictSegmentId; double conflictPosition{}; // the point on the major approach
    double gapTime{};   // seconds: a major vehicle arriving sooner than this is not yielded to
    double headway{};   // metres: a major vehicle closer than this to the point blocks regardless
    bool operator==(const PriorityRule&) const = default;
};
struct ScenarioDefinition {
    double duration{}, timeStep{};
    std::vector<Route> routes;
    std::vector<VehicleType> vehicleTypes;
    std::vector<DriverBehaviour> behaviours;
    std::vector<VehicleInput> inputs;
    std::vector<SignalProgram> signalPrograms;
    // Ordered last so every existing brace-initialisation of a definition keeps meaning what it
    // says. Empty is the normal state: a scenario with no merge needs no rule.
    std::vector<PriorityRule> priorityRules;
    // The two numbers a rule DERIVED from the drawing is given, read from data/priority-rules/ by
    // the project layer. Content, not code (hard rule 5). Left at zero here on purpose: a zero
    // gap time is an uncontrolled merge, so deriving a rule without loading these is refused
    // rather than silently allowed.
    PriorityDefaults priorityDefaults;
    // Value equality, so callers can tell "this edit changed nothing" without serialising.
    bool operator==(const ScenarioDefinition&) const = default;
};
struct Scenario : ScenarioDefinition {
    std::vector<Segment> segments;
    std::vector<SignalHead> signalHeads;
};
struct RoutePart { std::string segmentId; std::size_t segmentIndex{}; double start{}, length{}; };
// Route geometry is a pure function of an immutable Scenario, so it is resolved once per run
// instead of per vehicle per tick. parts[i] corresponds to Scenario::routes[i] after
// canonicalisation; nothing here is derived from vehicle state.
// A signal head that lies on a route, with the start station of the FIRST route part
// carrying that head's segment - the part the per-vehicle scan used to search for.
struct RouteHead { std::size_t headIndex{}; double partStart{}; };
// The same idea for a priority rule: a rule whose yield segment lies on this route, with the start
// station of the first route part carrying it, so the stop line is a route coordinate.
struct RouteRule { std::size_t ruleIndex{}; double partStart{}; };
// One id -> position entry in a sorted lookup table.
struct IdSlot { std::string id; std::size_t index{}; };
struct ScenarioIndex {
    std::vector<std::vector<RoutePart>> parts;
    std::vector<std::size_t> programOfHead;          // parallel to Scenario::signalHeads
    std::vector<std::vector<RouteHead>> routeHeads;  // parallel to Scenario::routes, in signalHeads order
    std::vector<std::vector<RouteRule>> routeRules;  // parallel to Scenario::routes, in priorityRules order
    // The segment index each rule watches, so the per-tick scan is a bucket lookup and never a
    // search by id. SIZE_MAX when the rule names a segment that does not exist; validation
    // rejects that scenario, but a hand-built index must not read out of bounds before it does.
    std::vector<std::size_t> conflictSegmentOfRule;  // parallel to Scenario::priorityRules
    // Id lookups a vehicle needs every tick, resolved once per scenario. Before this, every tick
    // re-derived the same three indices for every vehicle by linear search over string ids, which
    // the profile showed as a quarter of the whole run.
    //
    // Sorted by id, searched by lower_bound -- NOT a hash map: core/ may not use unordered
    // containers at all (hard rule 2, enforced by tools/check_architecture.cpp), because their
    // iteration order is unspecified and that would put reproducibility at the mercy of the
    // standard library. Ties are ordered by the element's own position, so a lookup on a repeated
    // id selects its FIRST occurrence -- exactly the element byId's linear scan returned.
    std::vector<IdSlot> routeOfId, typeOfId;
    // The behaviour a vehicle uses depends only on its TYPE, so it needs no per-vehicle lookup
    // at all once the type is known.
    std::vector<std::size_t> behaviourOfType;        // parallel to Scenario::vehicleTypes
};
// Scenario lookups for one vehicle, resolved once per tick instead of once per use.
struct VehicleRefs { std::size_t route{}, type{}, behaviour{}; };
enum class FollowingMode { free, approaching, following, braking };
struct PendingVehicle {
    std::uint64_t id{};
    std::string inputId, routeId, vehicleTypeId;
    double scheduledTime{}, desiredSpeed{}, driverFactor{};
    bool operator==(const PendingVehicle&) const = default;
};
struct Vehicle : PendingVehicle {
    double enteredTime{}, distance{}, speed{}, acceleration{};
    FollowingMode mode{FollowingMode::free};
    bool operator==(const Vehicle&) const = default;
};
struct InputState {
    std::string id;
    std::optional<double> nextArrival;
    std::vector<PendingVehicle> queue;
    bool operator==(const InputState&) const = default;
};
struct SignalEvent {
    double time{}; std::string signalId; SignalColor color{};
    bool operator==(const SignalEvent&) const = default;
};
struct DepartedEvent {
    double time{}; std::uint64_t vehicleId{}; std::string routeId;
    double scheduledTime{}, desiredSpeed{};
    bool operator==(const DepartedEvent&) const = default;
};
struct MovedEvent {
    double time{}; std::uint64_t vehicleId{}; std::string segmentId;
    double position{}, speed{}, acceleration{};
    bool operator==(const MovedEvent&) const = default;
};
struct SegmentEnteredEvent {
    double time{}; std::uint64_t vehicleId{}; std::string segmentId;
    bool operator==(const SegmentEnteredEvent&) const = default;
};
struct SafetyClampEvent {
    double time{}; std::uint64_t vehicleId{};
    bool operator==(const SafetyClampEvent&) const = default;
};
struct ArrivedEvent {
    double time{}; std::uint64_t vehicleId{}; std::string routeId;
    double travelTime{}, departureDelay{}, freeFlowTime{};
    bool operator==(const ArrivedEvent&) const = default;
};
using SimEvent = std::variant<SignalEvent, DepartedEvent, MovedEvent,
                              SegmentEnteredEvent, SafetyClampEvent, ArrivedEvent>;
struct SimState {
    // Detached at createSimulation; copies share only this immutable scenario.
    std::shared_ptr<const Scenario> scenario;
    // Derived from scenario alone; shared, never copied per tick.
    std::shared_ptr<const ScenarioIndex> index;
    std::uint32_t seed{}, randomState{};
    std::uint64_t tick{}, nextVehicleId{1}, completed{};
    double time{};
    std::vector<InputState> inputs;
    std::vector<Vehicle> vehicles;
    std::vector<SimEvent> events; // Latest step only.
};
struct ValidationIssue {
    std::string code, path;
    bool operator==(const ValidationIssue&) const = default;
};
}
