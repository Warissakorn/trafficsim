#pragma once
#include "document.hpp"
#include <filesystem>
#include <map>
namespace trafficsim {
struct DemandCatalog {
    std::vector<VehicleType> vehicleTypes;
    std::vector<DriverBehaviour> behaviours;
    std::vector<Composition> compositions;
    std::map<std::string,std::string> vehicleTypeNames;
};
// Resolve authored ownership without expanding routes, lanes or compositions.
DemandCatalog resolveDemandCatalog(const AuthoringDefinition&,const std::filesystem::path&,
                                   bool includeCompositions=true);
// Validate every owned composition and input reference, including unused entries.
std::vector<ValidationIssue> ownedCatalogIssues(const AuthoringDefinition&);
}
