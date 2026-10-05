#include "../src/shell/editor_window.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/demand_preview.hpp"
#include <nlohmann/json.hpp>
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T> T* item(QObject& parent,const char* name){auto* value=parent.findChild<T*>(name);require(value,"Missing time/type control");return value;}
void modal(const std::function<void()>& open,const std::function<void(QDialog&)>& inspect) {
    std::exception_ptr failure;QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());
        try{require(d,"Missing modal dialog");inspect(*d);}catch(...){failure=std::current_exception();if(d)d->reject();}});
    open();if(failure)std::rethrow_exception(failure);
}
void confirm(QDialog& d){item<QDialogButtonBox>(d,"")->button(QDialogButtonBox::Ok)->click();}
void spin(QTableWidget& table,int row,int column,double value){auto* field=qobject_cast<QDoubleSpinBox*>(table.cellWidget(row,column));require(field,"Missing matrix field");field->setValue(value);}
void editComposition(QDialog& parent,bool cancel=false){
    item<QTableWidget>(parent,"editorCatalogCompositions")->selectRow(0);
    modal([&]{item<QPushButton>(parent,"editorCatalogCompositionscatalogEdit")->click();},[&](QDialog& child){
        modal([&]{item<QPushButton>(child,"editorCompositionPeriods")->click();},[&](QDialog& periods){
            item<QPushButton>(periods,"editorDemandAddPeriod")->click();auto* table=item<QTableWidget>(periods,"editorDemandPeriodsTable");
            spin(*table,0,0,20);spin(*table,0,1,40);confirm(periods);
            require(periods.isVisible() && !item<QLabel>(periods,"editorDemandPeriodsError")->text().isEmpty(),"Empty composition period accepted");
            // Catalog order is car, heavy vehicle; zero car excludes it during this period.
            spin(*table,0,3,1);confirm(periods);
        });if(cancel)child.reject();else confirm(child);
    });
}
void editRules(QDialog& parent,bool clear=false) {
    modal([&]{item<QPushButton>(parent,"editorTypeRules")->click();},[&](QDialog& rules){
        auto* tabs=item<QTabWidget>(rules,"editorTypeRuleTabs");
        for(int k=0;k<tabs->count();++k) {
            auto* page=tabs->widget(k);item<QCheckBox>(*page,"editorTypeRuleOverride")->setChecked(!clear);
            auto* table=item<QTableWidget>(*page,"editorTypeRuleWeights");
            if(!clear){const bool car=tabs->tabText(k)=="car";spin(*table,0,1,car?1:0);spin(*table,1,1,car?0:1);}
        }
        if(!clear) {
            auto* table=item<QTableWidget>(*tabs->widget(0),"editorTypeRuleWeights");spin(*table,0,1,0);spin(*table,1,1,0);confirm(rules);
            require(rules.isVisible() && !item<QLabel>(rules,"editorTypeRulesError")->text().isEmpty(),"Zero type total accepted");
            const bool car=tabs->tabText(0)=="car";spin(*table,0,1,car?1:0);spin(*table,1,1,car?0:1);
        }confirm(rules);
    });
}
}
int main(int argc,char** argv){
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try{
        require(argc>1,"Expected data directory");QTemporaryDir temp;ProjectDocument d;
        const auto link=addLink(d,{{0,0},{400,0}},2,3.5);putRoute(d,{"first",{link}});putRoute(d,{"second",{link}});changeRunSettings(d,120,.1);
        auto catalog=resolveDemandCatalog(AuthoringDefinition{},argv[1]);catalog.compositions={{"mix",{{"car",1},{"heavy-vehicle",1}}}};putDemandCatalog(d,catalog);
        putRoutingDecision(d,{"choose","",{{"first",1},{"second",1}}});VehicleInput input{"in","","",600,0,120};input.compositionId="mix";input.routingDecisionId="choose";putInput(d,input);
        const auto path=temp.filePath("time-types.traffic.json");QFile f(path);require(f.open(QIODevice::WriteOnly),"Cannot write fixture");f.write(QByteArray::fromStdString(documentJson(d).dump()));f.close();
        EditorWindow w(argv[1]);w.openFile(path);w.show();QApplication::processEvents();const auto original=documentJson(w.history().document());
        modal([&]{item<QAction>(w,"editorDemandCatalog")->trigger();},[](QDialog& parent){editComposition(parent,true);confirm(parent);});
        require(documentJson(w.history().document())==original,"Child Cancel changed composition");
        modal([&]{item<QAction>(w,"editorDemandCatalog")->trigger();},[](QDialog& parent){editComposition(parent);parent.reject();});
        require(documentJson(w.history().document())==original,"Parent Cancel changed composition");
        modal([&]{item<QAction>(w,"editorDemandCatalog")->trigger();},[](QDialog& parent){editComposition(parent);confirm(parent);});
        const auto timed=documentJson(w.history().document());require(timed["schemaVersion"]==19,"Timed composition did not upgrade schema");
        item<QTableWidget>(w,"editorDecisionTable")->selectRow(0);
        modal([&]{item<QAction>(w,"editorEditDecision")->trigger();},[](QDialog& parent){editRules(parent);parent.reject();});
        require(documentJson(w.history().document())==timed,"Type rule Cancel changed document");
        modal([&]{item<QAction>(w,"editorEditDecision")->trigger();},[](QDialog& parent){editRules(parent);confirm(parent);});
        const auto before=documentJson(w.history().document());require(w.history().document().definition->routingDecisions[0].typeRules.size()==2,"Rules not committed");
        const auto preview=previewDemand(w.history().document(),std::filesystem::path(argv[1])/"missing");require(std::abs(preview.expectedVehicles-20)<1e-9,"Preview lost demand");
        for(const auto& row:preview.rows)require(row.vehicleTypeId=="car"?row.routeId.starts_with("first/"):row.routeId.starts_with("second/"),"Type routing ignored UI rules");
        const auto revision=w.history().document().revision;
        modal([&]{item<QAction>(w,"editorEditDecision")->trigger();},[](QDialog& parent){
            modal([&]{item<QPushButton>(parent,"editorTypeRules")->click();},[](QDialog& child){confirm(child);});
            modal([&]{item<QPushButton>(parent,"editorDecisionPeriods")->click();},[](QDialog& child){confirm(child);});confirm(parent);
        });require(documentJson(w.history().document())==before && w.history().document().revision==revision,"No-op rule/period edit changed History");
        item<QAction>(w,"editorUndo")->trigger();require(documentJson(w.history().document())==timed,"Undo lost timed composition");
        item<QAction>(w,"editorRedo")->trigger();require(documentJson(w.history().document())==before,"Redo lost type matrices");
        w.saveFile(path);w.openFile(path);require(documentJson(w.history().document())==before,"Save/reopen changed rules");
        item<QTableWidget>(w,"editorDecisionTable")->selectRow(0);
        modal([&]{item<QAction>(w,"editorEditDecision")->trigger();},[](QDialog& parent){
            modal([&]{item<QPushButton>(parent,"editorDecisionPeriods")->click();},[](QDialog& periods){
                item<QPushButton>(periods,"editorDemandAddPeriod")->click();auto* table=item<QTableWidget>(periods,"editorDemandPeriodsTable");
                require(table->columnCount()==8,"Shared periods omitted type matrices");
                for(int c=0;c<8;++c)spin(*table,0,c,std::vector<double>{10,30,1,1,0,1,1,0}[c]);
                confirm(periods);
            });confirm(parent);
        });
        const auto shared=documentJson(w.history().document());const auto sharedPreview=previewDemand(w.history().document(),std::filesystem::path(argv[1])/"missing");
        require(std::abs(sharedPreview.expectedVehicles-20)<1e-9,"Shared period edit lost demand");
        for(const auto& row:sharedPreview.rows)if(row.startTime>=10 && row.endTime<=30)
            require(row.vehicleTypeId=="car"?row.routeId.starts_with("second/"):row.routeId.starts_with("first/"),"Shared type interval counts ignored");
        modal([&]{item<QAction>(w,"editorEditDecision")->trigger();},[](QDialog& parent){
            modal([&]{item<QPushButton>(parent,"editorTypeRules")->click();},[](QDialog& child){confirm(child);});confirm(parent);
        });require(documentJson(w.history().document())==shared,"No-op changed nonempty type period matrix");
        w.saveFile(path);w.openFile(path);require(documentJson(w.history().document())==shared,"Shared periods save/reopen failed");
        item<QTableWidget>(w,"editorDecisionTable")->selectRow(0);
        modal([&]{item<QAction>(w,"editorEditDecision")->trigger();},[](QDialog& parent){editRules(parent,true);confirm(parent);});
        require(w.history().document().definition->routingDecisions[0].typeRules.empty(),"Removing overrides failed");
        std::cout<<"time/type Demand UI tests passed\n";
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
