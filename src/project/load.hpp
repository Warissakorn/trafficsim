#pragma once
#include "../model/network/network.hpp"
#include <filesystem>

namespace trafficsim {
struct LoadedScenario { Network network; Scenario scenario; };
// Loads the M0 authoring fixture and all JSON catalogs. This is not an M1 project editor.
LoadedScenario loadScenario(const std::filesystem::path& file, const std::filesystem::path& dataDirectory);
std::filesystem::path findDataDirectory(const std::filesystem::path& executable);
std::uint32_t parseSeed(const std::string& text);
}
