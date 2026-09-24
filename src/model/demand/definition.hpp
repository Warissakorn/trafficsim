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
// M2.4. Vissim's static routing decision, in its smallest form: turning proportions as relative
// flows over routes that all leave the same Link, for the whole period. Per-interval flows and a
// decision positioned part way along a Link are M2.1.
struct DecisionRoute {
    std::string routeId; double relativeFlow{};
    bool operator==(const DecisionRoute&) const = default;
};
struct RoutingDecision {
    std::string id, name; std::vector<DecisionRoute> routes;
    bool operator==(const RoutingDecision&) const = default;
};
struct AuthoringDefinition : ScenarioDefinition {
    bool externalVehicleTypes{true}, externalBehaviours{true};
    std::vector<RoutingDecision> routingDecisions; // M2.4
    AuthoringDefinition() { duration = 180; timeStep = 0.1; }
    bool operator==(const AuthoringDefinition&) const = default;
};
}
