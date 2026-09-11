#include "simulation.hpp"
#include <cmath>
#include <stdexcept>

namespace trafficsim {
SignalColor signalColorAt(const SignalProgram& program, double time) {
    if (program.phases.empty()) throw std::invalid_argument("EMPTY_SIGNAL_PROGRAM");
    double cycle = 0;
    for (const auto& phase : program.phases) cycle += phase.duration;
    double position = std::fmod(std::fmod(time + program.offset, cycle) + cycle, cycle);
    if (cycle - position < 1e-9) position = 0;
    for (const auto& phase : program.phases) {
        if (position < phase.duration - 1e-9) return phase.color;
        position -= phase.duration;
    }
    return program.phases.front().color;
}
}
