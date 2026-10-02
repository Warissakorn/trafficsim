// D102: the Run view draws a lane change as a slide from the lane the vehicle left to the one it is
// on, for 3 s after the engine moved it. Display only: the engine still changes in one tick.
#include "../src/shell/editor_window.hpp"
#include <QAction>
#include <QApplication>
#include <QGraphicsItem>
#include <QStandardPaths>
#include <QTest>
#include <cmath>
#include <iostream>
#include <optional>
#include <vector>
using namespace trafficsim;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Pose { double x{}, y{}, rotation{}; };
std::optional<Pose> poseOf(EditorCanvas& c, std::uint64_t id) {
    for (auto* item : c.scene()->items())
        if (item->data(0).toString() == "run-vehicle" && item->data(2).toULongLong() == id)
            return Pose{item->pos().x(), item->pos().y(), item->rotation()};
    return std::nullopt;
}
const Vehicle* find(const SimState& s, std::uint64_t id) {
    for (const auto& v : s.vehicles) if (v.id == id) return &v;
    return nullptr;
}
// The pose the vehicle would have without the slide: the same frame with its record cleared.
Pose lanePose(EditorWindow& w, std::uint64_t id) {
    auto copy = w.runState();
    for (auto& v : copy.vehicles) if (v.id == id) v.lastLaneChange.reset();
    w.canvas()->setRunFrame(copy);
    const auto pose = poseOf(*w.canvas(), id);
    w.canvas()->setRunFrame(w.runState());
    require(pose.has_value(), "Setup: the vehicle is not drawn without its record");
    return *pose;
}
double apart(Pose a, Pose b) { return std::hypot(a.x - b.x, a.y - b.y); }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 1, "data path");
        const std::filesystem::path data(argv[1]);
        EditorWindow w{data}; w.resize(1200, 800); w.show(); QTest::qWait(30);
        w.openFile(QString::fromStdString((data / "projects/lane-change-lab.traffic.json").string()));
        auto* step = w.findChild<QAction*>("editorStep");
        require(step != nullptr, "No Step action");

        // 1. Step until a vehicle has just changed lanes; the frame right after shows it.
        std::optional<std::uint64_t> changer;
        for (int i = 0; i < 6000 && !changer; ++i) {
            step->trigger();
            for (const auto& v : w.runState().vehicles)
                if (v.lastLaneChange && v.lastLaneChange->tick + 1 == w.runState().tick) { changer = v.id; break; }
        }
        require(changer.has_value(), "Setup: no vehicle changed lanes on the lab");
        const double dt = w.runState().scenario->timeStep;
        const auto startTick = find(w.runState(), *changer)->lastLaneChange->tick;

        // 2. Follow it: drawn off its lane, closer every frame, and on its lane from 3 s on.
        std::vector<double> gaps, turns;
        while (true) {
            const auto* v = find(w.runState(), *changer);
            require(v && v->lastLaneChange && v->lastLaneChange->tick == startTick,
                    "Setup: the changer left or changed again inside the window");
            const auto drawn = poseOf(*w.canvas(), *changer);
            require(drawn.has_value(), "The changer is not drawn");
            const auto lane = lanePose(w, *changer);
            const double elapsed = static_cast<double>(w.runState().tick - startTick) * dt;
            if (elapsed >= 3 - 1e-9) {
                require(apart(*drawn, lane) < 1e-9 && std::abs(drawn->rotation - lane.rotation) < 1e-9,
                        "The slide does not end on the lane after 3 s");
                break;
            }
            gaps.push_back(apart(*drawn, lane));
            if (argc > 2 && gaps.size() == 12) { // optional artifact: mid-slide, zoomed on the changer
                w.canvas()->setTransform(QTransform::fromScale(12, -12)); w.canvas()->centerOn(drawn->x, drawn->y);
                w.canvas()->setRunFrame(w.runState()); QTest::qWait(50);
                require(w.canvas()->grab().save(QString::fromUtf8(argv[2])), "Screenshot failed");
            }
            turns.push_back(std::abs(std::remainder(drawn->rotation - lane.rotation, 360.0)));
            step->trigger();
        }
        // The forcing worked: the first frame is drawn well off the lane the engine put it on.
        require(gaps.size() >= 10, "Setup: too few frames inside the window");
        require(gaps.front() > 2, "The first frame after the change is not drawn near the lane it left");
        require(gaps.front() < 8, "The first frame is drawn further off than the lane it left");
        for (std::size_t i = 1; i < gaps.size(); ++i)
            require(gaps[i] < gaps[i - 1], "The slide does not close on the lane every frame");
        // Mid-slide the nose turns toward the new lane, and only a little.
        const double middle = turns[turns.size() / 2];
        require(middle > 0.5 && middle < 30, "Mid-slide the body is not turned a few degrees toward the new lane");
        std::cout << "lane change display UI tests passed (" << gaps.size() << " frames, from "
                  << gaps.front() << " m)\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
