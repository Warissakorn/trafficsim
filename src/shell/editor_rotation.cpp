#include "editor_window.hpp"
#include <QInputDialog>
#include <QLabel>
namespace trafficsim {
void EditorWindow::rotateSelection() {
    const auto ids=canvas_->selection();
    const auto pivot=canvas_->rotationPivot();
    if(!pivot)return;
    canvas_->cancel();
    QInputDialog dialog(this);dialog.setObjectName("editorRotationDialog");
    dialog.setWindowTitle(text("editorRotate"));dialog.setLabelText(text("editorRotationHelp"));
    if(auto* help=dialog.findChild<QLabel*>()) {
        help->setWordWrap(true);help->setMinimumWidth(360);help->setMaximumWidth(520);
    }
    dialog.setInputMode(QInputDialog::DoubleInput);dialog.setDoubleRange(-360,360);
    dialog.setDoubleDecimals(2);dialog.setDoubleStep(15);dialog.setDoubleValue(90);
    dialog.setOkButtonText(text("editorConfirm"));dialog.setCancelButtonText(text("editorCancel"));
    if(dialog.exec()==QDialog::Accepted)
        execute("editorRotate",[&](auto& d){rotateObjects(d,ids,*pivot,dialog.doubleValue());});
}
}
