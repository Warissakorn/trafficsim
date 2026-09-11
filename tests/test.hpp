#pragma once
#include "../src/core/simulation.hpp"
#include "../src/core/validate.hpp"
#include "../src/project/load.hpp"
#include "../src/project/json.hpp"
#include <cmath>
#include <functional>
#include <stdexcept>

namespace test {
struct Case { std::string group, name; std::function<void()> run; };
std::vector<Case>& cases();
struct Register {
    Register(const char* group, const char* name, std::function<void()> run) {
        cases().push_back({group, name, std::move(run)});
    }
};
inline void require(bool value, const char* expression, const char* file, int line) {
    if (!value) throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + ": " + expression);
}
inline void near(double a, double b, double tolerance = 1e-7) {
    if (!std::isfinite(a) || !std::isfinite(b) || std::abs(a - b) > tolerance)
        throw std::runtime_error("Numeric mismatch: " + std::to_string(a) + " vs " + std::to_string(b));
}
template<class F> void throws(F action, const std::string& message = {}) {
    try { action(); } catch (const std::exception& error) {
        if (std::string(error.what()).find(message) == std::string::npos)
            throw std::runtime_error("Wrong exception: " + std::string(error.what()));
        return;
    }
    throw std::runtime_error("Expected an exception: " + message);
}
inline std::filesystem::path root() { return TRAFFICSIM_SOURCE_DIR; }
inline trafficsim::LoadedScenario demo() {
    return trafficsim::loadScenario(root() / "data/scenarios/crossing.json", root() / "data");
}
inline trafficsim::Scenario straight() {
    const auto source = demo().scenario;
    trafficsim::Scenario s;
    s.duration = 120; s.timeStep = 0.1;
    s.segments = {{"road", 150, {}}}; s.routes = {{"route", {"road"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.inputs = {{"input", "route", "car", 900, 0, 60}};
    return s;
}
inline trafficsim::Vehicle vehicle(std::uint64_t id, double distance, double speed = 0) {
    trafficsim::Vehicle v;
    v.id = id; v.distance = distance; v.speed = speed;
    v.inputId = "input"; v.routeId = "route"; v.vehicleTypeId = "car";
    v.desiredSpeed = 15; v.driverFactor = 0.5;
    return v;
}
inline trafficsim::SimState withVehicles(trafficsim::Scenario s, std::vector<trafficsim::Vehicle> vehicles) {
    s.inputs.clear(); auto state = trafficsim::createSimulation(s, 42);
    state.vehicles = std::move(vehicles);
    for (const auto& v : state.vehicles) state.nextVehicleId = std::max(state.nextVehicleId, v.id + 1);
    return state;
}
inline trafficsim::SimState finish(trafficsim::SimState state) {
    while (state.tick < trafficsim::totalTicks(*state.scenario)) state = trafficsim::stepSimulation(state);
    return state;
}
}
#define CHECK(...) test::require(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __FILE__, __LINE__)
#define TEST(group, name) static void group##_##name(); \
    static test::Register register_##group##_##name(#group, #name, group##_##name); \
    static void group##_##name()
