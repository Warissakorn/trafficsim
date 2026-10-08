#pragma once
#include "../editor/ui_design_tokens.hpp"
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <optional>

namespace trafficsim::results {
// The Results tables (editor_results.cpp, editor_batch.cpp): read-only rows, the names take the
// room and each figure column is exactly as wide as its header needs.
inline QTableWidget* table(QWidget* parent, const char* name, int columns) {
    auto* table=new QTableWidget(0,columns,parent); table->setObjectName(name);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    return table;
}
inline QLabel* note(QWidget* parent, const char* name) {
    auto* label=new QLabel(parent); label->setObjectName(name); label->setWordWrap(true); return label;
}
// An absent figure is an empty cell, never 0.
inline QTableWidgetItem* figure(const std::optional<double>& value, int decimals=1) {
    auto* item=new QTableWidgetItem(value?QString::number(*value,'f',decimals):QString());
    editorDesign::setNumericText(item,true);
    item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
    return item;
}
}
