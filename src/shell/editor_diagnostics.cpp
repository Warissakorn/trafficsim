#include "editor_window.hpp"
#include <QAction>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>

namespace trafficsim {
void EditorWindow::buildDiagnostics() {
    problemTable_=new QTableWidget(0,4,objects_); problemTable_->setObjectName("editorProblemTable");
    problemTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    problemTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    problemTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    problemTable_->verticalHeader()->setVisible(false);
    // The message earns the space here; the index path is a precise locator, not prose.
    problemTable_->horizontalHeader()->setStretchLastSection(false);
    problemTable_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    problemTable_->setWordWrap(false);
    objects_->addTab(problemTable_,QString());
    // Selecting a row is the jump. refreshDiagnostics rebuilds under a QSignalBlocker, so a
    // jump that re-renders the list cannot come back round as a second jump.
    connect(problemTable_,&QTableWidget::itemSelectionChanged,this,[this]{
        const auto rows=problemTable_->selectionModel()->selectedRows();
        if (!rows.isEmpty()) jumpTo(rows.front().row());
    });
    // On demand, never on every refresh: compiling the document per mouse-up would be wasteful,
    // and a verdict that reappears on its own reads as a claim the editor has not re-earned.
    action("editorRecheck",{},[this]{
        diagnostics_=runDiagnostics(history_.document(),data_);
        diagnosticRevision_=history_.revision();
        refreshDiagnostics();
        objects_->setCurrentIndex(3);
    });
}
void EditorWindow::refreshDiagnostics() {
    // Runtime findings are only true of the revision they were computed on.
    if (history_.revision()!=diagnosticRevision_) diagnostics_.clear();
    const QSignalBlocker block(problemTable_);
    std::vector<Diagnostic> rows=rejected_;
    for (const auto& row : diagnostics_) rows.push_back(row);
    problemTable_->setRowCount(rows.empty()?1:static_cast<int>(rows.size()));
    if (rows.empty()) {
        auto* note=new QTableWidgetItem(text("editorNoProblems"));
        problemTable_->setItem(0,0,note);
        for (int column=1; column<4; ++column) problemTable_->setItem(0,column,new QTableWidgetItem());
        problemTable_->resizeColumnToContents(0);
        return;
    }
    for (int row=0; row<static_cast<int>(rows.size()); ++row) {
        const auto& item=rows[static_cast<std::size_t>(row)];
        const auto translated=text(item.code);
        const QStringList values{
            text(item.severity==DiagnosticSeverity::draft?"editorDraftSeverity":
                 item.severity==DiagnosticSeverity::advisory?"editorAdvisorySeverity":"editorRuntimeSeverity"),
            translated.isEmpty()?QString::fromStdString(item.code):translated,
            QString::fromStdString(item.objectId),
            QString::fromStdString(item.path)};
        for (int column=0; column<4; ++column) {
            auto* cell=new QTableWidgetItem(values[column]);
            cell->setData(Qt::UserRole,QString::fromStdString(item.selectId));
            cell->setData(Qt::UserRole+1,QString::fromStdString(item.objectId));
            if (item.selectId.empty()) cell->setToolTip(text("EDIT_UNKNOWN_OBJECT"));
            problemTable_->setItem(row,column,cell);
        }
    }
    for (const int column : {0,2,3}) problemTable_->resizeColumnToContents(column);
}
void EditorWindow::jumpTo(int row) {
    const auto* cell=problemTable_->item(row,0);
    if (!cell) return;
    const auto id=cell->data(Qt::UserRole).toString().toStdString();
    if (id.empty()) {selectDemand(cell->data(Qt::UserRole+1).toString().toStdString());return;}
    canvas_->select(id);
    canvas_->frame(id);
}
}
