#pragma once
#include "../commands/network_commands.hpp"
#include "../editor/canvas.hpp"
#include <QMainWindow>
#include <QJsonObject>
#include <map>
#include <filesystem>
#include <QKeySequence>

class QAction;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QLabel;
class QFormLayout;
class QCloseEvent;
namespace trafficsim {
class EditorWindow : public QMainWindow {
public:
    explicit EditorWindow(const std::filesystem::path& data, const QString& language = "en", QWidget* parent = nullptr);
    const History& history() const { return history_; }
    EditorCanvas* canvas() const { return canvas_; }
    void openFile(const QString& path); // Parse and validate before replacing the document.
    void saveFile(const QString& path); // Atomic replacement; failures preserve dirty state.
protected:
    void closeEvent(QCloseEvent*) override;
private:
    History history_;
    QString file_;
    std::map<QString,QJsonObject> locales_;
    std::map<std::string,QAction*> actions_;
    std::map<std::string,QWidget*> texts_;
    EditorCanvas* canvas_{};
    QComboBox *language_{}, *tool_{}, *side_{};
    QSpinBox* count_{};
    QDoubleSpinBox *width_{}, *grid_{}, *split_{}, *gap_{}, *bgX_{}, *bgY_{}, *bgScale_{}, *bgAngle_{}, *bgOpacity_{};
    QLineEdit *id_{}, *widths_{};
    QLabel *error_{}, *coordinates_{}, *selectionInfo_{};
    QString text(const std::string& key) const;
    void buildInspector();
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
