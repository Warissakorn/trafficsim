#include "editor_window.hpp"
#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QSpinBox>
#include <QToolButton>
#include <QTabWidget>
#include <QVBoxLayout>

namespace trafficsim {
void EditorWindow::label(QFormLayout* form,const std::string& key,QWidget* field) {
    auto* label=new QLabel(this);label->setWordWrap(true);label->setBuddy(field);texts_[key]=label;
    field->setObjectName(QString::fromStdString(key));form->addRow(label,field);
}
void EditorWindow::buildInspector() {
    auto* dock=new QDockWidget(this);dock->setObjectName("editorInspectorDock");texts_["editorInspector"]=dock;
    dock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetClosable);
    auto* scroll=new QScrollArea(dock);scroll->setWidgetResizable(true);
    auto* body=new QWidget(scroll);auto* layout=new QVBoxLayout(body);
    auto* common=new QFormLayout;common->setRowWrapPolicy(QFormLayout::WrapLongRows);layout->addLayout(common);
    id_=new QLineEdit(body);id_->setReadOnly(true);label(common,"editorId",id_);
    // Vissim's Name sits beside No. on every object dialog, so it lives in the common section
    // rather than the Link tab: one field names a Link, a Connector or a signal head alike.
    name_=new QLineEdit(body);name_->setMaxLength(200);label(common,"editorName",name_);
    connect(name_,&QLineEdit::editingFinished,this,[this]{
        const auto typed=name_->text().toStdString();
        // editingFinished also fires on plain focus loss. Committing an unchanged name there
        // would put an empty entry on the undo stack for every click out of the field.
        if(canvas_->selected().empty() || typed==selectedName())return;
        const auto id=canvas_->selected();
        execute("editorName",[&](auto& d){renameObject(d,id,typed);});
    });
    selectionInfo_=new QLabel(body);selectionInfo_->setWordWrap(true);common->addRow(selectionInfo_);
    side_=new QComboBox(body);side_->addItems({"",""});label(common,"editorDrivingSide",side_);
    connect(side_,&QComboBox::currentIndexChanged,this,[this](int index){execute("editorDrivingSide",[&](auto& d){changeDrivingSide(d,index==0?DrivingSide::left:DrivingSide::right);});});
    buildAppearance(common);
    properties_=new QTabWidget(body);properties_->setObjectName("editorPropertyTabs");layout->addWidget(properties_);
    auto* linkPage=new QWidget(properties_);auto* form=new QFormLayout(linkPage);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);properties_->addTab(linkPage,QString());
    auto number=[&](double min,double max,double value,int decimals=2){
        auto* s=new QDoubleSpinBox(body);s->setDecimals(decimals);s->setRange(min,max);s->setValue(value);return s;
    };
    auto button=[&](const std::string& key,const std::function<void()>& callback){
        auto* b=new QToolButton(body);b->setToolButtonStyle(Qt::ToolButtonTextOnly);b->setDefaultAction(action(key,{},callback));form->addRow(b);
    };
    auto heading=[&](const std::string& key){auto* h=new QLabel(body);h->setStyleSheet("font-weight:600;margin-top:8px;");texts_[key]=h;form->addRow(h);};
    heading("editorRoadProperties");
    count_=new QSpinBox(body);count_->setRange(1,12);count_->setValue(2);label(form,"editorLaneCount",count_);
    width_=number(0.1,20,3.5);label(form,"editorDefaultWidth",width_);
    widths_=new QLineEdit(body);label(form,"editorLaneWidths",widths_);
    button("editorApplyLanes",[this]{
        const auto parts=widths_->text().split(',',Qt::SkipEmptyParts);std::vector<double> widths;
        for(const auto& part:parts){bool ok=false;const double w=part.trimmed().toDouble(&ok);if(!ok){showError(std::runtime_error("EDIT_LANES"));return;}widths.push_back(w);}
        if(widths.size()==1) widths.resize(static_cast<std::size_t>(count_->value()),widths[0]);
        if(widths.size()!=static_cast<std::size_t>(count_->value())){showError(std::runtime_error("EDIT_LANES"));return;}
        execute("editorApplyLanes",[&](auto& d){changeLanes(d,canvas_->selected(),widths);});
    });
    linkMarkings_=new QLineEdit(body);label(form,"editorLinkMarkings",linkMarkings_);
    button("editorApplyLinkMarkings",[this]{
        try {
            std::vector<MarkingType> markings;
            if(!linkMarkings_->text().trimmed().isEmpty())
                for(const auto& part:linkMarkings_->text().split(',',Qt::KeepEmptyParts))
                    markings.push_back(markingFromName(part.trimmed().toLower().toStdString()));
            execute("editorApplyLinkMarkings",[&](auto& d){changeLinkMarkings(d,canvas_->selected(),markings);});
        } catch(const std::exception& e){showError(e);}
    });
    linkPointStation_=number(0,1000000,10,3);linkPointStation_->setSuffix(" m");
    label(form,"editorLinkPointStation",linkPointStation_);
    button("editorInsertLinkPoint",[this]{execute("editorInsertLinkPoint",[&](auto& d){
        insertLinkPoint(d,canvas_->selected(),linkPointStation_->value());});});
    button("editorAddLinkPoint",[this]{execute("editorAddLinkPoint",[&](auto& d){
        addLinkIntermediatePoint(d,canvas_->selected());});});
    button("editorStraightLink",[this]{execute("editorStraightLink",[&](auto& d){
        straightenLink(d,canvas_->selected());});});
    button("editorReverseLink",[this]{execute("editorReverseLink",[&](auto& d){
        reverseLink(d,canvas_->selected());});});
    split_=number(0.21,100000,30);label(form,"editorSplitDistance",split_);
    button("editorSplitHere",[this]{
        std::string id;if(execute("editorSplitHere",[&](auto& d){id=splitLink(d,canvas_->selected(),split_->value());}))canvas_->select(id);
    });
    button("editorPocket",[this]{
        std::string id;if(execute("editorPocket",[&](auto& d){id=splitLink(d,canvas_->selected(),split_->value(),true);}))canvas_->select(id);
    });
    gap_=number(0,100,2);label(form,"editorMedianGap",gap_);
    button("editorOpposite",[this]{
        std::string id;if(execute("editorOpposite",[&](auto& d){id=oppositeLink(d,canvas_->selected(),gap_->value());}))canvas_->select(id);
    });
    properties_->addTab(buildConnectorInspector(),QString());
    auto* imagePage=new QWidget(properties_);form=new QFormLayout(imagePage);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);properties_->addTab(imagePage,QString());
    heading("editorBackground");button("editorImportImage",[this]{importImage();});
    bgX_=number(-1000000,1000000,0,4);label(form,"editorImageX",bgX_);
    bgY_=number(-1000000,1000000,0,4);label(form,"editorImageY",bgY_);
    bgScale_=number(0.000001,10000,1,6);label(form,"editorImageScale",bgScale_);
    bgAngle_=number(-360,360,0,4);label(form,"editorImageRotation",bgAngle_);
    bgOpacity_=number(0,1,0.5);bgOpacity_->setSingleStep(0.1);label(form,"editorImageOpacity",bgOpacity_);
    button("editorApplyImage",[this]{applyBackground();});
    button("editorRemoveImage",[this]{execute("editorRemoveImage",[](auto& d){d.background={};});});
    auto* help=new QLabel(body);help->setWordWrap(true);texts_["editorHelp"]=help;layout->addWidget(help);
    layout->addStretch();
    actions_["editorInspector"]=dock->toggleViewAction();
    actions_["editorInspector"]->setShortcut(QKeySequence("Ctrl+I"));
    scroll->setWidget(body);dock->setWidget(scroll);addDockWidget(Qt::RightDockWidgetArea,dock);
    resizeDocks({dock},{325},Qt::Horizontal);
}
}
