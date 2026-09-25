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
// M2.7b. Empty id allocates; same id replaces. Removing a group a head still shows is refused
// on commit (UNKNOWN_SIGNAL_GROUP); deleting a controller a head shows, here.
std::string putSignalController(ProjectDocument&, SignalController);
void deleteSignalController(ProjectDocument&, const std::string&);
void changeRunSettings(ProjectDocument&, double duration, double timeStep);
std::string putSignalHead(ProjectDocument&, NetworkSignalHead);
void deleteSignalHead(ProjectDocument&, const std::string&);
// Slides a head -- its stop line -- along the lane or path it stands on. The station is
// validated with the network on commit (INVALID_POSITION), never clamped here.
void moveSignalHead(ProjectDocument&, const std::string& id, double position);
}
