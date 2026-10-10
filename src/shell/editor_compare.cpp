#include "editor_window.hpp"
#include "editor_batch_cells.hpp"
#include "editor_storage.hpp"
#include "../project/batch_output.hpp"
#include "../project/load.hpp"
#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QToolBar>
#include <QVBoxLayout>

namespace trafficsim {
// M5.8b (BATCH §7, D149): the editor's `--compare`. The open project is the base and a chosen
// file the alternative, both compiled here on the UI thread; the batch worker runs the base's
// seeds, then the alternative's, and calls the CLI's aggregate and compareBatches. Export and Copy
// format with the CLI's comparisonCsv. A difference gets no LOS letter and keeps the
// not-validated marker (rule 4).
using namespace batchCells;
namespace {
// The CSV names each side by the file whose bytes produced it (BATCH §7). These are file-name
// annotations in the English CSV, as the CLI writes them, not UI text.
std::string baseName(const QString& file, bool dirty) {
    if(file.isEmpty()) return "unsaved project";
    const auto name=QFileInfo(file).fileName().toStdString();
    return dirty?name+" (unsaved edits)":name;
}
}
void EditorWindow::buildCompare(QToolBar* bar, QVBoxLayout* layout) {
    bar->addAction(action("editorCompareSeeds",{},[this]{
        if(!batchSeedList()) return; // a bad list is refused before the file is asked for
        const auto file=QFileDialog::getOpenFileName(this,text("editorCompareSeeds"),
            file_.isEmpty()?QString():QFileInfo(file_).absolutePath(),text("editorFilter"));
        if(!file.isEmpty()) compareWithFile(file);
    }));
    compareView_=new QWidget(layout->parentWidget()); layout->addWidget(compareView_,1);
    auto* column=new QVBoxLayout(compareView_); column->setContentsMargins(0,0,0,0); column->setSpacing(editorDesign::space1);
    compareNote_=new QLabel(compareView_); compareNote_->setObjectName("editorCompareNote"); compareNote_->setWordWrap(true);
    column->addWidget(compareNote_);
    compareMovementTable_=table(compareView_,"editorCompareMovementTable",7); column->addWidget(compareMovementTable_,3);
    compareSectionTable_=table(compareView_,"editorCompareSectionTable",7); column->addWidget(compareSectionTable_,2);
    compareQueueTable_=table(compareView_,"editorCompareQueueTable",7); column->addWidget(compareQueueTable_,2);
    compareView_->hide();
}
void EditorWindow::translateCompare() {
    if(!compareMovementTable_) return;
    const QStringList delay{text("editorCompareBaseN"),text("editorCompareBaseDelay"),text("editorCompareAlternativeN"),
        text("editorCompareAlternativeDelay"),text("editorCompareDifferenceDelay"),text("editorResultsCi")};
    compareMovementTable_->setHorizontalHeaderLabels(QStringList{text("editorResultsMovement")}+delay);
    compareSectionTable_->setHorizontalHeaderLabels(QStringList{text("editorResultsSection")}+delay);
    compareQueueTable_->setHorizontalHeaderLabels({text("editorResultsApproach"),text("editorCompareBaseN"),
        text("editorCompareBaseQueue"),text("editorCompareAlternativeN"),text("editorCompareAlternativeQueue"),
        text("editorCompareDifferenceQueue"),text("editorResultsQueueCi")});
}
void EditorWindow::refreshCompare() {
    if(!compareView_) return;
    const bool running=batchRunning()&&batchComparing_;
    compareView_->setVisible(running||comparison_);
    for(auto* t:{compareMovementTable_,compareSectionTable_,compareQueueTable_}) t->setRowCount(0);
    compareSectionTable_->hide();
    if(running){ compareNote_->setText(text("editorCompareProgress").arg(batchDone_).arg(batchTotal_)); return; }
    if(!comparison_) return;
    const auto& c=comparison_->comparison;
    const auto& base=comparison_->base; const auto& alternative=comparison_->alternative;
    const auto& first=base.runs.front().report;
    const auto figure=[&](const std::optional<double>& v){ return v?QString::number(*v,'f',1):text("empty"); };
    // Every argument in one arg() call, so a file name holding "%1" is never substituted again.
    QString note=text("editorCompareNote").arg(QString::number(c.seeds.size()),seedText(c.seeds),
        QString::fromStdString(base.name),QString::fromStdString(alternative.name),
        QString::number(first.warmup,'f',0),QString::number(first.evaluationEnd,'f',0));
    if(first.cooldown) note+=" "+text("editorCooldownNote").arg(*first.cooldown,0,'f',0); // M5.9
    note+=" "+text("editorCompareNetwork").arg(figure(c.network.base),figure(c.network.alternative),
        figure(c.network.difference),figure(c.network.halfWidth95));
    // "base 43; alternative 42" for the sides that have any (BATCH §3, §5, per side).
    const auto perSide=[&](const QStringList& b,const QStringList& a){
        QStringList out;
        if(!b.isEmpty()) out<<text("editorCompareBase")+" "+b.join(", ");
        if(!a.isEmpty()) out<<text("editorCompareAlternative")+" "+a.join(", ");
        return out.join("; ");
    };
    const auto seeds=[](const BatchReport& r){ QStringList out; for(const auto s:r.overloadedSeeds) out<<QString::number(s); return out; };
    const auto stuck=[](const BatchReport& r){ QStringList out; for(const auto& n:movementsWithUnfinished(r)) out<<QString::fromStdString(n); return out; };
    if(const auto s=perSide(seeds(base.report),seeds(alternative.report));!s.isEmpty()) note+=" "+text("editorBatchOverloaded").arg(s);
    if(const auto s=perSide(stuck(base.report),stuck(alternative.report));!s.isEmpty()) note+=" "+text("editorBatchUnfinished").arg(s);
    QStringList baseOnly,alternativeOnly,ambiguous;
    for(const auto* u:{&c.unmatchedMovements,&c.unmatchedSections,&c.unmatchedQueues}){
        for(const auto& n:u->baseOnly) baseOnly<<QString::fromStdString(n);
        for(const auto& n:u->alternativeOnly) alternativeOnly<<QString::fromStdString(n);
        for(const auto& n:u->ambiguous) ambiguous<<QString::fromStdString(n);
    }
    QStringList unmatched;
    if(!baseOnly.isEmpty()) unmatched<<text("editorCompareBaseOnly").arg(baseOnly.join(", "));
    if(!alternativeOnly.isEmpty()) unmatched<<text("editorCompareAlternativeOnly").arg(alternativeOnly.join(", "));
    if(!ambiguous.isEmpty()) unmatched<<text("editorCompareAmbiguous").arg(ambiguous.join(", "));
    if(!unmatched.isEmpty()) note+=" "+text("editorCompareUnmatched").arg(unmatched.join("; "));
    compareNote_->setText(note);
    const auto fill=[](QTableWidget* t,const std::vector<ComparisonRow>& rows){
        t->setRowCount(static_cast<int>(rows.size()));
        for(int i=0;i<t->rowCount();++i){
            const auto& r=rows[static_cast<std::size_t>(i)]; const auto& d=r.value;
            t->setItem(i,0,cell(QString::fromStdString(r.name),false));
            t->setItem(i,1,count(d.nBase)); t->setItem(i,2,number(d.base));
            t->setItem(i,3,count(d.nAlternative)); t->setItem(i,4,number(d.alternative));
            t->setItem(i,5,number(d.difference)); t->setItem(i,6,number(d.halfWidth95));
        }
    };
    fill(compareMovementTable_,c.movements);
    fill(compareSectionTable_,c.sections); compareSectionTable_->setVisible(!c.sections.empty());
    fill(compareQueueTable_,c.queues);
}
void EditorWindow::compareWithFile(const QString& file) {
    auto list=batchSeedList(); if(!list) return;
    // Everything is compiled and checked before the results on show are cleared (BATCH §7).
    std::optional<RunSnapshot> base; std::optional<EvaluationSpec> baseSpec;
    try{
        base=compileDocument(history_.document(),data_);
        baseSpec=evaluationSpec(history_.document(),*base,data_);
    }catch(const std::exception& e){
        diagnostics_=runDiagnostics(history_.document(),data_);diagnosticRevision_=history_.revision();
        showError(e);objects_->setCurrentIndex(3);return;
    }
    const auto alternativeName=QFileInfo(file).fileName();
    std::optional<RunSnapshot> alternative; std::optional<EvaluationSpec> alternativeSpec;
    try{
        const auto document=readEditorDocument(file);
        alternative=compileDocument(document,data_);
        alternativeSpec=evaluationSpec(document,*alternative,data_);
    }catch(const std::exception& e){
        // Its issues are the other network's: one line, never the open project's Problems tab.
        const auto reason=text(e.what());
        error_->setText(text("editorCompareAlternativeInvalid").arg(alternativeName,reason.isEmpty()?QString::fromUtf8(e.what()):reason));
        return;
    }
    try{ requireSameEvaluationPeriod(*baseSpec,base->scenario.duration,*alternativeSpec,alternative->scenario.duration); }
    catch(const std::invalid_argument&){ error_->setText(text("editorCompareWindows")); return; }
    auto names=std::pair{baseName(file_,history_.dirty()),alternativeName.toStdString()};
    clearRun(); // one table at a time: this cancels any batch or comparison and the single run
    error_->clear();
    const auto total=2*list->size(); // read before the call: the capture below moves the list
    startWorker(total,true,[this,names=std::move(names),seeds=std::move(*list),
        baseScenario=std::move(base->scenario),baseSpec=std::move(*baseSpec),
        alternativeScenario=std::move(alternative->scenario),alternativeSpec=std::move(*alternativeSpec)](const auto& progress)
        -> std::function<void()> {
        auto baseRuns=runSeeds(baseScenario,baseSpec,seeds,progress);
        if(baseRuns.empty()) return {}; // cancelled
        auto alternativeRuns=runSeeds(alternativeScenario,alternativeSpec,seeds,
            [&](std::size_t done){ return progress(seeds.size()+done); });
        if(alternativeRuns.empty()) return {};
        auto result=std::make_shared<ComparisonResult>();
        result->base.name=names.first; result->base.report=aggregate(baseRuns); result->base.runs=std::move(baseRuns);
        result->alternative.name=names.second; result->alternative.report=aggregate(alternativeRuns);
        result->alternative.runs=std::move(alternativeRuns);
        result->comparison=compareBatches(result->base.report,result->alternative.report);
        return [this,result]{ comparison_=std::move(*result); };
    });
}
}
