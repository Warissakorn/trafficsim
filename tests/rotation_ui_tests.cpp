#include "../src/shell/editor_window.hpp"
#include "../src/model/network/rotation.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QFocusEvent>
#include <QGraphicsPathItem>
#include <QInputDialog>
#include <QListWidget>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void near(double a,double b,double tolerance=1e-7) {
    require(std::isfinite(a) && std::isfinite(b) && std::abs(a-b)<=tolerance,"Rotation numeric mismatch");
}
template<class T>T* item(QObject& root,const char* name) {
    auto* found=root.findChild<T*>(name);require(found,"Missing rotation control");return found;
}
void action(EditorWindow& w,const char* name) {
    auto* a=item<QAction>(w,name);require(a->isEnabled(),"Disabled rotation action");
    a->trigger();QApplication::processEvents();
}
QPoint pixel(EditorCanvas& c,Point p) {
    const auto at=c.mapFromScene(p.x,p.y);require(c.viewport()->rect().contains(at),"Gesture outside viewport");return at;
}
void move(EditorCanvas& c,QPoint at,Qt::KeyboardModifiers modifiers=Qt::AltModifier) {
    QMouseEvent event(QEvent::MouseMove,QPointF(at),QPointF(c.viewport()->mapToGlobal(at)),
                      Qt::NoButton,Qt::LeftButton,modifiers);
    QApplication::sendEvent(c.viewport(),&event);
}
int previews(EditorCanvas& c) {
    int count=0;for(const auto* p:c.scene()->items())if(p->data(0)=="rotation-preview")++count;return count;
}
ProjectDocument roads(DrivingSide side) {
    ProjectDocument d;d.network.drivingSide=side;
    d.network.links={{"a",{{-70,0},{-10,0}},{{"a1",3.5}}},
                     {"b",{{10,0},{70,0}},{{"b1",3.5}}},
                     {"other",{{-70,35},{70,35}},{{"other1",3.5}}}};
    d.network.links[2].level=1;
    const auto connector=addConnector(d,{"a","a1"},{"b","b1"});
    const auto program=putProgram(d,{"",0,{{10,SignalColor::green}}});
    putSignalHead(d,{"ha",{"a","a1"},30,program,{}});
    putSignalHead(d,{"hc",{},5,program,connector});
    putSignalHead(d,{"ho",{"other","other1"},30,program,{}});
    validateDocument(d);return d;
}
void gestures(const QString& screenshot) {
    EditorCanvas c;c.resize(1000,760);c.show();QTest::qWait(20);
    History h;int rotations=0,otherEdits=0;double lastAngle=0;
    c.rotateRequested=[&](Point pivot,double degrees) {
        const auto ids=c.selection();++rotations;lastAngle=degrees;
        h.execute("rotate",[&](auto& d){rotateObjects(d,ids,pivot,degrees);});c.setDocument(&h.document());
    };
    c.translateRequested=[&](Point){++otherEdits;};
    c.editGeometry=[&](const auto&,const auto&){++otherEdits;};
    for(auto side:{DrivingSide::left,DrivingSide::right}) {
        h.reset(roads(side));c.setDocument(&h.document());c.setVisibleLevel({});
        c.setTransform(QTransform::fromScale(4,-4));c.centerOn(0,0);c.setTool(EditorCanvas::Tool::select);
        c.setSelection({"a","b","ho"});c.grid=25;c.snap=true;
        const auto before=h.document();const auto press=pixel(c,{-50,0}),release=pixel(c,{0,-50});
        QTest::mousePress(c.viewport(),Qt::LeftButton,Qt::AltModifier,press);move(c,release);
        require(previews(c)==5,"Preview missed internal Connector/heads or moved unrelated head");
        require(h.document()==before,"Preview committed an edit");
        // The selected Link's surface is horizontal before and vertical after +90 degrees.
        for(auto* graphic:c.scene()->items())if(graphic->data(0)=="rotation-preview" && graphic->data(1)=="a") {
            const auto box=graphic->sceneBoundingRect();near(box.center().x(),0);near(box.center().y(),-40);
            require(box.height()>box.width()*5,"Preview rotated in the wrong direction");
        }
        if(!screenshot.isEmpty() && side==DrivingSide::left)require(c.grab().save(screenshot),"Preview screenshot failed");
        QTest::keyClick(&c,Qt::Key_Right);require(otherEdits==0,"Nudge committed during rotation");
        QTest::mouseRelease(c.viewport(),Qt::LeftButton,Qt::AltModifier,release);
        near(lastAngle,90);require(h.document().network.links[0].geometry[0]==Point{0,-70},"Wrong world rotation");
        require(previews(c)==0 && h.states().size()==2,"Preview remained or drag made multiple edits");
        const auto after=h.document();h.undo();require(h.document()==before,"Rotation was not atomic");
        h.redo();require(h.document()==after,"Redo did not restore rotation");h.undo();c.setDocument(&h.document());
        // No move event: native release coordinates must still supply the committed angle.
        int count=rotations;QTest::mousePress(c.viewport(),Qt::LeftButton,Qt::AltModifier,press);
        QTest::mouseRelease(c.viewport(),Qt::LeftButton,Qt::AltModifier,release);
        require(rotations==count+1,"Coalesced rotation release was lost");near(lastAngle,90);
        h.undo();c.setDocument(&h.document());
        const auto end=pixel(c,{-48,-14}); // about 16 degrees; Shift quantizes to 15.
        QTest::mousePress(c.viewport(),Qt::LeftButton,Qt::AltModifier|Qt::ShiftModifier,press);
        move(c,end,Qt::AltModifier|Qt::ShiftModifier);
        QTest::mouseRelease(c.viewport(),Qt::LeftButton,Qt::AltModifier|Qt::ShiftModifier,end);
        near(lastAngle,15);h.undo();c.setDocument(&h.document());
        count=rotations;QTest::mouseClick(c.viewport(),Qt::LeftButton,Qt::AltModifier,press);
        QTest::mousePress(c.viewport(),Qt::LeftButton,Qt::AltModifier,press);
        QTest::mouseRelease(c.viewport(),Qt::LeftButton,Qt::AltModifier,press+QPoint(1,0));
        require(rotations==count && h.document()==before,"Click or jitter rotated objects");
        // Establish an armed preview before each cancellation, then deliver its stale release.
        for(int cancel=0;cancel<6;++cancel) {
            c.setVisibleLevel({});c.setTool(EditorCanvas::Tool::select);c.setSelection({"a","b"});
            QTest::mousePress(c.viewport(),Qt::LeftButton,Qt::AltModifier,press);move(c,release);
            require(previews(c)>0,"Cancellation fixture did not start a rotation");
            if(cancel==0)QTest::keyClick(&c,Qt::Key_Escape);
            if(cancel==1){QFocusEvent event(QEvent::FocusOut,Qt::OtherFocusReason);QApplication::sendEvent(&c,&event);}
            if(cancel==2)c.setTool(EditorCanvas::Tool::draw);
            if(cancel==3)c.select("other");
            if(cancel==4)c.setVisibleLevel(1);
            if(cancel==5)c.setDocument(&h.document());
            QTest::mouseRelease(c.viewport(),Qt::LeftButton,Qt::AltModifier,release);
            require(h.document()==before && rotations==count && previews(c)==0,"Cancelled rotation committed");
        }
        c.setVisibleLevel({});c.setTool(EditorCanvas::Tool::select);c.select("a");
        // A press at the single Link's pivot has no stable angle and must not become a move.
        QTest::mousePress(c.viewport(),Qt::LeftButton,Qt::AltModifier,pixel(c,{-40,0}));
        QTest::mouseRelease(c.viewport(),Qt::LeftButton,Qt::AltModifier,release);
        require(h.document()==before && rotations==count && otherEdits==0,"Centre press edited geometry");
        c.select("ho");require(!c.rotationPivot(),"Head-only selection has a rotation pivot");
    }
}
void write(const QString& path,const ProjectDocument& d) {
    QFile f(path);require(f.open(QIODevice::WriteOnly),"Cannot write fixture");
    const auto bytes=QByteArray::fromStdString(documentJson(d).dump());require(f.write(bytes)==bytes.size(),"Short fixture write");
}
void dialog(EditorWindow& w,double degrees,bool accept,bool shortcut=false,const QString& screenshot={}) {
    bool seen=false,bounded=false,captured=screenshot.isEmpty();QTimer timer;
    QObject::connect(&timer,&QTimer::timeout,[&] {
        auto* d=qobject_cast<QInputDialog*>(QApplication::activeModalWidget());if(!d)return;
        seen=d->objectName()=="editorRotationDialog";timer.stop();d->setDoubleValue(degrees);
        bounded=d->width()<=650;
        if(!screenshot.isEmpty())captured=d->grab().save(screenshot);
        if(accept && seen)d->accept();else d->reject();
    });timer.start(5);
    if(shortcut) {QApplication::setActiveWindow(&w);w.canvas()->setFocus();
        QTest::keyClick(w.canvas(),Qt::Key_R,Qt::ControlModifier|Qt::ShiftModifier);}
    else action(w,"editorRotate");
    require(seen,"Rotation dialog did not open");
    require(bounded,"Rotation instructions did not wrap within the dialog");
    require(captured,"Dialog screenshot failed");
    QApplication::setActiveWindow(&w);w.canvas()->setFocus();QApplication::processEvents();
}
void window(const std::filesystem::path& data,const QString& screenshot) {
    QTemporaryDir temp;require(temp.isValid(),"Temporary directory unavailable");
    const auto file=temp.filePath("rotation.traffic.json");
    EditorWindow w(data);w.resize(1600,1000);w.show();QTest::qWait(20);
    auto* c=w.canvas();require(!item<QAction>(w,"editorRotate")->isEnabled(),"Empty selection can rotate");
    write(file,roads(DrivingSide::left));w.openFile(file);c->select("ho");
    require(!item<QAction>(w,"editorRotate")->isEnabled(),"Head-only action enabled");
    c->select("other");c->setVisibleLevel(0);
    require(!item<QAction>(w,"editorRotate")->isEnabled(),"Hidden selection can rotate");
    // A running network verifies invalidation by the actual shell command.
    w.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
    std::vector<std::string> ids;for(const auto& l:w.history().document().network.links)ids.push_back(l.id);
    c->setSelection(ids);const auto before=w.history().document();
    action(w,"editorStep");require(w.runState().scenario && w.runState().tick==1,"Run fixture did not start");
    dialog(w,90,false);require(w.history().document()==before && w.runState().scenario,"Cancel changed model/run");
    dialog(w,360,true);require(w.history().document()==before && !w.history().dirty(),"Full turn created history");
    dialog(w,90,true,true);require(w.history().document()!=before && !w.runState().scenario,"Rotation retained model/run");
    require(item<QAction>(w,"editorUndo")->text().contains("Rotate selection"),"Rotation history name missing");
    auto* list=item<QListWidget>(w,"editorHistoryList");require(list->count()==2,"Numeric rotation made extra history");
    const auto after=w.history().document();action(w,"editorUndo");require(w.history().document()==before,"Dialog undo failed");
    action(w,"editorRedo");require(w.history().document()==after,"Dialog redo failed");
    item<QComboBox>(w,"editorLanguage")->setCurrentIndex(1);
    require(item<QAction>(w,"editorRotate")->text()==QString::fromUtf8("หมุนวัตถุที่เลือก"),"Rotation action not translated");
    require(item<QAction>(w,"editorUndo")->text().contains(QString::fromUtf8("หมุนวัตถุที่เลือก")),"History not translated");
    require(c->accessibleDescription().contains("15"),"Rotation help missing");
    c->setSelection(ids);dialog(w,-45,false,false,screenshot);
    w.saveFile(file);w.openFile(file);require(w.history().document()==after,"Save/reopen lost rotation");w.close();
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        require(argc>1,"Data directory required");
        gestures(argc>2?QString::fromUtf8(argv[2]):QString{});
        window(std::filesystem::path(argv[1]),argc>3?QString::fromUtf8(argv[3]):QString{});
        std::cout<<"PASS rotation preview, gestures, cancellation, dialog, history and persistence\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
