#include "behaviour_commands.hpp"
#include <algorithm>
#include <stdexcept>

namespace trafficsim {
namespace {
AuthoringDefinition& owned(ProjectDocument& d) {
    if (!d.definition || d.definition->externalBehaviours || d.definition->externalVehicleTypes)
        throw std::invalid_argument("EDIT_EXTERNAL_CATALOG");
    return *d.definition;
}
template<class T> void put(std::vector<T>& values, T value) {
    for (auto& existing : values) if (existing.id == value.id) { existing = std::move(value); return; }
    values.push_back(std::move(value));
}
template<class T> T& find(std::vector<T>& values, const std::string& id, const char* missing) {
    const auto it = std::find_if(values.begin(), values.end(), [&](const auto& v) { return v.id == id; });
    if (it == values.end()) throw std::invalid_argument(missing);
    return *it;
}
template<class T> std::string freshId(const std::vector<T>& values, const std::string& id) {
    const auto taken = [&](const std::string& candidate) {
        return std::any_of(values.begin(), values.end(), [&](const auto& v) { return v.id == candidate; });
    };
    auto candidate = id + "-copy";
    for (int n = 2; taken(candidate); ++n) candidate = id + "-copy-" + std::to_string(n);
    return candidate;
}
// Replacing an entry by itself would delete what every reference now points at.
void checkReplacement(const std::string& id, const std::optional<std::string>& replacement) {
    if (replacement && *replacement == id) throw std::invalid_argument("EDIT_INVALID_REPLACEMENT");
}
template<class Road> void roadUsers(const std::vector<Road>& roads, const std::string& id, std::vector<std::string>& out) {
    for (const auto& r : roads) if (r.behaviourTypeId == id) out.push_back(r.id);
}
template<class Road> void reassignRoads(std::vector<Road>& roads, const std::string& from, const std::string& to) {
    for (auto& r : roads) if (r.behaviourTypeId == from) r.behaviourTypeId = to;
}
}
void putBehaviour(ProjectDocument& d, DriverBehaviour value, const std::string& name) {
    auto& def = owned(d);
    if (name.empty()) def.behaviourNames.erase(value.id); else def.behaviourNames[value.id] = name;
    put(def.behaviours, std::move(value));
}
void putVehicleClass(ProjectDocument& d, VehicleClass value) { put(owned(d).vehicleClasses, std::move(value)); }
void putLinkBehaviourType(ProjectDocument& d, LinkBehaviourType value) { put(owned(d).linkBehaviourTypes, std::move(value)); }
std::string duplicateBehaviour(ProjectDocument& d, const std::string& id) {
    auto& def = owned(d);
    auto copy = find(def.behaviours, id, "EDIT_UNKNOWN_BEHAVIOUR");
    copy.id = freshId(def.behaviours, id);
    if (const auto name = def.behaviourNames.find(id); name != def.behaviourNames.end()) def.behaviourNames[copy.id] = name->second;
    def.behaviours.push_back(copy);
    return copy.id;
}
std::string duplicateVehicleClass(ProjectDocument& d, const std::string& id) {
    auto& def = owned(d);
    auto copy = find(def.vehicleClasses, id, "EDIT_UNKNOWN_VEHICLE_CLASS");
    copy.id = freshId(def.vehicleClasses, id); copy.vehicleTypeIds.clear();
    def.vehicleClasses.push_back(copy);
    return copy.id;
}
std::string duplicateLinkBehaviourType(ProjectDocument& d, const std::string& id) {
    auto& def = owned(d);
    auto copy = find(def.linkBehaviourTypes, id, "EDIT_UNKNOWN_BEHAVIOUR_TYPE");
    copy.id = freshId(def.linkBehaviourTypes, id);
    def.linkBehaviourTypes.push_back(copy);
    return copy.id;
}
void assignBehaviourType(ProjectDocument& d, const std::string& roadId, std::optional<std::string> behaviourTypeId) {
    for (auto& l : d.network.links) if (l.id == roadId) { l.behaviourTypeId = std::move(behaviourTypeId); return; }
    for (auto& c : d.network.connectors) if (c.id == roadId) { c.behaviourTypeId = std::move(behaviourTypeId); return; }
    throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
LibraryUsers behaviourUsers(const ProjectDocument& d, const std::string& id) {
    LibraryUsers users;
    if (!d.definition) return users;
    for (const auto& t : d.definition->vehicleTypes) if (t.behaviourId == id) users.vehicleTypes.push_back(t.id);
    for (const auto& t : d.definition->linkBehaviourTypes)
        if (t.defaultBehaviourId == id || std::any_of(t.overrides.begin(), t.overrides.end(), [&](const auto& o) { return o.behaviourId == id; }))
            users.behaviourTypes.push_back(t.id);
    return users;
}
LibraryUsers vehicleClassUsers(const ProjectDocument& d, const std::string& id) {
    LibraryUsers users;
    if (!d.definition) return users;
    for (const auto& t : d.definition->linkBehaviourTypes)
        if (std::any_of(t.overrides.begin(), t.overrides.end(), [&](const auto& o) { return o.classId == id; }))
            users.behaviourTypes.push_back(t.id);
    return users;
}
LibraryUsers behaviourTypeUsers(const ProjectDocument& d, const std::string& id) {
    LibraryUsers users;
    roadUsers(d.network.links, id, users.roads);
    roadUsers(d.network.connectors, id, users.roads);
    return users;
}
void deleteBehaviour(ProjectDocument& d, const std::string& id, const std::optional<std::string>& replacement) {
    auto& def = owned(d); checkReplacement(id, replacement);
    find(def.behaviours, id, "EDIT_UNKNOWN_BEHAVIOUR");
    if (!behaviourUsers(d, id).empty()) {
        if (!replacement) throw std::invalid_argument("EDIT_REFERENCED_BEHAVIOUR");
        for (auto& t : def.vehicleTypes) if (t.behaviourId == id) t.behaviourId = *replacement;
        for (auto& t : def.linkBehaviourTypes) {
            if (t.defaultBehaviourId == id) t.defaultBehaviourId = *replacement;
            for (auto& o : t.overrides) if (o.behaviourId == id) o.behaviourId = *replacement;
        }
    }
    std::erase_if(def.behaviours, [&](const auto& b) { return b.id == id; });
    def.behaviourNames.erase(id);
}
void deleteVehicleClass(ProjectDocument& d, const std::string& id, const std::optional<std::string>& replacement) {
    auto& def = owned(d); checkReplacement(id, replacement);
    find(def.vehicleClasses, id, "EDIT_UNKNOWN_VEHICLE_CLASS");
    if (!vehicleClassUsers(d, id).empty()) {
        if (!replacement) throw std::invalid_argument("EDIT_REFERENCED_VEHICLE_CLASS");
        for (auto& t : def.linkBehaviourTypes) for (auto& o : t.overrides) if (o.classId == id) o.classId = *replacement;
    }
    std::erase_if(def.vehicleClasses, [&](const auto& c) { return c.id == id; });
}
void deleteLinkBehaviourType(ProjectDocument& d, const std::string& id, const std::optional<std::string>& replacement) {
    auto& def = owned(d); checkReplacement(id, replacement);
    find(def.linkBehaviourTypes, id, "EDIT_UNKNOWN_BEHAVIOUR_TYPE");
    if (!behaviourTypeUsers(d, id).empty()) {
        if (!replacement) throw std::invalid_argument("EDIT_REFERENCED_BEHAVIOUR_TYPE");
        reassignRoads(d.network.links, id, *replacement);
        reassignRoads(d.network.connectors, id, *replacement);
    }
    std::erase_if(def.linkBehaviourTypes, [&](const auto& t) { return t.id == id; });
}
}
