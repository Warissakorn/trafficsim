#include "editor_window.hpp"
#include "../project/counted_volumes.hpp"
#include <QCheckBox>
#include "demand_period_editor.hpp"
#include "../project/demand_paths.hpp"
#include "../project/demand_catalog.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

namespace trafficsim {
// The vehicle-input dialog, split from editor_demand.cpp when M2.2 gave it counted intervals.
void EditorWindow::editInput(const std::string& id,const std::string& preselectedRoute,const std::string& preselectedLink) {
    VehicleInput value{id,{},{},600,0,history_.document().definition?history_.document().definition->duration:180};
    if(history_.document().definition)for(const auto& i:history_.document().definition->inputs)if(i.id==id)value=i;
    DemandCatalog catalog;
    std::vector<Composition> compositions;
    try {
        catalog=resolveDemandCatalog(history_.document().definition.value_or(AuthoringDefinition{}),data_);
        compositions=catalog.compositions;
    }
    catch(const std::exception& e){showError(e);return;}
    QDialog dialog(this);dialog.setObjectName("editorInputDialog");dialog.setWindowTitle(text("editorEditInput"));
    auto* form=new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3,editorDesign::space3,editorDesign::space3,editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2);form->setVerticalSpacing(editorDesign::space1);
    auto* route=new QComboBox(&dialog);route->setObjectName("editorInputRoute");
    // Routes, then routing decisions (M2.4): an input follows one route, or is split across the
    // routes of a decision by its turning proportions. Item data says which.
    if(history_.document().definition)for(const auto& r:history_.document().definition->routes)route->addItem(QString::fromStdString(r.id),"route:"+QString::fromStdString(r.id));
    if(history_.document().definition)for(const auto& x:history_.document().definition->routingDecisions)
        route->addItem(text("editorInputDecisionItem").arg(QString::fromStdString(x.name.empty()?x.id:x.name)),
                       "decision:"+QString::fromStdString(x.id));
    // M2.1.1: no route at all -- the vehicles follow the network from a Link, as in Vissim.
    for(const auto& l:history_.document().network.links)
        route->addItem(text("editorInputLinkItem").arg(QString::fromStdString(l.name.empty()?l.id:l.name)),
                       "link:"+QString::fromStdString(l.id));
    // A route placed by pointer names the route it was dropped on; the dialog opens on it. A Link
    // with no route opens on following the network from that Link.
    if(value.routeId.empty()&&value.linkId.empty()&&value.routingDecisionId.empty()){value.routeId=preselectedRoute;value.linkId=preselectedLink;}
    {
        const auto current=!value.linkId.empty()?"link:"+QString::fromStdString(value.linkId)
            :value.routingDecisionId.empty()?"route:"+QString::fromStdString(value.routeId)
                                            :"decision:"+QString::fromStdString(value.routingDecisionId);
        if(const int at=route->findData(current);at>=0)route->setCurrentIndex(at);
    }
    form->addRow(text("editorInputRoute"),route);
    // D142: the volume is the entry decision's turning counts, typed once in the decision. Offered
    // only where there is one to read: a decision named here, or one placed on the chosen Link.
    auto* fromCounts=new QCheckBox(text("editorInputFromCounts"),&dialog);fromCounts->setObjectName("editorInputFromCounts");
    fromCounts->setChecked(value.volumeFromCounts);form->addRow(fromCounts);
    auto* fromCountsTotal=new QLabel(&dialog);fromCountsTotal->setObjectName("editorInputFromCountsTotal");
    fromCountsTotal->setWordWrap(true);form->addRow(fromCountsTotal);
    const auto chosenTarget=[&]{
        auto chosen=value;const auto key=route->currentData().toString();
        const auto id=key.mid(key.indexOf(':')+1).toStdString();
        chosen.routeId=key.startsWith("route:")?id:std::string{};
        chosen.linkId=key.startsWith("link:")?id:std::string{};
        chosen.routingDecisionId=key.startsWith("decision:")?id:std::string{};
        return chosen;
    };
    const auto countedTarget=[&]()->std::optional<std::vector<VolumeInterval>>{
        const auto& definition=history_.document().definition;
        if(!definition)return std::nullopt;
        const auto* decision=countedDecision(*definition,chosenTarget());
        if(!decision||decision->position)return std::nullopt;
        return countedVolumes(*decision);
    };
    auto* type=new QComboBox(&dialog);type->setObjectName("editorInputType");
    // One list for both (M2.3): a vehicle type, or a composition of types from data/compositions/.
    // The item data says which, so an id shared by a type and a composition cannot be confused.
    for(const auto& t:catalog.vehicleTypes) {
        const auto at=catalog.vehicleTypeNames.find(t.id);
        const auto label=at==catalog.vehicleTypeNames.end()?t.id:at->second+" ("+t.id+")";
        type->addItem(QString::fromStdString(label),"type:"+QString::fromStdString(t.id));
    }
    for(const auto& c:compositions)
        type->addItem(text("editorInputCompositionItem").arg(QString::fromStdString(c.name.empty()?c.id:c.name+" ("+c.id+")")),
                      "composition:"+QString::fromStdString(c.id));
    const auto current=value.compositionId.empty()?"type:"+QString::fromStdString(value.vehicleTypeId)
                                                  :"composition:"+QString::fromStdString(value.compositionId);
    if(const int at=type->findData(current);at>=0)type->setCurrentIndex(at);
    form->addRow(text("editorInputType"),type);
    const auto number=[&](const char* key,double v){
        auto* field=new QDoubleSpinBox(&dialog);field->setObjectName(key);field->setRange(0,10000000);field->setDecimals(3);field->setValue(v);
        form->addRow(text(key),field);return field;
    };
    auto* volume=number("editorInputVolume",value.vehiclesPerHour);
    // Rule 4: say what the split is and is not. It divides the Link total equally (unless lane
    // shares are set) as an authoring convenience; the engine changes lanes only where a route
    // requires it, so the split is not a lane-choice model.
    auto* split=new QLabel(text("editorInputSplitHelp"),&dialog);split->setObjectName("editorInputSplitHelp");
    split->setWordWrap(true);form->addRow(split);
    // M1.26.1: one weight per lane the SELECTED route currently reaches. Rebuilt whenever the
    // route changes, because the lane count is the route's, not the input's. Seeded from a
    // stored laneShares only when its size still matches -- a stale one (the drawing changed
    // since) is exactly what buildScenario itself falls back from, so the dialog must not present
    // it as if it still applied. "Dirty" tracks whether the author touched a field since the
    // fields were last (re)built; leaving them alone keeps the input's default equal split,
    // which is what keeps an unedited input's compiled volumes bit-identical (D32).
    auto* sharesGroup=new QWidget(&dialog);sharesGroup->setObjectName("editorInputShares");
    auto* sharesForm=new QFormLayout(sharesGroup);sharesForm->setContentsMargins(0,0,0,0);
    sharesForm->setHorizontalSpacing(editorDesign::space2);sharesForm->setVerticalSpacing(editorDesign::space1);
    form->addRow(sharesGroup);
    auto* sharesHelp=new QLabel(text("editorInputShareHelp"),&dialog);sharesHelp->setObjectName("editorInputShareHelp");
    sharesHelp->setWordWrap(true);form->addRow(sharesHelp);
    auto sharesDirty=std::make_shared<bool>(false);
    auto shareFields=std::make_shared<std::vector<QDoubleSpinBox*>>();
    const auto policy=[&]{
        auto chosen=value;const auto key=route->currentData().toString();
        const auto id=key.mid(key.indexOf(':')+1).toStdString();
        chosen.routeId=key.startsWith("route:")?id:std::string{};
        chosen.linkId=key.startsWith("link:")?id:std::string{};
        chosen.routingDecisionId=key.startsWith("decision:")?id:std::string{};
        return inputLanePolicy(history_.document().network,history_.document().definition.value_or(AuthoringDefinition{}),chosen);
    };
    const auto rebuildShares=[&,sharesDirty,shareFields]{
        while(sharesForm->count()>0){auto* item=sharesForm->takeAt(0);delete item->widget();delete item;}
        shareFields->clear();*sharesDirty=false;
        const auto rule=policy();const auto lanes=rule.lanes;
        sharesGroup->setVisible(lanes>1 && rule.acceptsShares);sharesHelp->setVisible(lanes>1 || !rule.acceptsShares);
        sharesHelp->setText(text(rule.acceptsShares?"editorInputShareHelp":"DEMAND_LANE_SHARES_IGNORED"));
        if(lanes<=1 || !rule.acceptsShares)return;
        std::vector<double> seed(lanes,1.0);
        if(value.laneShares.size()==lanes)seed=value.laneShares;
        for(std::size_t k=0;k<lanes;++k){
            auto* field=new QDoubleSpinBox(sharesGroup);
            field->setObjectName(QString("editorInputShare%1").arg(k));
            field->setRange(0,1000000);field->setDecimals(3);
            {const QSignalBlocker guard(field);field->setValue(seed[k]);}
            connect(field,&QDoubleSpinBox::valueChanged,&dialog,[sharesDirty](double){*sharesDirty=true;});
            sharesForm->addRow(text("editorInputShareLane").arg(static_cast<int>(k+1)),field);
            shareFields->push_back(field);
        }
    };
    connect(route,&QComboBox::currentTextChanged,&dialog,[rebuildShares](const QString&){rebuildShares();});
    rebuildShares();
    auto* start=number("editorInputStart",value.startTime);auto* end=number("editorInputEnd",value.endTime);
    // M2.2: counted volumes, pasted as they come off a count sheet -- one count per interval, in
    // vehicles, over intervals of a fixed length from the start time above. When given they are
    // the input's volume and the two fields above them are derived from them on save.
    auto* minutes=new QDoubleSpinBox(&dialog);minutes->setObjectName("editorInputIntervalMinutes");
    minutes->setRange(1,1440);minutes->setDecimals(2);minutes->setValue(15);
    form->addRow(text("editorInputIntervalMinutes"),minutes);
    auto* counts=new QPlainTextEdit(&dialog);counts->setObjectName("editorInputCounts");
    counts->setPlaceholderText(text("editorInputCountsPlaceholder"));counts->setMaximumHeight(110);
    form->addRow(text("editorInputCounts"),counts);
    auto* countsHelp=new QLabel(text("editorInputCountsHelp"),&dialog);countsHelp->setWordWrap(true);
    countsHelp->setObjectName("editorInputCountsHelp");form->addRow(countsHelp);
    bool irregular=false;
    // Shown as counts only when the stored intervals are what this box can write back --
    // contiguous and of one length. Anything else is left alone unless the author types here.
    if(!value.intervals.empty()) {
        const double length=value.intervals.front().endTime-value.intervals.front().startTime;
        bool regular=length>0;
        for(std::size_t k=0;k<value.intervals.size()&&regular;++k)
            regular=std::abs(value.intervals[k].endTime-value.intervals[k].startTime-length)<1e-9 &&
                    (k==0||std::abs(value.intervals[k].startTime-value.intervals[k-1].endTime)<1e-9);
        irregular=!regular;
        if(regular) {
            QStringList lines;
            for(const auto& p:value.intervals)lines<<QString::number(p.vehiclesPerHour*length/3600,'g',12);
            const QSignalBlocker a(minutes),b(counts);
            minutes->setValue(length/60);counts->setPlainText(lines.join('\n'));
        }
    }
    auto* irregularNote=new QLabel(text("editorDemandIrregular"),&dialog);irregularNote->setWordWrap(true);
    irregularNote->setVisible(irregular);form->addRow(irregularNote);
    auto countsDirty=std::make_shared<bool>(false);
    connect(counts,&QPlainTextEdit::textChanged,&dialog,[countsDirty]{*countsDirty=true;});
    connect(minutes,&QDoubleSpinBox::valueChanged,&dialog,[countsDirty](double){*countsDirty=true;});
    const auto syncDerived=[&]{
        const auto periods=countedTarget();
        fromCounts->setEnabled(periods.has_value());
        const bool counted=fromCounts->isChecked()&&periods;
        fromCountsTotal->setVisible(counted);
        if(counted) {
            double vehicles=0;for(const auto& p:*periods)vehicles+=p.vehiclesPerHour*(p.endTime-p.startTime)/3600;
            fromCountsTotal->setText(text("editorInputFromCountsTotal").arg(vehicles,0,'f',0).arg(periods->size()));
        }
        const bool timed=(!value.intervals.empty() && !*countsDirty) || !counts->toPlainText().trimmed().isEmpty();
        volume->setEnabled(!timed&&!counted);start->setEnabled(!timed&&!counted);end->setEnabled(!timed&&!counted);
        minutes->setEnabled(!counted&&(!timed || !counts->toPlainText().trimmed().isEmpty()));
        counts->setEnabled(!counted);
    };
    connect(counts,&QPlainTextEdit::textChanged,&dialog,syncDerived);
    connect(fromCounts,&QCheckBox::toggled,&dialog,syncDerived);
    connect(route,&QComboBox::currentTextChanged,&dialog,syncDerived);syncDerived();
    auto* periods=new QPushButton(text("editorDemandPeriods"),&dialog);periods->setObjectName("editorInputPeriods");form->addRow(periods);
    connect(periods,&QPushButton::clicked,&dialog,[&]{
        std::vector<std::vector<double>> rows;
        auto pending=value;
        if(pending.intervals.empty()){pending.startTime=start->value();pending.endTime=end->value();pending.vehiclesPerHour=volume->value();}
        for(const auto& p:inputPeriods(pending))rows.push_back({p.startTime,p.endTime,p.vehiclesPerHour});
        const auto edited=editDemandPeriods(&dialog,{text("editorInputStart"),text("editorInputEnd"),text("editorInputVolume")},rows,
            [this](const char* key){return text(key);});
        if(!edited)return;
        value.intervals.clear();for(const auto& r:*edited)value.intervals.push_back({r[0],r[1],r[2]});
        deriveInputTotals(value);*countsDirty=false;
        const QSignalBlocker blockCounts(counts),blockMinutes(minutes);
        counts->clear();volume->setValue(value.vehiclesPerHour);start->setValue(value.startTime);end->setValue(value.endTime);
        irregularNote->setVisible(!value.intervals.empty());syncDerived();
    });
    periods->setToolTip(text("editorDemandSaveCountsFirst"));
    const auto syncPeriods=[&]{periods->setEnabled(!*countsDirty && !(fromCounts->isChecked()&&fromCounts->isEnabled()));};
    syncPeriods();
    connect(fromCounts,&QCheckBox::toggled,&dialog,syncPeriods);
    connect(route,&QComboBox::currentTextChanged,&dialog,syncPeriods);
    connect(counts,&QPlainTextEdit::textChanged,&dialog,syncPeriods);
    connect(minutes,&QDoubleSpinBox::valueChanged,&dialog,syncPeriods);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    buttons->button(QDialogButtonBox::Ok)->setEnabled(route->count()>0 && type->count()>0);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    {
        const auto chosen=route->currentData().toString();
        const bool decision=chosen.startsWith("decision:"),link=chosen.startsWith("link:");
        const auto id=chosen.mid(chosen.indexOf(':')+1).toStdString();
        value.routingDecisionId=decision?id:std::string{};
        value.linkId=link?id:std::string{};
        value.routeId=decision||link?std::string{}:id;
    }
    {
        const auto chosen=type->currentData().toString();
        const bool composition=chosen.startsWith("composition:");
        const auto id=chosen.mid(chosen.indexOf(':')+1).toStdString();
        value.compositionId=composition?id:std::string{};
        value.vehicleTypeId=composition?std::string{}:id;
    }
    value.vehiclesPerHour=volume->value();value.startTime=start->value();value.endTime=end->value();
    // Left alone, the fields the author saw stay unwritten and the input keeps whatever
    // laneShares it already had (usually empty -- the M1.26 equal split). Touched, they replace
    // it outright, weights for exactly the lanes shown.
    if(*sharesDirty && policy().acceptsShares){
        value.laneShares.clear();
        for(auto* field:*shareFields)value.laneShares.push_back(field->value());
    }
    value.volumeFromCounts=fromCounts->isChecked()&&fromCounts->isEnabled();
    const auto rawCounts=counts->toPlainText();const double length=minutes->value()*60;
    std::string created;if(execute("editorEditInput",[&](auto& d){
        if(*countsDirty && !value.volumeFromCounts) {
            value.intervals.clear();
            const auto items=rawCounts.split(QRegularExpression("[\\s,;]+"),Qt::SkipEmptyParts);
            for(int k=0;k<items.size();++k) {
                bool ok=false;const double count=items[k].toDouble(&ok);
                if(!ok||!(count>=0))throw std::invalid_argument("EDIT_INVALID_COUNTS");
                const double from=value.startTime+k*length;
                value.intervals.push_back({from,from+length,count*3600/length});
            }
        }
        created=putInput(d,value);
    }))selectDemand(created);
}
}
