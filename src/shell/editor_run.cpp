#include "editor_window.hpp"
#include "../core/simulation.hpp"
#include "../project/load.hpp"
#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QToolBar>
#include <algorithm>
namespace trafficsim {
void EditorWindow::buildRunControls() {
    addToolBarBreak();
    auto* bar=addToolBar(QString());bar->setObjectName("editorRunToolbar");texts_["editorRunToolbar"]=bar;
    bar->addAction(action("editorRun",QKeySequence(Qt::Key_F5),[this]{toggleRun();}));
    bar->addAction(action("editorStep",QKeySequence(Qt::Key_F6),[this]{pauseRun();stepRun();}));
    auto* space=action("editorStepSpace",QKeySequence(Qt::Key_Space),[this]{pauseRun();stepRun();});
    space->setShortcutContext(Qt::WidgetWithChildrenShortcut);canvas_->addAction(space);
    bar->addAction(action("editorReset",{},[this]{clearRun();prepareRun();refreshRun();}));
    runSeed_=new QLineEdit("42",bar);runSeed_->setObjectName("editorSeed");runSeed_->setMaxLength(10);runSeed_->setMaximumWidth(100);
    auto* seedLabel=new QLabel(bar);texts_["seed"]=seedLabel;seedLabel->setBuddy(runSeed_);bar->addWidget(seedLabel);bar->addWidget(runSeed_);
    runSpeed_=new QComboBox(bar);runSpeed_->setObjectName("editorSpeed");
    for(double n:{0.5,1.,2.,5.,10.,20.})runSpeed_->addItem(QString::number(n)+"×",n);
    runSpeed_->setCurrentIndex(1);bar->addWidget(runSpeed_);
    auto* faster=action("editorFaster",QKeySequence(Qt::Key_Plus),[this]{runSpeed_->setCurrentIndex(std::min(runSpeed_->count()-1,runSpeed_->currentIndex()+1));});
    auto* slower=action("editorSlower",QKeySequence(Qt::Key_Minus),[this]{runSpeed_->setCurrentIndex(std::max(0,runSpeed_->currentIndex()-1));});
    for(auto* a:{faster,slower}){a->setShortcutContext(Qt::WidgetWithChildrenShortcut);canvas_->addAction(a);}
    runInfo_=new QLabel(bar);runInfo_->setObjectName("editorRunInfo");bar->addWidget(runInfo_);
    canvas_->stopRequested=[this]{pauseRun();};
    connect(runSeed_,&QLineEdit::textChanged,this,[this]{clearRun();});
    connect(&runTimer_,&QTimer::timeout,this,[this]{tickRun();});
}
void EditorWindow::pauseRun(){runTimer_.stop();runCredit_=0;refreshRun();}
void EditorWindow::clearRun(){
    runTimer_.stop();runCredit_=0;runSnapshot_.reset();runState_={};canvas_->clearRunFrame();refreshRun();
}
bool EditorWindow::prepareRun() {
    if(runSnapshot_)return true;
    try {
        std::uint32_t seed{};
        try{seed=parseSeed(runSeed_->text().toStdString());}catch(const std::exception&){throw std::invalid_argument("invalidSeed");}
        auto snapshot=compileDocument(history_.document(),data_);
        auto state=createSimulation(snapshot.scenario,seed);
        runSnapshot_=std::move(snapshot);runState_=std::move(state);
        canvas_->setRunNetwork(runSnapshot_->network);canvas_->setRunFrame(runState_);
        error_->clear();return true;
    }catch(const std::exception& e){
        diagnostics_=runDiagnostics(history_.document(),data_);diagnosticRevision_=history_.revision();
        showError(e);objects_->setCurrentIndex(3);return false;
    }
}
void EditorWindow::toggleRun(){
    if(runTimer_.isActive()){pauseRun();return;}
    if(!prepareRun())return;
    if(runState_.tick>=totalTicks(*runState_.scenario)){refreshRun();return;}
    runElapsed_.start();runCredit_=0;runTimer_.start(16);refreshRun();
}
void EditorWindow::stepRun(){
    if(!prepareRun())return;
    if(runState_.tick<totalTicks(*runState_.scenario))runState_=stepSimulation(runState_);
    if(runState_.tick>=totalTicks(*runState_.scenario))runTimer_.stop();
    canvas_->setRunFrame(runState_);refreshRun();
}
void EditorWindow::tickRun(){
    if(!runSnapshot_)return;
    runCredit_+=std::min(.25,runElapsed_.restart()/1000.)*runSpeed_->currentData().toDouble();
    // Bound work per callback, preserving every fixed step while keeping the UI responsive.
    int budget=200;
    while(runCredit_>=runState_.scenario->timeStep && runTimer_.isActive() && budget-->0) {
        runCredit_-=runState_.scenario->timeStep;
        runState_=stepSimulation(runState_);
        if(runState_.tick>=totalTicks(*runState_.scenario))runTimer_.stop();
    }
    runCredit_=std::min(runCredit_,5.);
    canvas_->setRunFrame(runState_);refreshRun();
}
void EditorWindow::refreshRun(){
    if(!runInfo_)return;
    actions_.at("editorRun")->setText(text(runTimer_.isActive()?"editorPause":"editorRun"));
    actions_.at("editorStep")->setEnabled(!runTimer_.isActive());
    runSeed_->setAccessibleName(text("seed"));runSpeed_->setAccessibleName(text("speed"));
    if(!runSnapshot_){runInfo_->setText(text("editorRunReady"));return;}
    runInfo_->setText(text("editorRunStatus").arg(runSnapshot_->revision).arg(runState_.seed)
        .arg(runState_.time,0,'f',1).arg(runState_.vehicles.size()).arg(pendingCount(runState_)).arg(runState_.completed));
}
}
