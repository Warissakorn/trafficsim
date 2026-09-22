#pragma once
#include "history.hpp"
namespace trafficsim {
// Vissim's Name, on any Link, Connector or signal head. Free text, never a key: two objects
// may carry the same name, and an empty one is the normal state.
void renameObject(ProjectDocument&,const std::string& id,const std::string& name);
void changeAppearance(ProjectDocument&,const std::string&,int level,const std::string& displayType);
// Move a whole selection rigidly, the way Vissim's group drag does. Links in the selection
// carry their geometry; a Connector moves when explicitly selected or both its Links move.
// Other Connectors retain their world positions and re-read their attachment stations.
// Signal heads ride a station and need no moving at all.
void translateObjects(ProjectDocument&,const std::vector<std::string>&,Point offset);
// Rotate the same geometry around a fixed world pivot. Detached Connectors and their
// dependants are removed by the usual M1.20 rule, within the caller's History transaction.
void rotateObjects(ProjectDocument&,const std::vector<std::string>&,Point pivot,double degrees);
std::vector<std::string> duplicateObjects(ProjectDocument&,const std::vector<std::string>&,Point offset);
}
