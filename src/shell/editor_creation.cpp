#include "editor_window.hpp"
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
    count->setRange(1,12);count->setValue(count_->value());form->addRow(text("editorLaneCount"),count);
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
    auto* form=new QFormLayout(&dialog);auto* source=new QComboBox(&dialog);source->setObjectName("editorRangeFrom");
    auto* target=new QComboBox(&dialog);target->setObjectName("editorRangeTo");
    for(const auto& l:history_.document().network.links)for(const auto& lane:l.lanes) {
        if(l.id==from.linkId)source->addItem(QString::fromStdString(lane.id),QString::fromStdString(lane.id));
        if(l.id==to.linkId)target->addItem(QString::fromStdString(lane.id),QString::fromStdString(lane.id));
    }
    source->setCurrentIndex(source->findData(QString::fromStdString(from.laneId)));
    target->setCurrentIndex(target->findData(QString::fromStdString(to.laneId)));
    auto* fromCount=new QSpinBox(&dialog);fromCount->setObjectName("editorRangeFromCount");
    auto* toCount=new QSpinBox(&dialog);toCount->setObjectName("editorRangeToCount");
    const auto ranges=[&]{fromCount->setRange(1,std::max(1,source->count()-source->currentIndex()));toCount->setRange(1,std::max(1,target->count()-target->currentIndex()));};
    ranges();fromCount->setValue(std::min(fromCount->maximum(),toCount->maximum()));toCount->setValue(fromCount->value());
    connect(source,&QComboBox::currentIndexChanged,&dialog,ranges);connect(target,&QComboBox::currentIndexChanged,&dialog,ranges);
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
