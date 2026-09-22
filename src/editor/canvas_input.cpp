#include "canvas.hpp"
#include <QMouseEvent>
#include <QApplication>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QScrollBar>
#include <cmath>

namespace trafficsim {
void EditorCanvas::mousePressEvent(QMouseEvent* e) {
    setFocus();
    if(rotationPivot_) {if(e->button()==Qt::LeftButton)return;cancel();}
    if(creating_ && e->button()==Qt::LeftButton) {
        draft_.back()=world(e->pos());draft_.push_back(draft_.back());redraw();return;
    }
    if(e->button()==Qt::RightButton && (e->modifiers()&Qt::ControlModifier)) {
        lastPick_=world(e->pos(),false);
        if(tool_==Tool::route || tool_==Tool::input) {
            // Vissim starts a routing decision with Ctrl+right-click on the link it sits on.
            // The same press on the left button does the same thing, through demandPress.
            demandPress(e);return;
        }
        if(tool_==Tool::head) {
            const auto lane=nearestLane(lastPick_);
            if(lane && createDemandGesture)createDemandGesture(*lane,tool_);
            return;
        }
        if(tool_!=Tool::select && tool_!=Tool::draw && tool_!=Tool::connect)return;
        cancel();creationStart_=e->pos();
        gestureFrom_=hitLanePosition(lastPick_,true);
        creationTool_=(tool_==Tool::connect || gestureFrom_)?Tool::connect:Tool::draw;
        if(creationTool_==Tool::connect && !gestureFrom_) {if(creationRejected)creationRejected();return;}
        creating_=true;draft_={world(e->pos()),world(e->pos())};
        if(gestureFrom_)draft_.front()=laneAttachment(document_->network,*gestureFrom_,true);
        redraw();return;
    }
    if (e->button()==Qt::MiddleButton || e->button()==Qt::RightButton) { panning_=true; panStart_=panPress_=e->pos(); return; }
    if (e->button()!=Qt::LeftButton) return;
    auto p=world(e->pos());lastPick_=world(e->pos(),false);
    if((e->modifiers()&Qt::ControlModifier) && tool_==Tool::select) {
        const auto picked=hit(lastPick_);
        if(picked.first.empty()) {additive_=true;band_=QRectF(p.x,p.y,0,0);dragStart_=p;redraw();return;}
        copyPick_=picked.first;copyArmed_=isSelected(copyPick_);copyStart_=e->pos();dragStart_=p;
        copyDragging_=false;copyOffset_={};return;
    }
    if(demandPress(e))return;
    if(tool_==Tool::head)return;
    if(tool_==Tool::select && (e->modifiers()&Qt::AltModifier)) {startRotation(e->pos());return;}
    if(tool_==Tool::select && startLaneResize(e->pos()))return;
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
    const bool additive=(e->modifiers()&Qt::ShiftModifier)!=0;
    if (additive) {
        // Adding to a selection is never also a drag: the two gestures would fight over the press.
        if (picked.first.empty()) { additive_=true; band_=QRectF(p.x,p.y,0,0); dragStart_=p; redraw(); }
        else toggle(picked.first);
        return;
    }
    if (picked.first.empty() && vertexAt(e->pos())<0) {
        // A plain drag on empty space did nothing before, so the band displaces no gesture.
        additive_=false; band_=QRectF(p.x,p.y,0,0); dragStart_=p;
        if (!selection_.empty()) { selection_.clear(); if(selectionChanged) selectionChanged(); }
        redraw(); return;
    }
    vertex_=vertexAt(e->pos());
    if (vertex_<0 && !isSelected(picked.first)) { selection_={picked.first}; vertex_=vertexAt(e->pos()); }
    // A drag on a multi-selection moves all of it. Vertex editing is single-object -- vertexAt
    // already returns -1 for a multi-selection -- so the two gestures cannot collide.
    if (selection_.size()>1 && isSelected(picked.first)) {
        groupDrag_=true; groupDragging_=false; groupOffset_={}; dragStart_=p; dragPress_=e->pos();
        redraw(); if(selectionChanged) selectionChanged(); return;
    }
    if (const auto* geometry=selectedGeometry()) {
        const auto handles=handleGeometry();
        const bool end=selectedConnector() && vertex_>=0 && (vertex_==0 || vertex_==static_cast<int>(geometry->size())-1);
        // A connector end is not a free point: it rides a lane. Dragging it re-attaches the
        // connector instead of editing the polyline, which the attachment owns.
        if (end) { endpointDrag_=vertex_==0; endpointDraft_.reset(); dragStart_=p; dragPress_=e->pos(); }
        // A connector body CAN be translated: it keeps its own position, and dragging it off its
        // Links is how an author gets rid of one. Its two end handles are the branch above, which
        // re-attaches rather than translating.
        else {
            if (selection_.size()==1) {
                original_=*geometry; preview_=original_; dragging_=true; dragStart_=p; dragPress_=e->pos();
                handleOffset_= vertex_>=0 && static_cast<std::size_t>(vertex_)<handles.size()
                    ? Point{handles[static_cast<std::size_t>(vertex_)].x-(*geometry)[static_cast<std::size_t>(vertex_)].x,
                            handles[static_cast<std::size_t>(vertex_)].y-(*geometry)[static_cast<std::size_t>(vertex_)].y}
                    : Point{};
            }
        }
    }
    redraw(); if(selectionChanged) selectionChanged();
}
int EditorCanvas::vertexAt(QPoint position) const {
    // Pick the nearest handle: dense curve points must not steal each other's drags.
    const auto* geometry=selectedGeometry();
    if (!geometry || selection_.size()!=1) return -1;
    const auto handles=handleGeometry();
    if (handles.size()!=geometry->size()) return -1;
    int best=12, found=-1;
    for(std::size_t i=0;i<handles.size();++i) {
        const auto screen=mapFromScene(handles[i].x,handles[i].y);
        const int distance=(screen-position).manhattanLength();
        if (distance<best) { best=distance; found=static_cast<int>(i); }
    }
    return found;
}
void EditorCanvas::mouseMoveEvent(QMouseEvent* e) {
    if(cursorMoved) cursorMoved(world(e->pos(),false));
    if(rotationPivot_) {updateRotation(e->pos(),e->modifiers()&Qt::ShiftModifier);return;}
    if(creating_) {
        draft_.back()=world(e->pos());
        if(creationTool_==Tool::connect) {
            const auto target=hitLanePosition(world(e->pos(),false),false);
            if(target)draft_.back()=laneAttachment(document_->network,*target,false);
        }
        redraw();return;
    }
    if(!copyPick_.empty()) {
        copyDragging_=copyArmed_ && (e->pos()-copyStart_).manhattanLength()>=QApplication::startDragDistance();
        const auto p=world(e->pos());copyOffset_={p.x-dragStart_.x,p.y-dragStart_.y};redraw();return;
    }
    if(endpointDrag_) {
        if((e->pos()-dragPress_).manhattanLength()>=QApplication::startDragDistance())
            endpointDraft_=connectorEndpointTarget(world(e->pos(),false),*endpointDrag_);
        redraw();return;
    }
    if(laneResize_) {updateLaneResize(e->pos());return;}
    if(panning_) {
        const auto delta=e->pos()-panStart_; panStart_=e->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()-delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value()-delta.y()); return;
    }
    if(demandHover(e))return;
    if (tool_==Tool::connect && connectorFrom_) {
        const auto hovered=hitLanePosition(world(e->pos(),false),false);
        if (hovered!=connectorHover_) { connectorHover_=hovered; redraw(); }
        return;
    }
    if(groupDrag_) {
        groupDragging_=(e->pos()-dragPress_).manhattanLength()>=QApplication::startDragDistance();
        const auto p=world(e->pos());groupOffset_={p.x-dragStart_.x,p.y-dragStart_.y};redraw();return;
    }
    if(band_) { const auto p=world(e->pos(),false); band_=QRectF(QPointF(dragStart_.x,dragStart_.y),QPointF(p.x,p.y)).normalized(); redraw(); return; }
    if(dragging_) {
        if((e->pos()-dragPress_).manhattanLength()<QApplication::startDragDistance())return;
        const auto p=world(e->pos()); preview_=original_;
        if(vertex_>=0) preview_[static_cast<std::size_t>(vertex_)]={p.x-handleOffset_.x,p.y-handleOffset_.y};
        else for(auto& point:preview_) { point.x+=p.x-dragStart_.x; point.y+=p.y-dragStart_.y; }
        redraw();
    }
}
void EditorCanvas::mouseReleaseEvent(QMouseEvent* e) {
    if(e->button()==Qt::LeftButton && rotationPivot_) {
        // Native platforms may coalesce the last move; commit the release angle once.
        updateRotation(e->pos(),e->modifiers()&Qt::ShiftModifier);
        const auto pivot=*rotationPivot_;const double degrees=rotationDegrees_;
        cancel();
        if(degrees!=0 && rotateRequested)rotateRequested(pivot,degrees);
        return;
    }
    if(e->button()==Qt::RightButton && creating_) {
        // The release position is authoritative: native platforms may coalesce the final move.
        draft_.back()=world(e->pos());
        const auto target=hitLanePosition(world(e->pos(),false),false);
        if(creationTool_==Tool::connect && target)draft_.back()=laneAttachment(document_->network,*target,false);
        const auto points=draft_;const auto from=gestureFrom_;const auto mode=creationTool_;
        const bool click=(e->pos()-creationStart_).manhattanLength()<QApplication::startDragDistance();
        const auto pick=lastPick_;cancel();
        if(click && tool_==Tool::select){insertVertex(pick);return;}
        if(mode==Tool::draw && points.size()>=2 && points.front()!=points.back() && createLinkGesture)createLinkGesture(points);
        else if(mode==Tool::connect && from && target && createRangeGesture)createRangeGesture(*from,*target,points);
        else if(creationRejected)creationRejected();
        return;
    }
    if(e->button()==Qt::LeftButton && !copyPick_.empty()) {
        const auto picked=copyPick_;const bool duplicate=copyArmed_ && (e->pos()-copyStart_).manhattanLength()>=QApplication::startDragDistance();
        const auto p=world(e->pos());const Point delta{p.x-dragStart_.x,p.y-dragStart_.y};
        copyPick_.clear();copyArmed_=copyDragging_=false;copyOffset_={};
        if(duplicate) {if((delta.x!=0 || delta.y!=0) && duplicateRequested)duplicateRequested(delta);}
        else {auto ids=selection_;if(!isSelected(picked))ids.push_back(picked);setSelection(std::move(ids));}
        redraw();return;
    }
    if(e->button()==Qt::LeftButton && groupDrag_) {
        // The release position is authoritative here too, and a click that never became a drag
        // leaves the selection exactly as it was.
        const bool moved=(e->pos()-dragPress_).manhattanLength()>=QApplication::startDragDistance();
        const auto p=world(e->pos());
        const Point delta{p.x-dragStart_.x,p.y-dragStart_.y};
        groupDrag_=groupDragging_=false;groupOffset_={};
        if(moved && (delta.x!=0 || delta.y!=0) && translateRequested)translateRequested(delta);
        redraw();return;
    }
    if(e->button()==Qt::LeftButton && endpointDrag_) {
        const bool leading=*endpointDrag_;
        // The release position is authoritative, exactly as it is for creation gestures.
        std::optional<LaneReference> target;
        if((e->pos()-dragPress_).manhattanLength()>=QApplication::startDragDistance())
            target=connectorEndpointTarget(world(e->pos(),false),leading);
        endpointDrag_.reset();endpointDraft_.reset();
        if(target && moveConnectorEndpoint)moveConnectorEndpoint(leading,*target);
        redraw();return;
    }
    if(e->button()==Qt::LeftButton && laneResize_) {
        updateLaneResize(e->pos());
        const int kind=laneResize_->kind, from=previewFromCount_,to=previewToCount_,count=previewLinkCount_;
        laneResize_.reset();rangeCorner_=0;
        if(kind==4 || kind==8){if(resizeLinkRequested)resizeLinkRequested(count,kind==8);}
        else if(resizeRangeRequested)resizeRangeRequested(from,to,kind>4);
        redraw();return;
    }
    if(e->button()==Qt::MiddleButton || e->button()==Qt::RightButton) {
        const bool click=(e->pos()-panPress_).manhattanLength()<QApplication::startDragDistance();
        panning_=false;
        // A right-DRAG pans, which is the gesture this editor already spends the button on.
        // A right-CLICK spent nothing until now, so a context menu displaces no gesture.
        if(click && e->button()==Qt::RightButton && !(e->modifiers()&Qt::ControlModifier) && contextMenuRequested)
            contextMenuRequested(e->pos());
        return;
    }
    if(e->button()==Qt::LeftButton && band_) {
        const auto p=world(e->pos(),false);
        const auto box=QRectF(QPointF(dragStart_.x,dragStart_.y),QPointF(p.x,p.y)).normalized();
        const bool additive=additive_; band_.reset(); additive_=false;
        auto found=inRectangle({box.left(),box.top()},{box.right(),box.bottom()});
        if (additive) { auto merged=selection_; for(auto& id:found) merged.push_back(std::move(id)); found=std::move(merged); }
        setSelection(std::move(found)); return;
    }
    if(e->button()==Qt::LeftButton && dragging_) {
        if((e->pos()-dragPress_).manhattanLength()<QApplication::startDragDistance()) {dragging_=false;preview_.clear();original_.clear();redraw();return;}
        const auto p=world(e->pos());preview_=original_;
        if(vertex_>=0)preview_[static_cast<std::size_t>(vertex_)]={p.x-handleOffset_.x,p.y-handleOffset_.y};
        else for(auto& point:preview_){point.x+=p.x-dragStart_.x;point.y+=p.y-dragStart_.y;}
        dragging_=false;
        const auto geometry=preview_; preview_.clear();
        if(geometry!=original_ && editGeometry) editGeometry(selected(),geometry);
        original_.clear(); redraw();
    }
}
void EditorCanvas::mouseDoubleClickEvent(QMouseEvent* e) {
    if(e->button()!=Qt::LeftButton) return;
    if(e->modifiers()&Qt::AltModifier)return;
    if(tool_==Tool::draw) { finishDrawing(); return; }
    if(tool_==Tool::route) { commitRouteDraft(); return; }
    if(tool_!=Tool::select) return;
    cancel();
    insertVertex(world(e->pos(),false));
}
void EditorCanvas::insertVertex(Point p) {
    const auto picked=hit(p);
    if(picked.first.empty()) return;
    select(picked.first);
    const auto* current=selectedGeometry(); if (!current) return;
    auto geometry=*current;
    const auto inserted=pointAlong(geometry,picked.second);
    double distance=0;
    for(std::size_t i=1;i<geometry.size();++i) {
        const double length=std::hypot(geometry[i].x-geometry[i-1].x,geometry[i].y-geometry[i-1].y);
        if(picked.second<=distance+length) {
            if (picked.second-distance>0.01 && distance+length-picked.second>0.01) {
                geometry.insert(geometry.begin()+static_cast<std::ptrdiff_t>(i),inserted);
                if(editGeometry) editGeometry(selected(),geometry);
            }
            break;
        }
        distance+=length;
    }
}
void EditorCanvas::finishDrawing() {
    if(creating_ || tool_!=Tool::draw || draft_.size()<2) return;
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
    if(editGeometry) editGeometry(selected(),geometry);
}
void EditorCanvas::keyPressEvent(QKeyEvent* e) {
    if(e->key()==Qt::Key_Escape) { if(stopRequested)stopRequested(); cancel(); return; }
    if(e->key()==Qt::Key_Return || e->key()==Qt::Key_Enter) {
        if(!routeDraft_.empty()) {commitRouteDraft();return;}
        finishDrawing(); return;
    }
    // Only while a route draft is open: Backspace belongs to the view otherwise.
    if(e->key()==Qt::Key_Backspace && !routeDraft_.empty()) { dropLastRouteSegment(); return; }
    if(e->key()==Qt::Key_Tab) {cycleOverlap();return;}
    if(e->key()==Qt::Key_Delete) {if(e->modifiers()&Qt::ControlModifier)removeVertex();else if(deleteRequested)deleteRequested();return;}
    const bool arrow = e->key()==Qt::Key_Left || e->key()==Qt::Key_Right ||
                       e->key()==Qt::Key_Up || e->key()==Qt::Key_Down;
    if (arrow && tool_==Tool::select &&
        !(e->modifiers() & (Qt::ControlModifier|Qt::AltModifier|Qt::MetaModifier))) {
        // Never pan or commit a second edit underneath an unfinished mouse gesture.
        if (mouseGestureActive()) { e->accept(); return; }
        bool movable = false;
        if (document_) {
            for (const auto& link : document_->network.links) movable = movable || isSelected(link.id);
            for (const auto& connector : document_->network.connectors) movable = movable || isSelected(connector.id);
        }
        if (movable && translateRequested) {
            double step = snap && std::isfinite(grid) && grid>0 ? grid : 1.;
            if (e->modifiers() & Qt::ShiftModifier) step *= 10;
            Point delta{};
            if (e->key()==Qt::Key_Left) delta.x=-step;
            if (e->key()==Qt::Key_Right) delta.x=step;
            if (e->key()==Qt::Key_Up) delta.y=step;
            if (e->key()==Qt::Key_Down) delta.y=-step;
            cancel();
            translateRequested(delta);
            e->accept(); return;
        }
    }
    QGraphicsView::keyPressEvent(e);
}
bool EditorCanvas::mouseGestureActive() const {
    return creating_ || !routeDraft_.empty() || !copyPick_.empty() || groupDrag_ || rotationPivot_ || endpointDrag_ ||
           laneResize_ || panning_ || band_ || dragging_;
}
void EditorCanvas::focusOutEvent(QFocusEvent* e) {
    if (mouseGestureActive()) cancel();
    QGraphicsView::focusOutEvent(e);
}
}
