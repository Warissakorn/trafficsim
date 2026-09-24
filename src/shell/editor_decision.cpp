#include "editor_window.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace trafficsim {
// M2.4: a static routing decision -- turning proportions as relative flows over routes that all
// leave the same Link. One row per authored route; a flow of zero leaves the route out, and the
// one-origin rule is checked on commit, where the error names the route that breaks it.
void EditorWindow::editDecision(const std::string& id) {
    RoutingDecision value{id,{},{}};
    const auto& def=history_.document().definition;
    if(def)for(const auto& x:def->routingDecisions)if(x.id==id)value=x;
    QDialog dialog(this);dialog.setObjectName("editorDecisionDialog");dialog.setWindowTitle(text("editorEditDecision"));
    auto* layout=new QVBoxLayout(&dialog);
    auto* name=new QLineEdit(QString::fromStdString(value.name),&dialog);name->setObjectName("editorDecisionNameField");
    name->setPlaceholderText(text("editorDecisionName"));layout->addWidget(name);
    auto* help=new QLabel(text("editorDecisionHelp"),&dialog);help->setWordWrap(true);layout->addWidget(help);
    auto* flows=new QTableWidget(0,2,&dialog);flows->setObjectName("editorDecisionFlows");
    flows->setHorizontalHeaderLabels({text("editorInputRoute"),text("editorDecisionFlow")});
    flows->horizontalHeader()->setStretchLastSection(true);flows->verticalHeader()->hide();
    std::vector<std::pair<std::string,QDoubleSpinBox*>> fields;
    if(def)for(const auto& r:def->routes) {
        double flow=0;
        for(const auto& entry:value.routes)if(entry.routeId==r.id)flow=entry.relativeFlow;
        const int n=flows->rowCount();flows->insertRow(n);
        auto* cell=new QTableWidgetItem(QString::fromStdString(r.id));cell->setFlags(Qt::ItemIsEnabled);
        flows->setItem(n,0,cell);
        auto* field=new QDoubleSpinBox(flows);field->setObjectName(QString("editorDecisionFlow%1").arg(n));
        field->setRange(0,1000000);field->setDecimals(3);field->setValue(flow);
        flows->setCellWidget(n,1,field);fields.push_back({r.id,field});
    }
    layout->addWidget(flows);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    buttons->button(QDialogButtonBox::Ok)->setEnabled(!fields.empty());
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.resize(460,380);if(dialog.exec()!=QDialog::Accepted)return;
    value.name=name->text().trimmed().toStdString();value.routes.clear();
    for(const auto& [route,field]:fields)if(field->value()>0)value.routes.push_back({route,field->value()});
    std::string created;if(execute("editorEditDecision",[&](auto& d){created=putRoutingDecision(d,value);}))selectDemand(created);
}
}
