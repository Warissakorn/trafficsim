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
#include <memory>
#include <vector>

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
        layout->setContentsMargins(0,0,0,0);layout->setSpacing(3);
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
    // M2.7b: signal controllers, and below them any legacy program an older file still carries.
    page(programTable_,"editorProgramTable","editorAddController","editorEditController","editorDeleteController",
        [this](const auto& id){
            const auto& def=history_.document().definition;
            if(!id.empty() && def && std::any_of(def->signalPrograms.begin(),def->signalPrograms.end(),[&](const auto& p){return p.id==id;}))
                editProgram(id);
            else editController(id);
        },"signal");
    // Last, so every earlier tab keeps the index the rest of the window already addresses it by.
    page(decisionTable_,"editorDecisionTable","editorAddDecision","editorEditDecision","editorDeleteDecision",
        [this](const auto& id){editDecision(id);},"decision");
    connect(routeTable_,&QTableWidget::itemSelectionChanged,this,[this]{syncHighlightedRoute();});
    connect(inputTable_,&QTableWidget::itemSelectionChanged,this,[this]{syncHighlightedRoute();});
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
    action("editorRunSettings",{},[this]{editRunSettings();});
    // The toolbar belongs to the containing dock layout, not the table viewport.
    qobject_cast<QVBoxLayout*>(objects_->parentWidget()->layout())->insertWidget(0,bar);
    bar->setVisible(objects_->currentIndex()==2);
    connect(objects_,&QTabWidget::currentChanged,bar,[bar](int index){bar->setVisible(index==2);});
}
void EditorWindow::translateDemand() {
    const char* tabs[]={"editorRouteTable","editorInputTable","editorProgramTable","editorDecisionTable"};
    for(int i=0;i<4;++i) objects_->setTabText(i+4,text(tabs[i]));
    decisionTable_->setHorizontalHeaderLabels({text("editorColumnId"),text("editorDecisionName"),text("editorDecisionRoutes")});
    routeTable_->setHorizontalHeaderLabels({text("editorColumnId"),text("editorRouteSegments"),text("editorColumnLength")});
    inputTable_->setHorizontalHeaderLabels({text("editorColumnId"),text("editorInputRoute"),text("editorInputVolume")});
    programTable_->setHorizontalHeaderLabels({text("editorColumnId"),text("editorColumnName"),text("editorControllerTiming")});
}
void EditorWindow::refreshDemand() {
    if (!routeTable_) return;
    const QSignalBlocker a(routeTable_), b(inputTable_), c(programTable_), e(decisionTable_);
    const auto routeId=selectedId(routeTable_),inputId=selectedId(inputTable_),programId=selectedId(programTable_);
    const auto decisionId=selectedId(decisionTable_);
    routeTable_->setRowCount(0);inputTable_->setRowCount(0);programTable_->setRowCount(0);decisionTable_->setRowCount(0);
    if(history_.document().definition) {
        const auto& def=*history_.document().definition;
        const auto scenario=buildScenario(history_.document().network,def);
        for(const auto& r:def.routes) {
            double length=0;QStringList ids;
            for(const auto& id:r.segmentIds)ids<<QString::fromStdString(id);
            // The author sees the ids they stored; the length comes from the COMPILED route,
            // which is the expanded chain of sections. Summing the authored ids instead would
            // charge a route that turns off part way along a lane for the whole lane.
            // A route covers every lane, so the compiled id is the authored one only when it
            // expanded to a single lane; otherwise the lanes carry "/lane-k" and the row shows
            // the first of them -- one lane's distance, not the sum of all of them.
            const auto prefix=r.id+"/lane-";
            for(const auto& compiled:scenario.routes) {
                if(compiled.id!=r.id && compiled.id.rfind(prefix,0)!=0) continue;
                for(const auto& id:compiled.segmentIds)for(const auto& s:scenario.segments)if(s.id==id)length+=s.length;
                break;
            }
            const int n=routeTable_->rowCount();routeTable_->insertRow(n);
            row(routeTable_,n,{QString::fromStdString(r.id),ids.join(" → "),QString::number(length,'f',2)},r.id);
        }
        for(const auto& i:def.inputs) {
            // The authored number is the Link total. What the run receives is that total divided
            // across the lanes the route reaches, so the row says both -- an author reading only
            // the total would not know what each lane actually gets.
            std::size_t lanes=0;
            for(const auto& r:def.routes)if(r.id==i.routeId)
                lanes=routeLaneChains(history_.document().network,r.segmentIds).size();
            // M2.1.1: a routeless input names its Link, and splits across that Link's lanes.
            auto target=QString::fromStdString(i.routeId.empty()?i.routingDecisionId:i.routeId);
            if(!i.linkId.empty()) {
                target=text("editorInputLinkItem").arg(QString::fromStdString(i.linkId));
                for(const auto& l:history_.document().network.links)if(l.id==i.linkId)lanes=l.lanes.size();
                // A decision placed on this Link chooses the lanes by destination, so no equal split.
                for(const auto& x:def.routingDecisions)if(x.linkId==i.linkId)lanes=0;
            }
            auto volume=QString::number(i.vehiclesPerHour);
            if(lanes>1)volume+=" = "+QString::number(lanes)+QString::fromUtf8(" \u00d7 ")+
                QString::number(i.vehiclesPerHour/static_cast<double>(lanes),'f',1);
            const int n=inputTable_->rowCount();inputTable_->insertRow(n);
            // With counted intervals (M2.2) the figure is their mean over the span; say so.
            auto period=" ["+QString::number(i.startTime)+", "+QString::number(i.endTime)+"]";
            if(!i.intervals.empty())period+=" · "+text("editorInputIntervalCount").arg(static_cast<int>(i.intervals.size()));
            row(inputTable_,n,{QString::fromStdString(i.id),target,volume+period},i.id);
        }
        for(const auto& x:def.routingDecisions) {
            QStringList flows;
            for(const auto& r:x.routes)flows<<QString::fromStdString(r.destinationLinkId.empty()?r.routeId:"\u2192 "+r.destinationLinkId)
                +" \u00d7 "+QString::number(r.relativeFlow);
            // A placed decision (M2.1.1) says where it sits.
            if(!x.linkId.empty())flows.prepend(text("editorDecisionAtLink").arg(QString::fromStdString(x.linkId)));
            if(!x.intervals.empty())flows<<text("editorDecisionIntervalCount").arg(static_cast<int>(x.intervals.size())); // M2.1.2
            const int n=decisionTable_->rowCount();decisionTable_->insertRow(n);
            row(decisionTable_,n,{QString::fromStdString(x.id),QString::fromStdString(x.name),flows.join(", ")},x.id);
        }
        for(const auto& c:def.signalControllers) {
            const int n=programTable_->rowCount();programTable_->insertRow(n);
            row(programTable_,n,{QString::fromStdString(c.id),QString::fromStdString(c.name),
                text("editorControllerSummary").arg(c.cycle).arg(c.offset).arg(static_cast<int>(c.groups.size()))},c.id);
        }
        for(const auto& p:def.signalPrograms) {
            double cycle=0;for(const auto& f:p.phases)cycle+=f.duration;
            const int n=programTable_->rowCount();programTable_->insertRow(n);
            row(programTable_,n,{QString::fromStdString(p.id),text("editorLegacyProgram").arg(QString::fromStdString(p.id)),
                text("editorControllerSummary").arg(cycle).arg(p.offset).arg(1)},p.id);
        }
    }
    for(const auto& entry:std::vector<std::pair<QTableWidget*,std::string>>{{routeTable_,routeId},{inputTable_,inputId},{programTable_,programId},{decisionTable_,decisionId}}) {
        auto* t=entry.first;
        for(int r=0;r<t->rowCount();++r)if(t->item(r,0)->data(Qt::UserRole).toString().toStdString()==entry.second)t->selectRow(r);
        t->resizeColumnsToContents();
    }
}
void EditorWindow::selectDemand(const std::string& id) {
    for(auto* table:{routeTable_,inputTable_,programTable_,decisionTable_})
        for(int r=0;r<table->rowCount();++r)if(table->item(r,0)->data(Qt::UserRole).toString().toStdString()==id) {
            objects_->setCurrentIndex(table==routeTable_?4:table==inputTable_?5:table==programTable_?6:7);table->selectRow(r);return;
        }
}
void EditorWindow::deleteDemand(const std::string& kind,const std::string& id) {
    QMessageBox box(QMessageBox::Question,text("editorDeleteSelected"),text("editorDeleteDemandWarning"),
        QMessageBox::Yes|QMessageBox::No,this);
    box.button(QMessageBox::Yes)->setText(text("editorConfirm"));box.button(QMessageBox::No)->setText(text("editorCancel"));
    box.setDefaultButton(QMessageBox::No);if(box.exec()!=QMessageBox::Yes)return;
    execute("editorDeleteSelected",[&](auto& d){
        if(kind=="route")deleteRoute(d,id);else if(kind=="input")deleteInput(d,id);
        else if(kind=="signal") {
            const auto& controllers=demand(d).signalControllers;
            if(std::any_of(controllers.begin(),controllers.end(),[&](const auto& c){return c.id==id;}))deleteSignalController(d,id);
            else deleteProgram(d,id);
        }else if(kind=="decision")deleteRoutingDecision(d,id);
        else deleteSignalHead(d,id);
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
    const auto refresh=[&] {
        list->clear();next->clear();
        for(const auto& s:value.segmentIds)list->addItem(QString::fromStdString(s));
        // Links and Connectors, never a lane and never a derived section id: a route is stored
        // in the project file and belongs to the carriageway, so narrowing a Connector must not
        // be able to invalidate it. The rule lives in the model, where the canvas reads the same one.
        for(const auto& id:routeContinuations(history_.document().network,value.segmentIds))
            next->addItem(QString::fromStdString(id),QString::fromStdString(id));
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
}
