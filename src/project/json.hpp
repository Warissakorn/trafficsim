#pragma once
#include "../model/network/network.hpp"
#include "../eval/summary.hpp"
#include "../model/demand/definition.hpp"
#include <nlohmann/json_fwd.hpp>

namespace trafficsim {
using Json = nlohmann::json;
// Section accessors that fail with a named, translatable code instead of an nlohmann
// type_error. A JSON null reaching .at() is the difference between "this file is the
// wrong kind" and an exception string no user can act on.
const Json& section(const Json& value, const char* name);
bool present(const Json& value, const char* name); // an absent or null member is not present
// `schemaVersion` decides how an attachment position is read: metres along the link from
// version 5, a fraction of lane arclength below it, converted here once the links exist.
// 0 is a pre-schema M0 scenario, which is read with the old meaning. Reading the unit from
// the key alone would silently turn "0.4" of a lane into 0.4 metres.
Network parseNetwork(const Json& value, int schemaVersion);
ScenarioDefinition parseDefinition(const Json& value);
DriverBehaviour parseBehaviour(const Json& value);
PriorityDefaults parsePriorityDefaults(const Json& value);
VehicleType parseVehicleType(const Json& value);
Composition parseComposition(const Json& value);
std::vector<RoutingDecision> parseRoutingDecisions(const Json& definition); // M2.4
std::vector<SignalController> parseSignalControllers(const Json& definition); // M2.7b
Json eventJson(const SimEvent& event);
Json checkpointJson(const SimState& state);
Json summaryJson(const RunSummary& summary);
}
