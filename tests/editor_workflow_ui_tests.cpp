#include "../src/shell/editor_window.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFocusEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTemporaryDir>
#include <QTest>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
template<class T> T* item(QObject& root,const char* name) {
    auto* found=root.findChild<T*>(name); require(found,"Missing workflow control"); return found;
}
void action(EditorWindow& w,const char* name) {
    auto* a=item<QAction>(w,name); require(a->isEnabled(),"Disabled workflow action");
    a->trigger(); QApplication::processEvents();
}
QPoint pixel(EditorCanvas& c,Point p) {
    const auto at=c.mapFromScene(p.x,p.y);
    require(c.viewport()->rect().contains(at),"Gesture outside viewport"); return at;
}
ProjectDocument roads(DrivingSide side) {
    ProjectDocument d; d.network.drivingSide=side;
    d.network.links={{"a",{{-70,0},{-10,0}},{{"a1",3.5}}},
                     {"b",{{10,0},{70,0}},{{"b1",3.5}}},
                     {"bridge",{{-70,25},{70,25}},{{"bridge1",3.5}}}};
    d.network.links.back().level=1;
    addConnector(d,{"a","a1"},{"b","b1"});
    const auto program=putProgram(d,{"",0,{{10,SignalColor::green}}});
    putSignalHead(d,{"",{"bridge","bridge1"},30,program,{}});
    validateDocument(d); return d;
}
void canvasWorkflow() {
    EditorCanvas c; c.resize(900,600); c.show(); QTest::qWait(20);
    History h;
    c.editGeometry=[&](const auto& id,const auto& geometry) {
        h.execute("geometry",[&](auto& d) { changeGeometry(d,id,geometry); });
        c.setDocument(&h.document());
    };
    int translations=0, copies=0, creations=0;
    c.translateRequested=[&](Point delta) {
        const auto ids=c.selection(); ++translations;
        h.execute("move",[&](auto& d) { translateObjects(d,ids,delta); });
        c.setDocument(&h.document());
    };
    c.duplicateRequested=[&](Point) { ++copies; };
    c.createLinkGesture=[&](const auto&) { ++creations; };
    c.createRangeGesture=[&](auto,auto,const auto&) { ++creations; };
    for (auto side:{DrivingSide::left,DrivingSide::right}) {
        h.reset(roads(side)); c.setDocument(&h.document()); c.setVisibleLevel({});
        c.setTransform(QTransform::fromScale(4,-4)); c.centerOn(0,0);
        const auto connector=h.document().network.connectors.front().id;
        const auto head=h.document().network.signalHeads.front().id;
        c.setSelection({"a","bridge",head,connector});
        require(c.selection().size()==4,"Selection fixture was not established");
        int notifications=0; c.selectionChanged=[&] { ++notifications; };
        c.setVisibleLevel(0);
        require(c.selection()==std::vector<std::string>({"a",connector}),"Filter retained hidden selection");
        require(notifications==1,"Filter did not notify inspector exactly once");
        std::optional<int> filter=0;
        c.visibleLevelChanged=[&](auto level) { filter=level; };
        c.setSelection({"bridge",head,"missing","bridge"});
        require(c.selection()==std::vector<std::string>({"bridge",head}),"Invalid or duplicate selection survived");
        require(!filter,"Explicit hidden selection was not revealed");
        require(!c.hitObjects({0,25}).empty(),"Revealed road is not pickable");
        c.select("missing"); require(c.selection().empty(),"Unknown ID became an edit target");
        c.selectionChanged={}; c.visibleLevelChanged={};

        c.setSelection({"a","b"}); const auto before=h.document();
        c.snap=true; c.grid=.5;
        QTest::keyClick(&c,Qt::Key_Right);
        require(h.document().network.links.front().geometry.front()==Point{-69.5,0},"Arrow ignored grid interval");
        require(h.document().network.connectors.front().geometry.front().x==
                before.network.connectors.front().geometry.front().x+.5,"Nudge left connector behind");
        h.undo(); c.setDocument(&h.document()); require(h.document()==before,"Nudge was not one undo step");
        QTest::keyClick(&c,Qt::Key_Up,Qt::ShiftModifier);
        require(h.document().network.links.front().geometry.front()==Point{-70,5},"Shift nudge or Y direction wrong");
        h.undo(); c.setDocument(&h.document());
        c.snap=false; QTest::keyClick(&c,Qt::Key_Left);
        require(h.document().network.links.front().geometry.front()==Point{-71,0},"Unsnapped nudge is not one metre");
        h.undo(); c.setDocument(&h.document());
        QTest::keyClick(&c,Qt::Key_Down);
        require(h.document().network.links.front().geometry.front()==Point{-70,-1},"Down arrow direction wrong");
        h.undo(); c.setDocument(&h.document());
        QTest::keyClick(&c,Qt::Key_Right,Qt::ControlModifier);
        require(h.document()==before,"Modified navigation nudged objects");
        c.setTool(EditorCanvas::Tool::draw); QTest::keyClick(&c,Qt::Key_Right);
        require(h.document()==before,"Drawing tool unexpectedly nudged selection");
        c.setTool(EditorCanvas::Tool::select); c.select("a");
        c.centerOn(0,0);
        const auto press=pixel(c,{-40,0}), release=pixel(c,{-30,10});
        // Establish that this exact press/release really starts and commits a body drag.
        QTest::mousePress(c.viewport(),Qt::LeftButton,{},press);
        QTest::mouseRelease(c.viewport(),Qt::LeftButton,{},release);
        require(h.document()!=before,"Drag cancellation fixture did not start a drag");
        h.undo(); c.setDocument(&h.document()); c.select("a");
        QTest::mousePress(c.viewport(),Qt::LeftButton,{},press);
        const auto count=translations; QTest::keyClick(&c,Qt::Key_Right);
        require(translations==count && h.document()==before,"Arrow committed during drag");
        QFocusEvent lost(QEvent::FocusOut,Qt::OtherFocusReason);
        QApplication::sendEvent(&c,&lost);
        QTest::mouseRelease(c.viewport(),Qt::LeftButton,{},release);
        require(h.document()==before,"Focus-lost drag committed on stale release");
        QTest::mousePress(c.viewport(),Qt::LeftButton,Qt::ControlModifier,press);
        QApplication::sendEvent(&c,&lost);
        QTest::mouseRelease(c.viewport(),Qt::LeftButton,Qt::ControlModifier,release);
        require(copies==0,"Focus-lost copy committed");
        const auto start=pixel(c,{-60,-30}), end=pixel(c,{20,-30});
        QTest::mousePress(c.viewport(),Qt::RightButton,Qt::ControlModifier,start);
        QApplication::sendEvent(&c,&lost);
        QTest::mouseRelease(c.viewport(),Qt::RightButton,Qt::ControlModifier,end);
        require(creations==0,"Focus-lost creation committed");
        c.select("a"); QTest::mousePress(c.viewport(),Qt::LeftButton,{},press);
        c.setVisibleLevel(1);
        QTest::mouseRelease(c.viewport(),Qt::LeftButton,{},release);
        require(h.document()==before && c.selection().empty(),"Filter change did not cancel drag");
        c.setVisibleLevel({});
    }
}
void write(const QString& path,const ProjectDocument& d) {
    QFile file(path); require(file.open(QIODevice::WriteOnly),"Cannot write fixture");
    const auto data=QByteArray::fromStdString(documentJson(d).dump());
    require(file.write(data)==data.size(),"Short fixture write");
}
void windowWorkflow(const std::filesystem::path& data,const QString& screenshot) {
    QTemporaryDir temp; require(temp.isValid(),"Temporary directory unavailable");
    const auto file=temp.filePath("workflow.traffic.json");
    EditorWindow w(data); w.resize(1500,950); w.show(); QTest::qWait(30);
    auto* c=w.canvas(); auto* list=item<QListWidget>(w,"editorHistoryList");
    auto* level=item<QComboBox>(w,"editorVisibleLevel");
    for (auto side:{DrivingSide::left,DrivingSide::right}) {
        write(file,roads(side)); w.openFile(file);
        c->select("bridge"); level->setCurrentIndex(level->findData(0));
        require(c->selection().empty(),"Window retained hidden selection");
        require(!item<QAction>(w,"editorDeleteSelected")->isEnabled(),"Delete enabled for hidden selection");
        c->select("bridge");
        require(!level->currentData().isValid(),"Level dropdown did not follow revealed selection");
        require(item<QLineEdit>(w,"editorId")->text()=="bridge","Inspector did not follow selection");
    }
    // A runnable fixture verifies that history navigation invalidates an actual run snapshot.
    w.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
    const auto initial=w.history().document();
    std::vector<std::string> ids;
    for(const auto& link:initial.network.links) ids.push_back(link.id);
    c->setSelection(ids);
    item<QDoubleSpinBox>(w,"editorGrid")->setValue(.5);
    QApplication::setActiveWindow(&w); c->setFocus();
    QTest::keyClick(c,Qt::Key_Right); const auto moved=w.history().document();
    require(moved!=initial && list->count()==2,"Nudge did not reach history panel");
    require(item<QAction>(w,"editorUndo")->text().contains("Move selection"),"Undo did not name command");
    QTest::keyClick(c,Qt::Key_Up,Qt::ShiftModifier); const auto saved=w.history().document();
    w.saveFile(file);
    require(list->count()==3 && list->currentItem()->text().contains("Saved"),"History saved marker missing");
    auto* name=item<QLineEdit>(w,"editorName"); name->setFocus();
    QTest::keyClick(name,Qt::Key_Left);
    require(w.history().document()==saved,"Text-field arrow edited network");
    action(w,"editorHistory"); require(list->isVisible(),"History toggle failed");
    list->setFocus(); QTest::keyClick(list,Qt::Key_Up);
    require(w.history().document()==saved,"Browsing history restored without activation");
    action(w,"editorStep"); require(w.runState().scenario && w.runState().tick==1,"Run snapshot fixture failed");
    list->setCurrentRow(0); QTest::keyClick(list,Qt::Key_Return);
    require(w.history().document()==initial && !w.runState().scenario,"History restore retained edited model or run");
    require(list->count()==3 && w.history().canRedo(),"History navigation discarded redo");
    list->setCurrentRow(2); QTest::keyClick(list,Qt::Key_Return);
    require(w.history().document()==saved && !w.history().dirty(),"History did not restore saved state");
    item<QComboBox>(w,"editorLanguage")->setCurrentIndex(1);
    require(list->currentItem()->text().contains(QString::fromUtf8("ปัจจุบัน")),"History did not translate");
    require(item<QAction>(w,"editorUndo")->text().contains(QString::fromUtf8("ย้ายวัตถุที่เลือก")),"Command name did not translate");
    if(!screenshot.isEmpty()) require(w.grab().save(screenshot),"Screenshot failed");
    list->setCurrentRow(0); QTest::keyClick(list,Qt::Key_Return);
    c->setSelection(ids); c->setFocus(); QTest::keyClick(c,Qt::Key_Left);
    require(!w.history().canRedo() && list->count()==2,"New edit retained abandoned future");
    require(w.history().revision()>saved.revision,"History reused a revision after branching");
    w.saveFile(file); const auto final=w.history().document(); w.openFile(file);
    require(w.history().document()==final && list->count()==1,"Reopen lost project or kept old history");
    w.close();
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        require(argc>1,"Data directory required");
        canvasWorkflow();
        windowWorkflow(std::filesystem::path(argv[1]),argc>2?QString::fromUtf8(argv[2]):QString{});
        std::cout<<"PASS level selection, focus cancellation, keyboard nudging and history workflow\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
