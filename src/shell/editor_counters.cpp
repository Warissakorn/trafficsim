#include "editor_window.hpp"
#include "../commands/right_of_way_commands.hpp"
#include "../project/evaluation.hpp"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// M3.2.6c: the Queue counters tab. The tool's Enter (pointer) and Add queue counter over the
// selected heads (keyboard) submit the same command inside execute(), so each is one Undo step
// and invalidates the run. What a row replaces is read from the rule evaluation applies.
namespace trafficsim {
namespace {
constexpr int kCounterTab = 10; // after Conflict areas, so every earlier tab keeps its index
QString describe(const MeasurementLine& l) {
    if (!l.point) return QString::fromStdString(l.referenceId);
    return QString("%1/%2 @ %3 m").arg(QString::fromStdString(l.point->path.linkId), QString::fromStdString(l.point->path.laneId))
                                  .arg(l.point->station, 0, 'f', 1);
}
}
void EditorWindow::buildCounters() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(3);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    counterTable_ = new QTableWidget(0, 4, body); counterTable_->setObjectName("editorCounterTable");
    counterTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    counterTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    counterTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    counterTable_->horizontalHeader()->setStretchLastSection(true); counterTable_->verticalHeader()->hide();
    layout->addWidget(counterTable_);
    objects_->addTab(body, QString());
    bar->addAction(action("editorAddCounter", {}, [this] {
        std::vector<MeasurementLine> lines;
        const auto& heads = history_.document().network.signalHeads;
        for (const auto& id : canvas_->selection())
            if (std::any_of(heads.begin(), heads.end(), [&](const auto& h) { return h.id == id; })) lines.push_back({id, std::nullopt});
        addCounter(std::move(lines));
    }));
    bar->addAction(action("editorEditCounter", {}, [this] { const auto id = selectedCounter(); if (!id.empty()) editCounter(id); }));
    bar->addAction(action("editorDeleteCounter", {}, [this] {
        const auto id = selectedCounter(); if (id.empty()) return;
        execute("editorDeleteCounter", [&](auto& d) { deleteQueueCounter(d, id); });
    }));
    canvas_->counterDraftCommitted = [this](std::vector<MeasurementLine> lines) { addCounter(std::move(lines)); };
    connect(counterTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selectedCounter(); if (!id.empty()) editCounter(id); });
    connect(counterTable_, &QTableWidget::activated, this, [this](const QModelIndex&) { // Enter
        const auto id = selectedCounter(); if (!id.empty()) editCounter(id); });
    connect(counterTable_, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translateCounters() {
    objects_->setTabText(kCounterTab, text("editorCounterTable"));
    counterTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"),
                                              text("editorCounterLines"), text("editorCounterReplaces")});
    counterRevision_ = UINT64_MAX;
}
std::string EditorWindow::selectedCounter() const {
    const auto* cell = counterTable_ ? counterTable_->item(counterTable_->currentRow(), 0) : nullptr;
    return cell && !counterTable_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void EditorWindow::refreshCounters() {
    if (!counterTable_) return;
    const auto& n = history_.document().network;
    const auto keep = selectedCounter();
    if (counterRevision_ != history_.revision() || counterTable_->rowCount() != static_cast<int>(n.queueCounters.size())) {
        counterRevision_ = history_.revision();
        const QSignalBlocker block(counterTable_);
        counterTable_->setRowCount(static_cast<int>(n.queueCounters.size()));
        for (int r = 0; r < counterTable_->rowCount(); ++r) {
            const auto& c = n.queueCounters[static_cast<std::size_t>(r)];
            QStringList lines, replaced;
            for (const auto& l : c.lines) lines << describe(l);
            for (const auto& id : replacedApproaches(n, c)) {
                const auto link = std::find_if(n.links.begin(), n.links.end(), [&](const auto& x) { return x.id == id; });
                replaced << QString::fromStdString(link->name.empty() ? id : link->name);
            }
            const QStringList values{QString::fromStdString(c.id), QString::fromStdString(c.name),
                                     QString("%1: %2").arg(c.lines.size()).arg(lines.join(", ")), replaced.join(", ")};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(c.id));
                counterTable_->setItem(r, col, cell);
            }
            if (c.id == keep) counterTable_->selectRow(r);
        }
        counterTable_->resizeColumnsToContents();
    }
    const auto& heads = n.signalHeads;
    const auto& sel = canvas_->selection();
    actions_.at("editorAddCounter")->setEnabled(std::any_of(sel.begin(), sel.end(), [&](const auto& id) {
        return std::any_of(heads.begin(), heads.end(), [&](const auto& h) { return h.id == id; }); }));
    const bool chosen = !selectedCounter().empty();
    for (const auto* key : {"editorEditCounter", "editorDeleteCounter"}) actions_.at(key)->setEnabled(chosen);
}
void EditorWindow::showCounters() { objects_->setCurrentIndex(kCounterTab); }
void EditorWindow::addCounter(std::vector<MeasurementLine> lines) {
    if (lines.empty()) return;
    std::string id;
    if (!execute("editorAddCounter", [&](auto& d) { id = putQueueCounter(d, {"", "", std::move(lines)}); })) return;
    showCounters();
    for (int r = 0; r < counterTable_->rowCount(); ++r)
        if (counterTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) counterTable_->selectRow(r);
}
void EditorWindow::editCounter(const std::string& id) {
    const auto& counters = history_.document().network.queueCounters;
    const auto c = std::find_if(counters.begin(), counters.end(), [&](const auto& x) { return x.id == id; });
    if (c == counters.end()) return;
    QDialog dialog(this); dialog.setObjectName("editorCounterDialog"); dialog.setWindowTitle(text("editorEditCounter"));
    auto* form = new QFormLayout(&dialog);
    auto* name = new QLineEdit(QString::fromStdString(c->name), &dialog); name->setObjectName("editorCounterName");
    form->addRow(text("editorColumnName"), name);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    auto edited = *c; edited.name = name->text().toStdString();
    execute("editorEditCounter", [&](auto& d) { putQueueCounter(d, edited); });
}
}
