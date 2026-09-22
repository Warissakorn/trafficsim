#include "../src/editor/canvas.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/display.hpp"
#include <QApplication>
#include <QGraphicsPathItem>
#include <QMouseEvent>
#include <QTest>
#include <QPainter>
#include <QPainterPathStroker>
#include <numbers>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
ProjectDocument roads(DrivingSide side) {
    ProjectDocument d;d.network.drivingSide=side;
    d.network.links={{"a",{{-90,0},{-30,0}},{{"a1",3},{"a2",7},{"a3",2},{"a4",5}}},
        {"b",{{0,20},{90,20}},{{"b1",3},{"b2",7},{"b3",2},{"b4",5}}}};
    addConnectorRange(d,{"a","a1"},{"b","b1",40},2,2);
    require(validateNetwork(d.network).empty(),"Invalid fixture");return d;
}
QPoint pixel(EditorCanvas& c,Point p){const auto q=c.mapFromScene(p.x,p.y);require(c.viewport()->rect().contains(q),"Pick outside viewport");return q;}
void press(EditorCanvas& c,Point p){QTest::mousePress(c.viewport(),Qt::LeftButton,{},pixel(c,p));}
void release(EditorCanvas& c,Point p){QTest::mouseRelease(c.viewport(),Qt::LeftButton,{},pixel(c,p));QApplication::processEvents();}
void move(EditorCanvas& c,Point p){
    const QPointF pos=pixel(c,p);
    QMouseEvent event(QEvent::MouseMove,pos,c.viewport()->mapToGlobal(pos.toPoint()),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(c.viewport(),&event);
}
Point grip(EditorCanvas& c,bool source) {
    for(auto* item:c.scene()->items())if(item->data(0).toString()=="connector-end" && item->data(2).toBool()==source) {
        const auto p=item->sceneBoundingRect().center();return {p.x(),p.y()};
    }
    throw std::runtime_error("Missing endpoint grip");
}
QPainterPath surface(EditorCanvas& c,const std::string& id) {
    for(auto* item:c.scene()->items())if(item->data(0).toString()=="road-surface" && item->data(1).toString()==QString::fromStdString(id))
        if(auto* path=qgraphicsitem_cast<QGraphicsPathItem*>(item))return path->path();
    throw std::runtime_error("Missing road surface");
}
Point rangeCentre(const ProjectDocument& d,int first,double station) {
    const auto& l=d.network.links[1];const auto edge=[&](int i){
        const auto g=laneBoundaryGeometry(l,static_cast<std::size_t>(i),d.network.drivingSide);
        return pointAlong(g,matchedStation(l.geometry,g,station));
    };
    const auto a=edge(first),b=edge(first+2);return {(a.x+b.x)/2,(a.y+b.y)/2};
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        EditorCanvas c;c.resize(1100,700);c.snap=false;c.show();QTest::qWait(20);
        c.setDisplayCatalog(loadDisplayCatalog(std::filesystem::path(TRAFFICSIM_SOURCE_DIR)/"data"));
        History h;std::string error;
        const auto refresh=[&]{c.setDocument(&h.document());};
        c.moveConnectorEndpoint=[&](bool source,LaneReference ref){
            const auto old=*c.selectedConnector();
            try {h.execute("endpoint",[&](auto& d){changeConnectorEndpoints(d,old.id,source?ref:old.from,source?old.to:ref);});}
            catch(const std::exception& e){error=e.what();}refresh();
        };
        c.translateRequested=[&](Point delta){const auto ids=c.selection();h.execute("move",[&](auto& d){translateObjects(d,ids,delta);});refresh();};
        c.resizeRangeRequested=[&](int from,int to,bool leading){const auto id=c.selected();
            h.execute("range",[&](auto& d){changeConnectorRange(d,id,from,to,leading);});refresh();};
        c.editGeometry=[&](const std::string& id,const std::vector<Point>& g){
            try {h.execute("geometry",[&](auto& d){if(c.selectedConnector())changeConnectorGeometry(d,id,g);else changeGeometry(d,id,g);});}
            catch(const std::exception& e){error=e.what();}refresh();
        };
        for(auto side:{DrivingSide::left,DrivingSide::right}) {
            h.reset(roads(side));refresh();c.setTransform(QTransform::fromScale(5,-5));c.centerOn(0,0);
            const auto id=h.document().network.connectors.front().id;
            c.select(id);
            // Two lanes of unequal widths: a centre-based drag must not jump one lane.
            for(int first:{0,1,2,0}) {
                const auto p=rangeCentre(h.document(),first,60);
                press(c,grip(c,false));move(c,p);
                const auto preview=surface(c,id);
                release(c,p);
                require(error.empty(),"Valid endpoint rejected");
                require(h.document().network.connectors.front().to.laneId=="b"+std::to_string(first+1),"Grip picked wrong lane range");
                require(surface(c,id)==preview,"Preview differs from committed endpoint geometry");
                require(validateNetwork(h.document().network).empty(),"Retarget invalidated network");
            }
            const auto before=h.document();
            for(int cancel:{0,1,2}) {
                c.select(id);press(c,grip(c,false));move(c,rangeCentre(h.document(),2,20));
                if(cancel==0)QTest::keyClick(&c,Qt::Key_Escape);
                else if(cancel==1)c.setTool(EditorCanvas::Tool::draw);
                else c.setSelection({"a"});
                release(c,rangeCentre(h.document(),2,20));
                require(h.document()==before,"Cancelled drag committed");c.setTool(EditorCanvas::Tool::select);
            }
            c.select(id);press(c,grip(c,false));release(c,{0,-40});
            require(h.document()==before,"Off-road endpoint drop changed document");
            // Coalesced mouse movement: both group drag and rubber band use release position.
            c.setSelection({"a","b"});press(c,{-60,0});release(c,{-50,10});
            require(h.document()!=before,"Release-only group drag did nothing");
            h.undo();refresh();require(h.document()==before,"Group drag did not undo in one step");
            c.setSelection({});press(c,{-95,-15});release(c,{-25,15});
            require(c.isSelected("a"),"Release-only rectangle omitted road");
            // A collapsed draft is legal to preview, illegal to commit, and cannot destroy Undo.
            c.select("a");const auto link=h.document().network.links.front();
            error.clear();press(c,link.geometry.back());move(c,link.geometry.front());release(c,link.geometry.front());
            require(error=="INVALID_GEOMETRY","Collapsed Link was not rejected");
            require(h.document()==before,"Invalid geometry mutated document");error.clear();
        }
        // The reference stays nonzero while its offset centreline collapses during a drag.
        // This must preview without throwing, then reject the release as one unchanged draft.
        // A 3:4:5 outgoing leg gives an exactly representable miter and device-pixel grip;
        // an irrational miter would miss the zero-length forcing by mouse rounding.
        ProjectDocument corner;corner.network.links={{"a",{{0,0},{.25,0},{1,1}},{{"a1",3.5}}}};
        corner.network.links.front().laneOffset=1;h.reset(corner);refresh();c.select("a");
        c.setTransform(QTransform::fromScale(40,-40));c.centerOn(0,0);
        const auto middle=linkCentreline(corner.network.links.front(),DrivingSide::left)[1];
        require(middle==Point{-.25,1},"Offset-collapse fixture miter is not exact");
        press(c,middle);move(c,{middle.x+.75,middle.y});release(c,{middle.x+.75,middle.y});
        require(error.find("INVALID_GEOMETRY")!=std::string::npos,"Collapsed offset draft was not rejected");
        require(h.document()==corner,"Collapsed offset draft changed history");error.clear();
        c.setTransform(QTransform::fromScale(5,-5));c.centerOn(0,0);
        // The wide end of a merge may exceed the narrow road's capacity. Pulling its body
        // tab outward must not silently contract BOTH ends to that smaller capacity.
        auto tapered=roads(DrivingSide::left);auto& merge=tapered.network.connectors.front();
        tapered.network.links[1].lanes.resize(1);merge.toLaneCount=1;merge.fromLaneCount=3;
        anchorConnectorEnds(tapered.network,merge);h.reset(tapered);refresh();c.select(merge.id);
        require(merge.fromLaneCount>static_cast<int>(tapered.network.links[1].lanes.size()),"Merge fixture not capacity limited");
        Point tab{};bool found=false;
        for(auto* item:c.scene()->items())if(item->data(0).toString()=="lane-resize" && item->data(1).toInt()==3) {
            const auto p=item->sceneBoundingRect().center();tab={p.x(),p.y()};found=true;break;
        }
        require(found,"Missing merge body tab");
        const auto path=connectorPaths(tapered.network,merge).back().geometry;
        const auto tangent=directionAlong(path,polylineLength(path)/2,false);
        press(c,tab);release(c,{tab.x+10*tangent.y,tab.y-10*tangent.x});
        require(h.document()==tapered,"Outward body resize narrowed the merge");
        // Exercise the rendered pavement, not only finite boundary coordinates. Trimming a
        // closed rail ring can silently erase a whole lobe, including its driving path.
        QImage sheet(1200,1600,QImage::Format_ARGB32_Premultiplied);sheet.fill(Qt::white);
        QPainter painter(&sheet);int panel=0;
        for(auto side:{DrivingSide::left,DrivingSide::right})for(int mode=0;mode<6;++mode) {
            ProjectDocument d;d.network.drivingSide=side;
            const int degrees=mode<3?mode*90:0;
            const double angle=degrees*std::numbers::pi/180;
            d.network.links={{"a",{{-60,-30},{-30,-30}},{{"a1",3.5}}},
                {"b",{{-60*std::cos(angle),-60*std::sin(angle)},{60*std::cos(angle),60*std::sin(angle)}},{{"b1",3.5}}}};
            const auto id=addConnector(d,{"a","a1"},{"b","b1",60});
            if(mode>=3) {
                auto& connector=d.network.connectors.front();const auto end=connector.geometry.back();
                const double arrival=(86+mode)*std::numbers::pi/180;
                connector.geometry={connector.geometry.front(),{end.x-30*std::cos(arrival),end.y-30*std::sin(arrival)},
                    {end.x-15*std::cos(arrival),end.y-15*std::sin(arrival)},end};
            }
            require(validateNetwork(d.network).empty(),"Invalid visual fixture");h.reset(d);refresh();c.select({});
            const auto pavement=surface(c,id);QPainterPathStroker margin;margin.setWidth(.02);
            const auto covered=pavement.united(margin.createStroke(pavement));
            for(const auto& path:connectorPaths(d.network,d.network.connectors.front()))for(double t:{.1,.3,.5,.7,.9}) {
                const auto p=pointAlong(path.geometry,polylineLength(path.geometry)*t);
                require(covered.contains(QPointF(p.x,p.y)),"Rendered pavement lost its driving path");
            }
            if(argc>1) {
                c.fitNetwork();QApplication::processEvents();
                const int x=(panel%3)*400,y=(panel/3)*400;
                painter.drawText(x+10,y+20,QString("%1 / %2 %3 degrees").arg(side==DrivingSide::left?"Left driving":"Right driving")
                    .arg(mode<3?"generated":"authored arrival").arg(mode<3?degrees:86+mode));
                painter.drawPixmap(QRect(x,y+30,400,360),c.grab());++panel;
            }
        }
        painter.end();
        if(argc>1)require(sheet.save(QString::fromUtf8(argv[1])),"Screenshot failed");
        // The canvas caches each Connector's drawn geometry, keyed by the Connector, the two
        // Links it names and the driving side -- the only inputs connectorBoundaries reads. The
        // dangerous case is the one where the Connector itself does not change: widen a lane of
        // the Link it leaves and the mouth moves, so a cache that keyed on the Connector alone
        // would keep drawing the old road. Force it, check the forcing worked, then assert.
        {
            auto d=roads(DrivingSide::left);h.reset(d);refresh();
            const auto id=h.document().network.connectors.front().id;
            const auto before=surface(c,id);
            h.execute("lanes",[&](auto& doc){changeLanes(doc,"a",{3,9,4,6});});refresh();
            require(h.document().network.links.front().lanes[1].width==9,"Lane widths did not change");
            require(h.document().network.connectors.front()==d.network.connectors.front(),
                    "The Connector itself changed, so this does not test the cache");
            require(surface(c,id)!=before,"A Link edit left the cached connector surface stale");
            const auto widened=surface(c,id);
            h.execute("side",[&](auto& doc){changeDrivingSide(doc,DrivingSide::right);});refresh();
            require(surface(c,id)!=widened,"A driving-side change left the cached surface stale");
            // Deleting the Connector must not leave its entry to be drawn again.
            h.execute("delete",[&](auto& doc){deleteObjects(doc,{id});});refresh();
            bool drawn=false;
            for(auto* item:c.scene()->items())if(item->data(0).toString()=="road-surface" &&
                item->data(1).toString()==QString::fromStdString(id))drawn=true;
            require(!drawn,"A deleted connector was drawn from the cache");
        }
        std::cout<<"Endpoint range picking, preview/commit equality, cancellation, coalesced release and invalid drafts passed\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
