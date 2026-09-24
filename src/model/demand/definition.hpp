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
struct AuthoringDefinition : ScenarioDefinition {
    bool externalVehicleTypes{true}, externalBehaviours{true};
    AuthoringDefinition() { duration = 180; timeStep = 0.1; }
    bool operator==(const AuthoringDefinition&) const = default;
};
}
