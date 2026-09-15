#include "diagnostics.hpp"

namespace trafficsim {
namespace {
// A drawing with no authored demand can still be judged on its topology: the merge and
// reachability checks read segments alone. Demand findings against a probe definition would
// only report the absence we already report once, so they are dropped rather than shown.
bool aboutTopology(const Diagnostic& row) {
    return row.path.rfind("segments", 0) == 0 || row.code.rfind("EDIT_", 0) == 0 || row.code=="UNSUPPORTED_CONNECTOR_POSITION";
}
// Vehicle types and driver behaviours are catalog content (data/), not document content, so a
// document alone cannot resolve them. Claiming they are unknown would blame the drawing for an
// absence that is by design; resolving catalogs at run handoff is M1.7's job.
bool needsCatalog(const std::string& code) {
    return code == "UNKNOWN_VEHICLE_TYPE" || code == "UNKNOWN_BEHAVIOUR";
}
}
std::vector<Diagnostic> documentDiagnostics(const ProjectDocument& document) {
    std::vector<Diagnostic> result;
    try {
        bool blocked = false;
        for (auto& row : networkDiagnostics(document.network)) {
            // An empty network is nothing authored yet, not a fault in what was authored.
            if (!blocksDraft(row.code)) row.severity = DiagnosticSeverity::runtime;
            else blocked = true;
            result.push_back(std::move(row));
        }
        if (blocked) {
            // Nothing compilable exists yet, and saying so beats implying a clean runtime check.
            result.push_back({"EDIT_RUNTIME_SKIPPED", {}, {}, {}, DiagnosticSeverity::runtime});
            return result;
        }
        if (!document.definition) {
            result.push_back({"EDIT_NO_DEFINITION", "definition", {}, {}, DiagnosticSeverity::runtime});
            // Compiling against a default ScenarioDefinition reports INVALID_NUMBER for timeStep
            // and duration, so the probe carries usable values and only topology rows are kept.
            ScenarioDefinition probe; probe.timeStep = 0.1; probe.duration = 1;
            for (auto& row : runtimeDiagnostics(document.network, probe))
                if (aboutTopology(row)) result.push_back(std::move(row));
            return result;
        }
        const ScenarioDefinition& definition = *document.definition;
        const bool catalogs = !definition.vehicleTypes.empty() && !definition.behaviours.empty();
        bool withheld = false;
        for (auto& row : runtimeDiagnostics(document.network, definition)) {
            if (!catalogs && needsCatalog(row.code)) { withheld = true; continue; }
            result.push_back(std::move(row));
        }
        if (withheld) result.push_back({"EDIT_NO_CATALOG", "definition", {}, {}, DiagnosticSeverity::runtime});
    } catch (const std::exception& error) {
        result.push_back({error.what(), {}, {}, {}, DiagnosticSeverity::runtime});
    }
    return result;
}
}
