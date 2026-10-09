#include "run.hpp"
#include "input_manifest.hpp"
#include "diagnostics.hpp"
#include "behaviour_library.hpp"
#include "demand_paths.hpp"
#include "../core/validate.hpp"
namespace trafficsim {
std::vector<Diagnostic> runDiagnostics(const ProjectDocument& d, const std::filesystem::path& data) {
    // A new project carries run settings (M5.3) but no demand yet: it is still a drawing, and the
    // drawing's own problems are the useful list, exactly as with no definition at all.
    if (!d.definition || onlyRunSettings(*d.definition)) return documentDiagnostics(d);
    try {
        auto rows=runtimeDiagnostics(d.network,expandRouteless(d.network,*d.definition,resolveCatalogs(*d.definition,data)));
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
    // An input left out for an unknown or broken composition must stop Run, not vanish from it.
    if(auto issues=compositionIssues(*d.definition,data,manifest);!issues.empty())throw ValidationError(std::move(issues));
    // Resolve once: every part of this snapshot uses the same catalog values.
    if(auto issues=routelessIssues(d.network,*d.definition).blocking;!issues.empty())throw ValidationError(std::move(issues));
    auto definition=expandRouteless(d.network,*d.definition,resolveCatalogs(*d.definition,data,manifest));
    // M3.3.2b (D127): road assignments become per-segment selections; the library is authoring
    // data, sliced away by resolveCatalogs, so it is read from the document itself.
    definition.segmentBehaviours=compileBehaviourAssignments(d.network,*d.definition);
    if(manifest)manifest->validate();
    auto scenario=compileScenario(d.network,definition);
    // M5.9 (D146): the run goes on for the cool-down after the authored duration. Inputs still end
    // by that duration, so the engine releases no new demand in it; extended only here, after every
    // expansion that reads the authored duration, and for every caller (CLI, batch, editor).
    if(d.definition->evaluation)scenario.duration+=d.definition->evaluation->cooldown;
    return {d.revision,d.network,std::move(scenario)};
}
}
