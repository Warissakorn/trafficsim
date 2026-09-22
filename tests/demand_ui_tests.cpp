#include "../src/shell/editor_window.hpp"
#include <QApplication>
#include <QStandardPaths>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFile>
#include <QGraphicsPathItem>
#include <QLabel>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
template<class T> T* item(EditorWindow& w,const char* name) {
    auto* result=w.findChild<T*>(name);require(result,"Missing control");return result;
}
void action(EditorWindow& w,const char* name) {
    auto* a=item<QAction>(w,name);require(a->isEnabled(),"Action unexpectedly disabled");
    a->trigger();QApplication::processEvents();
}
void write(const QString& path,const ProjectDocument& d) {
    QFile f(path);require(f.open(QIODevice::WriteOnly),"Fixture open failed");
    const auto bytes=QByteArray::fromStdString(documentJson(d).dump());
    require(f.write(bytes)==bytes.size(),"Fixture write failed");
}
QGraphicsItem* drawn(EditorWindow& w,const char* kind) {
    for(auto* i:w.canvas()->scene()->items())if(i->data(0).toString()==kind)return i;
    return nullptr;
}
int drawnCount(EditorWindow& w,const char* kind) {
    int n=0;for(auto* i:w.canvas()->scene()->items())if(i->data(0).toString()==kind)++n;
    return n;
}
// The middle of a lane, in scene coordinates: a click has to land on the road, and a lane's
// centre is offset from the Link's reference polyline by half the cross-section.
QPoint laneMiddle(EditorWindow& w,const std::string& linkId,const std::string& laneId) {
    const auto& n=w.history().document().network;
    for(const auto& link:n.links)if(link.id==linkId) {
        const auto g=laneGeometry(link,laneId,n.drivingSide);
        const auto p=pointAlong(g,polylineLength(g)/2);
        return w.canvas()->mapFromScene(p.x,p.y);
    }
    require(false,"Unknown lane");return {};
}
void click(EditorWindow& w,QPoint position) {
    QTest::mouseClick(w.canvas()->viewport(),Qt::LeftButton,{},position);
    QApplication::processEvents();
}
void hover(EditorWindow& w,QPoint position) {
    QTest::mouseMove(w.canvas()->viewport(),position);
    QApplication::processEvents();
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    // Recovery copies live under AppLocalDataLocation. Test mode redirects that on every
    // platform, so this binary never reads or deletes a real editor session's drafts.
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>1,"Expected data directory");
        QTemporaryDir temp;require(temp.isValid(),"Temporary directory unavailable");
        ProjectDocument d;
        const auto west=addLink(d,{{-100,0},{-10,0}},1,3.5);
        const auto east=addLink(d,{{10,0},{100,0}},1,3.5);
        const auto north=addLink(d,{{200,-40},{200,40}},1,3.5); // never reachable from the others
        const auto westLane=d.network.links[0].lanes.front().id;
        const auto eastLane=d.network.links[1].lanes.front().id;
        const auto northLane=d.network.links[2].lanes.front().id;
        const auto connector=addConnector(d,{west,westLane},{east,eastLane});
        const auto file=temp.filePath("demand.traffic.json");write(file,d);
        EditorWindow w{std::filesystem::path(argv[1])};w.show();QTest::qWait(30);
        w.openFile(file);QApplication::processEvents();
        w.canvas()->fitNetwork();QApplication::processEvents();
        require(w.history().document().network.connectors.size()==1,"Fixture lost its connector");

        auto* tool=item<QComboBox>(w,"editorTool");
        auto* hint=item<QLabel>(w,"editorToolHint");
        require(hint->text().isEmpty(),"Select tool showed a gesture hint");
        tool->setCurrentIndex(6);QApplication::processEvents();
        require(!hint->text().isEmpty(),"Route tool explained nothing");

        // The forcing: the click really does land on the lane, and the draft is what starts.
        click(w,laneMiddle(w,west,westLane));
        require(w.canvas()->routeDraft()==std::vector<std::string>({westLane}),"Click did not start a route");
        // Hovering an unreachable lane says so BEFORE the click, and the click is refused.
        hover(w,laneMiddle(w,north,northLane));
        auto* halo=drawn(w,"demand-hover");
        require(halo && halo->data(1).toString().toStdString()==northLane,"Hover halo missed the lane");
        require(!halo->data(2).toBool(),"Unreachable lane drew as reachable");
        auto* band=drawn(w,"route-band");
        require(band && !band->data(2).toBool(),"Rubber band did not warn");
        click(w,laneMiddle(w,north,northLane));
        require(w.canvas()->routeDraft()==std::vector<std::string>({westLane}),"Unreachable click was appended");
        require(drawn(w,"demand-reject-pulse"),"Refused click drew no feedback");
        require(!w.history().document().definition || w.history().document().definition->routes.empty(),
            "A refused click authored a route");

        // One click on the far side of the junction appends the whole chain, Vissim's gesture.
        hover(w,laneMiddle(w,east,eastLane));
        require(drawn(w,"demand-hover")->data(2).toBool(),"Reachable lane drew as unreachable");
        click(w,laneMiddle(w,east,eastLane));
        require(w.canvas()->routeDraft()==std::vector<std::string>({westLane,connector,eastLane}),
            "Destination click did not append the chain");
        require(drawn(w,"route-draft"),"Draft route was not drawn");
        require(drawnCount(w,"route-arrow")>0,"Draft route drew no direction");
        QTest::keyClick(w.canvas(),Qt::Key_Backspace);QApplication::processEvents();
        require(w.canvas()->routeDraft().size()==2,"Backspace did not remove the last segment");
        click(w,laneMiddle(w,east,eastLane));
        const auto revision=w.history().revision();
        QTest::keyClick(w.canvas(),Qt::Key_Return);QApplication::processEvents();
        const auto& definition=w.history().document().definition;
        require(definition && definition->routes.size()==1,"Enter did not store the route");
        require(definition->routes.front().segmentIds==std::vector<std::string>({westLane,connector,eastLane}),
            "Stored route is not the drawn one");
        require(w.canvas()->routeDraft().empty(),"Draft survived the commit");
        require(w.history().revision()!=revision,"Commit made no history entry");
        require(item<QTableWidget>(w,"editorRouteTable")->rowCount()==1,"Route table missed the drawn route");
        const auto routeId=definition->routes.front().id;
        require(w.canvas()->highlightedRoute()==routeId,"Committed route was not drawn");
        require(drawn(w,"route-overlay"),"Selected route drew nothing");
        require(drawn(w,"route-committed-pulse"),"Commit drew no feedback");

        // The animation is paint state: a test sets the phase rather than waiting for a clock.
        auto* overlay=dynamic_cast<QGraphicsPathItem*>(drawn(w,"route-overlay"));
        require(overlay,"Route overlay is not a path");
        const double first=overlay->pen().dashOffset();
        w.canvas()->setAnimationPhase(w.canvas()->animationPhase()+3);QApplication::processEvents();
        overlay=dynamic_cast<QGraphicsPathItem*>(drawn(w,"route-overlay"));
        require(overlay && overlay->pen().dashOffset()!=first,"Route dashes did not march");

        // Esc drops a draft, and so does losing the canvas: an unfinished gesture never commits.
        click(w,laneMiddle(w,west,westLane));
        require(!w.canvas()->routeDraft().empty(),"Second draft did not start");
        QTest::keyClick(w.canvas(),Qt::Key_Escape);QApplication::processEvents();
        require(w.canvas()->routeDraft().empty(),"Esc left a draft open");
        require(definition->routes.size()==1,"Cancelled draft authored a route");

        action(w,"editorUndo");
        require(!w.history().document().definition || w.history().document().definition->routes.empty(),
            "Undo did not remove the drawn route");
        action(w,"editorRedo");
        require(w.history().document().definition->routes.size()==1,"Redo lost the drawn route");

        // A vehicle input is placed on the lane its traffic enters on, and opens on that route.
        const auto routeHint=hint->text();
        tool->setCurrentIndex(7);QApplication::processEvents();
        require(!hint->text().isEmpty() && hint->text()!=routeHint,"Input tool reused the route hint");
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog,"Input dialog did not open");
            auto* route=dialog->findChild<QComboBox*>("editorInputRoute");
            require(route && route->currentText().toStdString()==routeId,"Input did not open on the drawn route");
            dialog->findChild<QDoubleSpinBox*>("editorInputVolume")->setValue(1800);
            dialog->accept();
        });
        click(w,laneMiddle(w,west,westLane));
        QApplication::processEvents();
        require(w.history().document().definition->inputs.size()==1,"Pointer placement authored no input");
        require(w.history().document().definition->inputs.front().routeId==routeId,"Input took the wrong route");
        auto* marker=drawn(w,"input-marker");
        require(marker,"Vehicle input drew no marker");
        require(drawn(w,"input-volume"),"Vehicle input drew no volume");
        // The context menu asks the canvas what was right-clicked; the marker has to answer.
        const auto centre=marker->sceneBoundingRect().center();
        const auto found=w.canvas()->demandObjectAt(w.canvas()->mapFromScene(centre));
        require(found.first=="input","Right-click on the marker found no vehicle input");
        require(found.second==w.history().document().definition->inputs.front().id,"Marker named the wrong input");

        // Save and reopen: both objects are project data, not canvas state.
        w.saveFile(file);w.openFile(file);QApplication::processEvents();
        require(w.history().document().definition->routes.size()==1 &&
                w.history().document().definition->inputs.size()==1,"Reopen lost the drawn demand");
        require(drawn(w,"input-marker"),"Reopened input drew no marker");

        // Selecting the input row draws the route it feeds.
        item<QTableWidget>(w,"editorInputTable")->selectRow(0);QApplication::processEvents();
        require(w.canvas()->highlightedRoute()==routeId,"Input row did not draw its route");

        auto* language=item<QComboBox>(w,"editorLanguage");
        language->setCurrentIndex(language->findData("th"));QApplication::processEvents();
        tool->setCurrentIndex(6);QApplication::processEvents();
        require(item<QLabel>(w,"editorToolHint")->text().contains(QString::fromUtf8("เส้นทาง")),
            "Route gesture hint was not translated");
        std::cout<<"demand ui ok\n";
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
    return 0;
}
