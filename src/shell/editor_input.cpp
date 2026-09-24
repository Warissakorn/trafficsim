#include "editor_window.hpp"
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
void EditorWindow::editInput(const std::string& id,const std::string& preselectedRoute) {
    VehicleInput value{id,{},{},600,0,history_.document().definition?history_.document().definition->duration:180};
    if(history_.document().definition)for(const auto& i:history_.document().definition->inputs)if(i.id==id)value=i;
    ScenarioDefinition catalog;
    std::vector<Composition> compositions;
    try {
        catalog=resolveCatalogs(history_.document().definition.value_or(AuthoringDefinition{}),data_);
        compositions=loadCompositions(data_);
    }
    catch(const std::exception& e){showError(e);return;}
    QDialog dialog(this);dialog.setObjectName("editorInputDialog");dialog.setWindowTitle(text("editorEditInput"));
    auto* form=new QFormLayout(&dialog);
    auto* route=new QComboBox(&dialog);route->setObjectName("editorInputRoute");
    for(const auto& r:catalog.routes)route->addItem(QString::fromStdString(r.id));
    // A route placed by pointer names the route it was dropped on; the dialog opens on it.
    if(value.routeId.empty())value.routeId=preselectedRoute;
    if(!value.routeId.empty())route->setCurrentText(QString::fromStdString(value.routeId));
    form->addRow(text("editorInputRoute"),route);
    auto* type=new QComboBox(&dialog);type->setObjectName("editorInputType");
    // One list for both (M2.3): a vehicle type, or a composition of types from data/compositions/.
    // The item data says which, so an id shared by a type and a composition cannot be confused.
    for(const auto& t:catalog.vehicleTypes)
        type->addItem(QString::fromStdString(t.id),"type:"+QString::fromStdString(t.id));
    for(const auto& c:compositions)
        type->addItem(text("editorInputCompositionItem").arg(QString::fromStdString(c.id)),
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
    // Rule 4: say what the split is and is not. It divides the Link total equally because the
    // engine has no lane changing, not because traffic distributes itself that way.
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
    form->addRow(sharesGroup);
    auto* sharesHelp=new QLabel(text("editorInputShareHelp"),&dialog);sharesHelp->setObjectName("editorInputShareHelp");
    sharesHelp->setWordWrap(true);form->addRow(sharesHelp);
    auto sharesDirty=std::make_shared<bool>(false);
    auto shareFields=std::make_shared<std::vector<QDoubleSpinBox*>>();
    const auto laneCount=[&](const std::string& routeId)->std::size_t{
        for(const auto& r:catalog.routes)if(r.id==routeId)
            return std::max<std::size_t>(1,routeLaneChains(history_.document().network,r.segmentIds).size());
        return 1;
    };
    const auto rebuildShares=[&,sharesDirty,shareFields]{
        while(sharesForm->count()>0){auto* item=sharesForm->takeAt(0);delete item->widget();delete item;}
        shareFields->clear();*sharesDirty=false;
        const auto lanes=laneCount(route->currentText().toStdString());
        sharesGroup->setVisible(lanes>1);sharesHelp->setVisible(lanes>1);
        if(lanes<=1)return;
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
    // Shown as counts only when the stored intervals are what this box can write back --
    // contiguous and of one length. Anything else is left alone unless the author types here.
    if(!value.intervals.empty()) {
        const double length=value.intervals.front().endTime-value.intervals.front().startTime;
        bool regular=length>0;
        for(std::size_t k=0;k<value.intervals.size()&&regular;++k)
            regular=std::abs(value.intervals[k].endTime-value.intervals[k].startTime-length)<1e-9 &&
                    (k==0||std::abs(value.intervals[k].startTime-value.intervals[k-1].endTime)<1e-9);
        if(regular) {
            QStringList lines;
            for(const auto& p:value.intervals)lines<<QString::number(p.vehiclesPerHour*length/3600,'g',12);
            const QSignalBlocker a(minutes),b(counts);
            minutes->setValue(length/60);counts->setPlainText(lines.join('\n'));
        }
    }
    auto countsDirty=std::make_shared<bool>(false);
    connect(counts,&QPlainTextEdit::textChanged,&dialog,[countsDirty]{*countsDirty=true;});
    connect(minutes,&QDoubleSpinBox::valueChanged,&dialog,[countsDirty](double){*countsDirty=true;});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    buttons->button(QDialogButtonBox::Ok)->setEnabled(route->count()>0 && type->count()>0);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    value.routeId=route->currentText().toStdString();
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
    if(*sharesDirty){
        value.laneShares.clear();
        for(auto* field:*shareFields)value.laneShares.push_back(field->value());
    }
    const auto rawCounts=counts->toPlainText();const double length=minutes->value()*60;
    std::string created;if(execute("editorEditInput",[&](auto& d){
        if(*countsDirty) {
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
