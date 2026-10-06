#include "editor_window.hpp"
#include "demand_period_editor.hpp"
#include "demand_type_rules_dialog.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QVBoxLayout>
#include <cmath>
#include <stdexcept>
#include <QSignalBlocker>

namespace trafficsim {
// M2.4: a static routing decision -- turning proportions as relative flows over routes that all
// leave the same Link. One row per authored route; a flow of zero leaves the route out, and the
// one-origin rule is checked on commit, where the error names the route that breaks it.
// M2.1.1: the decision can be PLACED on a Link, as in Vissim. Then every routeless vehicle
// reaching that Link takes it, and a row may be a destination Link instead of a route -- counted
// turning volumes typed straight into the destinations, with no route drawn at all.
void EditorWindow::editDecision(const std::string& id) {
    RoutingDecision value{id,{},{},{}};
    const auto& def=history_.document().definition;
    const auto& network=history_.document().network;
    if(def)for(const auto& x:def->routingDecisions)if(x.id==id)value=x;
    QDialog dialog(this);dialog.setObjectName("editorDecisionDialog");dialog.setWindowTitle(text("editorEditDecision"));
    auto* layout=new QVBoxLayout(&dialog);
    layout->setContentsMargins(editorDesign::space3,editorDesign::space3,editorDesign::space3,editorDesign::space3);
    layout->setSpacing(editorDesign::space2);
    auto* name=new QLineEdit(QString::fromStdString(value.name),&dialog);name->setObjectName("editorDecisionNameField");
    name->setPlaceholderText(text("editorDecisionName"));layout->addWidget(name);
    auto* help=new QLabel(text("editorDecisionHelp"),&dialog);help->setWordWrap(true);layout->addWidget(help);
    auto* place=new QComboBox(&dialog);place->setObjectName("editorDecisionLink");
    place->addItem(text("editorDecisionUnplaced"),QString());
    for(const auto& l:network.links)
        place->addItem(QString::fromStdString(l.name.empty()?l.id:l.name),QString::fromStdString(l.id));
    if(const int at=place->findData(QString::fromStdString(value.linkId));at>=0)place->setCurrentIndex(at);
    auto* placeRow=new QFormLayout;placeRow->setContentsMargins(0,0,0,0);
    placeRow->setHorizontalSpacing(editorDesign::space2);placeRow->setVerticalSpacing(editorDesign::space1);
    placeRow->addRow(text("editorDecisionLinkLabel"),place);layout->addLayout(placeRow);
    auto* positioned=new QCheckBox(text("editorDecisionPositioned"),&dialog);positioned->setObjectName("editorDecisionPositioned");positioned->setChecked(value.position.has_value());
    auto* position=new QDoubleSpinBox(&dialog);position->setObjectName("editorDecisionPosition");position->setDecimals(3);position->setSuffix(" m");
    const auto syncPosition=[&]{
        double length=0;for(const auto& l:network.links)if(l.id==place->currentData().toString().toStdString())length=polylineLength(l.geometry);
        positioned->setEnabled(length>0);position->setRange(0,std::max(0.,length-1e-6));position->setEnabled(length>0 && positioned->isChecked());
    };
    connect(place,&QComboBox::currentIndexChanged,&dialog,[&]{syncPosition();});
    connect(positioned,&QCheckBox::toggled,&dialog,[&]{syncPosition();});syncPosition();position->setValue(value.position.value_or(0));
    placeRow->addRow(positioned);placeRow->addRow(text("editorDecisionPositionLabel"),position);
    // M2.1.2: counted turning volumes per interval, pasted per row as they come off a count sheet,
    // over intervals of one length from a start time -- the M2.2 input dialog's convention.
    auto* start=new QDoubleSpinBox(&dialog);start->setObjectName("editorDecisionIntervalStart");
    start->setRange(0,1e7);start->setDecimals(1);start->setSuffix(" s");
    auto* minutes=new QDoubleSpinBox(&dialog);minutes->setObjectName("editorDecisionIntervalMinutes");
    minutes->setRange(1,1440);minutes->setDecimals(2);minutes->setValue(15);
    // Shown as counts only when the stored intervals are contiguous and of one length.
    bool regular=!value.intervals.empty();
    const double storedLength=regular?value.intervals.front().endTime-value.intervals.front().startTime:0;
    for(std::size_t k=0;k<value.intervals.size()&&regular;++k)
        regular=std::abs(value.intervals[k].endTime-value.intervals[k].startTime-storedLength)<1e-9 &&
                (k==0||std::abs(value.intervals[k].startTime-value.intervals[k-1].endTime)<1e-9);
    if(regular){start->setValue(value.intervals.front().startTime);minutes->setValue(storedLength/60);}
    placeRow->addRow(text("editorDecisionIntervalStart"),start);
    placeRow->addRow(text("editorDecisionIntervalMinutes"),minutes);
    auto* flows=new QTableWidget(0,3,&dialog);flows->setObjectName("editorDecisionFlows");
    flows->setHorizontalHeaderLabels({text("editorDecisionTarget"),text("editorDecisionFlow"),text("editorDecisionCounts")});
    flows->horizontalHeader()->setStretchLastSection(true);flows->verticalHeader()->hide();
    // One row per route, then one per Link as a destination. `link` is empty for a route row.
    struct Row { std::string route, link; QDoubleSpinBox* field; QLineEdit* counts; std::vector<double> stored; };
    std::vector<Row> rows;
    const auto addRow=[&](const QString& label,const std::string& route,const std::string& link,double flow,
                          const std::vector<double>& stored){
        const int n=flows->rowCount();flows->insertRow(n);
        auto* cell=new QTableWidgetItem(label);cell->setFlags(Qt::ItemIsEnabled);editorDesign::setNumericText(cell);flows->setItem(n,0,cell);
        auto* field=new QDoubleSpinBox(flows);field->setObjectName(QString("editorDecisionFlow%1").arg(n));
        field->setRange(0,1000000);field->setDecimals(3);field->setValue(flow);
        flows->setCellWidget(n,1,field);
        auto* counts=new QLineEdit(flows);counts->setObjectName(QString("editorDecisionCounts%1").arg(n));
        counts->setFont(editorDesign::numericFont());counts->setProperty("numeric",true);
        counts->setPlaceholderText(text("editorDecisionCountsPlaceholder"));
        if(regular&&!stored.empty()){
            QStringList items;for(double f:stored)items<<QString::number(f,'g',12);
            counts->setText(items.join(' '));
        }
        flows->setCellWidget(n,2,counts);rows.push_back({route,link,field,counts,stored});
    };
    if(def)for(const auto& r:def->routes) {
        double flow=0;std::vector<double> stored;
        for(const auto& entry:value.routes)if(entry.destinationLinkId.empty()&&entry.routeId==r.id){flow=entry.relativeFlow;stored=entry.intervalFlows;}
        addRow(QString::fromStdString(r.id),r.id,{},flow,stored);
    }
    for(const auto& l:network.links) {
        double flow=0;std::vector<double> stored;
        for(const auto& entry:value.routes)if(entry.destinationLinkId==l.id){flow=entry.relativeFlow;stored=entry.intervalFlows;}
        addRow(text("editorDecisionDestinationItem").arg(QString::fromStdString(l.name.empty()?l.id:l.name)),{},l.id,flow,stored);
    }
    // A destination is reached from where the decision sits, so destination rows need a Link,
    // and the Link it sits on is no destination of its own.
    const auto sync=[&]{
        const auto at=place->currentData().toString().toStdString();
        for(const auto& row:rows)if(!row.link.empty()){
            const bool on=!at.empty()&&row.link!=at;row.field->setEnabled(on);row.counts->setEnabled(on);
        }
    };
    connect(place,&QComboBox::currentIndexChanged,&dialog,[sync](int){sync();});sync();
    layout->addWidget(flows);
    bool countsDirty=false,flowsDirty=false;
    for(const auto& row:rows) {
        connect(row.counts,&QLineEdit::textChanged,&dialog,[&]{countsDirty=true;});
        connect(row.field,&QDoubleSpinBox::valueChanged,&dialog,[&]{flowsDirty=true;});
    }
    connect(start,&QDoubleSpinBox::valueChanged,&dialog,[&]{countsDirty=true;});
    connect(minutes,&QDoubleSpinBox::valueChanged,&dialog,[&]{countsDirty=true;});
    auto* irregularNote=new QLabel(text("editorDemandIrregular"),&dialog);irregularNote->setWordWrap(true);layout->addWidget(irregularNote);
    const auto protectIrregular=[&]{
        const bool irregular=!value.intervals.empty() && !regular;
        irregularNote->setVisible(irregular);start->setEnabled(!irregular);minutes->setEnabled(!irregular);
        for(const auto& row:rows)row.counts->setReadOnly(irregular);
    };protectIrregular();
    auto* typeRules=new QPushButton(text("editorTypeRules"),&dialog);typeRules->setObjectName("editorTypeRules");layout->addWidget(typeRules);
    auto* typeHelp=new QLabel(text("editorTypeRulesTargetsHelp"),&dialog);typeHelp->setWordWrap(true);layout->addWidget(typeHelp);
    const auto protectTypeTargets=[&]{
        const bool locked=!value.typeRules.empty();place->setEnabled(!locked);start->setEnabled(!locked && (value.intervals.empty() || regular));minutes->setEnabled(!locked && (value.intervals.empty() || regular));
        for(const auto& row:rows){row.field->setEnabled(!locked && (row.link.empty() || (!value.linkId.empty() && row.link!=value.linkId)));row.counts->setReadOnly(locked || (!value.intervals.empty()&&!regular));}
    };
    connect(typeRules,&QPushButton::clicked,&dialog,[&]{
        try {
            const auto catalog=resolveDemandCatalog(def.value_or(AuthoringDefinition{}),data_,false);
            if(editDemandTypeRules(&dialog,value,catalog.vehicleTypes,[this](const char* key){return text(key);}))protectTypeTargets();
        }catch(const std::exception& e){showError(e);}
    });protectTypeTargets();
    auto* periods=new QPushButton(text("editorDemandPeriods"),&dialog);periods->setObjectName("editorDecisionPeriods");
    periods->setEnabled(!value.routes.empty());layout->addWidget(periods);
    connect(periods,&QPushButton::clicked,&dialog,[&]{
        QStringList columns{text("editorInputStart"),text("editorInputEnd")};
        for(const auto& r:value.routes)columns<<QString::fromStdString(r.destinationLinkId.empty()?r.routeId:r.destinationLinkId);
        for(const auto& rule:value.typeRules)for(const auto& r:value.routes)
            columns<<QString::fromStdString(rule.vehicleTypeId+": "+(r.destinationLinkId.empty()?r.routeId:r.destinationLinkId));
        std::vector<std::vector<double>> table;
        for(std::size_t k=0;k<value.intervals.size();++k) {
            std::vector<double> row{value.intervals[k].startTime,value.intervals[k].endTime};
            for(const auto& r:value.routes)row.push_back(r.intervalFlows[k]);
            for(const auto& rule:value.typeRules)for(const auto& flows:rule.intervalFlows)row.push_back(flows[k]);
            table.push_back(std::move(row));
        }
        const auto edited=editDemandPeriods(&dialog,columns,table,[this](const char* key){return text(key);});
        if(!edited)return;
        value.intervals.clear();for(auto& r:value.routes)r.intervalFlows.clear();
        for(auto& rule:value.typeRules)for(auto& flows:rule.intervalFlows)flows.clear();
        for(const auto& cells:*edited) {
            value.intervals.push_back({cells[0],cells[1]});
            std::size_t col=2;for(auto& r:value.routes)r.intervalFlows.push_back(cells[col++]);
            for(auto& rule:value.typeRules)for(auto& flows:rule.intervalFlows)flows.push_back(cells[col++]);
        }
        regular=!value.intervals.empty();
        const double length=regular?value.intervals.front().endTime-value.intervals.front().startTime:0;
        for(std::size_t k=0;k<value.intervals.size() && regular;++k)
            regular=std::abs(value.intervals[k].endTime-value.intervals[k].startTime-length)<1e-9 &&
                (k==0 || std::abs(value.intervals[k].startTime-value.intervals[k-1].endTime)<1e-9);
        const QSignalBlocker a(start),b(minutes);
        if(regular){start->setValue(value.intervals.front().startTime);minutes->setValue(length/60);}
        for(auto& row:rows) {
            row.stored.clear();for(const auto& r:value.routes)if(r.routeId==row.route && r.destinationLinkId==row.link)row.stored=r.intervalFlows;
            const QSignalBlocker block(row.counts);QStringList items;
            if(regular)for(double f:row.stored)items<<QString::number(f,'g',12);
            row.counts->setText(items.join(' '));
        }
        countsDirty=false;protectIrregular();protectTypeTargets();
    });
    periods->setToolTip(text("editorDemandSaveCountsFirst"));
    const auto syncPeriods=[&]{typeRules->setEnabled(!value.routes.empty() && !countsDirty && !flowsDirty &&
        value.linkId==place->currentData().toString().toStdString());periods->setEnabled(!value.routes.empty() && !countsDirty && !flowsDirty &&
        value.linkId==place->currentData().toString().toStdString());};
    for(const auto& row:rows) {
        connect(row.counts,&QLineEdit::textChanged,&dialog,syncPeriods);
        connect(row.field,&QDoubleSpinBox::valueChanged,&dialog,syncPeriods);
    }
    connect(start,&QDoubleSpinBox::valueChanged,&dialog,syncPeriods);
    connect(minutes,&QDoubleSpinBox::valueChanged,&dialog,syncPeriods);
    connect(place,&QComboBox::currentIndexChanged,&dialog,syncPeriods);
    syncPeriods();
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    buttons->button(QDialogButtonBox::Ok)->setEnabled(!rows.empty());
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.resize(640,480);if(dialog.exec()!=QDialog::Accepted)return;
    const bool targetChanged=value.linkId!=place->currentData().toString().toStdString();
    value.name=name->text().trimmed().toStdString();
    value.linkId=place->currentData().toString().toStdString();
    value.position=positioned->isChecked() && !value.linkId.empty()?std::optional<double>{position->value()}:std::nullopt;
    const double from=start->value(),length=minutes->value()*60;
    std::vector<std::pair<const Row*,QStringList>> typed;
    for(const auto& row:rows)if(row.field->isEnabled())
        typed.push_back({&row,row.counts->text().split(QRegularExpression("[\\s,;]+"),Qt::SkipEmptyParts)});
    std::string created;if(execute("editorEditDecision",[&](auto& d){
        if(!countsDirty) {
            if(flowsDirty || targetChanged) {
                value.routes.clear();
                for(const auto& row:rows)if(row.field->isEnabled() && row.field->value()>0)
                    value.routes.push_back({row.route,row.field->value(),row.link,row.stored});
            }
            created=putRoutingDecision(d,value);return;
        }
        value.routes.clear();value.intervals.clear();
        std::size_t periods=0;
        for(const auto& [row,items]:typed)periods=std::max<std::size_t>(periods,items.size());
        if(periods>0)for(const auto& [row,items]:typed)
            if((row->field->value()>0 || !items.empty()) && static_cast<std::size_t>(items.size())!=periods)
                throw std::invalid_argument("EDIT_INCOMPLETE_COUNTS");
        for(std::size_t k=0;k<periods;++k)value.intervals.push_back({from+k*length,from+(k+1)*length});
        for(const auto& [row,items]:typed) {
            if(periods==0){ if(row->field->value()>0)value.routes.push_back({row->route,row->field->value(),row->link,{}}); continue; }
            // With counts, a row's whole-period flow is its total, and a row with none is empty.
            std::vector<double> counts(periods,0.0);double total=0;
            for(int k=0;k<items.size();++k){
                bool ok=false;const double c=items[k].toDouble(&ok);
                if(!ok||!(c>=0))throw std::invalid_argument("EDIT_INVALID_COUNTS");
                counts[static_cast<std::size_t>(k)]=c;total+=c;
            }
            if(total>0)value.routes.push_back({row->route,total,row->link,std::move(counts)});
        }
        created=putRoutingDecision(d,value);
    }))selectDemand(created);
}
}
