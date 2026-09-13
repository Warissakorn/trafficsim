#include "validate.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace trafficsim {
namespace {
std::string issueMessage(const std::vector<ValidationIssue>& issues) {
    std::string message;
    for (const auto& issue : issues) message += issue.code + ": " + issue.path + '\n';
    return message;
}
bool blank(const std::string& id) { return id.find_first_not_of(" \t\r\n") == std::string::npos; }
template<class T>
std::map<std::string, const T*> index(const std::vector<T>& items, const std::string& path,
                                    std::vector<ValidationIssue>& issues) {
    std::map<std::string, const T*> result;
    for (std::size_t i = 0; i < items.size(); ++i) {
        const auto p = path + "[" + std::to_string(i) + "].id";
        if (blank(items[i].id)) issues.push_back({"INVALID_ID", p});
        else if (result.contains(items[i].id)) issues.push_back({"DUPLICATE_ID", p});
        result[items[i].id] = &items[i];
    }
    return result;
}
bool has(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}
}
ValidationError::ValidationError(std::vector<ValidationIssue> value)
    : std::runtime_error(issueMessage(value)), issues(std::move(value)) {}
bool onTimeGrid(double value, double timeStep) {
    const double ticks = value / timeStep;
    return std::isfinite(ticks) && std::abs(std::round(ticks)) <= 9007199254740991.0 &&
           std::abs(ticks - std::round(ticks)) < 1e-7;
}
std::vector<ValidationIssue> validateScenario(const Scenario& s) {
    std::vector<ValidationIssue> issues;
    const auto add = [&](const std::string& code, const std::string& path) { issues.push_back({code, path}); };
    const auto number = [&](double value, const std::string& path, bool zero = false) {
        if (!std::isfinite(value) || (zero ? value < 0 : value <= 0)) add("INVALID_NUMBER", path);
    };
    number(s.timeStep, "timeStep");
    if (s.timeStep > 0.5) add("TIMESTEP_TOO_LARGE", "timeStep");
    number(s.duration, "duration");
    if (!onTimeGrid(s.duration, s.timeStep)) add("OFF_TIME_GRID", "duration");
    const auto segments = index(s.segments, "segments", issues);
    const auto routes = index(s.routes, "routes", issues);
    const auto types = index(s.vehicleTypes, "vehicleTypes", issues);
    const auto behaviours = index(s.behaviours, "behaviours", issues);
    const auto programs = index(s.signalPrograms, "signalPrograms", issues);
    index(s.inputs, "inputs", issues); index(s.signalHeads, "signalHeads", issues);
    if (segments.empty()) add("EMPTY_NETWORK", "segments");
    std::map<std::string, unsigned> predecessors;
    for (std::size_t i = 0; i < s.segments.size(); ++i) {
        const auto& item = s.segments[i];
        const auto p = "segments[" + std::to_string(i) + "]";
        number(item.length, p + ".length");
        if (std::set<std::string>(item.next.begin(), item.next.end()).size() != item.next.size())
            add("DUPLICATE_CONNECTION", p + ".next");
        for (const auto& next : item.next) {
            if (!segments.contains(next)) add("UNKNOWN_SEGMENT", p + ".next");
            ++predecessors[next];
        }
    }
    for (const auto& [id, count] : predecessors)
        if (count > 1) add("UNSUPPORTED_MERGE", "segments." + id);
    for (std::size_t i = 0; i < s.routes.size(); ++i) {
        const auto& ids = s.routes[i].segmentIds;
        const auto p = "routes[" + std::to_string(i) + "].segmentIds";
        if (ids.empty()) add("EMPTY_ROUTE", p);
        if (std::set<std::string>(ids.begin(), ids.end()).size() != ids.size()) add("UNSUPPORTED_ROUTE_CYCLE", p);
        double routeLength = 0;
        for (std::size_t j = 0; j < ids.size(); ++j) {
            const auto segment = segments.find(ids[j]);
            if (segment == segments.end()) add("UNKNOWN_SEGMENT", p + "[" + std::to_string(j) + "]");
            else {
                routeLength += segment->second->length;
                if (j + 1 < ids.size() && !has(segment->second->next, ids[j + 1]))
                    add("DISCONNECTED_ROUTE", p + "[" + std::to_string(j + 1) + "]");
            }
        }
        if (!std::isfinite(routeLength)) add("INVALID_NUMBER", p);
    }
    for (std::size_t i = 0; i < s.behaviours.size(); ++i) {
        const auto& b = s.behaviours[i];
        const auto p = "behaviours[" + std::to_string(i) + "]";
        number(b.standstillDistance, p + ".standstillDistance");
        number(b.additiveSafetyDistance, p + ".additiveSafetyDistance", true);
        number(b.multiplicativeSafetyDistance, p + ".multiplicativeSafetyDistance", true);
        number(b.followingTime, p + ".followingTime"); number(b.speedThreshold, p + ".speedThreshold");
    }
    for (std::size_t i = 0; i < s.vehicleTypes.size(); ++i) {
        const auto& t = s.vehicleTypes[i];
        const auto p = "vehicleTypes[" + std::to_string(i) + "]";
        number(t.length, p + ".length"); number(t.width, p + ".width");
        number(t.maxAcceleration, p + ".maxAcceleration");
        number(t.comfortableDeceleration, p + ".comfortableDeceleration"); number(t.maxDeceleration, p + ".maxDeceleration");
        number(t.desiredSpeed.min, p + ".desiredSpeed.min"); number(t.desiredSpeed.max, p + ".desiredSpeed.max");
        if (t.desiredSpeed.max < t.desiredSpeed.min) add("INVALID_RANGE", p + ".desiredSpeed");
        if (t.maxDeceleration < t.comfortableDeceleration) add("INVALID_RANGE", p + ".maxDeceleration");
        if (!behaviours.contains(t.behaviourId)) add("UNKNOWN_BEHAVIOUR", p + ".behaviourId");
    }
    for (std::size_t i = 0; i < s.inputs.size(); ++i) {
        const auto& input = s.inputs[i];
        const auto p = "inputs[" + std::to_string(i) + "]";
        if (!routes.contains(input.routeId)) add("UNKNOWN_ROUTE", p + ".routeId");
        if (!types.contains(input.vehicleTypeId)) add("UNKNOWN_VEHICLE_TYPE", p + ".vehicleTypeId");
        number(input.vehiclesPerHour, p + ".vehiclesPerHour", true);
        number(input.startTime, p + ".startTime", true); number(input.endTime, p + ".endTime");
        if (input.startTime >= input.endTime || input.endTime > s.duration) add("INVALID_INTERVAL", p);
        const auto route = routes.find(input.routeId);
        if (route != routes.end() && !route->second->segmentIds.empty() &&
            predecessors.contains(route->second->segmentIds.front())) add("UNSUPPORTED_INTERNAL_INPUT", p + ".routeId");
    }
    for (std::size_t i = 0; i < s.signalPrograms.size(); ++i) {
        const auto& program = s.signalPrograms[i];
        const auto p = "signalPrograms[" + std::to_string(i) + "]";
        if (!onTimeGrid(program.offset, s.timeStep)) add("OFF_TIME_GRID", p + ".offset");
        if (program.phases.empty()) add("EMPTY_SIGNAL_PROGRAM", p + ".phases");
        double cycle = 0;
        for (std::size_t j = 0; j < program.phases.size(); ++j) {
            const auto& phase = program.phases[j];
            const auto q = p + ".phases[" + std::to_string(j) + "]";
            number(phase.duration, q + ".duration");
            if (!onTimeGrid(phase.duration, s.timeStep)) add("OFF_TIME_GRID", q + ".duration");
            if (phase.color != SignalColor::red && phase.color != SignalColor::amber && phase.color != SignalColor::green)
                add("INVALID_SIGNAL_COLOR", q + ".color");
            cycle += phase.duration;
        }
        if (!onTimeGrid(cycle, s.timeStep)) add("OFF_TIME_GRID", p + ".cycle");
    }
    for (std::size_t i = 0; i < s.signalHeads.size(); ++i) {
        const auto& head = s.signalHeads[i];
        const auto p = "signalHeads[" + std::to_string(i) + "]";
        const auto segment = segments.find(head.segmentId);
        if (segment == segments.end()) add("UNKNOWN_SEGMENT", p + ".segmentId");
        if (!programs.contains(head.programId)) add("UNKNOWN_SIGNAL_PROGRAM", p + ".programId");
        number(head.position, p + ".position", true);
        if (segment != segments.end() && head.position > segment->second->length) add("INVALID_POSITION", p + ".position");
    }
    return issues;
}
void assertValidScenario(const Scenario& scenario) {
    auto issues = validateScenario(scenario);
    if (!issues.empty()) throw ValidationError(std::move(issues));
}
}
