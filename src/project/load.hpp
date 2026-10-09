#pragma once
#include "../model/network/network.hpp"
#include <filesystem>
#include <stdexcept>

namespace trafficsim {
struct LoadedScenario { Network network; Scenario scenario; };
// Carries the file and a translatable code so the shell can say which kind of file this is
// rather than relaying a parser exception. An empty code means "detail only".
struct ScenarioLoadError : std::runtime_error {
    ScenarioLoadError(std::filesystem::path file, std::string code, const std::string& detail);
    std::filesystem::path file;
    std::string code;
    std::string detail;
};
// Loads the M0 authoring fixture and all JSON catalogs. This is not an M1 project editor.
LoadedScenario loadScenario(const std::filesystem::path& file, const std::filesystem::path& dataDirectory);
std::filesystem::path findDataDirectory(const std::filesystem::path& executable);
std::uint32_t parseSeed(const std::string& text);
// `42-51`, `1,5,9` or a mix (`1,3-4`), in the order written (M5.2). Rejects an empty item, a
// reversed range, a repeated seed and more than 1000 seeds, each as "Invalid seed list: ...".
std::vector<std::uint32_t> parseSeedList(const std::string& text);
}
