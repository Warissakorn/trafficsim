#include "editor_window.hpp"
#include "../project/demand_preview.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
namespace trafficsim {
void EditorWindow::showDemandPreview() {
    DemandPreview preview;
    try{preview=previewDemand(history_.document(),data_);}catch(const std::exception& e){showError(e);return;}
    QDialog dialog(this);dialog.setObjectName("editorDemandPreviewDialog");dialog.setWindowTitle(text("editorDemandPreview"));
    auto* layout=new QVBoxLayout(&dialog);
    auto* help=new QLabel(text("editorDemandPreviewHelp"),&dialog);help->setWordWrap(true);layout->addWidget(help);
    auto* total=new QLabel(text("editorDemandExpectedTotal").arg(preview.revision).arg(preview.expectedVehicles,0,'g',12),&dialog);
    total->setObjectName("editorDemandExpectedTotal");layout->addWidget(total);
    for(const auto& issue:preview.advisories) {
        auto* note=new QLabel(QString::fromStdString(issue.path)+": "+text(issue.code.c_str()),&dialog);
        note->setWordWrap(true);layout->addWidget(note);
    }
    auto* table=new QTableWidget(0,10,&dialog);table->setObjectName("editorDemandPreviewTable");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);table->verticalHeader()->hide();
    table->setHorizontalHeaderLabels({text("editorColumnId"),text("editorInputType"),text("editorInputRoute"),
        text("editorDemandEntryLink"),text("editorDemandEntryLane"),text("editorDemandLastLink"),
        text("editorInputStart"),text("editorInputEnd"),text("editorInputVolume"),text("editorDemandExpected")});
    for(const auto& r:preview.rows) {
        const QStringList cells{QString::fromStdString(r.inputId),QString::fromStdString(r.vehicleTypeId),r.deferredRouting?text("editorDemandDeferredRoute"):QString::fromStdString(r.routeId),
            QString::fromStdString(r.entryLinkId),QString::fromStdString(r.entryLaneId),QString::fromStdString(r.lastLinkId),
            QString::number(r.startTime,'g',12),QString::number(r.endTime,'g',12),QString::number(r.vehiclesPerHour,'g',12),
            QString::number(r.expectedVehicles,'g',12)};
        const int row=table->rowCount();table->insertRow(row);
        for(int c=0;c<cells.size();++c){auto* item=new QTableWidgetItem(cells[c]);editorDesign::setNumericText(item,c>=6);table->setItem(row,c,item);}
    }
    table->resizeColumnsToContents();layout->addWidget(table);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close,&dialog);
    buttons->button(QDialogButtonBox::Close)->setText(text("editorCancel"));
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);layout->addWidget(buttons);
    dialog.resize(1100,550);dialog.exec();
}
}
