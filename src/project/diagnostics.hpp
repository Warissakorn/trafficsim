#pragma once
#include "document.hpp"
#include "../model/network/diagnostics.hpp"

namespace trafficsim {
// Draft issues first, then non-blocking runtime issues. Never throws: parse and compile
// failures come back as rows, so a diagnostics pass cannot take the editor down.
// A definition holding only run settings (a new project, M5.3): no route, input, decision or
// signal timing yet. Diagnostics treat it as a drawing, exactly like no definition (D132).
bool onlyRunSettings(const AuthoringDefinition&);
std::vector<Diagnostic> documentDiagnostics(const ProjectDocument& document);
}
