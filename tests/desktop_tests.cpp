#include "../src/shell/main_window.hpp"
#include "../src/shell/path.hpp"
#include "../src/project/document.hpp"
#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
#include <cmath>
#include <fstream>
#include <iostream>

namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class T> T* widget(trafficsim::MainWindow& w, const char* name) {
    auto* value = w.findChild<T*>(name);
    require(value != nullptr, "Widget missing"); return value;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
        require(argc >= 2, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        const auto thaiPath = QString::fromUtf8("โครงการ/ทางแยก.json");
        require(trafficsim::displayPath(trafficsim::nativePath(thaiPath)) == thaiPath, "Unicode path roundtrip failed");
        trafficsim::MainWindow window(data, data / "scenarios/crossing.json");
        window.show(); QTest::qWait(50);
        auto* run = widget<QPushButton>(window, "run"); auto* step = widget<QPushButton>(window, "step");
        auto* reset = widget<QPushButton>(window, "reset"); auto* seed = widget<QLineEdit>(window, "seed");
        step->click(); require(window.state().tick == 1, "Step must advance one tick");
        run->click(); QTest::qWait(250);
        require(window.state().tick > 1, "Run must advance traffic");
        run->click(); require(!window.isRunning(), "Pause must stop timer");
        const auto paused = window.state().tick; QTest::qWait(150);
        require(window.state().tick == paused, "Paused scene advanced");
        seed->setText("4294967296"); require(!run->isEnabled() && !step->isEnabled(), "Overflow seed accepted");
        seed->setText("bad"); require(!run->isEnabled(), "Invalid seed accepted");
        seed->setText("43"); require(window.state().tick == 0 && window.state().seed == 43, "Seed change did not reset");
        auto* language = widget<QComboBox>(window, "language"); language->setCurrentIndex(1);
        require(run->text() == QString::fromUtf8("เริ่มจำลอง"), "Thai run label missing");
        require(widget<QLabel>(window, "validation")->text().contains(QString::fromUtf8("ยังไม่ผ่าน")), "Validation marker missing");
        language->setCurrentIndex(0); require(run->text() == "Run", "English switch failed");
        seed->setText("42");
        for (int i = 0; i < 1800; ++i) step->click();
        require(window.state().completed == 31, "Desktop run differs from CLI");
        require(std::abs(*window.summary().meanDelay - 29.249359418430977) < 1e-7, "Desktop summary differs from baseline");
        require(!run->isEnabled() && !step->isEnabled() && !window.isRunning(), "Completed run still enabled");
        reset->click(); require(window.state().tick == 0 && run->isEnabled(), "Reset failed");
        const auto before = window.state().scenario;
        bool rejected = false;
        try { window.loadFile(data / "missing.json"); }
        catch (const std::exception&) { rejected = true; }
        require(rejected, "Missing file accepted");
        require(window.state().scenario == before, "Failed load changed current scenario");
        {   // A network drawn in the editor is a project, not a runnable scenario: this window
            // must say which kind of file it is, never relay a JSON parser exception.
            const trafficsim::ProjectDocument drawn; // an empty network is a legal draft
            const auto path = std::filesystem::temp_directory_path() / "trafficsim-desktop-tests" / "network.traffic.json";
            std::filesystem::create_directories(path.parent_path());
            std::ofstream(path) << trafficsim::documentJson(drawn).dump(2);
            std::string code;
            try { window.loadFile(path); }
            catch (const trafficsim::ScenarioLoadError& error) { code = error.code; }
            require(code == "SCENARIO_IS_PROJECT", "Editor project was not recognised as a project");
            require(window.state().scenario == before, "Rejected project changed current scenario");
            std::filesystem::remove_all(path.parent_path());
        }
        window.resize(640, 760); language->setCurrentIndex(1); QTest::qWait(50);
        require(window.centralWidget()->geometry().width() <= window.width(), "Layout overflow");
        if (argc > 2) {
            for (int i = 0; i < 250; ++i) step->click();
            QTest::qWait(50);
            require(window.grab().save(QString::fromUtf8(argv[2])), "Screenshot failed");
        }
        std::cout << "Desktop controls, translation, replay and failed-load preservation passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
