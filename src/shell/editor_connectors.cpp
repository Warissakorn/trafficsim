#include "editor_window.hpp"
#include <QLineEdit>
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
// Comma-separated metres, blank meaning "none authored". Mirrors how editorLaneWidths is read for
// a Link, so the two fields behave the same way for an author who has used one of them.
std::vector<double> numbers(const QString& text) {
    std::vector<double> result;
    if(text.trimmed().isEmpty())return result;
    for(const auto& part:text.split(',',Qt::KeepEmptyParts)) {
        bool ok=false;const double value=part.trimmed().toDouble(&ok);
        if(!ok)throw std::invalid_argument("EDIT_LANES");
        result.push_back(value);
    }
    return result;
}
std::vector<MarkingType> markingTypes(const QString& text) {
    std::vector<MarkingType> result;
    if(text.trimmed().isEmpty())return result;
    for(const auto& part:text.split(',',Qt::KeepEmptyParts)) {
        const auto name=part.trimmed().toLower();
        result.push_back(markingFromName(name.toStdString()));
    }
    return result;
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
    // Vissim's Intermediate points: how many poly points shape the curve between the two
    // attachments. Changing it re-lays the Connector's own road, so a shape the author has bent
    // survives the change instead of snapping back to the default curve.
    connectorPoints_=new QSpinBox(page);connectorPoints_->setObjectName("editorConnectorPoints");
    connectorPoints_->setRange(0,40);label(form,"editorConnectorPoints",connectorPoints_);
    // Vissim's Lanes tab. Comma-separated metres, one per lane, and comma-separated marking names
    // for the interior dividers -- the same text shape the Link lane-width row uses, so an author
    // who has met one has met both. Empty means "derive it from the links", which is what every
    // Connector drawn before this field did.
    connectorWidths_=new QLineEdit(page);connectorWidths_->setObjectName("editorConnectorWidths");
    label(form,"editorConnectorWidths",connectorWidths_);
    connectorMarkings_=new QLineEdit(page);connectorMarkings_->setObjectName("editorConnectorMarkings");
    label(form,"editorConnectorMarkings",connectorMarkings_);
    connect(connectorPoints_,&QSpinBox::valueChanged,this,[this](int count){
        if(!canvas_->selectedConnector() || static_cast<int>(canvas_->selectedConnector()->geometry.size())-2==count)return;
        const auto id=canvas_->selected();
        execute("editorConnectorPoints",[&](auto& d){resampleConnectorPoints(d,id,count);});
    });
    connectorFromPosition_=new QDoubleSpinBox(page);connectorToPosition_=new QDoubleSpinBox(page);
    // Metres from the link's start, like Vissim's Pos. The maximum is per link, so it is set
    // by refreshConnectorRanges alongside the lane-count maxima.
    for(auto* position:{connectorFromPosition_,connectorToPosition_}){position->setRange(0,1e6);position->setDecimals(3);position->setSuffix(" m");}
    label(form,"editorFromPosition",connectorFromPosition_);label(form,"editorToPosition",connectorToPosition_);
    connect(connectorFrom_,&QComboBox::currentIndexChanged,this,[this]{refreshConnectorRanges();});
    connect(connectorTo_,&QComboBox::currentIndexChanged,this,[this]{refreshConnectorRanges();});
    const auto positioned=[this](bool outgoing) {
        auto ref=reference(outgoing?connectorFrom_:connectorTo_);
        const auto& network=history_.document().network;
        const double metres=(outgoing?connectorFromPosition_:connectorToPosition_)->value();
        double reference=0;
        for(const auto& l:network.links)if(l.id==ref.linkId)reference=polylineLength(l.geometry);
        if(const auto* c=canvas_->selectedConnector()) {
            const auto& old=outgoing?c->from:c->to;
            // An untouched Apply must not rewrite a stored value through the widget's decimals.
            if(old.linkId==ref.linkId && old.laneId==ref.laneId &&
               std::round(attachmentStation(network,old,outgoing)*1e3)/1e3==metres) {
                ref.station=old.station;return ref;
            }
        }
        if(metres!=(outgoing?reference:0.))ref.station=metres;
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
            const auto widths=numbers(connectorWidths_->text());
            const auto markings=markingTypes(connectorMarkings_->text());
            // One transaction, as before: endpoints, range and the Lanes tab commit or roll back
            // together. Ordered after the range change, because that is what decides how many
            // lanes there are for the widths to describe.
            execute("editorApplyConnector",[&](auto& d){
                changeConnectorEndpoints(d,id,from,to);
                changeConnectorRange(d,id,connectorFromCount_->value(),connectorToCount_->value());
                changeConnectorLanes(d,id,widths,markings);});
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
    for(auto trio:{std::tuple{connectorFrom_,connectorFromCount_,connectorFromPosition_},
                   std::tuple{connectorTo_,connectorToCount_,connectorToPosition_}}) {
        const auto& [box,count,position]=trio;
        int available=1;double reference=0;
        for(const auto& l:history_.document().network.links)for(std::size_t i=0;i<l.lanes.size();++i)
            if(QString::fromStdString(l.lanes[i].id)==box->currentData().toString()) {
                available=static_cast<int>(l.lanes.size()-i);reference=polylineLength(l.geometry);
            }
        count->setMaximum(available);
        // A station cannot name a place the link does not reach.
        position->setMaximum(reference>0?reference:1e6);
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
    connectorPoints_->setEnabled(connector!=nullptr);
    connectorWidths_->setEnabled(connector!=nullptr);connectorMarkings_->setEnabled(connector!=nullptr);
    actions_.at("editorCreateConnector")->setEnabled(connectorFrom_->count()>1);
    if(connector){
        refreshConnectorRanges();
        connectorFromPosition_->setValue(attachmentStation(history_.document().network,connector->from,true));
        connectorToPosition_->setValue(attachmentStation(history_.document().network,connector->to,false));
        connectorFromCount_->setValue(connector->fromLaneCount);connectorToCount_->setValue(connector->toLaneCount);
        QStringList widths;
        for(const double w:connector->laneWidths)widths<<QString::number(w,'g',10);
        connectorWidths_->setText(widths.join(", "));
        QStringList markings;
        for(const auto m:connector->laneMarkings)markings<<markingName(m);
        connectorMarkings_->setText(markings.join(", "));
        const QSignalBlocker block(connectorPoints_);
        connectorPoints_->setValue(static_cast<int>(connector->geometry.size())-2);}
    // These boxes double as the creation form. Leaving a selected connector's counts behind
    // would make the next Create connector inherit a width the new gesture never asked for.
    else {connectorFromCount_->setValue(1);connectorToCount_->setValue(1);
          connectorWidths_->clear();connectorMarkings_->clear();}
    if (connector) {
        // Say how many lanes each end carries. A connector that drops or gains lanes is legal
        // to author, and seeing 3 -> 2 on the canvas is how the author notices it is a merge.
        selectionInfo_->setText(text("editorConnectorLength").arg(polylineLength(connector->geometry),0,'f',2)+"   "+
            text("editorConnectorLanes").arg(connector->fromLaneCount).arg(connector->toLaneCount));
    }
    connectorHint();
}
}
