// M3.3.3a BA27: queue discharge of the four-leg project at dt 0.1 / 0.25 / 0.5, prototype and w74,
// seeds 42-81 (docs/evidence/w74-discharge.md). Not in `check`: evidence to archive, like the
// other sweeps. Sensitivity is recorded, never bounded; nothing here is calibration (rule 4).
//
//   trafficsim-w74-discharge-sweep <w74 behaviour.json> <project.traffic.json> <data dir> <out.csv>
//
// Both models run on the same project with its catalogs captured as owned; the w74 arm adds the
// given behaviour and points every vehicle type at it. The discharge spec is the CLI default.
#include "discharge_options.hpp"
#include "w74_fixture_document.hpp"
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
}
int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: trafficsim-w74-discharge-sweep <w74 behaviour.json> <project> <data dir> <out.csv>\n";
        return 2;
    }
    try {
        const std::filesystem::path data = argv[3], out = argv[4];
        if (std::filesystem::exists(out)) throw std::invalid_argument("Output already exists: " + out.string());
        const auto w74 = w74fixture::behaviour(argv[1]);
        const auto base = w74fixture::base(argv[2], data);
        std::ofstream csv(out);
        csv << "model,dt,seed,head,lane,go,complete,reason,samples,meanHeadway,vehiclesPerHour,startupLostTime,startupReason,runClamps\n";
        for (const std::string model : {"prototype", "w74"})
            for (const double dt : {0.1, 0.25, 0.5}) {
                const auto d = w74fixture::arm(base, model == "w74" ? &w74 : nullptr, dt);
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
