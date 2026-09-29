#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPushButton>
#include <QSpinBox>
#include <algorithm>
namespace trafficsim {
void EditorWindow::createLinkDialog(const std::vector<Point>& points) {
    pauseRun();QDialog dialog(this);dialog.setObjectName("editorLinkDialog");dialog.setWindowTitle(text("editorLinkData"));
    auto* form=new QFormLayout(&dialog);auto* count=new QSpinBox(&dialog);count->setObjectName("editorGestureLaneCount");
    form->setContentsMargins(editorDesign::space3,editorDesign::space3,editorDesign::space3,editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2);form->setVerticalSpacing(editorDesign::space1);
    count->setFont(editorDesign::numericFont());count->setRange(1,12);count->setValue(count_->value());form->addRow(text("editorLaneCount"),count);
    auto* width=new QDoubleSpinBox(&dialog);width->setRange(.1,20);width->setValue(width_->value());form->addRow(text("editorDefaultWidth"),width);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    count_->setValue(count->value());width_->setValue(width->value());
    auto geometry=points;geometry.erase(std::unique(geometry.begin(),geometry.end()),geometry.end());
    if(canvas_->createLink)canvas_->createLink(geometry);
}
void EditorWindow::createRangeDialog(LaneReference from,LaneReference to,const std::vector<Point>& points) {
    pauseRun();QDialog dialog(this);dialog.setObjectName("editorRangeDialog");dialog.setWindowTitle(text("editorConnect"));
    auto* form=new QFormLayout(&dialog);form->setContentsMargins(editorDesign::space3,editorDesign::space3,editorDesign::space3,editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2);form->setVerticalSpacing(editorDesign::space1);
    auto* source=new QComboBox(&dialog);source->setObjectName("editorRangeFrom");
    auto* target=new QComboBox(&dialog);target->setObjectName("editorRangeTo");
    for(const auto& l:history_.document().network.links)for(const auto& lane:l.lanes) {
        if(l.id==from.linkId)source->addItem(QString::fromStdString(lane.id),QString::fromStdString(lane.id));
        if(l.id==to.linkId)target->addItem(QString::fromStdString(lane.id),QString::fromStdString(lane.id));
    }
    source->setCurrentIndex(source->findData(QString::fromStdString(from.laneId)));
    target->setCurrentIndex(target->findData(QString::fromStdString(to.laneId)));
    auto* fromCount=new QSpinBox(&dialog);fromCount->setObjectName("editorRangeFromCount");
    auto* toCount=new QSpinBox(&dialog);toCount->setObjectName("editorRangeToCount");
    fromCount->setFont(editorDesign::numericFont());toCount->setFont(editorDesign::numericFont());
    // Each box stops at the lanes left on its Link and at two lanes from the other end (D73).
    const auto ranges=[&]{
        const int a=std::max(1,source->count()-source->currentIndex()),b=std::max(1,target->count()-target->currentIndex());
        const int f=fromCount->value(),t=toCount->value();
        fromCount->setRange(std::min(a,std::max(1,t-2)),std::min(a,t+2));
        toCount->setRange(std::min(b,std::max(1,f-2)),std::min(b,f+2));
    };
    // The owner's rule (M1.26): connect the WHOLE carriageway first and narrow it afterwards,
    // which is now safe because a route names the Connector rather than its paths. This
    // reverses the earlier default of one lane per gesture -- that default existed because
    // narrowing a routed Connector used to be refused outright.
    // Past a two-lane difference (M3.2.9d, D75) the wider end is narrowed to the lanes the rule
    // can pair, centred on the lane the drag ended on at that end.
    const auto focus=[](const QComboBox* box){return static_cast<std::size_t>(std::max(0,box->currentIndex()));};
    const std::size_t fromFocus=focus(source),toFocus=focus(target);
    source->setCurrentIndex(0);target->setCurrentIndex(0);
    int a=source->count(),b=target->count();
    if(a>b+2){a=b+2;source->setCurrentIndex(static_cast<int>(centredLaneRange(fromFocus,a,static_cast<std::size_t>(source->count()))));}
    if(b>a+2){b=a+2;target->setCurrentIndex(static_cast<int>(centredLaneRange(toFocus,b,static_cast<std::size_t>(target->count()))));}
    fromCount->setRange(1,12);toCount->setRange(1,12);fromCount->setValue(a);toCount->setValue(b);ranges();
    connect(source,&QComboBox::currentIndexChanged,&dialog,ranges);connect(target,&QComboBox::currentIndexChanged,&dialog,ranges);
    connect(fromCount,&QSpinBox::valueChanged,&dialog,ranges);connect(toCount,&QSpinBox::valueChanged,&dialog,ranges);
    form->addRow(text("editorConnectorFrom"),source);form->addRow(text("editorFromLaneCount"),fromCount);
    form->addRow(text("editorConnectorTo"),target);form->addRow(text("editorToLaneCount"),toCount);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    from.laneId=source->currentData().toString().toStdString();to.laneId=target->currentData().toString().toStdString();
    std::string created;
    if(execute("editorCreateConnector",[&](auto& d){
        created=addConnectorRange(d,from,to,fromCount->value(),toCount->value());
        if(points.size()>2) {
            auto& c=editableConnector(d,created);std::vector<Point> shape{c.geometry.front()};
            shape.insert(shape.end(),points.begin()+1,points.end()-1);shape.push_back(c.geometry.back());
            shape.erase(std::unique(shape.begin(),shape.end()),shape.end());changeConnectorGeometry(d,created,shape);
        }
    }))canvas_->select(created);
}
}
