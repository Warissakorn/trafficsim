#include "editor_window.hpp"
#include "../commands/right_of_way_commands.hpp"
#include "../model/network/right_of_way.hpp"
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// M3.2.4: the Conflict areas tab. Every edit goes through the M3.2.2 commands inside
// execute(), so it is one Undo step and a refused edit changes nothing; keyboard (table, Enter,
// the dialog) and pointer (double-click, toolbar) submit the same command.
namespace trafficsim {
namespace {
constexpr int kConflictTab = 9; // after Results, so every earlier tab keeps its index
QString owner(const ControlPathRef& p) { return QString::fromStdString(p.connectorId.empty() ? p.linkId : p.connectorId); }
}
PriorityDefaults EditorWindow::priorityDefaults() const {
    const auto& def = history_.document().definition;
    return resolveCatalogs(def ? *def : AuthoringDefinition{}, data_).priorityDefaults;
}
void EditorWindow::buildConflicts() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(3);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    conflictTable_ = new QTableWidget(0, 7, body); conflictTable_->setObjectName("editorConflictTable");
    conflictTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    conflictTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    conflictTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    conflictTable_->horizontalHeader()->setStretchLastSection(true); conflictTable_->verticalHeader()->hide();
    layout->addWidget(conflictTable_);
    objects_->addTab(body, QString());
    // The Run UI says what the solver protects: authored areas only (M3_PLAN §2).
    runProtection_ = new QLabel(centralWidget()); runProtection_->setObjectName("editorRunProtection");
    runProtection_->setWordWrap(true); runProtection_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    qobject_cast<QVBoxLayout*>(centralWidget()->layout())->insertWidget(2, runProtection_);
    runProtection_->hide();
    bar->addAction(action("editorAddCrossing", {}, [this] { addCrossings(); }));
    bar->addAction(action("editorTakeOverMerge", {}, [this] {
        const auto* connector = canvas_->selectedConnector(); if (!connector) return;
        const auto id = connector->id; const auto defaults = priorityDefaults();
        if (execute("editorTakeOverMerge", [&](auto& d) { takeOverMergesOf(d, id, defaults); }))
            objects_->setCurrentIndex(kConflictTab);
    }));
    bar->addAction(action("editorEditConflict", {}, [this] { const auto id = selectedConflict(); if (!id.empty()) editConflict(id); }));
    // P cycles the selected area's priority from the keyboard, as a second click does on the canvas.
    auto* cycle = action("editorCyclePriority", QKeySequence(Qt::Key_P), [this] { cyclePriority(selectedConflict()); });
    cycle->setShortcutContext(Qt::WidgetWithChildrenShortcut); canvas_->addAction(cycle); conflictTable_->addAction(cycle);
    bar->addAction(cycle);
    bar->addAction(action("editorRestorePriority", {}, [this] {
        const auto id = selectedConflict(); if (id.empty()) return;
        execute("editorRestorePriority", [&](auto& d) { restoreAutomaticPriorityOf(d, id); });
    }));
    bar->addAction(action("editorDeleteConflict", {}, [this] {
        const auto id = selectedConflict(); if (id.empty()) return;
        execute("editorDeleteConflict", [&](auto& d) { removeConflictArea(d, id); });
    }));
    // The Conflict area tool's gestures (M3.2.4b) submit the same commands as the tab.
    canvas_->conflictPicked = [this](const std::string& id) { selectConflict(id); };
    canvas_->conflictCycled = [this](const std::string& id) { cyclePriority(id); };
    canvas_->waitingLineMoved = [this](const std::string& id, double station) {
        execute("editorMoveWaitingLine", [&](auto& d) { moveWaitingLine(d, id, station); }); };
    connect(conflictTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selectedConflict(); if (!id.empty()) editConflict(id); });
    connect(conflictTable_, &QTableWidget::activated, this, [this](const QModelIndex&) { // Enter
        const auto id = selectedConflict(); if (!id.empty()) editConflict(id); });
    connect(conflictTable_, &QTableWidget::itemSelectionChanged, this, [this] {
        canvas_->setHighlightedConflict(selectedConflict()); refresh(false); });
}
void EditorWindow::translateConflicts() {
    objects_->setTabText(kConflictTab, text("editorConflictTable"));
    conflictTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorConflictKind"),
        text("editorConflictPriority"), text("editorConflictGap"), text("editorConflictHeadway"), text("editorConflictStatus")});
    if (runProtection_) runProtection_->setText(text("editorRunProtection"));
    conflictRevision_ = UINT64_MAX; // translated cells are rebuilt on the next refresh
}
std::string EditorWindow::selectedConflict() const {
    const auto* cell = conflictTable_ ? conflictTable_->item(conflictTable_->currentRow(), 0) : nullptr;
    return cell && !conflictTable_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void EditorWindow::refreshConflicts() {
    if (!conflictTable_) return;
    const auto& n = history_.document().network;
    const auto& row = n.rightOfWay;
    const auto keep = selectedConflict();
    if (conflictRevision_ != history_.revision() || conflictTable_->rowCount() != static_cast<int>(row.conflictAreas.size())) {
        conflictRevision_ = history_.revision();
        const QSignalBlocker block(conflictTable_);
        conflictTable_->setRowCount(static_cast<int>(row.conflictAreas.size()));
        // The resolver's verdict per area, from the same call Run makes: what runs, or the first
        // reason it does not. Defaults only matter for merges nobody overrode, not these rows.
        const auto verdict = row.conflictAreas.empty() ? RightOfWayResolution{} : resolveRightOfWay(n, runtimeSections(n), {1, 1});
        for (int r = 0; r < conflictTable_->rowCount(); ++r) {
            const auto& a = row.conflictAreas[static_cast<std::size_t>(r)];
            const auto prefix = "rightOfWay.conflictAreas[" + std::to_string(r) + "]";
            QString status = text("editorConflictRuns");
            for (const auto& issue : verdict.issues)
                if (issue.path.rfind(prefix, 0) == 0 && (issue.path.size() == prefix.size() || issue.path[prefix.size()] == '.')) {
                    const auto t = text(issue.code); status = t.isEmpty() ? QString::fromStdString(issue.code) : t; break;
                }
            const auto rule = std::find_if(row.priorityRules.begin(), row.priorityRules.end(), [&](const auto& x) { return x.conflictAreaId == a.id; });
            const QString priority = a.priority == ConflictPriority::undetermined ? text("editorConflictUndetermined")
                : text("editorConflictGivesWay").arg(owner(a.priority == ConflictPriority::firstYields ? a.first.path : a.second.path));
            const QStringList values{QString::fromStdString(a.id), QString::fromStdString(a.name),
                text(a.kind == ConflictKind::crossing ? "editorConflictCrossing" : "editorConflictMerge"), priority,
                rule == row.priorityRules.end() ? QString() : QString::number(rule->gapTime, 'f', 1),
                rule == row.priorityRules.end() ? QString() : QString::number(rule->headway, 'f', 1), status};
            for (int c = 0; c < values.size(); ++c) {
                auto* cell = new QTableWidgetItem(values[c]); cell->setData(Qt::UserRole, QString::fromStdString(a.id));
                conflictTable_->setItem(r, c, cell);
            }
            if (a.id == keep) conflictTable_->selectRow(r);
        }
        conflictTable_->resizeColumnsToContents();
    }
    canvas_->setHighlightedConflict(selectedConflict());
    // What each action needs: two roads to cross, a Connector to take over, a row to act on.
    const auto& sel = canvas_->selection();
    const auto road = [&](const std::string& id) {
        return std::any_of(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; }) ||
               std::any_of(n.connectors.begin(), n.connectors.end(), [&](const auto& c) { return c.id == id; });
    };
    actions_.at("editorAddCrossing")->setEnabled(sel.size() == 2 && road(sel[0]) && road(sel[1]));
    actions_.at("editorTakeOverMerge")->setEnabled(canvas_->selectedConnector() != nullptr);
    const bool chosen = !selectedConflict().empty();
    for (const auto* key : {"editorEditConflict", "editorCyclePriority", "editorRestorePriority", "editorDeleteConflict"}) actions_.at(key)->setEnabled(chosen);
}
void EditorWindow::showConflicts() { objects_->setCurrentIndex(kConflictTab); }
void EditorWindow::cyclePriority(const std::string& id) {
    if (id.empty()) return;
    const auto defaults = priorityDefaults();
    execute("editorCyclePriority", [&](auto& d) { cycleConflictPriority(d, id, defaults); });
}
bool EditorWindow::selectConflict(const std::string& id) {
    if (id.empty() || !conflictTable_) return false;
    const auto& row = history_.document().network.rightOfWay;
    std::string area = id;
    for (const auto& r : row.priorityRules) if (r.id == id) area = r.conflictAreaId;
    for (const auto& a : row.conflictAreas)
        if (a.first.waitingLineId == id || a.second.waitingLineId == id) { area = a.id; break; }
    for (int r = 0; r < conflictTable_->rowCount(); ++r)
        if (conflictTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == area) {
            objects_->setCurrentIndex(kConflictTab); conflictTable_->selectRow(r); return true;
        }
    return false;
}
void EditorWindow::editConflict(const std::string& id) {
    const auto& row = history_.document().network.rightOfWay;
    const auto a = std::find_if(row.conflictAreas.begin(), row.conflictAreas.end(), [&](const auto& x) { return x.id == id; });
    if (a == row.conflictAreas.end()) return;
    const auto rule = std::find_if(row.priorityRules.begin(), row.priorityRules.end(), [&](const auto& x) { return x.conflictAreaId == id; });
    const auto defaults = priorityDefaults();
    QDialog dialog(this); dialog.setObjectName("editorConflictDialog"); dialog.setWindowTitle(text("editorEditConflict"));
    auto* form = new QFormLayout(&dialog);
    auto* name = new QLineEdit(QString::fromStdString(a->name), &dialog); name->setObjectName("editorConflictName");
    auto* priority = new QComboBox(&dialog); priority->setObjectName("editorConflictPriority");
    priority->addItem(text("editorConflictGivesWay").arg(owner(a->first.path)), static_cast<int>(ConflictPriority::firstYields));
    priority->addItem(text("editorConflictGivesWay").arg(owner(a->second.path)), static_cast<int>(ConflictPriority::secondYields));
    priority->addItem(text("editorConflictUndetermined"), static_cast<int>(ConflictPriority::undetermined));
    priority->setCurrentIndex(priority->findData(static_cast<int>(a->priority)));
    // Parameter names stay as the file spells them, so an author can find them there.
    auto* gap = new QDoubleSpinBox(&dialog); gap->setObjectName("editorConflictGap");
    gap->setRange(0.1, 60); gap->setDecimals(1); gap->setSuffix(" s");
    gap->setValue(rule != row.priorityRules.end() ? rule->gapTime : std::max(0.1, defaults.gapTime));
    auto* headway = new QDoubleSpinBox(&dialog); headway->setObjectName("editorConflictHeadway");
    headway->setRange(0.1, 500); headway->setDecimals(1); headway->setSuffix(" m");
    headway->setValue(rule != row.priorityRules.end() ? rule->headway : std::max(0.1, defaults.headway));
    form->addRow(text("editorColumnName"), name);
    form->addRow(text("editorConflictPriority"), priority);
    form->addRow("gapTime", gap);
    form->addRow("headway", headway);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    const auto chosen = static_cast<ConflictPriority>(priority->currentData().toInt());
    execute("editorEditConflict", [&](auto& d) {
        setConflictControl(d, id, name->text().toStdString(), chosen, gap->value(), headway->value()); });
}
void EditorWindow::addCrossings() {
    const auto sel = canvas_->selection();
    if (sel.size() != 2) return;
    const auto defaults = priorityDefaults();
    QDialog dialog(this); dialog.setObjectName("editorCrossingDialog"); dialog.setWindowTitle(text("editorAddCrossing"));
    auto* form = new QFormLayout(&dialog);
    auto* yields = new QComboBox(&dialog); yields->setObjectName("editorCrossingYields");
    for (const auto& id : sel) yields->addItem(QString::fromStdString(id), QString::fromStdString(id));
    yields->setCurrentIndex(1); // the one selected last, as Vissim's second click marks the minor road
    // Say which lane pairs the gesture will create before it does (contract §1).
    auto preview = history_.document();
    std::size_t pairs = 0;
    try { pairs = addCrossingAreas(preview, sel[0], sel[1], sel[0], defaults).size(); } catch (const std::exception&) {}
    auto* count = new QLabel(text("editorCrossingPairs").arg(pairs), &dialog); count->setObjectName("editorCrossingPairs");
    form->addRow(text("editorCrossingYields"), yields);
    form->addRow(count);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    const auto minor = yields->currentData().toString().toStdString();
    if (execute("editorAddCrossing", [&](auto& d) { addCrossingAreas(d, sel[0], sel[1], minor, defaults); }))
        objects_->setCurrentIndex(kConflictTab);
}
}
