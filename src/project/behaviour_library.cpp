#include "behaviour_library.hpp"
#include "../core/validate.hpp"
#include "../core/w74.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <map>
#include <set>

namespace trafficsim {
namespace {
std::string at(const char* base, std::size_t i) { return std::string(base) + "[" + std::to_string(i) + "]"; }
bool blank(const std::string& id) { return id.find_first_not_of(" \t\r\n") == std::string::npos; }
const Json& list(const Json& object, const char* key) {
    if (!object.at(key).is_array()) throw std::invalid_argument("EDIT_CATALOG_READ");
    return object.at(key);
}
std::string text(const Json& object, const char* key, bool required = true) {
    if (!object.contains(key)) {
        if (required) throw std::invalid_argument(std::string("Missing field: ") + key);
        return {};
    }
    if (!object.at(key).is_string()) throw std::invalid_argument(std::string("Expected text: ") + key);
    return object.at(key).get<std::string>();
}
std::vector<std::string> texts(const Json& object, const char* key) {
    std::vector<std::string> result;
    for (const auto& item : list(object, key)) {
        if (!item.is_string()) throw std::invalid_argument(std::string("Expected text: ") + key);
        result.push_back(item.get<std::string>());
    }
    return result;
}
template<class Road> bool assigned(const std::vector<Road>& roads) {
    return std::any_of(roads.begin(), roads.end(), [](const auto& r) { return r.behaviourTypeId.has_value(); });
}
// Every road naming a behaviour type that does not exist.
template<class Road> void roadIssues(const std::vector<Road>& roads, const char* base, const std::set<std::string>& known,
                                     std::vector<ValidationIssue>& issues) {
    for (std::size_t i = 0; i < roads.size(); ++i)
        if (roads[i].behaviourTypeId && !known.contains(*roads[i].behaviourTypeId))
            issues.push_back({"UNKNOWN_BEHAVIOUR_TYPE", at(base, i) + ".behaviourType"});
}
}
bool ownsW74Behaviour(const ProjectDocument& d) {
    return d.definition && !d.definition->externalBehaviours &&
        std::any_of(d.definition->behaviours.begin(), d.definition->behaviours.end(), [](const auto& b) { return b.w74.has_value(); });
}
bool usesBehaviourLibrary(const ProjectDocument& d) {
    if (assigned(d.network.links) || assigned(d.network.connectors) || ownsW74Behaviour(d)) return true;
    return d.definition && (!d.definition->behaviourNames.empty() || !d.definition->vehicleClasses.empty() ||
                            !d.definition->linkBehaviourTypes.empty());
}
void rejectBehaviourLibraryBefore21(const Json& j) {
    if (!j.contains("definition") || !j.at("definition").is_object()) return;
    const auto& definition = j.at("definition");
    for (const char* key : {"vehicleClasses", "linkBehaviourTypes"})
        if (definition.contains(key)) throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", std::string("definition.") + key}});
    if (!definition.contains("behaviours") || !definition.at("behaviours").is_array()) return;
    const auto& behaviours = definition.at("behaviours");
    for (std::size_t i = 0; i < behaviours.size(); ++i)
        for (const char* key : {"name", "model"})
            if (behaviours[i].is_object() && behaviours[i].contains(key))
                throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", at("behaviours", i) + "." + key}});
}
void rejectW74Before25(const Json& j) {
    if (!j.contains("definition") || !j.at("definition").is_object()) return;
    const auto& definition = j.at("definition");
    if (!definition.contains("behaviours") || !definition.at("behaviours").is_array()) return;
    const auto& behaviours = definition.at("behaviours");
    for (std::size_t i = 0; i < behaviours.size(); ++i)
        if (behaviours[i].is_object() && behaviours[i].contains("model") && behaviours[i].at("model") == kW74BehaviourModel)
            throw ValidationError({{"UNSUPPORTED_BEHAVIOUR_MODEL", at("behaviours", i) + ".model"}});
}
void parseBehaviourLibrary(const Json& j, AuthoringDefinition& d) {
    if (j.contains("behaviours")) {
        const auto& behaviours = list(j, "behaviours");
        for (std::size_t i = 0; i < behaviours.size(); ++i) {
            const auto& b = behaviours[i]; const auto path = at("behaviours", i);
            // Every model's keys are known here; parseBehaviour already refused the other model's.
            for (const auto& [key, unused] : b.items()) {
                (void)unused;
                static constexpr const char* common[] = {"id", "name", "model", "standstillDistance",
                    "additiveSafetyDistance", "multiplicativeSafetyDistance", "followingTime", "speedThreshold",
                    "maxDecelerationCooperativeBraking", "discretionaryLaneChangeThreshold",
                    "acceptedDecelerationTrailingVehicle", "discretionaryLaneChangeHoldTime"};
                const auto& w74 = w74ParameterKeys();
                if (std::none_of(std::begin(common), std::end(common), [&](const char* k) { return key == k; }) &&
                    std::none_of(w74.begin(), w74.end(), [&](const auto& k) { return key == k.name; }))
                    throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", path + "." + key}});
            }
            // The model is explicit from schema 21: a missing or unknown model is never read as the prototype.
            if (!b.contains("model") || !b.at("model").is_string() ||
                (b.at("model") != kPrototypeBehaviourModel && b.at("model") != kW74BehaviourModel))
                throw ValidationError({{"UNSUPPORTED_BEHAVIOUR_MODEL", path + ".model"}});
            if (const auto name = text(b, "name", false); !name.empty()) d.behaviourNames[text(b, "id")] = name;
        }
    }
    if (j.contains("vehicleTypes")) {
        const auto& types = list(j, "vehicleTypes");
        for (std::size_t i = 0; i < types.size(); ++i)
            requireKnownFields(types[i], {"id", "name", "length", "width", "desiredSpeed", "maxAcceleration",
                "comfortableDeceleration", "maxDeceleration", "behaviourId", "axles"}, at("vehicleTypes", i));
    }
    if (j.contains("vehicleClasses")) {
        const auto& classes = list(j, "vehicleClasses");
        for (std::size_t i = 0; i < classes.size(); ++i) {
            const auto& c = classes[i];
            requireKnownFields(c, {"id", "name", "vehicleTypeIds"}, at("vehicleClasses", i));
            d.vehicleClasses.push_back({text(c, "id"), text(c, "name", false), texts(c, "vehicleTypeIds")});
        }
    }
    if (j.contains("linkBehaviourTypes")) {
        const auto& types = list(j, "linkBehaviourTypes");
        for (std::size_t i = 0; i < types.size(); ++i) {
            const auto& t = types[i]; const auto path = at("linkBehaviourTypes", i);
            requireKnownFields(t, {"id", "name", "defaultBehaviourId", "overrides"}, path);
            LinkBehaviourType type{text(t, "id"), text(t, "name", false), text(t, "defaultBehaviourId", false), {}};
            if (t.contains("overrides")) {
                const auto& overrides = list(t, "overrides");
                for (std::size_t k = 0; k < overrides.size(); ++k) {
                    requireKnownFields(overrides[k], {"classId", "behaviourId"}, path + at(".overrides", k));
                    type.overrides.push_back({text(overrides[k], "classId"), text(overrides[k], "behaviourId")});
                }
            }
            d.linkBehaviourTypes.push_back(std::move(type));
        }
    }
}
void addBehaviourLibraryJson(const AuthoringDefinition& d, Json& j) {
    if (!d.externalBehaviours && j.contains("behaviours"))
        for (auto& b : j["behaviours"]) {
            const auto id = b.at("id").get<std::string>();
            const auto owned = std::find_if(d.behaviours.begin(), d.behaviours.end(), [&](const auto& x) { return x.id == id; });
            b["model"] = owned != d.behaviours.end() && owned->w74 ? kW74BehaviourModel : kPrototypeBehaviourModel;
            if (const auto name = d.behaviourNames.find(b.at("id").get<std::string>()); name != d.behaviourNames.end())
                b["name"] = name->second;
        }
    if (!d.vehicleClasses.empty()) {
        j["vehicleClasses"] = Json::array();
        for (const auto& c : d.vehicleClasses) {
            Json item{{"id", c.id}, {"vehicleTypeIds", c.vehicleTypeIds}};
            if (!c.name.empty()) item["name"] = c.name;
            j["vehicleClasses"].push_back(std::move(item));
        }
    }
    if (!d.linkBehaviourTypes.empty()) {
        j["linkBehaviourTypes"] = Json::array();
        for (const auto& t : d.linkBehaviourTypes) {
            Json overrides = Json::array();
            for (const auto& o : t.overrides) overrides.push_back({{"classId", o.classId}, {"behaviourId", o.behaviourId}});
            Json item{{"id", t.id}, {"defaultBehaviourId", t.defaultBehaviourId}, {"overrides", overrides}};
            if (!t.name.empty()) item["name"] = t.name;
            j["linkBehaviourTypes"].push_back(std::move(item));
        }
    }
}
std::vector<ValidationIssue> behaviourLibraryIssues(const ProjectDocument& d) {
    std::vector<ValidationIssue> issues;
    if (!usesBehaviourLibrary(d)) return issues;
    std::set<std::string> behaviourTypes;
    if (d.definition) for (const auto& t : d.definition->linkBehaviourTypes) behaviourTypes.insert(t.id);
    roadIssues(d.network.links, "links", behaviourTypes, issues);
    roadIssues(d.network.connectors, "connectors", behaviourTypes, issues);
    if (!d.definition) return issues;
    const auto& def = *d.definition;
    // Portable ownership: the library names behaviours and types the file itself carries (BA09).
    if (def.externalBehaviours || def.externalVehicleTypes) {
        issues.push_back({"EXTERNAL_BEHAVIOUR_CATALOG", "definition"});
        return issues;
    }
    std::set<std::string> behaviours, types, classes, typeIds;
    for (const auto& b : def.behaviours) behaviours.insert(b.id);
    for (const auto& t : def.vehicleTypes) types.insert(t.id);
    for (const auto& [id, name] : def.behaviourNames)
        if (!behaviours.contains(id)) issues.push_back({"UNKNOWN_BEHAVIOUR", "behaviourNames." + id});
    std::set<std::string> members;
    for (std::size_t i = 0; i < def.vehicleClasses.size(); ++i) {
        const auto& c = def.vehicleClasses[i]; const auto path = at("vehicleClasses", i);
        if (blank(c.id)) issues.push_back({"INVALID_ID", path + ".id"});
        else if (!classes.insert(c.id).second) issues.push_back({"DUPLICATE_ID", path + ".id"});
        for (std::size_t k = 0; k < c.vehicleTypeIds.size(); ++k) {
            const auto& type = c.vehicleTypeIds[k]; const auto where = path + at(".vehicleTypeIds", k);
            if (!types.contains(type)) issues.push_back({"UNKNOWN_VEHICLE_TYPE", where});
            else if (!members.insert(type).second) issues.push_back({"DUPLICATE_CLASS_MEMBERSHIP", where});
        }
    }
    for (std::size_t i = 0; i < def.linkBehaviourTypes.size(); ++i) {
        const auto& t = def.linkBehaviourTypes[i]; const auto path = at("linkBehaviourTypes", i);
        if (blank(t.id)) issues.push_back({"INVALID_ID", path + ".id"});
        else if (!typeIds.insert(t.id).second) issues.push_back({"DUPLICATE_ID", path + ".id"});
        // A default is required on every behaviour type, used or not; there is no silent fallback.
        if (t.defaultBehaviourId.empty()) issues.push_back({"MISSING_DEFAULT_BEHAVIOUR", path + ".defaultBehaviourId"});
        else if (!behaviours.contains(t.defaultBehaviourId)) issues.push_back({"UNKNOWN_BEHAVIOUR", path + ".defaultBehaviourId"});
        std::set<std::string> overridden;
        for (std::size_t k = 0; k < t.overrides.size(); ++k) {
            const auto& o = t.overrides[k]; const auto where = path + at(".overrides", k);
            if (!classes.contains(o.classId)) issues.push_back({"UNKNOWN_VEHICLE_CLASS", where + ".classId"});
            else if (!overridden.insert(o.classId).second) issues.push_back({"DUPLICATE_OVERRIDE", where + ".classId"});
            if (!behaviours.contains(o.behaviourId)) issues.push_back({"UNKNOWN_BEHAVIOUR", where + ".behaviourId"});
        }
    }
    return issues;
}
std::vector<SegmentBehaviour> compileBehaviourAssignments(const Network& network, const AuthoringDefinition& d) {
    std::map<std::string, std::string> owner; // road id -> behaviour type id
    for (const auto& l : network.links) if (l.behaviourTypeId) owner[l.id] = *l.behaviourTypeId;
    for (const auto& c : network.connectors) if (c.behaviourTypeId) owner[c.id] = *c.behaviourTypeId;
    std::vector<SegmentBehaviour> result;
    if (owner.empty()) return result;
    std::map<std::string, std::vector<RoadBehaviour>> byType; // resolved once per behaviour type
    const auto select = [&](const std::string& segment, const std::string& road) {
        const auto assigned = owner.find(road);
        if (assigned == owner.end()) return;
        auto at = byType.find(assigned->second);
        if (at == byType.end()) at = byType.emplace(assigned->second, effectiveRoadBehaviours(d, assigned->second)).first;
        for (const auto& r : at->second) result.push_back({segment, r.vehicleTypeId, r.behaviourId});
    };
    const auto table = runtimeSections(network);
    for (const auto& section : table.sections) select(section.id, section.linkId);
    for (std::size_t p = 0; p < table.paths.size(); ++p) select(table.paths[p].id, table.pathConnector[p]);
    return result;
}
std::vector<RoadBehaviour> effectiveRoadBehaviours(const AuthoringDefinition& d, const std::optional<std::string>& id) {
    std::vector<RoadBehaviour> result;
    const LinkBehaviourType* type = nullptr;
    if (id) {
        const auto found = std::find_if(d.linkBehaviourTypes.begin(), d.linkBehaviourTypes.end(),
                                        [&](const auto& t) { return t.id == *id; });
        // Validation guarantees the reference; an unknown one stays refused, never defaulted.
        if (found == d.linkBehaviourTypes.end()) throw ValidationError({{"UNKNOWN_BEHAVIOUR_TYPE", *id}});
        type = &*found;
    }
    std::map<std::string, std::string> classOf;
    for (const auto& c : d.vehicleClasses) for (const auto& t : c.vehicleTypeIds) classOf[t] = c.id;
    for (const auto& vehicle : d.vehicleTypes) {
        RoadBehaviour r{vehicle.id, vehicle.behaviourId, BehaviourSource::inherited};
        if (type) {
            r = {vehicle.id, type->defaultBehaviourId, BehaviourSource::typeDefault};
            if (const auto group = classOf.find(vehicle.id); group != classOf.end())
                for (const auto& o : type->overrides)
                    if (o.classId == group->second) r = {vehicle.id, o.behaviourId, BehaviourSource::classOverride};
        }
        result.push_back(std::move(r));
    }
    return result;
}
}
