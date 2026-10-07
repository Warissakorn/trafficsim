#include "../src/core/simulation.hpp"
#include "discharge_options.hpp"
#include "../src/project/input_manifest.hpp"
#include "../src/project/load.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <optional>

namespace {
using namespace trafficsim;
// A project run steps the engine itself so the movement evaluation sees every state.
int runProject(const std::filesystem::path& file, const std::filesystem::path& csvFile,
               const std::filesystem::path& data, std::uint32_t seed, bool laneChanges, bool segmentTimes, bool stopLines,
               double phaseBin, bool waitCauses, const DischargeOptions& discharge) { // phaseBin 0: no --arrival-phases
    std::ifstream stream(file);
    if (!stream) throw std::runtime_error("Cannot read project: " + file.string());
    InputManifest inputs;
    auto* manifest=discharge.enabled?&inputs:nullptr;
    const auto document = parseDocument(manifest?readInputJson(file,"project",manifest):Json::parse(stream));
    const auto snapshot = compileDocument(document, data,manifest);
    // Refuse to overwrite before spending the run, as --events does.
    if (!csvFile.empty() && std::filesystem::exists(csvFile))
        throw std::invalid_argument("CSV output already exists: " + csvFile.string());
    const auto spec = evaluationSpec(document, snapshot, data,manifest);
    if(manifest)manifest->validate();
    MovementAccumulator movements(spec);
    std::optional<LaneChangeAccumulator> changes;
    if (laneChanges) changes.emplace(spec);
    std::optional<SegmentTimeAccumulator> segments;
    if (segmentTimes) segments.emplace(spec);
    std::optional<StopLineAccumulator> lines;
    if (stopLines) lines.emplace(spec);
    std::optional<ArrivalPhaseAccumulator> phases;
    if (phaseBin > 0) phases.emplace(spec, phaseBin);
    std::optional<DeadEndWaitAccumulator> waits;
    if (waitCauses) waits.emplace(spec);
    const auto dischargeSpec = discharge.enabled ? discharge.forDuration(snapshot.scenario.duration) : DischargeSpec{};
    std::optional<DischargeAccumulator> release;
    if (discharge.enabled) release.emplace(dischargeSpec, spec.queue);
    auto state = createSimulation(snapshot.scenario, seed);
    const auto observe = [&] {
        movements.observe(state);
        if (release) release->observe(state);
        if (changes) changes->observe(state);
        if (segments) segments->observe(state);
        if (lines) lines->observe(state);
        if (phases) phases->observe(state);
        if (waits) waits->observe(state);
    };
    observe();
    const auto ticks = totalTicks(snapshot.scenario);
    while (state.tick < ticks) { state = stepSimulation(std::move(state)); observe(); }
    const auto report = movements.report(state);
    if (!csvFile.empty()) {
        std::ofstream csv(csvFile);
        csv << movementCsv(report);
        if (!csv) throw std::runtime_error("Failed writing CSV output");
    }
    auto result = movementJson(report);
    if (release) {
        result["discharge"] = dischargeJson(release->report(), dischargeSpec);
        result["discharge"]["inputManifest"]=inputs.json();
    }
    result["engineVersion"] = std::string(TRAFFICSIM_VERSION) + "-cpp-m0";
    result["compiler"] = TRAFFICSIM_COMPILER;
    result["seed"] = seed;
    if (changes) result["laneChangeDiagnostics"] = laneChangeJson(changes->report());
    if (segments) result["segmentTimes"] = segmentTimeJson(segments->report());
    if (lines) result["stopLines"] = stopLineJson(lines->report());
    if (phases) result["arrivalPhases"] = arrivalPhaseJson(phases->report());
    if (waits) result["waitCauses"] = waitCauseJson(waits->report());
    std::cout << result.dump(2) << '\n';
    return std::cout ? 0 : 1;
}
}
int main(int argc, char** argv) {
    try {
        std::uint32_t seed = 42;
        std::filesystem::path data, scenarioFile, eventsFile, projectFile, csvFile;
        bool seedSet = false, laneChanges = false, segmentTimes = false, stopLines = false;
        bool arrivalPhases = false, waitCauses = false;
        DischargeOptions discharge;
        double binWidth = 10;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                std::cout << "TrafficSim (not yet validated)\n"
                             "Usage: trafficsim-cli [seed] [--seed N] [--data-dir DIR]\n"
                             "       [--scenario FILE] [--events FILE]\n"
                             "       [--project FILE.traffic.json [--csv FILE] [--lane-changes] [--segment-times]\n"
                             "        [--stop-lines] [--arrival-phases [--phase-bin S]] [--wait-causes] [--discharge]]\n"
                             "Outputs completed-trip diagnostics, not HCM control delay or LOS.\n"
                             "--project adds simulated movement delay and approach queues (M2.5).\n"
                             "--lane-changes adds where lane changes happen and dead-end waits (M3.2.8c).\n"
                             "--segment-times adds each movement's mean time to every segment (M3.2.8c).\n"
                             "--stop-lines adds each signal head's stop-line discharge (M3.2.8c).\n"
                             "--arrival-phases adds the cycle phase of segment entries and first stops (M3.2.8c),\n"
                             "  in --phase-bin S second bins (default 10).\n"
                             "--discharge adds unvalidated lane/cycle headways and startup estimates (ranks 3-5, 1-2).\n"
                             "  --discharge-start S --discharge-end S --discharge-warmup S (default full run, 0 warmup).\n"
                             "  --discharge-steady-first N --discharge-steady-last N --discharge-startup-last N.\n"
                             "  Repeat --discharge-type ID to select follower gaps at original ranks.\n"
                             "--wait-causes adds each dead-end wait's cause at its start (M3.2.8c).\n";
                return 0;
            }
            if (discharge.consume(arg, i, argc, argv)) continue;
            if (arg == "--lane-changes") { laneChanges = true; continue; }
            if (arg == "--segment-times") { segmentTimes = true; continue; }
            if (arg == "--stop-lines") { stopLines = true; continue; }
            if (arg == "--arrival-phases") { arrivalPhases = true; continue; }
            if (arg == "--wait-causes") { waitCauses = true; continue; }
            if (arg == "--seed" || arg == "--data-dir" || arg == "--scenario" || arg == "--events" ||
                arg == "--project" || arg == "--csv" || arg == "--phase-bin") {
                if (++i == argc) throw std::invalid_argument("Missing value for " + arg);
                if (arg == "--seed") {
                    if (seedSet) throw std::invalid_argument("Specify seed only once");
                    seed = parseSeed(argv[i]); seedSet = true;
                } else if (arg == "--data-dir") data = argv[i];
                else if (arg == "--scenario") scenarioFile = argv[i];
                else if (arg == "--project") projectFile = argv[i];
                else if (arg == "--csv") csvFile = argv[i];
                else if (arg == "--phase-bin") {
                    std::size_t used = 0;
                    binWidth = std::stod(argv[i], &used);
                    if (used != std::string(argv[i]).size() || !(binWidth > 0)) throw std::invalid_argument("--phase-bin needs a positive number of seconds");
                }
                else eventsFile = argv[i];
            } else {
                if (seedSet) throw std::invalid_argument("Unexpected argument: " + arg);
                seed = parseSeed(arg); seedSet = true;
            }
        }
        if (data.empty()) data = findDataDirectory(argv[0]);
        if (!csvFile.empty() && projectFile.empty()) throw std::invalid_argument("--csv needs --project");
        if (laneChanges && projectFile.empty()) throw std::invalid_argument("--lane-changes needs --project");
        if (segmentTimes && projectFile.empty()) throw std::invalid_argument("--segment-times needs --project");
        if (stopLines && projectFile.empty()) throw std::invalid_argument("--stop-lines needs --project");
        if (arrivalPhases && projectFile.empty()) throw std::invalid_argument("--arrival-phases needs --project");
        discharge.validateUsage(!projectFile.empty());
        if (waitCauses && projectFile.empty()) throw std::invalid_argument("--wait-causes needs --project");
        if (binWidth != 10 && !arrivalPhases) throw std::invalid_argument("--phase-bin needs --arrival-phases");
        if (!projectFile.empty()) {
            if (!scenarioFile.empty() || !eventsFile.empty())
                throw std::invalid_argument("--project cannot be combined with --scenario or --events");
            return runProject(projectFile, csvFile, data, seed, laneChanges, segmentTimes, stopLines, arrivalPhases ? binWidth : 0, waitCauses, discharge);
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
