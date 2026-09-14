#pragma once
#include "document.hpp"
#include "../model/network/diagnostics.hpp"

namespace trafficsim {
// Draft issues first, then non-blocking runtime issues. Never throws: parse and compile
// failures come back as rows, so a diagnostics pass cannot take the editor down.
std::vector<Diagnostic> documentDiagnostics(const ProjectDocument& document);
}
