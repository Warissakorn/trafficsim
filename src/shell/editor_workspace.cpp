#include "editor_window.hpp"
#include "editor_style.hpp"
#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QDockWidget>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QToolButton>
namespace trafficsim {
void EditorWindow::buildWorkspace() {
    auto* palette=findChild<QDockWidget*>("editorPaletteDock");
    palette->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    actions_["editorNetworkObjects"]=palette->toggleViewAction();
    actions_.at("editorNetworkObjects")->setShortcut(QKeySequence("Ctrl+Shift+T"));
    auto* focus=action("editorFocusCanvas",QKeySequence("Ctrl+Shift+F"),[]{});focus->setCheckable(true);
    connect(focus,&QAction::toggled,this,&EditorWindow::focusCanvas);
    action("editorResetLayout",{},[this]{resetWorkspace();});
    action("editorShortcuts",QKeySequence::HelpContents,[this]{
        QMessageBox::information(this,text("editorShortcuts"),text("editorKeyboardHelp")+"\n\n"+text("editorHelp"));
    });
    const auto menu=[&](const char* key,std::initializer_list<const char*> entries){
        auto* m=menuBar()->addMenu(QString());m->setObjectName(key);texts_[key]=m;
        for(const auto* entry:entries){if(!*entry)m->addSeparator();else m->addAction(actions_.at(entry));}
    };
    auto* files=findChild<QToolBar*>("editorFiles");
    for(const auto* key:{"editorRecover","editorEmbedCatalogs"})files->removeAction(actions_.at(key));
    menu("editorFileMenu",{"editorNew","editorOpen","editorSave","editorSaveAs","","editorRecover","editorEmbedCatalogs"});
    menu("editorEditMenu",{"editorUndo","editorRedo","","editorFinish","editorRotate","editorDeleteVertex","editorDeleteLink","editorDeleteSelected"});
    menu("editorViewMenu",{"editorFit","editorSnap","editorToggleBackground","","editorNetworkObjects","editorInspector","editorObjects","editorHistory","","editorFocusCanvas","editorResetLayout"});
    menu("editorSimulationMenu",{"editorRun","editorStep","editorReset","editorRunSettings","editorRecheck"});
    menu("editorHelpMenu",{"editorShortcuts"});
    // Menu alternatives keep widget controls reachable even at extreme toolbar widths.
    auto* languages=new QActionGroup(this);
    for(int index=0;index<language_->count();++index) {
        auto* choose=action(index==0?"english":"thai",{},[this,index]{language_->setCurrentIndex(index);});
        choose->setCheckable(true);languages->addAction(choose);
    }
    menu("language",{"english","thai"});
    connect(language_,&QComboBox::currentIndexChanged,this,[this](int index){actions_.at(index==0?"english":"thai")->setChecked(true);});
    actions_.at(language_->currentIndex()==0?"english":"thai")->setChecked(true);
    const std::pair<const char*,EditorIcon> icons[]={
        {"editorNew",EditorIcon::document},{"editorOpen",EditorIcon::open},{"editorSave",EditorIcon::save},
        {"editorUndo",EditorIcon::undo},{"editorRedo",EditorIcon::redo},{"editorFit",EditorIcon::fit},
        {"editorFinish",EditorIcon::finish},{"editorRotate",EditorIcon::rotate},{"editorDeleteSelected",EditorIcon::remove},
        {"editorSnap",EditorIcon::grid},{"editorRun",EditorIcon::run},{"editorStep",EditorIcon::step},
        {"editorReset",EditorIcon::reset},{"editorInspector",EditorIcon::inspector},{"editorObjects",EditorIcon::objects},
        {"editorFocusCanvas",EditorIcon::focus},{"editorNetworkObjects",EditorIcon::link}
    };
    for(const auto& [key,icon]:icons)actions_.at(key)->setIcon(editorIcon(icon));
    auto* workspace=addToolBar(QString());workspace->setObjectName("editorWorkspace");texts_["editorWorkspace"]=workspace;
    auto* spacer=new QWidget(workspace);spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);workspace->addWidget(spacer);
    for(const auto* key:{"editorInspector","editorObjects","editorFocusCanvas"})workspace->addAction(actions_.at(key));
    workspace->addSeparator();workspace->addWidget(language_);
    for(auto* bar:findChildren<QToolBar*>(QString(),Qt::FindDirectChildrenOnly)) {
        bar->setMovable(false);bar->setFloatable(false);bar->setIconSize(QSize(20,20));bar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    auto* runBar=findChild<QToolBar*>("editorRunToolbar");
    if(auto* button=qobject_cast<QToolButton*>(runBar->widgetForAction(actions_.at("editorRun")))) {
        button->setObjectName("editorRunButton");button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    for(const auto* key:{"editorNetworkObjects","editorInspector","editorObjects","editorHistory","editorFocusCanvas","editorShortcuts"}) {
        actions_.at(key)->setObjectName(key);addAction(actions_.at(key));
    }
    for(auto* combo:{objectLevel_,objectDisplay_,side_,visibleLevel_}) {
        combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);combo->setMinimumContentsLength(10);
    }
    for(auto* tabs:{properties_,objects_}){tabs->setDocumentMode(true);tabs->setUsesScrollButtons(true);tabs->setElideMode(Qt::ElideRight);}
    for(auto* table:findChildren<QTableWidget*>()) {
        table->setAlternatingRowColors(true);table->setShowGrid(false);table->verticalHeader()->setDefaultSectionSize(28);
    }
    for(auto* dock:findChildren<QDockWidget*>(QString(),Qt::FindDirectChildrenOnly))
        connect(dock,&QDockWidget::visibilityChanged,this,[this,dock](bool visible){
            if(visible&&!changingWorkspace_&&actions_.at("editorFocusCanvas")->isChecked()) {
                actions_.at("editorFocusCanvas")->setChecked(false);dock->show();
            }
        });
    resetWorkspace();
}
void EditorWindow::layoutToolbars() {
    if(auto* run=findChild<QToolBar*>("editorRunToolbar")) {
        const bool stacked=width()<1180;
        if(stacked!=toolBarBreak(run)){if(stacked)insertToolBarBreak(run);else removeToolBarBreak(run);}
    }
}
void EditorWindow::resizeEvent(QResizeEvent* event) {QMainWindow::resizeEvent(event);layoutToolbars();}
void EditorWindow::focusCanvas(bool enabled) {
    changingWorkspace_=true;
    if(enabled) {
        workspaceBeforeFocus_=saveState();
        for(auto* dock:findChildren<QDockWidget*>(QString(),Qt::FindDirectChildrenOnly))dock->hide();
    } else if(!workspaceBeforeFocus_.isEmpty()) {restoreState(workspaceBeforeFocus_);workspaceBeforeFocus_.clear();}
    layoutToolbars();changingWorkspace_=false;canvas_->setFocus();
}
void EditorWindow::resetWorkspace() {
    changingWorkspace_=true;
    {const QSignalBlocker blocked(actions_.at("editorFocusCanvas"));actions_.at("editorFocusCanvas")->setChecked(false);}
    workspaceBeforeFocus_.clear();
    const auto dock=[this](const char* name){return findChild<QDockWidget*>(name);};
    auto* palette=dock("editorPaletteDock");auto* inspector=dock("editorInspectorDock");
    auto* objects=dock("editorObjectsDock");auto* history=dock("editorHistoryDock");
    for(auto* panel:{palette,inspector,objects,history}){panel->setFloating(false);removeDockWidget(panel);}
    addDockWidget(Qt::LeftDockWidgetArea,palette);addDockWidget(Qt::RightDockWidgetArea,inspector);
    addDockWidget(Qt::RightDockWidgetArea,history);tabifyDockWidget(inspector,history);addDockWidget(Qt::BottomDockWidgetArea,objects);
    for(auto* panel:{palette,inspector,objects})panel->show();history->hide();inspector->raise();
    resizeDocks({palette,inspector},{180,310},Qt::Horizontal);resizeDocks({objects},{185},Qt::Vertical);
    layoutToolbars();changingWorkspace_=false;canvas_->setFocus();
}
}
