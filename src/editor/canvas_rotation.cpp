#include "canvas.hpp"
#include "../model/network/rotation.hpp"
#include <QApplication>
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <cmath>
#include <numbers>
namespace trafficsim {
std::optional<Point> EditorCanvas::rotationPivot() const {
    if(!document_)return {};
    return rotationCentre(document_->network,selection_);
}
void EditorCanvas::startRotation(QPoint position) {
    const auto picked=hit(world(position,false));
    if(picked.first.empty())return;
    if(!isSelected(picked.first))select(picked.first);
    const auto pivot=rotationPivot();
    if(!pivot)return;
    const auto p=world(position,false);
    // An angle has no stable direction at its centre. Leave the selection intact.
    if(std::hypot(p.x-pivot->x,p.y-pivot->y)*std::abs(transform().m11())<8)return;
    cancel();rotationPivot_=pivot;rotationStart_=p;dragPress_=position;
    redraw();
}
void EditorCanvas::updateRotation(QPoint position,bool angleSnap) {
    if(!rotationPivot_)return;
    rotationDragging_=(position-dragPress_).manhattanLength()>=QApplication::startDragDistance();
    const auto pivot=*rotationPivot_,p=world(position,false);
    rotationDegrees_=0;
    if(rotationDragging_ && std::hypot(p.x-pivot.x,p.y-pivot.y)*std::abs(transform().m11())>=8) {
        const double start=std::atan2(rotationStart_.y-pivot.y,rotationStart_.x-pivot.x);
        const double finish=std::atan2(p.y-pivot.y,p.x-pivot.x);
        rotationDegrees_=std::remainder((finish-start)*180./std::numbers::pi,360.);
        if(angleSnap)rotationDegrees_=std::round(rotationDegrees_/15.)*15.;
    }
    redraw();
}
void EditorCanvas::drawRotationPreview() {
    if(!rotationPivot_)return;
    const auto pivot=*rotationPivot_;
    QPen pen(QColor("#de8618"),2,Qt::DashLine);pen.setCosmetic(true);
    // Derive the matrix from the same transform as the command, including exact quarter turns.
    const auto origin=rotatePoint({0,0},pivot,rotationDegrees_);
    const auto x=rotatePoint({1,0},{},rotationDegrees_),y=rotatePoint({0,1},{},rotationDegrees_);
    const QTransform rotation(x.x,x.y,y.x,y.y,origin.x,origin.y);
    if(rotationDragging_)for(const auto& id:rotationObjects(document_->network,selection_)) {
        auto* item=scene_.addPath(rotation.map(objectShape(id)),pen,QBrush(QColor(222,134,24,60)));
        item->setZValue(200008);item->setData(0,QStringLiteral("rotation-preview"));
        item->setData(1,QString::fromStdString(id));
    }
    const double r=6/std::abs(transform().m11());
    auto* centre=scene_.addEllipse(pivot.x-r,pivot.y-r,2*r,2*r,pen,QBrush(Qt::white));
    centre->setZValue(200009);centre->setData(0,QStringLiteral("rotation-pivot"));
    const auto at=rotatePoint(rotationStart_,pivot,rotationDegrees_);
    auto* guide=scene_.addLine(pivot.x,pivot.y,at.x,at.y,pen);guide->setZValue(200009);
    auto* angle=scene_.addSimpleText(QString::number(rotationDegrees_,'f',1)+QChar(0x00b0));
    angle->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    angle->setBrush(QColor("#8a4a00"));angle->setPos(pivot.x+r*2,pivot.y-r*2);angle->setZValue(200010);
}
}
