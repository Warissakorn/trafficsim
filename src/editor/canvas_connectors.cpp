#include "canvas.hpp"
#include "canvas_style.hpp"
#include "../commands/connector_commands.hpp"
#include <QGraphicsPathItem>
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trafficsim {
namespace {
QPainterPath path(const std::vector<Point>& points) {
    QPainterPath result;
    if (!points.empty()) {
        result.moveTo(points.front().x, points.front().y);
        for (std::size_t i=1; i<points.size(); ++i) result.lineTo(points[i].x, points[i].y);
    }
    return result;
}
// The station some OTHER connector already attaches at on this lane, within `tolerance` of
// `station` -- the same pixel tolerance the lane-end snap uses. Two connectors an author means
// to meet at one corner routinely land a few centimetres apart by mouse precision alone, which
// is well under kMinSectionLength and only surfaces as UNSUPPORTED_CONNECTOR_POSITION at Run.
// Snapping here catches it at the point the mismatch is introduced, not after the fact.
std::optional<double> nearbyAttachment(const Network& network, const std::string& linkId,
                                       const std::string& laneId, double station, double tolerance) {
    for (const auto& other : network.connectors)
        for (bool dir : {true, false}) {
            const auto& ref = dir ? other.from : other.to;
            if (ref.linkId != linkId || ref.laneId != laneId) continue;
            const double existing = attachmentStation(network, ref, dir);
            if (std::abs(existing - station) < tolerance) return existing;
        }
    return {};
}
// Vissim's Snap to Points covers a Link's intermediate points as well as its two ends, so a
// Connector meeting a Link at a bend lands on the bend rather than beside it. A station is metres
// along the Link's own reference polyline, which is the coordinate its vertices already live in.
std::optional<double> nearbyVertex(const Link& link, double station, double tolerance) {
    double travelled = 0;
    // Accumulated in polylineLength's own order and with its own hypot, so the value returned is
    // the one measuring that prefix produces -- the station has to be exact, not merely close.
    for (std::size_t i = 1; i + 1 < link.geometry.size(); ++i) {
        travelled += std::hypot(link.geometry[i].x - link.geometry[i - 1].x,
                                link.geometry[i].y - link.geometry[i - 1].y);
        if (std::abs(travelled - station) < tolerance) return travelled;
    }
    return {};
}
}
std::optional<LaneReference> EditorCanvas::hitLanePosition(Point p, bool outgoing) const {
    auto result=nearestLane(p);if(!result)return {};
    for(const auto& l:document_->network.links)if(l.id==result->linkId) {
        const auto g=laneGeometry(l,result->laneId,document_->network.drivingSide);
        const double length=polylineLength(g),picked=stationOfClosestPoint(g,p);
        const double tolerance=4/std::abs(transform().m11()),reference=polylineLength(l.geometry);
        // The pick is on the lane; the attachment is stored on the link that lane belongs to.
        double station=matchedStation(g,l.geometry,picked);
        if(picked<tolerance)station=0;
        else if(length-picked<tolerance)station=reference;
        else if(const auto nearby=nearbyAttachment(document_->network,result->linkId,result->laneId,
                                                   station,tolerance))station=*nearby;
        else if(const auto vertex=nearbyVertex(l,station,tolerance))station=*vertex;
        if(station!=(outgoing?reference:0.))result->station=station;
    }
    return result;
}
std::optional<LaneReference> EditorCanvas::connectorEndpointTarget(Point p,bool leading) const {
    const auto* connector=selectedConnector();
    if(!connector)return {};
    auto ref=hitLanePosition(p,leading);
    if(!ref)return {};
    // Compare actual range centres, not integer lane indices. Even lane counts and unequal
    // lane widths put the grip between lane centres; rounding an index picks the wrong range.
    const int count=leading?connector->fromLaneCount:connector->toLaneCount;
    for(const auto& link:document_->network.links)if(link.id==ref->linkId) {
        const int lanes=static_cast<int>(link.lanes.size());
        const int width=std::min(count,lanes);double best=1e300;
        const double station=attachmentStation(document_->network,*ref,leading);
        for(int first=0;first+width<=lanes;++first) {
            const auto edge=[&](int index) {
                const auto g=laneBoundaryGeometry(link,static_cast<std::size_t>(index),document_->network.drivingSide);
                return pointAlong(g,matchedStation(link.geometry,g,station));
            };
            const auto a=edge(first),b=edge(first+width);
            const double distance=std::hypot((a.x+b.x)/2-p.x,(a.y+b.y)/2-p.y);
            if(distance<best){best=distance;ref->laneId=link.lanes[static_cast<std::size_t>(first)].id;}
        }
    }
    return ref;
}
void EditorCanvas::pickConnector(Point p) {
    const auto lane=hitLanePosition(p,!connectorFrom_);
    if (!lane) { clearSelection(false); return; }
    if (!connectorFrom_) {
        connectorFrom_=lane;
        if (connectorSourcePicked) connectorSourcePicked(*lane);
        if (connectorDraftChanged) connectorDraftChanged();
    } else {
        const auto from=*connectorFrom_;
        cancel();
        if (createConnector) createConnector(from,*lane);
    }
    redraw();
}
void EditorCanvas::drawConnectors() {
    const double radius=4/std::abs(transform().m11());
    const auto primary=selected();
    for (const auto& c : document_->network.connectors) {
        if(!levelVisible(c.level))continue;
        const double z=c.level*100.;
        const QColor colour(QString::fromStdString(style(c.displayType).connectorColor));
        auto preview=c;
        if(c.id==primary && !preview_.empty()){preview.geometry=preview_;preview.laneBlend.clear();}
        if(c.id==primary && rangeCorner_)resizeConnectorEdges(document_->network,preview,previewFromCount_,previewToCount_,rangeCorner_>4,true);
        // Preview the re-attachment the same way the command will perform it, so what the
        // pointer shows is what one release commits. An impossible drop shows the original.
        if(c.id==primary && endpointDraft_ && endpointDrag_) try {
            auto moved=preview;
            if(connectorReferenced(*document_,c))throw std::invalid_argument("EDIT_REFERENCED_CONNECTOR");
            retargetConnector(document_->network,moved,*endpointDrag_?*endpointDraft_:moved.from,
                              *endpointDrag_?moved.to:*endpointDraft_);
            preview=std::move(moved);
        } catch(const std::exception&) { /* Keep drawing the connector that still exists. */ }
        const auto& drawing=cachedSurface(preview);
        const QColor boundaryColour=canvasStyle::connectorBoundaryColor(colour);
        auto surface=path(drawing.outline);
        surface.closeSubpath();
        // A ribbon that overlaps itself on a tight turn is still road there. The even-odd
        // default punched the overlap out as a hole, which read as a tear in the surface.
        surface.setFillRule(Qt::WindingFill);
        auto* road=scene_.addPath(surface,QPen(Qt::NoPen),QBrush(colour));road->setZValue(z+4);
        road->setData(0,QStringLiteral("road-surface"));road->setData(1,QString::fromStdString(c.id));
        for(const auto& marking:markingStrokes(drawing.markings)) {
            // An outer edge is solid; an interior divider draws its own type.
            const QColor markingColour=marking.edge?boundaryColour:
                QColor(QString::fromStdString(style(c.displayType).laneColor));
            const auto pen=canvasStyle::markingPen(markingColour,marking.type);
            auto* item=scene_.addPath(path(marking.geometry),pen);item->setZValue(z+4.5);
            item->setData(0,QStringLiteral("road-marking"));item->setData(1,QString::fromStdString(c.id));
            item->setData(2,marking.edge?QStringLiteral("edge"):QStringLiteral("divider"));
        }
        const auto mouthEdge=[&](const std::optional<ConnectorMouth>& mouth,const char* end) {
            if(!mouth)return;
            auto* item=scene_.addPath(path({mouth->points[2],mouth->points[3]}),
                canvasStyle::markingPen(boundaryColour,MarkingType::solid));
            item->setZValue(z+4.5);item->setData(0,QStringLiteral("connector-mouth-edge"));
            item->setData(1,QString::fromStdString(c.id));item->setData(2,QString::fromLatin1(end));
        };
        mouthEdge(drawing.source,"source");mouthEdge(drawing.target,"target");
        // The rails run from the source end to the target end, so they carry the direction.
        std::vector<std::vector<Point>> edges;
        if(drawing.boundaries.size()>=2)edges={drawing.boundaries.front(),drawing.boundaries.back()};
        drawObjectFeedback(c.id,drawing.outline.empty()?objectShape(c.id):surface,z+4.75,edges);
        if(c.id==primary && tool_==Tool::select && selection_.size()==1)
            drawGeometryHandles(c.id,connectorGrips(document_->network,preview),true);
    }
    if (tool_!=Tool::connect) return;
    if(connectorFrom_) {
        const auto p=laneAttachment(document_->network,*connectorFrom_,true);
        scene_.addEllipse(p.x-radius,p.y-radius,2*radius,2*radius,QPen(canvasStyle::active()),QBrush(editorDesign::role(QPalette::Base)))->setZValue(200008);
    }
    if (connectorFrom_ && connectorHover_) {
        try {
            QPen pen(canvasStyle::active(),2,Qt::DashLine); pen.setCosmetic(true);
            scene_.addPath(path(connectorCurve(document_->network,*connectorFrom_,*connectorHover_)),pen)->setZValue(200007);
        } catch (const std::exception&) { /* Coincident endpoints have no default curve preview. */ }
    }
}
}
