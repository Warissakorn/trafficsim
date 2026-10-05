#include "demand_period_editor.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
namespace trafficsim {
std::optional<std::vector<std::vector<double>>> editDemandPeriods(QWidget* parent,const QStringList& columns,
    const std::vector<std::vector<double>>& values,const std::function<QString(const char*)>& text) {
    QDialog dialog(parent);dialog.setObjectName("editorDemandPeriodsDialog");dialog.setWindowTitle(text("editorDemandPeriods"));
    auto* layout=new QVBoxLayout(&dialog);
    auto* help=new QLabel(text("editorDemandPeriodsHelp"),&dialog);help->setWordWrap(true);layout->addWidget(help);
    auto* table=new QTableWidget(0,columns.size(),&dialog);table->setObjectName("editorDemandPeriodsTable");
    table->setHorizontalHeaderLabels(columns);table->horizontalHeader()->setStretchLastSection(true);layout->addWidget(table);
    const auto append=[&](const std::vector<double>& row) {
        const int at=table->rowCount();table->insertRow(at);
        for(int c=0;c<table->columnCount();++c) {
            auto* field=new QDoubleSpinBox(table);field->setRange(0,1e12);field->setDecimals(12);
            field->setFont(editorDesign::numericFont());field->setProperty("numeric",true);
            const double original=static_cast<std::size_t>(c)<row.size()?row[c]:0;
            field->setMaximum(std::max(1e12,original));field->setValue(original);
            field->setProperty("originalValue",original);field->setProperty("edited",false);
            QObject::connect(field,&QDoubleSpinBox::valueChanged,field,[field]{field->setProperty("edited",true);});
            table->setCellWidget(at,c,field);
        }
    };
    for(const auto& row:values)append(row);
    auto* add=new QPushButton(text("editorDemandAddPeriod"),&dialog);add->setObjectName("editorDemandAddPeriod");layout->addWidget(add);
    auto* remove=new QPushButton(text("editorDemandRemovePeriod"),&dialog);remove->setObjectName("editorDemandRemovePeriod");layout->addWidget(remove);
    auto* error=new QLabel(&dialog);error->setObjectName("editorDemandPeriodsError");error->setWordWrap(true);layout->addWidget(error);
    QObject::connect(add,&QPushButton::clicked,&dialog,[&]{
        const double from=table->rowCount()?qobject_cast<QDoubleSpinBox*>(table->cellWidget(table->rowCount()-1,1))->value():0;
        std::vector<double> row(columns.size(),0);row[0]=from;row[1]=from+60;append(row);
    });
    QObject::connect(remove,&QPushButton::clicked,&dialog,[&]{if(table->currentRow()>=0)table->removeRow(table->currentRow());});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    std::vector<std::vector<double>> result;
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        result.clear();double end=0;
        for(int r=0;r<table->rowCount();++r) {
            std::vector<double> row;
            for(int c=0;c<table->columnCount();++c) {
                const auto* field=qobject_cast<QDoubleSpinBox*>(table->cellWidget(r,c));
                row.push_back(field->property("edited").toBool()?field->value():field->property("originalValue").toDouble());
            }
            if(row[0]>=row[1] || row[0]<end){error->setText(text("INVALID_INTERVAL"));return;}
            end=row[1];result.push_back(std::move(row));
        }
        dialog.accept();
    });
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.resize(740,450);if(dialog.exec()!=QDialog::Accepted)return std::nullopt;return result;
}
}
