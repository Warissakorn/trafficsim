#pragma once
#include "../../core/types.hpp"

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
// M2.4. Vissim's static routing decision: turning proportions as relative flows over routes that
// all leave the same Link, for the whole period. M2.1.1 places it: with `linkId` set it acts on
// every routeless vehicle reaching that Link, and an entry may name a destination Link instead of
// a route. Per-interval flows and a station part way along the Link are still M2.1.
struct DecisionRoute {
    std::string routeId; double relativeFlow{};
    std::string destinationLinkId; // M2.1.1: instead of routeId, on a placed decision only
    bool operator==(const DecisionRoute&) const = default;
};
struct RoutingDecision {
    std::string id, name; std::vector<DecisionRoute> routes;
    std::string linkId; // M2.1.1: the Link it is placed on; empty keeps the M2.4 meaning
    bool operator==(const RoutingDecision&) const = default;
};
struct AuthoringDefinition : ScenarioDefinition {
    bool externalVehicleTypes{true}, externalBehaviours{true};
    std::vector<RoutingDecision> routingDecisions; // M2.4
    AuthoringDefinition() { duration = 180; timeStep = 0.1; }
    bool operator==(const AuthoringDefinition&) const = default;
};
}
