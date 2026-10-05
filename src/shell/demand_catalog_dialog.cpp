#include "demand_catalog_dialog.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
namespace trafficsim {
namespace {
QDoubleSpinBox* number(QWidget* parent,const char* name,double value) {
    auto* field=new QDoubleSpinBox(parent);field->setObjectName(name);field->setRange(0,std::max(1e9,value));field->setDecimals(9);
    field->setValue(value);field->setProperty("original",value);field->setProperty("edited",false);
    field->setFont(editorDesign::numericFont());
    QObject::connect(field,&QDoubleSpinBox::valueChanged,field,[field]{field->setProperty("edited",true);});return field;
}
double value(const QDoubleSpinBox* f){return f->property("edited").toBool()?f->value():f->property("original").toDouble();}
void buttons(QDialog& dialog,QLayout* layout,const CatalogText& text) {
    auto* b=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(b);
    b->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));b->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    QObject::connect(b,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(b,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
}
}
bool editCatalogType(QWidget* parent,VehicleType& type,std::string& name,const std::vector<DriverBehaviour>& behaviours,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorCatalogTypeDialog");dialog.setWindowTitle(text("editorCatalogType"));
    auto* form=new QFormLayout(&dialog);form->addRow(text("editorColumnId"),new QLabel(QString::fromStdString(type.id),&dialog));
    auto* label=new QLineEdit(QString::fromStdString(name),&dialog);label->setObjectName("editorCatalogTypeName");form->addRow(text("editorColumnName"),label);
    struct Field{const char* key;double* destination;QDoubleSpinBox* spin;};
    std::vector<Field> fields;
    const auto add=[&](const char* key,double& destination){
        auto* spin=number(&dialog,key,destination);form->addRow(text(key),spin);fields.push_back({key,&destination,spin});
    };
    auto staged=type;
    add("catalogLength",staged.length);add("catalogWidth",staged.width);
    add("catalogSpeedMin",staged.desiredSpeed.min);add("catalogSpeedMax",staged.desiredSpeed.max);
    add("catalogAcceleration",staged.maxAcceleration);add("catalogComfortableDeceleration",staged.comfortableDeceleration);
    add("catalogMaxDeceleration",staged.maxDeceleration);
    auto* behaviour=new QComboBox(&dialog);behaviour->setObjectName("editorCatalogBehaviour");
    for(const auto& b:behaviours)behaviour->addItem(QString::fromStdString(b.id));
    behaviour->setCurrentIndex(behaviour->findText(QString::fromStdString(staged.behaviourId)));form->addRow(text("catalogBehaviour"),behaviour);
    auto* axles=new QCheckBox(text("catalogAxles"),&dialog);axles->setObjectName("editorCatalogAxles");axles->setChecked(type.axles.has_value());form->addRow(axles);
    auto dimensions=type.axles.value_or(VehicleAxles{});
    auto* wheelbase=number(&dialog,"catalogWheelbase",dimensions.wheelbase);form->addRow(text("catalogWheelbase"),wheelbase);
    auto* front=number(&dialog,"catalogFrontOverhang",dimensions.frontOverhang);form->addRow(text("catalogFrontOverhang"),front);
    auto* rear=number(&dialog,"catalogRearOverhang",dimensions.rearOverhang);form->addRow(text("catalogRearOverhang"),rear);
    const auto sync=[&]{for(auto* f:{wheelbase,front,rear})f->setEnabled(axles->isChecked());};
    QObject::connect(axles,&QCheckBox::toggled,&dialog,sync);sync();buttons(dialog,form,text);
    if(dialog.exec()!=QDialog::Accepted)return false;
    for(const auto& f:fields)*f.destination=value(f.spin);
    staged.behaviourId=behaviour->currentText().toStdString();
    staged.axles=axles->isChecked()?std::optional<VehicleAxles>{{value(wheelbase),value(front),value(rear)}}:std::nullopt;
    type=std::move(staged);name=label->text().trimmed().toStdString();return true;
}
bool editCatalogComposition(QWidget* parent,Composition& composition,const std::vector<VehicleType>& types,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorCatalogCompositionDialog");dialog.setWindowTitle(text("editorCatalogComposition"));
    auto* layout=new QVBoxLayout(&dialog);layout->addWidget(new QLabel(QString::fromStdString(composition.id),&dialog));
    auto* name=new QLineEdit(QString::fromStdString(composition.name),&dialog);name->setObjectName("editorCatalogCompositionName");
    name->setPlaceholderText(text("editorColumnName"));layout->addWidget(name);
    auto* help=new QLabel(text("catalogShareHelp"),&dialog);help->setWordWrap(true);layout->addWidget(help);
    auto* table=new QTableWidget(0,2,&dialog);table->setObjectName("editorCatalogCompositionShares");
    table->setHorizontalHeaderLabels({text("editorInputType"),text("catalogShare")});layout->addWidget(table);
    std::vector<QDoubleSpinBox*> shares;
    for(const auto& t:types) {
        double weight=0;for(const auto& member:composition.types)if(member.vehicleTypeId==t.id)weight=member.share;
        const int at=table->rowCount();table->insertRow(at);
        auto* id=new QTableWidgetItem(QString::fromStdString(t.id));id->setFlags(Qt::ItemIsEnabled);table->setItem(at,0,id);
        auto* field=number(table,"editorCatalogShare",weight);table->setCellWidget(at,1,field);shares.push_back(field);
    }
    buttons(dialog,layout,text);dialog.resize(500,420);if(dialog.exec()!=QDialog::Accepted)return false;
    composition.name=name->text().trimmed().toStdString();
    if(std::none_of(shares.begin(),shares.end(),[](const auto* f){return f->property("edited").toBool();}))return true;
    composition.types.clear();
    for(std::size_t k=0;k<types.size();++k)if(value(shares[k])>0)composition.types.push_back({types[k].id,value(shares[k])});
    return true;
}
}
