#include "editor_window.hpp"
#include "editor_storage.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QLabel>
#include <QLockFile>
#include <QMessageBox>
#include <QAbstractButton>
#include <QStandardPaths>
#include <QToolBar>
#include <QUuid>
namespace trafficsim {
EditorWindow::~EditorWindow() = default;
void EditorWindow::buildRecovery() {
    recoveryDirectory_=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/recovery";
    QDir().mkpath(recoveryDirectory_);
    recoveryFile_=recoveryDirectory_+"/"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".traffic.json";
    recoveryLock_=std::make_unique<QLockFile>(recoveryFile_+".lock");
    recoveryLock_->setStaleLockTime(0);
    // A lock failure costs autosave, nothing else. Returning here also skipped the toolbar
    // actions below, so an unrelated feature disappeared and the session ran unprotected.
    const bool locked=recoveryLock_->tryLock();
    if(!locked){recoveryLock_.reset();recoveryFile_.clear();showError(std::runtime_error("EDIT_RECOVERY_LOCK"));}
    auto* files=findChild<QToolBar*>("editorFiles");
    files->addAction(action("editorRecover",{},[this]{recoverDialog();}));
    files->addAction(action("editorEmbedCatalogs",{},[this]{
        try {
            const auto resolved=resolveCatalogs(history_.document().definition.value_or(AuthoringDefinition{}),data_);
            execute("editorEmbedCatalogs",[&](auto& d){
                auto& def=demand(d);def.vehicleTypes=resolved.vehicleTypes;def.behaviours=resolved.behaviours;
                def.externalVehicleTypes=false;def.externalBehaviours=false;
            });
        }catch(const std::exception& e){showError(e);}
    }));
    connect(&autosaveTimer_,&QTimer::timeout,this,[this]{
        try{autosaveNow();}catch(const std::exception& e){showError(e);}
    });
    // Recovering another draft adopts its lock, so offer the dialog either way; without a
    // lock the timer stays stopped rather than reporting the same failure every 15 seconds.
    if(locked)startAutosave();
    QTimer::singleShot(0,this,[this]{recoverDialog(true);});
}
void EditorWindow::startAutosave() {
    if(recoveryLock_ && recoveryLock_->isLocked() && !autosaveTimer_.isActive())autosaveTimer_.start(15000);
}
void EditorWindow::autosaveNow() {
    if(!history_.dirty()){clearRecovery();return;}
    if(autosavedRevision_ && *autosavedRevision_==history_.revision())return;
    if(!recoveryLock_ || !recoveryLock_->isLocked())throw std::runtime_error("EDIT_RECOVERY_LOCK");
    validateDocument(history_.document());
    auto j=documentJson(history_.document());
    j["_recovery"]={{"source",file_.toStdString()},{"savedAt",QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString()}};
    writeEditorDocument(recoveryFile_,j);autosavedRevision_=history_.revision();
}
void EditorWindow::clearRecovery() {
    if(!recoveryFile_.isEmpty())QFile::remove(recoveryFile_);
    autosavedRevision_.reset();
}
void EditorWindow::recoverFile(const QString& file) {
    if(file==recoveryFile_)throw std::runtime_error("EDIT_RECOVERY_LOCK");
    auto lock=std::make_unique<QLockFile>(file+".lock");lock->setStaleLockTime(0);
    if(!lock->tryLock())throw std::runtime_error("EDIT_RECOVERY_LOCK");
    auto document=readEditorDocument(file);
    // Restored documents are untitled, so Save asks for a destination before replacing anything.
    clearRecovery();clearRun();history_.reset(std::move(document));history_.markUnsaved();file_.clear();
    recoveryLock_=std::move(lock);recoveryFile_=file;autosavedRevision_.reset();startAutosave();
    canvas_->select("");refresh();canvas_->fitNetwork();error_->setText(text("editorRecovered"));
}
void EditorWindow::recoverDialog(bool startup) {
    QStringList paths,labels;
    const auto entries=QDir(recoveryDirectory_).entryInfoList({"*.traffic.json"},QDir::Files,QDir::Time);
    for(const auto& entry:entries) {
        if(entry.absoluteFilePath()==recoveryFile_)continue;
        QLockFile lock(entry.absoluteFilePath()+".lock");lock.setStaleLockTime(0);
        if(!lock.tryLock())continue; // Never offer another active editor's recovery.
        paths<<entry.absoluteFilePath();labels<<entry.lastModified().toString(Qt::ISODate)+" — "+entry.fileName();
    }
    if(paths.empty()){if(!startup)error_->setText(text("editorNoRecovery"));return;}
    if(startup) {
        QMessageBox box(QMessageBox::Question,text("editorRecover"),text("editorRecoveryFound"),QMessageBox::Yes|QMessageBox::No,this);
        box.button(QMessageBox::Yes)->setText(text("editorRecover"));box.button(QMessageBox::No)->setText(text("editorCancel"));
        if(box.exec()!=QMessageBox::Yes)return;
    }
    QInputDialog dialog(this);dialog.setWindowTitle(text("editorRecover"));dialog.setLabelText(text("editorChooseRecovery"));
    dialog.setComboBoxItems(labels);dialog.setComboBoxEditable(false);dialog.setOkButtonText(text("editorRecover"));dialog.setCancelButtonText(text("editorCancel"));
    if(dialog.exec()!=QDialog::Accepted)return;
    const int index=labels.indexOf(dialog.textValue());if(index<0 || !confirmDiscard())return;
    try{recoverFile(paths[index]);}catch(const std::exception& e){showError(e);}
}
}
