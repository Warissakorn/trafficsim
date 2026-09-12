#include "editor_window.hpp"
#include <QBuffer>
#include <QCloseEvent>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QImageReader>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QAbstractButton>
#include <QSaveFile>
#include <cmath>

namespace trafficsim {
void EditorWindow::openFile(const QString& file) {
    QFile input(file);
    if(!input.open(QIODevice::ReadOnly) || input.size()>48*1024*1024) throw std::runtime_error("EDIT_FILE_READ");
    const auto bytes=input.readAll();
    auto document=parseDocument(Json::parse(bytes.constData(),bytes.constData()+bytes.size()));
    if(!document.background.pngBase64->empty()) {
        auto encoded=QByteArray::fromBase64Encoding(QByteArray::fromStdString(*document.background.pngBase64),QByteArray::AbortOnBase64DecodingErrors);
        if(!encoded) throw std::runtime_error("EDIT_BACKGROUND_INVALID");
        QBuffer imageBytes(&encoded.decoded);imageBytes.open(QIODevice::ReadOnly);QImageReader reader(&imageBytes,"PNG");
        const auto size=reader.size();
        if(!size.isValid()||static_cast<qint64>(size.width())*size.height()>32000000||reader.read().isNull())
            throw std::runtime_error("EDIT_BACKGROUND_INVALID");
    }
    history_.reset(std::move(document));file_=file;error_->clear();canvas_->select("");refresh();canvas_->fitNetwork();
}
void EditorWindow::saveFile(const QString& file) {
    validateDocument(history_.document());
    const auto bytes=documentJson(history_.document()).dump(2)+"\n";
    QSaveFile output(file); // No direct-write fallback: failed replacement must preserve the previous file.
    if(!output.open(QIODevice::WriteOnly) || output.write(bytes.data(),static_cast<qint64>(bytes.size()))!=static_cast<qint64>(bytes.size()) || !output.commit())
        throw std::runtime_error("EDIT_FILE_WRITE");
    file_=file;history_.markSaved();error_->clear();refresh();
}
bool EditorWindow::saveDialog(bool as) {
    auto file=file_;
    if(as || file.isEmpty()) file=QFileDialog::getSaveFileName(this,text("editorSave"),file.isEmpty()?"network.traffic.json":file,text("editorFilter"));
    if(file.isEmpty()) return false;
    try{saveFile(file);return true;}catch(const std::exception& e){showError(e);return false;}
}
bool EditorWindow::confirmDiscard() {
    if(!history_.dirty()) return true;
    QMessageBox box(QMessageBox::Warning,text("editorTitle"),text("editorUnsaved"),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,this);
    box.button(QMessageBox::Save)->setText(text("editorSave"));box.button(QMessageBox::Discard)->setText(text("editorDiscard"));
    box.button(QMessageBox::Cancel)->setText(text("editorCancel"));box.setDefaultButton(QMessageBox::Save);
    const auto answer=box.exec();
    if(answer==QMessageBox::Save)return saveDialog();
    return answer==QMessageBox::Discard;
}
void EditorWindow::closeEvent(QCloseEvent* event) { if(confirmDiscard()) event->accept();else event->ignore(); }
void EditorWindow::importImage() {
    const auto file=QFileDialog::getOpenFileName(this,text("editorImportImage"),{},text("editorImageFilter"));
    if(file.isEmpty())return;
    try {
        QFile source(file);if(!source.open(QIODevice::ReadOnly)||source.size()>24*1024*1024)throw std::runtime_error("EDIT_BACKGROUND_INVALID");
        QImageReader reader(&source);const auto size=reader.size();
        if(!size.isValid()||static_cast<qint64>(size.width())*size.height()>32000000)throw std::runtime_error("EDIT_BACKGROUND_INVALID");
        const auto image=reader.read();if(image.isNull())throw std::runtime_error("EDIT_BACKGROUND_INVALID");
        QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);
        if(!image.save(&buffer,"PNG"))throw std::runtime_error("EDIT_BACKGROUND_INVALID");
        const auto encoded=std::make_shared<const std::string>(png.toBase64().toStdString());
        execute("editorImportImage",[&](auto& d){d.background={};d.background.pngBase64=encoded;});canvas_->fitNetwork();
    }catch(const std::exception& e){showError(e);}
}
void EditorWindow::applyBackground() {
    execute("editorApplyImage",[&](auto& d){auto& b=d.background;b.x=bgX_->value();b.y=bgY_->value();b.metresPerPixel=bgScale_->value();b.rotation=bgAngle_->value();b.opacity=bgOpacity_->value();});
}
void EditorWindow::measure(Point a,Point b,bool calibrate) {
    const double distance=std::hypot(b.x-a.x,b.y-a.y);
    if(!calibrate){error_->setText(text("editorMeasured").arg(distance,0,'f',3));return;}
    if(history_.document().background.pngBase64->empty()||distance<1e-9){showError(std::runtime_error("EDIT_BACKGROUND_INVALID"));return;}
    QInputDialog dialog(this);dialog.setWindowTitle(text("editorCalibrate"));dialog.setLabelText(text("editorKnownDistance"));
    dialog.setInputMode(QInputDialog::DoubleInput);dialog.setDoubleRange(0.001,1000000);dialog.setDoubleDecimals(3);dialog.setDoubleValue(distance);
    dialog.setOkButtonText(text("editorConfirm"));dialog.setCancelButtonText(text("editorCancel"));
    if(dialog.exec()!=QDialog::Accepted)return;
    const double metres=dialog.doubleValue();
    execute("editorCalibrate",[&](auto& d){auto& bg=d.background;const double ratio=metres/distance;
        bg.metresPerPixel*=ratio;bg.x=a.x+(bg.x-a.x)*ratio;bg.y=a.y+(bg.y-a.y)*ratio;
    });
}
}
