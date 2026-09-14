#pragma once
#include "network.hpp"

namespace trafficsim {
// Draft issues reject an edit; runtime issues describe what the M0 compiler cannot run yet.
// The distinction is the point: a drawing can be perfectly valid and still not be runnable.
enum class DiagnosticSeverity { draft, runtime };
struct Diagnostic {
    std::string code;      // ValidationIssue::code, translated through data/locales
    std::string path;      // ValidationIssue::path, kept verbatim as the precise locator
    std::string objectId;  // link / lane / connector / head id, empty when network-wide
    std::string selectId;  // link or connector the canvas can select, empty when not selectable
    DiagnosticSeverity severity{DiagnosticSeverity::draft};
    bool operator==(const Diagnostic&) const = default;
};
// An empty network is a legal draft state, not an error. One place decides this.
bool blocksDraft(const std::string& code);
// "links[3].lanes[1].width" -> the lane id; "connectors[0].from" -> the connector id.
// Index paths go stale across edits, so the id is derived on read and never stored.
std::string objectIdForPath(const Network&, const std::string& path);
// A runtime segment id is a lane id or a connector id; returns what the canvas can select.
std::string selectableFor(const Network&, const std::string& objectId);
// Both never throw: a diagnostics pass must not be able to take the editor down.
std::vector<Diagnostic> networkDiagnostics(const Network&);
std::vector<Diagnostic> runtimeDiagnostics(const Network&, const ScenarioDefinition&);
}
