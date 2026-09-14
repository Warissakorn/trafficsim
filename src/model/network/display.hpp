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
struct DisplayCatalog {std::vector<DisplayLevel> levels;std::vector<DisplayType> types;};
}
