#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../core/simulation.hpp"
#include <map>
#include "editor_storage.hpp"
#include "../project/batch_output.hpp"
#include "../project/csv_format.hpp"
#include <QClipboard>
#include <QGuiApplication>
#include <QFileDialog>
#include <QToolBar>
#include <QHeaderView>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace trafficsim {
// M2.5: the per-movement table and the approach queues, beside what they are not. The note is
// never optional: these are one unvalidated run of a prototype car-following model (rule 4).
namespace {
QTableWidget* resultTable(QWidget* parent, const char* name, int columns) {
    auto* table=new QTableWidget(0,columns,parent); table->setObjectName(name);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    // The names take the room; each figure column is exactly as wide as its header needs.
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    return table;
}
// The Results page, which tells its owner when it becomes visible: a tab switch, the dock being
// shown again, or the window itself. refreshResults() skips work while it is hidden.
class ResultsPage : public QWidget {
public:
    ResultsPage(QWidget* parent, std::function<void()> shown) : QWidget(parent), shown_(std::move(shown)) {}
protected:
    void showEvent(QShowEvent* e) override { QWidget::showEvent(e); shown_(); }
private:
    std::function<void()> shown_;
};
QLabel* noteLabel(QWidget* parent,const char* name) {
    auto* label=new QLabel(parent); label->setObjectName(name); label->setWordWrap(true); return label;
}
QTableWidgetItem* figure(const std::optional<double>& value,int decimals=1) {
    auto* item=new QTableWidgetItem(value?QString::number(*value,'f',decimals):QString());
    editorDesign::setNumericText(item,true);
    item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
    return item;
}
}
void EditorWindow::buildResults() {
    auto* page=new ResultsPage(objects_,[this]{refreshResults();}); auto* layout=new QVBoxLayout(page);
    layout->setContentsMargins(editorDesign::space1,editorDesign::space1,editorDesign::space1,editorDesign::space1);
    layout->setSpacing(editorDesign::space1);
    // Export writes exactly what `trafficsim-cli --csv` writes, marker line first, and only for
    // a finished run: the CSV has no field saying its figures are partial.
    auto* bar=new QToolBar(page); layout->addWidget(bar);
    bar->addAction(action("editorExportResults",{},[this]{
        const auto file=QFileDialog::getSaveFileName(this,text("editorExportResults"),"results.csv",text("editorCsvFilter"));
        if(file.isEmpty())return;
        try{exportResults(file);}catch(const std::exception& e){showError(e);}
    }));
    bar->addAction(action("editorCopyResults",QKeySequence(),[this]{
        try{copyResults();}catch(const std::exception& e){showError(e);}
    }));
    // D144: one tab per kind of figure, each with its own statement of what it is not.
    resultsPage_=page; resultsTabs_=new QTabWidget(page); resultsTabs_->setObjectName("editorResultsTabs");
    layout->addWidget(resultsTabs_,1);
    const auto tab=[&](QLabel*& note,const char* noteName){
        auto* body=new QWidget(resultsTabs_); auto* box=new QVBoxLayout(body);
        box->setContentsMargins(0,editorDesign::space1,0,0); box->setSpacing(editorDesign::space1);
        note=noteLabel(body,noteName); box->addWidget(note); resultsTabs_->addTab(body,QString()); return box;
    };
    // Side by side, so the dock's height goes to rows: twelve movements beside four approaches.
    auto* movementsBox=tab(resultsNote_,"editorResultsNote");
    auto* row=new QHBoxLayout; movementsBox->addLayout(row,1);
    row->setContentsMargins(0,0,0,0);row->setSpacing(editorDesign::space1);
    movementTable_=resultTable(page,"editorMovementTable",4); row->addWidget(movementTable_,3);
    queueTable_=resultTable(page,"editorQueueTable",3); row->addWidget(queueTable_,2);
    dischargeTable_=resultTable(page,"editorDischargeTable",8); tab(dischargeNote_,"editorDischargeNote")->addWidget(dischargeTable_,1);
    clampTable_=resultTable(page,"editorClampTable",5); tab(clampNote_,"editorClampNote")->addWidget(clampTable_,1);
    // The text column takes the room: the reasons, and the route a clamped vehicle was on.
    for(auto [table,column]:{std::pair{dischargeTable_,7},std::pair{clampTable_,3}}){
        table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(column,QHeaderView::Stretch);
    }
    buildBatch(bar,movementsBox); // M5.6: the N-seed table takes the single run's place
    connect(resultsTabs_,&QTabWidget::currentChanged,this,[this]{refreshResults();});
    objects_->addTab(page,QString());
}
void EditorWindow::translateResults() {
    objects_->setTabText(8,text("editorResultsTable"));
    movementTable_->setHorizontalHeaderLabels({text("editorResultsMovement"),text("editorResultsVehicles"),
                                               text("editorResultsDelay"),text("editorResultsTravel")});
    queueTable_->setHorizontalHeaderLabels({text("editorResultsApproach"),text("editorResultsQueueMean"),
                                            text("editorResultsQueueMax")});
    translateBatch();
    resultsTabs_->setTabText(0,text("editorResultsTabMovements"));
    resultsTabs_->setTabText(1,text("editorResultsTabDischarge"));
    resultsTabs_->setTabText(2,text("editorResultsTabClamps"));
    dischargeTable_->setHorizontalHeaderLabels({text("editorDischargeHead"),text("editorDischargeLane"),
        text("editorDischargeGreens"),text("editorDischargeEstimates"),text("editorDischargeHeadway"),
        text("editorDischargeRate"),text("editorDischargeStartup"),text("editorDischargeReasons")});
    clampTable_->setHorizontalHeaderLabels({text("editorClampTime"),text("editorClampVehicle"),
        text("editorClampType"),text("editorClampRoute"),text("editorClampSegment")});
    refreshResults();
}
void EditorWindow::refreshResults() {
    // Rebuilt every Step and Play frame, so skip it while the tab is hidden; the page's Show
    // event refreshes it the moment it can be seen. Measured 2026-10-02 (Windows, Release, M2.6,
    // 3,000 Steps): -11% of the Run view's time per Step, -28% of the action.
    if(!resultsPage_ || !resultsPage_->isVisible())return;
    // Only the inner tab on show is rebuilt; switching tabs refreshes the new one.
    refreshDischarge(); refreshClamps();
    refreshBatch(); // M5.6: a running or finished batch takes the single run's place
    if(batchRunning()||batch_.has_value())return;
    if(!movementTable_->isVisible())return;
    const auto report=runReport();
    if(!report){
        movementTable_->setRowCount(0); queueTable_->setRowCount(0);
        resultsNote_->setText(text("editorResultsEmpty")); return;
    }
    movementTable_->setRowCount(static_cast<int>(report->movements.size()));
    for(int r=0;r<movementTable_->rowCount();++r){
        const auto& m=report->movements[static_cast<std::size_t>(r)];
        movementTable_->setItem(r,0,new QTableWidgetItem(QString::fromStdString(m.name)));
        auto* count=new QTableWidgetItem(QString::number(m.vehicles));
        editorDesign::setNumericText(count,true);
        count->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter); movementTable_->setItem(r,1,count);
        movementTable_->setItem(r,2,figure(m.meanDelay));
        movementTable_->setItem(r,3,figure(m.meanTravelTime));
    }
    queueTable_->setRowCount(static_cast<int>(report->queues.size()));
    for(int r=0;r<queueTable_->rowCount();++r){
        const auto& q=report->queues[static_cast<std::size_t>(r)];
        queueTable_->setItem(r,0,new QTableWidgetItem(QString::fromStdString(q.name)));
        queueTable_->setItem(r,1,figure(q.meanLength));
        queueTable_->setItem(r,2,figure(q.maxLength));
    }
    const bool finished=runFinished();
    QString note=text("editorResultsNote").arg(report->active).arg(report->pending)
        .arg(report->unassigned).arg(report->safetyClamps);
    if(!finished) note=text("editorResultsPartial").arg(report->time,0,'f',1)+" "+note;
    resultsNote_->setText(note);
}
// Per head and lane: every green so far, how many gave an estimate, the means over those, and
// why the others did not (the observer's own reason codes, DISCHARGE.md: shown, never hidden).
void EditorWindow::refreshDischarge() {
    if(!dischargeTable_->isVisible())return;
    dischargeTable_->setRowCount(0);
    if(!runDischarge_){
        dischargeNote_->setText(runDischargeError_.empty()?text("editorResultsEmpty")
                                 :text("editorDischargeUnavailable").arg(text(runDischargeError_.c_str())));
        return;
    }
    struct Head { std::string head,lane; int greens{},estimates{},startups{}; double headway{},rate{},startup{};
                  std::map<std::string,int> reasons; };
    std::vector<Head> heads;
    DischargeSpec spec; spec.windowEnd=runState_.scenario->duration;
    for(const auto& cycle:runDischarge_->report()){
        auto at=std::find_if(heads.begin(),heads.end(),[&](const auto& h){return h.head==cycle.headId&&h.lane==cycle.laneId;});
        if(at==heads.end()){heads.push_back({cycle.headId,cycle.laneId});at=heads.end()-1;}
        const auto e=estimateDischarge(cycle,spec); ++at->greens;
        if(e.meanHeadway){++at->estimates;at->headway+=*e.meanHeadway;at->rate+=e.dischargeVehiclesPerHour.value_or(0);}
        else ++at->reasons[e.reason.empty()?"unavailable":e.reason];
        if(e.startupLostTime){++at->startups;at->startup+=*e.startupLostTime;}
    }
    dischargeTable_->setRowCount(static_cast<int>(heads.size()));
    for(int r=0;r<dischargeTable_->rowCount();++r){
        const auto& h=heads[static_cast<std::size_t>(r)];
        const auto mean=[](double sum,int n){return n?std::optional(sum/n):std::nullopt;};
        dischargeTable_->setItem(r,0,new QTableWidgetItem(QString::fromStdString(h.head)));
        dischargeTable_->setItem(r,1,new QTableWidgetItem(QString::fromStdString(h.lane)));
        dischargeTable_->setItem(r,2,figure(double(h.greens),0)); dischargeTable_->setItem(r,3,figure(double(h.estimates),0));
        dischargeTable_->setItem(r,4,figure(mean(h.headway,h.estimates),2));
        dischargeTable_->setItem(r,5,figure(mean(h.rate,h.estimates),0));
        dischargeTable_->setItem(r,6,figure(mean(h.startup,h.startups),2));
        QStringList reasons; for(const auto& [reason,n]:h.reasons) reasons<<QString::fromStdString(reason)+" "+QString::number(n);
        dischargeTable_->setItem(r,7,new QTableWidgetItem(reasons.join(", ")));
    }
    dischargeNote_->setText(text("editorDischargeNote"));
}
void EditorWindow::refreshClamps() {
    if(!clampTable_->isVisible())return;
    clampTable_->setRowCount(static_cast<int>(runClamps_.size()));
    for(int r=0;r<clampTable_->rowCount();++r){
        const auto& c=runClamps_[static_cast<std::size_t>(r)];
        clampTable_->setItem(r,0,figure(c.time,1)); clampTable_->setItem(r,1,figure(double(c.vehicleId),0));
        clampTable_->setItem(r,2,new QTableWidgetItem(QString::fromStdString(c.typeId)));
        clampTable_->setItem(r,3,new QTableWidgetItem(QString::fromStdString(c.routeId)));
        clampTable_->setItem(r,4,new QTableWidgetItem(QString::fromStdString(c.segmentId)));
    }
    clampNote_->setText(text("editorClampNote").arg(runClamps_.size()));
}
bool EditorWindow::runFinished() const {
    return runState_.scenario && runState_.tick>=totalTicks(*runState_.scenario);
}
std::string EditorWindow::resultsCsv() const {
    if(batch_)return batchCsv(batch_->report,batch_->runs);
    const auto report=runReport();
    if(!report || !runFinished())throw std::runtime_error("EDIT_CSV_UNFINISHED");
    return movementCsv(*report);
}
void EditorWindow::exportResults(const QString& file) const {
    writeEditorBytes(file,resultsCsv(),"EDIT_CSV_WRITE");
}
void EditorWindow::copyResults() const {
    QGuiApplication::clipboard()->setText(QString::fromStdString(csvToTsv(resultsCsv())));
}
}
