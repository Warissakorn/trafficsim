#pragma once
#include "../commands/network_commands.hpp"
#include "../commands/connector_commands.hpp"
#include "../project/diagnostics.hpp"
#include "../project/run.hpp"
#include "../project/display.hpp"
#include "../commands/appearance_commands.hpp"
#include "../commands/demand_commands.hpp"
#include <QTimer>
#include <QElapsedTimer>
#include "../core/validate.hpp"
#include "../editor/canvas.hpp"
#include <QMainWindow>
#include <QJsonObject>
#include <map>
#include <filesystem>
#include <QKeySequence>

class QListWidget;
class QLockFile;
class QAction;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QLabel;
class QFormLayout;
class QCloseEvent;
class QTabWidget;
class QTableWidget;
namespace trafficsim {
class EditorWindow : public QMainWindow {
public:
    explicit EditorWindow(const std::filesystem::path& data, const QString& language = "en", QWidget* parent = nullptr);
    ~EditorWindow() override;
    void autosaveNow();
    void recoverFile(const QString&);
    QString recoveryPath() const { return recoveryFile_; }
    bool autosaveActive() const { return autosaveTimer_.isActive(); }
    const History& history() const { return history_; }
    EditorCanvas* canvas() const { return canvas_; }
    void openFile(const QString& path); // Parse and validate before replacing the document.
    // Same, but reports a failure through this window's translated error surface instead of
    // throwing a bare code at a caller that has no locale to resolve it with.
    void openFileOrReport(const QString& path);
    void saveFile(const QString& path); // Atomic replacement; failures preserve dirty state.
protected:
    void closeEvent(QCloseEvent*) override;
private:
    History history_;
    std::filesystem::path data_;
    DisplayCatalog displayCatalog_;
    QListWidget* palette_{};
    QListWidget* historyList_{};
    void buildHistory();
    void rotateSelection();
    void refreshHistory();
    void restoreHistory(std::uint64_t revision);
    QComboBox *objectLevel_{},*objectDisplay_{},*visibleLevel_{};
    QSpinBox *connectorFromCount_{},*connectorToCount_{},*connectorPoints_{};
    // Vissim's Lanes tab, as comma-separated text, the same shape the Link lane-width row uses.
    QLineEdit *connectorWidths_{},*connectorMarkings_{};
    QDoubleSpinBox *connectorFromPosition_{},*connectorToPosition_{};
    void buildPalette();
    void translatePalette();
    void buildAppearance(QFormLayout*);
    void refreshAppearance();
    // The Name of whatever is selected, empty when nothing is or the object carries none.
    std::string selectedName() const;
    void createLinkDialog(const std::vector<Point>&);
    void createRangeDialog(LaneReference,LaneReference,const std::vector<Point>&);
    QString recoveryDirectory_, recoveryFile_;
    std::unique_ptr<QLockFile> recoveryLock_;
    QTimer autosaveTimer_;
    std::optional<std::uint64_t> autosavedRevision_;
    void buildRecovery();
    void startAutosave();
    void clearRecovery();
    void recoverDialog(bool startup = false);
    QTableWidget *routeTable_{}, *inputTable_{}, *programTable_{};
    void buildDemandTables();
    void refreshDemand();
    void translateDemand();
    void editRoute(const std::string& id = {}, const std::vector<std::string>& initial = {});
    void editInput(const std::string& id = {});
    void editProgram(const std::string& id = {});
    void editHead(const std::string& id = {});
    void editRunSettings();
    void deleteDemand(const std::string& kind, const std::string& id);
    void selectDemand(const std::string& id);
    void buildRunControls();
    void refreshRun();
    bool prepareRun();
    void toggleRun();
    void stepRun();
    void tickRun();
    void clearRun();
    void pauseRun();
    QTimer runTimer_;
    QElapsedTimer runElapsed_;
    double runCredit_{};
    std::optional<RunSnapshot> runSnapshot_;
    SimState runState_;
    QLineEdit* runSeed_{};
    QComboBox* runSpeed_{};
    QLabel* runInfo_{};
public:
    const SimState& runState() const { return runState_; }
private:
    QString file_;
    std::map<QString,QJsonObject> locales_;
    std::map<std::string,QAction*> actions_;
    std::map<std::string,QWidget*> texts_;
    EditorCanvas* canvas_{};
    QComboBox *language_{}, *tool_{}, *side_{};
    QComboBox *connectorObject_{}, *connectorFrom_{}, *connectorTo_{};
    QTabWidget *properties_{}, *objects_{};
    QTableWidget *linkTable_{}, *connectorTable_{}, *signalTable_{}, *problemTable_{};
    // Diagnostics of the current revision, plus the issues of the most recent rejected edit.
    // Both are derived views; the document stays the single source of truth.
    std::vector<Diagnostic> diagnostics_, rejected_;
    std::uint64_t tableRevision_{}, diagnosticRevision_{};
    bool syncing_{};
    QLabel* connectorHint_{};
    QSpinBox* count_{};
    QDoubleSpinBox *width_{}, *grid_{}, *split_{}, *gap_{}, *bgX_{}, *bgY_{}, *bgScale_{}, *bgAngle_{}, *bgOpacity_{};
    QLineEdit *id_{}, *name_{}, *widths_{}, *linkMarkings_{};
    QDoubleSpinBox* linkPointStation_{};
    QLabel *error_{}, *coordinates_{}, *selectionInfo_{};
    QString text(const std::string& key) const;
    void buildInspector();
    QWidget* buildConnectorInspector();
    void refreshConnector();
    void refreshConnectorRanges();
    void buildObjectTables();
    void buildDiagnostics();
    void refreshTables(bool modelChanged);
    void refreshDiagnostics();
    void retranslateTables();
    void jumpTo(int row);
    void deleteSelected();
    void addConnection(const LaneReference& from, const LaneReference& to);
    void connectorHint();
    void refresh(bool modelChanged = true);
    void translate();
    bool execute(const std::string& name, const std::function<void(ProjectDocument&)>& action);
    void showError(const std::exception& error);
    bool confirmDiscard();
    bool saveDialog(bool as = false);
    void importImage();
    void applyBackground();
    void measure(Point a, Point b, bool calibrate);
    QAction* action(const std::string& key, const QKeySequence& shortcut, const std::function<void()>& run);
    void label(QFormLayout* form, const std::string& key, QWidget* field);
};
}
