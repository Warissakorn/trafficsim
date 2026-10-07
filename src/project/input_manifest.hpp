#pragma once
#include "json.hpp"
#include <filesystem>
#include <map>
#include <set>
#include <string_view>

namespace trafficsim {
// SHA-256 of exact file bytes, not normalized/re-serialized JSON.
std::string inputSha256(std::string_view bytes);
class InputManifest {
public:
    void record(const std::string& logicalPath,std::string_view bytes);
    void fallback(const std::string& scope);
    void validate() const; // changed bytes on a repeated logical input reject the run
    Json json() const;
private:
    struct Entry {std::string sha256;std::size_t bytes{},reads{};};
    std::map<std::string,Entry> entries_;
    std::set<std::string> fallbacks_;
    bool changed_{};
};
// Hash and parse the SAME captured binary bytes; null manifest keeps legacy stream parsing.
Json readInputJson(const std::filesystem::path&,const std::string& logicalPath,InputManifest* =nullptr);
}
