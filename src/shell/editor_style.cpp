#include "editor_style.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QString>
#include <QWidget>
#include <map>
#include <utility>
namespace trafficsim {
QIcon editorIcon(EditorIcon icon) {
    static std::map<EditorIcon,QIcon> cache;
    if(const auto found=cache.find(icon);found!=cache.end())return found->second;
    QIcon result;
    // Several raster sizes keep these small, locally drawn icons crisp at high DPI.
    for(int size:{20,40,60}) {
        QPixmap pixmap(size,size);pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);p.setRenderHint(QPainter::Antialiasing);p.scale(size/20.,size/20.);
        p.setPen(QPen(QColor(editorDesign::gray7),1.6,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
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
            p.setBrush(QColor(editorDesign::accent));p.setPen(Qt::NoPen);p.drawPolygon(points);
            if(icon==EditorIcon::step){p.setPen(QPen(QColor(editorDesign::gray7),2));line(17,4,17,16);}break;
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
void applyEditorStyle(QWidget* window) {
    QPalette palette=window->palette();
    palette.setColor(QPalette::Window,QColor(editorDesign::gray1));
    palette.setColor(QPalette::WindowText,QColor(editorDesign::gray9));
    palette.setColor(QPalette::Base,QColor(editorDesign::gray0));
    palette.setColor(QPalette::AlternateBase,QColor(editorDesign::gray1));
    palette.setColor(QPalette::Text,QColor(editorDesign::gray9));
    palette.setColor(QPalette::Button,QColor(editorDesign::gray1));
    palette.setColor(QPalette::ButtonText,QColor(editorDesign::gray9));
    palette.setColor(QPalette::Highlight,QColor(editorDesign::gray3));
    palette.setColor(QPalette::HighlightedText,QColor(editorDesign::gray9));
    palette.setColor(QPalette::Disabled,QPalette::Text,QColor(editorDesign::gray7));
    palette.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(editorDesign::gray7));
    window->setPalette(palette);
    QString stylesheet=QStringLiteral(R"(
        QWidget { color: @gray9; font-size: @fontBodypx; }
        QMainWindow::separator { background: @gray2; width: @space1px; height: @space1px; border-left: 1px solid @gray3; border-right: 1px solid @gray3; }
        QMainWindow::separator:hover { background: @gray4; }
        QMenuBar { background: @gray0; padding: 0 @space1px; border-bottom: 1px solid @gray3; min-height: @controlHeightpx; }
        QMenuBar::item { padding: @space1px @space2px; }
        QMenuBar::item:focus, QMenu::item:focus { border: 2px solid @accent; }
        QMenuBar::item:selected, QMenu::item:selected { background: @gray3; color: @gray9; }
        QMenu { background: @gray0; border: 1px solid @gray4; padding: @space1px; }
        QMenu::item { min-height: @space5px; padding: @space1px @space6px; }
        QToolBar { background: @gray0; border: 0; border-bottom: 1px solid @gray3; spacing: @space1px; padding: @space1px; }
        QToolBar::separator { background: @gray4; width: 1px; margin: @space1px; }
        QToolButton, QPushButton { min-height: @space5px; padding: @space1px @space2px; border: 1px solid @gray6; border-radius: 2px; background: @gray1; }
        QToolBar QToolButton { border-color: transparent; background: transparent; padding: @space1px; }
        QToolButton:hover, QPushButton:hover { background: @gray2; border-color: @gray7; }
        QToolButton:pressed, QPushButton:pressed, QToolButton:checked { background: @gray3; border-color: @accent; }
        QAbstractButton:focus { border: 2px solid @accent; }
        QToolButton:disabled, QPushButton:disabled { color: @gray7; background: @gray2; border-color: @gray4; }
        QToolButton#editorRunButton { background: @gray2; border-color: @accent; color: @gray9; font-weight: 600; }
        QDockWidget { border: 1px solid @gray3; }
        QDockWidget::title { background: @gray1; min-height: @space5px; padding: @space1px @space2px; color: @gray8; font-size: @fontLabelpx; font-weight: 600; }
        QDockWidget[englishLabels="true"]::title { letter-spacing: 1px; }
        QScrollArea, QTabWidget::pane { border: 0; background: @gray0; }
        QGraphicsView { background: @gray0; border: 1px solid @gray3; }
        QGraphicsView:focus { border: 2px solid @accent; }
        QTabBar::tab { min-height: @space5px; padding: @space1px @space2px; border-bottom: 1px solid transparent; color: @gray7; }
        QTabBar::tab:selected { color: @gray9; border-bottom: 2px solid @accent; background: @gray0; }
        QTabBar::tab:hover { background: @gray2; }
        QTabBar::tab:focus { border: 2px solid @accent; }
        QLineEdit, QComboBox, QAbstractSpinBox { min-height: @space5px; background: @gray0; border: 1px solid @gray6; border-radius: 2px; padding: @space1px @space2px; selection-background-color: @accent; selection-color: @gray0; }
        QAbstractSpinBox, QLineEdit[numeric="true"], QComboBox[numeric="true"] { font-family: monospace; font-size: @fontNumericpx; }
        QLineEdit:focus, QComboBox:focus, QAbstractSpinBox:focus { border: 2px solid @accent; }
        QLineEdit:read-only { background: @gray2; color: @gray7; }
        QLineEdit:disabled, QComboBox:disabled, QAbstractSpinBox:disabled { background: @gray2; color: @gray7; }
        QAbstractSpinBox[validationState="invalid"], QLineEdit[validationState="invalid"] { border: 2px solid @danger; }
        QComboBox::drop-down { width: @space6px; border: 0; border-left: 1px solid @gray3; }
        QTreeWidget, QListWidget { border: 0; background: @gray0; outline: 0; }
        QTreeWidget::item, QListWidget::item { min-height: @space5px; padding: @space1px; border: 1px solid transparent; }
        QTreeWidget::item:hover, QListWidget::item:hover { background: @gray2; }
        QTreeWidget::item:selected, QListWidget::item:selected { background: @gray3; color: @gray9; border-left: 2px solid @accent; }
        QTreeWidget::item:focus, QListWidget::item:focus { border: 2px solid @accent; }
        QTableView { background: @gray0; alternate-background-color: @gray1; border: 1px solid @gray6; selection-background-color: @gray3; selection-color: @gray9; gridline-color: @gray3; }
        QTableView::item { min-height: @space5px; padding: @space1px; border-bottom: 1px solid @gray3; }
        QTableView::item:focus { border: 2px solid @accent; }
        QHeaderView::section { min-height: @space5px; background: @gray1; color: @gray8; border: 0; border-right: 1px solid @gray3; padding: @space1px @space2px; font-size: @fontLabelpx; font-weight: 600; }
        QHeaderView[englishLabels="true"]::section { letter-spacing: 1px; }
        QLabel#editorScope { background: @gray1; color: @gray8; padding: @space1px @space2px; border-bottom: 1px solid @gray3; }
        QLabel#editorRunInfo { background: @gray0; color: @gray8; padding: @space1px @space2px; }
        QLabel#editorError { color: @danger; padding: @space1px @space2px; }
        QLabel[validationState="invalid"], QLabel#editorError { color: @danger; }
        QLabel[status="error"] { color: @danger; }
        QLabel[status="ok"] { color: @success; }
        QLabel#editorPaletteHint, QLabel#editorToolHint { color: @gray7; font-size: @fontNumericpx; }
        QLabel[editorEyebrow="true"] { color: @gray7; font-size: @fontLabelpx; font-weight: 600; }
        QLabel[editorEyebrow="true"][englishLabels="true"] { letter-spacing: 1px; }
        QLabel#editorCommandTitle { color: @gray9; font-size: @fontTitlepx; font-weight: 600; }
        QLabel[numeric="true"] { font-family: monospace; font-size: @fontNumericpx; }
        QStatusBar { background: @gray0; border-top: 1px solid @gray3; color: @gray8; min-height: @controlHeightpx; }
        QStatusBar::item { border: 0; }
        QToolTip { background: @gray9; color: @gray0; border: 1px solid @gray8; padding: @space1px @space2px; }
    )");
    const std::pair<const char*,const char*> tokens[]={
        {"@gray0",editorDesign::gray0},{"@gray1",editorDesign::gray1},{"@gray2",editorDesign::gray2},
        {"@gray3",editorDesign::gray3},{"@gray4",editorDesign::gray4},{"@gray5",editorDesign::gray5},
        {"@gray6",editorDesign::gray6},{"@gray7",editorDesign::gray7},{"@gray8",editorDesign::gray8},
        {"@gray9",editorDesign::gray9},{"@accent",editorDesign::accent},{"@danger",editorDesign::danger},
        {"@success",editorDesign::success}
    };
    for(const auto& [name,value]:tokens)stylesheet.replace(QLatin1String(name),QLatin1String(value));
    stylesheet.replace("@fontLabel",QString::number(editorDesign::fontSizeLabel));
    stylesheet.replace("@fontNumeric",QString::number(editorDesign::fontSizeNumeric));
    stylesheet.replace("@fontBody",QString::number(editorDesign::fontSizeBody));
    stylesheet.replace("@fontTitle",QString::number(editorDesign::fontSizeTitle));
    stylesheet.replace("@space1",QString::number(editorDesign::space1));
    stylesheet.replace("@space2",QString::number(editorDesign::space2));
    stylesheet.replace("@space3",QString::number(editorDesign::space3));
    stylesheet.replace("@space4",QString::number(editorDesign::space4));
    stylesheet.replace("@space5",QString::number(editorDesign::space5));
    stylesheet.replace("@space6",QString::number(editorDesign::space6));
    stylesheet.replace("@controlHeight",QString::number(editorDesign::controlHeight));
    window->setStyleSheet(stylesheet);
}
}
