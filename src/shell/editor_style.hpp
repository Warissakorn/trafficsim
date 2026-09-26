#pragma once
#include <QIcon>
class QWidget;
namespace trafficsim {
enum class EditorIcon { document, open, save, undo, redo, fit, finish, rotate, remove,
    select, link, connector, route, input, signal, split, measure, image, run, pause,
    step, reset, inspector, objects, focus, grid, conflict, counter };
QIcon editorIcon(EditorIcon icon);
void applyEditorStyle(QWidget* window);
}
