#include "run.hpp"
#include "input_manifest.hpp"
#include "diagnostics.hpp"
#include "behaviour_library.hpp"
#include "demand_paths.hpp"
#include "../core/validate.hpp"
namespace trafficsim {
namespace {
// M3.3.2a (D126): a road assignment has no runtime effect until M3.3.2b, so Run refuses it.
void appendAssignments(const Network& network, std::vector<Diagnostic>& rows) {
    for(const auto& issue:behaviourAssignmentIssues(network)) {
        const auto id=objectIdForPath(network,issue.path); // the row selects the assigned road
        rows.push_back({issue.code,issue.path,id,selectableFor(network,id),DiagnosticSeverity::runtime});
    }
}
}
std::vector<Diagnostic> runDiagnostics(const ProjectDocument& d, const std::filesystem::path& data) {
    if (!d.definition) { auto rows=documentDiagnostics(d); appendAssignments(d.network,rows); return rows; }
    try {
        auto rows=runtimeDiagnostics(d.network,expandRouteless(d.network,*d.definition,resolveCatalogs(*d.definition,data)));
        appendAssignments(d.network,rows);
        for(const auto& issue:demandAdvisories(d.network,*d.definition))
            rows.push_back({issue.code,issue.path,{},{},DiagnosticSeverity::advisory});
        // M2.1.1: a routeless walk that failed blocks Run; a lane a decision cannot serve advises.
        const auto routeless=routelessIssues(d.network,*d.definition);
        for(const auto& issue:routeless.blocking)rows.push_back({issue.code,issue.path,{},{},DiagnosticSeverity::runtime});
        for(const auto& issue:routeless.advisory)rows.push_back({issue.code,issue.path,{},{},DiagnosticSeverity::advisory});
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
RunSnapshot compileDocument(const ProjectDocument& d, const std::filesystem::path& data,InputManifest* manifest) {
    validateDocument(d);
    if (!d.definition) throw std::invalid_argument("EDIT_NO_DEFINITION");
    if (d.definition->inputs.empty()) throw std::invalid_argument("EDIT_NO_INPUTS");
    if(auto issues=behaviourAssignmentIssues(d.network);!issues.empty())throw ValidationError(std::move(issues));
    // An input left out for an unknown or broken composition must stop Run, not vanish from it.
    if(auto issues=compositionIssues(*d.definition,data,manifest);!issues.empty())throw ValidationError(std::move(issues));
    // Resolve once: every part of this snapshot uses the same catalog values.
    if(auto issues=routelessIssues(d.network,*d.definition).blocking;!issues.empty())throw ValidationError(std::move(issues));
    const auto definition=expandRouteless(d.network,*d.definition,resolveCatalogs(*d.definition,data,manifest));
    if(manifest)manifest->validate();
    return {d.revision,d.network,compileScenario(d.network,definition)};
}
}
