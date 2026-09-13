#pragma once
#include "../core/simulation.hpp"
#include "../eval/summary.hpp"
#include "../project/load.hpp"
#include "../render/network_view.hpp"
#include <QMainWindow>
#include <QElapsedTimer>
#include <QTimer>
#include <QJsonObject>
#include <map>

class QLabel;
class QPushButton;
class QLineEdit;
class QComboBox;
namespace trafficsim {
class MainWindow : public QMainWindow {
public:
    MainWindow(const std::filesystem::path& data, const std::filesystem::path& scenario,
               const QString& language = "en");
    const SimState& state() const { return state_; }
    RunSummary summary() const { return summary_.summary(); }
    bool isRunning() const { return timer_.isActive(); }
    void loadFile(const std::filesystem::path& file);
private:
    QString text(const char* key) const;
    void changeLanguage();
    void refresh();
    void reset();
    void pause();
    void step();
    void tick();
    void seedChanged();
    std::filesystem::path data_;
    LoadedScenario loaded_;
    SimState state_;
    SummaryAccumulator summary_;
    std::map<QString, QJsonObject> locales_;
    QTimer timer_;
    QElapsedTimer elapsed_;
    double credit_{};
    NetworkView* view_{};
    QPushButton *run_{}, *step_{}, *reset_{}, *open_{};
    QLineEdit* seed_{};
    QComboBox *language_{}, *speed_{};
    QLabel *validation_{}, *scope_{}, *error_{};
    std::map<std::string, QLabel*> labels_, values_;
};
}
