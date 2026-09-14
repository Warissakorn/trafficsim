#include "run.hpp"
#include "../core/validate.hpp"
namespace trafficsim {
std::vector<Diagnostic> runDiagnostics(const ProjectDocument& d, const std::filesystem::path& data) {
    if (!d.definition) return {{"EDIT_NO_DEFINITION","definition",{},{},DiagnosticSeverity::runtime}};
    try {
        auto rows=runtimeDiagnostics(d.network,resolveCatalogs(*d.definition,data));
        if (d.definition->inputs.empty())
            rows.push_back({"EDIT_NO_INPUTS","inputs",{},{},DiagnosticSeverity::runtime});
        return rows;
    } catch (const std::exception& e) {
        return {{e.what(),"catalogs",{},{},DiagnosticSeverity::runtime}};
    }
}
RunSnapshot compileDocument(const ProjectDocument& d, const std::filesystem::path& data) {
    validateDocument(d);
    if (!d.definition) throw std::invalid_argument("EDIT_NO_DEFINITION");
    if (d.definition->inputs.empty()) throw std::invalid_argument("EDIT_NO_INPUTS");
    // Resolve once: every part of this snapshot uses the same catalog values.
    const auto definition=resolveCatalogs(*d.definition,data);
    return {d.revision,d.network,compileScenario(d.network,definition)};
}
}
