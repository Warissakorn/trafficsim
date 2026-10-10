// M5.8b (D149, BATCH §7 rows EC1-EC7): scenario comparison in the editor. The table must be the
// CLI's compareBatches over the same two batches, Export its comparisonCsv byte for byte and Copy
// the same as tab-separated values; a refused, cancelled or edited-away comparison leaves nothing
// that claims runs it did not finish.
#include "../src/shell/editor_window.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/project/comparison_output.hpp"
#include "../src/project/csv_format.hpp"
#include "../src/project/document.hpp"
#include "../src/project/load.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QElapsedTimer>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class T> T* item(QObject& root, const char* name) {
    auto* p = root.findChild<T*>(name); require(p, "Missing widget"); return p;
}
void action(EditorWindow& w, const char* name) { item<QAction>(w, name)->trigger(); QApplication::processEvents(); }
// The worker posts its result through the event loop, so wait by running it, never by sleeping.
void waitForWorker(EditorWindow& w) {
    QElapsedTimer clock; clock.start();
    while (w.batchRunning()) { require(clock.elapsed() < 900000, "The comparison did not finish"); QTest::qWait(20); }
}
bool refuses(const std::function<void()>& f) { try { f(); } catch (const std::exception&) { return true; } return false; }
void save(const ProjectDocument& d, const QString& file) { std::ofstream out(file.toStdString()); out << documentJson(d).dump(2); }
ComparedBatch batchOf(const ProjectDocument& d, const std::filesystem::path& data, const std::string& name,
                      const std::vector<std::uint32_t>& seeds) {
    const auto snapshot = compileDocument(d, data);
    ComparedBatch b{name, {}, runSeeds(snapshot.scenario, evaluationSpec(d, snapshot, data), seeds)};
    b.report = aggregate(b.runs);
    return b;
}
QString cellText(QTableWidget* t, int row, int column) { return t->item(row, column)->text(); }
QString figure(const std::optional<double>& v) { return v ? QString::number(*v, 'f', 1) : QString(); }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc >= 2, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        // Base: four-leg. Alternative: one input's volume raised by half and a section of its own,
        // so differences are not all 0 and one name is unmatched. A third copy moves the warm-up.
        std::ifstream source(data / "projects/four-leg-signalised.traffic.json"); Json j; source >> j;
        const auto base = parseDocument(j);
        auto busier = base;
        busier.definition->inputs[0].vehiclesPerHour *= 1.5;
        putTravelTimeSection(busier, {"", "West through", {"link-1", 20}, {"link-20", 50}, SectionControl::signalised});
        auto shifted = base; shifted.definition->evaluation = EvaluationPeriod{60, std::nullopt, 0};
        QTemporaryDir dir; require(dir.isValid(), "No temporary directory");
        const auto baseFile = dir.filePath("base.traffic.json"), alternativeFile = dir.filePath("busier.traffic.json");
        const auto shiftedFile = dir.filePath("shifted.traffic.json");
        save(base, baseFile); save(busier, alternativeFile); save(shifted, shiftedFile);

        EditorWindow w{data}; w.show(); QTest::qWait(30);
        w.openFile(baseFile);
        auto* exportCsv = item<QAction>(w, "editorExportResults");
        auto* copy = item<QAction>(w, "editorCopyResults");
        auto* compare = item<QAction>(w, "editorCompareSeeds");
        auto* runSeeds_ = item<QAction>(w, "editorRunSeeds");
        auto* seeds = item<QLineEdit>(w, "editorSeeds");
        auto* error = item<QLabel>(w, "editorError");
        require(compare->isEnabled() && !exportCsv->isEnabled(), "Compare or Export starts in the wrong state");

        // EC4: a bad seed list is refused before any run.
        seeds->setText("42,42"); w.compareWithFile(alternativeFile);
        require(!w.batchRunning() && !w.comparisonResult(), "A bad seed list started a comparison");
        require(!error->text().isEmpty(), "A bad seed list was refused silently");
        // EC4: with a finished batch on show, an unreadable alternative and a different warm-up
        // are refused before any run, and the batch stays.
        seeds->setText("42"); action(w, "editorRunSeeds"); waitForWorker(w);
        require(w.batchResult().has_value(), "The one-seed batch did not finish");
        w.compareWithFile(dir.filePath("missing.traffic.json"));
        require(!w.batchRunning() && !w.comparisonResult() && w.batchResult(), "A missing alternative was not refused cleanly");
        require(error->text().contains("missing.traffic.json"), "The refusal does not name the alternative");
        w.compareWithFile(shiftedFile);
        require(!w.batchRunning() && !w.comparisonResult() && w.batchResult(), "Different warm-ups were not refused cleanly");
        require(error->text().contains("evaluation period"), "The window refusal does not say why");

        // EC1-EC3: two seeds, against the CLI's own runSeeds/aggregate/compareBatches/comparisonCsv.
        const std::vector<std::uint32_t> list{42, 43};
        seeds->setText("42-43"); w.compareWithFile(alternativeFile);
        require(w.batchRunning() && !compare->isEnabled() && !runSeeds_->isEnabled(), "The comparison did not start");
        require(!w.batchResult(), "The batch stayed beside the comparison");
        require(!exportCsv->isEnabled() && refuses([&] { w.resultsCsv(); }), "Export offered part way through a comparison");
        waitForWorker(w);
        require(error->text().isEmpty(), "A finished comparison left an error");
        require(w.comparisonResult().has_value(), "The finished comparison shows no result");
        const auto b = batchOf(base, data, "base.traffic.json", list);
        const auto a = batchOf(busier, data, "busier.traffic.json", list);
        const auto expected = compareBatches(b.report, a.report);
        const auto& got = *w.comparisonResult();
        require(got.comparison == expected, "The editor's comparison differs from the CLI's");
        require(got.base.report == b.report && got.alternative.report == a.report, "A side's batch differs from the CLI's");
        require(expected.unmatchedSections.alternativeOnly == std::vector<std::string>{"West through"},
                "The forcing failed: the alternative's own section is not unmatched");
        {
            auto* movements = item<QTableWidget>(w, "editorCompareMovementTable");
            require(movements->isVisible(), "The comparison table is not shown");
            require(!item<QTableWidget>(w, "editorBatchMovementTable")->isVisible() &&
                    !item<QTableWidget>(w, "editorMovementTable")->isVisible(), "Another table shows beside the comparison");
            require(movements->rowCount() == static_cast<int>(expected.movements.size()), "Movement rows differ from the CLI");
            bool changed = false;
            for (int r = 0; r < movements->rowCount(); ++r) {
                const auto& m = expected.movements[static_cast<std::size_t>(r)];
                require(cellText(movements, r, 0).toStdString() == m.name, "Movement name differs");
                require(cellText(movements, r, 1).toULongLong() == m.value.nBase, "Base n differs");
                require(cellText(movements, r, 2) == figure(m.value.base), "Base mean differs");
                require(cellText(movements, r, 4) == figure(m.value.alternative), "Alternative mean differs");
                require(cellText(movements, r, 5) == figure(m.value.difference), "Difference differs");
                require(cellText(movements, r, 6) == figure(m.value.halfWidth95), "Half-width differs");
                changed = changed || m.value.difference.value_or(0) != 0;
            }
            require(changed, "The forcing failed: every difference is 0");
            require(!item<QTableWidget>(w, "editorCompareSectionTable")->isVisible(), "A section table shows with no matched section");
            auto* queues = item<QTableWidget>(w, "editorCompareQueueTable");
            require(queues->rowCount() == static_cast<int>(expected.queues.size()), "Approach rows differ from the CLI");
            require(cellText(queues, 0, 5) == figure(expected.queues[0].value.difference), "Queue difference differs");
            const auto note = item<QLabel>(w, "editorCompareNote")->text();
            require(note.startsWith("Not yet validated") && note.contains("2 runs of each"), "The note lacks the marker or n");
            require(note.contains("base.traffic.json") && note.contains("busier.traffic.json"), "The note does not name both projects");
            require(note.contains("alternative only: West through"), "EC7: the unmatched section is not named"); // EC7
        }
        const auto csv = comparisonCsv(expected, b, a);
        require(exportCsv->isEnabled() && copy->isEnabled(), "Export or Copy not offered for a finished comparison");
        const auto csvFile = dir.filePath("comparison.csv");
        w.exportResults(csvFile);
        QFile written(csvFile); require(written.open(QIODevice::ReadOnly), "Exported CSV missing");
        require(written.readAll().toStdString() == csv, "Exported CSV differs from the CLI's --compare --csv"); written.close();
        require(csv.starts_with("# TrafficSim - not yet validated") &&
                csv.find("# Base: \"base.traffic.json\"; alternative: \"busier.traffic.json\"\n") != std::string::npos,
                "The CSV lacks the marker or the names");
        action(w, "editorCopyResults");
        const auto pasted = QApplication::clipboard()->text().toStdString();
        require(pasted == csvToTsv(csv), "Copy differs from the CSV as tab-separated values");
        require(pasted.starts_with("# TrafficSim - not yet validated") && pasted.find('\t') != std::string::npos,
                "Copy lacks the marker or the tabs");

        // EC6: a batch replaces the comparison at once.
        action(w, "editorRunSeeds");
        require(w.batchRunning() && !w.comparisonResult(), "A batch left the comparison standing");
        action(w, "editorCancelSeeds");

        // EC6: an edit discards a finished comparison; one seed is enough from here on.
        seeds->setText("42"); w.compareWithFile(alternativeFile); waitForWorker(w);
        require(w.comparisonResult().has_value(), "The one-seed comparison did not finish");
        const auto revision = w.history().revision();
        w.canvas()->select("link-1"); QApplication::processEvents();
        auto* name = item<QLineEdit>(w, "editorName");
        name->setText("Renamed after the comparison"); emit name->editingFinished(); QApplication::processEvents();
        require(w.history().revision() != revision && w.history().dirty(), "The rename was not an edit");
        require(!w.comparisonResult() && !exportCsv->isEnabled(), "An edit left the comparison standing");
        require(item<QTableWidget>(w, "editorMovementTable")->isVisible(), "The single-run view did not return");

        // EC7: with unsaved edits the CSV names the base as such.
        w.compareWithFile(alternativeFile); waitForWorker(w);
        require(w.comparisonResult().has_value(), "The comparison over unsaved edits did not finish");
        require(w.resultsCsv().find("# Base: \"base.traffic.json (unsaved edits)\"; alternative: \"busier.traffic.json\"\n") !=
                std::string::npos, "Unsaved edits are not named in the CSV");
        // EC6: a single run replaces it.
        action(w, "editorStep");
        require(!w.comparisonResult(), "A single run left the comparison standing");
        // Saved, so the autosave (every 15 s) leaves no draft to offer the next run of this test.
        w.saveFile(baseFile);

        // EC5: cancel part way leaves nothing, and a late result is discarded.
        seeds->setText("42-51"); w.compareWithFile(alternativeFile);
        require(w.batchRunning(), "The ten-seed comparison did not start");
        QTest::qWait(10); action(w, "editorCancelSeeds");
        require(!w.batchRunning() && !w.comparisonResult(), "Cancel left a comparison");
        require(!exportCsv->isEnabled() && refuses([&] { w.exportResults(dir.filePath("cancelled.csv")); }),
                "A cancelled comparison can be exported");
        require(refuses([&] { w.copyResults(); }), "A cancelled comparison can be copied");
        seeds->setText("44"); w.compareWithFile(alternativeFile); waitForWorker(w); // joins the cancelled worker first
        require(w.comparisonResult() && w.comparisonResult()->comparison.seeds == std::vector<std::uint32_t>{44},
                "A comparison after a cancel failed");
        QTest::qWait(50);
        require(w.comparisonResult()->comparison.seeds.size() == 1, "A cancelled comparison's result arrived late");

        // Closing the window mid-comparison must stop and join the worker, not crash.
        seeds->setText("42-51"); w.compareWithFile(alternativeFile);
        require(w.batchRunning(), "The closing comparison did not start");
    } catch (const std::exception& e) {
        std::cerr << "compare-run-ui: " << e.what() << '\n';
        return 1;
    }
    std::cout << "compare-run-ui: ok\n";
    return 0;
}
