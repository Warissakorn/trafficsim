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
        if (!document_) return true;
        for (const auto& link : document_->network.links) if (link.id == id) return false;
        for (const auto& c : document_->network.connectors) if (c.id == id) return false;
        return true;
    });
    redraw();
}
void EditorCanvas::setTool(Tool tool) {
    cancel(); tool_ = tool; setCursor(tool == Tool::select ? Qt::ArrowCursor : Qt::CrossCursor); redraw();
}
void EditorCanvas::cancel() {
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
std::pair<std::string, double> EditorCanvas::hit(Point p, bool connectors) const {
    std::pair<std::string, double> found; double best = 10 / std::abs(transform().m11());
    if (!document_) return found;
    const auto check = [&](const std::string& id, const std::vector<Point>& geometry) {
        double station = 0;
        for (std::size_t i = 1; i < geometry.size(); ++i) {
            const auto a=geometry[i-1], b=geometry[i];
            const double dx=b.x-a.x, dy=b.y-a.y, len=std::hypot(dx,dy);
            if (len <= 0) continue;
            const double t=std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/(len*len),0.0,1.0);
            const double dist=std::hypot(p.x-a.x-t*dx,p.y-a.y-t*dy);
            if (dist < best) { best=dist; found={id,station+t*len}; }
            station += len;
        }
    };
    for (const auto& l : document_->network.links) check(l.id, l.geometry);
    if (connectors) for (const auto& c : document_->network.connectors) check(c.id, c.geometry);
    return found;
}
void EditorCanvas::redraw() {
    scene_.clear();
    if (!document_) return;
    const auto& bg=document_->background;
    if (!bg.pngBase64->empty()) {
        if (cachedImage_ != bg.pngBase64) {
            image_.loadFromData(QByteArray::fromBase64(QByteArray::fromStdString(*bg.pngBase64)), "PNG");
            cachedImage_ = bg.pngBase64;
        }
        auto* image=scene_.addPixmap(image_);
        const double a=bg.rotation*std::numbers::pi/180, s=bg.metresPerPixel;
        image->setTransform(QTransform(std::cos(a)*s,std::sin(a)*s,std::sin(a)*s,-std::cos(a)*s,bg.x,bg.y));
        image->setOpacity(bg.opacity); image->setZValue(-10);
    }
    const auto primary = selected();
    for (auto link : document_->network.links) {
        const bool chosen=isSelected(link.id);
        if (chosen && !preview_.empty()) link.geometry=preview_;
        for (const auto& lane : link.lanes) {
            const auto geometry=laneGeometry(link,lane.id,document_->network.drivingSide);
            const QColor colour=link.id==primary?QColor("#167b98"):chosen?QColor("#3fa3bf"):QColor("#49596d");
            auto* item=scene_.addPath(path(geometry),QPen(colour,lane.width,Qt::SolidLine,Qt::FlatCap,Qt::RoundJoin));
            item->setZValue(1);
            QPen centre(QColor("#d0dfeb"),1,Qt::DashLine); centre.setCosmetic(true);
            scene_.addPath(path(geometry),centre)->setZValue(2);
        }
        // Direction triangle follows the centreline. Constant pixel size makes it readable when zoomed out.
        if (polylineLength(link.geometry) <= 0) continue;
        const auto mid=pointAlong(link.geometry,polylineLength(link.geometry)/2);
        const auto ahead=pointAlong(link.geometry,polylineLength(link.geometry)/2+0.05);
        const auto angle=std::atan2(ahead.y-mid.y,ahead.x-mid.x); const double r=5/std::abs(transform().m11());
        QPolygonF arrow;
        for (double offset : {0.0,2.5,-2.5}) arrow << QPointF(mid.x+r*std::cos(angle+offset),mid.y+r*std::sin(angle+offset));
        scene_.addPolygon(arrow,QPen(Qt::NoPen),QBrush(Qt::white))->setZValue(3);
        // Handles belong to the primary alone; drawing them for every selected link would
        // suggest a group drag that M1.5 deliberately does not implement.
        if (link.id==primary) for (std::size_t i=0;i<link.geometry.size();++i) {
            const auto p=link.geometry[i]; const double radius=4/std::abs(transform().m11());
            scene_.addEllipse(p.x-radius,p.y-radius,2*radius,2*radius,QPen(Qt::NoPen),
                QBrush(static_cast<int>(i)==vertex_?QColor("#ffb454"):QColor("#ffffff")))->setZValue(5);
        }
    }
    drawConnectors();
    if (band_) {
        QPen pen(QColor("#167b98"),1,Qt::DashLine); pen.setCosmetic(true);
        scene_.addRect(*band_,pen,QBrush(QColor(22,123,152,30)))->setZValue(9);
    }
    if (!draft_.empty()) {
        QPen pen(QColor("#de8618"),2,Qt::DashLine); pen.setCosmetic(true);
        scene_.addPath(path(draft_),pen)->setZValue(8);
        for (auto p:draft_) { const double r=3/std::abs(transform().m11()); scene_.addEllipse(p.x-r,p.y-r,2*r,2*r,pen)->setZValue(8); }
    }
    scene_.setSceneRect(scene_.itemsBoundingRect().adjusted(-10000,-10000,10000,10000).united(QRectF(-10000,-10000,20000,20000)));
    viewport()->update();
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
