#include "editor_style.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QString>
#include <QWidget>
#include <algorithm>
#include <map>
#include <utility>
namespace trafficsim {
QIcon editorIcon(EditorIcon icon) {
    static std::map<EditorIcon,QIcon> cache;
    if(const auto found=cache.find(icon);found!=cache.end())return found->second;
    QIcon result;
    // 16 px logical, rastered at 1x/2x/3x so QIcon picks the size matching the display. Glyphs
    // are drawn on a 20-unit grid, so a 1.5 px stroke is 1.5*20/16 grid units at every raster.
    constexpr double stroke=editorDesign::iconStroke*20./editorDesign::iconSize;
    for(int ratio:{1,2,3}) {
        const int size=editorDesign::iconSize*ratio;
        QPixmap pixmap(size,size);pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);p.setRenderHint(QPainter::Antialiasing);p.scale(size/20.,size/20.);
        p.setPen(QPen(editorDesign::role(QPalette::WindowText),stroke,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        const auto line=[&](int x,int y,int a,int b){p.drawLine(x,y,a,b);};
        const auto box=[&](int x,int y,int w,int h){p.drawRoundedRect(QRectF(x,y,w,h),1.5,1.5);};
        switch(icon) {
        case EditorIcon::document: box(4,2,12,16);line(7,8,13,8);line(7,12,13,12);break;
        case EditorIcon::open: box(2,6,16,11);line(3,6,3,3);line(3,3,9,3);line(9,3,11,6);break;
        case EditorIcon::save: box(3,3,14,14);box(6,3,8,5);box(6,12,8,5);break;
        case EditorIcon::undo: case EditorIcon::redo: {
            if(icon==EditorIcon::redo){p.translate(20,0);p.scale(-1,1);}
            QPainterPath path;path.moveTo(3,7);path.cubicTo(18,1,21,16,10,17);p.drawPath(path);
            line(3,7,3,2);line(3,7,8,9);break;
        }
        case EditorIcon::fit: case EditorIcon::focus:
            for(int i=0;i<4;++i){line(3,8,3,3);line(3,3,8,3);p.translate(20,0);p.rotate(90);}
            if(icon==EditorIcon::fit){line(7,10,13,10);line(10,7,10,13);}break;
        case EditorIcon::finish: line(3,10,8,15);line(8,15,17,5);break;
        case EditorIcon::rotate: case EditorIcon::reset:
            p.drawArc(QRectF(4,4,12,12),35*16,290*16);line(16,3,16,8);line(16,8,11,8);break;
        case EditorIcon::remove: line(3,5,17,5);line(7,2,13,2);box(5,5,10,13);line(8,9,8,14);line(12,9,12,14);break;
        case EditorIcon::select: {
            QPolygonF points;points<<QPointF(4,2)<<QPointF(16,10)<<QPointF(10,11)<<QPointF(7,17)<<QPointF(4,2);
            p.drawPolyline(points);break;
        }
        case EditorIcon::link: line(4,2,4,18);line(16,2,16,18);line(10,3,10,7);line(10,13,10,17);break;
        case EditorIcon::connector: case EditorIcon::route: {
            QPainterPath path;path.moveTo(3,17);path.cubicTo(3,6,17,14,17,3);p.drawPath(path);
            line(13,6,17,3);line(17,3,18,8);break;
        }
        case EditorIcon::input: box(3,6,14,9);line(5,6,7,3);line(7,3,13,3);line(13,3,15,6);line(6,15,6,17);line(14,15,14,17);break;
        case EditorIcon::signal: box(6,1,8,15);p.drawEllipse(QPointF(10,5),1.,1.);p.drawEllipse(QPointF(10,11),1.,1.);line(10,16,10,19);break;
        case EditorIcon::split: line(5,2,5,18);line(15,2,15,18);line(1,10,19,10);break;
        case EditorIcon::measure: box(2,5,16,10);for(int x=5;x<17;x+=3)line(x,5,x,9);break;
        case EditorIcon::image: box(2,3,16,14);line(3,15,8,9);line(8,9,13,14);line(13,14,17,9);p.drawEllipse(QPointF(13,7),1.5,1.5);break;
        case EditorIcon::run: case EditorIcon::step: {
            QPolygonF points;points<<QPointF(5,3)<<QPointF(15,10)<<QPointF(5,17);
            p.setBrush(editorDesign::accent());p.setPen(Qt::NoPen);p.drawPolygon(points);
            if(icon==EditorIcon::step){p.setPen(QPen(editorDesign::role(QPalette::WindowText),stroke));line(17,4,17,16);}break;
        }
        case EditorIcon::pause: line(7,3,7,17);line(13,3,13,17);break;
        case EditorIcon::inspector: box(2,3,16,14);line(12,3,12,17);line(14,7,16,7);line(14,11,16,11);break;
        case EditorIcon::objects: box(2,3,16,14);line(2,8,18,8);line(2,12,18,12);line(8,3,8,17);break;
        case EditorIcon::conflict: line(2,7,18,7);line(2,13,18,13);line(7,2,7,18);line(13,2,13,18);box(7,7,6,6);break;
        case EditorIcon::counter: line(2,3,18,3);box(6,6,8,3);box(6,11,8,3);box(6,16,8,3);break;
        case EditorIcon::grid: for(int v:{4,10,16}){line(v,2,v,18);line(2,v,18,v);}break;
        }
        p.end();result.addPixmap(pixmap);
    }
    cache.emplace(icon,result);return result;
}
// Every colour is a palette(role) reference, so the QSS holds no hex literal and follows the one
// editorPalette(). Box model: a control is 1 px border + 2 px padding + 18 px content = 24 px, and
// the 2 px focus / invalid border takes 1 px of padding back so a control never changes height.
// Fields and buttons are PINNED at 24 px (min-height = max-height, no vertical padding): the
// bundled Thai face has a 20 px line at 13 px, so padding-based heights came out 26-28 px, and
// Qt adds its own few pixels to spin boxes differently per version. A pinned height is the same
// for every font, language and Qt release; the text is centred in it by the widget.
// Qt's stylesheet style adds 3 px to a QToolButton's content before the box model, so a toolbar
// button is 16 icon + 3 + 3 padding + 2 border = 24 px. The odd pixel of padding sits top/left
// because Qt centres the icon in the 19 px content box with the extra pixel below/right.
// Qt Style Sheets have no letter-spacing, transition or shadow property; tracking is set on the
// label QFont (styleGroupLabel), and nothing here animates.
QString editorStyleSheet() {
    QString stylesheet=QStringLiteral(R"(
        QWidget { color: palette(text); font-size: @fontBodypx; }
        QMainWindow::separator { background: palette(midlight); width: @space1px; height: @space1px; border-left: 1px solid palette(mid); border-right: 1px solid palette(mid); }
        QMainWindow::separator:hover { background: palette(mid); }
        QMenuBar { background: palette(base); padding: 0 @space1px; border-bottom: 1px solid palette(mid); min-height: @controlHeightpx; }
        QMenuBar::item { padding: @space1px @space2px; }
        QMenuBar::item:focus, QMenu::item:focus { border: 2px solid palette(highlight); }
        QMenuBar::item:selected, QMenu::item:selected { background: palette(mid); color: palette(text); }
        QMenu { background: palette(base); border: 1px solid palette(mid); padding: @space1px; }
        QMenu::item { min-height: @controlContentpx; padding: @padYpx @space6px; }
        QToolBar { background: palette(base); border: 0; spacing: @space1px; padding: @space1px; }
        QToolBar::separator { background: palette(mid); width: 1px; margin: @space1px; }
        QToolButton, QPushButton { min-height: @fieldContentpx; max-height: @fieldContentpx; padding: 0 @space2px; border: 1px solid palette(dark); border-radius: @radiuspx; background: palette(button); }
        QToolBar QToolButton { min-width: @iconpx; min-height: @iconpx; border-color: transparent; background: transparent; padding: 2px 1px 1px 2px; }
        QToolButton:hover, QPushButton:hover { background: palette(midlight); border-color: palette(window-text); }
        QToolButton:pressed, QPushButton:pressed, QToolButton:checked { background: palette(mid); border-color: palette(highlight); }
        QToolButton:focus, QPushButton:focus { border: 2px solid palette(highlight); padding: 0 7px; min-height: @fieldFocuspx; max-height: @fieldFocuspx; }
        QToolBar QToolButton:focus { padding: 1px 0 0 1px; }
        QToolButton:disabled, QPushButton:disabled { color: palette(window-text); background: palette(midlight); border-color: palette(mid); }
        QToolButton#editorRunButton { background: palette(midlight); border-color: palette(highlight); color: palette(text); font-weight: 600; }
        QDockWidget { border: 1px solid palette(mid); }
        QDockWidget::title { background: palette(window); min-height: @controlContentpx; padding: @padYpx @space2px; color: palette(text); font-size: @fontLabelpx; font-weight: 600; }
        QScrollArea, QTabWidget::pane { border: 0; background: palette(base); }
        QGraphicsView { background: palette(base); border: 1px solid palette(mid); }
        QGraphicsView:focus { border: 2px solid palette(highlight); }
        QTabBar::tab { min-height: @controlContentpx; padding: @padYpx @space2px; border-bottom: 1px solid transparent; color: palette(window-text); }
        QTabBar::tab:selected { color: palette(text); border-bottom: 2px solid palette(highlight); background: palette(base); }
        QTabBar::tab:hover { background: palette(midlight); }
        QTabBar::tab:focus { border: 2px solid palette(highlight); }
        QLineEdit, QComboBox, QAbstractSpinBox { min-height: @fieldContentpx; max-height: @fieldContentpx; background: palette(base); border: 1px solid palette(dark); border-radius: @radiuspx; padding: 0 @space2px; selection-background-color: palette(highlight); selection-color: palette(highlighted-text); }
        QAbstractSpinBox, QLineEdit[numeric="true"], QComboBox[numeric="true"] { font-size: @fontNumericpx; }
        QAbstractSpinBox, QLineEdit[numeric="true"] { qproperty-alignment: AlignRight; }
        QLineEdit:hover, QComboBox:hover, QAbstractSpinBox:hover { border-color: palette(text); }
        QLineEdit:focus, QComboBox:focus, QAbstractSpinBox:focus { border: 2px solid palette(highlight); padding: 0 7px; min-height: @fieldFocuspx; max-height: @fieldFocuspx; }
        QLineEdit:read-only { background: palette(midlight); color: palette(window-text); }
        QLineEdit:disabled, QComboBox:disabled, QAbstractSpinBox:disabled { background: palette(midlight); color: palette(window-text); border-color: palette(mid); }
        QAbstractSpinBox[validationState="invalid"], QLineEdit[validationState="invalid"] { border: 2px solid palette(bright-text); padding: 0 7px; min-height: @fieldFocuspx; max-height: @fieldFocuspx; }
        QComboBox::drop-down { width: @space5px; border: 0; border-left: 1px solid palette(mid); }
        QComboBox::down-arrow { image: url(@glyphs/down.png); width: 8px; height: 5px; }
        QComboBox::down-arrow:disabled { image: url(@glyphs/down-off.png); }
        QAbstractSpinBox::up-button, QAbstractSpinBox::down-button { width: @space4px; border: 0; border-left: 1px solid palette(mid); background: transparent; }
        QAbstractSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; }
        QAbstractSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; }
        QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover { background: palette(midlight); }
        QAbstractSpinBox::up-button:pressed, QAbstractSpinBox::down-button:pressed { background: palette(mid); }
        QAbstractSpinBox::up-arrow { image: url(@glyphs/up.png); width: 8px; height: 5px; }
        QAbstractSpinBox::down-arrow { image: url(@glyphs/down.png); width: 8px; height: 5px; }
        QAbstractSpinBox::up-arrow:disabled, QAbstractSpinBox::up-arrow:off { image: url(@glyphs/up-off.png); }
        QAbstractSpinBox::down-arrow:disabled, QAbstractSpinBox::down-arrow:off { image: url(@glyphs/down-off.png); }
        QTreeWidget, QListWidget { border: 0; background: palette(base); outline: 0; }
        QTreeWidget::item, QListWidget::item { min-height: @controlContentpx; padding: @padYpx @space1px; border: 1px solid transparent; }
        QTreeWidget::item:hover, QListWidget::item:hover { background: palette(midlight); }
        QTreeWidget::item:selected, QListWidget::item:selected { background: palette(mid); color: palette(text); border-left: 2px solid palette(highlight); }
        QTreeWidget::item:focus, QListWidget::item:focus { border: 2px solid palette(highlight); padding: 1px 3px; }
        QTableView { background: palette(base); alternate-background-color: palette(window); border: 1px solid palette(dark); selection-background-color: palette(mid); selection-color: palette(text); gridline-color: palette(mid); }
        QTableView::item { min-height: @controlContentpx; padding: @padYpx @space1px; border-bottom: 1px solid palette(mid); }
        QTableView::item:hover { background: palette(midlight); }
        QTableView::item:focus { border: 2px solid palette(highlight); padding: 1px 3px; }
        QHeaderView::section { min-height: @controlContentpx; background: palette(window); color: palette(text); border: 0; border-right: 1px solid palette(mid); padding: @padYpx @space2px; font-size: @fontLabelpx; font-weight: 600; }
        QLabel#editorScope { background: palette(window); color: palette(text); padding: @space1px @space2px; border-bottom: 1px solid palette(mid); }
        QLabel#editorRunInfo { background: palette(base); color: palette(text); padding: @space1px @space2px; }
        QLabel#editorError, QLabel[validationState="invalid"], QLabel[status="error"] { color: palette(bright-text); padding: @space1px @space2px; }
        QLabel[status="warning"] { color: palette(link-visited); }
        QLabel[status="advisory"] { color: palette(link); }
        QLabel[status="ok"] { color: palette(shadow); }
        QLabel#editorPaletteHint, QLabel#editorToolHint { color: palette(window-text); font-size: @fontNumericpx; }
        QLabel[editorEyebrow="true"] { color: palette(window-text); font-size: @fontLabelpx; font-weight: 600; }
        QLabel#editorCommandTitle { color: palette(text); font-size: @fontTitlepx; font-weight: 600; }
        QLabel[numeric="true"] { font-size: @fontNumericpx; }
        QStatusBar { background: palette(base); border-top: 1px solid palette(mid); color: palette(text); min-height: @controlHeightpx; }
        QStatusBar::item { border: 0; }
        QToolTip { background: palette(text); color: palette(base); border: 1px solid palette(text); padding: @space1px @space2px; }
    )");
    const std::pair<const char*,int> sizes[]={
        {"@fontLabel",editorDesign::fontSizeLabel},{"@fontNumeric",editorDesign::fontSizeNumeric},
        {"@fontBody",editorDesign::fontSizeBody},{"@fontTitle",editorDesign::fontSizeTitle},
        {"@space1",editorDesign::space1},{"@space2",editorDesign::space2},{"@space4",editorDesign::space4},
        {"@space5",editorDesign::space5},{"@space6",editorDesign::space6},
        {"@controlHeight",editorDesign::controlHeight},{"@controlContent",editorDesign::controlHeight-2*editorDesign::controlPaddingY-2},
        {"@padY",editorDesign::controlPaddingY},
        {"@fieldContent",editorDesign::controlHeight-2},{"@fieldFocus",editorDesign::controlHeight-4},
        {"@radius",2},{"@icon",editorDesign::iconSize}
    };
    // No token is a prefix of another ("@space1" vs "@space6", "@controlHeight" vs "@controlContent").
    for(const auto& [name,value]:sizes)stylesheet.replace(QLatin1String(name),QString::number(value));
    stylesheet.replace(QLatin1String("@glyphs"),editorGlyphDirectory());
    return stylesheet;
}
void applyEditorStyle(QWidget* window) {
    installEditorStyle();
    window->setPalette(editorDesign::editorPalette());
    window->setStyleSheet(editorStyleSheet());
}
}
