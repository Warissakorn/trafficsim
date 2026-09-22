// A frame-time benchmark for the network editor, on a network far larger than anything the
// owner has drawn by hand. M1.23 asks for a reproducible real-network benchmark before any
// rendering performance is claimed; this is that harness, and M1.27.1's evidence.
//
// It is NOT a test and is not part of `check`: it prints timings, it does not assert them.
// Timings are wall clock and therefore machine-dependent -- compare two runs on one machine,
// never a number here against a number from somewhere else. The NETWORK is deterministic, so
// the work being timed is the same every run.
//
//   trafficsim-editor-benchmark [intersections] [frames]
#include "../src/editor/canvas.hpp"
#include "benchmark_network.hpp"
#include <QApplication>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
using namespace trafficsim;
namespace {
double millis(const std::function<void()>& work, int times) {
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < times; ++i) work();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count() / times;
}
void row(const char* label, double ms) {
    std::cout << std::left << std::setw(34) << label << std::right << std::fixed
              << std::setprecision(2) << std::setw(9) << ms << " ms\n";
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    const int intersections = argc > 1 ? std::atoi(argv[1]) : 40;
    const int frames = argc > 2 ? std::atoi(argv[2]) : 20;
    if (intersections < 2 || frames < 1) { std::cerr << "usage: [intersections>=2] [frames>=1]\n"; return 2; }
    const auto document = benchmark::corridor(intersections).document;
    EditorCanvas canvas;
    canvas.resize(1600, 900);
    canvas.setDocument(&document);
    canvas.fitNetwork();
    const auto& network = document.network;
    std::cout << intersections << " intersections: " << network.links.size() << " links, "
              << network.connectors.size() << " connectors, " << network.signalHeads.size()
              << " signal heads, " << frames << " repetitions\n";
    // The pointer sits over the middle of the corridor, which is where a hover redraw happens.
    const Point pointer{200. * (intersections / 2) + 45, 0};
    row("redraw (whole scene)", millis([&] { canvas.redraw(); }, frames));
    row("hitObjects (every mouse move)", millis([&] { (void)canvas.hitObjects(pointer); }, frames));
    // A selection turns on the lane handles and the copy preview, the two other per-frame
    // passes over every connector.
    canvas.select(network.links.front().id);
    row("redraw (with a selection)", millis([&] { canvas.redraw(); }, frames));
    return 0;
}
