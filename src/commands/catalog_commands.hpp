#pragma once
#include "../project/demand_catalog.hpp"
namespace trafficsim {
// One atomic ownership capture. History validates the staged result before committing.
void putDemandCatalog(ProjectDocument&,DemandCatalog);
}
