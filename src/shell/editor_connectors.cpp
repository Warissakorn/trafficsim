#include "editor_window.hpp"
#include <QAction>
#include <QAbstractButton>
#include <QComboBox>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <cmath>
#include <QLabel>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QToolButton>

namespace trafficsim {
namespace {
LaneReference reference(const QComboBox* box) {
    if (box->currentData().toString().isEmpty()) throw std::invalid_argument("EDIT_CONNECTOR_LANES");
    return {box->currentData(Qt::UserRole+1).toString().toStdString(),box->currentData().toString().toStdString()};
}
}
QWidget* EditorWindow::buildConnectorInspector() {
    auto* page=new QWidget(this);auto* form=new QFormLayout(page);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    connectorObject_=new QComboBox(page);label(form,"editorConnectorObject",connectorObject_);
    connectorFrom_=new QComboBox(page);connectorTo_=new QComboBox(page);
    for (auto* box : {connectorObject_,connectorFrom_,connectorTo_}) {
        box->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        box->setMinimumContentsLength(12);
    }
    label(form,"editorConnectorFrom",connectorFrom_);label(form,"editorConnectorTo",connectorTo_);
    connectorFromCount_=new QSpinBox(page);connectorToCount_=new QSpinBox(page);
    connectorFromCount_->setRange(1,12);connectorToCount_->setRange(1,12);
    label(form,"editorFromLaneCount",connectorFromCount_);label(form,"editorToLaneCount",connectorToCount_);
    connectorFromPosition_=new QDoubleSpinBox(page);connectorToPosition_=new QDoubleSpinBox(page);
    for(auto* position:{connectorFromPosition_,connectorToPosition_}){position->setRange(0,100);position->setDecimals(6);position->setSuffix(" %");}
    connectorFromPosition_->setValue(100);
    label(form,"editorFromPosition",connectorFromPosition_);label(form,"editorToPosition",connectorToPosition_);
    connect(connectorFrom_,&QComboBox::currentIndexChanged,this,[this]{refreshConnectorRanges();});
    connect(connectorTo_,&QComboBox::currentIndexChanged,this,[this]{refreshConnectorRanges();});
    const auto positioned=[this](bool outgoing) {
        auto ref=reference(outgoing?connectorFrom_:connectorTo_);
        const double percent=(outgoing?connectorFromPosition_:connectorToPosition_)->value();
        if(const auto* c=canvas_->selectedConnector()) {
            const auto& old=outgoing?c->from:c->to;
            if(std::round(old.fraction.value_or(outgoing?1.:0.)*1e8)/1e6==percent){ref.fraction=old.fraction;return ref;}
        }
        if(percent!=(outgoing?100.:0.))ref.fraction=percent/100.;
        return ref;
    };
    connect(connectorObject_,&QComboBox::currentIndexChanged,this,[this]{
        canvas_->select(connectorObject_->currentData().toString().toStdString());
    });
    auto button=[&](const std::string& key,const std::function<void()>& callback){
        auto* b=new QToolButton(page);b->setToolButtonStyle(Qt::ToolButtonTextOnly);
        b->setDefaultAction(action(key,{},callback));form->addRow(b);
    };
    button("editorCreateConnector",[this,positioned]{
        try {addConnection(positioned(true),positioned(false));}
        catch (const std::exception& e) {showError(e);}
    });
    button("editorApplyConnector",[this,positioned]{
        try {
            const auto from=positioned(true), to=positioned(false);
            const auto id=canvas_->selected();
            execute("editorApplyConnector",[&](auto& d){changeConnectorEndpoints(d,id,from,to);changeConnectorRange(d,id,connectorFromCount_->value(),connectorToCount_->value());});
        } catch (const std::exception& e) {showError(e);}
    });
    button("editorResetCurve",[this]{execute("editorResetCurve",[&](auto& d){resetConnectorCurve(d,canvas_->selected());});});
    button("editorStraightConnector",[this]{execute("editorStraightConnector",[&](auto& d){resetConnectorCurve(d,canvas_->selected(),true);});});
    button("editorDeleteConnector",[this]{
        const auto id=canvas_->selected();
        QMessageBox box(QMessageBox::Question,text("editorDeleteConnector"),text("editorDeleteConnectorWarning"),QMessageBox::Yes|QMessageBox::No,this);
        box.button(QMessageBox::Yes)->setText(text("editorConfirm"));box.button(QMessageBox::No)->setText(text("editorCancel"));box.setDefaultButton(QMessageBox::No);
        if (box.exec()==QMessageBox::Yes) execute("editorDeleteConnector",[&](auto& d){deleteConnector(d,id);});
    });
    connectorHint_=new QLabel(page);connectorHint_->setObjectName("editorConnectorHint");connectorHint_->setWordWrap(true);form->addRow(connectorHint_);
    auto* help=new QLabel(page);help->setWordWrap(true);texts_["editorConnectorHelp"]=help;form->addRow(help);
    return page;
}
void EditorWindow::connectorHint() {
    const auto key=tool_->currentIndex()!=5?"editorConnectorCanvasHint":
        canvas_->pickingConnectorTarget()?"editorPickTargetLane":"editorPickSourceLane";
    connectorHint_->setText(text(key));
}
void EditorWindow::addConnection(const LaneReference& from,const LaneReference& to) {
    const auto count=[this](const LaneReference& ref,int requested) {
        for(const auto& l:history_.document().network.links)for(std::size_t i=0;i<l.lanes.size();++i)
            if(l.id==ref.linkId && l.lanes[i].id==ref.laneId)return std::min(requested,static_cast<int>(l.lanes.size()-i));
        return requested;
    };
    std::string id;
    if (execute("editorCreateConnector",[&](auto& d){id=addConnectorRange(d,from,to,count(from,connectorFromCount_->value()),count(to,connectorToCount_->value()));})) canvas_->select(id);
}
void EditorWindow::refreshConnectorRanges() {
    for(auto pair:{std::pair{connectorFrom_,connectorFromCount_},std::pair{connectorTo_,connectorToCount_}}) {
        int available=1;
        for(const auto& l:history_.document().network.links)for(std::size_t i=0;i<l.lanes.size();++i)
            if(QString::fromStdString(l.lanes[i].id)==pair.first->currentData().toString())available=static_cast<int>(l.lanes.size()-i);
        pair.second->setMaximum(available);
    }
}
void EditorWindow::refreshConnector() {
    const auto* connector=canvas_->selectedConnector();
    {
        const QSignalBlocker block(connectorObject_);connectorObject_->clear();
        connectorObject_->addItem(text("editorChooseConnector"),QString());
        for (const auto& c : history_.document().network.connectors)
            connectorObject_->addItem(QString::fromStdString(c.id),QString::fromStdString(c.id));
        connectorObject_->setCurrentIndex(connector?connectorObject_->findData(QString::fromStdString(connector->id)):0);
    }
    for (auto* box : {connectorFrom_,connectorTo_}) {
        const auto selected=connector?QString::fromStdString(box==connectorFrom_?connector->from.laneId:connector->to.laneId):box->currentData().toString();
        const QSignalBlocker block(box);box->clear();box->addItem(text("editorChooseLane"),QString());
        for (const auto& link : history_.document().network.links) for (const auto& lane : link.lanes) {
            box->addItem(QString::fromStdString(link.id+" / "+lane.id),QString::fromStdString(lane.id));
            box->setItemData(box->count()-1,QString::fromStdString(link.id),Qt::UserRole+1);
        }
        const int index=box->findData(selected);box->setCurrentIndex(index<0?0:index);
    }
    for (const auto* key : {"editorApplyConnector","editorResetCurve","editorStraightConnector","editorDeleteConnector"})
        actions_.at(key)->setEnabled(connector);
    actions_.at("editorCreateConnector")->setEnabled(connectorFrom_->count()>1);
    if(connector){
        connectorFromPosition_->setValue(connector->from.fraction.value_or(1.)*100);
        connectorToPosition_->setValue(connector->to.fraction.value_or(0.)*100);
        refreshConnectorRanges();
        connectorFromCount_->setValue(connector->fromLaneCount);connectorToCount_->setValue(connector->toLaneCount);}
    if (connector) {
        // Say how many lanes each end carries. A connector that drops or gains lanes is legal
        // to author, and seeing 3 -> 2 on the canvas is how the author notices it is a merge.
        selectionInfo_->setText(text("editorConnectorLength").arg(polylineLength(connector->geometry),0,'f',2)+"   "+
            text("editorConnectorLanes").arg(connector->fromLaneCount).arg(connector->toLaneCount));
    }
    connectorHint();
}
}
