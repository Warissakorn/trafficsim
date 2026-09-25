#include "../src/shell/editor_window.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/model/network/right_of_way.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QGraphicsPathItem>
#include <QListWidget>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <cmath>
#include <iostream>
using namespace trafficsim;
// M3.2.4b: conflict areas on the canvas (A24, docs/VISSIM_PARITY.md §2b). The Conflict area tool
// picks and cycles areas and drags waiting lines; Select still picks the Link underneath; the
// two sides of a crossing stay readable; what was edited survives Save and reopen.
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void write(const QString& path, const ProjectDocument& d) {
    QFile f(path); require(f.open(QIODevice::WriteOnly), "Fixture open failed");
    const auto bytes = QByteArray::fromStdString(documentJson(d).dump());
    require(f.write(bytes) == bytes.size(), "Fixture write failed");
}
QAction* act(EditorWindow& w, const char* name) {
    auto* a = w.findChild<QAction*>(name); require(a, name); return a;
}
Point centre(const std::vector<Point>& outline) {
    Point c{};
    for (const auto& p : outline) { c.x += p.x / outline.size(); c.y += p.y / outline.size(); }
    return c;
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
void drag(EditorWindow& w, QPoint from, QPoint to) {
    QTest::qWait(600);
    QTest::mousePress(w.canvas()->viewport(), Qt::LeftButton, {}, from);
    for (int k = 1; k <= 5; ++k) QTest::mouseMove(w.canvas()->viewport(), from + (to - from) * k / 5);
    QTest::mouseRelease(w.canvas()->viewport(), Qt::LeftButton, {}, to); QApplication::processEvents();
}
ConflictPriority priority(EditorWindow& w, const std::string& id) {
    for (const auto& a : w.history().document().network.rightOfWay.conflictAreas) if (a.id == id) return a.priority;
    throw std::runtime_error("area gone");
}
std::vector<QGraphicsPathItem*> sides(EditorWindow& w, const std::string& id) {
    std::vector<QGraphicsPathItem*> found;
    for (auto* i : w.canvas()->scene()->items())
        if (i->data(0).toString() == "conflict-area" && i->data(1).toString().toStdString() == id)
            if (auto* path = dynamic_cast<QGraphicsPathItem*>(i)) found.push_back(path);
    return found;
}
int drawn(EditorWindow& w) {
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == "conflict-area" || i->data(0).toString() == "waiting-line";
    return n;
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
        const auto areas = addCrossingAreas(d, a, b, b, {3, 7}); // B gives way
        require(areas.size() == 2, "Fixture did not make two crossing areas");
        const auto file = temp.filePath("canvas.traffic.json"); write(file, d);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        auto* c = w.canvas();
        c->fitNetwork(); c->scale(4, 4); c->centerOn(100, 0); QApplication::processEvents();
        auto* table = w.findChild<QTableWidget*>("editorConflictTable"); require(table, "No conflict table");
        auto* tabs = w.findChild<QTabWidget*>("editorObjectTabs"); require(tabs, "No object tabs");
        const auto& network = [&]() -> const Network& { return w.history().document().network; };
        const auto first = network().rightOfWay.conflictAreas.front();
        const auto inside = centre(conflictSideOutline(network(), first.second));

        // Select never hit-tests areas: at the crossing it still picks a Link. The forcing first:
        // the point really is inside an area.
        require(c->conflictsAt(inside) == std::vector<std::string>{first.id}, "The probe point is not inside the first area");
        click(w, inside);
        require(c->selected() == a || c->selected() == b, "Select did not pick a Link at the crossing");
        require(c->highlightedConflict().empty(), "Select picked a conflict area");

        // The Conflict area tool (A): a click picks the area, a second click cycles, P cycles.
        QTest::keyClick(c, Qt::Key_A); QApplication::processEvents();
        require(w.findChild<QComboBox*>("editorTool")->currentIndex() == 9, "A did not choose the Conflict area tool");
        require(tabs->currentIndex() == 9, "The tool did not show the Conflict areas tab");
        const auto revision = w.history().revision();
        click(w, inside);
        require(c->highlightedConflict() == first.id, "A click did not pick the area");
        require(table->item(table->currentRow(), 0)->data(Qt::UserRole).toString().toStdString() == first.id, "The row was not selected");
        require(w.history().revision() == revision, "Picking an area edited the document");
        click(w, inside);
        require(priority(w, first.id) == ConflictPriority::undetermined, "A second click did not cycle the priority");
        c->setFocus(); QTest::keyClick(c, Qt::Key_P); QApplication::processEvents();
        require(priority(w, first.id) == ConflictPriority::firstYields, "P did not cycle the priority");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(priority(w, first.id) == ConflictPriority::undetermined, "P was not one Undo step");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(priority(w, first.id) == ConflictPriority::secondYields && w.history().revision() == revision, "The click was not one Undo step");
        click(w, {115, 15});
        require(w.history().revision() == revision && c->highlightedConflict() == first.id, "A click on empty space changed something");

        // Both sides stay readable: the one that gives way is hatched and drawn above the other.
        const auto items = sides(w, first.id);
        require(items.size() == 2, "The area did not draw two sides");
        const auto* hatched = items[0]->brush().style() == Qt::BDiagPattern ? items[0] : items[1];
        const auto* solid = hatched == items[0] ? items[1] : items[0];
        require(hatched->brush().style() == Qt::BDiagPattern && solid->brush().style() == Qt::SolidPattern, "The sides were not told apart");
        require(hatched->zValue() > solid->zValue(), "The hatched side is not drawn above the solid one");

        // Drag B's waiting line 5 m upstream, along its lane: one Undo step. A jitter moves nothing.
        const auto line = *std::find_if(network().rightOfWay.waitingLines.begin(), network().rightOfWay.waitingLines.end(),
                                        [&](const auto& l) { return l.id == first.second.waitingLineId; });
        const auto bar = *waitingLineBar(network(), line.point);
        const Point mid{(bar.first.x + bar.second.x) / 2, (bar.first.y + bar.second.y) / 2};
        const auto before = w.history().revision();
        drag(w, pixel(w, mid), pixel(w, mid) + QPoint(2, 1));
        require(w.history().revision() == before, "A jitter moved the waiting line");
        drag(w, pixel(w, mid), pixel(w, {mid.x + 0.7, mid.y - 5})); // sideways drift stays on the lane
        const auto moved = std::find_if(network().rightOfWay.waitingLines.begin(), network().rightOfWay.waitingLines.end(),
                                        [&](const auto& l) { return l.id == line.id; });
        require(moved->point.path == line.point.path, "The drag moved the line to another lane");
        require(std::abs(moved->point.station - (line.point.station - 5)) < .2, "The drag did not slide the line 5 m");
        require(priority(w, first.id) == ConflictPriority::secondYields, "The drag cycled the area underneath");
        const auto dragged = w.history().document();
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(w.history().revision() == before, "The drag was not one Undo step");
        act(w, "editorRedo")->trigger(); QApplication::processEvents();
        require(w.history().document() == dragged, "Redo did not restore the drag");

        // Save through the UI -- no dialog for a file already named -- and reopen it.
        bool asked = false;
        QTimer::singleShot(0, [&] { if (auto* m = QApplication::activeModalWidget()) { asked = true; m->close(); } });
        act(w, "editorSave")->trigger(); QApplication::processEvents();
        require(!asked, "Save asked for a file name");
        const auto saved = network().rightOfWay;
        const int shown = drawn(w);
        w.openFile(file); QApplication::processEvents();
        require(network().rightOfWay == saved, "Reopening lost a conflict edit");
        require(table->rowCount() == 2 && drawn(w) == shown, "Reopening did not show the same areas");
        for (int r = 0; r < table->rowCount(); ++r) require(table->item(r, 6)->text() == "Runs", "A reopened area does not run");
        std::cout << "priority canvas tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
