// A counted walkthrough of the editor's authoring gestures, and the gate for M1.27.3.
//
// The other two M1.27 stages measured time. This one measures the thing a Vissim user actually
// pays: how many primitive inputs a task costs, and -- more important -- what happens when they
// reach for the gesture their old tool taught them. A reflex that does NOTHING here is a dead
// end; a reflex that does something ELSE is worse than a missing feature, because it edits the
// drawing while the user believes they did something else.
//
// It is NOT a test and is not in `check`: it reports, it does not assert. Behaviour is guarded
// by the UI suites. What it prints is deterministic -- the same gestures on the same document --
// so two runs are comparable, and that is what a before/after delta needs.
//
//   QT_QPA_PLATFORM=offscreen trafficsim-gesture-walkthrough <data-dir>
#include "../src/shell/editor_window.hpp"
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <cmath>
#include <iomanip>
#include <iostream>
using namespace trafficsim;
namespace {
// One primitive input is one thing the hand does: a click, a drag, a keystroke, or one field
// of a dialog including its confirm button. Counting a drag as one is deliberate -- it is one
// motion, and the whole argument for Vissim's creation chord is that it is a single motion.
int inputs = 0;
EditorCanvas* canvas{};
QPoint pixel(Point p) {
    const auto q = canvas->mapFromScene(p.x, p.y);
    if (!canvas->viewport()->rect().contains(q)) throw std::runtime_error("Gesture outside the viewport");
    return q;
}
void settle() {
    // The offscreen platform has no window manager to reactivate the editor after a modal
    // closes, so supply that activation the way the UI suites do.
    QApplication::setActiveWindow(canvas->window()); canvas->setFocus();
    QTest::keyRelease(canvas, Qt::Key_Control);
    QTest::qWait(10); QApplication::processEvents();
}
void drag(Point a, Point b, Qt::MouseButton button, Qt::KeyboardModifiers modifiers = {}) {
    ++inputs;
    QTest::mousePress(canvas->viewport(), button, modifiers, pixel(a));
    QTest::mouseMove(canvas->viewport(), pixel(b));
    QTest::mouseRelease(canvas->viewport(), button, modifiers, pixel(b));
    settle();
}
void click(Point p, Qt::MouseButton button = Qt::LeftButton, Qt::KeyboardModifiers modifiers = {}) {
    ++inputs;
    QTest::mousePress(canvas->viewport(), button, modifiers, pixel(p));
    QTest::mouseRelease(canvas->viewport(), button, modifiers, pixel(p));
    settle();
}
void doubleClick(Point p) {
    ++inputs;
    QTest::mouseDClick(canvas->viewport(), Qt::LeftButton, {}, pixel(p));
    settle();
}
void key(Qt::Key k, Qt::KeyboardModifiers modifiers = {}) {
    ++inputs; QTest::keyClick(canvas, k, modifiers); QTest::qWait(10); QApplication::processEvents();
}
// Arms an accept for the next modal. `lanes` counts the fields a real user would touch; the
// confirm button is the one that is always paid, so it is charged here and not by the caller.
void accept(const char* expected, int lanes = 0, int fields = 0) {
    inputs += fields + 1;
    auto* timer = new QTimer(qApp);
    QObject::connect(timer, &QTimer::timeout, timer, [timer, expected, lanes] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        timer->stop(); timer->deleteLater();
        if (dialog->objectName() != expected) {
            std::cerr << "Unexpected dialog: " << dialog->objectName().toStdString() << '\n';
            dialog->reject(); return;
        }
        if (lanes) if (auto* count = dialog->findChild<QSpinBox*>("editorGestureLaneCount")) count->setValue(lanes);
        auto* buttons = dialog->findChild<QDialogButtonBox*>();
        if (buttons && buttons->button(QDialogButtonBox::Ok)) buttons->button(QDialogButtonBox::Ok)->click();
        else { std::cerr << "Missing confirmation button\n"; dialog->reject(); }
    });
    timer->start(5);
}
// Enough of the document to tell one verb from another: counts, where the geometry is, and
// what is selected. `span` moves when anything is dragged, which counts alone would miss.
struct Shape {
    std::size_t links{}, connectors{}, points{}, selected{};
    double span{};
    std::string primary;
    bool operator==(const Shape&) const = default;
};
Shape shapeOf(const EditorWindow& w) {
    const auto& n = w.history().document().network;
    Shape s{n.links.size(), n.connectors.size(), 0, w.canvas()->selection().size(), 0,
            w.canvas()->selected()};
    const auto add = [&](const std::vector<Point>& g) {
        s.points += g.size();
        for (const auto& p : g) s.span += p.x + p.y;
    };
    for (const auto& l : n.links) add(l.geometry);
    for (const auto& c : n.connectors) add(c.geometry);
    return s;
}
void task(const char* name, const char* reference, int count) {
    std::cout << "  " << std::left << std::setw(44) << name << std::setw(3) << count
              << " input" << (count == 1 ? " " : "s") << "   Vissim: " << reference << '\n';
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
        if (argc < 2) { std::cerr << "usage: trafficsim-gesture-walkthrough <data-dir>\n"; return 2; }
        QTemporaryDir directory;
        if (!directory.isValid()) throw std::runtime_error("Temporary directory");
        qputenv("XDG_DATA_HOME", directory.path().toUtf8());
        EditorWindow w{std::filesystem::path(argv[1])};
        w.resize(1600, 1000); w.show(); QTest::qWait(30);
        canvas = w.canvas();
        canvas->fitInView(QRectF(-140, -70, 280, 140), Qt::KeepAspectRatio); canvas->centerOn(0, 0);
        std::cout << "Counted walkthrough of the authoring gestures (M1.27.3)\n\n"
                     "One input is one click, one drag, one keystroke, or one dialog field.\n\n"
                     "Task cost, native path\n";

        // 1. Two three-lane links, the smallest drawing a connector can join.
        inputs = 0;
        key(Qt::Key_L); accept("editorLinkDialog", 3, 1); drag({-120, 0}, {-30, 0}, Qt::RightButton, Qt::ControlModifier);
        task("Draw a 3-lane link", "1 drag + Link Data dialog", inputs);
        accept("editorLinkDialog", 3, 1); drag({30, 0}, {120, 0}, Qt::RightButton, Qt::ControlModifier);
        const auto& network = w.history().document().network;
        if (network.links.size() != 2) throw std::runtime_error("The walkthrough needs two links");

        // 2. Join them across the whole carriageway in one gesture.
        inputs = 0;
        key(Qt::Key_C);
        const auto& left = network.links.front();
        const auto from = laneAttachment(network, LaneReference{left.id, left.lanes.front().id}, true);
        const auto& right = network.links.back();
        const auto to = laneAttachment(network, LaneReference{right.id, right.lanes.front().id}, false);
        accept("editorRangeDialog", 0, 0); drag(from, to, Qt::RightButton, Qt::ControlModifier);
        task("Connect two 3-lane links (all 3 lanes)", "1 drag + Connector dialog", inputs);
        if (network.connectors.size() != 1) throw std::runtime_error("The walkthrough needs a connector");

        inputs = 0; key(Qt::Key_S); click({-75, 0}); doubleClick({-60, 0});
        task("Add a curve point to a link", "Ctrl + right-click on the link", inputs);

        inputs = 0; click({-75, 0}); drag({-75, 0}, {-75, 30}, Qt::LeftButton, Qt::ControlModifier);
        task("Duplicate a link", "Ctrl + left-click on the selection", inputs);

        inputs = 0; click({-75, 0}); drag({-75, 0}, {-75, 20}, Qt::LeftButton, Qt::AltModifier);
        task("Rotate a link", "Alt + left-drag on the selection", inputs);

        std::cout << "\nVissim reflexes, replayed here\n";
        // Each row states, as a predicate, what the reflex means in Vissim translated into this
        // document. `transferred` is that predicate; `changed` separates a dead end from a wrong
        // verb, because doing nothing and doing the wrong thing are not the same failure. The
        // order matters: the rows that move geometry come last, so no row measures the one above.
        const auto reflex = [](const char* name, bool transferred, bool changed) {
            std::cout << "  " << std::left << std::setw(44) << name
                      << (transferred ? "transfers"
                          : changed ? "WRONG VERB -- it did something else"
                                    : "DEAD END -- nothing happened") << '\n';
        };
        const auto geometryOf = [&w](const std::string& id) {
            for (const auto& l : w.history().document().network.links) if (l.id == id) return l.geometry;
            return std::vector<Point>{};
        };
        const auto last = w.history().document().network.links.back().id;

        canvas->setSelection({}); click({-75, 0});
        auto before = shapeOf(w);
        click({-75, 0}, Qt::LeftButton, Qt::ControlModifier);
        auto after = shapeOf(w);
        // Vissim duplicates the selection: one more Link, and the copy becomes the selection.
        reflex("Ctrl + left-click on the selection",
               after.links == before.links + 1 && after.selected == 1, after != before);

        before = shapeOf(w);
        const auto centre = canvas->mapToScene(canvas->viewport()->rect().center());
        drag({-75, 0}, {-55, 10}, Qt::RightButton);
        const bool panned = canvas->mapToScene(canvas->viewport()->rect().center()) != centre;
        reflex("Right-drag", panned && shapeOf(w) == before, !panned || shapeOf(w) != before);

        canvas->setSelection({}); before = shapeOf(w);
        click({-90, 0}, Qt::RightButton, Qt::ControlModifier);
        after = shapeOf(w);
        reflex("Ctrl + right-click on a link",
               after.points == before.points + 1 && after.links == before.links, after != before);

        // Tab reaches the object behind, so it needs two objects at one point: draw a Link
        // across the first one, which is what overlaps in a real drawing.
        accept("editorLinkDialog", 3, 1); drag({-75, -25}, {-75, 25}, Qt::RightButton, Qt::ControlModifier);
        const auto crossing = w.history().document().network.links.back().id;
        canvas->setSelection({}); click({-75, 0}); before = shapeOf(w);
        key(Qt::Key_Tab);
        after = shapeOf(w);
        reflex("Tab at the pointer",
               after.primary != before.primary && !after.primary.empty() &&
               after.links == before.links && after.span == before.span,
               after != before);

        // Two Links that no Connector joins: moving only one end of a Connector detaches it,
        // which is a different behaviour and would be measured here by accident.
        canvas->setSelection({last, crossing});
        before = shapeOf(w);
        const auto lastWas = geometryOf(last), crossingWas = geometryOf(crossing);
        drag({-100, 30}, {-100, 40}, Qt::LeftButton);
        after = shapeOf(w);
        // BOTH selected Links travel 10 m in y. A gesture that moved only the one under the
        // pointer, or that changed the drawing's object count, fails this.
        const auto travelled = [](const std::vector<Point>& was, const std::vector<Point>& now) {
            if (was.size() != now.size() || was.empty()) return false;
            for (std::size_t i = 0; i < was.size(); ++i)
                if (std::abs(now[i].x - was[i].x) > 0.01 || std::abs(now[i].y - was[i].y - 10) > 0.01) return false;
            return true;
        };
        reflex("Drag a multi-selection",
               after.links == before.links && after.connectors == before.connectors &&
               travelled(lastWas, geometryOf(last)) && travelled(crossingWas, geometryOf(crossing)),
               after != before);

        // Rotate the crossing Link, which no Connector attaches to. Grab it away from its own
        // centre: an angle has no stable direction at the pivot, and the editor refuses it there.
        canvas->setSelection({}); click({-75, 28}); before = shapeOf(w);
        const auto crossingBefore = geometryOf(crossing);
        drag({-75, 28}, {-60, 28}, Qt::LeftButton, Qt::AltModifier);
        after = shapeOf(w);
        reflex("Alt + left-drag on the selection",
               after.links == before.links && after.connectors == before.connectors &&
               after.points == before.points && geometryOf(crossing) != crossingBefore,
               after != before);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Walkthrough: " << error.what() << '\n';
        return 1;
    }
}
