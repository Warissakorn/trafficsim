#include "catalog_commands.hpp"
#include "demand_commands.hpp"
namespace trafficsim {
void putDemandCatalog(ProjectDocument& d,DemandCatalog catalog) {
    auto& target=demand(d);
    target.vehicleTypes=std::move(catalog.vehicleTypes);target.behaviours=std::move(catalog.behaviours);
    target.compositions=std::move(catalog.compositions);target.vehicleTypeNames=std::move(catalog.vehicleTypeNames);
    target.externalVehicleTypes=false;target.externalBehaviours=false;target.externalCompositions=false;
}
}
