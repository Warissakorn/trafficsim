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
struct Route { std::string id; std::vector<std::string> segmentIds; };
struct DriverBehaviour {
    std::string id;
    double standstillDistance{}, additiveSafetyDistance{}, multiplicativeSafetyDistance{};
    double followingTime{}, speedThreshold{};
};
struct SpeedRange { double min{}, max{}; };
struct VehicleType {
    std::string id;
    double length{}, width{};
    SpeedRange desiredSpeed;
    double maxAcceleration{}, comfortableDeceleration{}, maxDeceleration{};
    std::string behaviourId;
};
struct VehicleInput {
    std::string id, routeId, vehicleTypeId;
    double vehiclesPerHour{}, startTime{}, endTime{};
};
enum class SignalColor { red, amber, green };
struct SignalPhase { double duration{}; SignalColor color{}; };
struct SignalProgram { std::string id; double offset{}; std::vector<SignalPhase> phases; };
struct SignalHead { std::string id, segmentId; double position{}; std::string programId; };
struct ScenarioDefinition {
    double duration{}, timeStep{};
    std::vector<Route> routes;
    std::vector<VehicleType> vehicleTypes;
    std::vector<DriverBehaviour> behaviours;
    std::vector<VehicleInput> inputs;
    std::vector<SignalProgram> signalPrograms;
};
struct Scenario : ScenarioDefinition {
    std::vector<Segment> segments;
    std::vector<SignalHead> signalHeads;
};
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
