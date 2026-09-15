#include "../src/shell/editor_window.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGraphicsItem>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <cmath>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T>T* item(QObject& root,const char* name){auto* p=root.findChild<T*>(name);require(p,"Missing widget");return p;}
void action(EditorWindow& w,const char* name){item<QAction>(w,name)->trigger();QApplication::processEvents();}
QPoint pixel(EditorCanvas* c,Point p){const auto q=c->mapFromScene(p.x,p.y);require(c->viewport()->rect().contains(q),"Outside viewport");return q;}
void confirm(const char* expected,bool accept=true,int lanes=1) {
    auto* timer=new QTimer(qApp);timer->setInterval(5);
    QObject::connect(timer,&QTimer::timeout,timer,[timer,expected,accept,lanes]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
        timer->stop();timer->deleteLater();
        if(dialog->objectName()!=expected || !accept){dialog->reject();return;}
        for(const auto* name:{"editorGestureLaneCount","editorRangeFromCount","editorRangeToCount"})
            if(auto* count=dialog->findChild<QSpinBox*>(name))count->setValue(lanes);
        dialog->accept();
    });timer->start();
}
void releaseDrag(EditorCanvas* c,Point a,Point b,Qt::MouseButton button,bool cancel=false) {
    const auto modifiers=button==Qt::RightButton?Qt::ControlModifier:Qt::NoModifier;
    QTest::mousePress(c->viewport(),button,modifiers,pixel(c,a));
    // Deliberately omit mouseMove: native systems may coalesce the last move.
    if(cancel)QTest::keyClick(c,Qt::Key_Escape);
    QTest::mouseRelease(c->viewport(),button,Qt::NoModifier,pixel(c,b));
    QApplication::setActiveWindow(c->window());c->setFocus();QTest::keyRelease(c,Qt::Key_Control);
    QApplication::processEvents();
}
Point handle(EditorCanvas* c,int kind) {
    for(auto* item:c->scene()->items())if(item->data(0).toString()=="lane-resize" && item->data(1).toInt()==kind) {
        const auto p=item->sceneBoundingRect().center();return {p.x(),p.y()};
    }
    throw std::runtime_error("Missing visible lane resize handle");
}
void attached(const ProjectDocument& d) {
    require(validateNetwork(d.network).empty(),"Invalid network");
    for(const auto& c:d.network.connectors)for(const auto& p:connectorPaths(d.network,c)) {
        require(p.geometry.front()==laneAttachment(d.network,p.from,true),"Source detached");
        require(p.geometry.back()==laneAttachment(d.network,p.to,false),"Target detached");
    }
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        require(argc>1,"Data required");QTemporaryDir dir;
        EditorWindow w{std::filesystem::path(argv[1])};w.resize(1600,1000);w.show();QTest::qWait(30);
        auto* c=w.canvas();c->fitInView(QRectF(-110,-60,220,120),Qt::KeepAspectRatio);c->centerOn(0,0);
        // Select is the initial mode. This used to draw a disappearing dashed line.
        confirm("editorLinkDialog",true,3);releaseDrag(c,{-90,-15},{-10,-15},Qt::RightButton);
        require(w.history().document().network.links.size()==1,"Select-mode release did not create Link");
        confirm("editorLinkDialog",true,3);releaseDrag(c,{10,15},{90,15},Qt::RightButton);
        require(w.history().document().network.links.size()==2,"Second Link missing");
        const auto links=w.history().document().network.links;
        const auto before=documentJson(w.history().document());
        confirm("editorLinkDialog",false);releaseDrag(c,{-90,40},{-20,40},Qt::RightButton);
        require(documentJson(w.history().document())==before,"Dialog Cancel mutated document");
        releaseDrag(c,{-90,40},{-20,40},Qt::RightButton,true);
        require(documentJson(w.history().document())==before,"Esc mutated document");
        const auto lanePoint=[&](int link,int lane,double fraction){const auto g=laneGeometry(links[link],links[link].lanes[lane].id,DrivingSide::left);return pointAlong(g,fraction*polylineLength(g));};
        releaseDrag(c,lanePoint(0,0,.6),{0,45},Qt::RightButton);
        require(documentJson(w.history().document())==before,"Invalid target created an object");
        require(item<QLabel>(w,"editorError")->text().contains("No object was created"),"Invalid target disappeared without feedback");
        confirm("editorRangeDialog",true,1);releaseDrag(c,lanePoint(0,0,.6),lanePoint(1,0,.4),Qt::RightButton);
        require(w.history().document().network.connectors.size()==1,"Body-to-body Connector missing");
        const auto id=c->selected();auto connector=w.history().document().network.connectors.front();
        require(connector.from.fraction.value_or(1.)>.5 && connector.from.fraction.value_or(1.)<.7,"Source snapped to endpoint");
        require(connector.to.fraction.value_or(0.)>.3 && connector.to.fraction.value_or(0.)<.5,"Target snapped to endpoint");
        attached(w.history().document());
        auto p=handle(c,1);releaseDrag(c,p,{p.x,p.y-3.5},Qt::LeftButton);
        require(w.history().document().network.connectors.front().fromLaneCount==2,"Single lane source cannot grow");
        p=handle(c,2);releaseDrag(c,p,{p.x,p.y-7},Qt::LeftButton);
        require(w.history().document().network.connectors.front().toLaneCount==3,"Target range cannot grow independently");
        const auto ranged=documentJson(w.history().document());
        p=handle(c,1);releaseDrag(c,p,{p.x,p.y+3.5},Qt::LeftButton,true);
        require(documentJson(w.history().document())==ranged,"Esc committed resize");
        // At the middle of this diagonal connector, drag perpendicular to its path.
        const auto current=w.history().document().network.connectors.front();
        const auto outer=connectorPaths(w.history().document().network,current).back().geometry;
        const auto mid=pointAlong(outer,polylineLength(outer)/2),ahead=pointAlong(outer,polylineLength(outer)/2+.01);
        const double norm=std::hypot(ahead.x-mid.x,ahead.y-mid.y);
        p=handle(c,3);releaseDrag(c,p,{p.x-(ahead.y-mid.y)/norm*3.5,p.y+(ahead.x-mid.x)/norm*3.5},Qt::LeftButton);
        connector=w.history().document().network.connectors.front();
        require(connector.fromLaneCount==2 && connector.toLaneCount==2,"Middle handle did not set connector lane count");
        action(w,"editorUndo");require(documentJson(w.history().document())==ranged,"Resize not one undo transaction");
        action(w,"editorRedo");attached(w.history().document());
        c->select(links[0].id);p=handle(c,4);releaseDrag(c,p,{p.x,p.y-3.5},Qt::LeftButton);
        require(w.history().document().network.links[0].lanes.size()==4,"Link side cannot grow");attached(w.history().document());
        p=handle(c,4);releaseDrag(c,p,{p.x,p.y+3.5},Qt::LeftButton);
        require(w.history().document().network.links[0].lanes.size()==3,"Link side cannot shrink");attached(w.history().document());
        // Both click-pick positions are also on link bodies, in connector mode.
        item<QComboBox>(w,"editorTool")->setCurrentIndex(5);
        QTest::mouseClick(c->viewport(),Qt::LeftButton,{},pixel(c,lanePoint(0,2,.25)));
        QTest::mouseClick(c->viewport(),Qt::LeftButton,{},pixel(c,lanePoint(1,2,.75)));
        require(w.history().document().network.connectors.size()==2,"Two-click body attachment failed");attached(w.history().document());
        item<QComboBox>(w,"editorTool")->setCurrentIndex(0);c->select(id);
        const auto stored=w.history().document().network.connectors.front();action(w,"editorApplyConnector");
        require(w.history().document().network.connectors.front()==stored,"Applying unchanged inspector lost precise positions");
        item<QComboBox>(w,"editorDrivingSide")->setCurrentIndex(1);attached(w.history().document());
        p=handle(c,1);releaseDrag(c,p,{p.x,p.y+3.5},Qt::LeftButton);
        require(w.history().document().network.connectors.front().fromLaneCount==3,"Right-driving source cannot grow outward");
        action(w,"editorUndo");action(w,"editorUndo");attached(w.history().document());
        const auto file=dir.path()+"/body-connectors.traffic.json";const auto saved=documentJson(w.history().document());
        w.saveFile(file);w.openFile(file);require(documentJson(w.history().document())==saved,"Save/reopen lost attachments");
        c->select(id);item<QComboBox>(w,"editorLanguage")->setCurrentIndex(1);action(w,"editorFit");QTest::qWait(30);
        if(argc>2)require(w.grab().save(QString::fromUtf8(argv[2])),"Screenshot failed");
        std::cout<<"Release-only creation, body attachments, side resizing, cancel, history and reopen passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
