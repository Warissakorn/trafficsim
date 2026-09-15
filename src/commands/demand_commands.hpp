#pragma once
#include "history.hpp"
namespace trafficsim {
AuthoringDefinition& demand(ProjectDocument&);
std::string putRoute(ProjectDocument&, Route); // Empty id allocates; same id replaces.
std::string putInput(ProjectDocument&, VehicleInput);
std::string putProgram(ProjectDocument&, SignalProgram);
void deleteRoute(ProjectDocument&, const std::string&); // Cascades its inputs.
void deleteInput(ProjectDocument&, const std::string&);
void deleteProgram(ProjectDocument&, const std::string&); // Reject referenced programs.
void changeRunSettings(ProjectDocument&, double duration, double timeStep);
std::string putSignalHead(ProjectDocument&, NetworkSignalHead);
void deleteSignalHead(ProjectDocument&, const std::string&);
}
