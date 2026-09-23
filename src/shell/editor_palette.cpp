#include "editor_window.hpp"
#include "editor_style.hpp"
#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>
namespace trafficsim {
void EditorWindow::buildPalette() {
    auto* dock=new QDockWidget(this);dock->setObjectName("editorPaletteDock");texts_["editorNetworkObjects"]=dock;
    dock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* body=new QWidget(dock);auto* layout=new QVBoxLayout(body);
    layout->setContentsMargins(6,8,6,8);layout->setSpacing(6);
    palette_=new QListWidget(body);palette_->setObjectName("editorObjectPalette");layout->addWidget(palette_,1);
    palette_->setIconSize(QSize(20,20));palette_->setSpacing(2);
    palette_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    palette_->setTextElideMode(Qt::ElideRight);palette_->setMinimumWidth(0);
    const int modes[]={0,1,5,6,7,8,2,3,4};
    const char* shortcuts[]={"S","L","C","R","V","H","X","M","K"};
    const EditorIcon icons[]={EditorIcon::select,EditorIcon::link,EditorIcon::connector,EditorIcon::route,
        EditorIcon::input,EditorIcon::signal,EditorIcon::split,EditorIcon::measure,EditorIcon::image};
    for(int row=0;row<9;++row){
        auto* entry=new QListWidgetItem(palette_);entry->setData(Qt::UserRole,modes[row]);entry->setIcon(editorIcon(icons[row]));
        auto* key=action("editorTool"+std::to_string(modes[row]),QKeySequence(shortcuts[row]),[this,row]{palette_->setCurrentRow(row);canvas_->setFocus();});
        key->setShortcutContext(Qt::WidgetWithChildrenShortcut);canvas_->addAction(key);
    }
    visibleLevel_=new QComboBox(body);visibleLevel_->setObjectName("editorVisibleLevel");
    auto* label=new QLabel(body);texts_["editorVisibleLevel"]=label;layout->addWidget(label);layout->addWidget(visibleLevel_);
    auto* help=new QLabel(body);help->setWordWrap(true);help->setObjectName("editorPaletteHint");texts_["editorPaletteHint"]=help;layout->addWidget(help);
    connect(palette_,&QListWidget::currentRowChanged,this,[this](int row){
        if(row<0)return;tool_->setCurrentIndex(palette_->item(row)->data(Qt::UserRole).toInt());canvas_->setFocus();
    });
    connect(tool_,&QComboBox::currentIndexChanged,this,[this](int mode){
        const QSignalBlocker blocked(palette_);
        for(int row=0;row<palette_->count();++row)if(palette_->item(row)->data(Qt::UserRole).toInt()==mode)palette_->setCurrentRow(row);
    });
    connect(visibleLevel_,&QComboBox::currentIndexChanged,this,[this]{
        if(visibleLevel_->currentData().isValid())canvas_->setVisibleLevel(visibleLevel_->currentData().toInt());
        else canvas_->setVisibleLevel({});
    });
    canvas_->visibleLevelChanged=[this](std::optional<int> level){
        const QSignalBlocker block(visibleLevel_);
        if (level && visibleLevel_->findData(*level)<0)
            visibleLevel_->addItem(QString::number(*level),*level);
        visibleLevel_->setCurrentIndex(level?visibleLevel_->findData(*level):0);
    };
    auto* background=action("editorToggleBackground",QKeySequence("Ctrl+B"),[this]{
        canvas_->setBackgroundVisible(actions_.at("editorToggleBackground")->isChecked());
    });
    background->setCheckable(true);background->setChecked(true);addAction(background);
    auto* button=new QToolButton(body);button->setDefaultAction(background);button->setToolButtonStyle(Qt::ToolButtonTextOnly);layout->addWidget(button);
    auto redo=QKeySequence::keyBindings(QKeySequence::Redo);
    if(!redo.contains(QKeySequence("Ctrl+Y")))redo.push_back(QKeySequence("Ctrl+Y"));
    actions_.at("editorRedo")->setShortcuts(redo);
    palette_->setCurrentRow(0);dock->setWidget(body);addDockWidget(Qt::LeftDockWidgetArea,dock);
    resizeDocks({dock},{190},Qt::Horizontal);
}
void EditorWindow::translatePalette() {
    canvas_->setToolTip(text("editorKeyboardHelp"));
    texts_.at("editorPaletteHint")->setToolTip(text("editorGestureHelp"));
    palette_->setAccessibleName(text("editorNetworkObjects"));
    visibleLevel_->setAccessibleName(text("editorVisibleLevel"));
    canvas_->setAccessibleDescription(text("editorKeyboardHelp"));
    const char* keys[]={"editorSelect","editorDraw","editorSplit","editorMeasure","editorCalibrate","editorConnect","editorRouteTable","editorInputTable","editorSignalTable"};
    const char* shortcuts[]={"S","L","X","M","K","C","R","V","H"};
    for(int row=0;row<palette_->count();++row){
        const int mode=palette_->item(row)->data(Qt::UserRole).toInt();
        palette_->item(row)->setText(text(keys[mode])+" ("+shortcuts[mode]+")");
        palette_->item(row)->setToolTip(palette_->item(row)->text());
        actions_.at("editorTool"+std::to_string(mode))->setText(text(keys[mode]));
    }
    const QSignalBlocker block(visibleLevel_);const auto selected=visibleLevel_->currentData();visibleLevel_->clear();
    visibleLevel_->addItem(text("editorAllLevels"));
    const auto lang=language_->currentData().toString().toStdString();
    for(const auto& level:displayCatalog_.levels)visibleLevel_->addItem(QString::number(level.order)+" · "+QString::fromStdString(level.name.at(lang)),level.order);
    if(selected.isValid() && visibleLevel_->findData(selected)<0)
        visibleLevel_->addItem(QString::number(selected.toInt()),selected);
    visibleLevel_->setCurrentIndex(selected.isValid()?visibleLevel_->findData(selected):0);
}
void EditorWindow::buildAppearance(QFormLayout* form) {
    objectLevel_=new QComboBox(this);objectDisplay_=new QComboBox(this);
    label(form,"editorObjectLevel",objectLevel_);label(form,"editorDisplayType",objectDisplay_);
    auto* apply=new QToolButton(this);apply->setDefaultAction(action("editorApplyDisplay",{},[this]{
        execute("editorApplyDisplay",[&](auto& d){changeAppearance(d,canvas_->selected(),objectLevel_->currentData().toInt(),objectDisplay_->currentData().toString().toStdString());});
    }));form->addRow(apply);
}
void EditorWindow::refreshAppearance() {
    int level=objectLevel_->currentData().isValid()?objectLevel_->currentData().toInt():0;
    auto type=objectDisplay_->currentData().isValid()?objectDisplay_->currentData().toString().toStdString():std::string("default");
    bool chosen=false;
    for(const auto& l:history_.document().network.links)if(l.id==canvas_->selected()){level=l.level;type=l.displayType;chosen=true;}
    if(const auto* c=canvas_->selectedConnector()){level=c->level;type=c->displayType;chosen=true;}
    const QSignalBlocker a(objectLevel_),b(objectDisplay_);objectLevel_->clear();objectDisplay_->clear();
    const auto lang=language_->currentData().toString().toStdString();
    for(const auto& l:displayCatalog_.levels)objectLevel_->addItem(QString::number(l.order)+" · "+QString::fromStdString(l.name.at(lang)),l.order);
    if(objectLevel_->findData(level)<0)objectLevel_->addItem(QString::number(level),level);
    objectLevel_->setCurrentIndex(objectLevel_->findData(level));
    for(const auto& t:displayCatalog_.types)objectDisplay_->addItem(QString::fromStdString(t.name.at(lang)),QString::fromStdString(t.id));
    if(objectDisplay_->findData(QString::fromStdString(type))<0)
        objectDisplay_->addItem(QString::fromStdString(type)+" "+text("editorMissingStyle"),QString::fromStdString(type));
    objectDisplay_->setCurrentIndex(objectDisplay_->findData(QString::fromStdString(type)));
    actions_.at("editorApplyDisplay")->setEnabled(chosen);
}
}
