#include "editor_style.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QApplication>
#include <QDir>
#include <QPainter>
#include <QStyleFactory>
#include <QTemporaryDir>
namespace trafficsim {
void installEditorStyle() {
    // Fusion on every platform: a control looks and measures the same on Windows, Linux and
    // every Qt version CI runs. The native Windows styles each draw their own sub-controls under
    // a style sheet, and they differ between Qt releases.
    if(qobject_cast<QApplication*>(QCoreApplication::instance()) &&
       QApplication::style()->name().compare(QStringLiteral("fusion"),Qt::CaseInsensitive)!=0)
        QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
}
QString editorGlyphDirectory() {
    // A style sheet that styles a combo box's drop-down paints its arrow ONLY from an `image`,
    // which is why the arrow had vanished. The chevrons are drawn here from the palette -- so no
    // colour is written twice -- once per process, at 1x and 2x (Qt picks `@2x` by itself).
    static QTemporaryDir directory;
    static const bool written=[]{
        if(!directory.isValid())return false;
        struct Glyph { const char* name; bool up; QPalette::ColorRole role; };
        for(const auto& g:{Glyph{"down",false,QPalette::Text},Glyph{"up",true,QPalette::Text},
                           Glyph{"down-off",false,QPalette::Dark},Glyph{"up-off",true,QPalette::Dark}})
            for(int ratio:{1,2}) {
                constexpr int w=8,h=5;
                QImage image(w*ratio,h*ratio,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);
                QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);p.scale(ratio,ratio);
                p.setPen(QPen(editorDesign::role(g.role),editorDesign::iconStroke,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
                const double top=.9,bottom=h-.9;
                const QPointF points[3]={{.9,g.up?bottom:top},{w/2.,g.up?top:bottom},{w-.9,g.up?bottom:top}};
                p.drawPolyline(points,3);p.end();
                image.save(directory.filePath(QString::fromLatin1(g.name)+(ratio==2?"@2x.png":".png")));
            }
        return true;
    }();
    return written?QDir::fromNativeSeparators(directory.path()):QString();
}
}
