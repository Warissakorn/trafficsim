#include "editor_window.hpp"
#include "editor_style.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QVBoxLayout>
#include "../core/simulation.hpp"
#include "../core/routes.hpp"
#include "../project/load.hpp"
#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QToolBar>
#include <algorithm>
namespace trafficsim {
// Every path that advances the run funnels through here. SimState::events holds the latest
// step only, so a step whose events are not accumulated is one whose clamps are lost for good.
void EditorWindow::observeRun() {
    for (const auto& event : runState_.events) {
        runSummary_.add(event);
        if (const auto* clamp = std::get_if<SafetyClampEvent>(&event)) {
            ClampRow row{clamp->time, clamp->vehicleId, {}, {}, {}};
            const auto& vehicles = runState_.vehicles; // sorted by id
            const auto v = std::lower_bound(vehicles.begin(), vehicles.end(), clamp->vehicleId,
                                            [](const auto& x, std::uint64_t id) { return x.id < id; });
            if (v != vehicles.end() && v->id == clamp->vehicleId) {
                const auto& scenario = *runState_.scenario;
                row.typeId = scenario.vehicleTypes[v->typeIndex].id; row.routeId = scenario.routes[v->routeIndex].id;
                row.segmentId = locateVehicle(scenario, *v, *runState_.index).segmentId;
            }
            runClamps_.push_back(std::move(row));
        }
    }
    if (runMovements_) runMovements_->observe(runState_);
    if (runDischarge_) runDischarge_->observe(runState_);
}
void EditorWindow::buildRunControls() {
    auto* bar=addToolBar(QString());bar->setObjectName("editorRunToolbar");texts_["editorRunToolbar"]=bar;
    bar->addAction(action("editorRun",QKeySequence(Qt::Key_F5),[this]{toggleRun();}));
    bar->addAction(action("editorStep",QKeySequence(Qt::Key_F6),[this]{pauseRun();stepRun();}));
    auto* space=action("editorStepSpace",QKeySequence(Qt::Key_Space),[this]{pauseRun();stepRun();});
    space->setShortcutContext(Qt::WidgetWithChildrenShortcut);canvas_->addAction(space);
    bar->addAction(action("editorReset",{},[this]{clearRun();prepareRun();refreshRun();}));
    runSeed_=new QLineEdit("42",bar);runSeed_->setObjectName("editorSeed");runSeed_->setMaxLength(10);runSeed_->setMaximumWidth(80);
    runSeed_->setFont(editorDesign::numericFont());runSeed_->setProperty("numeric",true);
    auto* seedLabel=new QLabel(bar);texts_["editorSeedLabel"]=seedLabel;seedLabel->setBuddy(runSeed_);bar->addWidget(seedLabel);bar->addWidget(runSeed_);
    runSpeed_=new QComboBox(bar);runSpeed_->setObjectName("editorSpeed");
    for(double n:{0.5,1.,2.,5.,10.,20.})runSpeed_->addItem(QString::number(n)+"×",n);
    runSpeed_->setCurrentIndex(1);bar->addWidget(runSpeed_);
    auto* faster=action("editorFaster",QKeySequence(Qt::Key_Plus),[this]{runSpeed_->setCurrentIndex(std::min(runSpeed_->count()-1,runSpeed_->currentIndex()+1));});
    auto* slower=action("editorSlower",QKeySequence(Qt::Key_Minus),[this]{runSpeed_->setCurrentIndex(std::max(0,runSpeed_->currentIndex()-1));});
    for(auto* a:{faster,slower}){a->setShortcutContext(Qt::WidgetWithChildrenShortcut);canvas_->addAction(a);}
    runInfo_=new QLabel(centralWidget());runInfo_->setObjectName("editorRunInfo");
    runInfo_->setFont(editorDesign::numericFont());runInfo_->setProperty("numeric",true);
    runInfo_->setWordWrap(true);runInfo_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    qobject_cast<QVBoxLayout*>(centralWidget()->layout())->insertWidget(1,runInfo_);
    canvas_->stopRequested=[this]{pauseRun();};
    connect(runSeed_,&QLineEdit::textChanged,this,[this]{clearRun();});
    connect(&runTimer_,&QTimer::timeout,this,[this]{tickRun();});
}
void EditorWindow::pauseRun(){runTimer_.stop();runCredit_=0;refreshRun();}
void EditorWindow::clearRun(){
    cancelBatch(); // every edit, Undo, Open and Reset passes here, so it also ends a batch (M5.6)
    runTimer_.stop();runCredit_=0;runSnapshot_.reset();runState_={};runSummary_={};runMovements_.reset();
    runDischarge_.reset();runDischargeError_.clear();runClamps_.clear();canvas_->clearRunFrame();refreshRun();
}
bool EditorWindow::prepareRun() {
    if(runSnapshot_)return true;
    try {
        std::uint32_t seed{};
        try{seed=parseSeed(runSeed_->text().toStdString());}catch(const std::exception&){throw std::invalid_argument("invalidSeed");}
        auto snapshot=compileDocument(history_.document(),data_);
        auto state=createSimulation(snapshot.scenario,seed);
        const auto evaluation=evaluationSpec(history_.document(),snapshot,data_);
        MovementAccumulator movements(evaluation);
        // The CLI's --discharge defaults: the whole run, no warmup, ranks 3-5 steady and 1-2 startup.
        try {
            DischargeSpec discharge; discharge.windowEnd=snapshot.scenario.duration; validateDischargeSpec(discharge);
            runDischarge_.emplace(discharge,evaluation.queue);
        }catch(const std::exception& e){runDischarge_.reset();runDischargeError_=e.what();}
        runSnapshot_=std::move(snapshot);runState_=std::move(state);runMovements_.emplace(std::move(movements));
        if(batchRunning()||batch_)cancelBatch(); // one table at a time: a single run replaces the batch
        // createSimulation can already emit events at t=0; start the count from them, not from
        // the first step, or a departure at time zero is missing from every later figure.
        runSummary_={};runClamps_.clear();observeRun();
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
    if(runState_.tick<totalTicks(*runState_.scenario)){runState_=stepSimulation(std::move(runState_));observeRun();}
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
        runState_=stepSimulation(std::move(runState_));observeRun();
        if(runState_.tick>=totalTicks(*runState_.scenario))runTimer_.stop();
    }
    runCredit_=std::min(runCredit_,5.);
    canvas_->setRunFrame(runState_);refreshRun();
}
void EditorWindow::refreshRun(){
    if(!runInfo_)return;
    refreshResults();
    if(runProtection_)runProtection_->setVisible(runSnapshot_.has_value()); // M3.2.4: shown with every run
    actions_.at("editorRun")->setText(text(runTimer_.isActive()?"editorPause":"editorRun"));
    actions_.at("editorRun")->setIcon(editorIcon(runTimer_.isActive()?EditorIcon::pause:EditorIcon::run));
    actions_.at("editorStep")->setEnabled(!runTimer_.isActive());
    const bool exportable=runFinished()||batch_.has_value();
    for(const char* key:{"editorExportResults","editorCopyResults"})
        if(const auto e=actions_.find(key);e!=actions_.end())e->second->setEnabled(exportable);
    if(const auto e=actions_.find("editorRunSeeds");e!=actions_.end())e->second->setEnabled(!batchRunning());
    if(const auto e=actions_.find("editorCancelSeeds");e!=actions_.end())e->second->setEnabled(batchRunning());
    runSeed_->setAccessibleName(text("seed"));runSpeed_->setAccessibleName(text("speed"));
    if(!runSnapshot_){runInfo_->setText(text("editorRunReady"));runInfo_->setToolTip({});return;}
    // All diagnostic figures wrap above the canvas; their interpretation stays in the tooltip.
    runInfo_->setToolTip(text("editorRunStatusNote"));
    const auto summary=runSummary_.summary();
    runInfo_->setText(text("editorRunStatus").arg(runSnapshot_->revision).arg(runState_.seed)
        .arg(runState_.time,0,'f',1).arg(runState_.vehicles.size()).arg(pendingCount(runState_)).arg(runState_.completed)
        .arg(summary.meanDelay?QString::number(*summary.meanDelay,'f',2):text("empty")).arg(summary.safetyClamps));
}
}
