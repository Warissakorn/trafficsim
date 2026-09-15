#include "display.hpp"
#include "json.hpp"
#include <algorithm>
#include <fstream>
#include <set>
namespace trafficsim {
namespace {
std::vector<Json> files(const std::filesystem::path& path) {
    std::vector<std::filesystem::path> names;
    for(const auto& entry:std::filesystem::directory_iterator(path))
        if(entry.is_regular_file() && entry.path().extension()==".json")names.push_back(entry.path());
    std::sort(names.begin(),names.end());std::vector<Json> result;
    for(const auto& name:names){std::ifstream input(name);if(!input)throw std::runtime_error("EDIT_DISPLAY_CATALOG");result.push_back(Json::parse(input));}
    return result;
}
std::string color(const Json& j,const char* key) {
    const auto c=j.at(key).get<std::string>();
    if(c.size()!=7 || c[0]!='#' || c.find_first_not_of("0123456789abcdefABCDEF",1)!=std::string::npos)
        throw std::runtime_error("EDIT_DISPLAY_CATALOG");
    return c;
}
std::map<std::string,std::string> name(const Json& j) {
    std::map<std::string,std::string> result;
    for(const auto* code:{"en","th"}){
        const auto value=j.at("name").at(code).get<std::string>();if(value.empty())throw std::runtime_error("EDIT_DISPLAY_CATALOG");
        result[code]=value;
    }
    return result;
}
}
DisplayCatalog loadDisplayCatalog(const std::filesystem::path& data) {
    try {
        DisplayCatalog catalog;std::set<int> orders;std::set<std::string> ids;
        for(const auto& j:files(data/"levels")) {
            if(!j.at("order").is_number_integer() || j.at("order") < -1000 || j.at("order") > 1000)throw std::runtime_error("EDIT_DISPLAY_CATALOG");
            const int order=j.at("order").get<int>();if(!orders.insert(order).second)throw std::runtime_error("EDIT_DISPLAY_CATALOG");
            catalog.levels.push_back({order,name(j)});
        }
        std::sort(catalog.levels.begin(),catalog.levels.end(),[](const auto& a,const auto& b){return a.order<b.order;});
        for(const auto& j:files(data/"display-types")) {
            const auto id=j.at("id").get<std::string>();
            if(id.empty() || !ids.insert(id).second)throw std::runtime_error("EDIT_DISPLAY_CATALOG");
            catalog.types.push_back({id,name(j),color(j,"linkColor"),color(j,"connectorColor"),color(j,"laneColor"),color(j,"vehicleColor")});
        }
        if(!orders.contains(0) || !ids.contains("default"))throw std::runtime_error("EDIT_DISPLAY_CATALOG");
        return catalog;
    }catch(const std::exception&){throw std::runtime_error("EDIT_DISPLAY_CATALOG");}
}
}
