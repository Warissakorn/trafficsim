#include "json.hpp"
#include <type_traits>

namespace trafficsim {
namespace {
const char* colorName(SignalColor color) {
    switch (color) { case SignalColor::red: return "red"; case SignalColor::amber: return "amber";
                     case SignalColor::green: return "green"; }
    return "invalid";
}
const char* modeName(FollowingMode mode) {
    switch (mode) { case FollowingMode::free: return "free"; case FollowingMode::approaching: return "approaching";
                   case FollowingMode::following: return "following"; case FollowingMode::braking: return "braking"; }
    return "invalid";
}
// A vehicle carries scenario SLOTS, not names; a checkpoint carries the names, because it is
// read by people and by the frozen fixtures and a slot means nothing outside one Scenario.
Json pendingJson(const Scenario& scenario, const PendingVehicle& v) {
    const std::string inputId = v.inputIndex == PendingVehicle::kNoInput ? std::string{}
                                                                        : scenario.inputs[v.inputIndex].id;
    return {{"id", v.id}, {"inputId", inputId}, {"routeId", scenario.routes[v.routeIndex].id},
            {"vehicleTypeId", scenario.vehicleTypes[v.typeIndex].id},
            {"scheduledTime", v.scheduledTime}, {"desiredSpeed", v.desiredSpeed}, {"driverFactor", v.driverFactor}};
}
Json optionalNumber(const std::optional<double>& value) { return value ? Json(*value) : Json(nullptr); }
}
Json eventJson(const SimEvent& event) {
    return std::visit([](const auto& e) -> Json {
        using T = std::decay_t<decltype(e)>;
        Json j{{"time", e.time}};
        if constexpr (std::is_same_v<T, SignalEvent>) {
            j["kind"] = "signal"; j["signalId"] = e.signalId; j["color"] = colorName(e.color);
        } else {
            j["vehicleId"] = e.vehicleId;
            if constexpr (std::is_same_v<T, DepartedEvent>) {
                j["kind"] = "departed"; j["routeId"] = e.routeId;
                j["scheduledTime"] = e.scheduledTime; j["desiredSpeed"] = e.desiredSpeed;
            } else if constexpr (std::is_same_v<T, MovedEvent>) {
                j["kind"] = "moved"; j["segmentId"] = e.segmentId; j["position"] = e.position;
                j["speed"] = e.speed; j["acceleration"] = e.acceleration;
            } else if constexpr (std::is_same_v<T, SegmentEnteredEvent>) {
                j["kind"] = "segment-entered"; j["segmentId"] = e.segmentId;
            } else if constexpr (std::is_same_v<T, SafetyClampEvent>) j["kind"] = "safety-clamp";
            else {
                j["kind"] = "arrived"; j["routeId"] = e.routeId; j["travelTime"] = e.travelTime;
                j["departureDelay"] = e.departureDelay; j["freeFlowTime"] = e.freeFlowTime;
            }
        }
        return j;
    }, event);
}
Json checkpointJson(const SimState& state) {
    // Every slot in the state is a position in this scenario, so there is nothing to serialise
    // without it. Say that, rather than dereferencing a null.
    if (!state.scenario) throw std::invalid_argument("Checkpoint requires a scenario");
    const auto& scenario = *state.scenario;
    Json vehicles = Json::array(), inputs = Json::array();
    for (const auto& vehicle : state.vehicles) {
        auto j = pendingJson(scenario, vehicle);
        j["enteredTime"] = vehicle.enteredTime; j["distance"] = vehicle.distance;
        j["speed"] = vehicle.speed; j["acceleration"] = vehicle.acceleration; j["mode"] = modeName(vehicle.mode);
        vehicles.push_back(std::move(j));
    }
    for (std::size_t i = 0; i < state.inputs.size(); ++i) {
        Json queue = Json::array();
        for (const auto& pending : state.inputs[i].queue) queue.push_back(pendingJson(scenario, pending));
        inputs.push_back({{"id", scenario.inputs[i].id},
                          {"nextArrival", optionalNumber(state.inputs[i].nextArrival)}, {"queue", queue}});
    }
    return {{"tick", state.tick}, {"randomState", state.randomState}, {"nextVehicleId", state.nextVehicleId},
            {"completed", state.completed}, {"vehicles", vehicles}, {"inputs", inputs}};
}
Json summaryJson(const RunSummary& summary) {
    return {{"validation", "not-yet-validated"}, {"completed", summary.completed}, {"safetyClamps", summary.safetyClamps},
            {"meanTravelTime", optionalNumber(summary.meanTravelTime)}, {"meanDelay", optionalNumber(summary.meanDelay)}};
}
}
