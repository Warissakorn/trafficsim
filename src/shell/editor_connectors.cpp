#include "editor_window.hpp"
#include <QAction>
#include <QAbstractButton>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSignalBlocker>
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
    connect(connectorObject_,&QComboBox::currentIndexChanged,this,[this]{
        canvas_->select(connectorObject_->currentData().toString().toStdString());
    });
    auto button=[&](const std::string& key,const std::function<void()>& callback){
        auto* b=new QToolButton(page);b->setToolButtonStyle(Qt::ToolButtonTextOnly);
        b->setDefaultAction(action(key,{},callback));form->addRow(b);
    };
    button("editorCreateConnector",[this]{
        try {addConnection(reference(connectorFrom_),reference(connectorTo_));}
        catch (const std::exception& e) {showError(e);}
    });
    button("editorApplyConnector",[this]{
        try {
            const auto from=reference(connectorFrom_), to=reference(connectorTo_);
            const auto id=canvas_->selected();
            execute("editorApplyConnector",[&](auto& d){changeConnectorEndpoints(d,id,from,to);});
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
    std::string id;
    if (execute("editorCreateConnector",[&](auto& d){id=addConnector(d,from,to);})) canvas_->select(id);
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
    if (connector) selectionInfo_->setText(text("editorConnectorLength").arg(polylineLength(connector->geometry),0,'f',2));
    connectorHint();
}
}
