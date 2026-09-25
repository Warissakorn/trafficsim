#include "../src/shell/editor_window.hpp"
#include "../src/model/network/right_of_way.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QFile>
#include <QGraphicsItem>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <set>
using namespace trafficsim;
// M3.2.6c: queue counters in the editor (A24, docs/VISSIM_PARITY.md §2b). The Queue counter tool
// builds a counter from clicks and commits it on Enter; the tab adds one over the selected heads,
// renames and deletes it; each is one Undo step. The Results tab shows the authored row in place
// of the derived one, and everything survives Save and reopen.
namespace {
// Printed before the throw: a window left with unsaved edits can abort while it unwinds.
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
QAction* act(EditorWindow& w, const char* name) {
    auto* a = w.findChild<QAction*>(name); require(a, name); return a;
}
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
void onDialog(const char* name, std::function<void(QDialog*)> answer, bool& ran) {
    QTimer::singleShot(0, [=, &ran] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        require(dialog && dialog->objectName() == name, name);
        answer(dialog); ran = true;
    });
}
// The kinds of counter mark drawn, in no particular order: "head", "line" or "point" per line.
std::multiset<std::string> marks(EditorWindow& w, bool draft = false) {
    std::multiset<std::string> found;
    for (auto* i : w.canvas()->scene()->items())
        if (i->data(0).toString() == "queue-counter" && i->data(1).toString().isEmpty() == draft)
            found.insert(i->data(2).toString().toStdString());
    return found;
}
QStringList column(QTableWidget* t, int c) {
    QStringList values;
    for (int r = 0; r < t->rowCount(); ++r) values << t->item(r, c)->text();
    return values;
}
QStringList queueRows(EditorWindow& w) {
    act(w, "editorStep")->trigger(); QApplication::processEvents();
    auto* t = w.findChild<QTableWidget*>("editorQueueTable"); require(t, "No queue table");
    return column(t, 0);
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto file = temp.filePath("counters.traffic.json");
        require(QFile::copy(QString::fromStdString((std::filesystem::path(argv[1]) / "projects/four-leg-signalised.traffic.json").string()), file),
                "Fixture copy failed");
        QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        auto* c = w.canvas();
        auto* table = w.findChild<QTableWidget*>("editorCounterTable"); require(table, "No counter table");
        auto* tabs = w.findChild<QTabWidget*>("editorObjectTabs"); require(tabs, "No object tabs");
        const auto& network = [&]() -> const Network& { return w.history().document().network; };
        require(network().queueCounters.empty(), "The template already has a counter");

        // The west approach's pocket (link-4) has three heads; link-1 upstream of it has none.
        std::vector<const NetworkSignalHead*> heads;
        for (const auto& h : network().signalHeads) if (h.lane.linkId == "link-4") heads.push_back(&h);
        require(heads.size() == 3, "The pocket does not have three heads");
        const auto stop = [&](const NetworkSignalHead& h) { const auto s = headSlot(network(), h); return pointAlong(s->geometry, h.position); };
        const auto& upstream = *std::find_if(network().links.begin(), network().links.end(), [](const auto& l) { return l.id == "link-1"; });
        const auto laneLine = laneGeometry(upstream, upstream.lanes.front().id, network().drivingSide);
        const auto onLane = pointAlong(laneLine, polylineLength(laneLine) / 2);
        const Point centre{(stop(*heads[0]).x + onLane.x) / 2, (stop(*heads[0]).y + onLane.y) / 2};
        c->fitNetwork(); c->centerOn(centre.x, centre.y);
        c->scale(3, 3); QApplication::processEvents();
        for (const auto* h : heads) pixel(w, stop(*h)); // every target is on screen
        pixel(w, onLane);

        // The derived row, before any counter: one run step lists it.
        const auto derived = queueRows(w);
        require(derived.contains("West approach, right-turn pocket"), "The pocket has no derived row");

        // The Queue counter tool (Q) shows its tab.
        key(w, Qt::Key_Q);
        require(w.findChild<QComboBox*>("editorTool")->currentIndex() == 10, "Q did not choose the Queue counter tool");
        require(tabs->currentIndex() == 10, "The tool did not show the Queue counters tab");
        const auto revision = w.history().revision();
        // Empty ground is refused. The forcing first: nothing is there.
        const Point empty{centre.x, centre.y + 40};
        require(c->hitObjects(empty).empty(), "The empty probe point is on a road");
        click(w, empty);
        require(c->counterDraft().empty(), "A click on empty ground added a line");
        // Esc drops a draft, and nothing is written.
        click(w, stop(*heads[0]));
        require(c->counterDraft().size() == 1, "A click on a stop line did not add a line");
        key(w, Qt::Key_Escape);
        require(c->counterDraft().empty() && w.history().revision() == revision, "Esc did not drop the draft");

        // Three stop lines and a lane place; Backspace drops the last; Enter commits once.
        for (const auto* h : heads) click(w, stop(*h));
        click(w, stop(*heads[1])); // the same head twice is one line
        click(w, onLane);
        require(c->counterDraft().size() == 4, "The draft does not hold four lines");
        require(marks(w, true) == std::multiset<std::string>{"head", "head", "head", "point"}, "The draft is not drawn");
        key(w, Qt::Key_Backspace);
        require(c->counterDraft().size() == 3, "Backspace did not drop the last line");
        click(w, onLane);
        require(w.history().revision() == revision, "The draft wrote to the document");
        key(w, Qt::Key_Return);
        require(network().queueCounters.size() == 1, "Enter did not create one counter");
        const auto created = network().queueCounters.front();
        for (std::size_t k = 0; k < 3; ++k)
            require(created.lines[k].referenceId == heads[k]->id && !created.lines[k].point, "A stop-line click is not a head reference");
        const auto& point = created.lines[3].point;
        require(point && point->path.linkId == "link-1", "The lane click is not a point on link-1");
        const auto bar = *waitingLineBar(network(), *point);
        require(std::hypot((bar.first.x + bar.second.x) / 2 - onLane.x, (bar.first.y + bar.second.y) / 2 - onLane.y) <= 1 / c->transform().m11(),
                "The point is not where the lane was clicked (within the click's pixel)");
        require(marks(w) == std::multiset<std::string>{"head", "head", "head", "point"} && marks(w, true).empty(), "The counter is not drawn");
        require(table->rowCount() == 1 && table->item(0, 3)->text() == "West approach, right-turn pocket", "The tab does not say what the row replaces");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(network().queueCounters.empty() && w.history().revision() == revision, "Creating was not one Undo step");
        act(w, "editorRedo")->trigger(); QApplication::processEvents();

        // Rename it through the dialog (Enter on the row opens the same one): one Undo step.
        table->setFocus(); table->selectRow(0);
        bool ran = false;
        onDialog("editorCounterDialog", [&](QDialog* dialog) {
            dialog->findChild<QLineEdit*>("editorCounterName")->setText("Counted"); dialog->accept(); }, ran);
        act(w, "editorEditCounter")->trigger(); QApplication::processEvents();
        require(ran && network().queueCounters.front().name == "Counted", "The counter was not renamed");

        // Results: the authored row stands in for the derived one, and no row is added.
        const auto rows = queueRows(w);
        require(rows.contains("Counted") && !rows.contains("West approach, right-turn pocket"), "The authored row did not replace the derived one");
        require(rows.size() == derived.size(), "The report has a different number of approach rows");

        // Keyboard: a head chosen in the Signal heads table, then Add queue counter.
        auto* signalTable = w.findChild<QTableWidget*>("editorSignalTable"); require(signalTable, "No signal table");
        const auto east = std::find_if(network().signalHeads.begin(), network().signalHeads.end(), [](const auto& h) { return h.lane.linkId == "link-16"; })->id;
        for (int r = 0; r < signalTable->rowCount(); ++r)
            if (signalTable->item(r, 0)->text().toStdString() == east) signalTable->selectRow(r);
        QApplication::processEvents();
        require(c->selected() == east, "The table did not select the head");
        require(act(w, "editorAddCounter")->isEnabled(), "Add queue counter is disabled with a head selected");
        act(w, "editorAddCounter")->trigger(); QApplication::processEvents();
        require(network().queueCounters.size() == 2 && network().queueCounters.back().lines == std::vector<MeasurementLine>{{east, std::nullopt}},
                "Add queue counter did not measure the selected head");
        require(tabs->currentIndex() == 10 && table->currentRow() == 1, "The new counter's row is not selected");
        act(w, "editorDeleteCounter")->trigger(); QApplication::processEvents();
        require(network().queueCounters.size() == 1, "Delete did not remove the counter");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(network().queueCounters.size() == 2, "Delete was not one Undo step");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(network().queueCounters.size() == 1, "Add was not one Undo step");
        // Deleting the pointer-made counter brings the derived row back.
        table->selectRow(0); act(w, "editorDeleteCounter")->trigger(); QApplication::processEvents();
        require(queueRows(w) == derived, "Deleting the counter did not restore the derived rows");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();

        // Thai: the tab and its headers are translated.
        auto* language = w.findChild<QComboBox*>("editorLanguage");
        const auto english = tabs->tabText(10);
        language->setCurrentIndex(1); QApplication::processEvents();
        require(!tabs->tabText(10).isEmpty() && tabs->tabText(10) != english, "The tab title is not translated");
        for (int k = 0; k < table->columnCount(); ++k)
            require(!table->horizontalHeaderItem(k)->text().isEmpty(), "A header has no Thai text");
        language->setCurrentIndex(0); QApplication::processEvents();

        // Save through the UI and reopen: the same counter, drawn again, still replacing the row.
        bool asked = false;
        QTimer::singleShot(0, [&] { if (auto* m = QApplication::activeModalWidget()) { asked = true; m->close(); } });
        act(w, "editorSave")->trigger(); QApplication::processEvents();
        require(!asked, "Save asked for a file name");
        const auto saved = network().queueCounters;
        w.openFile(file); QApplication::processEvents();
        require(network().queueCounters == saved, "Reopening lost the counter");
        require(marks(w) == std::multiset<std::string>{"head", "head", "head", "point"} && table->rowCount() == 1, "The reopened counter is not shown");
        const auto reopened = queueRows(w);
        require(reopened.contains("Counted") && !reopened.contains("West approach, right-turn pocket"), "The reopened counter does not run");
        std::cout << "queue counter ui tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
