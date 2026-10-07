#pragma once
#include "history.hpp"
namespace trafficsim {
// M3.3.2a (D126, DRIVING_BEHAVIOUR.md §1): the project-owned behaviour library. Every function is
// one part of a History transaction; History validates the result, so a bad reference rejects the
// whole edit. Library edits need owned behaviour and vehicle-type catalogs (EDIT_EXTERNAL_CATALOG);
// capture them first with putDemandCatalog.
// Create, or replace by id. An empty name removes the display name.
void putBehaviour(ProjectDocument&, DriverBehaviour, const std::string& name);
void putVehicleClass(ProjectDocument&, VehicleClass);
void putLinkBehaviourType(ProjectDocument&, LinkBehaviourType);
// An independent copy under a fresh id ("<id>-copy", "<id>-copy-2", ...), which is returned.
// A class copy has no members: a vehicle type belongs to at most one class.
std::string duplicateBehaviour(ProjectDocument&, const std::string& id);
std::string duplicateVehicleClass(ProjectDocument&, const std::string& id);
std::string duplicateLinkBehaviourType(ProjectDocument&, const std::string& id);
// Sets or clears the behaviour type of the Link or Connector with this id.
void assignBehaviourType(ProjectDocument&, const std::string& roadId, std::optional<std::string> behaviourTypeId);
// Who refers to an entry, for showing users before a shared edit or a delete.
struct LibraryUsers {
    std::vector<std::string> vehicleTypes, behaviourTypes, roads;
    bool empty() const { return vehicleTypes.empty() && behaviourTypes.empty() && roads.empty(); }
    bool operator==(const LibraryUsers&) const = default;
};
LibraryUsers behaviourUsers(const ProjectDocument&, const std::string& behaviourId);
LibraryUsers vehicleClassUsers(const ProjectDocument&, const std::string& classId);
LibraryUsers behaviourTypeUsers(const ProjectDocument&, const std::string& behaviourTypeId);
// A referenced entry is deleted only with an explicit replacement, which every reference is
// rewritten to in the same transaction (EDIT_REFERENCED_BEHAVIOUR / _VEHICLE_CLASS / _BEHAVIOUR_TYPE).
void deleteBehaviour(ProjectDocument&, const std::string& id, const std::optional<std::string>& replacement = {});
void deleteVehicleClass(ProjectDocument&, const std::string& id, const std::optional<std::string>& replacement = {});
void deleteLinkBehaviourType(ProjectDocument&, const std::string& id, const std::optional<std::string>& replacement = {});
}
