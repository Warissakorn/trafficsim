#pragma once
#include "history.hpp"
namespace trafficsim {
void changeAppearance(ProjectDocument&,const std::string&,int level,const std::string& displayType);
std::vector<std::string> duplicateObjects(ProjectDocument&,const std::vector<std::string>&,Point offset);
}
