#include "editor_window.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace trafficsim {
namespace {
QDoubleSpinBox* number(QDialog& dialog,double min,double max,double value,const char* name) {
    auto* field=new QDoubleSpinBox(&dialog);field->setObjectName(name);field->setDecimals(3);
    field->setRange(min,max);field->setValue(value);return field;
}
}
void EditorWindow::editRunSettings() {
    const auto def=history_.document().definition.value_or(AuthoringDefinition{});
    QDialog dialog(this);dialog.setObjectName("editorSettingsDialog");dialog.setWindowTitle(text("editorRunSettings"));
    auto* form=new QFormLayout(&dialog);
    auto* duration=number(dialog,0.001,10000000,def.duration,"editorDuration");
    auto* dt=number(dialog,0.001,0.5,def.timeStep,"editorTimeStep");
    form->addRow(text("editorDuration"),duration);form->addRow(text("editorTimeStep"),dt);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()==QDialog::Accepted)execute("editorRunSettings",[&](auto& d){changeRunSettings(d,duration->value(),dt->value());});
}
void EditorWindow::editProgram(const std::string& id) {
    SignalProgram value{id,0,{{30,SignalColor::red},{30,SignalColor::green},{3,SignalColor::amber}}};
    if(history_.document().definition)for(const auto& p:history_.document().definition->signalPrograms)if(p.id==id)value=p;
    QDialog dialog(this);dialog.setObjectName("editorProgramDialog");dialog.setWindowTitle(text("editorEditProgram"));
    auto* layout=new QVBoxLayout(&dialog);auto* form=new QFormLayout;layout->addLayout(form);
    auto* offset=number(dialog,-10000000,10000000,value.offset,"editorProgramOffset");form->addRow(text("editorProgramOffset"),offset);
    auto* phases=new QTableWidget(0,2,&dialog);phases->setObjectName("editorPhases");
    phases->setHorizontalHeaderLabels({text("editorPhaseDuration"),text("editorPhaseColor")});
    phases->horizontalHeader()->setStretchLastSection(true);layout->addWidget(phases);
    const auto append=[&](SignalPhase phase){
        const int row=phases->rowCount();phases->insertRow(row);
        phases->setCellWidget(row,0,number(dialog,0.001,10000000,phase.duration,"editorPhaseDuration"));
        auto* color=new QComboBox(phases);color->addItem(text("editorRed"),static_cast<int>(SignalColor::red));
        color->addItem(text("editorAmber"),static_cast<int>(SignalColor::amber));color->addItem(text("editorGreen"),static_cast<int>(SignalColor::green));
        color->setCurrentIndex(color->findData(static_cast<int>(phase.color)));phases->setCellWidget(row,1,color);
    };
    for(const auto& phase:value.phases)append(phase);
    auto* add=new QPushButton(text("editorAddPhase"),&dialog);layout->addWidget(add);
    connect(add,&QPushButton::clicked,&dialog,[&]{append({10,SignalColor::red});});
    auto* remove=new QPushButton(text("editorRemoveLast"),&dialog);layout->addWidget(remove);
    connect(remove,&QPushButton::clicked,&dialog,[&]{if(phases->rowCount())phases->removeRow(phases->rowCount()-1);});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    value.offset=offset->value();value.phases.clear();
    for(int row=0;row<phases->rowCount();++row)value.phases.push_back({
        qobject_cast<QDoubleSpinBox*>(phases->cellWidget(row,0))->value(),
        static_cast<SignalColor>(qobject_cast<QComboBox*>(phases->cellWidget(row,1))->currentData().toInt())});
    std::string created;if(execute("editorEditProgram",[&](auto& d){created=putProgram(d,value);}))selectDemand(created);
}
void EditorWindow::editHead(const std::string& id,const std::optional<HeadPlacement>& placed) {
    const auto& network=history_.document().network;
    NetworkSignalHead value{id,{},0,{}};
    for(const auto& h:network.signalHeads)if(h.id==id)value=h;
    if(placed){value.lane=placed->slot.lane;value.connectorId=placed->slot.connectorId;value.position=placed->station;}
    if(value.programId.empty())value.programId=lastHeadProgram_;
    QDialog dialog(this);dialog.setObjectName("editorHeadDialog");dialog.setWindowTitle(text("editorEditHead"));
    auto* form=new QFormLayout(&dialog);auto* lane=new QComboBox(&dialog);lane->setObjectName("editorHeadLane");
    // Names first, the way the canvas shows them: "Main St · lane 2", never "link-4 / lane-9".
    // The station's upper bound is the chosen lane's own length, so a stop line cannot be typed
    // off the end of the road it holds.
    const auto addSlot=[&](const QString& label,const std::string& key,const std::string& linkId,const NetworkSignalHead& probe){
        const auto slot=headSlot(network,probe);
        lane->addItem(label,QString::fromStdString(key));
        lane->setItemData(lane->count()-1,QString::fromStdString(linkId),Qt::UserRole+1);
        lane->setItemData(lane->count()-1,slot?polylineLength(slot->geometry):0.,Qt::UserRole+2);
    };
    for(const auto& link:network.links)for(std::size_t i=0;i<link.lanes.size();++i) {
        NetworkSignalHead probe;probe.lane={link.id,link.lanes[i].id};
        addSlot(QString::fromStdString(link.name.empty()?link.id:link.name)+" · "+text("editorLaneNumber").arg(i+1),link.lanes[i].id,link.id,probe);
    }
    for(const auto& c:network.connectors){const auto paths=connectorPaths(network,c);for(std::size_t i=0;i<paths.size();++i) {
        NetworkSignalHead probe;probe.connectorId=paths[i].id;
        addSlot(QString::fromStdString(c.name.empty()?c.id:c.name)+" · "+text("editorPathNumber").arg(i+1),paths[i].id,{},probe);
    }}
    if(!value.connectorId.empty())lane->setCurrentIndex(lane->findData(QString::fromStdString(value.connectorId)));
    if(!value.lane.laneId.empty())lane->setCurrentIndex(lane->findData(QString::fromStdString(value.lane.laneId)));
    auto* program=new QComboBox(&dialog);program->setObjectName("editorHeadProgram");
    if(history_.document().definition)for(const auto& p:history_.document().definition->signalPrograms)program->addItem(QString::fromStdString(p.id));
    if(!value.programId.empty())program->setCurrentText(QString::fromStdString(value.programId));
    auto* station=number(dialog,0,10000000,value.position,"editorHeadPosition");
    const auto bound=[&]{
        const double length=lane->currentData(Qt::UserRole+2).toDouble();
        station->setMaximum(length>0?length:10000000);
    };
    bound();station->setValue(value.position);
    connect(lane,qOverload<int>(&QComboBox::currentIndexChanged),&dialog,bound);
    form->addRow(text("editorColumnLane"),lane);form->addRow(text("editorColumnProgram"),program);
    form->addRow(text("editorStopLine"),station);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    buttons->button(QDialogButtonBox::Ok)->setEnabled(lane->count()>0 && program->count()>0);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    value.lane={lane->currentData(Qt::UserRole+1).toString().toStdString(),lane->currentData().toString().toStdString()};
    value.connectorId.clear();
    if(value.lane.linkId.empty()){value.connectorId=value.lane.laneId;value.lane={};}
    value.position=station->value();value.programId=program->currentText().toStdString();
    if(execute("editorEditHead",[&](auto& d){putSignalHead(d,value);}))lastHeadProgram_=value.programId;
}
}
