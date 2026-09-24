#pragma once
#include "../../core/types.hpp"
#include <algorithm>

namespace trafficsim {
// The existing Route, VehicleInput and fixed-time SignalProgram value contracts are
// also the authoring types. Catalog presence is explicit: an empty override stays empty.
// M2.3. A vehicle composition: vehicle types and their relative shares of an input's volume.
// Catalog content (data/compositions/), like vehicle types, never compiled in (hard rule 5).
struct CompositionShare {
    std::string vehicleTypeId; double share{};
    bool operator==(const CompositionShare&) const = default;
};
struct Composition {
    std::string id; std::vector<CompositionShare> types;
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
// part way along the Link is still M2.1.
struct DecisionRoute {
    std::string routeId; double relativeFlow{};
    std::string destinationLinkId; // M2.1.1: instead of routeId, on a placed decision only
    std::vector<double> intervalFlows; // M2.1.2: parallel to RoutingDecision::intervals
    bool operator==(const DecisionRoute&) const = default;
};
struct RoutingDecision {
    std::string id, name; std::vector<DecisionRoute> routes;
    std::string linkId; // M2.1.1: the Link it is placed on; empty keeps the M2.4 meaning
    std::vector<DecisionInterval> intervals; // M2.1.2: ordered, non-overlapping
    bool operator==(const RoutingDecision&) const = default;
};
// M2.1.2 (D45). An entry's relative flow at time t: its interval's flow inside an interval, the
// whole-period relativeFlow outside every one. t is when the vehicle ENTERS the network.
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
struct AuthoringDefinition : ScenarioDefinition {
    bool externalVehicleTypes{true}, externalBehaviours{true};
    std::vector<RoutingDecision> routingDecisions; // M2.4
    AuthoringDefinition() { duration = 180; timeStep = 0.1; }
    bool operator==(const AuthoringDefinition&) const = default;
};
}
