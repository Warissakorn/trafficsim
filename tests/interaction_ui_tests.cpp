#include "../src/shell/editor_window.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QLineEdit>
#include <QLineF>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>
#include <iostream>
#include <map>

using namespace trafficsim;
namespace {
void require(bool ok,const char* message) { if(!ok)throw std::runtime_error(message); }
QPoint pixel(EditorCanvas& c,Point p) {
    const auto at=c.mapFromScene(p.x,p.y);
    require(c.viewport()->rect().contains(at),"Gesture outside viewport");return at;
}
void click(EditorCanvas& c,Point p,Qt::KeyboardModifiers modifiers={}) {
    QTest::mouseClick(c.viewport(),Qt::LeftButton,modifiers,pixel(c,p));
}
int count(EditorCanvas& c,const char* tag) {
    int n=0;for(auto* item:c.scene()->items())if(item->data(0).toString()==tag)++n;return n;
}
bool feedback(EditorCanvas& c,const char* id,const char* state) {
    for(auto* item:c.scene()->items())if(item->data(0).toString()=="object-feedback" &&
        item->data(1).toString()==id && item->data(2).toString()==state)return true;
    return false;
}
ProjectDocument fixture(DrivingSide side=DrivingSide::left) {
    ProjectDocument d;d.network.drivingSide=side;
    d.network.links={{"a",{{-80,0},{-50,0},{-20,0}},{{"a1",3.5},{"a2",3.5}}},
                     {"b",{{20,0},{80,0}},{{"b1",3.5},{"b2",3.5}}}};
    addConnectorRange(d,{"a","a1"},{"b","b1"},2,2);
    validateDocument(d);return d;
}
void selectionWorkflow(EditorCanvas& c) {
    auto d=fixture();c.setDocument(&d);c.setTransform(QTransform::fromScale(4,-4));c.centerOn(0,0);
    int edits=0,rejections=0,clears=0;
    c.editGeometry=[&](const auto&,const auto&){++edits;};
    c.deleteRequested=[&]{++edits;};c.creationRejected=[&]{++rejections;};
    c.selectionCleared=[&]{++clears;};
    QTest::mouseMove(c.viewport(),pixel(c,{-65,0}));
    require(feedback(c,"a","hover"),"Select hover has no feedback");
    click(c,{-65,0});require(c.selected()=="a" && feedback(c,"a","selected"),"Click did not select hovered road");
    require(count(c,"geometry-point")==3 && count(c,"lane-resize")==6,"Single selection controls missing");
    click(c,{50,0},Qt::ShiftModifier);
    require(c.selection().size()==2 && count(c,"geometry-point")==0 && count(c,"lane-resize")==0,"Group selection exposes single-object grips");
    click(c,{-50,-30},Qt::ShiftModifier);require(c.selection().size()==2,"Shift-empty click cleared selection");
    click(c,{-50,-30});require(c.selection().empty(),"Empty click did not clear selection");
    c.select("a");QTest::keyClick(&c,Qt::Key_Escape);
    require(c.selection().empty(),"Idle Escape did not clear selection");
    // Escape cancels an in-flight edit; the late release cannot commit it or erase selection.
    c.select("a");const auto start=pixel(c,{-65,0}),end=pixel(c,{-65,10});
    QTest::mousePress(c.viewport(),Qt::LeftButton,{},start);
    QTest::mouseMove(c.viewport(),end);QTest::keyClick(&c,Qt::Key_Escape);
    QTest::mouseRelease(c.viewport(),Qt::LeftButton,{},end);
    require(edits==0 && c.selected()=="a","Cancellation changed geometry or selection");
    for(auto tool:{EditorCanvas::Tool::draw,EditorCanvas::Tool::split,EditorCanvas::Tool::measure,
                   EditorCanvas::Tool::calibrate,EditorCanvas::Tool::connect,EditorCanvas::Tool::route,
                   EditorCanvas::Tool::input,EditorCanvas::Tool::head,EditorCanvas::Tool::conflict,EditorCanvas::Tool::counter}) {
        c.setTool(EditorCanvas::Tool::select);c.select("a");c.setHighlightedRoute("route");c.setHighlightedConflict("area");
        c.setTool(tool);
        require(c.selection().empty() && c.highlightedRoute().empty() && c.highlightedConflict().empty(),"Mode change left a stale selection");
        c.select("a"); // table selection in another tool is visible but cannot expose edit grips
        require(count(c,"geometry-point")==0 && count(c,"connector-end")==0 && count(c,"lane-resize")==0,"Inactive tool exposed editable grips");
        QTest::keyClick(&c,Qt::Key_Delete);require(edits==0,"Authoring tool deleted a road");
        c.clearSelection();click(c,{-50,-30});c.cancel();
    }
    require(rejections==0 && clears>0,"Empty-space navigation reported a creation error");
    c.setTool(EditorCanvas::Tool::route);click(c,{-65,0});
    require(c.routeDraft()==std::vector<std::string>{"a"},"Route draft did not start");
    click(c,{-50,-30});QTest::keyClick(&c,Qt::Key_Tab);
    require(c.routeDraft()==std::vector<std::string>{"a"} && c.selection().empty(),"Empty space or Tab discarded the route draft");
    c.setTool(EditorCanvas::Tool::select);QTest::mouseMove(c.viewport(),pixel(c,{-60,0}));
    require(feedback(c,"a","hover"),"Hover fixture missing");
    QEvent leave(QEvent::Leave);QApplication::sendEvent(&c,&leave);
    require(!feedback(c,"a","hover"),"Hover remained after leaving canvas");
    c.editGeometry={};c.deleteRequested={};c.creationRejected={};c.selectionCleared={};c.setDocument(nullptr);
}
void laneTabs(EditorCanvas& c) {
    History h;int commits=0;
    c.resizeLinkRequested=[&](int lanes,bool leading){
        const auto id=c.selected();++commits;
        h.execute("resize",[&](auto& d){resizeLinkLanes(d,id,lanes,leading);});c.setDocument(&h.document());
    };
    for(auto side:{DrivingSide::left,DrivingSide::right})for(int location=0;location<3;++location)for(int kind:{4,8}) {
        h.reset(fixture(side));c.setDocument(&h.document());c.select("a");
        c.setTransform(QTransform::fromScale(4,-4));c.centerOn(0,0);c.redraw();
        const auto before=h.document();QPoint press;bool found=false;
        for(auto* item:c.scene()->items())if(item->data(0).toString()=="lane-resize" &&
            item->data(1).toInt()==kind && item->data(2).toInt()==location) {
            auto* path=dynamic_cast<QGraphicsPathItem*>(item);require(path,"Lane grip is not a path");
            const auto bounds=path->path().boundingRect();
            require(std::abs(bounds.width()*4-24)<1e-7 && std::abs(bounds.height()*4-8)<1e-7,"Lane grip is not a constant-size rectangle");
            const auto& link=before.network.links.front();
            const auto edge=laneBoundaryGeometry(link,kind==8?0:link.lanes.size(),side);
            const auto at=pointAlong(edge,matchedStation(link.geometry,edge,polylineLength(link.geometry)*location/2));
            require(std::min(std::abs(bounds.top()-at.y),std::abs(bounds.bottom()-at.y))<1e-7,"Lane grip floats away from road edge");
            // The end of the rectangular target is usable, not only a circular centre hit area.
            const auto p=bounds.center()+QPointF(9/4.,0);press=c.mapFromScene(p);found=true;break;
        }
        require(found,"Missing lane tab at start/middle/end");
        for(auto* item:c.scene()->items())require(!dynamic_cast<QGraphicsSimpleTextItem*>(item),"Lane tab contains a numeric label");
        const double sign=(side==DrivingSide::left?-1.:1.)*(kind==8?-1.:1.);
        const auto target=c.mapToScene(press)+QPointF(0,sign*3.5);
        const int old=commits;
        QTest::mousePress(c.viewport(),Qt::LeftButton,{},press);
        QTest::mouseRelease(c.viewport(),Qt::LeftButton,{},c.mapFromScene(target));
        require(commits==old+1 && h.document().network.links.front().lanes.size()==3,"Lane tab drag did not add one lane");
        h.undo();require(h.document()==before,"Lane tab resize was not one undoable edit");
    }
    // A miter moves along both axes when widened. Tabs must follow the rendered edge,
    // including during the drag, rather than jump from a translated approximation on release.
    auto curved=fixture();curved.network.connectors.clear();
    curved.network.links.front().geometry={{-80,-20},{-50,-20},{-50,10}};
    h.reset(curved);c.setDocument(&h.document());c.select("a");
    const auto tabs=[&] {
        std::map<std::pair<int,int>,QRectF> result;
        for(auto* item:c.scene()->items())if(item->data(0).toString()=="lane-resize")
            result[{item->data(1).toInt(),item->data(2).toInt()}]=dynamic_cast<QGraphicsPathItem*>(item)->path().boundingRect();
        return result;
    };
    const auto press=c.mapFromScene(tabs().at({4,1}).center()),target=press+QPoint(14,0);
    QTest::mousePress(c.viewport(),Qt::LeftButton,{},press);
    QTest::mouseMove(c.viewport(),target);const auto preview=tabs();
    QTest::mouseRelease(c.viewport(),Qt::LeftButton,{},target);
    require(h.document().network.links.front().lanes.size()==3,"Curved Link resize fixture did not grow");
    require(preview.size()==6 && preview==tabs(),"Lane tabs jumped from the preview edge on release");
    c.resizeLinkRequested={};c.setDocument(nullptr);
}
void connectorEndTabs(EditorCanvas& c) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=fixture(side);d.network.connectors.clear();
        d.network.links[1].geometry={{25,24},{55,76}};
        addConnectorRange(d,{"a","a1"},{"b","b1"},2,2);validateDocument(d);
        c.setDocument(&d);c.select(d.network.connectors.front().id);
        c.setTransform(QTransform::fromScale(4,-4));c.centerOn(0,0);c.redraw();
        const auto boundaries=connectorBoundaries(d.network,d.network.connectors.front());
        const auto surface=connectorSurface(d.network,d.network.connectors.front());
        const double halfButton=canvasStyle::laneTabLength/2/4;
        for(const int kind:{1,2,5,6}) {
            QGraphicsPathItem* tab=nullptr;
            for(auto* item:c.scene()->items())if(item->data(0).toString()=="lane-resize" && item->data(1).toInt()==kind)
                tab=dynamic_cast<QGraphicsPathItem*>(item);
            require(tab,"Connector is missing an end tab");
            const auto polygon=tab->path().toFillPolygon();require(polygon.size()>=4,"Connector end tab is not rectangular");
            const QPointF midpoint=(polygon[0]+polygon[1])/2;
            const double length=QLineF(polygon[0],polygon[1]).length();
            require(std::abs(length*4-24)<1e-7,"Connector end tab is not 24 pixels long");
            const bool source=kind==1 || kind==5;
            const auto& rail=kind>4?boundaries.front():boundaries.back();
            const double railLength=polylineLength(rail);
            const double station=stationOfClosestPoint(rail,{midpoint.x(),midpoint.y()});
            require(std::abs(station-(source?halfButton:railLength-halfButton))<1e-6,
                    "Connector end tab is centred beyond the road end");
            const double first=stationOfClosestPoint(rail,{polygon[0].x(),polygon[0].y()});
            const double second=stationOfClosestPoint(rail,{polygon[1].x(),polygon[1].y()});
            const double expectedFirst=source?0.:railLength-2*halfButton;
            const double expectedLast=source?2*halfButton:railLength;
            require(std::abs(std::min(first,second)-expectedFirst)<.05 &&
                    std::abs(std::max(first,second)-expectedLast)<.05,
                    "Connector end tab extends past the road end");
            const auto on=pointAlong(rail,station),tangent=directionAlong(rail,station,false);
            const double nearest=std::hypot(midpoint.x()-on.x,midpoint.y()-on.y);
            const double dx=polygon[1].x()-polygon[0].x(),dy=polygon[1].y()-polygon[0].y();
            const double alignment=std::abs(dx*tangent.x+dy*tangent.y)/length;
            require(nearest<1e-7,"Connector end tab is detached from its painted edge");
            require(alignment>.999,"Connector end tab does not follow its edge at the Link joint");
        }
        for(const auto* mouth:{&surface.source,&surface.target}) {
            require(mouth->has_value(),"Angled Connector is missing its four-point mouth");
            bool found=false;
            for(auto* item:c.scene()->items())if(item->data(0).toString()=="connector-mouth-edge" &&
                item->data(1).toString()==QString::fromStdString(d.network.connectors.front().id) &&
                item->data(2).toString()==(mouth==&surface.source?"source":"target")) {
                const auto* line=dynamic_cast<QGraphicsPathItem*>(item);require(line,"Mouth edge is not a painted path");
                const auto path=line->path();
                const auto p3=path.elementAt(0),p4=path.elementAt(1);
                const auto& points=mouth->value().points;
                require(QLineF(p3.x,p3.y,points[2].x,points[2].y).length()<1e-9 &&
                        QLineF(p4.x,p4.y,points[3].x,points[3].y).length()<1e-9,
                        "Connector edge is not the P3–P4 segment");
                require(line->pen().style()==Qt::SolidLine && !line->pen().isCosmetic() &&
                        std::abs(line->pen().widthF()-.10)<1e-9,"Connector mouth edge is not a solid road marking");
                found=true;
            }
            require(found,"P3–P4 is not drawn as a Connector boundary");
        }
    }
    c.setDocument(nullptr);
}
void markings(EditorCanvas& c) {
    auto d=fixture();c.setDocument(&d);
    for(double zoom:{2.,10.,40.}) {
        c.setTransform(QTransform::fromScale(zoom,-zoom));c.redraw();int marks=0;
        for(auto* item:c.scene()->items())if(item->data(0).toString()=="road-marking") {
            const auto* path=dynamic_cast<QGraphicsPathItem*>(item);require(path,"Marking path missing");
            require(!path->pen().isCosmetic() && std::abs(path->pen().widthF()-.10)<1e-9,"Lane marking is not 10 cm in world units");++marks;
        }
        require(marks>=9,"Fixture is missing Link or Connector markings");
    }
    c.setDocument(nullptr);
}
void windowWorkflow(const std::filesystem::path& data,const QString& screenshot) {
    QTemporaryDir temp;require(temp.isValid(),"Temporary directory unavailable");
    auto d=fixture();const auto route=putRoute(d,Route{"",{"a",d.network.connectors.front().id,"b"}});
    const auto path=temp.filePath("interaction.traffic.json");
    {QFile file(path);require(file.open(QIODevice::WriteOnly),"Cannot write fixture");file.write(QByteArray::fromStdString(documentJson(d).dump()));}
    EditorWindow w(data);w.resize(1400,900);w.show();QTest::qWait(30);w.openFile(path);
    auto& c=*w.canvas();c.setTransform(QTransform::fromScale(4,-4));c.centerOn(0,0);
    auto* routes=w.findChild<QTableWidget*>("editorRouteTable");require(routes && routes->rowCount()==1,"Route table fixture missing");
    routes->selectRow(0);require(c.highlightedRoute()==route,"Route was not highlighted from table");
    click(c,{-50,-30});
    require(c.selection().empty() && c.highlightedRoute().empty() && routes->selectedItems().isEmpty() && routes->currentRow()==-1,"Empty canvas left table-owned selection behind");
    auto* id=w.findChild<QLineEdit*>("editorId");require(id && id->text().isEmpty(),"Inspector retained old selection");
    auto* remove=w.findChild<QAction*>("editorDeleteSelected");require(remove && !remove->isEnabled(),"Delete stayed enabled after clearing");
    auto* tool=w.findChild<QComboBox*>("editorTool");require(tool,"Tool selector missing");
    routes->selectRow(0);tool->setCurrentIndex(8);
    require(c.highlightedRoute().empty() && routes->selectedItems().isEmpty(),"Tool change retained route selection");
    tool->setCurrentIndex(0);c.select("a");
    if(!screenshot.isEmpty())require(w.grab().save(screenshot),"Cannot save interaction preview");
    c.setFocus(); // Also exercise destruction of a visible editor with canvas focus.
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        require(argc>1,"Data directory required");EditorCanvas c;c.resize(1000,650);c.show();QTest::qWait(20);
        selectionWorkflow(c);laneTabs(c);connectorEndTabs(c);markings(c);c.hide();
        windowWorkflow(std::filesystem::path(argv[1]),argc>2?QString::fromUtf8(argv[2]):QString{});
        std::cout<<"PASS selection workflow, hover, rectangular edge tabs and 10 cm markings\n";return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
