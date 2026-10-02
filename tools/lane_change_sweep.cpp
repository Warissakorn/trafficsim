// D98: the lane-change sweep -- a project across seeds and discretionary thresholds in one
// process, printed as markdown. Not in `check`: development evidence, like the other sweeps.
//
//   trafficsim-lane-change-sweep <repo root> [--project FILE] [--seeds 42-51]
//                                [--thresholds absent,0.25,0.5,1.0] [--accepted 1]
//
// The project defaults to the lane-change lab; A55 runs four-leg and M2.6 through the same tool.
// Deltas are paired by seed against the first variant, so list `absent` first to read them as A55.
#include "lane_change_sweep.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace trafficsim;
namespace {
struct Variant { std::string name; std::optional<double> threshold; };
struct Totals { std::uint64_t mandatory{}, discretionary{}, quick{}, back{}, afterMandatory{}, onward{}; };
void add(Totals& t, const LaneChangeRow& r) {
    t.mandatory += r.changes; t.discretionary += r.discretionaryChanges; t.quick += r.quickRepeats;
    t.back += r.quickBack; t.afterMandatory += r.quickAfterMandatory; t.onward += r.quickOnward;
}
std::vector<std::string> split(const std::string& text) {
    std::vector<std::string> out; std::stringstream in(text);
    for (std::string item; std::getline(in, item, ',');) out.push_back(item);
    return out;
}
std::string fixed(double v, int places = 2) { std::ostringstream o; o << std::fixed << std::setprecision(places) << v; return o.str(); }
}
int main(int argc, char** argv) {
    try {
        if (argc < 2 || argc % 2) {
            std::cerr << "usage: trafficsim-lane-change-sweep <repo root> [--project FILE] [--seeds 42-51]\n"
                         "       [--thresholds absent,0.25,0.5,1.0] [--accepted 1]\n";
            return 2;
        }
        const std::filesystem::path root = argv[1];
        auto project = root / "data/projects/lane-change-lab.traffic.json";
        std::uint32_t first = 42, last = 51;
        std::vector<Variant> variants;
        double accepted = 1;
        for (int i = 2; i < argc; i += 2) {
            const std::string flag = argv[i], value = argv[i + 1];
            if (flag == "--project") project = value;
            else if (flag == "--seeds") {
                const auto dash = value.find('-');
                first = static_cast<std::uint32_t>(std::stoul(value.substr(0, dash)));
                last = dash == std::string::npos ? first : static_cast<std::uint32_t>(std::stoul(value.substr(dash + 1)));
            } else if (flag == "--thresholds") {
                for (const auto& v : split(value)) variants.push_back({v, v == "absent" ? std::nullopt : std::optional{std::stod(v)}});
            } else if (flag == "--accepted") accepted = std::stod(value);
            else throw std::invalid_argument("unknown option " + flag);
        }
        if (variants.empty()) variants = {{"absent", {}}, {"0.25", 0.25}, {"0.5", 0.5}, {"1.0", 1.0}};
        if (last < first) throw std::invalid_argument("empty seed range");

        std::ifstream stream(project);
        if (!stream) throw std::runtime_error("Cannot read project: " + project.string());
        const auto document = parseDocument(Json::parse(stream));
        const auto data = root / "data";
        const auto snapshot = compileDocument(document, data);
        const auto started = std::chrono::steady_clock::now();
        // runs[variant][seed]
        std::vector<std::vector<sweep::LaneChangeRun>> runs(variants.size());
        for (std::size_t v = 0; v < variants.size(); ++v)
            for (auto seed = first; seed <= last; ++seed)
                runs[v].push_back(sweep::runLaneChanges(document, snapshot, data, seed, variants[v].threshold, accepted));
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();

        std::cout << "## " << project.filename().string() << ", seeds " << first << "-" << last
                  << ", acceptedDecelerationTrailingVehicle " << accepted << "\n\n"
                  << "Not validated; development evidence (D98). " << fixed(seconds, 1) << " s for "
                  << variants.size() * (last - first + 1) << " runs.\n\n"
                  << "| variant | mean delay (s) | completed | mandatory | discretionary | quick repeats | back | afterMandatory | onward | safety clamps |\n"
                  << "|---|---|---|---|---|---|---|---|---|---|\n";
        for (std::size_t v = 0; v < variants.size(); ++v) {
            Totals t; double delay = 0, completed = 0; std::uint64_t clamps = 0;
            for (const auto& r : runs[v]) {
                for (const auto& row : r.changes.rows) add(t, row);
                delay += r.movements.meanDelay.value_or(0); completed += static_cast<double>(r.movements.completed);
                clamps += r.movements.safetyClamps;
            }
            const auto n = static_cast<double>(runs[v].size());
            std::cout << "| " << variants[v].name << " | " << fixed(delay / n) << " | " << fixed(completed / n, 1) << " | "
                      << t.mandatory << " | " << t.discretionary << " | " << t.quick << " | " << t.back << " | "
                      << t.afterMandatory << " | " << t.onward << " | " << clamps << " |\n";
        }

        std::cout << "\nPer movement, summed over seeds: discretionary changes / back-and-forth repeats.\n\n| movement |";
        for (const auto& v : variants) std::cout << ' ' << v.name << " |";
        std::cout << "\n|---|";
        for (std::size_t v = 0; v < variants.size(); ++v) std::cout << "---|";
        const auto& names = runs[0][0].changes.rows;
        for (std::size_t m = 0; m < names.size(); ++m) {
            std::cout << "\n| " << names[m].name << " |";
            for (std::size_t v = 0; v < variants.size(); ++v) {
                Totals t;
                for (const auto& r : runs[v]) add(t, r.changes.rows[m]);
                std::cout << ' ' << t.discretionary << " / " << t.back << " |";
            }
        }

        std::cout << "\n\nPer-movement mean delay; delta = variant - " << variants[0].name << ", paired by seed, +- SE.\n\n| movement | "
                  << variants[0].name << " |";
        for (std::size_t v = 1; v < variants.size(); ++v) std::cout << " delta " << variants[v].name << " |";
        std::cout << "\n|---|---|";
        for (std::size_t v = 1; v < variants.size(); ++v) std::cout << "---|";
        const auto& movements = runs[0][0].movements.movements;
        for (std::size_t m = 0; m < movements.size(); ++m) {
            double base = 0; int counted = 0;
            for (const auto& r : runs[0]) if (const auto& d = r.movements.movements[m].meanDelay) { base += *d; ++counted; }
            std::cout << "\n| " << movements[m].name << " | " << (counted ? fixed(base / counted) : "n/a") << " |";
            for (std::size_t v = 1; v < variants.size(); ++v) {
                std::vector<double> diffs;
                for (std::size_t s = 0; s < runs[v].size(); ++s) {
                    const auto& a = runs[0][s].movements.movements[m].meanDelay;
                    const auto& b = runs[v][s].movements.movements[m].meanDelay;
                    if (a && b) diffs.push_back(*b - *a);
                }
                if (diffs.size() < 2) { std::cout << " n/a |"; continue; }
                double mean = 0, ss = 0;
                for (const auto d : diffs) mean += d;
                mean /= static_cast<double>(diffs.size());
                for (const auto d : diffs) ss += (d - mean) * (d - mean);
                const double se = std::sqrt(ss / static_cast<double>(diffs.size() - 1) / static_cast<double>(diffs.size()));
                std::cout << ' ' << (mean >= 0 ? "+" : "") << fixed(mean) << " +- " << fixed(se) << " |";
            }
        }
        std::cout << '\n';
        return std::cout ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
