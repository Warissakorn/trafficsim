// M3.3.3a BA27: queue discharge of the four-leg project at dt 0.1 / 0.25 / 0.5, prototype and w74,
// seeds 42-81 (docs/evidence/w74-discharge.md). Not in `check`: evidence to archive, like the
// other sweeps. Sensitivity is recorded, never bounded; nothing here is calibration (rule 4).
//
//   trafficsim-w74-discharge-sweep <w74 behaviour.json> <project.traffic.json> <data dir> <out.csv>
//
// Both models run on the same project with its catalogs captured as owned; the w74 arm adds the
// given behaviour and points every vehicle type at it. The discharge spec is the CLI default.
#include "discharge_options.hpp"
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/core/simulation.hpp"
#include "../src/project/run.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
using namespace trafficsim;
namespace {
std::string figure(const std::optional<double>& v) {
    if (!v) return "";
    std::ostringstream out; out << std::setprecision(10) << *v; return out.str();
}
Json read(const std::filesystem::path& file) {
    std::ifstream in(file);
    if (!in) throw std::runtime_error("Cannot read " + file.string());
    return Json::parse(in);
}
}
int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: trafficsim-w74-discharge-sweep <w74 behaviour.json> <project> <data dir> <out.csv>\n";
        return 2;
    }
    try {
        const std::filesystem::path data = argv[3], out = argv[4];
        if (std::filesystem::exists(out)) throw std::invalid_argument("Output already exists: " + out.string());
        const auto w74 = parseBehaviour(read(argv[1]));
        if (!w74.w74) throw std::invalid_argument("The behaviour file must be a w74 behaviour");
        auto base = parseDocument(read(argv[2]));
        putDemandCatalog(base, resolveDemandCatalog(*base.definition, data));
        const double duration = base.definition->duration;
        std::ofstream csv(out);
        csv << "model,dt,seed,head,lane,go,complete,reason,samples,meanHeadway,vehiclesPerHour,startupLostTime,startupReason,runClamps\n";
        for (const std::string model : {"prototype", "w74"})
            for (const double dt : {0.1, 0.25, 0.5}) {
                auto d = base;
                if (model == "w74") {
                    putBehaviour(d, w74, "W74 discharge fixture");
                    for (auto& t : d.definition->vehicleTypes) t.behaviourId = w74.id;
                }
                changeRunSettings(d, duration, dt);
                validateDocument(d);
                const auto snapshot = compileDocument(d, data);
                const auto spec = evaluationSpec(d, snapshot, data);
                const auto discharge = DischargeOptions{}.forDuration(snapshot.scenario.duration);
                for (std::uint32_t seed = 42; seed <= 81; ++seed) {
                    DischargeAccumulator release(discharge, spec.queue);
                    auto state = createSimulation(snapshot.scenario, seed);
                    release.observe(state);
                    std::size_t clamps = 0;
                    while (state.tick < totalTicks(snapshot.scenario)) {
                        state = stepSimulation(std::move(state));
                        release.observe(state);
                        for (const auto& e : state.events) clamps += std::holds_alternative<SafetyClampEvent>(e);
                    }
                    for (const auto& cycle : release.report()) {
                        const auto e = estimateDischarge(cycle, discharge);
                        csv << model << ',' << dt << ',' << seed << ',' << cycle.headId << ',' << cycle.laneId << ','
                            << std::setprecision(10) << cycle.go << ',' << cycle.complete << ',' << e.reason << ','
                            << e.samples << ',' << figure(e.meanHeadway) << ',' << figure(e.dischargeVehiclesPerHour) << ','
                            << figure(e.startupLostTime) << ',' << e.startupUnavailableReason << ',' << clamps << '\n';
                    }
                    std::cerr << model << " dt " << dt << " seed " << seed << " clamps " << clamps << '\n';
                }
            }
        if (!csv) throw std::runtime_error("Failed writing " + out.string());
    } catch (const std::exception& e) { std::cerr << "error: " << e.what() << '\n'; return 1; }
}
