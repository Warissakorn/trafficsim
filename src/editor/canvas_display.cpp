#include "canvas.hpp"
#include <QKeyEvent>
#include <QPainterPathStroker>
#include <algorithm>
#include <cmath>
namespace trafficsim {
const DisplayType& EditorCanvas::style(const std::string& id) const {
    for(const auto& type:display_.types)if(type.id==id)return type;
    for(const auto& type:display_.types)if(type.id=="default")return type;
    static const DisplayType absent{};return absent;
}
std::vector<std::pair<std::string,double>> EditorCanvas::hitObjects(Point p,bool connectors) const {
    struct Hit {std::string id;double station{},distance{};int level{},order{};};
    std::vector<Hit> hits;
    if(!document_)return {};
    const double tolerance=6/std::abs(transform().m11());
    QPainterPathStroker stroke;stroke.setWidth(tolerance*2);
    const auto contains=[&](const std::string& id) {
        const auto shape=objectShape(id);return shape.contains(QPointF(p.x,p.y)) || stroke.createStroke(shape).contains(QPointF(p.x,p.y));
    };
    const auto proximity=[&](const std::vector<Point>& geometry){
        const auto point=pointAlong(geometry,stationOfClosestPoint(geometry,p));return std::hypot(point.x-p.x,point.y-p.y);
    };
    for(const auto& l:document_->network.links)if(levelVisible(l.level)) {
        double distance=1e300;
        for(const auto& lane:l.lanes) {
            const auto gap=proximity(laneGeometry(l,lane.id,document_->network.drivingSide));
            distance=std::min(distance,gap);
        }
        if(contains(l.id))hits.push_back({l.id,stationOfClosestPoint(l.geometry,p),distance,l.level,1});
    }
    if(connectors)for(const auto& c:document_->network.connectors)if(levelVisible(c.level)) {
        double distance=proximity(c.geometry);
        for(const auto& path:connectorPaths(document_->network,c))distance=std::min(distance,proximity(path.geometry));
        if(contains(c.id))hits.push_back({c.id,stationOfClosestPoint(c.geometry,p),distance,c.level,4});
    }
    if(connectors)for(const auto& h:document_->network.signalHeads)if(const auto at=headPosition(h))
        if(levelVisible(at->second) && contains(h.id))hits.push_back({h.id,h.position,-1,at->second,10});
    std::stable_sort(hits.begin(),hits.end(),[](const auto& a,const auto& b){
        if(a.level!=b.level)return a.level>b.level;
        if(std::abs(a.distance-b.distance)>1e-7)return a.distance<b.distance;
        return a.order>b.order;
    });
    std::vector<std::pair<std::string,double>> result;for(const auto& hit:hits)result.emplace_back(hit.id,hit.station);
    return result;
}
std::optional<LaneReference> EditorCanvas::nearestLane(Point p) const {
    if(!document_)return {};
    const auto hits=hitObjects(p,false);if(hits.empty())return {};
    for(const auto& l:document_->network.links)if(l.id==hits.front().first) {
        double best=1e300;std::optional<LaneReference> result;
        for(const auto& lane:l.lanes) {
            const auto g=laneGeometry(l,lane.id,document_->network.drivingSide);
            const auto at=pointAlong(g,stationOfClosestPoint(g,p));const double distance=std::hypot(p.x-at.x,p.y-at.y);
            if(distance<best){best=distance;result=LaneReference{l.id,lane.id};}
        }
        return result;
    }
    return {};
}
void EditorCanvas::cycleOverlap() {
    const auto hits=hitObjects(lastPick_);if(hits.empty())return;
    const auto at=std::find_if(hits.begin(),hits.end(),[&](const auto& hit){return hit.first==selected();});
    const auto i=at==hits.end()?0:(static_cast<std::size_t>(std::distance(hits.begin(),at))+1)%hits.size();
    select(hits[i].first);
}
bool EditorCanvas::focusNextPrevChild(bool next) {
    if(next){cycleOverlap();return true;}
    return QGraphicsView::focusNextPrevChild(next);
}
}
