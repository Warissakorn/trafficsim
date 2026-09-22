#include "canvas.hpp"
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <cmath>
#include <algorithm>
#include <numbers>

namespace trafficsim {
namespace {
QPointF q(Point p) { return {p.x, p.y}; }
QPainterPath path(const std::vector<Point>& points) {
    QPainterPath p;
    if (!points.empty()) { p.moveTo(q(points.front())); for (std::size_t i = 1; i < points.size(); ++i) p.lineTo(q(points[i])); }
    return p;
}
}
EditorCanvas::EditorCanvas(QWidget* parent) : QGraphicsView(parent), scene_(this) {
    setScene(&scene_); setObjectName("editorCanvas");
    setRenderHint(QPainter::Antialiasing); setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus); setTransformationAnchor(NoAnchor); setResizeAnchor(AnchorViewCenter);
    setTransform(QTransform::fromScale(4, -4));
    scene_.setSceneRect(-10000, -10000, 20000, 20000); centerOn(0, 0);
}
const Link* EditorCanvas::selectedLink() const {
    const auto primary = selected();
    if (document_ && !primary.empty())
        for (const auto& l : document_->network.links) if (l.id == primary) return &l;
    return nullptr;
}
const Connector* EditorCanvas::selectedConnector() const {
    const auto primary = selected();
    if (document_ && !primary.empty())
        for (const auto& c : document_->network.connectors) if (c.id == primary) return &c;
    return nullptr;
}
const std::vector<Point>* EditorCanvas::selectedGeometry() const {
    if (const auto* link = selectedLink()) return &link->geometry;
    if (const auto* connector = selectedConnector()) return &connector->geometry;
    return nullptr;
}
void EditorCanvas::setDocument(const ProjectDocument* d) {
    document_ = d; cancel();
    // Drop ids the new document no longer has, rather than clearing an otherwise valid selection.
    std::erase_if(selection_, [&](const auto& id) {
        const auto level = objectLevel(id);
        return !level || !levelVisible(*level);
    });
    redraw();
}
void EditorCanvas::setTool(Tool tool) {
    cancel(); tool_ = tool; setCursor(tool == Tool::select ? Qt::ArrowCursor : Qt::CrossCursor); redraw();
}
std::vector<Point> EditorCanvas::handleGeometry() const {
    const auto* geometry=selectedGeometry();
    if(!document_ || !geometry)return {};
    const auto& points=preview_.empty()?*geometry:preview_;
    try {
        if(const auto* link=selectedLink()) {
            auto preview=*link;preview.geometry=points;
            return linkCentreline(preview,document_->network.drivingSide);
        }
        if(const auto* connector=selectedConnector()) {
            auto preview=*connector;preview.geometry=points;
            if(points.size()!=connector->geometry.size())preview.laneBlend.clear();
            return connectorCentreline(document_->network,preview);
        }
    } catch(const std::exception&) { /* An invalid draft still needs draggable grips. */ }
    return points;
}
void EditorCanvas::cancel() {
    copyPick_.clear();copyArmed_=copyDragging_=false;copyOffset_={};
    groupDrag_=groupDragging_=false;groupOffset_={};
    rotationPivot_.reset();rotationDegrees_=0;rotationDragging_=false;
    endpointDrag_.reset();endpointDraft_.reset();handleOffset_={};
    creating_=false;gestureFrom_.reset();rangeCorner_=0;laneResize_.reset();previewLinkCount_=0;
    draft_.clear(); preview_.clear(); original_.clear(); vertex_ = -1; band_.reset();
    connectorFrom_.reset(); connectorHover_.reset(); dragging_ = false; panning_ = false;
    if (connectorDraftChanged) connectorDraftChanged();
    redraw();
}
Point EditorCanvas::world(QPoint position, bool snapped) const {
    const auto p = mapToScene(position); Point result{p.x(), p.y()};
    if (snapped && snap && grid > 0) { result.x = std::round(result.x/grid)*grid; result.y = std::round(result.y/grid)*grid; }
    return result;
}
std::pair<std::string, double> EditorCanvas::hit(Point p,bool connectors) const {
    const auto hits=hitObjects(p,connectors);
    return hits.empty()?std::pair<std::string,double>{}:hits.front();
}
void EditorCanvas::redraw() {
    runItems_.clear();scene_.clear();
    if (!document_) return;
    const auto& bg=document_->background;
    if (backgroundVisible_ && !bg.pngBase64->empty()) {
        if (cachedImage_ != bg.pngBase64) {
            image_.loadFromData(QByteArray::fromBase64(QByteArray::fromStdString(*bg.pngBase64)), "PNG");
            cachedImage_ = bg.pngBase64;
        }
        auto* image=scene_.addPixmap(image_);
        const double a=bg.rotation*std::numbers::pi/180, s=bg.metresPerPixel;
        image->setTransform(QTransform(std::cos(a)*s,std::sin(a)*s,std::sin(a)*s,-std::cos(a)*s,bg.x,bg.y));
        image->setOpacity(bg.opacity); image->setZValue(-200000);
    }
    const auto primary = selected();
    for (auto link : document_->network.links) {
        if(!levelVisible(link.level))continue;
        const auto& appearance=style(link.displayType);const double z=link.level*100.;
        const bool chosen=isSelected(link.id);
        if (link.id==primary && !preview_.empty()) link.geometry=preview_;
        if(link.id==primary && laneResize_ && (laneResize_->kind==4 || laneResize_->kind==8)) {
            const bool leading=laneResize_->kind==8;auto lanes=link.lanes;
            const double width=(leading?lanes.front():lanes.back()).width;
            while(static_cast<int>(lanes.size())<previewLinkCount_)
                lanes.insert(leading?lanes.begin():lanes.end(),{"preview-"+std::to_string(lanes.size()),width});
            while(static_cast<int>(lanes.size())>previewLinkCount_)lanes.erase(leading?lanes.begin():lanes.end()-1);
            replaceLaneBundle(link,std::move(lanes),leading);
        }
        const QColor colour=link.id==primary?QColor("#167b98"):chosen?QColor("#3fa3bf"):QColor(QString::fromStdString(appearance.linkColor));
        const auto road=linkCentreline(link,document_->network.drivingSide);
        if(!std::isfinite(polylineLength(road)) || polylineLength(road)<=0) {
            // Invalid transient geometry must remain a cancellable gesture, not an exception
            // from pointAlong while painting. The release command will reject it atomically.
            QPen invalid(QColor("#ef4444"),2,Qt::DashLine);invalid.setCosmetic(true);
            scene_.addPath(path(link.geometry),invalid)->setZValue(z+5);continue;
        }
        // Drawn lines only: an edge offset round a bend tighter than the lane can loop back on
        // itself, which fills as a hole and reads as a tear in the road.
        const auto left=trimSelfIntersections(laneBoundaryGeometry(link,0,document_->network.drivingSide));
        const auto right=trimSelfIntersections(laneBoundaryGeometry(link,link.lanes.size(),document_->network.drivingSide));
        auto surface=path(left);for(auto it=right.rbegin();it!=right.rend();++it)surface.lineTo(q(*it));surface.closeSubpath();
        scene_.addPath(surface,QPen(Qt::NoPen),QBrush(colour))->setZValue(z+1);
        for(const auto& marking:markingStrokes(linkMarkings(link,document_->network.drivingSide))) {
            QPen pen(QColor(QString::fromStdString(appearance.laneColor)),1,marking.type==MarkingType::solid?Qt::SolidLine:Qt::DashLine);pen.setCosmetic(true);
            auto* mark=scene_.addPath(path(marking.geometry),pen);
            mark->setZValue(z+2);mark->setData(0,QStringLiteral("road-marking"));mark->setData(1,QString::fromStdString(link.id));
        }
        // Direction triangle follows the centreline. Constant pixel size makes it readable when zoomed out.
        if (polylineLength(link.geometry) <= 0) continue;
        const auto mid=pointAlong(road,polylineLength(road)/2);
        const auto ahead=pointAlong(road,polylineLength(road)/2+0.05);
        const auto angle=std::atan2(ahead.y-mid.y,ahead.x-mid.x); const double r=5/std::abs(transform().m11());
        QPolygonF arrow;
        for (double offset : {0.0,2.5,-2.5}) arrow << QPointF(mid.x+r*std::cos(angle+offset),mid.y+r*std::sin(angle+offset));
        scene_.addPolygon(arrow,QPen(Qt::NoPen),QBrush(Qt::white))->setZValue(z+3);
        // Handles belong to the primary alone; drawing them for every selected link would
        // suggest a group drag that M1.5 deliberately does not implement.
        // Grips sit on the bundle centreline, where Vissim shows them, not on the reference
        // polyline, which ends up at one edge as soon as lanes are added to a single side.
        if (link.id==primary) {
            const auto handles=linkCentreline(link,document_->network.drivingSide);
            const double radius=4/std::abs(transform().m11());
            QPen outline(QColor("#334155"),1);outline.setCosmetic(true);
            for (std::size_t i=0;i<handles.size();++i)
                scene_.addEllipse(handles[i].x-radius,handles[i].y-radius,2*radius,2*radius,outline,
                    QBrush(static_cast<int>(i)==vertex_?QColor("#ffb454"):QColor("#ffffff")))->setZValue(z+5);
        }
    }
    drawConnectors();
    drawLaneHandles();
    drawCopyPreview();drawRotationPreview();
    for(const auto& head:document_->network.signalHeads) {
        std::vector<Point> geometry;int level=0;
        if(head.connectorId.empty()) {
            for(const auto& l:document_->network.links)if(l.id==head.lane.linkId) {
                geometry=laneGeometry(l,head.lane.laneId,document_->network.drivingSide);level=l.level;
            }
        } else for(const auto& c:document_->network.connectors)for(const auto& p:connectorPaths(document_->network,c))
            if(p.id==head.connectorId){geometry=p.geometry;level=c.level;}
        if(geometry.empty() || !levelVisible(level))continue;
        const auto p=pointAlong(geometry,head.position);const double r=3/std::abs(transform().m11());
        scene_.addEllipse(p.x-r,p.y-r,2*r,2*r,QPen(Qt::darkGray),QBrush(isSelected(head.id)?QColor("#ffb454"):QColor(Qt::white)))->setZValue(level*100.+10);
    }
    if (band_) {
        QPen pen(QColor("#167b98"),1,Qt::DashLine); pen.setCosmetic(true);
        scene_.addRect(*band_,pen,QBrush(QColor(22,123,152,30)))->setZValue(200009);
    }
    if (!draft_.empty()) {
        QPen pen(QColor("#de8618"),2,Qt::DashLine); pen.setCosmetic(true);
        scene_.addPath(path(draft_),pen)->setZValue(200008);
        for (auto p:draft_) { const double r=3/std::abs(transform().m11()); scene_.addEllipse(p.x-r,p.y-r,2*r,2*r,pen)->setZValue(8); }
    }
    scene_.setSceneRect(scene_.itemsBoundingRect().adjusted(-10000,-10000,10000,10000).united(QRectF(-10000,-10000,20000,20000)));
    drawRunItems();viewport()->update();
}
void EditorCanvas::fitNetwork() {
    auto bounds=scene_.itemsBoundingRect(); if (bounds.isEmpty()) bounds=QRectF(-50,-50,100,100);
    fitInView(bounds.adjusted(-10,-10,10,10),Qt::KeepAspectRatio);
    const double scale=std::clamp(std::abs(transform().m11()),0.05,100.0);
    setTransform(QTransform::fromScale(scale,-scale)); centerOn(bounds.center()); redraw();
}
void EditorCanvas::drawBackground(QPainter* painter,const QRectF& rect) {
    painter->fillRect(rect,QColor("#f0f4f8"));
    double step=grid;
    if (step<=0) return;
    while(step*std::abs(transform().m11())<20) step*=10;
    QPen pen(QColor("#dce3eb"),1); pen.setCosmetic(true); painter->setPen(pen);
    for(double x=std::floor(rect.left()/step)*step;x<=rect.right();x+=step) painter->drawLine(QPointF(x,rect.top()),QPointF(x,rect.bottom()));
    for(double y=std::floor(rect.top()/step)*step;y<=rect.bottom();y+=step) painter->drawLine(QPointF(rect.left(),y),QPointF(rect.right(),y));
}
void EditorCanvas::wheelEvent(QWheelEvent* e) {
    const auto before=mapToScene(e->position().toPoint());
    const double old=std::abs(transform().m11()), next=std::clamp(old*std::pow(1.0015,e->angleDelta().y()),0.05,100.0);
    scale(next/old,next/old);
    const auto after=mapToScene(e->position().toPoint());
    const auto centre=mapToScene(viewport()->rect().center()); centerOn(centre+before-after); redraw(); e->accept();
}
}
