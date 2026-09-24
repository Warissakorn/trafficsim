#include "editor_window.hpp"
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
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
// M2.1.1: the decision can be PLACED on a Link, as in Vissim. Then every routeless vehicle
// reaching that Link takes it, and a row may be a destination Link instead of a route -- counted
// turning volumes typed straight into the destinations, with no route drawn at all.
void EditorWindow::editDecision(const std::string& id) {
    RoutingDecision value{id,{},{},{}};
    const auto& def=history_.document().definition;
    const auto& network=history_.document().network;
    if(def)for(const auto& x:def->routingDecisions)if(x.id==id)value=x;
    QDialog dialog(this);dialog.setObjectName("editorDecisionDialog");dialog.setWindowTitle(text("editorEditDecision"));
    auto* layout=new QVBoxLayout(&dialog);
    auto* name=new QLineEdit(QString::fromStdString(value.name),&dialog);name->setObjectName("editorDecisionNameField");
    name->setPlaceholderText(text("editorDecisionName"));layout->addWidget(name);
    auto* help=new QLabel(text("editorDecisionHelp"),&dialog);help->setWordWrap(true);layout->addWidget(help);
    auto* place=new QComboBox(&dialog);place->setObjectName("editorDecisionLink");
    place->addItem(text("editorDecisionUnplaced"),QString());
    for(const auto& l:network.links)
        place->addItem(QString::fromStdString(l.name.empty()?l.id:l.name),QString::fromStdString(l.id));
    if(const int at=place->findData(QString::fromStdString(value.linkId));at>=0)place->setCurrentIndex(at);
    auto* placeRow=new QFormLayout;placeRow->addRow(text("editorDecisionLinkLabel"),place);layout->addLayout(placeRow);
    auto* flows=new QTableWidget(0,2,&dialog);flows->setObjectName("editorDecisionFlows");
    flows->setHorizontalHeaderLabels({text("editorDecisionTarget"),text("editorDecisionFlow")});
    flows->horizontalHeader()->setStretchLastSection(true);flows->verticalHeader()->hide();
    // One row per route, then one per Link as a destination. `link` is empty for a route row.
    struct Row { std::string route, link; QDoubleSpinBox* field; };
    std::vector<Row> rows;
    const auto addRow=[&](const QString& label,const std::string& route,const std::string& link,double flow){
        const int n=flows->rowCount();flows->insertRow(n);
        auto* cell=new QTableWidgetItem(label);cell->setFlags(Qt::ItemIsEnabled);flows->setItem(n,0,cell);
        auto* field=new QDoubleSpinBox(flows);field->setObjectName(QString("editorDecisionFlow%1").arg(n));
        field->setRange(0,1000000);field->setDecimals(3);field->setValue(flow);
        flows->setCellWidget(n,1,field);rows.push_back({route,link,field});
    };
    if(def)for(const auto& r:def->routes) {
        double flow=0;
        for(const auto& entry:value.routes)if(entry.destinationLinkId.empty()&&entry.routeId==r.id)flow=entry.relativeFlow;
        addRow(QString::fromStdString(r.id),r.id,{},flow);
    }
    for(const auto& l:network.links) {
        double flow=0;
        for(const auto& entry:value.routes)if(entry.destinationLinkId==l.id)flow=entry.relativeFlow;
        addRow(text("editorDecisionDestinationItem").arg(QString::fromStdString(l.name.empty()?l.id:l.name)),{},l.id,flow);
    }
    // A destination is reached from where the decision sits, so destination rows need a Link,
    // and the Link it sits on is no destination of its own.
    const auto sync=[&]{
        const auto at=place->currentData().toString().toStdString();
        for(const auto& row:rows)if(!row.link.empty())row.field->setEnabled(!at.empty()&&row.link!=at);
    };
    connect(place,&QComboBox::currentIndexChanged,&dialog,[sync](int){sync();});sync();
    layout->addWidget(flows);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    buttons->button(QDialogButtonBox::Ok)->setEnabled(!rows.empty());
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.resize(480,460);if(dialog.exec()!=QDialog::Accepted)return;
    value.name=name->text().trimmed().toStdString();value.routes.clear();
    value.linkId=place->currentData().toString().toStdString();
    for(const auto& row:rows)if(row.field->isEnabled()&&row.field->value()>0)
        value.routes.push_back({row.route,row.field->value(),row.link});
    std::string created;if(execute("editorEditDecision",[&](auto& d){created=putRoutingDecision(d,value);}))selectDemand(created);
}
}
