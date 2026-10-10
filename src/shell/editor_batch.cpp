#include "editor_window.hpp"
#include "editor_batch_cells.hpp"
#include "../project/batch_output.hpp"
#include "../project/load.hpp"
#include "../project/los_output.hpp"
#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>

namespace trafficsim {
// M5.6 (BATCH §6): the editor's `--seeds`. The document is compiled here, on the UI thread, and
// only value copies cross to the worker, which calls the CLI's runSeeds and aggregate; Export and
// Copy then format with the CLI's batchCsv. Every number keeps the not-validated marker (rule 4).
using namespace batchCells;
void EditorWindow::buildBatch(QToolBar* bar, QVBoxLayout* layout) {
    bar->addSeparator();
    auto* label=new QLabel(bar); texts_["editorSeedsLabel"]=label;
    batchSeeds_=new QLineEdit("42-51",bar); batchSeeds_->setObjectName("editorSeeds"); batchSeeds_->setMaximumWidth(120);
    batchSeeds_->setFont(editorDesign::numericFont()); batchSeeds_->setProperty("numeric",true);
    label->setBuddy(batchSeeds_); bar->addWidget(label); bar->addWidget(batchSeeds_);
    bar->addAction(action("editorRunSeeds",{},[this]{startBatch();}));
    buildCompare(bar,layout); // M5.8b: Compare with... sits beside Run seeds and shares Cancel
    bar->addAction(action("editorCancelSeeds",{},[this]{cancelBatch(); refreshRun();}));
    // A different list is a different batch; the finished one no longer answers what it says.
    connect(batchSeeds_,&QLineEdit::textChanged,this,[this]{ if(batchShown()){cancelBatch(); refreshRun();} });

    batchView_=new QWidget(layout->parentWidget()); layout->addWidget(batchView_,1);
    auto* column=new QVBoxLayout(batchView_); column->setContentsMargins(0,0,0,0); column->setSpacing(editorDesign::space1);
    batchNote_=new QLabel(batchView_); batchNote_->setObjectName("editorBatchNote"); batchNote_->setWordWrap(true);
    column->addWidget(batchNote_);
    batchMovementTable_=table(batchView_,"editorBatchMovementTable",7); column->addWidget(batchMovementTable_,3);
    batchSectionTable_=table(batchView_,"editorBatchSectionTable",7); column->addWidget(batchSectionTable_,2);
    batchQueueTable_=table(batchView_,"editorBatchQueueTable",5); column->addWidget(batchQueueTable_,2);
    batchView_->hide();
}
void EditorWindow::translateBatch() {
    if(!batchMovementTable_) return;
    batchMovementTable_->setHorizontalHeaderLabels({text("editorResultsMovement"),text("editorResultsN"),text("editorResultsDelay"),
        text("editorResultsCi"),text("editorResultsVehicles"),text("editorResultsTravel"),text("editorResultsUnfinished")});
    batchSectionTable_->setHorizontalHeaderLabels({text("editorResultsSection"),text("editorResultsN"),text("editorResultsDelay"),
        text("editorResultsCi"),text("editorResultsVehicles"),text("editorResultsControl"),text("editorResultsLos")});
    batchQueueTable_->setHorizontalHeaderLabels({text("editorResultsApproach"),text("editorResultsN"),text("editorResultsQueueMean"),
        text("editorResultsQueueCi"),text("editorResultsQueueMax")});
    translateCompare();
}
void EditorWindow::refreshBatch() {
    if(!batchView_) return;
    // One table at a time: the single run, a batch, or a comparison (M5.8b).
    const bool shown=(batchRunning()&&!batchComparing_)||batch_;
    batchView_->setVisible(shown);
    for(QWidget* single:{static_cast<QWidget*>(movementTable_),static_cast<QWidget*>(queueTable_),static_cast<QWidget*>(resultsNote_)})single->setVisible(!batchShown());
    refreshCompare();
    batchMovementTable_->setRowCount(0); batchSectionTable_->setRowCount(0); batchQueueTable_->setRowCount(0);
    batchSectionTable_->hide();
    if(batchRunning()){ if(!batchComparing_) batchNote_->setText(text("editorBatchProgress").arg(batchDone_).arg(batchTotal_)); return; }
    if(!batch_) return;
    const auto& r=batch_->report;
    const auto& first=batch_->runs.front().report;
    QString note=text("editorBatchNote").arg(r.seeds.size()).arg(seedText(r.seeds))
        .arg(first.warmup,0,'f',0).arg(first.evaluationEnd,0,'f',0);
    if(first.cooldown) note+=" "+text("editorCooldownNote").arg(*first.cooldown,0,'f',0); // M5.9
    if(!r.overloadedSeeds.empty()) note+=" "+text("editorBatchOverloaded").arg(seedText(r.overloadedSeeds));
    if(const auto stuck=movementsWithUnfinished(r);!stuck.empty()){
        QStringList names; for(const auto& n:stuck) names<<QString::fromStdString(n);
        note+=" "+text("editorBatchUnfinished").arg(names.join(", "));
    }
    if(r.los && !r.sections.empty()) note+=" "+text("editorBatchLos").arg(QString::fromStdString(r.los->id));
    batchNote_->setText(note);
    const auto rows=[&](QTableWidget* table, const auto& source, auto fill){
        table->setRowCount(static_cast<int>(source.size()));
        for(int i=0;i<table->rowCount();++i) fill(i,source[static_cast<std::size_t>(i)]);
    };
    rows(batchMovementTable_,r.movements,[&](int i,const BatchMovementRow& m){
        batchMovementTable_->setItem(i,0,cell(QString::fromStdString(m.name),false));
        batchMovementTable_->setItem(i,1,count(m.meanDelay.n));
        batchMovementTable_->setItem(i,2,number(m.meanDelay.mean));
        batchMovementTable_->setItem(i,3,number(m.meanDelay.halfWidth95));
        batchMovementTable_->setItem(i,4,number(m.vehicles.mean));
        batchMovementTable_->setItem(i,5,number(m.meanTravelTime.mean));
        batchMovementTable_->setItem(i,6,number(m.unfinished.mean));
    });
    rows(batchSectionTable_,r.sections,[&](int i,const BatchSectionRow& m){
        batchSectionTable_->setItem(i,0,cell(QString::fromStdString(m.name),false));
        batchSectionTable_->setItem(i,1,count(m.meanDelay.n));
        batchSectionTable_->setItem(i,2,number(m.meanDelay.mean));
        batchSectionTable_->setItem(i,3,number(m.meanDelay.halfWidth95));
        batchSectionTable_->setItem(i,4,number(m.vehicles.mean));
        batchSectionTable_->setItem(i,5,cell(QString::fromStdString(m.controlType.value_or("")),false));
        batchSectionTable_->setItem(i,6,cell(QString::fromStdString(losCell(m.meanDelay.mean,m.controlType,r.los)),false));
    });
    batchSectionTable_->setVisible(!r.sections.empty());
    rows(batchQueueTable_,r.queues,[&](int i,const BatchQueueRow& q){
        batchQueueTable_->setItem(i,0,cell(QString::fromStdString(q.name),false));
        batchQueueTable_->setItem(i,1,count(q.meanLength.n));
        batchQueueTable_->setItem(i,2,number(q.meanLength.mean));
        batchQueueTable_->setItem(i,3,number(q.meanLength.halfWidth95));
        batchQueueTable_->setItem(i,4,number(q.maxLength.mean));
    });
}
std::optional<std::vector<std::uint32_t>> EditorWindow::batchSeedList() {
    try{ return parseSeedList(batchSeeds_->text().toStdString()); }
    catch(const std::exception& e){ error_->setText(text("invalidSeeds")+" ("+QString::fromUtf8(e.what())+")"); return std::nullopt; }
}
void EditorWindow::startBatch() {
    auto list=batchSeedList(); if(!list) return;
    auto seeds=std::move(*list);
    clearRun(); // one table at a time: the batch replaces the single run, and this cancels any batch
    std::optional<RunSnapshot> snapshot; std::optional<EvaluationSpec> spec;
    try{
        snapshot=compileDocument(history_.document(),data_);
        spec=evaluationSpec(history_.document(),*snapshot,data_);
    }catch(const std::exception& e){
        diagnostics_=runDiagnostics(history_.document(),data_);diagnosticRevision_=history_.revision();
        showError(e);objects_->setCurrentIndex(3);return;
    }
    error_->clear();
    const auto total=seeds.size(); // read before the call: the capture below moves `seeds`
    startWorker(total,false,[this,scenario=std::move(snapshot->scenario),spec=std::move(*spec),seeds=std::move(seeds)](const auto& progress)
        -> std::function<void()> {
        auto runs=runSeeds(scenario,spec,seeds,progress);
        if(runs.empty()) return {}; // cancelled
        auto report=aggregate(runs);
        auto result=std::make_shared<BatchResult>(BatchResult{std::move(report),std::move(runs)});
        return [this,result]{ batch_=std::move(*result); };
    });
}
void EditorWindow::startWorker(std::size_t total, bool comparing, BatchJob job) {
    // The previous worker was told to stop by clearRun(); it returns after the seed it is on.
    if(batchThread_.joinable()) batchThread_.join();
    auto cancel=std::make_shared<std::atomic<bool>>(false); batchCancel_=cancel;
    const auto generation=++batchGeneration_;
    batchDone_=0; batchTotal_=total; batchComparing_=comparing;
    batchThread_=std::thread([this,cancel,generation,job=std::move(job)]{
        std::function<void()> install; std::string error;
        try{
            install=job([&](std::size_t done){
                if(*cancel) return false;
                QMetaObject::invokeMethod(this,[this,generation,done]{
                    if(generation!=batchGeneration_) return;
                    batchDone_=done; refreshBatch();
                },Qt::QueuedConnection);
                return true;
            });
        }catch(const std::exception& e){ error=e.what(); }
        QMetaObject::invokeMethod(this,[this,generation,install,error]{ finishBatch(generation,install,error); },Qt::QueuedConnection);
    });
    objects_->setCurrentIndex(objects_->indexOf(resultsPage_)); resultsTabs_->setCurrentIndex(0); // the Movements tab holds the batch
    refreshRun();
}
void EditorWindow::finishBatch(std::uint64_t generation, const std::function<void()>& install, const std::string& error) {
    if(generation!=batchGeneration_) return; // cancelled or replaced: whatever it carries is not shown
    if(batchThread_.joinable()) batchThread_.join(); // it posted this as its last act
    batchTotal_=0; batchDone_=0;
    if(!error.empty()) showError(std::runtime_error(error));
    else if(install) install();
    refreshRun();
}
void EditorWindow::cancelBatch() {
    if(batchCancel_) *batchCancel_=true;
    ++batchGeneration_;
    batchTotal_=0; batchDone_=0; batch_.reset(); comparison_.reset();
    refreshBatch();
}
}
