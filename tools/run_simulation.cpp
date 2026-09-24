#include "../src/core/simulation.hpp"
#include "../src/project/load.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace {
using namespace trafficsim;
// A project run steps the engine itself so the movement evaluation sees every state.
int runProject(const std::filesystem::path& file, const std::filesystem::path& csvFile,
               const std::filesystem::path& data, std::uint32_t seed) {
    std::ifstream stream(file);
    if (!stream) throw std::runtime_error("Cannot read project: " + file.string());
    const auto document = parseDocument(Json::parse(stream));
    const auto snapshot = compileDocument(document, data);
    // Refuse to overwrite before spending the run, as --events does.
    if (!csvFile.empty() && std::filesystem::exists(csvFile))
        throw std::invalid_argument("CSV output already exists: " + csvFile.string());
    MovementAccumulator movements(evaluationSpec(document, snapshot, data));
    auto state = createSimulation(snapshot.scenario, seed);
    movements.observe(state);
    const auto ticks = totalTicks(snapshot.scenario);
    while (state.tick < ticks) { state = stepSimulation(state); movements.observe(state); }
    const auto report = movements.report(state);
    if (!csvFile.empty()) {
        std::ofstream csv(csvFile);
        csv << movementCsv(report);
        if (!csv) throw std::runtime_error("Failed writing CSV output");
    }
    auto result = movementJson(report);
    result["engineVersion"] = std::string(TRAFFICSIM_VERSION) + "-cpp-m0";
    result["compiler"] = TRAFFICSIM_COMPILER;
    result["seed"] = seed;
    std::cout << result.dump(2) << '\n';
    return std::cout ? 0 : 1;
}
}
int main(int argc, char** argv) {
    try {
        std::uint32_t seed = 42;
        std::filesystem::path data, scenarioFile, eventsFile, projectFile, csvFile;
        bool seedSet = false;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                std::cout << "TrafficSim (not yet validated)\n"
                             "Usage: trafficsim-cli [seed] [--seed N] [--data-dir DIR]\n"
                             "       [--scenario FILE] [--events FILE]\n"
                             "       [--project FILE.traffic.json [--csv FILE]]\n"
                             "Outputs completed-trip diagnostics, not HCM control delay or LOS.\n"
                             "--project adds simulated movement delay and approach queues (M2.5).\n";
                return 0;
            }
            if (arg == "--seed" || arg == "--data-dir" || arg == "--scenario" || arg == "--events" ||
                arg == "--project" || arg == "--csv") {
                if (++i == argc) throw std::invalid_argument("Missing value for " + arg);
                if (arg == "--seed") {
                    if (seedSet) throw std::invalid_argument("Specify seed only once");
                    seed = parseSeed(argv[i]); seedSet = true;
                } else if (arg == "--data-dir") data = argv[i];
                else if (arg == "--scenario") scenarioFile = argv[i];
                else if (arg == "--project") projectFile = argv[i];
                else if (arg == "--csv") csvFile = argv[i];
                else eventsFile = argv[i];
            } else {
                if (seedSet) throw std::invalid_argument("Unexpected argument: " + arg);
                seed = parseSeed(arg); seedSet = true;
            }
        }
        if (data.empty()) data = findDataDirectory(argv[0]);
        if (!csvFile.empty() && projectFile.empty()) throw std::invalid_argument("--csv needs --project");
        if (!projectFile.empty()) {
            if (!scenarioFile.empty() || !eventsFile.empty())
                throw std::invalid_argument("--project cannot be combined with --scenario or --events");
            return runProject(projectFile, csvFile, data, seed);
        }
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
