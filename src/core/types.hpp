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
struct ScenarioDefinition {
    double duration{}, timeStep{};
    std::vector<Route> routes;
    std::vector<VehicleType> vehicleTypes;
    std::vector<DriverBehaviour> behaviours;
    std::vector<VehicleInput> inputs;
    std::vector<SignalProgram> signalPrograms;
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
struct ScenarioIndex {
    std::vector<std::vector<RoutePart>> parts;
    std::vector<std::size_t> programOfHead;          // parallel to Scenario::signalHeads
    std::vector<std::vector<RouteHead>> routeHeads;  // parallel to Scenario::routes, in signalHeads order
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
