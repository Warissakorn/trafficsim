#include "run.hpp"
#include "diagnostics.hpp"
#include "../core/validate.hpp"
namespace trafficsim {
std::vector<Diagnostic> runDiagnostics(const ProjectDocument& d, const std::filesystem::path& data) {
    if (!d.definition) return documentDiagnostics(d);
    try {
        auto rows=runtimeDiagnostics(d.network,resolveCatalogs(*d.definition,data));
        // A route is not a network object, and neither is an input: the row names its path.
        for(const auto& issue:compositionIssues(*d.definition,data))
            rows.push_back({issue.code,issue.path,{},{},DiagnosticSeverity::runtime});
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
    // An input left out for an unknown or broken composition must stop Run, not vanish from it.
    if(auto issues=compositionIssues(*d.definition,data);!issues.empty())throw ValidationError(std::move(issues));
    // Resolve once: every part of this snapshot uses the same catalog values.
    const auto definition=resolveCatalogs(*d.definition,data);
    return {d.revision,d.network,compileScenario(d.network,definition)};
}
}
