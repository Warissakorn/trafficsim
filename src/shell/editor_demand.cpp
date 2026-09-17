#include "editor_window.hpp"
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QAbstractButton>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

namespace trafficsim {
namespace {
std::string selectedId(QTableWidget* table) {
    const auto* cell=table->item(table->currentRow(),0);
    return cell?cell->data(Qt::UserRole).toString().toStdString():std::string{};
}
void row(QTableWidget* t,int r,const QStringList& values,const std::string& id) {
    for (int c=0;c<values.size();++c) {
        auto* cell=new QTableWidgetItem(values[c]); cell->setData(Qt::UserRole,QString::fromStdString(id));
        t->setItem(r,c,cell);
    }
}
}
void EditorWindow::buildDemandTables() {
    const auto page=[&](QTableWidget*& view,const char* name,const char* add,const char* edit,const char* remove,
                        const std::function<void(const std::string&)>& open,const char* kind) {
        auto* body=new QWidget(objects_); auto* layout=new QVBoxLayout(body);
        auto* bar=new QToolBar(body); layout->addWidget(bar);
        view=new QTableWidget(0,3,body); view->setObjectName(name);
        view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->horizontalHeader()->setStretchLastSection(true); view->verticalHeader()->hide();
        layout->addWidget(view); objects_->addTab(body,QString());
        auto* table=view;
        bar->addAction(action(add,{},[open]{open({});}));
        bar->addAction(action(edit,{},[table,open]{const auto id=selectedId(table); if(!id.empty()) open(id);}));
        bar->addAction(action(remove,{},[this,table,kind]{const auto id=selectedId(table); if(!id.empty()) deleteDemand(kind,id);}));
        connect(table,&QTableWidget::cellDoubleClicked,this,[table,open](int,int){open(selectedId(table));});
    };
    page(routeTable_,"editorRouteTable","editorAddRoute","editorEditRoute","editorDeleteRoute",
        [this](const auto& id){editRoute(id);},"route");
    page(inputTable_,"editorInputTable","editorAddInput","editorEditInput","editorDeleteInput",
        [this](const auto& id){editInput(id);},"input");
    page(programTable_,"editorProgramTable","editorAddProgram","editorEditProgram","editorDeleteProgram",
        [this](const auto& id){editProgram(id);},"program");
    connect(routeTable_,&QTableWidget::itemSelectionChanged,this,[this]{
        if(syncing_ || !history_.document().definition) return;
        const auto id=selectedId(routeTable_);
        for(const auto& r:history_.document().definition->routes) if(r.id==id) {
            std::vector<std::string> ids;
            for(const auto& s:r.segmentIds) {auto found=selectableFor(history_.document().network,s); if(!found.empty()) ids.push_back(found);}
            canvas_->setSelection(ids);
        }
    });
    auto* bar=new QToolBar(objects_); // Head editing acts on the selected signal row.
    bar->addAction(action("editorAddHead",{},[this]{editHead();}));
    bar->addAction(action("editorEditHead",{},[this]{auto id=selectedId(signalTable_);if(!id.empty())editHead(id);}));
    bar->addAction(action("editorDeleteHead",{},[this]{auto id=selectedId(signalTable_);if(!id.empty())deleteDemand("head",id);}));
    bar->addAction(action("editorRunSettings",{},[this]{editRunSettings();}));
    // The toolbar belongs to the containing dock layout, not the table viewport.
    qobject_cast<QVBoxLayout*>(objects_->parentWidget()->layout())->insertWidget(0,bar);
}
void EditorWindow::translateDemand() {
    const char* tabs[]={"editorRouteTable","editorInputTable","editorProgramTable"};
    for(int i=0;i<3;++i) objects_->setTabText(i+4,text(tabs[i]));
    routeTable_->setHorizontalHeaderLabels({text("editorColumnId"),text("editorRouteSegments"),text("editorColumnLength")});
    inputTable_->setHorizontalHeaderLabels({text("editorColumnId"),text("editorInputRoute"),text("editorInputVolume")});
    programTable_->setHorizontalHeaderLabels({text("editorColumnId"),text("editorProgramOffset"),text("editorProgramCycle")});
}
void EditorWindow::refreshDemand() {
    if (!routeTable_) return;
    const QSignalBlocker a(routeTable_), b(inputTable_), c(programTable_);
    const auto routeId=selectedId(routeTable_),inputId=selectedId(inputTable_),programId=selectedId(programTable_);
    routeTable_->setRowCount(0);inputTable_->setRowCount(0);programTable_->setRowCount(0);
    if(history_.document().definition) {
        const auto& def=*history_.document().definition;
        const auto scenario=buildScenario(history_.document().network,def);
        for(const auto& r:def.routes) {
            double length=0;QStringList ids;
            for(const auto& id:r.segmentIds)ids<<QString::fromStdString(id);
            // The author sees the ids they stored; the length comes from the COMPILED route,
            // which is the expanded chain of sections. Summing the authored ids instead would
            // charge a route that turns off part way along a lane for the whole lane.
            for(const auto& compiled:scenario.routes)if(compiled.id==r.id)
                for(const auto& id:compiled.segmentIds)for(const auto& s:scenario.segments)if(s.id==id)length+=s.length;
            const int n=routeTable_->rowCount();routeTable_->insertRow(n);
            row(routeTable_,n,{QString::fromStdString(r.id),ids.join(" → "),QString::number(length,'f',2)},r.id);
        }
        for(const auto& i:def.inputs) {
            const int n=inputTable_->rowCount();inputTable_->insertRow(n);
            row(inputTable_,n,{QString::fromStdString(i.id),QString::fromStdString(i.routeId),
                QString::number(i.vehiclesPerHour)+" ["+QString::number(i.startTime)+", "+QString::number(i.endTime)+"]"},i.id);
        }
        for(const auto& p:def.signalPrograms) {
            double cycle=0;for(const auto& f:p.phases)cycle+=f.duration;
            const int n=programTable_->rowCount();programTable_->insertRow(n);
            row(programTable_,n,{QString::fromStdString(p.id),QString::number(p.offset),QString::number(cycle)},p.id);
        }
    }
    for(const auto& entry:std::vector<std::pair<QTableWidget*,std::string>>{{routeTable_,routeId},{inputTable_,inputId},{programTable_,programId}}) {
        auto* t=entry.first;
        for(int r=0;r<t->rowCount();++r)if(t->item(r,0)->data(Qt::UserRole).toString().toStdString()==entry.second)t->selectRow(r);
        t->resizeColumnsToContents();
    }
}
void EditorWindow::selectDemand(const std::string& id) {
    for(auto* table:{routeTable_,inputTable_,programTable_})
        for(int r=0;r<table->rowCount();++r)if(table->item(r,0)->data(Qt::UserRole).toString().toStdString()==id) {
            objects_->setCurrentIndex(table==routeTable_?4:table==inputTable_?5:6);table->selectRow(r);return;
        }
}
void EditorWindow::deleteDemand(const std::string& kind,const std::string& id) {
    QMessageBox box(QMessageBox::Question,text("editorDeleteSelected"),text("editorDeleteDemandWarning"),
        QMessageBox::Yes|QMessageBox::No,this);
    box.button(QMessageBox::Yes)->setText(text("editorConfirm"));box.button(QMessageBox::No)->setText(text("editorCancel"));
    box.setDefaultButton(QMessageBox::No);if(box.exec()!=QMessageBox::Yes)return;
    execute("editorDeleteSelected",[&](auto& d){
        if(kind=="route")deleteRoute(d,id);else if(kind=="input")deleteInput(d,id);
        else if(kind=="program")deleteProgram(d,id);else deleteSignalHead(d,id);
    });
}
void EditorWindow::editRoute(const std::string& id,const std::vector<std::string>& initial) {
    Route value{id,initial};
    if(history_.document().definition)for(const auto& r:history_.document().definition->routes)if(r.id==id)value=r;
    QDialog dialog(this);dialog.setObjectName("editorRouteDialog");dialog.setWindowTitle(text("editorEditRoute"));
    auto* layout=new QVBoxLayout(&dialog);
    auto* help=new QLabel(text("editorRouteHelp"),&dialog);help->setWordWrap(true);layout->addWidget(help);
    auto* list=new QListWidget(&dialog);list->setObjectName("editorRoutePath");layout->addWidget(list);
    auto* next=new QComboBox(&dialog);next->setObjectName("editorRouteNext");layout->addWidget(next);
    auto* add=new QPushButton(text("editorAppendSegment"),&dialog);add->setObjectName("editorAppendSegment");layout->addWidget(add);
    auto* back=new QPushButton(text("editorRemoveLast"),&dialog);back->setObjectName("editorRemoveLast");layout->addWidget(back);
    // Whole lanes and connector paths, never a derived section id: a route is stored in the
    // project file, and offering a section would put a copy of derived data in it.
    const auto table=runtimeSections(history_.document().network);
    auto segments=authoringSegments(table);
    for(std::size_t p=0;p<table.paths.size();++p)
        segments.push_back({table.paths[p].id,polylineLength(table.paths[p].geometry),{table.pathNext[p]}});
    // A path's successor is a section; name the lane it belongs to, which is what an author picks.
    for(auto& segment:segments)for(auto& next:segment.next)
        for(const auto& section:table.sections)if(section.id==next)next=section.laneId;
    const auto refresh=[&] {
        list->clear();next->clear();
        for(const auto& s:value.segmentIds)list->addItem(QString::fromStdString(s));
        for(const auto& s:segments) {
            bool allowed=value.segmentIds.empty();
            if(!allowed)for(const auto& last:segments)if(last.id==value.segmentIds.back())
                allowed=std::find(last.next.begin(),last.next.end(),s.id)!=last.next.end();
            if(allowed && std::find(value.segmentIds.begin(),value.segmentIds.end(),s.id)==value.segmentIds.end())
                next->addItem(QString::fromStdString(s.id),QString::fromStdString(s.id));
        }
        add->setEnabled(next->count()>0);back->setEnabled(!value.segmentIds.empty());
    };
    connect(add,&QPushButton::clicked,&dialog,[&]{value.segmentIds.push_back(next->currentData().toString().toStdString());refresh();});
    connect(back,&QPushButton::clicked,&dialog,[&]{if(!value.segmentIds.empty())value.segmentIds.pop_back();refresh();});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    refresh();dialog.resize(500,430);if(dialog.exec()!=QDialog::Accepted)return;
    std::string created;if(execute("editorEditRoute",[&](auto& d){created=putRoute(d,value);}))selectDemand(created);
}
void EditorWindow::editInput(const std::string& id) {
    VehicleInput value{id,{},{},600,0,history_.document().definition?history_.document().definition->duration:180};
    if(history_.document().definition)for(const auto& i:history_.document().definition->inputs)if(i.id==id)value=i;
    ScenarioDefinition catalog;
    try {catalog=resolveCatalogs(history_.document().definition.value_or(AuthoringDefinition{}),data_);}
    catch(const std::exception& e){showError(e);return;}
    QDialog dialog(this);dialog.setObjectName("editorInputDialog");dialog.setWindowTitle(text("editorEditInput"));
    auto* form=new QFormLayout(&dialog);
    auto* route=new QComboBox(&dialog);route->setObjectName("editorInputRoute");
    for(const auto& r:catalog.routes)route->addItem(QString::fromStdString(r.id));
    if(!value.routeId.empty())route->setCurrentText(QString::fromStdString(value.routeId));
    form->addRow(text("editorInputRoute"),route);
    auto* type=new QComboBox(&dialog);type->setObjectName("editorInputType");
    for(const auto& t:catalog.vehicleTypes)type->addItem(QString::fromStdString(t.id));
    if(!value.vehicleTypeId.empty())type->setCurrentText(QString::fromStdString(value.vehicleTypeId));
    form->addRow(text("editorInputType"),type);
    const auto number=[&](const char* key,double v){
        auto* field=new QDoubleSpinBox(&dialog);field->setObjectName(key);field->setRange(0,10000000);field->setDecimals(3);field->setValue(v);
        form->addRow(text(key),field);return field;
    };
    auto* volume=number("editorInputVolume",value.vehiclesPerHour);
    auto* start=number("editorInputStart",value.startTime);auto* end=number("editorInputEnd",value.endTime);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    buttons->button(QDialogButtonBox::Ok)->setEnabled(route->count()>0 && type->count()>0);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    value.routeId=route->currentText().toStdString();value.vehicleTypeId=type->currentText().toStdString();
    value.vehiclesPerHour=volume->value();value.startTime=start->value();value.endTime=end->value();
    std::string created;if(execute("editorEditInput",[&](auto& d){created=putInput(d,value);}))selectDemand(created);
}
}
