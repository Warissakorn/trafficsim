#include "connector_surface_fixture.hpp"
#include "../src/editor/canvas.hpp"
#include "../src/commands/history.hpp"
#include <QApplication>
#include <QGraphicsPathItem>
#include <QPainterPathStroker>
#include <algorithm>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message) { if(!ok)throw std::runtime_error(message); }
QPainterPath shape(const std::vector<Point>& ring) {
    QPainterPath p;p.moveTo(ring.front().x,ring.front().y);
    for(std::size_t i=1;i<ring.size();++i)p.lineTo(ring[i].x,ring[i].y);
    p.closeSubpath();p.setFillRule(Qt::WindingFill);return p;
}
QPainterPath verify(EditorCanvas& canvas,const ProjectDocument& doc) {
    canvas.setDocument(&doc);canvas.redraw();
    const auto expected=shape(connectorSurface(doc.network,doc.network.connectors.front()).outline);
    bool found=false;
    for(auto* item:canvas.scene()->items())if(item->data(0).toString()=="road-surface" && item->data(1).toString()=="c") {
        const auto* path=dynamic_cast<QGraphicsPathItem*>(item);
        require(path && path->path()==expected,"Paint did not use the four-point outline");found=true;
    }
    require(found,"Connector surface missing");
    // Picking uses a six-pixel halo, so include that exact public UI tolerance in the
    // independent expectation; sample both sides of the new cap, not just the old centreline.
    QPainterPathStroker stroke;stroke.setWidth(12/std::abs(canvas.transform().m11()));
    const auto halo=stroke.createStroke(expected);
    for(double x=-6;x<=4;x+=.5)for(double y=-2;y<=6;y+=.5) {
        const auto hits=canvas.hitObjects({x,y});
        const bool hit=std::any_of(hits.begin(),hits.end(),[](const auto& h){return h.first=="c";});
        require(hit==(expected.contains({x,y}) || halo.contains({x,y})),"Picking and visible cap disagree");
    }
    return expected;
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        EditorCanvas canvas;canvas.resize(1000,700);canvas.show();
        canvas.scale(12,12);canvas.centerOn(-5,0);
        History history;ProjectDocument doc;doc.network=surface_fixture::arrival(45);history.reset(doc);
        const auto before=verify(canvas,history.document());
        history.execute("test-change-angle",[](auto& d){d.network=surface_fixture::arrival(89);});
        const auto after=verify(canvas,history.document());require(before!=after,"Angle edit did not change cap");
        history.undo();require(verify(canvas,history.document())==before,"Undo left stale surface cache");
        history.redo();require(verify(canvas,history.document())==after,"Redo left stale surface cache");
        for(double angle:{90.,91.,135.}) {
            doc.network=surface_fixture::arrival(angle);verify(canvas,doc);
        }
        // Same Connector, changed Link: exercise the dependency in the cache key.
        doc.network.links.front().lanes.front().width=5;
        verify(canvas,doc);
        if(argc>1) {
            doc.network=surface_fixture::arrival(89);verify(canvas,doc);
            canvas.select("c"); // Contrast the cap against its Link in the visual evidence.
            canvas.fitInView(QRectF(-10,-10,16,18),Qt::KeepAspectRatio);
            canvas.redraw();QApplication::processEvents();
            require(canvas.grab().save(QString::fromLocal8Bit(argv[1])),"Could not save visual evidence");
        }
        std::cout<<"PASS four-point surface, picking, cache, Undo/Redo\n";return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
