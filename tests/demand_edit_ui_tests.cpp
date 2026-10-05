#include "../src/shell/editor_window.hpp"
#include "../src/shell/demand_period_editor.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLineEdit>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <cmath>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T> T* control(QObject& parent,const char* name) {
    auto* found=parent.findChild<T*>(name);require(found,"Missing control");return found;
}
void edit(EditorWindow& w,const char* action,const std::function<void(QDialog&)>& inspect) {
    std::exception_ptr failure;
    QTimer::singleShot(0,[&]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
        try {require(dialog,"No demand dialog");inspect(*dialog);}
        catch(...) {failure=std::current_exception();if(dialog)dialog->reject();}
    });
    control<QAction>(w,action)->trigger();
    if(failure)std::rethrow_exception(failure);
}
ProjectDocument fixture() {
    ProjectDocument d;const auto link=addLink(d,{{0,0},{400,0}},2,3.5);
    const auto route=putRoute(d,{"route",{link}});changeRunSettings(d,300,.1);
    VehicleInput input{"input",route,"car",0,0,0};
    input.intervals={{0,60,900},{90,210,1200}};putInput(d,input);
    RoutingDecision decision{"decision","Original",{{route,10,"",{2,8}}}};
    decision.intervals={{0,60},{90,210}};putRoutingDecision(d,decision);
    return d;
}
void save(const QString& path,const ProjectDocument& d) {
    QFile file(path);require(file.open(QIODevice::WriteOnly),"Cannot create fixture");
    const auto bytes=QByteArray::fromStdString(documentJson(d).dump());
    require(file.write(bytes)==bytes.size(),"Cannot write fixture");
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>1,"Expected data directory");QTemporaryDir temp;require(temp.isValid(),"No temp directory");
        const auto original=fixture();const auto path=temp.filePath("demand.traffic.json");save(path,original);
        EditorWindow w(argv[1]);w.openFile(path);w.show();QApplication::processEvents();
        control<QTableWidget>(w,"editorInputTable")->selectRow(0);
        edit(w,"editorEditInput",[&](QDialog& d){
            require(!control<QDoubleSpinBox>(d,"editorInputVolume")->isEnabled(),"Derived volume is editable");
            require(!control<QDoubleSpinBox>(d,"editorInputStart")->isEnabled(),"Derived start is editable");
            require(!control<QDoubleSpinBox>(d,"editorInputEnd")->isEnabled(),"Derived end is editable");
            d.accept();
        });
        require(w.history().document().definition->inputs==original.definition->inputs,"No-op input edit changed irregular periods");
        control<QTableWidget>(w,"editorDecisionTable")->selectRow(0);
        edit(w,"editorEditDecision",[](QDialog& d){d.accept();});
        require(w.history().document().definition->routingDecisions==original.definition->routingDecisions,"No-op decision edit lost irregular counts");
        edit(w,"editorEditDecision",[](QDialog& d){control<QLineEdit>(d,"editorDecisionNameField")->setText("Renamed");d.accept();});
        const auto renamed=w.history().document().definition->routingDecisions.front();
        require(renamed.name=="Renamed" && renamed.intervals==original.definition->routingDecisions.front().intervals &&
                renamed.routes==original.definition->routingDecisions.front().routes,"Name edit changed interval demand");
        control<QAction>(w,"editorUndo")->trigger();
        require(w.history().document().definition->routingDecisions==original.definition->routingDecisions,"Undo failed to restore demand");
        edit(w,"editorEditDecision",[](QDialog& d){control<QLineEdit>(d,"editorDecisionNameField")->setText("Cancelled");d.reject();});
        require(w.history().document().definition->routingDecisions==original.definition->routingDecisions,"Cancel changed demand");
        w.saveFile(path);w.openFile(path);
        require(w.history().document().definition->inputs==original.definition->inputs &&
                w.history().document().definition->routingDecisions==original.definition->routingDecisions,"Demand did not round-trip");
        const auto beforePreview=documentJson(w.history().document());
        edit(w,"editorDemandPreview",[](QDialog& d){
            auto* table=control<QTableWidget>(d,"editorDemandPreviewTable");
            require(table->rowCount()==4,"Preview did not expand the two lanes and intervals");
            require(table->editTriggers()==QAbstractItemView::NoEditTriggers,"Preview permits edits");
            double expected=0;for(int r=0;r<table->rowCount();++r)expected+=table->item(r,9)->text().toDouble();
            require(std::abs(expected-55)<1e-9,"Preview total differs from authored demand");d.reject();
        });
        require(documentJson(w.history().document())==beforePreview,"Preview mutated project");
        control<QTableWidget>(w,"editorInputTable")->selectRow(0);
        edit(w,"editorEditInput",[](QDialog& parent){
            QTimer::singleShot(0,[]{
                auto* child=qobject_cast<QDialog*>(QApplication::activeModalWidget());
                auto* table=child->findChild<QTableWidget*>("editorDemandPeriodsTable");
                qobject_cast<QDoubleSpinBox*>(table->cellWidget(1,2))->setValue(600);
                child->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
            });
            control<QPushButton>(parent,"editorInputPeriods")->click();parent.accept();
        });
        const auto edited=w.history().document().definition->inputs.front();
        require(edited.intervals[1].vehiclesPerHour==600 && edited.intervals[1].startTime==90,"Explicit editor lost interval gap");
        control<QAction>(w,"editorUndo")->trigger();
        require(w.history().document().definition->inputs==original.definition->inputs,"Interval edit undo changed original demand");
        control<QTableWidget>(w,"editorDecisionTable")->selectRow(0);
        edit(w,"editorEditDecision",[](QDialog& parent){
            QTimer::singleShot(0,[]{
                auto* child=qobject_cast<QDialog*>(QApplication::activeModalWidget());
                auto* table=child->findChild<QTableWidget*>("editorDemandPeriodsTable");
                qobject_cast<QDoubleSpinBox*>(table->cellWidget(1,2))->setValue(4);
                child->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
            });
            control<QPushButton>(parent,"editorDecisionPeriods")->click();parent.accept();
        });
        const auto decision=w.history().document().definition->routingDecisions.front();
        require(decision.routes.front().intervalFlows==std::vector<double>({2,4}) &&
            decision.intervals==original.definition->routingDecisions.front().intervals,"Explicit decision editor lost flows or gaps");
        control<QAction>(w,"editorUndo")->trigger();
        require(w.history().document().definition->routingDecisions==original.definition->routingDecisions,"Decision interval undo failed");
        const std::vector<std::vector<double>> precise{{0,60,1.0/3}};
        QTimer::singleShot(0,[]{
            auto* child=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            child->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
        });
        const auto preserved=editDemandPeriods(&w,{"Start","End","Rate"},precise,
            [](const char* key){return QString::fromUtf8(key);});
        require(preserved && *preserved==precise,"Unedited period value lost numeric precision");
        bool overlapAccepted=false;
        QTimer::singleShot(0,[&]{
            auto* child=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            child->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
            // Overlap must keep the dialog open; Cancel then leaves no edit to commit.
            overlapAccepted=!child->isVisible();
            child->reject();
        });
        require(!editDemandPeriods(&w,{"Start","End","Rate"},{{0,60,900},{30,90,900}},
            [](const char* key){return QString::fromUtf8(key);}),"Invalid period dialog returned edits");
        require(!overlapAccepted,"Overlapping intervals accepted");
        auto counted=original;
        putRoute(counted,{"other",counted.definition->routes.front().segmentIds});
        auto& turning=counted.definition->routingDecisions.front();
        turning.intervals={{0,60},{60,120}};turning.routes.push_back({"other",5,"",{1,4}});
        const auto countPath=temp.filePath("counts.traffic.json");save(countPath,counted);w.openFile(countPath);
        control<QTableWidget>(w,"editorDecisionTable")->selectRow(0);
        const auto beforeCounts=documentJson(w.history().document());
        edit(w,"editorEditDecision",[&](QDialog& parent){
            control<QLineEdit>(parent,"editorDecisionCounts1")->setText("3");
            parent.accept();
        });
        require(!control<QLabel>(w,"editorError")->text().isEmpty(),"Missing turn count did not produce an error");
        require(documentJson(w.history().document())==beforeCounts,"Missing turn count changed history");
        std::cout<<"demand edit ui ok\n";
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
