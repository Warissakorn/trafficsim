#pragma once
#include "history.hpp"
namespace trafficsim {
// Vissim's Name, on any Link, Connector or signal head. Free text, never a key: two objects
// may carry the same name, and an empty one is the normal state.
void renameObject(ProjectDocument&,const std::string& id,const std::string& name);
void changeAppearance(ProjectDocument&,const std::string&,int level,const std::string& displayType);
// Move a whole selection rigidly, the way Vissim's group drag does. Links in the selection
// carry their geometry; a Connector moves with its two Links when both are in the set, and
// otherwise stays attached where it is, because its ends belong to Links that did not move.
// Signal heads ride a station and need no moving at all.
void translateObjects(ProjectDocument&,const std::vector<std::string>&,Point offset);
std::vector<std::string> duplicateObjects(ProjectDocument&,const std::vector<std::string>&,Point offset);
}
