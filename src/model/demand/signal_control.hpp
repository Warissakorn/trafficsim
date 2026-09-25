#pragma once
#include "definition.hpp"
#include "../network/network.hpp"

namespace trafficsim {
// M2.7b. The compile-time expansion of Signal Controllers into ordinary core SignalPrograms, and
// the checks an authored controller must pass first. Pure functions: no I/O, no Qt.

// The program a group expands to: green, amber, red, starting at greenStart, with an offset that
// puts the controller's cycle second back where it belongs. Zero-length phases are left out.
SignalProgram signalGroupProgram(const SignalController&, const SignalGroup&);
std::vector<SignalProgram> signalGroupPrograms(const std::vector<SignalController>&);
// The colour a group shows at simulation time t, straight from the authored numbers. Tests and
// the timing diagram read it; the run uses the expanded program.
SignalColor signalGroupColorAt(const SignalController&, const SignalGroup&, double t);
// Seconds of green, derived: (greenEnd - greenStart) mod cycle.
double signalGroupGreen(const SignalController&, const SignalGroup&);
// Timing, numbering and head references. Paths name the authored field
// (`signalControllers[i].groups[j].greenEnd`, `signalHeads[k].groupNumber`).
std::vector<ValidationIssue> signalControlIssues(const Network&, const AuthoringDefinition&);
}
