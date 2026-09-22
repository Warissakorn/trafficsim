// A tick-rate benchmark for the simulation engine, on a network far larger than the shipped
// scenario. data/scenarios/crossing.json is 31 trips in 180 s and runs in under 60 ms, and
// profiling a toy input finds toy problems -- the 2026-09-18 profile session said so and
// generated its corridors by hand. This is that fixture, committed.
//
// It is NOT a test and is not part of `check`: it prints timings, it does not assert them.
// Timings are wall clock and machine-dependent -- compare two runs on one machine, never a
// number here against a number from elsewhere. The SCENARIO is deterministic, and the seed is
// fixed, so the work being timed is identical every run.
//
//   trafficsim-engine-benchmark [intersections] [seconds]
#include "../src/core/simulation.hpp"
#include "../src/project/run.hpp"
#include "../src/project/load.hpp"
#include "benchmark_network.hpp"
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
using namespace trafficsim;
namespace {
// One long through movement plus one entering movement per crossing, each fed by its own input.
// Volumes are the Link totals M1.26 made them, split across the three lanes at compile time, so
// a corridor of N crossings carries roughly 3N runtime routes and the same number of inputs.
ProjectDocument scenarioDocument(int intersections, double seconds) {
    auto built = benchmark::corridor(intersections, 30);
    auto& d = built.document;
    std::vector<std::string> mainLine{built.eastbound.front()};
    for (std::size_t k = 0; k < built.through.size(); ++k) {
        mainLine.push_back(built.through[k]);
        mainLine.push_back(built.eastbound[k + 1]);
    }
    const auto through = putRoute(d, {"", mainLine});
    putInput(d, {"", through, "car", 1800, 0, seconds});
    for (std::size_t k = 0; k < built.entering.size(); ++k) {
        const auto side = putRoute(d, {"", {built.northbound[k], built.entering[k],
                                            built.eastbound[k + 1]}});
        putInput(d, {"", side, "car", 600, 0, seconds});
    }
    changeRunSettings(d, seconds, 0.1);
    return d;
}
}
int main(int argc, char** argv) {
    try {
        const int intersections = argc > 1 ? std::atoi(argv[1]) : 12;
        const double seconds = argc > 2 ? std::atof(argv[2]) : 300;
        if (intersections < 2 || seconds < 1) { std::cerr << "usage: [intersections>=2] [seconds>=1]\n"; return 2; }
        const auto data = findDataDirectory(argv[0]);
        const auto compiled = compileDocument(scenarioDocument(intersections, seconds), data);
        const auto& scenario = compiled.scenario;
        auto state = createSimulation(scenario, 42);
        const auto ticks = totalTicks(scenario);
        std::uint64_t vehicleTicks = 0, peak = 0;
        const auto start = std::chrono::steady_clock::now();
        for (std::uint64_t t = 0; t < ticks; ++t) {
            state = stepSimulation(state);
            vehicleTicks += state.vehicles.size();
            peak = std::max<std::uint64_t>(peak, state.vehicles.size());
        }
        const double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        std::cout << intersections << " intersections: " << scenario.segments.size() << " segments, "
                  << scenario.routes.size() << " routes, " << scenario.inputs.size() << " inputs, "
                  << ticks << " ticks of " << scenario.timeStep << " s\n";
        std::cout << std::fixed << std::setprecision(2)
                  << std::left << std::setw(30) << "run" << std::right << std::setw(10) << ms << " ms\n"
                  << std::left << std::setw(30) << "per tick" << std::right << std::setw(10)
                  << ms / static_cast<double>(ticks) << " ms\n"
                  << std::left << std::setw(30) << "per vehicle-tick" << std::right << std::setw(10)
                  << (vehicleTicks ? ms * 1000 / static_cast<double>(vehicleTicks) : 0) << " us\n";
        std::cout << "completed " << state.completed << " trips, peak " << peak << " vehicles\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
