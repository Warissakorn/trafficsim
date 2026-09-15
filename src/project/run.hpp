#pragma once
#include "document.hpp"
#include "../model/network/diagnostics.hpp"
#include <filesystem>
namespace trafficsim {
ScenarioDefinition resolveCatalogs(const AuthoringDefinition&, const std::filesystem::path& dataDirectory);
struct RunSnapshot {
    std::uint64_t revision{};
    Network network;
    Scenario scenario;
};
std::vector<Diagnostic> runDiagnostics(const ProjectDocument&, const std::filesystem::path&);
RunSnapshot compileDocument(const ProjectDocument&, const std::filesystem::path&);
}
