#include "signal_control.hpp"
#include <cmath>
#include <set>

namespace trafficsim {
namespace {
double wrap(double value, double cycle) {
    double r = std::fmod(value, cycle);
    if (r < 0) r += cycle;
    // A value a rounding error short of the cycle is the cycle's start.
    return cycle - r < 1e-9 ? 0 : r;
}
bool onGrid(double value, double step) {
    if (!(step > 0)) return true;
    const double k = value / step;
    return std::abs(k - std::round(k)) < 1e-6;
}
}
double signalGroupGreen(const SignalController& c, const SignalGroup& g) {
    return wrap(g.greenEnd - g.greenStart, c.cycle);
}
SignalProgram signalGroupProgram(const SignalController& c, const SignalGroup& g) {
    SignalProgram program{signalGroupProgramId(c.id, g.number), wrap(c.offset - g.greenStart, c.cycle), {}};
    const double green = signalGroupGreen(c, g);
    const double red = c.cycle - green - g.amber;
    if (green > 0) program.phases.push_back({green, SignalColor::green});
    if (g.amber > 0) program.phases.push_back({g.amber, SignalColor::amber});
    if (red > 1e-9) program.phases.push_back({red, SignalColor::red});
    return program;
}
std::vector<SignalProgram> signalGroupPrograms(const std::vector<SignalController>& controllers) {
    std::vector<SignalProgram> programs;
    for (const auto& c : controllers) for (const auto& g : c.groups) programs.push_back(signalGroupProgram(c, g));
    return programs;
}
SignalColor signalGroupColorAt(const SignalController& c, const SignalGroup& g, double t) {
    const double since = wrap(wrap(t + c.offset, c.cycle) - g.greenStart, c.cycle);
    const double green = signalGroupGreen(c, g);
    if (since < green - 1e-9) return SignalColor::green;
    if (since < green + g.amber - 1e-9) return SignalColor::amber;
    return SignalColor::red;
}
std::vector<ValidationIssue> signalControlIssues(const Network& network, const AuthoringDefinition& d) {
    std::vector<ValidationIssue> issues;
    const auto add = [&](const char* code, std::string path) { issues.push_back({code, std::move(path)}); };
    std::set<std::string> ids;
    for (std::size_t i = 0; i < d.signalControllers.size(); ++i) {
        const auto& c = d.signalControllers[i];
        const auto p = "signalControllers[" + std::to_string(i) + "]";
        if (c.id.find_first_not_of(" \t\r\n") == std::string::npos || !ids.insert(c.id).second) add("INVALID_ID", p + ".id");
        const bool cycleOk = std::isfinite(c.cycle) && c.cycle > 0 && onGrid(c.cycle, d.timeStep);
        if (!cycleOk) { add("INVALID_SIGNAL_TIMING", p + ".cycle"); continue; }
        if (!std::isfinite(c.offset) || c.offset < 0 || c.offset >= c.cycle || !onGrid(c.offset, d.timeStep))
            add("INVALID_SIGNAL_TIMING", p + ".offset");
        if (c.groups.empty()) add("EMPTY_SIGNAL_CONTROLLER", p + ".groups");
        std::set<int> numbers;
        for (std::size_t j = 0; j < c.groups.size(); ++j) {
            const auto& g = c.groups[j];
            const auto q = p + ".groups[" + std::to_string(j) + "]";
            if (g.number <= 0 || !numbers.insert(g.number).second) add("DUPLICATE_SIGNAL_GROUP", q + ".number");
            const auto second = [&](double v) { return std::isfinite(v) && v >= 0 && v < c.cycle && onGrid(v, d.timeStep); };
            if (!second(g.greenStart)) add("INVALID_SIGNAL_TIMING", q + ".greenStart");
            if (!second(g.greenEnd) || g.greenEnd == g.greenStart) add("INVALID_SIGNAL_TIMING", q + ".greenEnd");
            else if (!std::isfinite(g.amber) || g.amber < 0 || !onGrid(g.amber, d.timeStep) ||
                     signalGroupGreen(c, g) + g.amber > c.cycle + 1e-9)
                add("INVALID_SIGNAL_TIMING", q + ".amber");
        }
    }
    for (std::size_t k = 0; k < network.signalHeads.size(); ++k) {
        const auto& h = network.signalHeads[k];
        if (h.controllerId.empty()) continue;
        const auto p = "signalHeads[" + std::to_string(k) + "]";
        bool found = false;
        for (const auto& c : d.signalControllers) if (c.id == h.controllerId)
            for (const auto& g : c.groups) found = found || g.number == h.groupNumber;
        if (!found) add("UNKNOWN_SIGNAL_GROUP", p + ".groupNumber");
    }
    return issues;
}
}
