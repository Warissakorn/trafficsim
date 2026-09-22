#include "editor_window.hpp"
#include "path.hpp"
#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QLockFile>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QAbstractButton>
#include <QTabWidget>
#include <cmath>

namespace trafficsim {
EditorWindow::EditorWindow(const std::filesystem::path& data,const QString& language,QWidget* parent) : QMainWindow(parent), data_(data) {
    setObjectName("networkEditor");displayCatalog_=loadDisplayCatalog(data);
    const int font=QFontDatabase::addApplicationFont(displayPath(data/"fonts/NotoSansThai.ttf"));
    if(font<0) throw std::runtime_error("Cannot load bundled Thai font");
    setFont(QFont(QFontDatabase::applicationFontFamilies(font).front(),10));
    for(const auto* code:{"en","th"}) {
        QFile f(displayPath(data/"locales"/(std::string(code)+".json")));
        if(!f.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot load locale");
        locales_[code]=QJsonDocument::fromJson(f.readAll()).object();
    }
    language_=new QComboBox(this); language_->setObjectName("editorLanguage");
    language_->addItem(locales_.at("en").value("english").toString(),"en");
    language_->addItem(locales_.at("en").value("thai").toString(),"th"); language_->setCurrentIndex(language=="th"?1:0);
    auto* central=new QWidget(this); auto* layout=new QVBoxLayout(central);
    auto* scope=new QLabel(central); scope->setWordWrap(true); texts_["editorScope"]=scope; layout->addWidget(scope);
    scope->setStyleSheet("background:#fff3cd;color:#614700;padding:8px;");
    canvas_=new EditorCanvas(central);canvas_->setDisplayCatalog(displayCatalog_); layout->addWidget(canvas_,1);
    error_=new QLabel(central); error_->setObjectName("editorError"); error_->setWordWrap(true);
    error_->setStyleSheet("color:#a5263c"); layout->addWidget(error_); setCentralWidget(central);
    auto* files=addToolBar(QString());texts_["editorFiles"]=files; files->setObjectName("editorFiles");
    files->addAction(action("editorNew",QKeySequence::New,[this]{ if(confirmDiscard()){ clearRecovery(); clearRun(); history_.reset(); file_.clear(); canvas_->select(""); refresh(); canvas_->fitNetwork(); } }));
    files->addAction(action("editorOpen",QKeySequence::Open,[this]{
        if(!confirmDiscard()) return;
        const auto file=QFileDialog::getOpenFileName(this,text("editorOpen"),{},text("editorFilter"));
        if(!file.isEmpty()) try{openFile(file);}catch(const std::exception& e){showError(e);}
    }));
    files->addAction(action("editorSave",QKeySequence::Save,[this]{saveDialog();}));
    files->addAction(action("editorSaveAs",QKeySequence::SaveAs,[this]{saveDialog(true);}));
    files->addSeparator();
    files->addAction(action("editorUndo",QKeySequence::Undo,[this]{clearRun();history_.undo();refresh();}));
    files->addAction(action("editorRedo",QKeySequence::Redo,[this]{clearRun();history_.redo();refresh();}));
    buildHistory(); files->addAction(actions_.at("editorHistory"));
    files->addWidget(language_);
    addToolBarBreak(); auto* tools=addToolBar(QString());texts_["editorTools"]=tools; tools->setObjectName("editorTools");
    tool_=new QComboBox(this); tool_->setObjectName("editorTool");
    for(int i=0;i<9;++i) tool_->addItem("",i);
    tool_->hide();
    tools->addAction(action("editorFinish",{},[this]{canvas_->finishDrawing();}));
    tools->addAction(action("editorFit",QKeySequence(Qt::Key_F),[this]{canvas_->fitNetwork();}));
    tools->addAction(action("editorRotate",QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_R),[this]{rotateSelection();}));
    tools->addAction(action("editorDeleteVertex",{},[this]{canvas_->removeVertex();}));
    tools->addAction(action("editorDeleteLink",{},[this]{
        const auto id=canvas_->selected(); if(id.empty()) return;
        QMessageBox box(QMessageBox::Question,text("editorDeleteLink"),text("editorDeleteWarning"),QMessageBox::Yes|QMessageBox::No,this);
        box.button(QMessageBox::Yes)->setText(text("editorConfirm"));box.button(QMessageBox::No)->setText(text("editorCancel"));box.setDefaultButton(QMessageBox::No);
        if(box.exec()==QMessageBox::Yes) execute("editorDeleteLink",[&](auto& d){deleteLink(d,id);});
    }));
    auto* snap=action("editorSnap",{},[this]{canvas_->snap=actions_.at("editorSnap")->isChecked();});
    snap->setCheckable(true); snap->setChecked(true); tools->addAction(snap);
    grid_=new QDoubleSpinBox(this); grid_->setRange(0.1,100); grid_->setValue(1); grid_->setSuffix(" m"); grid_->setObjectName("editorGrid"); tools->addWidget(grid_);
    coordinates_=new QLabel(this); statusBar()->addPermanentWidget(coordinates_);
    buildInspector(); tools->addAction(actions_.at("editorInspector"));
    tools->addAction(action("editorDeleteSelected",{},[this]{deleteSelected();}));
    buildObjectTables(); tools->addAction(actions_.at("editorObjects"));
    tools->addAction(actions_.at("editorRecheck"));
    connect(language_,&QComboBox::currentIndexChanged,this,[this]{translate();});
    connect(tool_,&QComboBox::currentIndexChanged,this,[this](int index){
        canvas_->setTool(static_cast<EditorCanvas::Tool>(index));
        if (index==5) properties_->setCurrentIndex(1);
        else if (index==4) properties_->setCurrentIndex(2);
        else if (index==1 || index==2) properties_->setCurrentIndex(0);
    });
    connect(grid_,&QDoubleSpinBox::valueChanged,this,[this](double n){canvas_->grid=n;canvas_->redraw();});
    canvas_->selectionChanged=[this]{
        refresh(false);
        if (!canvas_->selected().empty()) properties_->setCurrentIndex(canvas_->selectedConnector()?1:0);
    };
    canvas_->cursorMoved=[this](Point p){coordinates_->setText(QString("x %1 m   y %2 m").arg(p.x,0,'f',2).arg(p.y,0,'f',2));};
    canvas_->createLink=[this](const auto& points){
        std::string created; if(execute("editorDraw",[&](auto& d){created=addLink(d,points,count_->value(),width_->value());
            changeAppearance(d,created,objectLevel_->currentData().toInt(),objectDisplay_->currentData().toString().toStdString());})) canvas_->select(created);
    };
    canvas_->editGeometry=[this](const auto& id,const auto& points){
        const bool connector=canvas_->selectedConnector()!=nullptr;
        execute("editorGeometry",[&](auto& d){
            if (connector) changeConnectorGeometry(d,id,points); else changeGeometry(d,id,points);
        });
    };
    canvas_->createConnector=[this](const auto& from,const auto& to){addConnection(from,to);};
    canvas_->moveConnectorEndpoint=[this](bool leading,LaneReference ref){
        const auto* connector=canvas_->selectedConnector();if(!connector)return;
        const auto id=connector->id;
        const LaneReference from=leading?ref:connector->from, to=leading?connector->to:ref;
        execute("editorMoveConnectorEnd",[&](auto& d){changeConnectorEndpoints(d,id,from,to);});
    };
    canvas_->connectorSourcePicked=[this](const auto& lane){
        connectorFrom_->setCurrentIndex(connectorFrom_->findData(QString::fromStdString(lane.laneId)));
        connectorFromPosition_->setValue(attachmentStation(history_.document().network,lane,true));
        // These boxes double as the creation form. A fresh pick starts at one lane, like the
        // drag dialog, instead of inheriting the width of whatever Connector was selected.
        connectorFromCount_->setValue(1);connectorToCount_->setValue(1);
    };
    canvas_->connectorDraftChanged=[this]{connectorHint();};
    canvas_->splitAt=[this](const auto& id,double distance){
        std::string created; if(execute("editorSplit",[&](auto& d){created=splitLink(d,id,distance);})) canvas_->select(created);
    };
    canvas_->deleteRequested=[this]{deleteSelected();};
    canvas_->createLinkGesture=[this](const auto& points){createLinkDialog(points);};
    canvas_->createRangeGesture=[this](auto from,auto to,const auto& points){createRangeDialog(from,to,points);};
    canvas_->resizeRangeRequested=[this](int from,int to,bool leading){
        execute("editorApplyConnector",[&](auto& d){changeConnectorRange(d,canvas_->selected(),from,to,leading);});
    };
    canvas_->resizeLinkRequested=[this](int count,bool leading){
        execute("editorApplyLanes",[&](auto& d){resizeLinkLanes(d,canvas_->selected(),count,leading);});
    };
    canvas_->creationRejected=[this]{showError(std::invalid_argument("EDIT_CREATION_TARGET"));};
    canvas_->duplicateRequested=[this](Point delta){
        const auto ids=canvas_->selection();if(ids.empty())return;
        std::vector<std::string> copied;
        if(execute("editorDuplicate",[&](auto& d){copied=duplicateObjects(d,ids,delta);}))canvas_->setSelection(copied);
    };
    canvas_->translateRequested=[this](Point delta){
        const auto ids=canvas_->selection();if(ids.empty())return;
        execute("editorMove",[&](auto& d){translateObjects(d,ids,delta);});
    };
    canvas_->rotateRequested=[this](Point pivot,double degrees){
        const auto ids=canvas_->selection();
        execute("editorRotate",[&](auto& d){rotateObjects(d,ids,pivot,degrees);});
    };
    canvas_->createDemandGesture=[this](const auto& lane,auto mode){
        if(mode==EditorCanvas::Tool::route)editRoute({}, {lane.laneId});
        else if(mode==EditorCanvas::Tool::input)editInput();
        else editHead();
    };
    canvas_->measured=[this](Point a,Point b,bool calibration){measure(a,b,calibration);};
    buildDemandTables(); buildRunControls(); buildRecovery(); buildPalette();
    history_.reset(); translate(); refresh(); resize(1280,850); canvas_->centerOn(0,0);
}
QAction* EditorWindow::action(const std::string& key,const QKeySequence& shortcut,const std::function<void()>& run) {
    auto* a=new QAction(this); a->setObjectName(QString::fromStdString(key)); a->setShortcut(shortcut);
    actions_[key]=a; connect(a,&QAction::triggered,this,[run]{run();}); return a;
}
QString EditorWindow::text(const std::string& key) const { return locales_.at(language_->currentData().toString()).value(QString::fromStdString(key)).toString(); }
void EditorWindow::translate() {
    for(const auto& [key,a]:actions_) a->setText(text(key));
    for(const auto& [key,w]:texts_) {
        if(auto* label=qobject_cast<QLabel*>(w)) label->setText(text(key));
        else if(auto* dock=qobject_cast<QDockWidget*>(w)) dock->setWindowTitle(text(key));
        else if(auto* bar=qobject_cast<QToolBar*>(w)) bar->setWindowTitle(text(key));
    }
    const char* modes[]={"editorSelect","editorDraw","editorSplit","editorMeasure","editorCalibrate","editorConnect","editorRouteTable","editorInputTable","editorSignalTable"};
    for(int i=0;i<9;++i) tool_->setItemText(i,text(modes[i]));
    const char* tabs[]={"editorLinksTab","editorConnectorsTab","editorBackgroundTab"};
    for (int i=0;i<3;++i) properties_->setTabText(i,text(tabs[i]));
    side_->setItemText(0,text("editorLeft"));side_->setItemText(1,text("editorRight"));
    retranslateTables(); translateDemand(); translatePalette();
    canvas_->setAccessibleName(text("editorTitle")); grid_->setAccessibleName(text("editorGrid"));
    error_->clear(); refresh();
}
bool EditorWindow::execute(const std::string& name,const std::function<void(ProjectDocument&)>& change) {
    try {const bool changed=history_.execute(name,change);if(changed)clearRun();error_->clear();rejected_.clear();refresh();return changed;}
    catch(const std::exception& e){showError(e);canvas_->setDocument(&history_.document());return false;}
}
void EditorWindow::deleteSelected() {
    const auto ids=canvas_->selection();
    if (ids.empty()) return;
    QMessageBox box(QMessageBox::Question,text("editorDeleteSelected"),text("editorDeleteSelectedWarning"),QMessageBox::Yes|QMessageBox::No,this);
    box.button(QMessageBox::Yes)->setText(text("editorConfirm"));box.button(QMessageBox::No)->setText(text("editorCancel"));box.setDefaultButton(QMessageBox::No);
    if (box.exec()==QMessageBox::Yes) execute("editorDeleteSelected",[&](auto& d){deleteObjects(d,ids);});
}
void EditorWindow::showError(const std::exception& e) {
    // Keep the structured issues instead of collapsing them into one line: the red label says
    // what went wrong, the Problems tab says which objects it happened to.
    rejected_.clear();
    if (const auto* invalid=dynamic_cast<const ValidationError*>(&e))
        for (const auto& issue : invalid->issues) {
            const auto& network=history_.document().network;
            auto object=objectIdForPath(network,issue.path);
            auto select=selectableFor(network,object);
            rejected_.push_back({issue.code,issue.path,std::move(object),std::move(select),DiagnosticSeverity::draft});
        }
    const auto translated=text(e.what()); error_->setText(translated.isEmpty()?text("editorError")+" "+QString::fromUtf8(e.what()):translated);
    refreshDiagnostics();
    if (!rejected_.empty()) objects_->setCurrentIndex(3);
}
std::string EditorWindow::selectedName() const {
    const auto& network=history_.document().network;const auto& id=canvas_->selected();
    for(const auto& l:network.links)if(l.id==id)return l.name;
    for(const auto& c:network.connectors)if(c.id==id)return c.name;
    for(const auto& h:network.signalHeads)if(h.id==id)return h.name;
    return {};
}
void EditorWindow::refresh(bool modelChanged) {
    if(modelChanged) canvas_->setDocument(&history_.document());
    setWindowTitle(text("editorTitle")+" — "+(file_.isEmpty()?text("editorUntitled"):file_)+(history_.dirty()?" *":""));
    actions_.at("editorUndo")->setEnabled(history_.canUndo());actions_.at("editorRedo")->setEnabled(history_.canRedo());
    refreshHistory();
    const auto selected=canvas_->selected(); const Link* link=nullptr;
    for(const auto& l:history_.document().network.links) if(l.id==selected) link=&l;
    id_->setText(QString::fromStdString(selected));
    name_->setEnabled(!selected.empty());
    {const QSignalBlocker block(name_);name_->setText(QString::fromStdString(selectedName()));}
    widths_->setEnabled(link); actions_.at("editorApplyLanes")->setEnabled(link);
    linkMarkings_->setEnabled(link);linkPointStation_->setEnabled(link);
    for(const auto* key:{"editorApplyLinkMarkings","editorInsertLinkPoint","editorAddLinkPoint",
                         "editorStraightLink","editorReverseLink"})actions_.at(key)->setEnabled(link);
    QStringList markings;
    if(link)for(const auto m:link->boundaryMarkings)markings<<markingName(m);
    linkMarkings_->setText(markings.join(", "));
    for(const auto* key:{"editorDeleteLink","editorOpposite","editorPocket","editorSplitHere"}) actions_.at(key)->setEnabled(link);
    actions_.at("editorDeleteVertex")->setEnabled(link || canvas_->selectedConnector());
    if(link) {
        count_->setValue(static_cast<int>(link->lanes.size()));
        QStringList widths; for(const auto& lane:link->lanes) widths<<QString::number(lane.width,'g',10);
        widths_->setText(widths.join(", "));
        selectionInfo_->setText(text("editorLength").arg(polylineLength(link->geometry),0,'f',2));
    } else {widths_->clear();selectionInfo_->setText(text("editorNoSelection"));}
    // Property edits always act on the primary object, so say so rather than letting a
    // multi-selection look as if lane or geometry changes will apply to all of it.
    if (canvas_->selection().size()>1)
        selectionInfo_->setText(text("editorSelectionCount").arg(canvas_->selection().size()));
    refreshConnector();refreshAppearance();
    {const QSignalBlocker block(side_);side_->setCurrentIndex(history_.document().network.drivingSide==DrivingSide::left?0:1);}
    const auto& b=history_.document().background;
    bgX_->setValue(b.x);bgY_->setValue(b.y);bgScale_->setValue(b.metresPerPixel);bgAngle_->setValue(b.rotation);bgOpacity_->setValue(b.opacity);
    actions_.at("editorDeleteSelected")->setEnabled(!canvas_->selection().empty());
    actions_.at("editorRotate")->setEnabled(canvas_->rotationPivot().has_value());
    refreshTables(modelChanged);if(modelChanged)refreshDemand();refreshDiagnostics();refreshRun();
}
}
