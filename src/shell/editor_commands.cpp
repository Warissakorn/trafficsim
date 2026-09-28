#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QAbstractItemView>
#include <QAction>
#include <QDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSize>
#include <QVBoxLayout>
#include <algorithm>
#include <functional>

namespace trafficsim {
namespace {
class CommandSearch final : public QLineEdit {
public:
    using QLineEdit::QLineEdit;
    std::function<void(int)> moveSelection;
    std::function<void()> activateSelection;
protected:
    void keyPressEvent(QKeyEvent* event) override {
        if(event->key()==Qt::Key_Down||event->key()==Qt::Key_Up) {
            if(moveSelection)moveSelection(event->key()==Qt::Key_Down?1:-1);
            event->accept();return;
        }
        if(event->key()==Qt::Key_Return||event->key()==Qt::Key_Enter) {
            if(activateSelection)activateSelection();
            event->accept();return;
        }
        QLineEdit::keyPressEvent(event);
    }
};
}

void EditorWindow::showCommandPalette() {
    QDialog dialog(this);dialog.setObjectName("editorCommandPaletteDialog");dialog.setModal(true);
    dialog.setWindowTitle(text("editorTitle"));dialog.setMinimumWidth(560);
    auto* layout=new QVBoxLayout(&dialog);
    layout->setContentsMargins(editorDesign::space3,editorDesign::space3,editorDesign::space3,editorDesign::space3);
    layout->setSpacing(editorDesign::space2);
    auto* title=new QLabel(text("editorCommandPaletteTitle"),&dialog);title->setObjectName("editorCommandTitle");
    layout->addWidget(title);
    auto* search=new CommandSearch(&dialog);search->setObjectName("editorCommandSearch");
    search->setPlaceholderText(text("editorCommandPaletteSearch"));
    search->setAccessibleName(text("editorCommandPaletteSearch"));layout->addWidget(search);
    auto* results=new QListWidget(&dialog);results->setObjectName("editorCommandResults");
    results->setSelectionMode(QAbstractItemView::SingleSelection);results->setUniformItemSizes(true);
    results->setMinimumHeight(280);layout->addWidget(results);
    auto emptyText=text("editorCommandPaletteEmpty");
    if(language_->currentData().toString()=="en")emptyText=emptyText.toUpper();
    auto* empty=new QLabel(emptyText,&dialog);empty->setObjectName("editorCommandPaletteEmpty");
    empty->setProperty("editorEyebrow",true);empty->setProperty("englishLabels",language_->currentData().toString()=="en");
    layout->addWidget(empty);empty->hide();

    const auto rebuild=[this,search,results,empty]{
        results->clear();const QString query=search->text().trimmed();
        for(const auto& [key,command]:actions_) {
            if(key=="editorCommandPalette"||!command->isEnabled())continue;
            QString shortcuts;
            for(const auto& shortcut:command->shortcuts()) {
                if(!shortcuts.isEmpty())shortcuts+="  ";
                shortcuts+=shortcut.toString(QKeySequence::NativeText);
            }
            const QString title=command->text().replace('&'," ").simplified();
            const QString searchable=title+" "+QString::fromStdString(key)+" "+shortcuts;
            if(!query.isEmpty()&&!searchable.contains(query,Qt::CaseInsensitive))continue;
            auto* item=new QListWidgetItem(title+(shortcuts.isEmpty()?QString():"    "+shortcuts),results);
            item->setData(Qt::UserRole,QString::fromStdString(key));item->setToolTip(searchable);
            item->setSizeHint(QSize(item->sizeHint().width(),editorDesign::tableRowHeight));
        }
        empty->setVisible(results->count()==0);
        if(results->count())results->setCurrentRow(0);
    };
    const auto activate=[&dialog,results]{if(results->currentItem())dialog.accept();};
    search->moveSelection=[results](int step){
        if(!results->count())return;
        results->setCurrentRow(std::clamp(results->currentRow()+step,0,results->count()-1));
    };
    search->activateSelection=activate;
    connect(search,&QLineEdit::textChanged,&dialog,rebuild);
    connect(results,&QListWidget::itemActivated,&dialog,[&dialog](QListWidgetItem*){dialog.accept();});
    connect(results,&QListWidget::itemClicked,&dialog,[&dialog](QListWidgetItem*){dialog.accept();});
    rebuild();search->setFocus();search->selectAll();
    if(dialog.exec()!=QDialog::Accepted||!results->currentItem())return;
    const auto key=results->currentItem()->data(Qt::UserRole).toString().toStdString();
    if(const auto found=actions_.find(key);found!=actions_.end()&&found->second->isEnabled())found->second->trigger();
}
}
