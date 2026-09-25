#include "document.hpp"
#include <cmath>
#include <map>
#include <optional>

namespace trafficsim {
namespace {
struct Run { SignalColor color; double start, duration; };
// A legacy program's colours as runs round its cycle: adjacent phases of one colour are one run,
// and the last and first merge when they share a colour, because the cycle wraps.
std::vector<Run> cyclicRuns(const SignalProgram& p) {
    std::vector<Run> runs; double t = 0;
    for (const auto& phase : p.phases) {
        if (!runs.empty() && runs.back().color == phase.color) runs.back().duration += phase.duration;
        else runs.push_back({phase.color, t, phase.duration});
        t += phase.duration;
    }
    if (runs.size() > 1 && runs.front().color == runs.back().color) {
        runs.back().duration += runs.front().duration;
        runs.erase(runs.begin());
    }
    return runs;
}
bool onGrid(double value, double step) {
    const double k = value / step;
    return std::abs(k - std::round(k)) < 1e-6;
}
double wrap(double value, double cycle) {
    double r = std::fmod(value, cycle);
    if (r < 0) r += cycle;
    return cycle - r < 1e-9 ? 0 : r;
}
struct Candidate { const SignalProgram* program; double cycle, greenStart, green, amber; };
// One green, then optionally amber, then optionally red: the only shape a signal group has.
std::optional<Candidate> asGroup(const SignalProgram& p, double step) {
    double cycle = 0;
    for (const auto& phase : p.phases) {
        if (!(phase.duration > 0) || !std::isfinite(phase.duration) || !onGrid(phase.duration, step)) return {};
        cycle += phase.duration;
    }
    if (!(cycle > 0) || !std::isfinite(p.offset) || !onGrid(p.offset, step)) return {};
    const auto runs = cyclicRuns(p);
    std::size_t green = runs.size();
    for (std::size_t i = 0; i < runs.size(); ++i) if (runs[i].color == SignalColor::green) {
        if (green != runs.size()) return {}; // two greens
        green = i;
    }
    if (green == runs.size() || runs.size() == 1) return {}; // never green, or always green
    std::vector<SignalColor> after;
    for (std::size_t k = 1; k < runs.size(); ++k) after.push_back(runs[(green + k) % runs.size()].color);
    const bool amberThenRed = after == std::vector<SignalColor>{SignalColor::amber, SignalColor::red};
    const bool one = after.size() == 1 && after.front() != SignalColor::green;
    if (!amberThenRed && !one) return {};
    const double amber = after.front() == SignalColor::amber ? runs[(green + 1) % runs.size()].duration : 0;
    return Candidate{&p, cycle, runs[green].start, runs[green].duration, amber};
}
}
// Schema 12 and earlier gave each head a program of its own colours. Schema 13 gives it a signal
// group (M2.7b). A program shaped like a group -- one green, optional amber, red -- becomes one;
// programs sharing a cycle length share a controller with offset 0, each group's green start
// carrying the program's own offset, so every head shows the colour it showed before at every
// instant. Anything else (two greens, always green, off the time grid) stays a legacy program.
void migrateSignalPrograms(ProjectDocument& d) {
    if (!d.definition) return;
    auto& def = *d.definition;
    const double step = def.timeStep > 0 ? def.timeStep : 0.1;
    std::vector<Candidate> found;
    for (const auto& p : def.signalPrograms) if (auto c = asGroup(p, step)) found.push_back(*c);
    if (found.empty()) return;
    std::vector<double> cycles;
    for (const auto& c : found) {
        bool known = false;
        for (double k : cycles) known = known || std::abs(k - c.cycle) < 1e-9;
        if (!known) cycles.push_back(c.cycle);
    }
    std::map<std::string, std::pair<std::string, int>> moved;
    for (double cycle : cycles) {
        SignalController controller{allocateId(d, "controller"), {}, cycle, 0, {}};
        for (const auto& c : found) if (std::abs(c.cycle - cycle) < 1e-9) {
            const int number = static_cast<int>(controller.groups.size()) + 1;
            const double start = wrap(c.greenStart - c.program->offset, cycle);
            controller.groups.push_back({number, c.program->id, start, wrap(start + c.green, cycle), c.amber});
            moved[c.program->id] = {controller.id, number};
        }
        def.signalControllers.push_back(std::move(controller));
    }
    std::erase_if(def.signalPrograms, [&](const auto& p) { return moved.contains(p.id); });
    for (auto& h : d.network.signalHeads) if (const auto it = moved.find(h.programId); it != moved.end()) {
        h.controllerId = it->second.first; h.groupNumber = it->second.second; h.programId.clear();
    }
}
}
