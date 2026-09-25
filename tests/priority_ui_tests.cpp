#include "../src/shell/editor_window.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QGraphicsItem>
#include <QLabel>
#include <QPushButton>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace trafficsim;
// M3.2.4: the Conflict areas tab (A24). Keyboard and pointer reach the same commands; each
// edit is one Undo step; Problems rows lead to the area; the Run UI says what is protected.
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void write(const QString& path, const ProjectDocument& d) {
    QFile f(path); require(f.open(QIODevice::WriteOnly), "Fixture open failed");
    const auto bytes = QByteArray::fromStdString(documentJson(d).dump());
    require(f.write(bytes) == bytes.size(), "Fixture write failed");
}
int items(EditorWindow& w, const char* kind) {
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == kind;
    return n;
}
QAction* act(EditorWindow& w, const char* name) {
    auto* a = w.findChild<QAction*>(name); require(a, name); return a;
}
// Runs `answer` on the next modal dialog, which must be `name`.
void onDialog(const char* name, std::function<void(QDialog*)> answer, bool& ran) {
    QTimer::singleShot(0, [=, &ran] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        require(dialog && dialog->objectName() == name, name);
        answer(dialog); ran = true;
    });
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        ProjectDocument d;
        const auto a = addLink(d, {{0, 0}, {200, 0}}, 2, 3.5);
        const auto b = addLink(d, {{100, -100}, {100, 100}}, 1, 3.5);
        const auto x = addLink(d, {{0, 300}, {300, 300}}, 1, 3.5);
        const auto z = addLink(d, {{0, 360}, {100, 320}}, 1, 3.5);
        const auto join = addConnector(d, {z, d.network.links[3].lanes[0].id}, {x, d.network.links[2].lanes[0].id, 150.0});
        putInput(d, {"", putRoute(d, {"", {a}}), "car", 600, 0, 50});
        putInput(d, {"", putRoute(d, {"", {b}}), "car", 300, 0, 50});
        const auto file = temp.filePath("priority.traffic.json"); write(file, d);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        w.canvas()->fitNetwork(); QApplication::processEvents();
        auto* table = w.findChild<QTableWidget*>("editorConflictTable"); require(table, "No conflict table");
        auto* tabs = w.findChild<QTabWidget*>("editorObjectTabs"); require(tabs, "No object tabs");

        // Two Links selected: add crossing areas, B gives way. The dialog says what it will make.
        require(!act(w, "editorAddCrossing")->isEnabled(), "Add crossing areas enabled with nothing selected");
        w.canvas()->setSelection({a, b}); QApplication::processEvents();
        require(act(w, "editorAddCrossing")->isEnabled(), "Add crossing areas disabled for two crossing Links");
        bool ran = false;
        onDialog("editorCrossingDialog", [&](QDialog* dialog) {
            auto* yields = dialog->findChild<QComboBox*>("editorCrossingYields");
            auto* pairs = dialog->findChild<QLabel*>("editorCrossingPairs");
            require(yields && pairs, "Crossing dialog lost a field");
            require(yields->currentData().toString().toStdString() == b, "The last-selected object did not default to giving way");
            require(pairs->text().contains("2"), "The dialog did not show the two lane pairs");
            dialog->accept();
        }, ran);
        act(w, "editorAddCrossing")->trigger(); QApplication::processEvents();
        require(ran, "Crossing dialog never opened");
        require(table->rowCount() == 2, "Two lane pairs did not make two rows");
        require(tabs->currentIndex() == 9, "The Conflict areas tab was not shown");
        require(items(w, "conflict-area") == 4 && items(w, "waiting-line") == 3, "Areas and waiting lines were not drawn"); // one line per lane (D63)
        require(table->item(0, 6)->text() == "Runs", "A complete crossing did not report that it runs");
        const auto revision = w.history().revision();

        // Keyboard: select the first row and press Enter; change priority and gapTime; OK.
        table->setFocus(); table->selectRow(0); QApplication::processEvents();
        const auto area = table->item(0, 0)->data(Qt::UserRole).toString().toStdString();
        require(w.canvas()->highlightedConflict() == area, "Selecting a row did not highlight its area");
        ran = false;
        onDialog("editorConflictDialog", [&](QDialog* dialog) {
            auto* priority = dialog->findChild<QComboBox*>("editorConflictPriority");
            auto* gap = dialog->findChild<QDoubleSpinBox*>("editorConflictGap");
            require(priority && gap, "Conflict dialog lost a field");
            priority->setCurrentIndex(0); gap->setValue(5.5);
            dialog->accept();
        }, ran);
        QTest::keyClick(table, Qt::Key_Return); QApplication::processEvents();
        require(ran, "Enter on a row did not open the conflict dialog");
        const auto& row = w.history().document().network.rightOfWay;
        require(row.conflictAreas.front().priority == ConflictPriority::firstYields, "The priority was not committed");
        require(row.priorityRules.front().gapTime == 5.5, "gapTime was not committed");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(w.history().revision() == revision, "The edit was not one Undo step");
        require(w.history().document().network.rightOfWay.priorityRules.front().gapTime != 5.5, "Undo did not restore gapTime");

        // Take over the merge from its Connector, then hand it back.
        w.canvas()->select(join); QApplication::processEvents();
        require(act(w, "editorTakeOverMerge")->isEnabled(), "Take over merge disabled on a Connector");
        act(w, "editorTakeOverMerge")->trigger(); QApplication::processEvents();
        require(table->rowCount() == 3, "Taking over the merge added no row");
        int merge = -1;
        for (int r = 0; r < table->rowCount(); ++r) if (table->item(r, 2)->text() == "Merge") merge = r;
        require(merge >= 0, "The taken-over area is not a merge row");
        table->selectRow(merge); QApplication::processEvents();
        act(w, "editorRestorePriority")->trigger(); QApplication::processEvents();
        require(table->rowCount() == 2, "Restore automatic priority did not remove the merge");

        // An undetermined area is a Problems row that leads back to the area.
        table->selectRow(1); QApplication::processEvents();
        const auto second = table->item(1, 0)->data(Qt::UserRole).toString().toStdString();
        ran = false;
        onDialog("editorConflictDialog", [&](QDialog* dialog) {
            dialog->findChild<QComboBox*>("editorConflictPriority")->setCurrentIndex(2); dialog->accept(); }, ran);
        act(w, "editorEditConflict")->trigger(); QApplication::processEvents();
        require(ran, "The edit action opened no dialog");
        act(w, "editorRecheck")->trigger(); QApplication::processEvents();
        auto* problems = w.findChild<QTableWidget*>("editorProblemTable");
        int found = -1;
        for (int r = 0; r < problems->rowCount(); ++r)
            if (problems->item(r, 2) && problems->item(r, 2)->text().toStdString() == second) found = r;
        require(found >= 0, "The undetermined area is not a Problems row");
        problems->selectRow(found); QApplication::processEvents();
        require(tabs->currentIndex() == 9 && table->item(table->currentRow(), 0)->data(Qt::UserRole).toString().toStdString() == second,
                "The Problems row did not lead to its area");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();

        // Running shows what the solver protects; Thai text replaces English.
        auto* protection = w.findChild<QLabel*>("editorRunProtection"); require(protection, "No Run protection note");
        require(!protection->isVisible(), "The protection note showed before a run");
        act(w, "editorStep")->trigger(); QApplication::processEvents();
        require(protection->isVisible() && !protection->text().isEmpty(), "The Run UI did not say what is protected");
        auto* language = w.findChild<QComboBox*>("editorLanguage");
        language->setCurrentIndex(1); QApplication::processEvents();
        require(tabs->tabText(9) != "Conflict areas" && !tabs->tabText(9).isEmpty(), "The tab was not translated");
        require(act(w, "editorAddCrossing")->text() != "Add crossing areas", "The action was not translated");
        std::cout << "priority UI tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
