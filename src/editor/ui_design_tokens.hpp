#pragma once

#include <QFont>
#include <QFontDatabase>
#include <QTableWidgetItem>

namespace trafficsim::editorDesign {
inline constexpr const char* gray0="#FFFFFF";
inline constexpr const char* gray1="#F9FAFB";
inline constexpr const char* gray2="#F3F4F6";
inline constexpr const char* gray3="#E5E7EB";
inline constexpr const char* gray4="#D1D5DB";
inline constexpr const char* gray5="#9CA3AF";
inline constexpr const char* gray6="#6B7280";
inline constexpr const char* gray7="#4B5563";
inline constexpr const char* gray8="#374151";
inline constexpr const char* gray9="#111827";
inline constexpr const char* accent="#0F766E";
inline constexpr const char* danger="#B42318";
inline constexpr const char* warning="#8A4B08";
inline constexpr const char* success="#176B44";

inline constexpr int space1=4,space2=8,space3=12,space4=16,space5=20,space6=24;
inline constexpr int fontSizeLabel=11,fontSizeNumeric=12,fontSizeBody=13;
inline constexpr int fontSizeSection=14,fontSizeTitle=18;
inline constexpr int controlHeight=28,tableRowHeight=28;

inline QFont numericFont() {
    auto font=QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setStyleHint(QFont::Monospace);font.setFixedPitch(true);font.setPixelSize(fontSizeNumeric);
    return font;
}
inline void setNumericText(QTableWidgetItem* item,bool rightAligned=false) {
    if(!item)return;
    item->setFont(numericFont());
    if(rightAligned)item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
}
}
