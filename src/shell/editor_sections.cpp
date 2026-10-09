#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/right_of_way_commands.hpp"
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

// M5.4b (D133): the Travel-time sections tab. The Section tool's second Ctrl+right-click commits
// through execute(), so a placed section is one Undo step and invalidates the run, as rename and
// delete are.
namespace trafficsim {
namespace {
constexpr int kSectionTab = 11; // after Queue counters, so every earlier tab keeps its index
QString describe(const Network& n, const SectionLine& l) {
    const auto link = std::find_if(n.links.begin(), n.links.end(), [&](const auto& x) { return x.id == l.linkId; });
    const auto name = link == n.links.end() || link->name.empty() ? l.linkId : link->name;
    return QString("%1 @ %2 m").arg(QString::fromStdString(name)).arg(l.station, 0, 'f', 1);
}
}
void EditorWindow::buildSections() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    sectionTable_ = new QTableWidget(0, 4, body); sectionTable_->setObjectName("editorSectionTable");
    sectionTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    sectionTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    sectionTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    sectionTable_->horizontalHeader()->setStretchLastSection(true); sectionTable_->verticalHeader()->hide();
    layout->addWidget(sectionTable_);
    objects_->addTab(body, QString());
    bar->addAction(action("editorEditSection", {}, [this] { const auto id = selectedSection(); if (!id.empty()) editSection(id); }));
    bar->addAction(action("editorDeleteSection", {}, [this] {
        const auto id = selectedSection(); if (id.empty()) return;
        execute("editorDeleteSection", [&](auto& d) { deleteTravelTimeSection(d, id); });
    }));
    canvas_->sectionCommitted = [this](SectionLine start, SectionLine end) {
        std::string id;
        if (!execute("editorSectionTool", [&](auto& d) { id = putTravelTimeSection(d, {"", "", start, end}); })) return;
        showSections();
        for (int r = 0; r < sectionTable_->rowCount(); ++r)
            if (sectionTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) sectionTable_->selectRow(r);
    };
    connect(sectionTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selectedSection(); if (!id.empty()) editSection(id); });
    connect(sectionTable_, &QTableWidget::activated, this, [this](const QModelIndex&) { // Enter
        const auto id = selectedSection(); if (!id.empty()) editSection(id); });
    connect(sectionTable_, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translateSections() {
    objects_->setTabText(kSectionTab, text("editorSectionTable"));
    sectionTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"),
                                              text("editorSectionStart"), text("editorSectionEnd")});
    sectionRevision_ = UINT64_MAX;
}
std::string EditorWindow::selectedSection() const {
    const auto* cell = sectionTable_ ? sectionTable_->item(sectionTable_->currentRow(), 0) : nullptr;
    return cell && !sectionTable_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void EditorWindow::refreshSections() {
    if (!sectionTable_) return;
    const auto& n = history_.document().network;
    const auto keep = selectedSection();
    if (sectionRevision_ != history_.revision() || sectionTable_->rowCount() != static_cast<int>(n.travelTimeSections.size())) {
        sectionRevision_ = history_.revision();
        const QSignalBlocker block(sectionTable_);
        sectionTable_->setRowCount(static_cast<int>(n.travelTimeSections.size()));
        for (int r = 0; r < sectionTable_->rowCount(); ++r) {
            const auto& s = n.travelTimeSections[static_cast<std::size_t>(r)];
            const QStringList values{QString::fromStdString(s.id), QString::fromStdString(s.name), describe(n, s.start), describe(n, s.end)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(s.id));
                if (col == 0 || col >= 2) editorDesign::setNumericText(cell, col >= 2);
                sectionTable_->setItem(r, col, cell);
            }
            if (s.id == keep) sectionTable_->selectRow(r);
        }
        sectionTable_->resizeColumnsToContents();
    }
    const bool chosen = !selectedSection().empty();
    for (const auto* key : {"editorEditSection", "editorDeleteSection"}) actions_.at(key)->setEnabled(chosen);
}
void EditorWindow::showSections() { objects_->setCurrentIndex(kSectionTab); }
void EditorWindow::editSection(const std::string& id) {
    const auto& sections = history_.document().network.travelTimeSections;
    const auto s = std::find_if(sections.begin(), sections.end(), [&](const auto& x) { return x.id == id; });
    if (s == sections.end()) return;
    QDialog dialog(this); dialog.setObjectName("editorSectionDialog"); dialog.setWindowTitle(text("editorEditSection"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2); form->setVerticalSpacing(editorDesign::space1);
    auto* name = new QLineEdit(QString::fromStdString(s->name), &dialog); name->setObjectName("editorSectionName");
    form->addRow(text("editorColumnName"), name);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    auto edited = *s; edited.name = name->text().toStdString();
    execute("editorEditSection", [&](auto& d) { putTravelTimeSection(d, edited); });
}
}
