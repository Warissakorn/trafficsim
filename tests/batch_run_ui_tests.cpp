// M5.6 (D141, BATCH §6 rows EB1-EB6): Run N seeds in the editor. The table must be the CLI's
// aggregate, Export its CSV byte for byte and Copy the same as tab-separated values; a cancelled or
// edited-away batch leaves no table that claims runs it did not finish.
#include "../src/shell/editor_window.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/project/batch_output.hpp"
#include "../src/project/csv_format.hpp"
#include "../src/project/document.hpp"
#include "../src/project/load.hpp"
#include "../src/project/los_output.hpp"
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
void waitForBatch(EditorWindow& w) {
    QElapsedTimer clock; clock.start();
    while (w.batchRunning()) { require(clock.elapsed() < 900000, "Batch did not finish"); QTest::qWait(20); }
}
bool refuses(const std::function<void()>& f) { try { f(); } catch (const std::exception&) { return true; } return false; }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc >= 2, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        // The four-leg template with two lettered sections, so the table carries LOS too.
        std::ifstream source(data / "projects/four-leg-signalised.traffic.json"); Json j; source >> j;
        auto document = parseDocument(j);
        putTravelTimeSection(document, {"", "West through", {"link-1", 20}, {"link-20", 50}, SectionControl::signalised});
        putTravelTimeSection(document, {"", "West left", {"link-1", 20}, {"link-44", 50}, SectionControl::signalised});
        QTemporaryDir dir; require(dir.isValid(), "No temporary directory");
        const auto project = dir.filePath("batch.traffic.json");
        { std::ofstream out(project.toStdString()); out << documentJson(document).dump(2); }

        EditorWindow w{data}; w.show(); QTest::qWait(30);
        w.openFile(project);
        auto* exportCsv = item<QAction>(w, "editorExportResults");
        auto* copy = item<QAction>(w, "editorCopyResults");
        auto* runSeeds_ = item<QAction>(w, "editorRunSeeds");
        auto* cancel = item<QAction>(w, "editorCancelSeeds");
        auto* seeds = item<QLineEdit>(w, "editorSeeds");
        require(seeds->text() == "42-51", "The default batch is not ten seeds (O9)");
        require(!exportCsv->isEnabled() && !copy->isEnabled(), "Export or Copy offered before any run");
        require(runSeeds_->isEnabled() && !cancel->isEnabled(), "Run seeds / Cancel start in the wrong state");

        // EB6: a repeated seed is refused before anything runs.
        seeds->setText("42,42"); action(w, "editorRunSeeds");
        require(!w.batchRunning() && !w.batchResult(), "A bad seed list started a batch");
        require(!item<QLabel>(w, "editorError")->text().isEmpty(), "A bad seed list was refused silently");

        // EB1-EB3: three seeds, against the CLI's own runSeeds/aggregate/batchCsv.
        const std::vector<std::uint32_t> list{42, 43, 44};
        seeds->setText("42-44"); action(w, "editorRunSeeds");
        require(w.batchRunning() && cancel->isEnabled() && !runSeeds_->isEnabled(), "The batch did not start");
        require(!exportCsv->isEnabled() && refuses([&] { w.resultsCsv(); }), "Export offered part way through a batch");
        waitForBatch(w);
        require(w.batchResult().has_value(), "The finished batch shows no result");
        const auto snapshot = compileDocument(w.history().document(), data);
        const auto spec = evaluationSpec(w.history().document(), snapshot, data);
        const auto runs = trafficsim::runSeeds(snapshot.scenario, spec, list);
        const auto expected = aggregate(runs);
        const auto& got = *w.batchResult();
        require(got.report == expected, "The editor's batch differs from the CLI's aggregate");
        require(got.report.seeds == list && got.report.sections.size() == 2, "Batch seeds or sections are wrong");

        {
            auto* movements = item<QTableWidget>(w, "editorBatchMovementTable");
            require(movements->isVisible(), "The batch table is not shown");
            require(!item<QTableWidget>(w, "editorMovementTable")->isVisible(), "The single-run table shows beside the batch");
            require(movements->rowCount() == static_cast<int>(expected.movements.size()), "Movement rows differ from the CLI");
            for (int r = 0; r < movements->rowCount(); ++r) {
                const auto& m = expected.movements[static_cast<std::size_t>(r)];
                require(movements->item(r, 0)->text().toStdString() == m.name, "Movement name differs");
                require(movements->item(r, 1)->text().toULongLong() == m.meanDelay.n, "Movement n differs");
                const auto delay = m.meanDelay.mean ? QString::number(*m.meanDelay.mean, 'f', 1) : QString();
                require(movements->item(r, 2)->text() == delay, "Movement mean delay differs");
            }
            auto* sections = item<QTableWidget>(w, "editorBatchSectionTable");
            require(sections->isVisible() && sections->rowCount() == 2, "Section rows missing");
            for (int r = 0; r < 2; ++r) {
                const auto& s = expected.sections[static_cast<std::size_t>(r)];
                require(sections->item(r, 6)->text().toStdString() == losCell(s.meanDelay.mean, s.controlType, expected.los),
                        "Section LOS differs from the CLI's");
            }
            require(!sections->item(0, 6)->text().isEmpty(), "A signalised section carries no LOS letter");
            require(item<QTableWidget>(w, "editorBatchQueueTable")->rowCount() == static_cast<int>(expected.queues.size()),
                    "Approach rows differ from the CLI");
            const auto note = item<QLabel>(w, "editorBatchNote")->text();
            require(note.startsWith("Not yet validated") && note.contains("3 runs"), "The batch note lacks the marker or n");
        }

        const auto csv = batchCsv(expected, runs);
        require(exportCsv->isEnabled() && copy->isEnabled(), "Export or Copy not offered for a finished batch");
        const auto csvFile = dir.filePath("batch.csv");
        w.exportResults(csvFile);
        QFile written(csvFile); require(written.open(QIODevice::ReadOnly), "Exported CSV missing");
        require(written.readAll().toStdString() == csv, "Exported CSV differs from the CLI's --csv"); written.close();
        require(csv.starts_with("# TrafficSim - not yet validated"), "Batch CSV carries no marker");
        action(w, "editorCopyResults");
        const auto pasted = QApplication::clipboard()->text().toStdString();
        require(pasted == csvToTsv(csv), "Copy differs from the CSV as tab-separated values");
        require(pasted.starts_with("# TrafficSim - not yet validated") && pasted.find("\t") != std::string::npos,
                "Copy lacks the marker or the tabs");

        // EB5: an edit discards it. Renaming a Link goes through History like every other edit.
        const auto revision = w.history().revision();
        w.canvas()->select("link-1"); QApplication::processEvents();
        auto* name = item<QLineEdit>(w, "editorName");
        name->setText("Renamed after the batch"); emit name->editingFinished(); QApplication::processEvents();
        require(w.history().revision() != revision, "The rename was not an edit");
        require(!w.batchResult() && !exportCsv->isEnabled() && !copy->isEnabled(), "An edit left the batch standing");
        require(item<QTableWidget>(w, "editorMovementTable")->isVisible(), "The single-run view did not return");
        // Saved, so the autosave (every 15 s) leaves no draft to offer the next run of this test.
        w.saveFile(project);

        // EB4: cancel part way leaves nothing claiming ten runs, and a later batch still works.
        seeds->setText("42-51"); action(w, "editorRunSeeds");
        require(w.batchRunning(), "The ten-seed batch did not start");
        QTest::qWait(10); action(w, "editorCancelSeeds");
        require(!w.batchRunning() && !w.batchResult(), "Cancel left a batch");
        require(!exportCsv->isEnabled() && refuses([&] { w.exportResults(dir.filePath("cancelled.csv")); }),
                "A cancelled batch can be exported");
        require(refuses([&] { w.copyResults(); }), "A cancelled batch can be copied");
        require(item<QTableWidget>(w, "editorMovementTable")->isVisible(), "Cancel did not restore the single-run view");
        seeds->setText("44"); action(w, "editorRunSeeds"); waitForBatch(w); // joins the cancelled worker first
        require(w.batchResult() && w.batchResult()->report.seeds == std::vector<std::uint32_t>{44}, "A batch after a cancel failed");
        // Whatever the cancelled worker posts late is discarded, never shown as a result.
        QTest::qWait(50);
        require(w.batchResult()->report.seeds.size() == 1, "A cancelled batch's result arrived late");

        // A single run replaces the batch: one table at a time.
        action(w, "editorStep");
        require(!w.batchResult(), "A single run left the batch standing");

        // Closing the window mid-batch must stop and join the worker, not crash.
        seeds->setText("42-51"); action(w, "editorRunSeeds");
        require(w.batchRunning(), "The closing batch did not start");
    } catch (const std::exception& e) {
        std::cerr << "batch-run-ui: " << e.what() << '\n';
        return 1;
    }
    std::cout << "batch-run-ui: ok\n";
    return 0;
}
