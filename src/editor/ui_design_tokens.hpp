#pragma once

#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QLocale>
#include <QPalette>
#include <QPen>
#include <QString>
#include <QStyleOption>
#include <QTableWidgetItem>
#include <QTransform>
#include <algorithm>
#include <cmath>

// Presentation tokens. Every colour the editor draws is read back from editorPalette() by
// QPalette role, so this header is the only place a hex literal may appear.
namespace trafficsim::editorDesign {
inline constexpr const char* gray0="#FFFFFF";
inline constexpr const char* gray1="#F9FAFB";
inline constexpr const char* gray2="#F3F4F6";
inline constexpr const char* gray3="#E5E7EB";
inline constexpr const char* gray6="#6B7280";
inline constexpr const char* gray7="#4B5563";
inline constexpr const char* gray9="#111827";
inline constexpr const char* accentHex="#2F6FED";
inline constexpr const char* errorHex="#B42318";
inline constexpr const char* warningHex="#8A4B08";
inline constexpr const char* advisoryHex="#0B6E8A";
inline constexpr const char* okHex="#176B44";

inline constexpr int space1=4,space2=8,space3=12,space4=16,space5=20,space6=24;
inline constexpr int fontSizeLabel=11,fontSizeNumeric=12,fontSizeBody=13;
inline constexpr int fontSizeSection=14,fontSizeTitle=18;
inline constexpr int controlHeight=28,tableRowHeight=28,toolbarHeight=32,iconSize=16;
inline constexpr int maxRadius=3,maxMotionMs=150;
inline constexpr double iconStroke=1.5;
inline constexpr double labelTracking=1.0; // px, English uppercase group labels only

enum class Semantic { error, warning, advisory, ok };

// Role map. Neutrals: Base canvas/inputs, Window panels, Midlight hover, Mid hairline, Dark
// control outline, WindowText secondary text, Text primary. Accent is Highlight. Semantic
// colour reuses the four roles the editor never draws with: BrightText, LinkVisited, Link, Shadow.
inline QPalette editorPalette() {
    static const QPalette palette=[]{
        QPalette p;
        const auto all=[&](QPalette::ColorRole role,const char* colour){p.setColor(role,QColor(colour));};
        all(QPalette::Window,gray1);all(QPalette::Base,gray0);all(QPalette::AlternateBase,gray1);
        all(QPalette::Button,gray1);all(QPalette::Light,gray0);all(QPalette::Midlight,gray2);
        all(QPalette::Mid,gray3);all(QPalette::Dark,gray6);all(QPalette::PlaceholderText,gray7);
        all(QPalette::WindowText,gray7);all(QPalette::Text,gray9);all(QPalette::ButtonText,gray9);
        all(QPalette::ToolTipBase,gray9);all(QPalette::ToolTipText,gray0);
        all(QPalette::Highlight,accentHex);all(QPalette::HighlightedText,gray0);
#if QT_VERSION >= QT_VERSION_CHECK(6,6,0)
        all(QPalette::Accent,accentHex); // Qt 6.6+ has its own accent role; keep it on the one accent
#endif
        all(QPalette::BrightText,errorHex);all(QPalette::LinkVisited,warningHex);
        all(QPalette::Link,advisoryHex);all(QPalette::Shadow,okHex);
        for(const auto role:{QPalette::Text,QPalette::ButtonText,QPalette::WindowText})
            p.setColor(QPalette::Disabled,role,QColor(gray7));
        return p;
    }();
    return palette;
}
inline QColor role(QPalette::ColorRole r) { return editorPalette().color(r); }
inline QColor accent() { return role(QPalette::Highlight); }
inline QColor semantic(Semantic s) {
    switch(s) {
    case Semantic::error: return role(QPalette::BrightText);
    case Semantic::warning: return role(QPalette::LinkVisited);
    case Semantic::advisory: return role(QPalette::Link);
    case Semantic::ok: return role(QPalette::Shadow);
    }
    return role(QPalette::Text);
}

// Monospace faces have equal-width digits, which is the tabular-figure guarantee: Qt 6.4 has
// no OpenType feature switch, so a proportional face with "tnum" is not available here.
inline QFont numericFont() {
    auto font=QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setStyleHint(QFont::Monospace);font.setFixedPitch(true);font.setPixelSize(fontSizeNumeric);
    return font;
}
// Group labels: 11 px, tracked, upper case. Thai has no case and tracking detaches its
// combining marks, so only English gets either.
inline void styleGroupLabel(QFont& font,bool english) {
    font.setPixelSize(fontSizeLabel);font.setBold(true);
    font.setCapitalization(english?QFont::AllUppercase:QFont::MixedCase);
    font.setLetterSpacing(english?QFont::AbsoluteSpacing:QFont::PercentageSpacing,english?labelTracking:100.);
}
// Fixed decimals and a unit suffix, through QLocale so the separator follows the user. The unit
// is separated by a no-break space so a value never wraps from its unit.
inline QString formatValue(double value,int decimals,const QString& unit=QString(),
                           const QLocale& locale=QLocale()) {
    auto text=locale.toString(value,'f',decimals);
    return unit.isEmpty()?text:text+QChar(0x00A0)+unit;
}
inline void setNumericText(QTableWidgetItem* item,bool rightAligned=false) {
    if(!item)return;
    item->setFont(numericFont());
    if(rightAligned)item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
}

// Hairlines. A cosmetic pen is measured in logical pixels, so one device pixel is 1/dpr wide,
// and it is centred on the coordinate: the coordinate has to sit on a pixel centre (+0.5).
inline QPen hairlinePen(const QColor& colour,double devicePixelRatio) {
    QPen pen(colour,1./std::max(1.,devicePixelRatio));pen.setCosmetic(true);pen.setCapStyle(Qt::FlatCap);
    return pen;
}
// Snaps a scene coordinate along one axis to the centre of a device pixel. scale/offset are
// the device transform's m11/dx (or m22/dy) for that axis.
inline double snapHairline(double scene,double scale,double offset) {
    if(scale==0)return scene;
    return (std::floor(scene*scale+offset)+0.5-offset)/scale;
}

// Two-tier grid. LOD is the transform's linear scale (QStyleOptionGraphicsItem's definition),
// so pixelsPerMetre is what the user sees. The minor step is the smallest decade of the base
// step that is at least minorMinPixels apart; the major step is ten minors.
struct GridTiers { double minor=0,major=0; };
inline constexpr double minorMinPixels=8;
inline GridTiers gridTiers(double baseStep,double pixelsPerMetre) {
    GridTiers tiers;
    if(!(baseStep>0)||!(pixelsPerMetre>0)||!std::isfinite(baseStep)||!std::isfinite(pixelsPerMetre))return tiers;
    double step=baseStep;
    for(int guard=0;step*pixelsPerMetre<minorMinPixels&&guard<24;++guard)step*=10;
    tiers.minor=step;tiers.major=step*10;
    return tiers;
}
inline double levelOfDetail(const QTransform& transform) {
    return QStyleOptionGraphicsItem::levelOfDetailFromTransform(transform);
}
}
