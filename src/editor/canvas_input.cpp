#include "canvas.hpp"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <cmath>

namespace trafficsim {
void EditorCanvas::mousePressEvent(QMouseEvent* e) {
    setFocus();
    if (e->button()==Qt::MiddleButton || e->button()==Qt::RightButton) { panning_=true; panStart_=e->pos(); return; }
    if (e->button()!=Qt::LeftButton) return;
    auto p=world(e->pos());
    if (tool_==Tool::connect) { pickConnector(world(e->pos(),false)); return; }
    if (tool_==Tool::draw || tool_==Tool::measure || tool_==Tool::calibrate) {
        if (tool_!=Tool::draw) p=world(e->pos(),false);
        if (draft_.empty() || draft_.back()!=p) draft_.push_back(p);
        if (tool_!=Tool::draw && draft_.size()==2) {
            const auto a=draft_[0], b=draft_[1]; draft_.clear();
            if (measured) measured(a,b,tool_==Tool::calibrate);
        }
        redraw(); return;
    }
    const auto picked=hit(world(e->pos(),false),tool_!=Tool::split);
    if (tool_==Tool::split) { if (!picked.first.empty() && splitAt) splitAt(picked.first,picked.second); return; }
    vertex_=-1;
    if (const auto* geometry=selectedGeometry()) {
        // Pick the nearest handle: dense curve points must not steal each other's drags.
        int best=12;
        for(std::size_t i=0;i<geometry->size();++i) {
            const auto screen=mapFromScene((*geometry)[i].x,(*geometry)[i].y);
            const int distance=(screen-e->pos()).manhattanLength();
            if (distance<best) { best=distance; vertex_=static_cast<int>(i); }
        }
    }
    if (vertex_<0) selected_=picked.first;
    if (const auto* geometry=selectedGeometry()) {
        // Connector endpoints are read-only. Body selection never translates attached endpoints.
        const bool locked=selectedConnector() && (vertex_<=0 || vertex_==static_cast<int>(geometry->size())-1);
        if (!locked) { original_=*geometry; preview_=original_; dragging_=true; dragStart_=p; }
    }
    redraw(); if(selectionChanged) selectionChanged();
}
void EditorCanvas::mouseMoveEvent(QMouseEvent* e) {
    if(cursorMoved) cursorMoved(world(e->pos(),false));
    if(panning_) {
        const auto delta=e->pos()-panStart_; panStart_=e->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()-delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value()-delta.y()); return;
    }
    if (tool_==Tool::connect && connectorFrom_) {
        const auto hovered=hitLaneEnd(world(e->pos(),false),false);
        if (hovered!=connectorHover_) { connectorHover_=hovered; redraw(); }
        return;
    }
    if(dragging_) {
        const auto p=world(e->pos()); preview_=original_;
        if(vertex_>=0) preview_[static_cast<std::size_t>(vertex_)]=p;
        else for(auto& point:preview_) { point.x+=p.x-dragStart_.x; point.y+=p.y-dragStart_.y; }
        redraw();
    }
}
void EditorCanvas::mouseReleaseEvent(QMouseEvent* e) {
    if(e->button()==Qt::MiddleButton || e->button()==Qt::RightButton) { panning_=false; return; }
    if(e->button()==Qt::LeftButton && dragging_) {
        dragging_=false;
        const auto geometry=preview_; preview_.clear();
        if(geometry!=original_ && editGeometry) editGeometry(selected_,geometry);
        original_.clear(); redraw();
    }
}
void EditorCanvas::mouseDoubleClickEvent(QMouseEvent* e) {
    if(e->button()!=Qt::LeftButton) return;
    if(tool_==Tool::draw) { finishDrawing(); return; }
    if(tool_!=Tool::select) return;
    dragging_=false; preview_.clear(); original_.clear();
    const auto picked=hit(world(e->pos(),false));
    if(picked.first.empty()) return;
    select(picked.first);
    const auto* selected=selectedGeometry(); if (!selected) return;
    auto geometry=*selected;
    const auto inserted=pointAlong(geometry,picked.second);
    double distance=0;
    for(std::size_t i=1;i<geometry.size();++i) {
        const double length=std::hypot(geometry[i].x-geometry[i-1].x,geometry[i].y-geometry[i-1].y);
        if(picked.second<=distance+length) {
            if (picked.second-distance>0.01 && distance+length-picked.second>0.01) {
                geometry.insert(geometry.begin()+static_cast<std::ptrdiff_t>(i),inserted);
                if(editGeometry) editGeometry(selected_,geometry);
            }
            break;
        }
        distance+=length;
    }
}
void EditorCanvas::finishDrawing() {
    if(tool_!=Tool::draw || draft_.size()<2) return;
    const auto geometry=draft_; draft_.clear();
    if(createLink) createLink(geometry);
    redraw();
}
void EditorCanvas::removeVertex() {
    if(vertex_<0 || !selectedGeometry()) return;
    auto geometry=*selectedGeometry();
    if (static_cast<std::size_t>(vertex_)>=geometry.size()) return;
    if (selectedConnector() && (vertex_==0 || vertex_==static_cast<int>(geometry.size())-1)) return;
    if(geometry.size()<=2) return;
    geometry.erase(geometry.begin()+vertex_); vertex_=-1;
    if(editGeometry) editGeometry(selected_,geometry);
}
void EditorCanvas::keyPressEvent(QKeyEvent* e) {
    if(e->key()==Qt::Key_Escape) { cancel(); return; }
    if(e->key()==Qt::Key_Return || e->key()==Qt::Key_Enter) { finishDrawing(); return; }
    if(e->key()==Qt::Key_Delete) { removeVertex(); return; }
    QGraphicsView::keyPressEvent(e);
}
}
