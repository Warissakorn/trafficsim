#include "canvas.hpp"
#include "canvas_style.hpp"
#include <QApplication>
#include <cmath>

namespace trafficsim {
void EditorCanvas::drawPositionedDecisions() {
    if(!document_ || !document_->definition)return;
    const auto& n=document_->network;
    for(const auto& decision:document_->definition->routingDecisions)if(decision.position)
        for(const auto& link:n.links)if(link.id==decision.linkId && levelVisible(link.level)) {
            const double position=decisionDrag_ && decisionDrag_->id==decision.id?decisionDrag_->position:*decision.position;
            const auto axis=linkCentreline(link,n.drivingSide);
            const auto bar=roadCrossbar(axis,laneBoundaryGeometry(link,0,n.drivingSide),
                laneBoundaryGeometry(link,link.lanes.size(),n.drivingSide),matchedStation(link.geometry,axis,position),link.level);
            if(!bar)continue;
            auto* item=new RoadCrossbarItem(*bar,canvasStyle::active(),std::abs(transform().m11()));scene_.addItem(item);
            item->setZValue(200008);item->setData(0,QStringLiteral("decision-marker"));item->setData(1,QString::fromStdString(decision.id));
        }
}
bool EditorCanvas::startDecisionDrag(QPoint point) {
    if(!document_ || !document_->definition)return false;
    const auto [kind,id]=demandObjectAt(point);
    if(kind!="decision")return false;
    for(const auto& decision:document_->definition->routingDecisions)if(decision.id==id && decision.position) {
        decisionDrag_=DecisionDrag{id,decision.linkId,*decision.position,point,false};return true;
    }
    return false;
}
void EditorCanvas::updateDecisionDrag(QPoint point) {
    if(!decisionDrag_ || !document_)return;
    auto& drag=*decisionDrag_;
    if(!drag.moved && (point-drag.press).manhattanLength()<QApplication::startDragDistance())return;
    for(const auto& link:document_->network.links)if(link.id==drag.linkId) {
        drag.position=std::min(stationOfClosestPoint(link.geometry,world(point,false)),std::max(0.,polylineLength(link.geometry)-1e-6));
        drag.moved=true;redraw();return;
    }
}
void EditorCanvas::finishDecisionDrag(QPoint point) {
    updateDecisionDrag(point);const auto drag=*decisionDrag_;decisionDrag_.reset();redraw();
    if(drag.moved && decisionMoved)decisionMoved(drag.id,drag.position);
}
}
