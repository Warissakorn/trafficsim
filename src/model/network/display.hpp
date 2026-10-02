#pragma once
#include <map>
#include <string>
#include <vector>
namespace trafficsim {
struct DisplayLevel {int order{};std::map<std::string,std::string> name;};
struct DisplayType {
    std::string id;
    std::map<std::string,std::string> name;
    std::string linkColor,connectorColor,laneColor,vehicleColor;
};
// vehicleColors: vehicle-type id -> #rrggbb, from data/vehicle-appearance (D97). A type with no
// entry is drawn in its segment's DisplayType::vehicleColor.
struct DisplayCatalog {
    std::vector<DisplayLevel> levels;std::vector<DisplayType> types;
    std::map<std::string,std::string> vehicleColors;
};
}
