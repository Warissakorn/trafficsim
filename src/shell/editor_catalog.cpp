#include "editor_window.hpp"
#include "demand_catalog_dialog.hpp"
#include "../commands/catalog_commands.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
namespace trafficsim {
namespace {
template<class T> std::string nextCatalogId(const std::vector<T>& values,const std::string& prefix) {
    for(unsigned n=1;;++n){const auto id=prefix+std::to_string(n);
        if(std::none_of(values.begin(),values.end(),[&](const auto& v){return v.id==id;}))return id;}
}
}
void EditorWindow::editDemandCatalog() {
    DemandCatalog staged;
    try{staged=resolveDemandCatalog(history_.document().definition.value_or(AuthoringDefinition{}),data_);}
    catch(const std::exception& e){showError(e);return;}
    const CatalogText tr=[this](const char* key){return text(key);};
    QDialog dialog(this);dialog.setObjectName("editorCatalogDialog");dialog.setWindowTitle(text("editorDemandCatalog"));
    auto* layout=new QVBoxLayout(&dialog);
    auto* help=new QLabel(text("editorCatalogHelp"),&dialog);help->setWordWrap(true);layout->addWidget(help);
    auto* tabs=new QTabWidget(&dialog);layout->addWidget(tabs);
    auto* error=new QLabel(&dialog);error->setObjectName("editorCatalogError");error->setWordWrap(true);
    QTableWidget* typeTable=nullptr;QTableWidget* compositionTable=nullptr;
    const auto refresh=[&]{
        typeTable->setRowCount(0);compositionTable->setRowCount(0);
        const auto append=[](QTableWidget* table,const std::string& id,const std::string& name){
            const int at=table->rowCount();table->insertRow(at);
            table->setItem(at,0,new QTableWidgetItem(QString::fromStdString(id)));
            table->setItem(at,1,new QTableWidgetItem(QString::fromStdString(name)));
        };
        for(const auto& t:staged.vehicleTypes) {
            const auto at=staged.vehicleTypeNames.find(t.id);append(typeTable,t.id,at==staged.vehicleTypeNames.end()?std::string{}:at->second);
        }
        for(const auto& c:staged.compositions)append(compositionTable,c.id,c.name);
    };
    const auto typeEdit=[&](bool create){
        const int row=typeTable->currentRow();if(!create && row<0)return;
        VehicleType value=create?(staged.vehicleTypes.empty()?VehicleType{}:staged.vehicleTypes.front()):staged.vehicleTypes[row];
        std::string name;
        if(!create)if(const auto at=staged.vehicleTypeNames.find(value.id);at!=staged.vehicleTypeNames.end())name=at->second;
        if(create)value.id=nextCatalogId(staged.vehicleTypes,"vehicle-type-");
        if(!editCatalogType(&dialog,value,name,staged.behaviours,tr))return;
        if(name.empty())staged.vehicleTypeNames.erase(value.id);else staged.vehicleTypeNames[value.id]=name;
        if(create)staged.vehicleTypes.push_back(value);else staged.vehicleTypes[row]=value;refresh();
    };
    const auto compositionEdit=[&](bool create){
        const int row=compositionTable->currentRow();if(!create && row<0)return;
        Composition value=create?Composition{nextCatalogId(staged.compositions,"composition-"),{}}:staged.compositions[row];
        if(!editCatalogComposition(&dialog,value,staged.vehicleTypes,tr))return;
        if(create)staged.compositions.push_back(value);else staged.compositions[row]=value;refresh();
    };
    const auto removeType=[&]{
        const int row=typeTable->currentRow();if(row<0)return;const auto id=staged.vehicleTypes[row].id;
        bool referenced=false;
        if(history_.document().definition)for(const auto& i:history_.document().definition->inputs)referenced|=i.vehicleTypeId==id;
        for(const auto& c:staged.compositions) {
            for(const auto& t:c.types)referenced|=t.vehicleTypeId==id;
            for(const auto& p:c.intervals)for(const auto& t:p.types)referenced|=t.vehicleTypeId==id;
        }
        if(history_.document().definition)for(const auto& x:history_.document().definition->routingDecisions)
            for(const auto& rule:x.typeRules)referenced|=rule.vehicleTypeId==id;
        if(referenced){error->setText(text("EDIT_REFERENCED_VEHICLE_TYPE"));return;}
        staged.vehicleTypes.erase(staged.vehicleTypes.begin()+row);staged.vehicleTypeNames.erase(id);error->clear();refresh();
    };
    const auto removeComposition=[&]{
        const int row=compositionTable->currentRow();if(row<0)return;const auto id=staged.compositions[row].id;
        if(history_.document().definition)for(const auto& i:history_.document().definition->inputs)if(i.compositionId==id){
            error->setText(text("EDIT_REFERENCED_COMPOSITION"));return;}
        staged.compositions.erase(staged.compositions.begin()+row);error->clear();refresh();
    };
    const auto page=[&](QTableWidget*& table,const char* key,const char* object,const std::function<void(bool)>& edit,
                        const std::function<void()>& remove){
        auto* body=new QWidget(tabs);auto* box=new QVBoxLayout(body);table=new QTableWidget(0,2,body);table->setObjectName(object);
        table->setHorizontalHeaderLabels({text("editorColumnId"),text("editorColumnName")});
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);table->horizontalHeader()->setStretchLastSection(true);box->addWidget(table);
        for(const auto& pair:std::vector<std::pair<const char*,std::function<void()>>>{{"catalogAdd",[edit]{edit(true);}},
            {"catalogEdit",[edit]{edit(false);}},{"catalogDelete",remove}}){
            auto* button=new QPushButton(text(pair.first),body);button->setObjectName(QString(object)+pair.first);box->addWidget(button);
            connect(button,&QPushButton::clicked,&dialog,pair.second);
        }
        connect(table,&QTableWidget::cellDoubleClicked,&dialog,[edit](int,int){edit(false);});tabs->addTab(body,text(key));
    };
    page(typeTable,"editorCatalogType","editorCatalogTypes",typeEdit,removeType);
    page(compositionTable,"editorCatalogComposition","editorCatalogCompositions",compositionEdit,removeComposition);refresh();layout->addWidget(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        try{auto candidate=history_.document();putDemandCatalog(candidate,staged);validateDocument(candidate);dialog.accept();}
        catch(const ValidationError& e){const auto& issue=e.issues.front();error->setText(text(issue.code)+" · "+QString::fromStdString(issue.path));}
        catch(const std::exception& e){error->setText(text(e.what()));}
    });
    dialog.resize(640,520);if(dialog.exec()!=QDialog::Accepted)return;
    execute("editorDemandCatalog",[&](auto& document){putDemandCatalog(document,std::move(staged));});
}
}
