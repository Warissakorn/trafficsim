#include "../src/shell/editor_window.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QFile>
#include <QGraphicsPathItem>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QTest>
#include <nlohmann/json.hpp>
#include <cmath>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool yes,const char* message){if(!yes)throw std::runtime_error(message);}
void click(EditorCanvas* c,Point p){QTest::mouseClick(c->viewport(),Qt::LeftButton,{},c->mapFromScene(p.x,p.y));QApplication::processEvents();}
void hover(EditorCanvas* c,Point p){QTest::mouseMove(c->viewport(),c->mapFromScene(p.x,p.y));QApplication::processEvents();}
void undo(EditorWindow& w){auto* a=w.findChild<QAction*>("editorUndo");require(a,"No Undo action");a->trigger();QApplication::processEvents();}
const RoutingDecision& decision(EditorWindow& w){return w.history().document().definition->routingDecisions.front();}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>1,"Expected data directory");QTemporaryDir tmp;require(tmp.isValid(),"No temporary directory");
        ProjectDocument d;d.definition=AuthoringDefinition{};
        const auto a=addLink(d,{{0,0},{100,0}},1,3.5);
        const auto b=addLink(d,{{120,0},{220,0}},1,3.5);
        const auto c=addLink(d,{{120,50},{220,50}},1,3.5);
        addConnector(d,{a,d.network.links[0].lanes[0].id},{b,d.network.links[1].lanes[0].id});
        addConnector(d,{a,d.network.links[0].lanes[0].id},{c,d.network.links[2].lanes[0].id});
        const auto file=tmp.filePath("stations.traffic.json");QFile out(file);require(out.open(QIODevice::WriteOnly),"Cannot write fixture");
        out.write(QByteArray::fromStdString(documentJson(d).dump()));out.close();
        EditorWindow w{std::filesystem::path(argv[1])};w.show();w.openFile(file);QApplication::processEvents();
        auto* canvas=w.canvas();canvas->fitNetwork();canvas->snap=false;
        w.findChild<QComboBox*>("editorTool")->setCurrentIndex(6);
        const auto before=w.history().revision();click(canvas,{40,0});
        require(canvas->routeStartPosition().has_value(),"Click did not record its station");
        const auto at=*canvas->routeStartPosition();require(std::abs(at-40)<1,"Click snapped to road start");
        hover(canvas,{170,0});require(w.history().revision()==before,"Hover wrote to History");click(canvas,{170,0});
        require(decision(w).position==at,"Committed decision lost the click station");
        require(decision(w).routes.size()==1,"Route was not linked to its decision");
        bool overlay=false;for(auto* item:canvas->scene()->items())if(item->data(0).toString()=="route-overlay") {
            auto* path=dynamic_cast<QGraphicsPathItem*>(item);require(path,"Route overlay has no road shape");
            require(!path->path().contains(QPointF(10,0)),"Route tint extends upstream of recognition");overlay=true;
        }
        require(overlay,"Committed Route has no overlay");
        require(documentJson(w.history().document())["schemaVersion"]==20,"Position not persisted in schema 20");
        click(canvas,{at,0});hover(canvas,{170,50});click(canvas,{170,50});
        require(w.history().document().definition->routingDecisions.size()==1 && decision(w).routes.size()==2,
            "Starting on the same line did not group the second route");
        undo(w);require(decision(w).routes.size()==1,"Undo did not remove the second route atomically");
        w.findChild<QComboBox*>("editorTool")->setCurrentIndex(0);
        const auto p=canvas->mapFromScene(at,0),q=canvas->mapFromScene(60,0);
        const auto revision=w.history().revision();QTest::mousePress(canvas->viewport(),Qt::LeftButton,{},p);
        QTest::mouseMove(canvas->viewport(),q);QApplication::processEvents();
        require(w.history().revision()==revision && decision(w).position==at,"Drag preview wrote to History");
        QTest::mouseRelease(canvas->viewport(),Qt::LeftButton,{},q);QApplication::processEvents();
        require(std::abs(*decision(w).position-60)<1,"Drag did not move along the Link");
        undo(w);require(decision(w).position==at,"Undo lost the original position");
        QTest::mousePress(canvas->viewport(),Qt::LeftButton,{},p);QTest::mouseMove(canvas->viewport(),q);
        QTest::keyClick(canvas,Qt::Key_Escape);QTest::mouseRelease(canvas->viewport(),Qt::LeftButton,{},q);
        require(decision(w).position==at,"Cancelled drag committed");
        w.saveFile(file);w.openFile(file);QApplication::processEvents();require(decision(w).position==at,"Reopen lost the station");
        undo(w); // Reopened history is empty: the stored document remains intact.
        require(decision(w).position==at,"Empty Undo modified the reopened station");
        std::cout<<"station routing UI ok\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
