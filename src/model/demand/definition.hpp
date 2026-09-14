#pragma once
#include "../../core/types.hpp"

namespace trafficsim {
// The existing Route, VehicleInput and fixed-time SignalProgram value contracts are
// also the authoring types. Catalog presence is explicit: an empty override stays empty.
struct AuthoringDefinition : ScenarioDefinition {
    bool externalVehicleTypes{true}, externalBehaviours{true};
    AuthoringDefinition() { duration = 180; timeStep = 0.1; }
};
}
