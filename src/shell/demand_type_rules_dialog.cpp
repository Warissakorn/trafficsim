#include "demand_type_rules_dialog.hpp"
#include "../project/demand_time_types.hpp"
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
namespace trafficsim {
bool editDemandTypeRules(QWidget* parent,RoutingDecision& decision,const std::vector<VehicleType>& types,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorTypeRulesDialog");dialog.setWindowTitle(text("editorTypeRules"));
    auto* layout=new QVBoxLayout(&dialog);auto* help=new QLabel(text("editorTypeRulesHelp"),&dialog);help->setWordWrap(true);layout->addWidget(help);
    auto* tabs=new QTabWidget(&dialog);tabs->setObjectName("editorTypeRuleTabs");layout->addWidget(tabs);
    struct Page {std::string id;QCheckBox* enabled;QTableWidget* table;};std::vector<Page> pages;
    std::vector<std::string> ids;for(const auto& rule:decision.typeRules)ids.push_back(rule.vehicleTypeId);
    for(const auto& type:types)if(std::find(ids.begin(),ids.end(),type.id)==ids.end())ids.push_back(type.id);
    for(const auto& id:ids) {
        const auto at=std::find_if(decision.typeRules.begin(),decision.typeRules.end(),[&](const auto& r){return r.vehicleTypeId==id;});
        const bool existing=at!=decision.typeRules.end();
        auto* body=new QWidget(tabs);auto* box=new QVBoxLayout(body);
        auto* enabled=new QCheckBox(text("editorTypeRuleOverride"),body);enabled->setObjectName("editorTypeRuleOverride");enabled->setChecked(existing);box->addWidget(enabled);
        auto* table=new QTableWidget(decision.routes.size(),decision.intervals.size()+2,body);table->setObjectName("editorTypeRuleWeights");
        QStringList columns{text("editorDecisionTarget"),text("editorDecisionFlow")};
        for(const auto& p:decision.intervals)columns<<QString("[%1, %2) s").arg(p.startTime,0,'g',12).arg(p.endTime,0,'g',12);
        table->setHorizontalHeaderLabels(columns);table->horizontalHeader()->setStretchLastSection(true);box->addWidget(table);
        for(std::size_t r=0;r<decision.routes.size();++r) {
            const auto& target=decision.routes[r];auto* cell=new QTableWidgetItem(QString::fromStdString(target.destinationLinkId.empty()?target.routeId:target.destinationLinkId));
            cell->setFlags(Qt::ItemIsEnabled);table->setItem(r,0,cell);
            for(int c=1;c<table->columnCount();++c) {
                const double original=c==1?(existing?at->relativeFlows.at(r):target.relativeFlow):
                    existing?at->intervalFlows.at(r).at(c-2):target.intervalFlows.at(c-2);
                auto* spin=new QDoubleSpinBox(table);spin->setRange(0,std::max(1e12,original));spin->setDecimals(12);spin->setValue(original);
                spin->setProperty("original",original);spin->setProperty("edited",false);
                QObject::connect(spin,&QDoubleSpinBox::valueChanged,spin,[spin]{spin->setProperty("edited",true);});table->setCellWidget(r,c,spin);
            }
        }
        table->setEnabled(existing);QObject::connect(enabled,&QCheckBox::toggled,table,&QWidget::setEnabled);
        tabs->addTab(body,QString::fromStdString(id));pages.push_back({id,enabled,table});
    }
    auto* error=new QLabel(&dialog);error->setObjectName("editorTypeRulesError");layout->addWidget(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    auto staged=decision;
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        staged.typeRules.clear();
        for(const auto& p:pages)if(p.enabled->isChecked()) {
            RoutingTypeRule rule{p.id,{}, {}};
            for(int r=0;r<p.table->rowCount();++r) {
                std::vector<double> row;
                for(int c=1;c<p.table->columnCount();++c) {
                    const auto* spin=qobject_cast<QDoubleSpinBox*>(p.table->cellWidget(r,c));
                    const double v=spin->property("edited").toBool()?spin->value():spin->property("original").toDouble();
                    if(c==1)rule.relativeFlows.push_back(v);else row.push_back(v);
                }
                rule.intervalFlows.push_back(std::move(row));
            }
            staged.typeRules.push_back(std::move(rule));
        }
        AuthoringDefinition check;check.routingDecisions={staged};check.vehicleTypes=types;check.externalVehicleTypes=false;
        const auto issues=timeTypeIssues(check);if(!issues.empty()){error->setText(text(issues.front().code.c_str()));return;}dialog.accept();
    });
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.resize(740,460);if(dialog.exec()!=QDialog::Accepted)return false;decision.typeRules=std::move(staged.typeRules);return true;
}
}
