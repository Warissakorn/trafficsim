#include "../src/shell/editor_window.hpp"
#include "../tools/t_junction_network.hpp"
#include "../src/model/network/right_of_way.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QGraphicsItem>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace trafficsim;
// M3.2.4c (D68): automatic conflict areas in the editor, as the owner asked -- they appear where
// roads overlap with no Add step; the Conflict area tool only sets priority. A passive crossing is
// authored by its first click (or P on its row) and made passive again by Delete; a derived merge is
// taken over by a click. Each is one Undo step, and nothing automatic is ever saved.
namespace {
// Printed before the throw: a window left with unsaved edits can abort while it unwinds.
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
QAction* act(EditorWindow& w, const char* name) { auto* a = w.findChild<QAction*>(name); require(a, name); return a; }
QPoint pixel(EditorWindow& w, Point p) {
    const auto q = w.canvas()->mapFromScene(p.x, p.y);
    require(w.canvas()->viewport()->rect().contains(q), "Point outside the viewport");
    return q;
}
void click(EditorWindow& w, Point p) {
    QTest::qWait(600); // never let two clicks become a double-click
    QTest::mouseClick(w.canvas()->viewport(), Qt::LeftButton, {}, pixel(w, p)); QApplication::processEvents();
}
void key(EditorWindow& w, Qt::Key k) { w.canvas()->setFocus(); QTest::keyClick(w.canvas(), k); QApplication::processEvents(); }
int drawn(EditorWindow& w, const char* kind) {
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == "auto-conflict" && i->data(2).toString() == kind;
    return n;
}
Point centre(const std::vector<Point>& outline) {
    Point c{};
    for (const auto& p : outline) { c.x += p.x / outline.size(); c.y += p.y / outline.size(); }
    return c;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        fixture::TJunctionOptions o; o.authoredControls = false;
        const auto t = fixture::tJunction(o);
        require(t.document.network.rightOfWay.empty(), "The drawing already has authored controls");
        const auto file = temp.filePath("bare.traffic.json");
        {
            QFile f(file); require(f.open(QIODevice::WriteOnly), "Fixture open failed");
            const auto bytes = QByteArray::fromStdString(documentJson(t.document).dump());
            require(f.write(bytes) == bytes.size(), "Fixture write failed");
        }
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        auto* c = w.canvas();
        c->fitNetwork(); c->centerOn(0, 0); c->scale(3, 3); QApplication::processEvents();
        auto* table = w.findChild<QTableWidget*>("editorConflictTable"); require(table, "No conflict table");
        const auto& network = [&]() -> const Network& { return w.history().document().network; };
        const auto all = automaticConflicts(network());
        const auto passive = *std::find_if(all.begin(), all.end(), [](const auto& a) { return a.kind == ConflictKind::crossing; });
        const auto merge = *std::find_if(all.begin(), all.end(), [](const auto& a) { return a.kind == ConflictKind::merge; });
        // Click where only the crossing is: the middle of its Link side (the turn's side is the same square).
        const auto onLink = passive.first.path.linkId.empty() ? passive.second : passive.first;
        const auto crossingPoint = centre(conflictSideOutline(network(), onLink));
        const auto mergePoint = centre(conflictSideOutline(network(), merge.first));

        // Select draws no automatic area and still picks a road there.
        require(drawn(w, "passive") == 0 && drawn(w, "merge") == 0, "Automatic areas drawn outside the Conflict area tool");
        click(w, crossingPoint);
        require(!c->selected().empty(), "Select did not pick a road at the crossing");

        // The Conflict area tool shows what the drawing implies, with no Add step.
        key(w, Qt::Key_A);
        require(drawn(w, "passive") == 2 && drawn(w, "merge") == 4, "The passive crossing and the two merges are not drawn");
        int automatic = 0;
        for (int r = 0; r < table->rowCount(); ++r) automatic += table->item(r, 6)->text().startsWith("Passive") || table->item(r, 6)->text().startsWith("Automatic");
        require(automatic == 3 && table->rowCount() == 3, "The table does not list the three automatic areas");

        // First click: the passive crossing gets a priority -- the turn gives way to the road.
        const auto revision = w.history().revision();
        click(w, crossingPoint);
        require(network().rightOfWay.conflictAreas.size() == 1, "The click did not author the crossing");
        const auto authored = network().rightOfWay.conflictAreas.front();
        const auto& yielding = authored.priority == ConflictPriority::firstYields ? authored.first : authored.second;
        require(yielding.path.connectorId == t.crossingTurn, "The turn does not give way to the road");
        require(drawn(w, "passive") == 0, "The authored crossing is still drawn as passive");
        require(c->highlightedConflict() == authored.id, "The new area is not selected");
        // A second click cycles it, as on any authored area.
        click(w, crossingPoint);
        require(network().rightOfWay.conflictAreas.front().priority != authored.priority, "A second click did not cycle");
        act(w, "editorUndo")->trigger(); act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(network().rightOfWay.empty() && w.history().revision() == revision, "Authoring was not one Undo step");
        act(w, "editorRedo")->trigger(); QApplication::processEvents();

        // Delete makes it passive again.
        require(network().rightOfWay.conflictAreas.size() == 1, "Redo did not restore the crossing");
        require(w.findChild<QAction*>("editorDeleteConflict")->isEnabled(), "Delete is disabled on the authored crossing");
        act(w, "editorDeleteConflict")->trigger(); QApplication::processEvents();
        require(network().rightOfWay.empty() && drawn(w, "passive") == 2, "Delete did not make the crossing passive");

        // A click on a merge takes it over, one step, with the priority it already ran with.
        const auto beforeMerge = w.history().revision();
        click(w, mergePoint);
        require(network().rightOfWay.conflictAreas.size() == 1 && network().rightOfWay.conflictAreas.front().kind == ConflictKind::merge,
                "The click did not take the merge over");
        require(network().rightOfWay.conflictAreas.front().priority == merge.priority, "The take-over changed the priority");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(w.history().revision() == beforeMerge, "The take-over was not one Undo step");

        // Keyboard: select the passive row and press P.
        int row = -1;
        for (int r = 0; r < table->rowCount(); ++r) if (table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == passive.key) row = r;
        require(row >= 0, "The passive crossing has no row");
        table->setFocus(); table->selectRow(row); QApplication::processEvents();
        require(!w.findChild<QAction*>("editorDeleteConflict")->isEnabled(), "Delete is enabled on an automatic row");
        QTest::keyClick(table, Qt::Key_P); QApplication::processEvents();
        require(network().rightOfWay.conflictAreas.size() == 1 && network().rightOfWay.conflictAreas.front().kind == ConflictKind::crossing,
                "P on the passive row did not author it");

        // Thai: the automatic statuses are translated.
        auto* language = w.findChild<QComboBox*>("editorLanguage");
        int automaticRow = -1;
        for (int r = 0; r < table->rowCount(); ++r) if (table->item(r, 0)->text() == QStringLiteral("—")) automaticRow = r;
        require(automaticRow >= 0, "No automatic row left to translate");
        const auto english = table->item(automaticRow, 6)->text();
        language->setCurrentIndex(1); QApplication::processEvents();
        for (int r = 0; r < table->rowCount(); ++r)
            if (table->item(r, 0)->text() == QStringLiteral("—")) require(table->item(r, 6)->text() != english, "An automatic status is not translated");
        language->setCurrentIndex(0); QApplication::processEvents();

        // Save and reopen: only the authored crossing is in the file; the merges reappear on their own.
        bool asked = false;
        QTimer::singleShot(0, [&] { if (auto* m = QApplication::activeModalWidget()) { asked = true; m->close(); } });
        act(w, "editorSave")->trigger(); QApplication::processEvents();
        require(!asked, "Save asked for a file name");
        {
            QFile f(file); require(f.open(QIODevice::ReadOnly), "Saved file unreadable");
            const auto saved = nlohmann::json::parse(f.readAll().toStdString());
            require(saved["network"]["rightOfWay"]["conflictAreas"].size() == 1, "The file holds more than the authored area");
        }
        w.openFile(file); QApplication::processEvents();
        key(w, Qt::Key_A);
        require(network().rightOfWay.conflictAreas.size() == 1 && drawn(w, "merge") == 4 && drawn(w, "passive") == 0,
                "Reopening did not derive the same automatic areas");
        std::cout << "conflict auto ui tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
