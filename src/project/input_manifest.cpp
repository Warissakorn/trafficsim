#include "input_manifest.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace trafficsim {
void InputManifest::record(const std::string& name,std::string_view bytes) {
    const auto digest=inputSha256(bytes);
    const auto [it,added]=entries_.try_emplace(name,Entry{digest,bytes.size(),0});
    if(!added&&(it->second.sha256!=digest||it->second.bytes!=bytes.size())) {
        changed_=true;throw std::runtime_error("Input changed during compilation: "+name);
    }
    ++it->second.reads;
}
void InputManifest::fallback(const std::string& scope) {fallbacks_.insert(scope);}
void InputManifest::validate() const {
    if(changed_)throw std::runtime_error("Input changed during compilation");
}
Json InputManifest::json() const {
    validate();Json files=Json::array();
    for(const auto& [name,e]:entries_)files.push_back({{"logicalPath",name},{"sha256",e.sha256},
                                                     {"bytes",e.bytes},{"reads",e.reads}});
    return {{"schemaVersion",1},{"algorithm","SHA-256"},
        {"coverage","exact bytes parsed from project, catalog and evaluation files actually read"},
        {"files",files},{"fallbacks",fallbacks_}};
}
Json readInputJson(const std::filesystem::path& path,const std::string& name,InputManifest* manifest) {
    if(!manifest) {
        std::ifstream stream(path);
        if(!stream)throw std::runtime_error("Cannot read JSON: "+path.string());
        return Json::parse(stream);
    }
    std::ifstream stream(path,std::ios::binary);
    if(!stream)throw std::runtime_error("Cannot read JSON: "+path.string());
    const std::string bytes{std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()};
    if(stream.bad())throw std::runtime_error("Failed reading JSON: "+path.string());
    manifest->record(name,bytes);
    return Json::parse(bytes);
}
}
