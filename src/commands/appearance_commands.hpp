#pragma once
#include "history.hpp"
namespace trafficsim {
// Vissim's Name, on any Link, Connector or signal head. Free text, never a key: two objects
// may carry the same name, and an empty one is the normal state.
void renameObject(ProjectDocument&,const std::string& id,const std::string& name);
void changeAppearance(ProjectDocument&,const std::string&,int level,const std::string& displayType);
std::vector<std::string> duplicateObjects(ProjectDocument&,const std::vector<std::string>&,Point offset);
}
