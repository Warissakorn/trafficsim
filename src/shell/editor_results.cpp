#include "editor_window.hpp"
#include "../core/simulation.hpp"
#include <QHeaderView>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace trafficsim {
// M2.5: the per-movement table and the approach queues, beside what they are not. The note is
// never optional: these are one unvalidated run of a prototype car-following model (rule 4).
namespace {
QTableWidget* resultTable(QWidget* parent, const char* name, int columns) {
    auto* table=new QTableWidget(0,columns,parent); table->setObjectName(name);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    // The names take the room; each figure column is exactly as wide as its header needs.
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    return table;
}
QTableWidgetItem* figure(const std::optional<double>& value) {
    auto* item=new QTableWidgetItem(value?QString::number(*value,'f',1):QString());
    item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
    return item;
}
}
void EditorWindow::buildResults() {
    auto* page=new QWidget(objects_); auto* layout=new QVBoxLayout(page);
    resultsNote_=new QLabel(page); resultsNote_->setObjectName("editorResultsNote");
    resultsNote_->setWordWrap(true); layout->addWidget(resultsNote_);
    // Side by side, so the dock's height goes to rows: twelve movements beside four approaches.
    auto* row=new QHBoxLayout; layout->addLayout(row,1);
    movementTable_=resultTable(page,"editorMovementTable",4); row->addWidget(movementTable_,3);
    queueTable_=resultTable(page,"editorQueueTable",3); row->addWidget(queueTable_,2);
    objects_->addTab(page,QString());
}
void EditorWindow::translateResults() {
    objects_->setTabText(8,text("editorResultsTable"));
    movementTable_->setHorizontalHeaderLabels({text("editorResultsMovement"),text("editorResultsVehicles"),
                                               text("editorResultsDelay"),text("editorResultsTravel")});
    queueTable_->setHorizontalHeaderLabels({text("editorResultsApproach"),text("editorResultsQueueMean"),
                                            text("editorResultsQueueMax")});
    refreshResults();
}
void EditorWindow::refreshResults() {
    if(!movementTable_)return;
    const auto report=runReport();
    if(!report){
        movementTable_->setRowCount(0); queueTable_->setRowCount(0);
        resultsNote_->setText(text("editorResultsEmpty")); return;
    }
    movementTable_->setRowCount(static_cast<int>(report->movements.size()));
    for(int r=0;r<movementTable_->rowCount();++r){
        const auto& m=report->movements[static_cast<std::size_t>(r)];
        movementTable_->setItem(r,0,new QTableWidgetItem(QString::fromStdString(m.name)));
        auto* count=new QTableWidgetItem(QString::number(m.vehicles));
        count->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter); movementTable_->setItem(r,1,count);
        movementTable_->setItem(r,2,figure(m.meanDelay));
        movementTable_->setItem(r,3,figure(m.meanTravelTime));
    }
    queueTable_->setRowCount(static_cast<int>(report->queues.size()));
    for(int r=0;r<queueTable_->rowCount();++r){
        const auto& q=report->queues[static_cast<std::size_t>(r)];
        queueTable_->setItem(r,0,new QTableWidgetItem(QString::fromStdString(q.name)));
        queueTable_->setItem(r,1,figure(q.meanLength));
        queueTable_->setItem(r,2,figure(q.maxLength));
    }
    const bool finished=runState_.scenario && runState_.tick>=totalTicks(*runState_.scenario);
    QString note=text("editorResultsNote").arg(report->active).arg(report->pending)
        .arg(report->unassigned).arg(report->safetyClamps);
    if(!finished) note=text("editorResultsPartial").arg(report->time,0,'f',1)+" "+note;
    resultsNote_->setText(note);
}
}
