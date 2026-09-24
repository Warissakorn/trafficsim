#pragma once
#include "document.hpp"
#include "../model/network/routeless.hpp"

namespace trafficsim {
// M2.1.1 glue between the authored demand and the model's routeless walk.
// The decisions that are placed on a Link, with each destination resolved to the object chain a
// route would name. Network-dependent problems (unknown Link, a destination no chain reaches or
// two chains reach equally) go to `issues` when given; that decision entry is left out.
std::vector<PlacedDecision> placedDecisions(const Network&, const AuthoringDefinition&,
                                            std::vector<ValidationIssue>* issues = nullptr);
// Every input with a linkId (or naming a placed decision, which withRoutingDecisions turns into
// one) becomes one input per complete path, each on an already-expanded runtime route
// `link:<id>/path-k` that buildScenario passes through. An input whose walk has a problem is left
// out -- routelessIssues names it and Run refuses, while an edit still goes through.
ScenarioDefinition expandRouteless(const Network&, const AuthoringDefinition& authored,
                                   ScenarioDefinition resolved);
struct RoutelessIssues { std::vector<ValidationIssue> blocking, advisory; };
RoutelessIssues routelessIssues(const Network&, const AuthoringDefinition&);
// The runtime route id for path k of n from a Link.
std::string routelessRouteId(const std::string& linkId, std::size_t k, std::size_t n);
}
