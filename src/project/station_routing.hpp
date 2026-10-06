#pragma once
#include "demand_paths.hpp"

namespace trafficsim {
struct StationRoute { Route route; std::vector<FamilyTag> families; };
// Converts physical stations to route distances and clips awareness-dependent lateral spans.
void appendStationRouting(const Network&,const AuthoringDefinition&,const std::string& type,
                          const std::vector<StationRoute>&,ScenarioDefinition&);
}
