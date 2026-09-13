#pragma once
#include "../model/network/network.hpp"
#include "../eval/summary.hpp"
#include <nlohmann/json.hpp>

namespace trafficsim {
using Json = nlohmann::json;
Network parseNetwork(const Json& value);
ScenarioDefinition parseDefinition(const Json& value);
DriverBehaviour parseBehaviour(const Json& value);
VehicleType parseVehicleType(const Json& value);
Json eventJson(const SimEvent& event);
Json checkpointJson(const SimState& state);
Json summaryJson(const RunSummary& summary);
}
