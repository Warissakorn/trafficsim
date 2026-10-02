// Wireframe display (Ctrl+A, Vissim's simple link display): Links and Connectors draw, hit and
// select as their centre line; vehicles keep their lane positions; the project is untouched.
#include "../src/shell/editor_window.hpp"
#include "../src/model/network/network.hpp"
#include <QAction>
#include <QApplication>
#include <QGraphicsItem>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTest>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <tuple>
#include <vector>
using namespace trafficsim;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int count(EditorCanvas& c, const char* tag) {
    int n = 0; for (auto* item : c.scene()->items()) if (item->data(0).toString() == tag) ++n; return n;
}
bool hits(EditorCanvas& c, Point p, const std::string& id) {
    const auto found = c.hitObjects(p);
    return std::any_of(found.begin(), found.end(), [&](const auto& h) { return h.first == id; });
}
using Pose = std::tuple<double, double, double>;
std::vector<Pose> vehicles(EditorCanvas& c) {
    std::vector<Pose> poses;
    for (auto* item : c.scene()->items()) if (item->data(0).toString() == "run-vehicle")
        poses.emplace_back(item->pos().x(), item->pos().y(), item->rotation());
    std::sort(poses.begin(), poses.end()); return poses;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 1, "data path");
        const std::filesystem::path data(argv[1]);
        EditorWindow w{data}; w.resize(1400, 900); w.show(); QTest::qWait(30);
        w.openFile(QString::fromStdString((data / "projects/m2.6-study-template.traffic.json").string()));
        auto* c = w.canvas(); auto* toggle = w.findChild<QAction*>("editorToggleWireframe");
        require(toggle && toggle->isCheckable() && !toggle->isChecked(), "No unchecked wireframe action");
        const auto& network = w.history().document().network;
        require(count(*c, "road-surface") > 0 && count(*c, "road-marking") > 0 && count(*c, "centre-line") == 0,
                "The normal display is not lane surfaces and markings");
        const auto revision = w.history().revision();

        // 1. Ctrl+A on the canvas swaps every surface and marking for one centre line per object.
        c->setFocus(); QTest::keyClick(c, Qt::Key_A, Qt::ControlModifier);
        require(toggle->isChecked() && c->wireframe(), "Ctrl+A did not turn the wireframe on");
        require(count(*c, "road-surface") == 0 && count(*c, "road-marking") == 0 && count(*c, "connector-mouth-edge") == 0,
                "Wireframe still draws lane surfaces or markings");
        require(count(*c, "centre-line") == static_cast<int>(network.links.size() + network.connectors.size()),
                "Wireframe does not draw exactly one centre line per Link and Connector");

        // 2. What is hit is what is drawn. A lane centre 1.75 m off a two-lane Link's centre line
        // hits the Link normally (asserted first, so the forcing worked) and misses in wireframe.
        const auto wide = std::find_if(network.links.begin(), network.links.end(), [](const Link& l) { return l.lanes.size() == 2; });
        require(wide != network.links.end(), "The template has no two-lane Link");
        const auto side = network.drivingSide;
        const auto centre = linkCentreline(*wide, side), lane = laneGeometry(*wide, wide->lanes.front().id, side);
        const auto onLine = pointAlong(centre, polylineLength(centre) / 2), onLane = pointAlong(lane, polylineLength(lane) / 2);
        c->setTransform(QTransform::fromScale(8, -8)); c->centerOn(onLine.x, onLine.y);
        c->setWireframe(false);
        require(hits(*c, onLane, wide->id), "Setup: the lane centre does not hit its Link in the normal display");
        c->setWireframe(true);
        require(!hits(*c, onLane, wide->id), "Wireframe hits the invisible carriageway");
        require(hits(*c, onLine, wide->id), "Wireframe misses its own centre line");

        // 3. A click on the line selects it; the feedback rides the line and no lane tab appears.
        QTest::mouseClick(c->viewport(), Qt::LeftButton, {}, c->mapFromScene(onLine.x, onLine.y));
        require(c->selected() == wide->id, "Clicking the centre line did not select the Link");
        bool selected = false;
        for (auto* item : c->scene()->items())
            if (item->data(0).toString() == "object-feedback" && item->data(1).toString().toStdString() == wide->id
                && item->data(2).toString() == "selected") selected = true;
        require(selected, "The selected centre line shows no selection feedback");
        c->setWireframe(false);
        require(c->selected() == wide->id && count(*c, "lane-resize") > 0, "Setup: a selected Link shows no lane tabs normally");
        c->setWireframe(true);
        require(count(*c, "lane-resize") == 0, "Wireframe offers lane tabs on rails it does not draw");

        // 4. Vehicles stay in their lanes: the same frame places them identically either way.
        auto* step = w.findChild<QAction*>("editorStep");
        for (int i = 0; i < 1200 && w.runState().vehicles.size() < 5; ++i) step->trigger();
        QApplication::processEvents();
        const auto wire = vehicles(*c);
        require(wire.size() >= 5, "Setup: the run placed too few vehicles to compare");
        c->setWireframe(false);
        require(vehicles(*c) == wire, "Wireframe moved vehicles off their lane positions");
        c->setWireframe(true);
        // Captured mid-run, so the artifact shows centre lines with vehicles in their lanes.
        if (argc > 2) {
            c->clearSelection(); c->fitNetwork();
            const auto middle = c->mapToScene(c->viewport()->rect().center());
            c->scale(5, 5); c->centerOn(middle); c->redraw(); QTest::qWait(50);
            require(w.grab().save(QString::fromUtf8(argv[2])), "Screenshot failed");
        }

        // 5. View state only: toggling leaves the document's revision alone.
        require(w.history().revision() == revision, "Toggling the wireframe changed the project");

        // 6. A focused text field keeps Ctrl+A as select-all; the toggle does not move.
        auto* field = new QLineEdit("abc", &w); field->show(); field->setFocus(); QApplication::processEvents();
        QTest::keyClick(field, Qt::Key_A, Qt::ControlModifier);
        require(field->selectedText() == "abc", "Ctrl+A in a text field did not select its text");
        require(toggle->isChecked(), "Ctrl+A in a text field toggled the wireframe");
        std::cout << "wireframe UI tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
