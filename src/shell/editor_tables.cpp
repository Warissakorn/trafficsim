#include "editor_window.hpp"
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QHeaderView>
#include <QLabel>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace trafficsim {
namespace {
QTableWidget* table(QWidget* parent, const char* name, int columns) {
    auto* view=new QTableWidget(0,columns,parent); view->setObjectName(name);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view->verticalHeader()->setVisible(false);
    view->horizontalHeader()->setStretchLastSection(true);
    return view;
}
void fill(QTableWidget* view,int row,const QStringList& values,const QString& id) {
    for (int column=0; column<values.size(); ++column) {
        auto* cell=new QTableWidgetItem(values[column]);
        // Every row carries the object id it names; nothing reads a row back out of its text.
        cell->setData(Qt::UserRole,id);
        view->setItem(row,column,cell);
    }
}
QString metres(double value) { return QString::number(value,'f',2); }
}
void EditorWindow::buildObjectTables() {
    auto* dock=new QDockWidget(this); dock->setObjectName("editorObjectsDock"); texts_["editorObjectsDock"]=dock;
    dock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetClosable);
    auto* body=new QWidget(dock); auto* layout=new QVBoxLayout(body);
    layout->setContentsMargins(6,2,6,6);layout->setSpacing(4);
    objects_=new QTabWidget(body); objects_->setObjectName("editorObjectTabs"); layout->addWidget(objects_);
    linkTable_=table(objects_,"editorLinkTable",5);
    connectorTable_=table(objects_,"editorConnectorTable",5);
    signalTable_=table(objects_,"editorSignalTable",5);
    for (auto* view : {linkTable_,connectorTable_,signalTable_}) objects_->addTab(view,QString());
    buildDiagnostics();

    // Selection flows one way at a time: the sync flag keeps the two views from echoing.
    for (auto* view : {linkTable_,connectorTable_,signalTable_})
        connect(view,&QTableWidget::itemSelectionChanged,this,[this,view]{
            if (syncing_) return;
            std::vector<std::string> ids;
            if(QApplication::keyboardModifiers()&(Qt::ControlModifier|Qt::ShiftModifier)) {
                ids=canvas_->selection();
                for(int row=0;row<view->rowCount();++row) {
                    const auto id=selectableFor(history_.document().network,view->item(row,0)->data(Qt::UserRole).toString().toStdString());
                    std::erase(ids,id);
                }
            }
            for (const auto* item : view->selectedItems()) {
                if (item->column()) continue;
                const auto id=selectableFor(history_.document().network,item->data(Qt::UserRole).toString().toStdString());
                if (!id.empty()) ids.push_back(id);
            }
            canvas_->setSelection(ids);
            if (!ids.empty()) canvas_->frame(ids.back());
        });
    dock->setWidget(body); addDockWidget(Qt::BottomDockWidgetArea,dock);
    actions_["editorObjects"]=dock->toggleViewAction();
    actions_["editorObjects"]->setShortcut(QKeySequence("Ctrl+Shift+O"));
    resizeDocks({dock},{230},Qt::Vertical);
}
void EditorWindow::retranslateTables() {
    const char* tabs[]={"editorLinkTable","editorConnectorTable","editorSignalTable","editorProblemTable"};
    for (int i=0;i<4;++i) objects_->setTabText(i,text(tabs[i]));
    // Name sits next to ID, as it does in every Vissim list.
    const char* linkColumns[]={"editorColumnId","editorColumnName","editorColumnLanes","editorColumnLength","editorColumnHeads"};
    const char* connectorColumns[]={"editorColumnId","editorColumnName","editorColumnFrom","editorColumnTo","editorColumnLength"};
    const char* signalColumns[]={"editorColumnId","editorColumnName","editorColumnLane","editorColumnPosition","editorColumnProgram"};
    const char* problemColumns[]={"editorColumnSeverity","editorColumnCode","editorColumnObject","editorColumnWhere"};
    const std::pair<QTableWidget*,const char**> views[]={{linkTable_,linkColumns},{connectorTable_,connectorColumns},
        {signalTable_,signalColumns},{problemTable_,problemColumns}};
    for (const auto& [view,columns] : views) {
        QStringList labels;
        for (int i=0;i<view->columnCount();++i) labels<<text(columns[i]);
        view->setHorizontalHeaderLabels(labels);
    }
}
void EditorWindow::refreshTables(bool modelChanged) {
    const QSignalBlocker blockLinks(linkTable_), blockConnectors(connectorTable_), blockSignals(signalTable_);
    const auto& network=history_.document().network;
    if (modelChanged || history_.revision()!=tableRevision_ || linkTable_->rowCount()!=static_cast<int>(network.links.size())) {
        tableRevision_=history_.revision();
        const auto laneOf=[&](const LaneReference& r){ return QString::fromStdString(r.linkId+" / "+r.laneId); };
        linkTable_->setRowCount(static_cast<int>(network.links.size()));
        for (int row=0; row<linkTable_->rowCount(); ++row) {
            const auto& link=network.links[static_cast<std::size_t>(row)];
            int heads=0;
            for (const auto& head : network.signalHeads) if (head.lane.linkId==link.id) ++heads;
            fill(linkTable_,row,{QString::fromStdString(link.id),QString::fromStdString(link.name),QString::number(link.lanes.size()),
                metres(polylineLength(link.geometry)),QString::number(heads)},QString::fromStdString(link.id));
        }
        connectorTable_->setRowCount(static_cast<int>(network.connectors.size()));
        for (int row=0; row<connectorTable_->rowCount(); ++row) {
            const auto& c=network.connectors[static_cast<std::size_t>(row)];
            fill(connectorTable_,row,{QString::fromStdString(c.id),QString::fromStdString(c.name),laneOf(c.from),laneOf(c.to),
                metres(polylineLength(c.geometry))},QString::fromStdString(c.id));
        }
        signalTable_->setRowCount(static_cast<int>(network.signalHeads.size()));
        for (int row=0; row<signalTable_->rowCount(); ++row) {
            const auto& head=network.signalHeads[static_cast<std::size_t>(row)];
            fill(signalTable_,row,{QString::fromStdString(head.id),QString::fromStdString(head.name),head.connectorId.empty()?laneOf(head.lane):QString::fromStdString(head.connectorId),metres(head.position),
                QString::fromStdString(head.programId)},QString::fromStdString(head.id));
        }
    }
    for (auto* view : {linkTable_,connectorTable_,signalTable_}) view->resizeColumnsToContents();
    syncing_=true;
    for (auto* view : {linkTable_,connectorTable_,signalTable_}) {
        view->clearSelection();
        for (int row=0; row<view->rowCount(); ++row) {
            const auto* cell=view->item(row,0);
            if (cell && canvas_->isSelected(selectableFor(network,cell->data(Qt::UserRole).toString().toStdString())))
                view->selectionModel()->select(view->model()->index(row,0),QItemSelectionModel::Select|QItemSelectionModel::Rows);
        }
    }
    syncing_=false;
}
}
