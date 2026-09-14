#pragma once
#include "../model/network/network.hpp"
#include "../eval/summary.hpp"
#include <nlohmann/json.hpp>

namespace trafficsim {
using Json = nlohmann::json;
// Section accessors that fail with a named, translatable code instead of an nlohmann
// type_error. A JSON null reaching .at() is the difference between "this file is the
// wrong kind" and an exception string no user can act on.
const Json& section(const Json& value, const char* name);
bool present(const Json& value, const char* name); // an absent or null member is not present
Network parseNetwork(const Json& value);
ScenarioDefinition parseDefinition(const Json& value);
DriverBehaviour parseBehaviour(const Json& value);
VehicleType parseVehicleType(const Json& value);
Json eventJson(const SimEvent& event);
Json checkpointJson(const SimState& state);
Json summaryJson(const RunSummary& summary);
}
