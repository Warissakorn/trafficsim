#include "../src/shell/editor_window.hpp"
#include <QApplication>
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
}
void confirm(const char* expected) {
    // QTest's mouse events process timers before release opens the modal. Wait for
    // the actual dialog; never throw through a Qt event handler.
    auto* timer=new QTimer(qApp);
    QObject::connect(timer,&QTimer::timeout,timer,[timer,expected]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
        timer->stop();timer->deleteLater();
        if(dialog->objectName()!=expected){std::cerr<<"Unexpected dialog: "<<dialog->objectName().toStdString()<<'\n';dialog->reject();return;}
        if(auto* count=dialog->findChild<QSpinBox*>("editorGestureLaneCount"))count->setValue(3);
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
        QTest::keyClick(c,Qt::Key_C);const auto before=documentJson(w.history().document());
        QTest::mousePress(c->viewport(),Qt::RightButton,Qt::ControlModifier,pixel(c,from));
        QTest::mouseMove(c->viewport(),pixel(c,to));QTest::keyClick(c,Qt::Key_Escape);
        QTest::mouseRelease(c->viewport(),Qt::RightButton,Qt::ControlModifier,pixel(c,to));
        require(documentJson(w.history().document())==before,"Cancelled creation changed document");
        confirm("editorRangeDialog");drag(c,from,to,Qt::RightButton,Qt::ControlModifier);
        require(w.history().document().network.connectors.size()==1,"Range was not one object");
        const auto connector=w.history().document().network.connectors.front();
        require(connector.fromLaneCount==3 && connector.toLaneCount==3,"Dialog lost lane ranges");
        require(w.history().revision()==3,"Range gesture was not one command");
        const auto paths=connectorPaths(w.history().document().network,connector);
        QTest::keyClick(c,Qt::Key_S);c->select(connector.id);const auto full=documentJson(w.history().document());
        drag(c,paths.back().geometry.front(),paths[1].geometry.front(),Qt::LeftButton);
        require(w.history().document().network.connectors.front().fromLaneCount==2,"Corner did not resize range");
        action(w,"editorUndo");require(documentJson(w.history().document())==full,"Range Undo changed data");

        c->select(links[0].id);
        QTest::mouseClick(c->viewport(),Qt::LeftButton,Qt::ControlModifier,pixel(c,links[0].geometry.front()));
        require(w.history().document().network.links.size()==3,"Ctrl-left-click did not duplicate link");
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
        item<QComboBox>(w,"editorLanguage")->setCurrentIndex(1);action(w,"editorFit");
        require(item<QListWidget>(w,"editorObjectPalette")->item(2)->text().contains(QString::fromUtf8("เชื่อม")),"Thai palette missing");
        if(argc>2)require(w.grab().save(QString::fromUtf8(argv[2])),"Screenshot failed");
        w.close();std::cout<<"Creation, lane ranges, corner resize, duplication and level selection passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
