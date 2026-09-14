#include "../src/shell/editor_window.hpp"
#include <QApplication>
#include <QAction>
#include <QAbstractButton>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* msg) { if (!ok) throw std::runtime_error(msg); }
template<class T> T* item(EditorWindow& w,const char* name) {
    auto* p=w.findChild<T*>(name);require(p,"Missing widget");return p;
}
void action(EditorWindow& w,const char* name) { item<QAction>(w,name)->trigger();QApplication::processEvents(); }
QPoint pixel(EditorCanvas* c,Point p) {
    const auto result=c->mapFromScene(p.x,p.y);require(c->viewport()->rect().contains(result),"Gesture outside viewport");return result;
}
void click(EditorCanvas* c,Point p) { QTest::mouseClick(c->viewport(),Qt::LeftButton,{},pixel(c,p)); }
void drag(EditorCanvas* c,Point from,Point to,bool cancel=false) {
    const auto a=pixel(c,from),b=pixel(c,to);
    QTest::mousePress(c->viewport(),Qt::LeftButton,{},a);QTest::mouseMove(c->viewport(),b);
    if (cancel) QTest::keyClick(c,Qt::Key_Escape);
    QTest::mouseRelease(c->viewport(),Qt::LeftButton,{},b);
}
void lane(QComboBox* box,const std::string& id) {
    const auto index=box->findData(QString::fromStdString(id));require(index>0,"Missing lane option");box->setCurrentIndex(index);
}
void anchored(const ProjectDocument& d) {
    require(validateNetwork(d.network).empty(),"Invalid authored geometry");
    for (const auto& c : d.network.connectors) for (const auto& link : d.network.links) {
        if (link.id==c.from.linkId) require(c.geometry.front()==laneGeometry(link,c.from.laneId,d.network.drivingSide).back(),"Source detached");
        if (link.id==c.to.linkId) require(c.geometry.back()==laneGeometry(link,c.to.laneId,d.network.drivingSide).front(),"Target detached");
    }
}
void answer(QMessageBox::StandardButton choice) {
    QTimer::singleShot(10,[choice]{for (auto* top : QApplication::topLevelWidgets())
        if (auto* box=qobject_cast<QMessageBox*>(top)) box->button(choice)->click();});
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        require(argc>=2,"Expected data directory");QTemporaryDir directory;require(directory.isValid(),"temp directory");
        EditorWindow w{std::filesystem::path(argv[1])};w.show();QTest::qWait(30);
        auto* c=w.canvas();auto* tool=item<QComboBox>(w,"editorTool");
        const auto draw=[&](Point a,Point b){tool->setCurrentIndex(1);click(c,a);click(c,b);QTest::keyClick(c,Qt::Key_Return);};
        draw({-65,-30},{-15,-30});draw({15,0},{15,50});draw({25,-30},{65,-30});
        require(w.history().document().network.links.size()==3,"Road drawing failed");action(w,"editorFit");
        const auto links=w.history().document().network.links;
        const auto from=laneGeometry(links[0],links[0].lanes[0].id,DrivingSide::left).back();
        const auto to=laneGeometry(links[1],links[1].lanes[1].id,DrivingSide::left).front();
        const auto beforeDraw=documentJson(w.history().document());
        tool->setCurrentIndex(5);click(c,from);
        require(c->pickingConnectorTarget(),"Source endpoint was not picked");
        require(documentJson(w.history().document())==beforeDraw,"First click changed document");
        QTest::keyClick(c,Qt::Key_Escape);click(c,to);
        require(w.history().document().network.connectors.empty(),"Escape did not cancel source");
        click(c,from);tool->setCurrentIndex(0);require(!c->pickingConnectorTarget(),"Tool switch retained source");
        tool->setCurrentIndex(5);click(c,from);
        QTest::mouseMove(c->viewport(),pixel(c,to));
        require(documentJson(w.history().document())==beforeDraw,"Hover preview mutated document");
        click(c,to);require(w.history().revision()==4,"Two-click connector was not one command");
        require(w.history().document().network.connectors.size()==1,"Connector not created");
        const auto id=c->selected();const auto created=w.history().document().network.connectors[0];
        require(created.from.laneId==links[0].lanes[0].id && created.to.laneId==links[1].lanes[1].id,"Wrong lane mapping");
        anchored(w.history().document());
        require(item<QTabWidget>(w,"editorPropertyTabs")->currentIndex()==1,"Connector properties not shown");

        tool->setCurrentIndex(0);c->select("");click(c,created.geometry[6]);
        require(c->selected()==id,"Connector path not selectable");
        c->snap=false;const auto beforeDrag=documentJson(w.history().document());
        const auto handle=created.geometry[6];const Point moved{handle.x-4,handle.y+5};
        drag(c,handle,moved);require(w.history().revision()==5,"Curve drag was not one command");
        require(w.history().document().network.connectors[0].geometry!=created.geometry,"Curve handle did not move");
        anchored(w.history().document());action(w,"editorUndo");
        require(documentJson(w.history().document())==beforeDrag,"Curve undo lost geometry");
        action(w,"editorRedo");const auto reshaped=documentJson(w.history().document());
        const auto shape=w.history().document().network.connectors[0].geometry;
        drag(c,shape[6],{shape[6].x+4,shape[6].y+3},true);
        require(documentJson(w.history().document())==reshaped,"Cancelled handle drag committed");
        drag(c,shape.front(),{shape.front().x+5,shape.front().y+5});QTest::keyClick(c,Qt::Key_Delete,Qt::ControlModifier);
        require(documentJson(w.history().document())==reshaped,"Source endpoint moved or was removed");
        drag(c,shape.back(),{shape.back().x+5,shape.back().y+5});QTest::keyClick(c,Qt::Key_Delete,Qt::ControlModifier);
        require(documentJson(w.history().document())==reshaped,"Target endpoint moved or was removed");
        const Point insert{(shape[3].x+shape[4].x)/2,(shape[3].y+shape[4].y)/2};
        QTest::mouseDClick(c->viewport(),Qt::LeftButton,{},pixel(c,insert));
        require(w.history().document().network.connectors[0].geometry.size()==shape.size()+1,"Point insertion failed");
        click(c,w.history().document().network.connectors[0].geometry[4]);QTest::keyClick(c,Qt::Key_Delete,Qt::ControlModifier);
        require(w.history().document().network.connectors[0].geometry==shape,"Point removal changed other points");
        action(w,"editorStraightConnector");require(w.history().document().network.connectors[0].geometry.size()==2,"Straighten failed");
        action(w,"editorResetCurve");require(w.history().document().network.connectors[0].geometry==created.geometry,"Curve reset failed");

        auto* source=item<QComboBox>(w,"editorConnectorFrom");auto* target=item<QComboBox>(w,"editorConnectorTo");
        const auto beforeRetarget=documentJson(w.history().document());
        lane(target,links[2].lanes[0].id);action(w,"editorApplyConnector");
        require(w.history().document().network.connectors[0].to.linkId==links[2].id,"Retarget failed");anchored(w.history().document());
        action(w,"editorUndo");require(documentJson(w.history().document())==beforeRetarget,"Retarget undo failed");
        lane(source,links[0].lanes[1].id);lane(target,links[1].lanes[0].id);action(w,"editorCreateConnector");
        require(w.history().document().network.connectors.size()==2,"Inspector creation failed");
        const auto beforeDuplicate=documentJson(w.history().document());action(w,"editorCreateConnector");
        require(documentJson(w.history().document())==beforeDuplicate,"Duplicate changed document");
        require(item<QLabel>(w,"editorError")->text().contains("already have"),"Duplicate feedback missing");

        c->select(links[0].id);item<QLineEdit>(w,"editorLaneWidths")->setText("4, 5");action(w,"editorApplyLanes");
        anchored(w.history().document());const auto beforeMove=documentJson(w.history().document());
        drag(c,links[0].geometry.back(),{-15,-35});anchored(w.history().document());
        require(documentJson(w.history().document())!=beforeMove,"Connected link did not move");
        action(w,"editorUndo");require(documentJson(w.history().document())==beforeMove,"Connected-link undo failed");
        item<QComboBox>(w,"editorDrivingSide")->setCurrentIndex(1);anchored(w.history().document());
        lane(item<QComboBox>(w,"editorConnectorObject"),id);
        require(c->selected()==id,"Connector ID selector did not select the object");
        item<QComboBox>(w,"editorLanguage")->setCurrentIndex(1);action(w,"editorCreateConnector");
        require(item<QLabel>(w,"editorError")->text().contains(QString::fromUtf8("มี Connector อยู่แล้ว")),"Thai duplicate feedback missing");
        require(tool->itemText(5)==QString::fromUtf8("เชื่อมเลน"),"Thai tool translation missing");
        const auto file=directory.path()+QString::fromUtf8("/ทางเชื่อม.traffic.json");
        const auto saved=documentJson(w.history().document());w.saveFile(file);w.openFile(file);
        require(!w.history().dirty() && documentJson(w.history().document())==saved,"Connector save/reopen lost data");
        require(!w.history().canUndo(),"Reopen retained old history");

        w.openFile(QString::fromUtf8(argv[1])+"/scenarios/crossing.json");
        item<QComboBox>(w,"editorLanguage")->setCurrentIndex(0);
        lane(item<QComboBox>(w,"editorConnectorObject"),"west-east");
        lane(target,"north-1");const auto controlled=documentJson(w.history().document());action(w,"editorApplyConnector");
        require(documentJson(w.history().document())==controlled,"Retarget broke existing route");
        require(item<QLabel>(w,"editorError")->text().contains("route uses"),"Referenced-route feedback missing");
        answer(QMessageBox::No);action(w,"editorDeleteConnector");
        require(documentJson(w.history().document())==controlled,"Cancelled delete changed document");
        answer(QMessageBox::Yes);action(w,"editorDeleteConnector");
        require(w.history().document().network.connectors.size()==1,"Confirmed connector delete failed");
        require(w.history().document().definition->routes.size()==1 && w.history().document().definition->inputs.size()==1,"Delete left dangling routes or inputs");
        action(w,"editorUndo");require(documentJson(w.history().document())==controlled,"Delete undo lost related objects");

        w.openFile(file);item<QComboBox>(w,"editorLanguage")->setCurrentIndex(1);c->select(id);
        w.resize(1000,760);action(w,"editorFit");QTest::qWait(30);
        require(c->viewport()->width()>350,"Properties leave too little canvas space");
        if (argc>2) require(w.grab().save(QString::fromUtf8(argv[2])),"Screenshot failed");
        std::cout<<"Connector gestures, endpoint locks, history, reference safety, persistence and Thai controls passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
