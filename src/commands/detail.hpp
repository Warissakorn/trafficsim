#pragma once
#include "../project/document.hpp"
#include <set>

namespace trafficsim::detail {
void removeRoutesUsingSegments(ProjectDocument&, const std::set<std::string>& segments);
// M2.4. After any route removal: drop decision entries for missing routes, then decisions left
// empty and the inputs that named them. The one place both removal paths share.
// M2.1.1: also drops destination entries, placed decisions and routeless inputs whose Link is gone.
void pruneRoutingDecisions(AuthoringDefinition&, const Network&);
}
