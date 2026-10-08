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
// The `model` key selects the keys (D131): absent or "prototype" reads the prototype's, "w74" the
// 18 W74 keys (INVALID_BEHAVIOUR_PARAMETER when one is missing). Each model's keys are refused on
// the other (EDIT_UNSUPPORTED_FIELD); any other model is UNSUPPORTED_BEHAVIOUR_MODEL. `path`
// names the entry in those issues.
DriverBehaviour parseBehaviour(const Json& value, const std::string& path = "behaviour");
inline constexpr const char* kPrototypeBehaviourModel = "prototype";
inline constexpr const char* kW74BehaviourModel = "w74";
PriorityDefaults parsePriorityDefaults(const Json& value);
// EDIT_UNSUPPORTED_FIELD for any key outside `keys`, whatever the schema (M3.3.2a strict sections).
void requireKnownFields(const Json& value, std::initializer_list<const char*> keys, const std::string& path);
VehicleType parseVehicleType(const Json& value);
Composition parseComposition(const Json& value);
std::vector<RoutingDecision> parseRoutingDecisions(const Json& definition); // M2.4
std::vector<SignalController> parseSignalControllers(const Json& definition); // M2.7b
Json eventJson(const SimEvent& event);
Json checkpointJson(const SimState& state);
Json summaryJson(const RunSummary& summary);
}
