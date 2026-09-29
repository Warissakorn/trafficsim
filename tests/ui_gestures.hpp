#pragma once
// Canvas gestures shared by the Qt UI suites. D84: every tool creates or changes with
// Ctrl+right-click or Ctrl+right-drag; a plain left click only selects.
#include "../src/editor/canvas.hpp"
#include <QApplication>
#include <QDialog>
#include <QTest>
#include <QTimer>
namespace trafficsim::test {
// Accepts the next modal dialog as it opens. Polls, because QTest's mouse events process timers
// before the release opens the modal; never throws through a Qt event handler.
inline void acceptNextDialog() {
    auto* timer=new QTimer(qApp);
    QObject::connect(timer,&QTimer::timeout,timer,[timer]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
        timer->stop();timer->deleteLater();dialog->accept();
    });
    timer->start(5);
}
// Ctrl+right-click: create or change whatever the current tool authors at `at`.
inline void authorClick(EditorCanvas* c,QPoint at) {
    QTest::mouseClick(c->viewport(),Qt::RightButton,Qt::ControlModifier,at);
    QApplication::processEvents();
}
// Ctrl+right-drag from a to b: draws a Link (or a Connector between lanes), accepting the dialog.
inline void drawLink(EditorCanvas* c,QPoint a,QPoint b) {
    acceptNextDialog();
    QTest::mousePress(c->viewport(),Qt::RightButton,Qt::ControlModifier,a);
    QTest::mouseMove(c->viewport(),b);
    QTest::mouseRelease(c->viewport(),Qt::RightButton,Qt::ControlModifier,b);
    // The offscreen platform has no window manager to reactivate the editor after the modal.
    QApplication::setActiveWindow(c->window());c->setFocus();QTest::keyRelease(c,Qt::Key_Control);
    QTest::qWait(10);QApplication::processEvents();
}
}
