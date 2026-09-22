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
struct Shape {
    std::size_t links{}, connectors{}, points{}, selected{};
    bool operator==(const Shape&) const = default;
};
Shape shapeOf(const EditorWindow& w) {
    const auto& n = w.history().document().network;
    Shape s{n.links.size(), n.connectors.size(), 0, w.canvas()->selection().size()};
    for (const auto& l : n.links) s.points += l.geometry.size();
    for (const auto& c : n.connectors) s.points += c.geometry.size();
    return s;
}
void task(const char* name, const char* reference, int count) {
    std::cout << "  " << std::left << std::setw(44) << name << std::setw(3) << count
              << " input" << (count == 1 ? " " : "s") << "   Vissim: " << reference << '\n';
}
// What the reflex did, judged against what the user reaching for it meant. `expected` is the
// document shape that reflex produces in Vissim, translated into this editor's terms.
void reflex(const char* name, const Shape& before, const Shape& after, const Shape& expected) {
    const char* verdict = after == expected ? "transfers"
        : after == before ? "DEAD END -- nothing happened"
        : "WRONG VERB -- it did something else";
    std::cout << "  " << std::left << std::setw(44) << name << verdict;
    if (after != expected && after != before)
        std::cout << " (links " << after.links - before.links << ", points " << after.points - before.points
                  << ", selected " << after.selected << "; wanted links " << expected.links - before.links
                  << ", points " << expected.points - before.points << ", selected " << expected.selected << ")";
    std::cout << '\n';
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
        // Each reflex is judged against the document shape the same reflex produces in Vissim.
        canvas->setSelection({}); click({-75, 0});
        auto before = shapeOf(w);
        Shape duplicated = before; duplicated.links += 1; duplicated.points += 2;
        click({-75, 0}, Qt::LeftButton, Qt::ControlModifier);
        reflex("Ctrl + left-click on the selection", before, shapeOf(w), duplicated);

        canvas->setSelection({}); before = shapeOf(w);
        Shape pointAdded = before; pointAdded.points += 1; pointAdded.selected = 1;
        click({-90, 0}, Qt::RightButton, Qt::ControlModifier);
        reflex("Ctrl + right-click on a link", before, shapeOf(w), pointAdded);

        canvas->setSelection({}); before = shapeOf(w);
        Shape rotated = before; rotated.selected = 1;
        click({-75, 0}); drag({-75, 0}, {-75, 20}, Qt::LeftButton, Qt::AltModifier);
        reflex("Alt + left-drag on the selection", before, shapeOf(w), rotated);

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Walkthrough: " << error.what() << '\n';
        return 1;
    }
}
