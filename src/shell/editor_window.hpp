#pragma once
#include "../commands/network_commands.hpp"
#include "../commands/connector_commands.hpp"
#include "../project/diagnostics.hpp"
#include "../project/run.hpp"
#include "../eval/summary.hpp"
#include "../eval/discharge.hpp"
#include "../project/evaluation.hpp"
#include "../project/display.hpp"
#include "../runner/batch.hpp"
#include "../commands/appearance_commands.hpp"
#include "../commands/demand_commands.hpp"
#include <QTimer>
#include <QElapsedTimer>
#include "../core/validate.hpp"
#include "../editor/canvas.hpp"
#include <QMainWindow>
#include <QJsonObject>
#include <atomic>
#include <map>
#include <memory>
#include <thread>
#include <filesystem>
#include <functional>
#include <vector>
#include <QKeySequence>

class QTreeWidget;
class QTreeWidgetItem;
class QListWidget;
class QLockFile;
class QAction;
class QComboBox;
class QVBoxLayout;
class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QLabel;
class QFormLayout;
class QCloseEvent;
class QTabWidget;
class QTableWidget;
class QToolBar;
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
    void resizeEvent(QResizeEvent*) override;
private:
    History history_;
    std::filesystem::path data_;
    DisplayCatalog displayCatalog_;
    QTreeWidget* palette_{};
    std::map<int,QTreeWidgetItem*> paletteItems_;
    QListWidget* historyList_{};
    void buildHistory();
    void rotateSelection();
    void refreshHistory();
    void restoreHistory(std::uint64_t revision);
    QComboBox *objectLevel_{},*objectDisplay_{},*visibleLevel_{};
    QSpinBox *connectorFromCount_{},*connectorToCount_{},*connectorPoints_{};
    QComboBox* connectorLaneSide_{}; // M3.2.9c: laneChangeSide, one-lane differences only (D73)
    // Vissim's Lanes tab, as comma-separated text, the same shape the Link lane-width row uses.
    QLineEdit *connectorWidths_{},*connectorMarkings_{};
    QDoubleSpinBox *connectorFromPosition_{},*connectorToPosition_{};
    void buildPalette();
    void translatePalette();
    void showCommandPalette();
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
    QTableWidget *routeTable_{}, *inputTable_{}, *programTable_{}, *decisionTable_{};
    void buildDemandTables();
    void refreshDemand();
    void translateDemand();
    void editRoute(const std::string& id = {}, const std::vector<std::string>& initial = {});
    void showDemandPreview();
    void editDemandCatalog();
    // M3.3.2c (D128): driving-behaviour library dialog and Link/Connector assignment.
    void openBehaviourLibrary();
    void buildBehaviourInspector(QVBoxLayout*);
    void refreshBehaviourInspector();
    std::vector<std::string> selectedRoads() const; // selected Links and Connectors, in selection order
    QComboBox* behaviourType_{};
    QLabel* effectiveBehaviour_{};
    void editInput(const std::string& id = {}, const std::string& preselectedRoute = {}, const std::string& preselectedLink = {});
    void editProgram(const std::string& id = {}); // legacy programs only (M2.7b)
    // M2.7b, src/shell/editor_signal.cpp. Returns the id committed, empty on cancel.
    std::string editController(const std::string& id = {});
    QString signalLabel(const std::string& controllerId, int groupNumber, const std::string& programId) const;
    void editDecision(const std::string& id = {}); // M2.4, src/shell/editor_decision.cpp
    // `placed` is the canvas click: the dialog opens on that lane and station.
    void editHead(const std::string& id = {}, const std::optional<HeadPlacement>& placed = {});
    void editRunSettings();
    void deleteDemand(const std::string& kind, const std::string& id);
    void selectDemand(const std::string& id);
    // Pointer authoring (M1.25): the canvas draws and gestures, the window commits. Both go
    // through the same putRoute/putInput commands the dialogs use.
    void buildRouting();
    void commitDrawnRoute(const std::vector<std::string>& segmentIds,std::optional<double> position = {});
    void placeInputOnLink(const std::string& linkId);
    void showDemandMenu(QPoint viewportPosition);
    void syncHighlightedRoute();
    void refreshToolHint();
    QLabel* toolHint_{};
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
    // Fed every step, exactly as the retired M0 window did: the clamp count is the only signal
    // that the prototype car-following is being pushed, so it has to be visible where the run is.
    SummaryAccumulator runSummary_;
    // M2.5: per-movement delay and approach queues, fed the same states as runSummary_.
    std::optional<MovementAccumulator> runMovements_;
    // D135: queue discharge with the CLI's default spec, fed the same states; the reason it
    // could not be set up, if any, is shown instead of a table.
    std::optional<DischargeAccumulator> runDischarge_;
    std::string runDischargeError_;
    // D135: one row per SafetyClampEvent, read off the published vehicle (blank once it left).
    struct ClampRow { double time{}; std::uint64_t vehicleId{}; std::string typeId, routeId, segmentId; };
    std::vector<ClampRow> runClamps_;
    QWidget* resultsPage_{};
    QTabWidget* resultsTabs_{};
    QTableWidget *movementTable_{}, *queueTable_{}, *dischargeTable_{}, *clampTable_{};
    QLabel *resultsNote_{}, *dischargeNote_{}, *clampNote_{};
    void refreshDischarge();
    void refreshClamps();
    void buildResults();
    void translateResults();
    void refreshResults();
    bool runFinished() const;
    void observeRun();
    // M5.6 (BATCH §6), src/shell/editor_batch.cpp: Run N seeds on one worker thread over value
    // copies of the compiled document. A generation number discards whatever a cancelled or
    // replaced batch posts back, so no table ever shows fewer runs than its n.
public:
    struct BatchResult { BatchReport report; std::vector<SeedRun> runs; };
private:
    QLineEdit* batchSeeds_{};
    QLabel* batchNote_{};
    QWidget* batchView_{};
    QTableWidget *batchMovementTable_{}, *batchSectionTable_{}, *batchQueueTable_{};
    std::thread batchThread_;
    std::shared_ptr<std::atomic<bool>> batchCancel_;
    std::uint64_t batchGeneration_{};
    std::size_t batchDone_{}, batchTotal_{}; // batchTotal_ is non-zero while a batch runs
    std::optional<BatchResult> batch_;
    void buildBatch(QToolBar*, QVBoxLayout*);
    void translateBatch();
    void refreshBatch();
    void startBatch();
    void cancelBatch(); // stops a running batch and discards a finished one
    void finishBatch(std::uint64_t generation, std::shared_ptr<BatchResult>, const std::string& error);
    // M3.2.4, src/shell/editor_priority.cpp: the Conflict areas tab, its dialogs and actions.
    QTableWidget* conflictTable_{};
    QLabel* runProtection_{};
    std::uint64_t conflictRevision_{UINT64_MAX}; // the rows are rebuilt only when this goes stale
    void buildConflicts();
    void translateConflicts();
    void refreshConflicts();
    void editConflict(const std::string& id);
    void addCrossings();
    // Shows the area in the tab and on the canvas; a waiting line or rule id names its area.
    bool selectConflict(const std::string& id);
    void showConflicts();
    void cyclePriority(const std::string& id);
    std::string selectedConflict() const;
    PriorityDefaults priorityDefaults() const; // data/priority-rules, as Run reads them
    // M3.2.4c (D68): the automatic areas, derived once per revision and only while the Conflict
    // area tool or tab is in use (3.4 ms a revision on the four-leg template, Release).
    std::vector<AutomaticConflict> automatic_;
    std::uint64_t automaticRevision_{UINT64_MAX};
    bool automaticShown() const;
    void authorConflict(const std::string& key);
    // M3.2.6c, src/shell/editor_counters.cpp: the Queue counters tab and the tool's commit.
    QTableWidget* counterTable_{};
    std::uint64_t counterRevision_{UINT64_MAX};
    void buildCounters();
    void translateCounters();
    void refreshCounters();
    void showCounters();
    void editCounter(const std::string& id);
    void addCounter(std::vector<MeasurementLine> lines);
    std::string selectedCounter() const;
    // M5.4b, src/shell/editor_sections.cpp: the Travel-time sections tab and the tool's commit.
    QTableWidget* sectionTable_{};
    std::uint64_t sectionRevision_{UINT64_MAX};
    void buildSections();
    void translateSections();
    void refreshSections();
    void showSections();
    void editSection(const std::string& id);
    std::string selectedSection() const;
    QLineEdit* runSeed_{};
    QComboBox* runSpeed_{};
    QLabel* runInfo_{};
public:
    const SimState& runState() const { return runState_; }
    RunSummary runSummary() const { return runSummary_.summary(); }
    std::optional<MovementReport> runReport() const {
        return runMovements_ ? std::optional(runMovements_->report(runState_)) : std::nullopt;
    }
    // The finished run's report as the CLI's CSV, replaced atomically; throws before the end.
    // With a finished batch, the batch's CSV instead (`--seeds --csv`).
    void exportResults(const QString& file) const;
    // The same text as tab-separated values on the clipboard (M5.6); throws when Export would.
    void copyResults() const;
    std::string resultsCsv() const; // what Export writes; throws EDIT_CSV_UNFINISHED without a finished run or batch
    bool batchRunning() const { return batchTotal_ > 0; }
    const std::optional<BatchResult>& batchResult() const { return batch_; }
private:
    QString file_;
    std::map<QString,QJsonObject> locales_;
    std::map<std::string,QAction*> actions_;
    std::map<std::string,QWidget*> texts_;
    std::vector<std::function<void()>> validationRefresh_;
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
    void buildWorkspace();
    void resetWorkspace();
    void layoutToolbars();
    void focusCanvas(bool enabled);
    QByteArray workspaceBeforeFocus_;
    bool changingWorkspace_{};
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
    std::string lastHeadSignal_; // "c:<controller>#<group>" or "p:<program>" the last head took; the next defaults to it
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
