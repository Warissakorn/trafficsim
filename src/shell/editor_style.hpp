#pragma once
#include <QIcon>
#include <QFont>
#include <QString>
#include <filesystem>
class QWidget;
namespace trafficsim {
enum class EditorIcon { document, open, save, undo, redo, fit, finish, rotate, remove,
    select, link, connector, route, input, signal, split, measure, image, run, pause,
    step, reset, inspector, objects, focus, grid, conflict, counter };
QIcon editorIcon(EditorIcon icon);
// The complete editor QSS: palette(role) references only, no colour literal.
QString editorStyleSheet();
// Registers the bundled face (400 and 600) and makes it the application font at the body size.
// Throws when the files are missing. Idempotent (editor_appearance.cpp).
QFont loadEditorFont(const std::filesystem::path& data);
// Fusion as the application style; idempotent (editor_appearance.cpp).
void installEditorStyle();
// Where the dropdown/spin chevrons drawn from the palette live, for the style sheet's `image`.
QString editorGlyphDirectory();
void applyEditorStyle(QWidget* window);
}
