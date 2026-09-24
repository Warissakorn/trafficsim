#pragma once
#include "history.hpp"
namespace trafficsim {
AuthoringDefinition& demand(ProjectDocument&);
std::string putRoute(ProjectDocument&, Route); // Empty id allocates; same id replaces.
std::string putInput(ProjectDocument&, VehicleInput);
std::string putProgram(ProjectDocument&, SignalProgram);
void deleteRoute(ProjectDocument&, const std::string&); // Cascades its inputs and decision entries.
// M2.4. Empty id allocates; same id replaces. Validated with the rest of the demand on commit.
std::string putRoutingDecision(ProjectDocument&, RoutingDecision);
void deleteRoutingDecision(ProjectDocument&, const std::string&); // Cascades its inputs.
void deleteInput(ProjectDocument&, const std::string&);
void deleteProgram(ProjectDocument&, const std::string&); // Reject referenced programs.
void changeRunSettings(ProjectDocument&, double duration, double timeStep);
std::string putSignalHead(ProjectDocument&, NetworkSignalHead);
void deleteSignalHead(ProjectDocument&, const std::string&);
}
