// M3.2.7b: the T-junction's diagnostic seeded sweep (docs/M3_ACCEPTANCE.md §2). Not in `check`:
// it is evidence to archive, like the benchmarks, not a pass/fail test.
//
//   trafficsim-t-junction-sweep --metadata <out.json> <repo root>   what the sweep depends on
//   trafficsim-t-junction-sweep --run <out.csv> <repo root>         one row per seed and rule
//   ... --metadata-congested / --run-congested                       M3.2.7c: the headway arm on
//                                                                     the congested variant
//
// The metadata is committed before any sweep output is looked at; the rows after.
#include "t_junction_sweep.hpp"
#include <iomanip>
#include <iostream>
using namespace trafficsim;
namespace {
std::string build() {
    std::ostringstream out;
#if defined(_MSC_VER)
    out << "MSVC " << _MSC_VER;
#elif defined(__clang__)
    out << "Clang " << __clang_version__;
#elif defined(__GNUC__)
    out << "GCC " << __VERSION__;
#endif
#ifdef NDEBUG
    out << ", NDEBUG";
#else
    out << ", assertions on";
#endif
    return out.str();
}
std::string figure(const std::optional<double>& v) {
    if (!v) return "";
    std::ostringstream out; out << std::setprecision(10) << *v; return out.str();
}
}
int main(int argc, char** argv) {
    const std::string mode = argc == 4 ? argv[1] : "";
    if (mode != "--metadata" && mode != "--run" && mode != "--metadata-congested" && mode != "--run-congested") {
        std::cerr << "usage: trafficsim-t-junction-sweep --metadata|--run|--metadata-congested|--run-congested <out> <repo root>\n"; return 2;
    }
    const bool congested = mode.ends_with("-congested");
    const std::filesystem::path root = argv[3];
    std::ofstream out(argv[2], std::ios::binary);
    if (mode.starts_with("--metadata")) {
        auto j = congested ? sweep::congestedMetadata(root) : sweep::fixtureMetadata(root);
        j["build"] = build();
        out << j.dump(2) << "\n";
        return out ? 0 : 1;
    }
    // Movement columns in the fixture's own order: the report lists them as evaluationSpec does.
    bool header = false;
    for (const auto& [gap, headway] : congested ? sweep::kHeadwayRules : sweep::kRules)
        for (const auto seed : sweep::kSeeds) {
            const auto row = sweep::runOne(root / "data", seed, gap, headway, congested);
            const auto& r = row.report;
            if (!header) {
                out << "seed,gapTime,headway";
                for (const auto& m : r.movements) out << ",\"" << m.name << " completed\",\"" << m.name << " meanDelay\"";
                for (const auto& q : r.queues) out << ",\"" << q.name << " mean\",\"" << q.name << " max\"";
                out << ",active,pending,safetyClamps\n";
                header = true;
            }
            out << seed << ',' << gap << ',' << headway;
            for (const auto& m : r.movements) out << ',' << m.vehicles << ',' << figure(m.meanDelay);
            for (const auto& q : r.queues) out << ',' << figure(q.meanLength) << ',' << figure(q.maxLength);
            out << ',' << r.active << ',' << r.pending << ',' << r.safetyClamps << '\n';
        }
    return out ? 0 : 1;
}
