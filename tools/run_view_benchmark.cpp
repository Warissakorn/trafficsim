// A per-Step cost benchmark for the Run view, on a real project (the M2.6 study template is the
// reference). NEXT.md's "slowing down" note was measured by hand; this makes it repeatable. It
// splits each Step into the synchronous part (the action: stepSimulation, observe, setRunFrame,
// refreshRun) and the deferred part (processEvents: layout and painting), because the two have
// different owners. On 2026-10-02 (Windows, Release) painting was 93% of a Step.
//
// It is NOT a test and is not part of `check`: it prints timings, it does not assert them.
// Timings are wall clock and depend on the window's size and the screen -- compare two runs on
// one machine. The window is fixed at 1600x1000 and the Results tab stays hidden.
//
//   trafficsim-run-view-benchmark <project.traffic.json> [steps] [batch]
#include "../src/shell/editor_window.hpp"
#include "../src/project/load.hpp"
#include <QAction>
#include <QApplication>
#include <QStandardPaths>
#include <QTest>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
using namespace trafficsim;
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    if (argc < 2) { std::cerr << "usage: <project.traffic.json> [steps] [batch]\n"; return 2; }
    const int steps = argc > 2 ? std::atoi(argv[2]) : 12000, batch = argc > 3 ? std::atoi(argv[3]) : 1000;
    if (steps < 1 || batch < 1) { std::cerr << "steps and batch must be positive\n"; return 2; }
    EditorWindow w{findDataDirectory(argv[0])};
    w.resize(1600, 1000); w.show(); QTest::qWait(50);
    w.openFile(QString::fromLocal8Bit(argv[1])); QTest::qWait(50);
    auto* step = w.findChild<QAction*>("editorStep");
    if (!step) { std::cerr << "no editorStep action\n"; return 1; }
    using clock = std::chrono::steady_clock;
    const auto ms = [](clock::duration d) { return std::chrono::duration<double, std::milli>(d).count(); };
    std::cout << std::setw(8) << "steps" << std::setw(10) << "vehicles" << std::setw(14) << "action ms"
              << std::setw(14) << "events ms" << std::setw(14) << "total ms" << "   (per Step)\n";
    double sumAction = 0, sumEvents = 0;
    for (int done = 0; done < steps;) {
        clock::duration action{}, events{};
        for (int i = 0; i < batch && done < steps; ++i, ++done) {
            const auto a = clock::now(); step->trigger();
            const auto b = clock::now(); QApplication::processEvents();
            const auto c = clock::now(); action += b - a; events += c - b;
        }
        sumAction += ms(action); sumEvents += ms(events);
        std::cout << std::fixed << std::setprecision(3) << std::setw(8) << done << std::setw(10)
                  << w.runState().vehicles.size() << std::setw(14) << ms(action) / batch << std::setw(14)
                  << ms(events) / batch << std::setw(14) << ms(action + events) / batch << "\n";
    }
    std::cout << std::setprecision(0) << "whole run: action " << sumAction << " ms, events " << sumEvents
              << " ms, total " << sumAction + sumEvents << " ms\n";
    return 0;
}
