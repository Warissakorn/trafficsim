// The M0 scenario, run through the editor. This is the replay-equivalence check that the
// retired M0 harness window used to carry: same fixture, same seed, same completed count and
// same mean delay as `trafficsim-cli 42`, reached through the only UI that still runs.
#include "../src/shell/editor_window.hpp"
#include "../src/shell/path.hpp"
#include "../src/core/simulation.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTest>
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class T> T* item(QObject& root, const char* name) {
    auto* p = root.findChild<T*>(name); require(p, "Missing widget"); return p;
}
void action(EditorWindow& w, const char* name) { item<QAction>(w, name)->trigger(); QApplication::processEvents(); }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc >= 2, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        const auto thaiPath = QString::fromUtf8("โครงการ/ทางแยก.json");
        require(displayPath(nativePath(thaiPath)) == thaiPath, "Unicode path roundtrip failed");

        EditorWindow w{data}; w.show(); QTest::qWait(30);
        // A bare M0 scenario carries no schemaVersion and is read with the pre-5 meaning. The
        // editor opening it is what makes retiring the separate M0 window safe.
        w.openFile(QString::fromStdString((data / "scenarios/crossing.json").string()));
        require(w.history().document().network.links.size() == 4, "M0 scenario did not load into the editor");
        require(w.history().document().definition
                && w.history().document().definition->inputs.size() == 2, "M0 definition did not survive the load");

        item<QLineEdit>(w, "editorSeed")->setText("42");
        action(w, "editorStep");
        require(w.runState().tick == 1, "Step must advance one tick");
        require(w.runState().seed == 42, "Seed was not applied to the run");
        for (int i = 1; i < 1800; ++i) action(w, "editorStep");
        // Same numbers CTest pins on `trafficsim-cli 42`; the engine is shared, so any drift
        // here is a UI stepping bug, not a modelling change.
        require(w.runState().completed == 31, "Editor run differs from the CLI baseline");
        const auto summary = w.runSummary();
        require(summary.meanDelay.has_value(), "Completed trips produced no delay");
        require(std::abs(*summary.meanDelay - 29.249359418430977) < 1e-7, "Editor summary differs from baseline");

        // Assert the forcing worked before the consequence: the run must actually be finished
        // before a further Step proving it cannot advance means anything.
        require(w.runState().tick == totalTicks(*w.runState().scenario), "Run did not reach its end");
        const auto finished = w.runState().tick;
        action(w, "editorStep");
        require(w.runState().tick == finished, "Completed run advanced past its end");

        // Both figures the retired window owned are on the editor's status line now.
        auto* info = item<QLabel>(w, "editorRunInfo");
        require(info->text().contains("mean delay 29.25"), "Mean delay missing from the run status");
        require(info->text().contains("safety clamps"), "Safety clamps missing from the run status");
        // M2.5: the finished run fills the Results tab from the same states, marker included.
        const auto report = w.runReport();
        require(report && !report->movements.empty(), "A finished run has no movement report");
        std::uint64_t trips = 0; for (const auto& m : report->movements) trips += m.vehicles;
        require(trips + report->unassigned == 31, "Movement trips do not add up to the run's completed trips");
        auto* movements = item<QTableWidget>(w, "editorMovementTable");
        require(movements->rowCount() == static_cast<int>(report->movements.size()), "Results table rows differ from the report");
        require(movements->item(0, 0)->text() == QString::fromStdString(report->movements[0].name), "Movement name missing");
        require(item<QTableWidget>(w, "editorQueueTable")->rowCount() == static_cast<int>(report->queues.size()), "Queue rows differ");
        require(item<QLabel>(w, "editorResultsNote")->text().startsWith("Not yet validated"), "Results carry no validation marker");
        // Captured on the finished run, so the artifact shows the figures being asserted.
        if (argc > 2) {
            w.resize(1280, 860); QTest::qWait(50);
            require(w.grab().save(QString::fromUtf8(argv[2])), "Screenshot failed");
        }

        action(w, "editorReset");
        require(w.runState().tick == 0 && !w.runSummary().meanDelay, "Reset left a stale run summary");
        // Reset prepares a fresh run at t = 0: the table is there, and nothing in it is stale.
        const auto fresh = w.runReport();
        require(fresh && std::all_of(fresh->movements.begin(), fresh->movements.end(),
                                     [](const auto& m) { return m.vehicles == 0; }), "Reset left stale results");

        item<QLineEdit>(w, "editorSeed")->setText("4294967296");
        action(w, "editorRun");
        require(!w.runState().scenario, "Overflow seed produced a run");
        item<QLineEdit>(w, "editorSeed")->setText("42");

        auto* language = item<QComboBox>(w, "editorLanguage");
        language->setCurrentIndex(1); QApplication::processEvents();
        action(w, "editorStep");
        require(item<QLabel>(w, "editorRunInfo")->text().contains(QString::fromUtf8("ความล่าช้าเฉลี่ย")),
                "Thai run status is missing the delay figure");
        std::cout << "Editor run of the M0 scenario matches the CLI baseline; delay and clamps are visible\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
