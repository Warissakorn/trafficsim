#include "../src/shell/editor_window.hpp"
#include <nlohmann/json.hpp>
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
#include <QLineEdit>
#include <QPlainTextEdit>
#include <cmath>
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
        // Two lanes each way, because the point of M1.26 is that one click routes the whole
        // carriageway rather than the lane the pointer happened to land on.
        const auto west=addLink(d,{{-100,0},{-10,0}},2,3.5);
        const auto east=addLink(d,{{10,0},{100,0}},2,3.5);
        const auto north=addLink(d,{{200,-40},{200,40}},2,3.5); // never reachable from the others
        const auto westLane=d.network.links[0].lanes.front().id;
        const auto eastLane=d.network.links[1].lanes.front().id;
        const auto northLane=d.network.links[2].lanes.front().id;
        const auto connector=addConnectorRange(d,{west,westLane},{east,eastLane},2,2);
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

        // The forcing: the click lands on a LANE of the Link, and what starts is a route on the
        // whole Link -- the lane clicked is not stored anywhere.
        click(w,laneMiddle(w,west,westLane));
        require(w.canvas()->routeDraft()==std::vector<std::string>({west}),"Click did not start a route on the Link");
        // Hovering an unreachable Link says so BEFORE the click, and the click is refused.
        hover(w,laneMiddle(w,north,northLane));
        auto* halo=drawn(w,"demand-hover");
        require(halo && halo->data(1).toString().toStdString()==north,"Hover halo missed the Link");
        require(!halo->data(2).toBool(),"Unreachable Link drew as reachable");
        auto* band=drawn(w,"route-band");
        require(band && !band->data(2).toBool(),"Rubber band did not warn");
        click(w,laneMiddle(w,north,northLane));
        require(w.canvas()->routeDraft()==std::vector<std::string>({west}),"Unreachable click was appended");
        require(drawn(w,"demand-reject-pulse"),"Refused click drew no feedback");
        require(!w.history().document().definition || w.history().document().definition->routes.empty(),
            "A refused click authored a route");

        // One click on the far side of the junction appends the whole chain, Vissim's gesture.
        hover(w,laneMiddle(w,east,eastLane));
        require(drawn(w,"demand-hover")->data(2).toBool(),"Reachable lane drew as unreachable");
        click(w,laneMiddle(w,east,eastLane));
        require(w.canvas()->routeDraft()==std::vector<std::string>({west,connector,east}),
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
        require(definition->routes.front().segmentIds==std::vector<std::string>({west,connector,east}),
            "Stored route is not the drawn one");
        // One route, both lanes: the carriageway is routed, and the compiler is what turns that
        // into one runtime route per lane.
        require(routeLaneChains(w.history().document().network,
                                definition->routes.front().segmentIds).size()==2,
                "The route did not cover every lane of the Link");
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
        require(w.history().document().definition->inputs.front().laneShares.empty(),
            "An unedited input authored lane shares nobody set");

        // M1.26.1: reopening the input on its two-lane route shows one weight field per lane,
        // defaulted to equal (1 each) because nothing has set laneShares yet.
        auto* inputTable=item<QTableWidget>(w,"editorInputTable");
        inputTable->selectRow(0);QApplication::processEvents();
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog,"Input dialog did not reopen");
            auto* shares=dialog->findChild<QWidget*>("editorInputShares");
            require(shares && shares->isVisible(),"Two-lane route showed no lane-share fields");
            auto* first=dialog->findChild<QDoubleSpinBox*>("editorInputShare0");
            auto* second=dialog->findChild<QDoubleSpinBox*>("editorInputShare1");
            require(first && second,"Lane-share fields missing for a two-lane route");
            require(first->value()==1.0 && second->value()==1.0,"Unedited shares did not default to equal");
            // Weighting the first lane twice the second: touching a field is what makes the
            // dialog write laneShares at all -- leaving them alone must not have.
            first->setValue(2.0);
            dialog->accept();
        });
        action(w,"editorEditInput");
        const auto& withShares=w.history().document().definition->inputs.front();
        require(withShares.laneShares==std::vector<double>({2.0,1.0}),"Edited weights were not stored");
        {
            const auto scenario=buildScenario(w.history().document().network,*w.history().document().definition);
            require(scenario.inputs.size()==2,"Weighted input did not expand to both lanes");
            double first=0,second=0;
            for(const auto& in:scenario.inputs) {
                if(in.id==withShares.id+"/lane-1") first=in.vehiclesPerHour;
                else if(in.id==withShares.id+"/lane-2") second=in.vehiclesPerHour;
            }
            require(std::abs(first-1200)<1e-6 && std::abs(second-600)<1e-6,
                "The 2:1 weight did not compile to a 1200/600 split of 1800");
        }
        // Reopening and leaving the fields alone must not disturb what was just set.
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog,"Input dialog did not reopen a second time");
            require(dialog->findChild<QDoubleSpinBox*>("editorInputShare0")->value()==2.0,
                "Stored weights were not shown back to the author");
            require(dialog->findChild<QDoubleSpinBox*>("editorInputShare1")->value()==1.0,
                "Stored weights were not shown back to the author");
            dialog->reject();
        });
        action(w,"editorEditInput");
        require(w.history().document().definition->inputs.front().laneShares==std::vector<double>({2.0,1.0}),
            "Cancelling a reopened dialog changed the stored weights");

        // M2.2: counts pasted as they come off a count sheet become the input's intervals.
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog,"Input dialog did not open for counts");
            auto* counts=dialog->findChild<QPlainTextEdit*>("editorInputCounts");
            require(counts && counts->toPlainText().isEmpty(),"An input with no intervals showed counts");
            dialog->findChild<QDoubleSpinBox*>("editorInputIntervalMinutes")->setValue(1);
            counts->setPlainText("30\t60\n");
            dialog->accept();
        });
        action(w,"editorEditInput");
        {
            const auto& counted=w.history().document().definition->inputs.front();
            require(counted.intervals==std::vector<VolumeInterval>({{0,60,1800},{60,120,3600}}),
                "Pasted counts did not become one-minute intervals in veh/h");
            require(counted.endTime==120 && std::abs(counted.vehiclesPerHour-2700)<1e-9,
                "The scalars were not derived from the intervals");
            require(counted.laneShares==std::vector<double>({2.0,1.0}),"Counts disturbed the lane weights");
            require(item<QTableWidget>(w,"editorInputTable")->item(0,2)->text().contains("2 intervals"),
                "The input row did not say its volume is a mean over intervals");
        }
        // Reopened, the counts read back as counts; cancelling changes nothing.
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog,"Input dialog did not reopen for counts");
            require(dialog->findChild<QPlainTextEdit*>("editorInputCounts")->toPlainText()=="30\n60",
                "Stored intervals were not shown back as counts");
            require(dialog->findChild<QDoubleSpinBox*>("editorInputIntervalMinutes")->value()==1,
                "Stored interval length was not shown back");
            dialog->reject();
        });
        const auto beforeCancel=w.history().document().definition->inputs.front();
        action(w,"editorEditInput");
        require(w.history().document().definition->inputs.front()==beforeCancel,"Cancelling changed the counts");

        // M2.3: a composition is chosen from the same list as a vehicle type.
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog,"Input dialog did not open for a composition");
            auto* type=dialog->findChild<QComboBox*>("editorInputType");
            const int at=type->findData("composition:urban-mixed");
            require(at>=0,"The composition catalog was not offered");
            require(type->currentData().toString()=="type:car","An input's own type was not preselected");
            type->setCurrentIndex(at);dialog->accept();
        });
        action(w,"editorEditInput");
        require(w.history().document().definition->inputs.front().compositionId=="urban-mixed" &&
                w.history().document().definition->inputs.front().vehicleTypeId.empty(),
                "Choosing a composition did not store it in place of the type");
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog,"Input dialog did not reopen on a composition");
            require(dialog->findChild<QComboBox*>("editorInputType")->currentData().toString()=="composition:urban-mixed",
                "A stored composition was not shown back");
            dialog->reject();
        });
        action(w,"editorEditInput");

        // M2.4: a routing decision is authored in its own tab, and an input can follow it.
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog,"Routing decision dialog did not open");
            dialog->findChild<QLineEdit*>("editorDecisionNameField")->setText("West turns");
            auto* flow=dialog->findChild<QDoubleSpinBox*>("editorDecisionFlow0");
            require(flow && flow->value()==0,"A new decision did not list the route with no flow");
            flow->setValue(3);dialog->accept();
        });
        action(w,"editorAddDecision");
        require(w.history().document().definition->routingDecisions.size()==1,"The decision was not stored");
        const auto decisionId=w.history().document().definition->routingDecisions.front().id;
        require(w.history().document().definition->routingDecisions.front().routes==
                std::vector<DecisionRoute>({{routeId,3}}),"The decision's flow was not stored");
        item<QTableWidget>(w,"editorInputTable")->selectRow(0);QApplication::processEvents();
        const auto chooseRoute=[&](const QString& data){
            QTimer::singleShot(0,[&,data]{
                auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
                require(dialog,"Input dialog did not open for a decision");
                auto* route=dialog->findChild<QComboBox*>("editorInputRoute");
                const int at=route->findData(data);require(at>=0,"Route list is missing an entry");
                route->setCurrentIndex(at);dialog->accept();
            });
            action(w,"editorEditInput");
        };
        chooseRoute("decision:"+QString::fromStdString(decisionId));
        require(w.history().document().definition->inputs.front().routingDecisionId==decisionId &&
                w.history().document().definition->inputs.front().routeId.empty(),
                "Choosing a decision did not store it in place of the route");
        chooseRoute("route:"+QString::fromStdString(routeId));
        require(w.history().document().definition->inputs.front().routeId==routeId &&
                w.history().document().definition->inputs.front().routingDecisionId.empty(),
                "Choosing a route again did not clear the decision");

        // Save and reopen: both objects are project data, not canvas state.
        w.saveFile(file);w.openFile(file);QApplication::processEvents();
        require(w.history().document().definition->routes.size()==1 &&
                w.history().document().definition->inputs.size()==1 &&
                w.history().document().definition->routingDecisions.size()==1,"Reopen lost the drawn demand");
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
