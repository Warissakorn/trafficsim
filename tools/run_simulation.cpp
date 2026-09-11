#include "../src/core/simulation.hpp"
#include "../src/project/load.hpp"
#include "../src/project/json.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    using namespace trafficsim;
    try {
        std::uint32_t seed = 42;
        std::filesystem::path data, scenarioFile, eventsFile;
        bool seedSet = false;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                std::cout << "TrafficSim (not yet validated)\n"
                             "Usage: trafficsim-cli [seed] [--seed N] [--data-dir DIR]\n"
                             "       [--scenario FILE] [--events FILE]\n"
                             "Outputs completed-trip diagnostics, not HCM control delay or LOS.\n";
                return 0;
            }
            if (arg == "--seed" || arg == "--data-dir" || arg == "--scenario" || arg == "--events") {
                if (++i == argc) throw std::invalid_argument("Missing value for " + arg);
                if (arg == "--seed") {
                    if (seedSet) throw std::invalid_argument("Specify seed only once");
                    seed = parseSeed(argv[i]); seedSet = true;
                } else if (arg == "--data-dir") data = argv[i];
                else if (arg == "--scenario") scenarioFile = argv[i];
                else eventsFile = argv[i];
            } else {
                if (seedSet) throw std::invalid_argument("Unexpected argument: " + arg);
                seed = parseSeed(arg); seedSet = true;
            }
        }
        if (data.empty()) data = findDataDirectory(argv[0]);
        if (scenarioFile.empty()) scenarioFile = data / "scenarios" / "crossing.json";
        const auto loaded = loadScenario(scenarioFile, data);
        std::ofstream events;
        if (!eventsFile.empty()) {
            // Never overwrite an existing scenario, catalog or previous event log.
            if (std::filesystem::exists(eventsFile)) throw std::invalid_argument("Event output already exists: " + eventsFile.string());
            events.open(eventsFile);
            if (!events) throw std::runtime_error("Cannot open event output: " + eventsFile.string());
        }
        SummaryAccumulator summary;
        const auto state = runSimulation(loaded.scenario, seed, [&](const SimEvent& event) {
            summary.add(event);
            if (events.is_open()) events << eventJson(event).dump() << '\n';
        }, events.is_open());
        if (events.is_open()) {
            events.flush();
            if (!events) throw std::runtime_error("Failed writing event output");
        }
        auto result = summaryJson(summary.summary());
        result["engineVersion"] = std::string(TRAFFICSIM_VERSION) + "-cpp-m0";
        result["compiler"] = TRAFFICSIM_COMPILER;
        result["seed"] = seed; result["time"] = state.time; result["active"] = state.vehicles.size();
        result["pending"] = pendingCount(state);
        std::cout << result.dump(2) << '\n';
        return std::cout ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "TrafficSim: " << error.what() << '\n';
        return 1;
    }
}
