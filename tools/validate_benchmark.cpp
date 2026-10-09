// A latency benchmark for document validation, which History runs on every command: what an
// author waits for between an edit and the canvas showing it. D140 took the M2.6 study template
// from 21.6 to 5.0 ms here; a routing change that recomputes geometry per query shows up first.
//
// It is NOT a test and is not part of `check`: it prints timings, it does not assert them.
// Timings are wall clock and machine-dependent -- compare two runs on one machine, never a
// number here against a number from elsewhere. The document is parsed once and never changed,
// so the work being timed is identical every repetition.
//
//   trafficsim-validate-benchmark [project.traffic.json] [repetitions]
#include "../src/project/document.hpp"
#include "../src/project/load.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
using namespace trafficsim;
int main(int argc, char** argv) {
    try {
        const auto file = argc > 1 ? std::filesystem::path(argv[1])
                                   : findDataDirectory(argv[0]) / "projects" / "m2.6-study-template.traffic.json";
        const int repetitions = argc > 2 ? std::atoi(argv[2]) : 15;
        if (repetitions < 1) { std::cerr << "usage: [project.traffic.json] [repetitions>=1]\n"; return 2; }
        std::ifstream stream(file);
        if (!stream) { std::cerr << "FAIL: cannot open " << file.string() << '\n'; return 1; }
        const auto document = parseDocument(Json::parse(stream));
        std::vector<double> ms;
        for (int i = 0; i < repetitions; ++i) {
            const auto start = std::chrono::steady_clock::now();
            validateDocument(document);
            ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
        }
        std::sort(ms.begin(), ms.end());
        std::cout << file.filename().string() << ": " << document.network.links.size() << " links, "
                  << document.network.connectors.size() << " connectors, " << repetitions << " repetitions\n"
                  << std::fixed << std::setprecision(2)
                  << std::left << std::setw(30) << "validateDocument median" << std::right << std::setw(10)
                  << ms[ms.size() / 2] << " ms (min " << ms.front() << ", max " << ms.back() << ")\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
