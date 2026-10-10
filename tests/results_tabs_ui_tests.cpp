// D144: the Results tabs for queue discharge and safety clamps, on the four-leg signalised
// project (900 s, seed 42). The discharge rows are checked against an independent
// DischargeAccumulator over the same seed; the clamp rows against the run's own count.
#include "../src/shell/editor_window.hpp"
#include "../src/core/simulation.hpp"
#include "../src/project/demand_catalog.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <fstream>
#include <iostream>
#include <map>
using namespace trafficsim;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class T> T* item(QObject& root, const char* name) { auto* p = root.findChild<T*>(name); require(p, name); return p; }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc >= 2, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        std::ifstream in(data / "projects/four-leg-signalised.traffic.json");
        auto document = parseDocument(Json::parse(in));
        // The clamp page needs clamps. Since M4.2 (D147) the catalog decides at amber and seed 42
        // has none, so this copy owns the catalog behaviour without amberDeceleration: D36's run.
        auto behaviours = resolveDemandCatalog(*document.definition, data).behaviours;
        for (auto& b : behaviours) b.amberDeceleration.reset();
        document.definition->behaviours = behaviours; document.definition->externalBehaviours = false;
        QTemporaryDir dir; require(dir.isValid(), "No temporary directory");
        const auto path = dir.filePath("four-leg.traffic.json");
        QFile f(path); require(f.open(QIODevice::WriteOnly), "Fixture write failed");
        f.write(QByteArray::fromStdString(documentJson(document).dump())); f.close();

        EditorWindow w{data}; w.show(); QTest::qWait(30);
        w.openFile(path);
        item<QLineEdit>(w, "editorSeed")->setText("42");
        auto* step = item<QAction>(w, "editorStep");
        step->trigger(); require(w.runState().scenario != nullptr, "Run did not start");
        for (std::uint64_t i = 1; i < totalTicks(*w.runState().scenario); ++i) {
            step->trigger(); if (i % 500 == 0) QApplication::processEvents();
        }
        QApplication::processEvents();
        require(w.runState().tick == totalTicks(*w.runState().scenario), "Run did not finish");

        // The same figures, independently: one accumulator over a fresh run of the same scenario.
        const auto snapshot = compileDocument(w.history().document(), data);
        DischargeSpec spec; spec.windowEnd = snapshot.scenario.duration;
        DischargeAccumulator expected(spec, evaluationSpec(w.history().document(), snapshot, data).queue);
        auto s = createSimulation(snapshot.scenario, 42); expected.observe(s);
        while (s.tick < totalTicks(snapshot.scenario)) { s = stepSimulation(std::move(s)); expected.observe(s); }
        std::map<std::pair<std::string, std::string>, std::pair<int, std::vector<double>>> heads;
        for (const auto& cycle : expected.report()) {
            auto& h = heads[{cycle.headId, cycle.laneId}]; ++h.first;
            if (const auto e = estimateDischarge(cycle, spec); e.meanHeadway) h.second.push_back(*e.meanHeadway);
        }

        auto* objects = item<QTabWidget>(w, "editorObjectTabs");
        auto* inner = item<QTabWidget>(w, "editorResultsTabs");
        objects->setCurrentWidget(inner->parentWidget()); inner->setCurrentIndex(1); QApplication::processEvents();
        auto* discharge = item<QTableWidget>(w, "editorDischargeTable");
        require(discharge->isVisible(), "Discharge tab did not open");
        // Optional screenshots (argv[2] is a path prefix): the discharge page, then the clamps page.
        const auto shot = [&](const char* suffix) {
            if (argc <= 2) return;
            w.resize(1280, 860); QTest::qWait(50);
            require(w.grab().save(QString::fromUtf8(argv[2]) + suffix), "Screenshot failed");
        };
        require(discharge->rowCount() == static_cast<int>(heads.size()) && !heads.empty(), "Discharge rows differ from the observer");
        int estimated = 0;
        for (int r = 0; r < discharge->rowCount(); ++r) {
            const auto& h = heads.at({discharge->item(r, 0)->text().toStdString(), discharge->item(r, 1)->text().toStdString()});
            require(discharge->item(r, 2)->text().toInt() == h.first, "Greens differ");
            require(discharge->item(r, 3)->text().toInt() == static_cast<int>(h.second.size()), "Estimate count differs");
            if (h.second.empty()) { require(discharge->item(r, 4)->text().isEmpty() && !discharge->item(r, 7)->text().isEmpty(),
                                            "An unavailable head shows a figure or no reason"); continue; }
            double sum = 0; for (double x : h.second) sum += x;
            require(discharge->item(r, 4)->text() == QString::number(sum / h.second.size(), 'f', 2), "Mean headway differs");
            ++estimated;
        }
        require(estimated > 0, "No head gave an estimate: the comparison was not exercised");
        shot("-discharge.png");
        require(item<QLabel>(w, "editorDischargeNote")->text().startsWith("Not yet validated"), "Discharge carries no marker");

        inner->setCurrentIndex(2); QApplication::processEvents();
        auto* clamps = item<QTableWidget>(w, "editorClampTable");
        const auto count = w.runSummary().safetyClamps;
        require(count > 0, "Seed 42 produced no clamp: the list was not exercised");
        require(clamps->rowCount() == static_cast<int>(count), "Clamp rows differ from the run's count");
        require(!clamps->item(0, 3)->text().isEmpty(), "Clamp row has no route");
        require(item<QLabel>(w, "editorClampNote")->text().startsWith(QString::number(count)), "Clamp note lacks the count");
        shot("-clamps.png");

        // Reset clears both lists; Thai text is present.
        item<QAction>(w, "editorReset")->trigger(); QApplication::processEvents();
        require(clamps->rowCount() == 0, "Reset left clamp rows");
        inner->setCurrentIndex(1); QApplication::processEvents();
        for (int r = 0; r < discharge->rowCount(); ++r)
            require(discharge->item(r, 3)->text().toInt() == 0, "Reset left discharge estimates");
        item<QComboBox>(w, "editorLanguage")->setCurrentIndex(1); QApplication::processEvents();
        require(inner->tabText(1) == QString::fromUtf8("การระบายคิว"), "Discharge tab not translated");
        std::cout << "Results discharge and clamp tabs match the observer and the run\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
