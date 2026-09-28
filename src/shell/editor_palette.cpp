#include "editor_window.hpp"
#include "editor_style.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QAction>
#include <QComboBox>
#include <QColor>
#include <QDockWidget>
#include <QFont>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QAbstractItemView>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>
#include <initializer_list>
namespace trafficsim {
void EditorWindow::buildPalette() {
    auto* dock=new QDockWidget(this);dock->setObjectName("editorPaletteDock");texts_["editorNetworkObjects"]=dock;
    dock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* body=new QWidget(dock);auto* layout=new QVBoxLayout(body);
    layout->setContentsMargins(editorDesign::space2,editorDesign::space2,editorDesign::space2,editorDesign::space2);
    layout->setSpacing(editorDesign::space1);
    palette_=new QTreeWidget(body);palette_->setObjectName("editorObjectPalette");
    palette_->setHeaderHidden(true);palette_->setRootIsDecorated(true);palette_->setIndentation(editorDesign::space3);
    palette_->setUniformRowHeights(true);palette_->setSelectionMode(QAbstractItemView::SingleSelection);
    palette_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);layout->addWidget(palette_,1);
    palette_->setIconSize(QSize(editorDesign::space4,editorDesign::space4));
    palette_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    palette_->setTextElideMode(Qt::ElideRight);palette_->setMinimumWidth(0);
    struct Group {const char* key;std::initializer_list<int> modes;};
    const Group groups[]={
        {"editorPaletteGeometry",{0,1,5,2}},
        {"editorPaletteControl",{6,7,8,9,10}},
        {"editorPaletteTools",{3,4}}
    };
    const char* shortcuts[]={"S","L","X","M","K","C","R","V","H","A","Q"};
    const EditorIcon icons[]={EditorIcon::select,EditorIcon::link,EditorIcon::split,EditorIcon::measure,
        EditorIcon::image,EditorIcon::connector,EditorIcon::route,EditorIcon::input,EditorIcon::signal,
        EditorIcon::conflict,EditorIcon::counter};
    for(const auto& group:groups){
        auto* root=new QTreeWidgetItem(palette_);root->setData(0,Qt::UserRole,QString::fromLatin1(group.key));
        root->setFlags(Qt::ItemIsEnabled);root->setData(0,Qt::UserRole+1,true);
        auto groupFont=root->font(0);groupFont.setPixelSize(editorDesign::fontSizeLabel);groupFont.setBold(true);
        groupFont.setLetterSpacing(QFont::AbsoluteSpacing,1);root->setFont(0,groupFont);
        root->setForeground(0,QColor(editorDesign::gray7));
        for(const int mode:group.modes){
            auto* entry=new QTreeWidgetItem(root);entry->setData(0,Qt::UserRole,mode);
            entry->setIcon(0,editorIcon(icons[mode]));entry->setData(0,Qt::UserRole+2,QString::fromLatin1(shortcuts[mode]));
            paletteItems_[mode]=entry;
            auto* key=action("editorTool"+std::to_string(mode),QKeySequence(shortcuts[mode]),[this,mode]{
                palette_->setCurrentItem(paletteItems_.at(mode));canvas_->setFocus();
            });
            key->setShortcutContext(Qt::WidgetWithChildrenShortcut);canvas_->addAction(key);
        }
    }
    visibleLevel_=new QComboBox(body);visibleLevel_->setObjectName("editorVisibleLevel");
    auto* label=new QLabel(body);label->setProperty("editorEyebrow",true);texts_["editorVisibleLevel"]=label;layout->addWidget(label);layout->addWidget(visibleLevel_);
    connect(palette_,&QTreeWidget::currentItemChanged,this,[this](QTreeWidgetItem* current){
        if(!current||current->data(0,Qt::UserRole+1).toBool())return;
        tool_->setCurrentIndex(current->data(0,Qt::UserRole).toInt());
    });
    connect(tool_,&QComboBox::currentIndexChanged,this,[this](int mode){
        const QSignalBlocker blocked(palette_);if(paletteItems_.contains(mode))palette_->setCurrentItem(paletteItems_.at(mode));
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
    auto* button=new QToolButton(body);button->setObjectName("editorBackgroundButton");button->setDefaultAction(background);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);layout->addWidget(button);
    auto redo=QKeySequence::keyBindings(QKeySequence::Redo);
    if(!redo.contains(QKeySequence("Ctrl+Y")))redo.push_back(QKeySequence("Ctrl+Y"));
    actions_.at("editorRedo")->setShortcuts(redo);
    for(int index=0;index<palette_->topLevelItemCount();++index)palette_->expandItem(palette_->topLevelItem(index));
    palette_->setCurrentItem(paletteItems_.at(0));dock->setWidget(body);addDockWidget(Qt::LeftDockWidgetArea,dock);
    resizeDocks({dock},{224},Qt::Horizontal);
}
void EditorWindow::translatePalette() {
    canvas_->setToolTip(text("editorKeyboardHelp"));palette_->setToolTip(text("editorGestureHelp"));
    palette_->setAccessibleName(text("editorNetworkObjects"));
    visibleLevel_->setAccessibleName(text("editorVisibleLevel"));
    canvas_->setAccessibleDescription(text("editorKeyboardHelp"));
    const char* modes[]={"editorSelect","editorDraw","editorSplit","editorMeasure","editorCalibrate","editorConnect","editorRouteTable","editorInputTable","editorSignalTable","editorConflictTool","editorCounterTool"};
    for(const auto& [mode,entry]:paletteItems_){
        const auto label=text(modes[mode]);const auto shortcut=entry->data(0,Qt::UserRole+2).toString();
        entry->setText(0,label+"  "+shortcut);entry->setToolTip(0,label+" · "+shortcut);
        actions_.at("editorTool"+std::to_string(mode))->setText(label);
    }
    for(int index=0;index<palette_->topLevelItemCount();++index){
        auto* root=palette_->topLevelItem(index);const bool english=language_->currentData().toString()=="en";
        auto title=text(root->data(0,Qt::UserRole).toString().toStdString());
        if(english)title=title.toUpper();
        auto font=root->font(0);font.setLetterSpacing(QFont::AbsoluteSpacing,english?1:0);root->setFont(0,font);
        root->setText(0,title);
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
