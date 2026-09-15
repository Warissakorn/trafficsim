#include "../src/shell/editor_window.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T>T* item(QObject& root,const char* name){auto* p=root.findChild<T*>(name);require(p,"Missing widget");return p;}
void action(EditorWindow& w,const char* name){item<QAction>(w,name)->trigger();QApplication::processEvents();}
void accept(QDialog& dialog){item<QDialogButtonBox>(dialog,"")->button(QDialogButtonBox::Ok)->click();}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        require(argc>1,"data path");QTemporaryDir directory;require(directory.isValid(),"temporary directory");
        EditorWindow w{std::filesystem::path(argv[1])};w.show();QTest::qWait(30);
        auto* c=w.canvas();item<QSpinBox>(w,"editorLaneCount")->setValue(1);
        item<QComboBox>(w,"editorTool")->setCurrentIndex(1);
        QTest::mouseClick(c->viewport(),Qt::LeftButton,{},c->mapFromScene(-50,0));
        QTest::mouseClick(c->viewport(),Qt::LeftButton,{},c->mapFromScene(50,0));
        QTest::keyClick(c,Qt::Key_Return);
        require(w.history().document().network.links.size()==1,"draw");
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());require(dialog,"route dialog");
            item<QPushButton>(*dialog,"editorAppendSegment")->click();accept(*dialog);
        });
        action(w,"editorAddRoute");require(w.history().document().definition->routes.size()==1,"route command");
        require(item<QTableWidget>(w,"editorRouteTable")->rowCount()==1,"route table");
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());require(dialog,"input dialog");
            item<QDoubleSpinBox>(*dialog,"editorInputVolume")->setValue(1800);accept(*dialog);
        });
        action(w,"editorAddInput");require(w.history().document().definition->inputs.size()==1,"input command");
        action(w,"editorStep");require(w.runState().tick==1,"in-editor step");
        for(int i=0;i<300 && w.runState().vehicles.empty();++i)action(w,"editorStep");
        require(!w.runState().vehicles.empty(),"authored demand produced no vehicles");
        require(c->renderedVehicles()==w.runState().vehicles.size(),"vehicle layer did not receive frame");
        const auto checkpoint=checkpointJson(w.runState());
        const auto sampledTick=w.runState().tick;
        const auto revision=w.history().revision();
        action(w,"editorReset");require(w.runState().tick==0,"reset");
        for(std::uint64_t i=0;i<sampledTick;++i)action(w,"editorStep");
        require(checkpointJson(w.runState())==checkpoint,"reset/seed replay drift");
        action(w,"editorRun");QTest::qWait(180);
        require(w.runState().tick>sampledTick,"timer did not step");
        QTest::keyClick(c,Qt::Key_Escape);const auto stopped=w.runState().tick;QTest::qWait(150);
        require(w.runState().tick==stopped,"Escape did not pause");
        const auto frozen=w.runState();action(w,"editorUndo");
        require(!w.runState().scenario,"edit left stale run active");
        require(frozen.scenario && frozen.tick==stopped,"published snapshot mutated");
        action(w,"editorRedo");require(w.history().revision()==revision,"redo changed revision");
        w.autosaveNow();require(QFile::exists(w.recoveryPath()),"autosave missing");
        bool locked=false;try{w.recoverFile(w.recoveryPath());}catch(const std::exception&){locked=true;}
        require(locked,"active recovery was not locked");
        const auto copy=directory.path()+"/recovered.traffic.json";require(QFile::copy(w.recoveryPath(),copy),"copy crash fixture");
        const auto original=documentJson(w.history().document());
        w.recoverFile(copy);require(w.history().dirty(),"recovery incorrectly marked clean");
        require(documentJson(w.history().document())==original,"recovery changed authored data");
        const auto saved=directory.path()+QString::fromUtf8("/โครงการ.traffic.json");
        w.saveFile(saved);require(!w.history().dirty(),"save after recovery");
        require(!QFile::exists(copy),"successful save left consumed recovery copy");
        item<QComboBox>(w,"editorDrivingSide")->setCurrentIndex(1);w.autosaveNow();
        require(QFile::exists(w.recoveryPath()),"dirty edit did not create recovery");
        action(w,"editorUndo");require(!w.history().dirty(),"Undo did not reach save point");
        w.autosaveNow();require(!QFile::exists(w.recoveryPath()),"clean revision retained stale recovery");
        w.openFile(saved);action(w,"editorStep");require(w.runState().tick==1,"reopened project cannot run");
        item<QComboBox>(w,"editorLanguage")->setCurrentIndex(1);
        require(item<QAction>(w,"editorRun")->text().contains(QString::fromUtf8("จำลอง")),"Thai run label missing");
        require(item<QLabel>(w,"editorRunInfo")->text().contains(QString::fromUtf8("รุ่น")),"Thai revision missing");
        item<QLineEdit>(w,"editorSeed")->setText("4294967296");action(w,"editorRun");
        require(!w.runState().scenario,"invalid seed ran");
        w.close();
        // A window that cannot take its recovery lock keeps every unrelated control and stays
        // usable; only autosave is withheld. Point the data location at a regular file so the
        // lock cannot be created for any user, including root.
        const auto blocked=directory.path()+"/not-a-directory";
        {QFile f(blocked);require(f.open(QIODevice::WriteOnly),"blocking file");f.write("x");}
        const auto previous=qgetenv("XDG_DATA_HOME");
        qputenv("XDG_DATA_HOME",blocked.toUtf8());
        {
            EditorWindow locked{std::filesystem::path(argv[1])};locked.show();QTest::qWait(30);
            require(locked.findChild<QAction*>("editorEmbedCatalogs"),"lock failure removed catalog embedding");
            require(locked.findChild<QAction*>("editorRecover"),"lock failure removed recovery browsing");
            require(!locked.autosaveActive(),"autosave ran without a recovery lock");
            require(locked.recoveryPath().isEmpty(),"unlocked window kept a recovery path");
            item<QSpinBox>(locked,"editorLaneCount")->setValue(1);
            require(!locked.history().dirty(),"fresh window started dirty");
            locked.close();
        }
        previous.isEmpty()?qunsetenv("XDG_DATA_HOME"):qputenv("XDG_DATA_HOME",previous);
        std::cout<<"M1 demand, run, replay and recovery workflow passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
