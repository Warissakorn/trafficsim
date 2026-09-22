#include "editor_window.hpp"
#include <QAction>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

namespace trafficsim {
void EditorWindow::buildHistory() {
    auto* dock = new QDockWidget(this);
    dock->setObjectName("editorHistoryDock");
    texts_["editorHistory"] = dock;
    auto* body = new QWidget(dock);
    auto* layout = new QVBoxLayout(body);
    auto* help = new QLabel(body);
    help->setWordWrap(true);
    texts_["editorHistoryHelp"] = help;
    layout->addWidget(help);
    historyList_ = new QListWidget(body);
    historyList_->setObjectName("editorHistoryList");
    layout->addWidget(historyList_);
    auto* buttons = new QHBoxLayout;
    for (const auto* key : {"editorUndo", "editorRedo"}) {
        auto* button = new QToolButton(body);
        button->setDefaultAction(actions_.at(key));
        buttons->addWidget(button);
    }
    buttons->addStretch();
    layout->addLayout(buttons);
    // Browsing with the arrow keys is harmless; Enter/double-click explicitly restores.
    connect(historyList_, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        restoreHistory(item->data(Qt::UserRole).toULongLong());
    });
    dock->setWidget(body);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    auto* toggle = dock->toggleViewAction();
    toggle->setObjectName("editorHistory");
    toggle->setShortcut(QKeySequence("Ctrl+Shift+H"));
    actions_["editorHistory"] = toggle;
    addAction(toggle);
    dock->hide();
}

void EditorWindow::refreshHistory() {
    if (!historyList_) return;
    const auto command = [this](const std::string& name) {
        const auto translated = text(name);
        return translated.isEmpty() ? text("editorHistoryChange") : translated;
    };
    for (const auto* key : {"editorUndo", "editorRedo"}) {
        const auto name = std::string(key)=="editorUndo" ? history_.undoName() : history_.redoName();
        actions_.at(key)->setText(name.empty() ? text(key) : text(key)+" — "+command(name));
    }
    const QSignalBlocker block(historyList_);
    historyList_->clear();
    historyList_->setAccessibleName(text("editorHistory"));
    for (const auto& state : history_.states()) {
        const bool current = state.revision == history_.revision();
        auto label = state.name.empty() ? text("editorHistoryStart") : command(state.name);
        if (state.saved) label += " · "+text("editorHistorySaved");
        if (current) label += " · "+text("editorHistoryCurrent");
        auto* item = new QListWidgetItem(label, historyList_);
        item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(state.revision));
        auto font = item->font(); font.setBold(current); item->setFont(font);
        if (current) historyList_->setCurrentItem(item);
    }
}

void EditorWindow::restoreHistory(std::uint64_t revision) {
    try {
        if (!history_.restore(revision)) return;
        clearRun();
        rejected_.clear(); error_->clear();
        refresh();
    } catch (const std::exception& error) { showError(error); }
}
}
