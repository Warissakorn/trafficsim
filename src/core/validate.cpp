#include "validate.hpp"
#include "conflicts.hpp"
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
    // The predecessor LIST, not just a count: whether a merge is arbitrated depends on which
    // segments feed it, because a priority rule names one of them as the one to give way to.
    std::map<std::string, std::vector<std::string>> predecessors;
    for (std::size_t i = 0; i < s.segments.size(); ++i) {
        const auto& item = s.segments[i];
        const auto p = "segments[" + std::to_string(i) + "]";
        number(item.length, p + ".length");
        if (std::set<std::string>(item.next.begin(), item.next.end()).size() != item.next.size())
            add("DUPLICATE_CONNECTION", p + ".next");
        for (const auto& next : item.next) {
            if (!segments.contains(next)) add("UNKNOWN_SEGMENT", p + ".next");
            predecessors[next].push_back(item.id);
        }
    }
    // M3.2.2c: a minor approach may wait before the segment that yields -- a waiting line on the
    // Link before a Connector -- but only as far back as EVERY route onto that segment must come:
    // along its chain of single predecessors. Past a second predecessor, a route could reach the
    // conflict without ever crossing the line. Shared by priority rules and conflict zones.
    const auto singleApproach = [&](const std::string& segment) {
        double upstream = 0;
        std::set<std::string> chain{segment};
        for (auto at = segment;;) {
            const auto feeding = predecessors.find(at);
            if (feeding == predecessors.end() || feeding->second.size() != 1) break;
            const auto before = segments.find(feeding->second.front());
            if (before == segments.end() || !chain.insert(before->first).second) break;
            upstream += before->second->length;
            at = before->first;
        }
        return upstream;
    };
    const auto rules = index(s.priorityRules, "priorityRules", issues);
    for (std::size_t i = 0; i < s.priorityRules.size(); ++i) {
        const auto& rule = s.priorityRules[i];
        const auto p = "priorityRules[" + std::to_string(i) + "]";
        number(rule.gapTime, p + ".gapTime", true); number(rule.headway, p + ".headway", true);
        const auto yieldOn = segments.find(rule.yieldSegmentId), conflictOn = segments.find(rule.conflictSegmentId);
        const double upstream = singleApproach(rule.yieldSegmentId);
        if (yieldOn == segments.end()) add("UNKNOWN_SEGMENT", p + ".yieldSegmentId");
        else if (!std::isfinite(rule.yieldPosition) || rule.yieldPosition < -upstream ||
                 rule.yieldPosition > yieldOn->second->length) add("INVALID_POSITION", p + ".yieldPosition");
        if (conflictOn == segments.end()) add("UNKNOWN_SEGMENT", p + ".conflictSegmentId");
        else if (!std::isfinite(rule.conflictPosition) || rule.conflictPosition < 0 ||
                 rule.conflictPosition > conflictOn->second->length) add("INVALID_POSITION", p + ".conflictPosition");
        // A segment cannot give way to itself: that is a rule that can never be satisfied, and it
        // would also let the merge check below count it as arbitration.
        if (rule.yieldSegmentId == rule.conflictSegmentId) add("INVALID_RANGE", p + ".conflictSegmentId");
    }
    (void)rules;
    // M3.2.3a/b. A conflict zone: two chains of consecutive segments, an area from entry on the
    // first to exit on the last, a waiting line on the minor approach no later than its entry,
    // and the two threshold numbers.
    index(s.conflictZones, "conflictZones", issues);
    bool zonesValid = true;
    for (std::size_t i = 0; i < s.conflictZones.size(); ++i) {
        const auto& zone = s.conflictZones[i];
        const auto p = "conflictZones[" + std::to_string(i) + "]";
        number(zone.gapTime, p + ".gapTime"); number(zone.headway, p + ".headway", true);
        for (const auto& [side, name] : {std::pair{&zone.major, ".major"}, std::pair{&zone.minor, ".minor"}}) {
            const auto& chain = side->segmentIds;
            bool known = !chain.empty();
            for (std::size_t k = 0; k < chain.size(); ++k) {
                const auto on = segments.find(chain[k]);
                if (on == segments.end()) { add("UNKNOWN_SEGMENT", p + name + ".segmentIds"); known = false; break; }
                if (k + 1 < chain.size() && !has(on->second->next, chain[k + 1])) {
                    add("INVALID_RANGE", p + name + ".segmentIds"); known = false; break;
                }
            }
            if (!known) { zonesValid = false; continue; }
            const double first = segments.at(chain.front())->length, last = segments.at(chain.back())->length;
            if (!std::isfinite(side->entry) || !std::isfinite(side->exit) || side->entry < 0 || side->entry >= first ||
                side->exit <= 0 || side->exit > last || (chain.size() == 1 && side->entry >= side->exit))
                add("INVALID_RANGE", p + name);
        }
        for (const auto& id : zone.major.segmentIds)
            if (has(zone.minor.segmentIds, id)) add("INVALID_RANGE", p + ".minor.segmentIds");
        if (!std::isfinite(zone.waitPosition) || zone.waitPosition > zone.minor.entry || zone.minor.segmentIds.empty() ||
            zone.waitPosition < -singleApproach(zone.minor.segmentIds.front())) add("INVALID_POSITION", p + ".waitPosition");
    }
    // Per route, through the same incidence the run uses (conflicts.hpp).
    double longest = 0;
    for (const auto& type : s.vehicleTypes) if (std::isfinite(type.length)) longest = std::max(longest, type.length);
    for (std::size_t r = 0; zonesValid && !s.conflictZones.empty() && r < s.routes.size(); ++r) {
        std::vector<RoutePart> parts;
        double start = 0;
        for (const auto& id : s.routes[r].segmentIds) {
            const auto seg = segments.find(id);
            if (seg == segments.end()) { parts.clear(); break; }
            parts.push_back({id, 0, start, seg->second->length});
            start += seg->second->length;
        }
        if (parts.empty()) continue;
        const auto p = "routes[" + std::to_string(r) + "]";
        bool major = false, minor = false;
        for (const auto& rz : zoneIncidence(s, parts)) {
            const auto zone = "conflictZones[" + std::to_string(rz.zoneIndex) + "] " + p;
            (rz.role == ZoneRole::major ? major : minor) = true;
            // The route must run on past the area far enough to carry the longest vehicle's rear
            // out of it, or removal at the route's end would drop a tail still inside (A12).
            if (start - rz.exitAt < longest) add("CONFLICT_SINK_TOO_CLOSE", zone);
            if (rz.role != ZoneRole::minor) continue;
            // Meeting a minor chain part way, or starting past its line, enters without asking.
            if (rz.joinsInside) add("CONFLICT_ROUTE_JOINS_INSIDE", zone);
            else if (rz.waitAt < 0) add("CONFLICT_ROUTE_STARTS_PAST_LINE", zone);
        }
        // Minor at one zone and major at another: it could hold one while waiting at the other,
        // whose holder waits on it. Refused until an arbitration exists for it (M3.2.3c).
        if (major && minor) add("CONFLICT_MIXED_ROLES", p);
    }
    // The one guard M3.1 loosens, and it is loosened BY CONSTRUCTION, never by removal: a place
    // fed by n segments is runnable only when at least n-1 of them give way to another of them,
    // so exactly one has priority and the rest have somewhere to wait. A network that has not
    // been through the priority model reports UNSUPPORTED_MERGE exactly as it always did.
    for (const auto& [id, feeding] : predecessors) {
        if (feeding.size() <= 1) continue;
        std::size_t yielding = 0;
        for (const auto& minor : feeding) {
            const bool gives = std::any_of(s.priorityRules.begin(), s.priorityRules.end(),
                [&](const auto& rule) {
                    return rule.yieldSegmentId == minor &&
                           std::any_of(feeding.begin(), feeding.end(), [&](const auto& major) {
                               return major != minor && rule.conflictSegmentId == major; });
                });
            if (gives) ++yielding;
        }
        if (yielding + 1 < feeding.size()) add("UNSUPPORTED_MERGE", "segments." + id);
    }
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
