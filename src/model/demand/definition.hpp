#pragma once
#include "../../core/types.hpp"
#include <algorithm>
#include <map>

namespace trafficsim {
// The existing Route, VehicleInput and fixed-time SignalProgram value contracts are
// also the authoring types. Catalog presence is explicit: an empty override stays empty.
// M2.3. A vehicle composition: vehicle types and their relative shares of an input's volume.
// Catalog content (data/compositions/), like vehicle types, never compiled in (hard rule 5).
struct CompositionShare {
    std::string vehicleTypeId; double share{};
    bool operator==(const CompositionShare&) const = default;
};
struct CompositionInterval {
    double startTime{}, endTime{}; std::vector<CompositionShare> types;
    bool operator==(const CompositionInterval&) const = default;
};
struct Composition {
    std::string id; std::vector<CompositionShare> types;
    std::string name;
    std::vector<CompositionInterval> intervals; // Half-open; gaps use types.
    bool operator==(const Composition&) const = default;
};
// M2.1.2. One time interval of a routing decision's counted turning volumes.
struct DecisionInterval {
    double startTime{}, endTime{};
    bool operator==(const DecisionInterval&) const = default;
};
// M2.4. Vissim's static routing decision: turning proportions as relative flows over routes that
// all leave the same Link, for the whole period. M2.1.1 places it: with `linkId` set it acts on
// every routeless vehicle reaching that Link, and an entry may name a destination Link instead of
// a route. M2.1.2: `intervals` with one `intervalFlows` value per entry per interval give turning
// proportions that change over the run; outside every interval `relativeFlow` applies. A station
// part way along the Link opts into passage-time recognition with `position` (D119).
struct DecisionRoute {
    std::string routeId; double relativeFlow{};
    std::string destinationLinkId; // M2.1.1: instead of routeId, on a placed decision only
    std::vector<double> intervalFlows; // M2.1.2: parallel to RoutingDecision::intervals
    bool operator==(const DecisionRoute&) const = default;
};
// Complete weights in decision route order; intervalFlows is route-major. Missing type
// rules inherit default flows. A zero interval total falls back to that type's whole-period weights.
struct RoutingTypeRule {
    std::string vehicleTypeId; std::vector<double> relativeFlows;
    std::vector<std::vector<double>> intervalFlows;
    bool operator==(const RoutingTypeRule&) const = default;
};
struct RoutingDecision {
    std::string id, name; std::vector<DecisionRoute> routes;
    std::string linkId; // M2.1.1: the Link it is placed on; empty keeps the M2.4 meaning
    std::vector<DecisionInterval> intervals; // M2.1.2: ordered, non-overlapping
    std::vector<RoutingTypeRule> typeRules;
    // Metres on the Link reference polyline. Absent retains legacy scheduled-time routing.
    std::optional<double> position;
    bool operator==(const RoutingDecision&) const = default;
};
// M2.1.2 (D45). An entry's relative flow at time t: its interval's flow inside an interval, the
// whole-period relativeFlow outside every one. t is the scheduled demand time, before source queueing.
// The decision's counts are proportions only: the input's volume is what is split (D46), so the
// two tables need not agree. An interval with no turn counted at all (every flow 0) falls back
// to the whole-period proportions rather than stranding the input's vehicles in it.
inline double decisionFlowAt(const RoutingDecision& d, const DecisionRoute& entry, double t) {
    const auto at = [](const DecisionRoute& e, std::size_t k) { return k < e.intervalFlows.size() ? e.intervalFlows[k] : 0.0; };
    for (std::size_t k = 0; k < d.intervals.size(); ++k)
        if (t >= d.intervals[k].startTime && t < d.intervals[k].endTime) {
            double sum = 0;
            for (const auto& e : d.routes) if (at(e, k) > 0) sum += at(e, k);
            return sum > 0 ? at(entry, k) : entry.relativeFlow;
        }
    return entry.relativeFlow;
}
// Every time at which some decision's flows change, ascending, without duplicates.
inline std::vector<double> decisionBreakpoints(const std::vector<const RoutingDecision*>& decisions) {
    std::vector<double> points;
    for (const auto* d : decisions) for (const auto& i : d->intervals) { points.push_back(i.startTime); points.push_back(i.endTime); }
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    return points;
}
// M2.7b. Vissim's fixed-time signal controller: one cycle and offset, and signal groups that each
// turn green at `greenStart` and end green at `greenEnd` -- seconds of the controller's cycle,
// the green wrapping past the cycle's end when greenEnd < greenStart -- followed by `amber`, red
// for the rest. A controller's cycle second at simulation time t is (t + offset) mod cycle, the
// same convention a core SignalProgram uses. Expanded at compile time into one core
// SignalProgram per group (signal_control.hpp), so core never sees a controller.
struct SignalGroup {
    int number{}; std::string name; double greenStart{}, greenEnd{}, amber{3};
    bool operator==(const SignalGroup&) const = default;
};
struct SignalController {
    std::string id, name; double cycle{90}, offset{}; std::vector<SignalGroup> groups;
    bool operator==(const SignalController&) const = default;
};
// M3.3.2a (D126, DRIVING_BEHAVIOUR.md §1). Authoring only: nothing here reaches the engine until
// M3.3.2b compiles it. A class groups vehicle types for assignment; a type is in at most one.
struct VehicleClass {
    std::string id, name; std::vector<std::string> vehicleTypeIds;
    bool operator==(const VehicleClass&) const = default;
};
struct BehaviourOverride {
    std::string classId, behaviourId;
    bool operator==(const BehaviourOverride&) const = default;
};
// Vissim's link behaviour type: a required default behaviour and at most one override per class.
struct LinkBehaviourType {
    std::string id, name, defaultBehaviourId; std::vector<BehaviourOverride> overrides;
    bool operator==(const LinkBehaviourType&) const = default;
};
// M5.4 (D139), schema 23: the part of a run that is measured -- trips arriving after `warmup` and
// no later than `end`, seconds from the start. Both are the author's; absent means the whole run.
struct EvaluationPeriod {
    double warmup{}, end{};
    bool operator==(const EvaluationPeriod&) const = default;
};
struct AuthoringDefinition : ScenarioDefinition {
    bool externalVehicleTypes{true}, externalBehaviours{true};
    bool externalCompositions{true};
    std::vector<Composition> compositions;
    std::map<std::string,std::string> vehicleTypeNames;
    std::vector<RoutingDecision> routingDecisions; // M2.4
    std::vector<SignalController> signalControllers; // M2.7b; `signalPrograms` keeps only legacy ones
    // M3.3.2a, schema 21: owned-behaviour display names, classes and link behaviour types.
    std::map<std::string,std::string> behaviourNames;
    std::vector<VehicleClass> vehicleClasses;
    std::vector<LinkBehaviourType> linkBehaviourTypes;
    std::optional<EvaluationPeriod> evaluationPeriod; // M5.4, schema 23
    AuthoringDefinition() { duration = 180; timeStep = 0.1; }
    bool operator==(const AuthoringDefinition&) const = default;
};
}
