#pragma once
#include "../editor/ui_design_tokens.hpp"
#include <QHeaderView>
#include <QStringList>
#include <QTableWidget>
#include <cstdint>
#include <optional>
#include <vector>

namespace trafficsim::batchCells {
// The cells the batch (M5.6) and comparison (M5.8b) tables share: read-only rows, names in the
// stretching first column, figures right-aligned in the numeric font to one decimal.
inline QTableWidget* table(QWidget* parent, const char* name, int columns) {
    auto* table=new QTableWidget(0,columns,parent); table->setObjectName(name);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    return table;
}
inline QTableWidgetItem* cell(const QString& text, bool numeric) {
    auto* item=new QTableWidgetItem(text);
    if(numeric){ editorDesign::setNumericText(item,true); item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter); }
    return item;
}
inline QTableWidgetItem* number(const std::optional<double>& value) {
    return cell(value?QString::number(*value,'f',1):QString(),true);
}
inline QTableWidgetItem* count(std::size_t n) { return cell(QString::number(n),true); }
inline QString seedText(const std::vector<std::uint32_t>& seeds) {
    QStringList out; for(const auto s:seeds) out<<QString::number(s); return out.join(", ");
}
}
