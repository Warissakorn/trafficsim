#include "editor_window.hpp"
#include "result_table.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/load.hpp"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <atomic>
#include <limits>

namespace trafficsim {
// M5.3 (D138): "Run N seeds". The document is compiled once on this thread, exactly as Run
// compiles it; the worker gets its own copy of the scenario and evaluation spec and runs the
// seeds one by one through the same runSeeds/aggregate as `trafficsim-cli --seeds`, so it can
// stop between seeds. It touches nothing of the window: results come back as queued calls,
// and only the job still held in batchJob_ may publish one.
struct EditorWindow::BatchJob {
    Scenario scenario;
    EvaluationSpec spec;
    std::vector<std::uint32_t> seeds;
    std::atomic<bool> cancel{false}, finished{false};
};
using results::figure;
namespace {
QTableWidgetItem* count(double value) { return figure(value,0); }
}
void EditorWindow::buildBatch(QTabWidget* tabs) {
    auto* body=new QWidget(tabs); auto* box=new QVBoxLayout(body);
    box->setContentsMargins(0,editorDesign::space1,0,0); box->setSpacing(editorDesign::space1);
    auto* top=new QHBoxLayout; top->setContentsMargins(0,0,0,0); top->setSpacing(editorDesign::space1); box->addLayout(top);
    batchNote_=results::note(body,"editorBatchNote"); top->addWidget(batchNote_,1);
    batchCancel_=new QPushButton(body); batchCancel_->setObjectName("editorBatchCancel"); top->addWidget(batchCancel_,0,Qt::AlignTop);
    connect(batchCancel_,&QPushButton::clicked,this,[this]{clearBatch(true);});
    // Movements beside approaches, as on the single-run tab; the per-seed accounting shares the
    // approaches' place, so each table keeps the dock's full height.
    auto* row=new QHBoxLayout; row->setContentsMargins(0,0,0,0); row->setSpacing(editorDesign::space1); box->addLayout(row,1);
    batchMovementTable_=results::table(body,"editorBatchMovementTable",7); row->addWidget(batchMovementTable_,3);
    batchSide_=new QTabWidget(body); batchSide_->setObjectName("editorBatchSideTabs"); row->addWidget(batchSide_,2);
    batchQueueTable_=results::table(batchSide_,"editorBatchQueueTable",6); batchSide_->addTab(batchQueueTable_,QString());
    // One accounting row per seed: a seed that ends congested is shown, never averaged away.
    batchSeedTable_=results::table(batchSide_,"editorBatchSeedTable",7); batchSide_->addTab(batchSeedTable_,QString());
    batchSeedTable_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    batchSeedTable_->horizontalHeader()->setStretchLastSection(true);
    // The side's tab bar must not raise the dock's minimum height above the single-run tab's.
    for(auto* table:{batchQueueTable_,batchSeedTable_})
        table->setMinimumHeight(std::max(0,batchMovementTable_->minimumSizeHint().height()-batchSide_->tabBar()->sizeHint().height()));
    tabs->addTab(body,QString());
}
void EditorWindow::translateBatch() {
    const auto pm=text("editorBatchHalfWidth");
    batchMovementTable_->setHorizontalHeaderLabels({text("editorResultsMovement"),text("editorBatchN"),text("editorResultsVehicles"),
        text("editorResultsDelay"),pm,text("editorResultsTravel"),pm});
    batchQueueTable_->setHorizontalHeaderLabels({text("editorResultsApproach"),text("editorBatchN"),
        text("editorResultsQueueMean"),pm,text("editorResultsQueueMax"),pm});
    batchSeedTable_->setHorizontalHeaderLabels({text("editorBatchSeed"),text("editorBatchGenerated"),text("editorBatchCompleted"),
        text("editorBatchActive"),text("editorBatchPending"),text("editorBatchClamps"),text("editorResultsDelay")});
    batchCancel_->setText(text("editorCancel"));
    batchSide_->setTabText(0,text("editorBatchApproaches")); batchSide_->setTabText(1,text("editorBatchSeeds"));
}
void EditorWindow::refreshBatch() {
    if(const auto a=actions_.find("editorRunSeeds");a!=actions_.end())a->second->setEnabled(!batchJob_);
    batchCancel_->setVisible(batchJob_!=nullptr);
    if(!batchMovementTable_->isVisible())return;
    if(!batch_){
        for(auto* table:{batchMovementTable_,batchQueueTable_,batchSeedTable_})table->setRowCount(0);
        batchNote_->setText(batchJob_?text("editorBatchRunning").arg(batchDone_).arg(batchCount_)
                            :text(batchCancelled_?"editorBatchCancelled":"editorBatchEmpty"));
        return;
    }
    const auto& b=*batch_;
    batchMovementTable_->setRowCount(static_cast<int>(b.movements.size()));
    for(int r=0;r<batchMovementTable_->rowCount();++r){
        const auto& m=b.movements[static_cast<std::size_t>(r)];
        batchMovementTable_->setItem(r,0,new QTableWidgetItem(QString::fromStdString(m.name)));
        batchMovementTable_->setItem(r,1,count(double(m.meanDelay.n)));
        batchMovementTable_->setItem(r,2,figure(m.vehicles.mean));
        batchMovementTable_->setItem(r,3,figure(m.meanDelay.mean)); batchMovementTable_->setItem(r,4,figure(m.meanDelay.halfWidth95));
        batchMovementTable_->setItem(r,5,figure(m.meanTravelTime.mean)); batchMovementTable_->setItem(r,6,figure(m.meanTravelTime.halfWidth95));
    }
    batchQueueTable_->setRowCount(static_cast<int>(b.queues.size()));
    for(int r=0;r<batchQueueTable_->rowCount();++r){
        const auto& q=b.queues[static_cast<std::size_t>(r)];
        batchQueueTable_->setItem(r,0,new QTableWidgetItem(QString::fromStdString(q.name)));
        batchQueueTable_->setItem(r,1,count(double(q.meanLength.n)));
        batchQueueTable_->setItem(r,2,figure(q.meanLength.mean)); batchQueueTable_->setItem(r,3,figure(q.meanLength.halfWidth95));
        batchQueueTable_->setItem(r,4,figure(q.maxLength.mean)); batchQueueTable_->setItem(r,5,figure(q.maxLength.halfWidth95));
    }
    batchSeedTable_->setRowCount(static_cast<int>(b.runs.size()));
    for(int r=0;r<batchSeedTable_->rowCount();++r){
        const auto& run=b.runs[static_cast<std::size_t>(r)]; const auto& x=run.report;
        batchSeedTable_->setItem(r,0,count(run.seed)); batchSeedTable_->setItem(r,1,count(double(run.generated)));
        batchSeedTable_->setItem(r,2,count(double(x.completed))); batchSeedTable_->setItem(r,3,count(double(x.active)));
        batchSeedTable_->setItem(r,4,count(double(x.pending))); batchSeedTable_->setItem(r,5,count(double(x.safetyClamps)));
        batchSeedTable_->setItem(r,6,figure(x.meanDelay,2));
    }
    const auto run=[&](const Estimate& e,int decimals){
        return e.mean?QString::number(*e.mean,'f',decimals)+(e.halfWidth95?" ± "+QString::number(*e.halfWidth95,'f',decimals):QString())
                     :text("empty");
    };
    batchNote_->setText(text("editorBatchNote").arg(b.runs.size()).arg(b.runs.front().seed).arg(b.runs.back().seed)
        .arg(run(b.meanDelay,2)).arg(run(b.completed,1)).arg(run(b.safetyClamps,1))+" "+windowNote(b.runs.front().report.window));
}
void EditorWindow::runBatchDialog() {
    QDialog dialog(this); dialog.setObjectName("editorBatchDialog"); dialog.setWindowTitle(text("editorRunSeeds"));
    auto* form=new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3,editorDesign::space3,editorDesign::space3,editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2); form->setVerticalSpacing(editorDesign::space1);
    // Starting values only, not model parameters: the Run seed and the ten seeds of M5's goal.
    auto* first=new QLineEdit(runSeed_->text(),&dialog); first->setObjectName("editorBatchFirst"); first->setMaxLength(10);
    auto* seeds=new QSpinBox(&dialog); seeds->setObjectName("editorBatchCount"); seeds->setRange(2,10000); seeds->setValue(10);
    form->addRow(text("editorBatchFirst"),first); form->addRow(text("editorBatchCount"),seeds);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorBatchStart")); buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    std::uint32_t seed{};
    try{seed=parseSeed(first->text().toStdString());}catch(const std::exception&){showError(std::invalid_argument("invalidSeed"));return;}
    startBatch(seed,static_cast<std::uint32_t>(seeds->value()));
}
bool EditorWindow::startBatch(std::uint32_t first, std::uint32_t count) {
    clearBatch();
    // The CLI's limits (--seeds): at least two seeds, at most 10,000, none past 2^32-1.
    if(count<2||count>10000||first>std::numeric_limits<std::uint32_t>::max()-(count-1)){
        showError(std::invalid_argument("editorBatchRange")); return false;
    }
    try {
        auto job=std::make_shared<BatchJob>();
        auto snapshot=compileDocument(history_.document(),data_);
        job->spec=evaluationSpec(history_.document(),snapshot,data_);
        job->scenario=std::move(snapshot.scenario);
        for(std::uint32_t i=0;i<count;++i)job->seeds.push_back(first+i);
        batchJob_=job; batchDone_=0; batchCount_=count; error_->clear();
        batchThreads_.emplace_back(job,std::thread([this,job]{
            std::vector<SeedRun> runs;
            try {
                for(const auto seed:job->seeds){
                    if(job->cancel)break;
                    runs.push_back(runSeeds(job->scenario,job->spec,{seed}).front());
                    QMetaObject::invokeMethod(this,[this,job,done=runs.size()]{
                        if(job==batchJob_){batchDone_=done; refreshBatch();}
                    },Qt::QueuedConnection);
                }
                if(!job->cancel){
                    auto report=std::make_shared<BatchReport>(aggregate(std::move(runs)));
                    QMetaObject::invokeMethod(this,[this,job,report]{
                        if(job==batchJob_){batch_=std::move(*report); batchJob_.reset(); refreshBatch();}
                    },Qt::QueuedConnection);
                }
            }catch(const std::exception& e){
                QMetaObject::invokeMethod(this,[this,job,code=std::string(e.what())]{
                    if(job==batchJob_){batchJob_.reset(); refreshBatch(); showError(std::runtime_error(code));}
                },Qt::QueuedConnection);
            }
            job->finished=true;
            QMetaObject::invokeMethod(this,[this]{joinBatchThreads(false);},Qt::QueuedConnection);
        }));
        objects_->setCurrentWidget(resultsPage_); resultsTabs_->setCurrentIndex(3);
        refreshBatch(); return true;
    }catch(const std::exception& e){
        diagnostics_=runDiagnostics(history_.document(),data_);diagnosticRevision_=history_.revision();
        showError(e); objects_->setCurrentIndex(3);
        refreshBatch(); return false;
    }
}
// Drops the running job (it stops after its current seed and can no longer publish) and the
// shown result. Never waits: the thread is joined once it says it has finished, or at exit.
void EditorWindow::clearBatch(bool cancelled) {
    if(batchJob_)batchJob_->cancel=true;
    batchCancelled_=cancelled&&batchJob_;
    batchJob_.reset(); batch_.reset(); batchDone_=batchCount_=0;
    joinBatchThreads(false);
    if(batchMovementTable_)refreshBatch();
}
void EditorWindow::joinBatchThreads(bool all) {
    for(auto it=batchThreads_.begin();it!=batchThreads_.end();){
        if(all)it->first->cancel=true;
        if(all||it->first->finished){it->second.join();it=batchThreads_.erase(it);}else ++it;
    }
}
}
