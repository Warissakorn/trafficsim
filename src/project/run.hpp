#pragma once
#include "document.hpp"
#include "../model/network/diagnostics.hpp"
#include <filesystem>
namespace trafficsim {
// Also expands every input that names a composition (M2.3) into one input per vehicle type,
// `id/type-<type>` at volume x normalised share -- a single-type composition keeps the plain id.
// An input whose composition is unknown or broken is left out; compositionIssues names it.
ScenarioDefinition resolveCatalogs(const AuthoringDefinition&, const std::filesystem::path& dataDirectory);
std::vector<Composition> loadCompositions(const std::filesystem::path& dataDirectory);
// UNKNOWN_COMPOSITION at inputs[i].compositionId, and for a composition an input uses:
// INVALID_SHARE (no types, or a share not positive and finite) or UNKNOWN_VEHICLE_TYPE.
std::vector<ValidationIssue> compositionIssues(const AuthoringDefinition&, const std::filesystem::path& dataDirectory);
struct RunSnapshot {
    std::uint64_t revision{};
    Network network;
    Scenario scenario;
};
std::vector<Diagnostic> runDiagnostics(const ProjectDocument&, const std::filesystem::path&);
RunSnapshot compileDocument(const ProjectDocument&, const std::filesystem::path&);
}
