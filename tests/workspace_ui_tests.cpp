#include "../src/shell/editor_window.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QDialog>
#include <QHeaderView>
#include <QLineEdit>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QSpinBox>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTabWidget>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T> T* item(QObject& root,const char* name){auto* value=root.findChild<T*>(name);require(value,"Missing workspace widget");return value;}
void settle(){QApplication::processEvents();QTest::qWait(20);}
void trigger(EditorWindow& window,const char* name){item<QAction>(window,name)->trigger();settle();}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>=2,"Expected data directory");const std::filesystem::path data(argv[1]);
        EditorWindow window(data);window.resize(1360,860);window.show();settle();
        window.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
        auto* canvas=window.canvas();canvas->select(window.history().document().network.links.front().id);canvas->fitNetwork();settle();
        const auto revision=window.history().revision();const auto selection=canvas->selected();
        const auto defaultCanvas=canvas->viewport()->size();
        auto* palette=item<QDockWidget>(window,"editorPaletteDock");
        auto* toolTree=item<QTreeWidget>(window,"editorObjectPalette");
        auto* inspector=item<QDockWidget>(window,"editorInspectorDock");
        auto* objects=item<QDockWidget>(window,"editorObjectsDock");
        auto* history=item<QDockWidget>(window,"editorHistoryDock");
        require(palette->isVisible()&&inspector->isVisible()&&objects->isVisible(),"Default panels missing");
        require(toolTree->topLevelItemCount()==3,"Tool tree categories missing");
        require(item<QTableWidget>(window,"editorLinkTable")->verticalHeader()->defaultSectionSize()==28,"Object row density changed");
        auto* laneCount=item<QSpinBox>(window,"editorLaneCount");
        require(laneCount->font().fixedPitch()&&laneCount->font().pixelSize()==12,"Numeric control is not tabular monospace");
        auto* laneCountEdit=laneCount->findChild<QLineEdit*>();require(laneCountEdit,"Lane count editor missing");
        laneCountEdit->setText("13");settle();
        bool rangeShown=false;
        for(auto* hint:window.findChildren<QLabel*>("editorRangeHint"))
            rangeShown=rangeShown||(hint->isVisible()&&hint->text().contains("1–12"));
        require(rangeShown,"Invalid lane count did not show its valid range");laneCount->setValue(2);
        auto* laneWidths=item<QLineEdit>(window,"editorLaneWidths");laneWidths->setText("not a width");settle();
        bool widthRangeShown=false;
        for(auto* hint:window.findChildren<QLabel*>("editorLaneWidthsRange"))
            widthRangeShown=widthRangeShown||(hint->isVisible()&&hint->text().contains("> 0 m"));
        require(widthRangeShown,"Invalid lane width did not explain its valid range");laneWidths->clear();
        toolTree->setFocus();QTest::keyClick(toolTree,Qt::Key_Tab);settle();
        require(item<QComboBox>(window,"editorVisibleLevel")->hasFocus(),"Tab order skipped the level filter");
        QTest::keyClick(item<QComboBox>(window,"editorVisibleLevel"),Qt::Key_Tab);settle();
        require(item<QToolButton>(window,"editorBackgroundButton")->hasFocus(),"Tab order skipped the background toggle");
        QTest::keyClick(item<QToolButton>(window,"editorBackgroundButton"),Qt::Key_Tab);settle();
        require(canvas->hasFocus(),"Tab order did not move from the tree to the canvas");
        QTest::keyClick(canvas,Qt::Key_Tab);settle();
        require(item<QLineEdit>(window,"editorId")->hasFocus(),"Tab order did not move from the canvas to the inspector");
        canvas->setFocus();QTest::keyClick(canvas,Qt::Key_K,Qt::ControlModifier);settle();
        auto* commandDialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
        require(commandDialog&&commandDialog->objectName()=="editorCommandPaletteDialog","Ctrl+K did not open command palette");
        QPointer<QDialog> commandGuard(commandDialog);
        auto* commandSearch=item<QLineEdit>(*commandDialog,"editorCommandSearch");
        commandSearch->setText("Draw link");settle();QTest::keyClick(commandSearch,Qt::Key_Return);settle();
        require(commandGuard.isNull()&&item<QComboBox>(window,"editorTool")->currentIndex()==1,
                "Command search did not activate the Link tool");
        require(defaultCanvas.width()>=700&&defaultCanvas.height()>=350,"Default canvas crowded out");
        const int top=item<QToolBar>(window,"editorFiles")->y();
        for(const auto* name:{"editorTools","editorRunToolbar","editorWorkspace"})
            require(item<QToolBar>(window,name)->y()==top,"Wide toolbar wrapped into another row");
        // A user's hidden, moved and floating panels must survive focus mode.
        palette->hide();window.addDockWidget(Qt::LeftDockWidgetArea,inspector);history->setFloating(true);history->show();settle();
        canvas->setFocus();QTest::keyClick(canvas,Qt::Key_F,Qt::ControlModifier|Qt::ShiftModifier);settle();
        require(item<QAction>(window,"editorFocusCanvas")->isChecked(),"Focus shortcut did not activate");
        for(auto* panel:{palette,inspector,objects,history})require(!panel->isVisible(),"Focus left a panel visible");
        require(canvas->viewport()->width()>defaultCanvas.width()+250,"Focus did not reclaim side panels");
        require(canvas->viewport()->height()>defaultCanvas.height()+100,"Focus did not reclaim bottom panel");
        require(item<QLabel>(window,"editorScope")->isVisible(),"Validation marker disappeared in focus mode");
        trigger(window,"editorFocusCanvas");
        require(!palette->isVisible()&&inspector->isVisible()&&objects->isVisible(),"Focus forgot visibility");
        require(history->isFloating()&&history->isVisible(),"Focus forgot a floating panel");
        require(window.dockWidgetArea(inspector)==Qt::LeftDockWidgetArea,"Focus forgot panel placement");
        trigger(window,"editorFocusCanvas");trigger(window,"editorInspector");
        require(!item<QAction>(window,"editorFocusCanvas")->isChecked()&&inspector->isVisible(),"Opening a panel did not exit focus");
        trigger(window,"editorResetLayout");
        require(window.dockWidgetArea(inspector)==Qt::RightDockWidgetArea&&!history->isFloating(),"Reset left custom docking behind");
        require(palette->isVisible()&&objects->isVisible()&&!history->isVisible(),"Reset did not restore defaults");
        require(window.history().revision()==revision&&canvas->selected()==selection,"Layout changed the document or selection");
        auto* tabs=item<QTabWidget>(window,"editorPropertyTabs");
        for(int i=0;i<tabs->count();++i)require(qobject_cast<QScrollArea*>(tabs->widget(i)),"Property page is not scrollable");
        auto* language=item<QComboBox>(window,"editorLanguage");
        for(int lang:{0,1}) {
            language->setCurrentIndex(lang);window.resize(1024,768);settle();
            require(window.width()<=1024&&window.height()<=768,"Localized content forced the window larger");
            require(canvas->viewport()->width()>=400&&canvas->viewport()->height()>=250,"Small-window canvas is unusable");
            require(language->isVisible()&&window.rect().contains(QRect(language->mapTo(&window,QPoint()),language->size())),"Language selector outside window");
            require(item<QMenu>(window,"language")->actions().size()==2,"Language fallback missing");
            auto* scope=item<QLabel>(window,"editorScope");
            require(scope->wordWrap()&&scope->isVisible()&&!scope->text().isEmpty(),"Scope marker lost");
            auto* menu=item<QMenu>(window,"editorViewMenu");require(!menu->title().isEmpty(),"Workspace menu untranslated");
            require(menu->actions().contains(item<QAction>(window,"editorFocusCanvas")),"Focus missing from menu");
            require(item<QMenu>(window,"editorSimulationMenu")->actions().contains(item<QAction>(window,"editorRunSettings")),"Run settings unreachable");
            for(int i=0;i<tabs->count();++i){tabs->setCurrentIndex(i);settle();require(tabs->isVisible(),"Tab navigation disappeared");}
        }
        require(item<QAction>(window,"editorFocusCanvas")->text().contains(QString::fromUtf8("แผนที่")),"Thai focus label missing");
        for(int i=0;i<100;++i)item<QAction>(window,"editorStep")->trigger();settle();
        auto* info=item<QLabel>(window,"editorRunInfo");
        require(info->wordWrap()&&info->isVisible()&&window.runState().tick==100,"Run summary or stepping regressed");
        const auto tick=window.runState().tick;
        trigger(window,"editorFocusCanvas");window.resize(1360,860);settle();trigger(window,"editorFocusCanvas");
        require(!window.toolBarBreak(item<QToolBar>(window,"editorRunToolbar")),"Focus restored a stale toolbar breakpoint");
        require(window.runState().tick==tick,"Layout reset the run");
        if(argc>2) {
            tabs->setCurrentIndex(0);language->setCurrentIndex(0);settle();
            std::cout<<"English canvas: "<<canvas->viewport()->width()<<" x "<<canvas->viewport()->height()<<'\n';
            require(window.grab().save(QString::fromUtf8(argv[2])+"-en.png"),"English screenshot failed");
            language->setCurrentIndex(1);window.resize(1024,768);settle();
            std::cout<<"Thai canvas: "<<canvas->viewport()->width()<<" x "<<canvas->viewport()->height()<<'\n';
            require(window.grab().save(QString::fromUtf8(argv[2])+"-th.png"),"Thai screenshot failed");
            trigger(window,"editorFocusCanvas");
            require(window.grab().save(QString::fromUtf8(argv[2])+"-focus.png"),"Focus screenshot failed");
        }
        std::cout<<"Workspace layout, focus restoration, bilingual sizing and playback passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
