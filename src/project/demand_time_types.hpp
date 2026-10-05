#pragma once
#include "demand_catalog.hpp"
namespace trafficsim {
// Authoring-only rules; compiler emits ordinary independent Poisson streams. No new RNG.
const std::vector<CompositionShare>& compositionTypesAt(const Composition&,double scheduledTime);
std::vector<ValidationIssue> timeTypeIssues(const AuthoringDefinition&);
bool hasTypeRouting(const AuthoringDefinition&);
bool hasTimeTypeDemand(const AuthoringDefinition&);
// Empty type or no matching override uses existing default weights, including gaps/zero fallback.
double typeDecisionFlowAt(const RoutingDecision&,std::size_t route,double scheduledTime,const std::string& type);
// Fixed catalogs retain the previous split exactly. Timed ones cut only active input periods.
std::vector<VehicleInput> expandCompositions(const std::vector<VehicleInput>&,const std::vector<Composition>&);
}
