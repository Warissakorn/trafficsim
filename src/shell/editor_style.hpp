#pragma once
#include <QIcon>
#include <QString>
class QWidget;
namespace trafficsim {
enum class EditorIcon { document, open, save, undo, redo, fit, finish, rotate, remove,
    select, link, connector, route, input, signal, split, measure, image, run, pause,
    step, reset, inspector, objects, focus, grid, conflict, counter };
QIcon editorIcon(EditorIcon icon);
// The complete editor QSS: palette(role) references only, no colour literal.
QString editorStyleSheet();
// Fusion as the application style; idempotent (editor_glyphs.cpp).
void installEditorStyle();
// Where the dropdown/spin chevrons drawn from the palette live, for the style sheet's `image`.
QString editorGlyphDirectory();
void applyEditorStyle(QWidget* window);
}
