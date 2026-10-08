// M5.3 (D138): Simulation ▸ Run N seeds on the four-leg signalised project. The batch tab must
// equal the CLI's batch over the same seeds (the same runSeeds/aggregate on the same compiled
// document); an edit or Cancel must leave no table claiming N runs, also once the worker's
// in-flight seed has finished; the not-validated / not-HCM statement stays in both languages.
#include "../src/shell/editor_window.hpp"
#include "../src/project/batch_output.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <fstream>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class T> T* item(QObject& root, const char* name) { auto* p = root.findChild<T*>(name); require(p, name); return p; }
// Answers the next modal dialog: `fill` sets its fields, then it is accepted.
void answer(const std::function<void(QDialog&)>& fill) {
    QTimer::singleShot(0, [fill] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog) { std::cerr << "No dialog\n"; std::exit(1); }
        fill(*dialog); dialog->accept();
    });
}
bool waitFor(const std::function<bool()>& done, int ms) {
    QElapsedTimer timer; timer.start();
    while (!done()) { if (timer.elapsed() > ms) return false; QTest::qWait(20); }
    return true;
}
QString cell(QTableWidget* t, int r, int c) { return t->item(r, c) ? t->item(r, c)->text() : QString(); }
QString shown(const std::optional<double>& v, int decimals = 1) { return v ? QString::number(*v, 'f', decimals) : QString(); }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc >= 2, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        std::ifstream in(data / "projects/four-leg-signalised.traffic.json");
        auto document = parseDocument(Json::parse(in));
        QTemporaryDir dir; require(dir.isValid(), "No temporary directory");
        const auto path = dir.filePath("four-leg.traffic.json");
        QFile f(path); require(f.open(QIODevice::WriteOnly), "Fixture write failed");
        f.write(QByteArray::fromStdString(documentJson(document).dump())); f.close();

        // This test edits; a recovery file left by an earlier run would open the recovery dialog.
        const auto recovery = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/recovery";
        QDir(recovery).removeRecursively();
        EditorWindow w{data}; w.show(); QTest::qWait(30);
        w.openFile(path);
        auto* runSeeds = item<QAction>(w, "editorRunSeeds");
        auto* note = item<QLabel>(w, "editorBatchNote");
        auto* movements = item<QTableWidget>(w, "editorBatchMovementTable");
        auto* queues = item<QTableWidget>(w, "editorBatchQueueTable");
        auto* seeds = item<QTableWidget>(w, "editorBatchSeedTable");
        auto* cancel = item<QPushButton>(w, "editorBatchCancel");
        const auto empty = [&] { return movements->rowCount() == 0 && queues->rowCount() == 0 && seeds->rowCount() == 0; };
        const auto start = [&](const char* first, int count) {
            answer([&](QDialog& d) {
                item<QLineEdit>(d, "editorBatchFirst")->setText(first);
                item<QSpinBox>(d, "editorBatchCount")->setValue(count);
            });
            runSeeds->trigger();
        };

        // 1. Seeds 42-44 through the menu action: progress first, no partial table, then the batch.
        start("42", 3);
        require(w.batchRunning(), "The batch did not start");
        require(movements->isVisible(), "Results did not open on the batch tab");
        require(!runSeeds->isEnabled() && cancel->isVisible(), "A running batch must offer Cancel, not a second start");
        require(empty() && note->text().contains("of 3"), "A running batch shows progress, never rows");
        require(waitFor([&] { return !w.batchRunning(); }, 60000), "The batch did not finish");
        require(w.batchReport().has_value(), "A finished batch has no report");

        // The CLI's figures for the same seeds, computed independently of the window.
        const auto snapshot = compileDocument(w.history().document(), data);
        const auto expected = aggregate(trafficsim::runSeeds(snapshot.scenario,
            evaluationSpec(w.history().document(), snapshot, data), {42, 43, 44}));
        require(*w.batchReport() == expected, "The editor batch differs from runSeeds/aggregate");
        require(batchCsv(*w.batchReport()) == batchCsv(expected), "The editor batch differs from the CLI's CSV");
        require(movements->rowCount() == static_cast<int>(expected.movements.size()) && movements->rowCount() > 0, "Movement rows");
        bool absent = false;
        for (int r = 0; r < movements->rowCount(); ++r) {
            const auto& m = expected.movements[static_cast<std::size_t>(r)];
            require(cell(movements, r, 0) == QString::fromStdString(m.name), "Movement name");
            require(cell(movements, r, 1) == QString::number(m.meanDelay.n), "Movement n");
            require(cell(movements, r, 2) == shown(m.vehicles.mean), "Movement vehicles");
            require(cell(movements, r, 3) == shown(m.meanDelay.mean) && cell(movements, r, 4) == shown(m.meanDelay.halfWidth95), "Movement delay");
            require(cell(movements, r, 5) == shown(m.meanTravelTime.mean) && cell(movements, r, 6) == shown(m.meanTravelTime.halfWidth95), "Movement travel");
            absent = absent || !m.meanDelay.halfWidth95;
        }
        require(queues->rowCount() == static_cast<int>(expected.queues.size()) && queues->rowCount() > 0, "Queue rows");
        for (int r = 0; r < queues->rowCount(); ++r) {
            const auto& q = expected.queues[static_cast<std::size_t>(r)];
            require(cell(queues, r, 0) == QString::fromStdString(q.name) && cell(queues, r, 1) == QString::number(q.meanLength.n), "Queue name/n");
            require(cell(queues, r, 2) == shown(q.meanLength.mean) && cell(queues, r, 3) == shown(q.meanLength.halfWidth95), "Queue mean");
            require(cell(queues, r, 4) == shown(q.maxLength.mean) && cell(queues, r, 5) == shown(q.maxLength.halfWidth95), "Queue max");
        }
        require(seeds->rowCount() == 3, "One accounting row per seed");
        for (int r = 0; r < 3; ++r) {
            const auto& run = expected.runs[static_cast<std::size_t>(r)];
            require(cell(seeds, r, 0) == QString::number(run.seed) && cell(seeds, r, 1) == QString::number(run.generated), "Seed/generated");
            require(cell(seeds, r, 2) == QString::number(run.report.completed) && cell(seeds, r, 3) == QString::number(run.report.active)
                    && cell(seeds, r, 4) == QString::number(run.report.pending) && cell(seeds, r, 5) == QString::number(run.report.safetyClamps), "Seed accounting");
            require(cell(seeds, r, 6) == shown(run.report.meanDelay, 2), "Seed delay");
        }
        std::cout << "Movements with no ±95% (n < 2): " << (absent ? "some" : "none") << '\n';
        require(note->text().contains("Not yet validated") && note->text().contains("not HCM control delay or LOS")
                && note->text().contains("3 seeds (42–44)"), "The English note lost the marker or the seeds");
        require(runSeeds->isEnabled() && !cancel->isVisible(), "A finished batch must allow the next one");
        if (argc > 2) {
            w.resize(1280, 860); QTest::qWait(50); require(w.grab().save(QString::fromUtf8(argv[2]) + "-batch.png"), "Screenshot failed");
            item<QTabWidget>(w, "editorBatchSideTabs")->setCurrentIndex(1); QTest::qWait(50);
            require(w.grab().save(QString::fromUtf8(argv[2]) + "-seeds.png"), "Screenshot failed");
        }
        auto* language = item<QComboBox>(w, "editorLanguage");
        language->setCurrentIndex(1); QApplication::processEvents();
        require(note->text().contains("not yet validated") && note->text().contains("ไม่ใช่ HCM control delay หรือ LOS"),
                "The Thai note lost the marker");
        require(movements->rowCount() == static_cast<int>(expected.movements.size()), "Switching language dropped the batch");
        language->setCurrentIndex(0); QApplication::processEvents();

        // 2. An edit invalidates: the shown batch at once, and a running one, whose in-flight
        //    seed must not repopulate the table when it finishes.
        start("42", 10);
        require(w.batchRunning(), "The second batch did not start");
        answer([&](QDialog& d) { item<QDoubleSpinBox>(d, "editorTimeStep")->setValue(0.05); });
        item<QAction>(w, "editorRunSettings")->trigger();
        require(w.history().document().definition->timeStep == 0.05, "The edit did not apply");
        require(!w.batchRunning() && !w.batchReport() && empty(), "An edit left a batch standing");
        require(waitFor([&] { return w.batchThreadsJoined(); }, 60000), "The dropped job never stopped");
        QTest::qWait(100);
        require(!w.batchReport() && empty() && !note->text().contains("of 10"), "A dropped job published after the edit");
        item<QAction>(w, "editorUndo")->trigger();

        // 3. Cancel after the first seed: no rows, no claim of 10 runs, and the action is back.
        start("42", 10);
        require(waitFor([&] { return note->text().contains("1 of 10"); }, 30000), "No progress after the first seed");
        QTest::mouseClick(cancel, Qt::LeftButton);
        require(!w.batchRunning() && !w.batchReport() && empty() && runSeeds->isEnabled(), "Cancel left a batch standing");
        require(note->text().contains("cancelled") && !note->text().contains("10"), "Cancel must say there is no result");
        QElapsedTimer stopping; stopping.start();
        require(waitFor([&] { return w.batchThreadsJoined(); }, 60000), "The cancelled job never stopped");
        std::cout << "Cancelled job stopped " << stopping.elapsed() << " ms after Cancel\n";
        QTest::qWait(100);
        require(!w.batchReport() && empty() && note->text().contains("cancelled"), "A cancelled job published");

        // 4. Seeds past 2^32-1 are refused before anything runs.
        require(!w.startBatch(4294967295u, 2) && !w.batchRunning(), "An overflowing seed range was accepted");
        require(!item<QLabel>(w, "editorError")->text().isEmpty(), "The refusal was not shown");

        // 5. Closing with a batch running stops it (the destructor joins after the current seed).
        start("42", 10);
        require(w.batchRunning(), "The last batch did not start");
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n'; return 1;
    }
    std::cout << "batch-run-ui passed\n";
    return 0;
}
