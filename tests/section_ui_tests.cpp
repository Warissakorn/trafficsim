#include "../src/shell/editor_window.hpp"
#include "../src/model/network/right_of_way.hpp"
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
#include <iostream>
#include <set>
using namespace trafficsim;
// M5.4b (D133, TT11) and M5.5 (D134, L7): travel-time sections in the editor. Two Ctrl+right-clicks with the Section
// tool make one section in one Undo step; the tab renames and deletes it through History; a run
// accepts it; Save and reopen keep it.
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
void author(EditorWindow& w, Point p) { // D84: Ctrl+right-click authors
    QTest::qWait(600);
    QTest::mouseClick(w.canvas()->viewport(), Qt::RightButton, Qt::ControlModifier, pixel(w, p)); QApplication::processEvents();
}
void key(EditorWindow& w, Qt::Key k) { w.canvas()->setFocus(); QTest::keyClick(w.canvas(), k); QApplication::processEvents(); }
// The section marks drawn: "start" or "end" per lane bar, solid ones or the draft's.
std::multiset<std::string> marks(EditorWindow& w, bool draft = false) {
    std::multiset<std::string> found;
    for (auto* i : w.canvas()->scene()->items())
        if (i->data(0).toString() == "travel-time-section" && i->data(1).toString().isEmpty() == draft)
            found.insert(i->data(2).toString().toStdString());
    return found;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto file = temp.filePath("sections.traffic.json");
        require(QFile::copy(QString::fromStdString((std::filesystem::path(argv[1]) / "projects/four-leg-signalised.traffic.json").string()), file),
                "Fixture copy failed");
        QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        auto* c = w.canvas();
        auto* table = w.findChild<QTableWidget*>("editorSectionTable"); require(table, "No section table");
        auto* tabs = w.findChild<QTabWidget*>("editorObjectTabs"); require(tabs, "No object tabs");
        const auto& network = [&]() -> const Network& { return w.history().document().network; };
        require(network().travelTimeSections.empty(), "The template already has a section");

        // West approach (link-1) to east exit (link-20): the west through movement.
        const auto lanePoint = [&](const char* id, double fraction) {
            const auto& link = *std::find_if(network().links.begin(), network().links.end(), [&](const auto& l) { return l.id == id; });
            const auto line = laneGeometry(link, link.lanes.front().id, network().drivingSide);
            return pointAlong(line, polylineLength(line) * fraction);
        };
        const auto from = lanePoint("link-1", 0.75), to = lanePoint("link-20", 0.2);
        c->fitNetwork(); c->centerOn((from.x + to.x) / 2, (from.y + to.y) / 2);
        c->scale(1.5, 1.5); QApplication::processEvents();
        pixel(w, from); pixel(w, to); // both targets are on screen

        // The Section tool (T) shows its tab.
        key(w, Qt::Key_T);
        require(w.findChild<QComboBox*>("editorTool")->currentIndex() == 11, "T did not choose the Section tool");
        require(tabs->currentIndex() == 11, "The tool did not show the Travel-time sections tab");
        const auto revision = w.history().revision();
        // Empty ground is refused. The forcing first: nothing is there.
        const Point empty{from.x, from.y + 60};
        require(c->hitObjects(empty).empty(), "The empty probe point is on a road");
        author(w, empty);
        require(!c->sectionStart(), "A click on empty ground started a section");
        // A start line is a draft drawn across both lanes; Esc drops it and writes nothing.
        author(w, from);
        require(c->sectionStart() && c->sectionStart()->linkId == "link-1", "A click on link-1 did not start a section");
        require(marks(w, true) == std::multiset<std::string>{"start", "start"}, "The draft start is not drawn across both lanes");
        key(w, Qt::Key_Escape);
        require(!c->sectionStart() && w.history().revision() == revision, "Esc did not drop the start");

        // Start, then end: one section, one Undo step.
        author(w, from); author(w, to);
        require(!c->sectionStart(), "The second click did not finish the section");
        require(network().travelTimeSections.size() == 1, "Two clicks did not make one section");
        const auto made = network().travelTimeSections.front();
        require(made.start.linkId == "link-1" && made.end.linkId == "link-20", "The section's lines are not on the clicked Links");
        const auto& link1 = network().links.front();
        const auto bar = *waitingLineBar(network(), {{"link-1", link1.lanes.front().id, {}, {}, {}}, made.start.station});
        require(std::hypot((bar.first.x + bar.second.x) / 2 - from.x, (bar.first.y + bar.second.y) / 2 - from.y) <= 1 / c->transform().m11(),
                "The start is not where the lane was clicked (within the click's pixel)");
        require(marks(w) == std::multiset<std::string>{"start", "start", "end", "end"} && marks(w, true).empty(), "The section is not drawn");
        require(table->rowCount() == 1 && table->currentRow() == 0, "The new section's row is not selected");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(network().travelTimeSections.empty() && w.history().revision() == revision, "Placing was not one Undo step");
        act(w, "editorRedo")->trigger(); QApplication::processEvents();

        // Rename through the dialog: one Undo step.
        table->setFocus(); table->selectRow(0);
        bool ran = false;
        QTimer::singleShot(0, [&] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog && dialog->objectName() == "editorSectionDialog", "No section dialog");
            dialog->findChild<QLineEdit*>("editorSectionName")->setText("West through");
            auto* control = dialog->findChild<QComboBox*>("editorSectionControl"); require(control, "No control type field");
            require(control->currentData().toInt() == -1, "A new section already has a control type");
            control->setCurrentIndex(control->findData(static_cast<int>(SectionControl::signalised)));
            dialog->accept(); ran = true;
        });
        act(w, "editorEditSection")->trigger(); QApplication::processEvents();
        require(ran && network().travelTimeSections.front().name == "West through", "The section was not renamed");
        require(network().travelTimeSections.front().controlType == SectionControl::signalised, "The control type was not set");
        require(table->item(0, 2)->text() == "Signalised", "The tab does not show the control type");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(network().travelTimeSections.front().name.empty() && !network().travelTimeSections.front().controlType,
                "Rename and control type were not one Undo step");
        act(w, "editorRedo")->trigger(); QApplication::processEvents();

        // A run accepts the section. The Results tab is rebuilt only while it can be seen.
        auto* queues = w.findChild<QTableWidget*>("editorQueueTable"); require(queues, "No queue table");
        tabs->setCurrentWidget(w.findChild<QTabWidget*>("editorResultsTabs")->parentWidget()); // the Results page (D144)
        act(w, "editorStep")->trigger(); QApplication::processEvents();
        require(queues->rowCount() > 0, "The run with a section did not report");

        // Delete is one Undo step.
        table->selectRow(0); act(w, "editorDeleteSection")->trigger(); QApplication::processEvents();
        require(network().travelTimeSections.empty() && marks(w).empty(), "Delete did not remove the section");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(network().travelTimeSections.size() == 1, "Delete was not one Undo step");

        // Thai: the tab and its headers are translated.
        auto* language = w.findChild<QComboBox*>("editorLanguage");
        const auto english = tabs->tabText(11);
        language->setCurrentIndex(1); QApplication::processEvents();
        require(!tabs->tabText(11).isEmpty() && tabs->tabText(11) != english, "The tab title is not translated");
        for (int k = 0; k < table->columnCount(); ++k)
            require(!table->horizontalHeaderItem(k)->text().isEmpty(), "A header has no Thai text");
        language->setCurrentIndex(0); QApplication::processEvents();

        // Save through the UI and reopen: the same section, drawn again.
        bool asked = false;
        QTimer::singleShot(0, [&] { if (auto* m = QApplication::activeModalWidget()) { asked = true; m->close(); } });
        act(w, "editorSave")->trigger(); QApplication::processEvents();
        require(!asked, "Save asked for a file name");
        const auto saved = network().travelTimeSections;
        w.openFile(file); QApplication::processEvents();
        require(network().travelTimeSections == saved, "Reopening lost the section");
        require(marks(w) == std::multiset<std::string>{"start", "start", "end", "end"} && table->rowCount() == 1, "The reopened section is not shown");
        std::cout << "section ui tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
