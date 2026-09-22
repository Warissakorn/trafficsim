#include "../src/shell/editor_window.hpp"
#include <QApplication>
#include <QGraphicsItem>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <cmath>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T>T* item(QObject& root,const char* name){auto* p=root.findChild<T*>(name);require(p,"Missing widget");return p;}
void action(EditorWindow& w,const char* name){item<QAction>(w,name)->trigger();QApplication::processEvents();}
QPoint pixel(EditorCanvas* c,Point p){const auto q=c->mapFromScene(p.x,p.y);require(c->viewport()->rect().contains(q),"Outside viewport");return q;}
void drag(EditorCanvas* c,Point a,Point b,Qt::MouseButton button,Qt::KeyboardModifiers modifiers={}) {
    QTest::mousePress(c->viewport(),button,modifiers,pixel(c,a));QTest::mouseMove(c->viewport(),pixel(c,b));
    QTest::mouseRelease(c->viewport(),button,modifiers,pixel(c,b));
    // The offscreen platform has no window manager to reactivate the editor after
    // a modal closes. Supply that activation, and release the creation modifier.
    QApplication::setActiveWindow(c->window());c->setFocus();
    if(modifiers&Qt::ControlModifier)QTest::keyRelease(c,Qt::Key_Control);
    QTest::qWait(10);QApplication::processEvents();
}
// `lanes` fills the Connector range boxes when a test is about the dialog honouring them.
// Left at 0 the dialog is accepted exactly as it opens, which is how its defaults are tested.
void confirm(const char* expected,int lanes=0) {
    // QTest's mouse events process timers before release opens the modal. Wait for
    // the actual dialog; never throw through a Qt event handler.
    auto* timer=new QTimer(qApp);
    QObject::connect(timer,&QTimer::timeout,timer,[timer,expected,lanes]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
        timer->stop();timer->deleteLater();
        if(dialog->objectName()!=expected){std::cerr<<"Unexpected dialog: "<<dialog->objectName().toStdString()<<'\n';dialog->reject();return;}
        if(auto* count=dialog->findChild<QSpinBox*>("editorGestureLaneCount"))count->setValue(3);
        if(lanes)for(const auto* name:{"editorRangeFromCount","editorRangeToCount"})
            if(auto* count=dialog->findChild<QSpinBox*>(name))count->setValue(lanes);
        auto* buttons=dialog->findChild<QDialogButtonBox*>();
        if(buttons && buttons->button(QDialogButtonBox::Ok))buttons->button(QDialogButtonBox::Ok)->click();
        else {std::cerr<<"Missing confirmation button\n";dialog->reject();}
    });
    timer->start(5);
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        require(argc>1,"Data directory required");QTemporaryDir directory;require(directory.isValid(),"Temporary directory");
        qputenv("XDG_DATA_HOME",directory.path().toUtf8());
        EditorWindow w{std::filesystem::path(argv[1])};w.resize(1600,1000);w.show();QTest::qWait(30);
        auto* c=w.canvas();c->fitInView(QRectF(-100,-40,200,80),Qt::KeepAspectRatio);c->centerOn(0,0);
        QTest::keyClick(c,Qt::Key_L);require(item<QListWidget>(w,"editorObjectPalette")->currentRow()==1,"L did not select Links");
        confirm("editorLinkDialog");drag(c,{-80,0},{-20,0},Qt::RightButton,Qt::ControlModifier);
        confirm("editorLinkDialog");drag(c,{20,0},{80,0},Qt::RightButton,Qt::ControlModifier);
        require(w.history().document().network.links.size()==2,"Ctrl-right-drag did not draw two links");
        const auto links=w.history().document().network.links;
        const auto from=laneGeometry(links[0],links[0].lanes[0].id,DrivingSide::left).back();
        const auto to=laneGeometry(links[1],links[1].lanes[0].id,DrivingSide::left).front();
        QTest::keyClick(c,Qt::Key_C);require(item<QListWidget>(w,"editorObjectPalette")->currentRow()==2,"C did not select Connectors");const auto before=documentJson(w.history().document());
        QTest::mousePress(c->viewport(),Qt::RightButton,Qt::ControlModifier,pixel(c,from));
        QTest::mouseMove(c->viewport(),pixel(c,to));QTest::keyClick(c,Qt::Key_Escape);
        QTest::mouseRelease(c->viewport(),Qt::RightButton,Qt::ControlModifier,pixel(c,to));
        require(documentJson(w.history().document())==before,"Cancelled creation changed document");
        confirm("editorRangeDialog",3);drag(c,from,to,Qt::RightButton,Qt::ControlModifier);
        require(w.history().document().network.connectors.size()==1,"Range was not one object");
        const auto connector=w.history().document().network.connectors.front();
        require(connector.fromLaneCount==3 && connector.toLaneCount==3,"Dialog lost lane ranges");
        require(w.history().revision()==3,"Range gesture was not one command");
        const auto paths=connectorPaths(w.history().document().network,connector);
        QTest::keyClick(c,Qt::Key_S);c->select(connector.id);const auto full=documentJson(w.history().document());
        Point resize{};bool found=false;
        for(auto* item:c->scene()->items())if(item->data(0).toString()=="lane-resize" && item->data(1).toInt()==1) {
            const auto p=item->sceneBoundingRect().center();resize={p.x(),p.y()};found=true;break;
        }
        require(found,"Source side handle missing");
        drag(c,resize,{resize.x,resize.y+3.5},Qt::LeftButton);
        require(w.history().document().network.connectors.front().fromLaneCount==2,"Corner did not resize range");
        action(w,"editorUndo");require(documentJson(w.history().document())==full,"Range Undo changed data");

        // A drag on a multi-selection moves all of it, connectors and heads included.
        c->setSelection({links[0].id,links[1].id});
        const auto beforeMove=w.history().document();
        QTest::mouseClick(c->viewport(),Qt::LeftButton,{},pixel(c,{-50,0}));
        require(w.history().document()==beforeMove,"A click on a selected object moved something");
        c->setSelection({links[0].id,links[1].id});
        drag(c,{-50,0},{-40,5},Qt::LeftButton);
        const auto& after=w.history().document().network;
        const auto moved=[&](const std::vector<Point>& was,const std::vector<Point>& now,const char* what) {
            require(was.size()==now.size(),what);
            for(std::size_t i=0;i<was.size();++i)
                require(std::abs(now[i].x-was[i].x-10)<1e-9 && std::abs(now[i].y-was[i].y-5)<1e-9,what);
        };
        moved(beforeMove.network.links[0].geometry,after.links[0].geometry,"First link did not move");
        moved(beforeMove.network.links[1].geometry,after.links[1].geometry,"Second link did not move");
        moved(beforeMove.network.connectors[0].geometry,after.connectors[0].geometry,"Connector did not ride its links");
        // One Undo takes the whole move back, which is what makes it one command and not one per object.
        action(w,"editorUndo");require(w.history().document()==beforeMove,"Group move was not one undoable command");
        // Hand tremor must not move a whole selection, even when the two pixels fall either
        // side of a grid line and so snap a metre apart. Nothing else guards that case: the
        // press and release pixels differ, so the delta is not zero.
        c->setTransform(QTransform::fromScale(40,-40));c->centerOn(-50,0);
        c->setSelection({links[0].id,links[1].id});
        const auto beforeJitter=w.history().document();
        const auto press=pixel(c,{-50.6,0});
        QTest::mousePress(c->viewport(),Qt::LeftButton,{},press);
        QTest::mouseMove(c->viewport(),press+QPoint(8,0));QTest::mouseRelease(c->viewport(),Qt::LeftButton,{},press+QPoint(8,0));
        require(w.history().document()==beforeJitter,"A jitter under the drag threshold moved the selection");
        // The forcing: the same press pixel, dragged far enough to count, does move the
        // selection -- so the check above is the threshold, not a dead press point.
        QTest::qWait(600);
        QTest::mousePress(c->viewport(),Qt::LeftButton,{},press);
        QTest::mouseMove(c->viewport(),press+QPoint(60,0));QTest::mouseRelease(c->viewport(),Qt::LeftButton,{},press+QPoint(60,0));
        require(w.history().document()!=beforeJitter,"A drag past the threshold moved nothing");
        require(std::abs(w.history().document().network.links[1].geometry.front().x
                        -beforeJitter.network.links[1].geometry.front().x-2)<1e-9,"Both links did not move together");
        action(w,"editorUndo");require(w.history().document()==beforeJitter,"Jitter-drag Undo changed data");
        c->fitInView(QRectF(-100,-40,200,80),Qt::KeepAspectRatio);c->centerOn(0,0);
        c->select(links[0].id);
        QTest::mouseClick(c->viewport(),Qt::LeftButton,Qt::ControlModifier,pixel(c,links[0].geometry.front()));
        require(w.history().document().network.links.size()==2,"Ctrl-click duplicated instead of selecting");
        const auto beforeCopy=documentJson(w.history().document());
        QTest::mouseClick(c->viewport(),Qt::LeftButton,Qt::ControlModifier,pixel(c,{50,0}));
        require(c->selection().size()==2,"Ctrl-click did not extend selection");
        require(documentJson(w.history().document())==beforeCopy,"Selection mutated document");
        c->select(links[0].id);
        drag(c,{-50,0},{-45,0},Qt::LeftButton,Qt::ControlModifier);
        require(w.history().document().network.links.size()==3,"Ctrl-drag did not duplicate link");
        const auto upper=c->selected();require(upper!=links[0].id,"Duplicate reused ID");
        auto* level=item<QComboBox>(w,"editorObjectLevel");level->setCurrentIndex(level->findData(1));
        auto* style=item<QComboBox>(w,"editorDisplayType");style->setCurrentIndex(style->findData(QStringLiteral("ramp")));action(w,"editorApplyDisplay");
        for(const double scale:{2.,8.}) {
            c->setTransform(QTransform::fromScale(scale,-scale));c->centerOn(-50,0);
            QTest::mouseClick(c->viewport(),Qt::LeftButton,{},pixel(c,{-50,0}));
            require(c->selected()==upper,"Upper level was not picked first");
            QTest::keyClick(c,Qt::Key_Tab);require(c->selected()==links[0].id,"Tab did not reach lower level");
        }
        auto* visible=item<QComboBox>(w,"editorVisibleLevel");visible->setCurrentIndex(visible->findData(0));
        const auto ground=c->hitObjects({-50,0});require(ground.size()==1 && ground.front().first==links[0].id,"Hidden level remained pickable");
        visible->setCurrentIndex(0);c->select(upper);const auto decorated=documentJson(w.history().document());
        const auto file=directory.path()+"/ranges.traffic.json";w.saveFile(file);w.openFile(file);
        require(documentJson(w.history().document())==decorated,"Range/level/display reopen lost data");
        require(w.history().document().network.links.back().displayType=="ramp","Display type was not applied");
        // A drag connects the whole carriageway: the range dialog pre-fills every lane of both
        // Links, and the author narrows it afterwards (M1.26).
        c->setTransform(QTransform::fromScale(4,-4));c->centerOn(0,-60);
        confirm("editorLinkDialog");drag(c,{-60,-60},{-10,-60},Qt::RightButton,Qt::ControlModifier);
        confirm("editorLinkDialog");drag(c,{10,-60},{60,-60},Qt::RightButton,Qt::ControlModifier);
        const auto fresh=w.history().document().network.links;
        require(fresh.size()==5,"Default-count fixture links missing");
        const auto& source=fresh[3];const auto& target=fresh[4];
        require(source.lanes.size()==3 && target.lanes.size()==3,"Fixture links are not three lanes");
        confirm("editorRangeDialog");
        drag(c,laneGeometry(source,source.lanes[0].id,DrivingSide::left).back(),
             laneGeometry(target,target.lanes[0].id,DrivingSide::left).front(),Qt::RightButton,Qt::ControlModifier);
        const auto single=w.history().document().network.connectors.back();
        require(w.history().document().network.connectors.size()==2,"Connector was not created");
        // M1.26, at the owner's request: the gesture connects the WHOLE carriageway and the
        // author narrows it afterwards, which is safe now that a route names the Connector
        // rather than its paths. Before, a routed Connector could not be narrowed at all, so
        // the drag deliberately defaulted to the single lane it started on.
        require(single.fromLaneCount==3 && single.toLaneCount==3,"Drag did not connect every lane");
        int markings=0;
        for(auto* item:c->scene()->items())
            if(item->data(0).toString()=="road-marking" && item->data(1).toString()==QString::fromStdString(single.id))++markings;
        require(markings==4,"A three-lane Connector is not drawn with its two dividers");
        w.saveFile(file); // Leave the document clean; closing a dirty window waits on a prompt.
        item<QComboBox>(w,"editorLanguage")->setCurrentIndex(1);action(w,"editorFit");
        require(item<QListWidget>(w,"editorObjectPalette")->item(2)->text().contains(QString::fromUtf8("เชื่อม")),"Thai palette missing");
        if(argc>2)require(w.grab().save(QString::fromUtf8(argv[2])),"Screenshot failed");
        w.close();std::cout<<"Creation, lane ranges, corner resize, duplication and level selection passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
