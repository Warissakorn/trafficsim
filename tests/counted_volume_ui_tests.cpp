// D142 (row VC8): the input dialog's "Volume from the decision's turning counts". Offered only
// with a decision to read, it locks the typed volume and counts and shows the counted total; the
// input table says so, and the file reopens as the same document.
#include "../src/shell/editor_window.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
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
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>1,"Expected data directory");QTemporaryDir temp;require(temp.isValid(),"No temp directory");
        ProjectDocument d;const auto link=addLink(d,{{0,0},{400,0}},1,3.5);
        const auto route=putRoute(d,{"route",{link}});changeRunSettings(d,300,.1);
        RoutingDecision decision{"decision","Count sheet",{{route,1,"",{2,8}}}};
        decision.intervals={{0,60},{60,120}};putRoutingDecision(d,decision);
        putInput(d,{"input",route,"car",600,0,300});
        const auto path=temp.filePath("counts.traffic.json");
        {QFile file(path);require(file.open(QIODevice::WriteOnly),"Cannot create fixture");
         file.write(QByteArray::fromStdString(documentJson(d).dump()));}
        EditorWindow w(argv[1]);w.openFile(path);w.show();QApplication::processEvents();
        control<QTableWidget>(w,"editorInputTable")->selectRow(0);
        edit(w,"editorEditInput",[&](QDialog& dialog){
            auto* box=control<QCheckBox>(dialog,"editorInputFromCounts");
            require(!box->isEnabled(),"Counts offered for an input on a plain route");
            auto* target=control<QComboBox>(dialog,"editorInputRoute");
            target->setCurrentIndex(target->findData("decision:decision"));
            require(box->isEnabled(),"Counts not offered for an input on a decision");
            box->setChecked(true);
            require(!control<QDoubleSpinBox>(dialog,"editorInputVolume")->isEnabled() &&
                    !control<QPlainTextEdit>(dialog,"editorInputCounts")->isEnabled() &&
                    !control<QPushButton>(dialog,"editorInputPeriods")->isEnabled(),"A typed volume stays editable beside the counts");
            auto* total=control<QLabel>(dialog,"editorInputFromCountsTotal");
            require(!total->isHidden() && total->text().contains("10 vehicles") && total->text().contains("2 counted"),
                    "The counted total is not shown");
            dialog.accept();
        });
        const auto& input=w.history().document().definition->inputs.front();
        require(input.volumeFromCounts && input.routingDecisionId=="decision","The input does not take its volume from the counts");
        require(input.intervals==(std::vector<VolumeInterval>{{0,60,120},{60,120,480}}),"The volume is not the counts");
        require(control<QTableWidget>(w,"editorInputTable")->item(0,2)->text().contains("volume from counts"),"The input row does not say so");
        require(control<QTableWidget>(w,"editorDecisionTable")->item(0,2)->text().contains("volume of 1 input"),"The decision row does not say so");
        const auto edited=w.history().document();
        w.saveFile(path);w.openFile(path);
        require(w.history().document().definition->inputs==edited.definition->inputs,"Counted volume did not round-trip");
        QFile file(path);require(file.open(QIODevice::ReadOnly),"Saved file missing");
        const auto j=nlohmann::json::parse(file.readAll().toStdString());
        require(j["schemaVersion"]==26 && !j["definition"]["inputs"][0].contains("intervals"),"The file holds a second copy of the volume");
        control<QAction>(w,"editorUndo")->trigger();
    } catch(const std::exception& e) {std::cerr<<"counted-volume-ui: "<<e.what()<<'\n';return 1;}
    std::cout<<"counted-volume-ui: ok\n";return 0;
}
