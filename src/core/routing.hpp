#pragma once
#include "routes.hpp"

namespace trafficsim {
// Validation is shared by direct scenarios and the project compiler.
std::vector<ValidationIssue> routingIssues(const Scenario&);
// The nearest unresolved station on this route; absent leaves legacy motion untouched.
const RouteDecision* nextRouteDecision(const Scenario&,const Vehicle&);
// Called at a reached station, never before it. Uses the seeded engine RNG once per selection.
void applyRouteDecision(const Scenario&,Vehicle&,const RouteDecision&,double time,
                        std::uint32_t& randomState,std::vector<SimEvent>&);
}
