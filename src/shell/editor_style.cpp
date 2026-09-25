#include "editor_style.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QWidget>
#include <map>
namespace trafficsim {
QIcon editorIcon(EditorIcon icon) {
    static std::map<EditorIcon,QIcon> cache;
    if(const auto found=cache.find(icon);found!=cache.end())return found->second;
    QIcon result;
    // Several raster sizes keep these small, locally drawn icons crisp at high DPI.
    for(int size:{20,40,60}) {
        QPixmap pixmap(size,size);pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);p.setRenderHint(QPainter::Antialiasing);p.scale(size/20.,size/20.);
        p.setPen(QPen(QColor("#475569"),1.6,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
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
            p.setBrush(QColor("#0f766e"));p.setPen(Qt::NoPen);p.drawPolygon(points);
            if(icon==EditorIcon::step){p.setPen(QPen(QColor("#475569"),2));line(17,4,17,16);}break;
        }
        case EditorIcon::pause: line(7,3,7,17);line(13,3,13,17);break;
        case EditorIcon::inspector: box(2,3,16,14);line(12,3,12,17);line(14,7,16,7);line(14,11,16,11);break;
        case EditorIcon::objects: box(2,3,16,14);line(2,8,18,8);line(2,12,18,12);line(8,3,8,17);break;
        case EditorIcon::conflict: line(2,7,18,7);line(2,13,18,13);line(7,2,7,18);line(13,2,13,18);box(7,7,6,6);break;
        case EditorIcon::grid: for(int v:{4,10,16}){line(v,2,v,18);line(2,v,18,v);}break;
        }
        p.end();result.addPixmap(pixmap);
    }
    cache.emplace(icon,result);return result;
}
void applyEditorStyle(QWidget* window) {
    QPalette palette=window->palette();
    palette.setColor(QPalette::Window,QColor("#f3f6fa"));
    palette.setColor(QPalette::WindowText,QColor("#243247"));
    palette.setColor(QPalette::Base,Qt::white);
    palette.setColor(QPalette::AlternateBase,QColor("#f6f8fb"));
    palette.setColor(QPalette::Text,QColor("#243247"));
    palette.setColor(QPalette::Button,QColor("#f8fafc"));
    palette.setColor(QPalette::ButtonText,QColor("#243247"));
    palette.setColor(QPalette::Highlight,QColor("#d8eeeb"));
    palette.setColor(QPalette::HighlightedText,QColor("#115e59"));
    palette.setColor(QPalette::Disabled,QPalette::Text,QColor("#8793a3"));
    palette.setColor(QPalette::Disabled,QPalette::ButtonText,QColor("#8793a3"));
    window->setPalette(palette);
    window->setStyleSheet(QStringLiteral(R"(
        QMainWindow::separator { background: #e2e8f0; width: 5px; height: 5px; }
        QMainWindow::separator:hover { background: #94c9c2; }
        QMenuBar { background: #ffffff; padding: 2px 6px; border-bottom: 1px solid #e2e8f0; }
        QMenuBar::item { padding: 4px 10px; border-radius: 4px; }
        QMenuBar::item:selected, QMenu::item:selected { background: #d8eeeb; color: #115e59; }
        QMenu { background: white; border: 1px solid #d6dee8; padding: 5px; }
        QMenu::item { padding: 6px 24px; }
        QToolBar { background: #ffffff; border: 0; spacing: 2px; padding: 4px; }
        QToolBar::separator { background: #e2e8f0; width: 1px; margin: 5px; }
        QToolButton, QPushButton { padding: 5px 7px; border: 1px solid #dbe3ec; border-radius: 5px; background: #f8fafc; }
        QToolBar QToolButton { border-color: transparent; background: transparent; padding: 4px; }
        QToolButton:hover, QPushButton:hover { background: #eaf2f5; border-color: #b9ccd5; }
        QToolButton:pressed, QPushButton:pressed, QToolButton:checked { background: #d8eeeb; border-color: #94c9c2; color: #115e59; }
        QToolButton:focus, QPushButton:focus { border-color: #0f766e; }
        QToolButton#editorRunButton { background: #d8eeeb; border-color: #94c9c2; color: #115e59; font-weight: 600; }
        QDockWidget { border: 1px solid #e2e8f0; }
        QDockWidget::title { background: #eaf0f5; padding: 7px 9px; font-weight: 600; }
        QScrollArea, QTabWidget::pane { border: 0; }
        QTabBar::tab { padding: 7px 10px; border-bottom: 2px solid transparent; color: #64748b; }
        QTabBar::tab:selected { color: #0f766e; border-bottom-color: #0f766e; background: #ffffff; }
        QTabBar::tab:hover { background: #eaf2f5; }
        QLineEdit, QComboBox, QAbstractSpinBox { background: white; border: 1px solid #cfd9e4; border-radius: 4px; padding: 4px 6px; min-height: 18px; }
        QLineEdit:focus, QComboBox:focus, QAbstractSpinBox:focus { border-color: #0f766e; }
        QLineEdit:read-only { background: #eaf0f5; color: #64748b; }
        QLineEdit:disabled, QComboBox:disabled, QAbstractSpinBox:disabled { background: #f0f3f7; color: #8793a3; }
        QListWidget { border: 0; background: transparent; outline: 0; }
        QListWidget::item { padding: 6px; border-radius: 5px; }
        QListWidget::item:hover { background: #eaf2f5; }
        QListWidget::item:selected { background: #d8eeeb; color: #115e59; }
        QListWidget::item:focus { border: 1px solid #0f766e; }
        QTableView { background: white; alternate-background-color: #f6f8fb; border: 1px solid #e2e8f0; selection-background-color: #d8eeeb; selection-color: #115e59; }
        QHeaderView::section { background: #eef3f7; color: #475569; border: 0; border-right: 1px solid #e2e8f0; padding: 5px 8px; font-weight: 600; }
        QLabel#editorScope { background: #fff5db; color: #795c16; padding: 5px 8px; border-radius: 5px; }
        QLabel#editorRunInfo { background: #ffffff; color: #475569; padding: 5px 8px; border-radius: 5px; }
        QLabel#editorError { color: #a5263c; padding: 2px 6px; }
        QLabel#editorPaletteHint, QLabel#editorToolHint { color: #64748b; }
        QStatusBar { background: #ffffff; border-top: 1px solid #e2e8f0; color: #64748b; }
        QStatusBar::item { border: 0; }
        QToolTip { background: #243247; color: #ffffff; border: 0; padding: 6px; }
    )"));
}
}
