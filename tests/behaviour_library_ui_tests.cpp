#include "../src/shell/editor_window.hpp"
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/editor/canvas.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
using namespace trafficsim;
// M3.3.2c (D128): BA19/BA20 automated evidence for the behaviour library dialog and road assignment.
namespace {
const char* kDefault = "wiedemann-inspired-prototype";
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class T> T* item(QObject& parent, const char* name) { auto* v = parent.findChild<T*>(name); require(v, name); return v; }
void modal(const std::function<void()>& open, const std::function<void(QDialog&)>& inspect) {
    std::exception_ptr failure;
    QTimer::singleShot(0, [&] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        try { require(dialog, "Missing modal dialog"); inspect(*dialog); }
        catch (...) { failure = std::current_exception(); if (dialog) dialog->reject(); }
    });
    open(); if (failure) std::rethrow_exception(failure);
}
void select(QTableWidget& table, const std::string& id) {
    for (int r = 0; r < table.rowCount(); ++r) if (table.item(r, 0)->text().toStdString() == id) { table.selectRow(r); return; }
    require(false, "Missing library row");
}
void confirm(QDialog& d) { item<QDialogButtonBox>(d, "")->button(QDialogButtonBox::Ok)->click(); }
void click(QDialog& d, const char* table, const char* button, const std::string& row = {}) {
    if (!row.empty()) select(*item<QTableWidget>(d, table), row);
    item<QPushButton>(d, (std::string(table) + button).c_str())->click();
}
int rows(QDialog& d, const char* table) { return item<QTableWidget>(d, table)->rowCount(); }
// Library edits through the dialog: duplicate the default set, slow its copy, a heavy class, an urban type.
void buildLibrary(QDialog& d) {
    click(d, "editorBehaviours", "editorBehaviourDuplicate", kDefault);
    require(rows(d, "editorBehaviours") == 2, "Duplicate did not add an independent set");
    modal([&] { click(d, "editorBehaviours", "catalogEdit", std::string(kDefault) + "-copy"); }, [](QDialog& e) {
        require(item<QLabel>(e, "editorBehaviourUsers")->text().size() > 0, "Users are not shown first");
        item<QLineEdit>(e, "editorBehaviourName")->setText("Slow");
        item<QDoubleSpinBox>(e, "followingTime")->setValue(2.5); confirm(e);
    });
    modal([&] { click(d, "editorVehicleClasses", "catalogAdd"); }, [](QDialog& e) {
        item<QLineEdit>(e, "editorClassName")->setText("Heavy");
        auto* types = item<QListWidget>(e, "editorClassTypes");
        for (int i = 0; i < types->count(); ++i) if (types->item(i)->text() == "heavy-vehicle") types->item(i)->setCheckState(Qt::Checked);
        confirm(e);
    });
    modal([&] { click(d, "editorBehaviourTypes", "catalogAdd"); }, [](QDialog& e) {
        item<QLineEdit>(e, "editorBehaviourTypeName")->setText("Urban");
        auto* overrides = item<QTableWidget>(e, "editorBehaviourTypeOverrides");
        require(overrides->rowCount() == 1, "Override table lacks the class");
        auto* choice = qobject_cast<QComboBox*>(overrides->cellWidget(0, 1));
        choice->setCurrentIndex(choice->findData(QString(kDefault) + "-copy")); confirm(e);
    });
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv); QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 1, "Expected data directory"); QTemporaryDir temp; require(temp.isValid(), "Missing temp directory");
        ProjectDocument d; const auto a = addLink(d, {{0, 0}, {200, 0}}, 1, 3.5), b = addLink(d, {{220, 0}, {400, 0}}, 1, 3.5);
        addConnector(d, {a, d.network.links[0].lanes[0].id, {}}, {b, d.network.links[1].lanes[0].id, {}});
        const auto connector = d.network.connectors[0].id;
        const auto route = putRoute(d, {"route", {a, connector, b}}); changeRunSettings(d, 60, .1);
        putInput(d, {"in", route, "car", 600, 0, 60});
        const auto path = temp.filePath("library.traffic.json"); QFile f(path); require(f.open(QIODevice::WriteOnly), "Fixture write failed");
        f.write(QByteArray::fromStdString(documentJson(d).dump())); f.close();
        EditorWindow w(argv[1]); w.openFile(path); w.show(); QApplication::processEvents();
        const auto original = documentJson(w.history().document()); const auto revision = w.history().revision();
        require(w.history().document().definition->externalBehaviours, "Fixture should start with external catalogs");
        // Cancel discards every staged edit, including the catalog capture.
        modal([&] { item<QAction>(w, "editorBehaviourLibrary")->trigger(); }, [](QDialog& d) { buildLibrary(d); d.reject(); });
        require(documentJson(w.history().document()) == original, "Cancel leaked staged edits");
        // Confirm without an edit: no revision, catalogs stay external.
        modal([&] { item<QAction>(w, "editorBehaviourLibrary")->trigger(); }, [](QDialog& d) { confirm(d); });
        require(w.history().revision() == revision && documentJson(w.history().document()) == original, "No-op confirm made a revision");
        // An invalid set keeps the dialog open with the reason and commits nothing.
        modal([&] { item<QAction>(w, "editorBehaviourLibrary")->trigger(); }, [](QDialog& d) {
            modal([&] { click(d, "editorBehaviours", "catalogEdit", kDefault); }, [](QDialog& e) {
                item<QDoubleSpinBox>(e, "followingTime")->setValue(0); confirm(e);
            });
            confirm(d);
            require(d.isVisible() && !item<QLabel>(d, "editorBehaviourError")->text().isEmpty(), "Invalid behaviour was accepted");
            d.reject();
        });
        require(w.history().revision() == revision, "Invalid edit made a revision");
        // Capture + duplicate + edit + class + type: one revision.
        modal([&] { item<QAction>(w, "editorBehaviourLibrary")->trigger(); }, [](QDialog& d) { buildLibrary(d); confirm(d); });
        const auto built = w.history().document(); const auto& def = *built.definition;
        require(built.revision != revision && w.history().canUndo(), "Library confirm did not commit");
        require(!def.externalBehaviours && !def.externalVehicleTypes, "Catalogs were not captured");
        require(def.behaviours.size() == 2 && def.behaviourNames.at(std::string(kDefault) + "-copy") == "Slow", "Behaviour copy lost");
        require(def.vehicleClasses.size() == 1 && def.linkBehaviourTypes.size() == 1, "Class or behaviour type lost");
        require(def.linkBehaviourTypes[0].overrides.size() == 1, "Override lost");
        item<QAction>(w, "editorUndo")->trigger();
        require(documentJson(w.history().document()) == original, "One Undo did not restore the library edit");
        item<QAction>(w, "editorRedo")->trigger();
        require(documentJson(w.history().document()) == documentJson(built), "Redo lost the library edit");
        // Bulk road assignment: two Links and the Connector in one revision.
        auto* canvas = w.canvas(); canvas->setSelection({a, b, connector}); QApplication::processEvents();
        auto* combo = item<QComboBox>(w, "editorBehaviourType");
        combo->setCurrentIndex(combo->findData(QString::fromStdString(def.linkBehaviourTypes[0].id)));
        const auto beforeAssign = w.history().revision();
        item<QAction>(w, "editorApplyBehaviourType")->trigger();
        const auto& n = w.history().document().network;
        require(n.links[0].behaviourTypeId && n.links[1].behaviourTypeId && n.connectors[0].behaviourTypeId, "Bulk assignment missed a road");
        require(w.history().revision() != beforeAssign, "Assignment made no revision");
        item<QAction>(w, "editorUndo")->trigger();
        require(!w.history().document().network.links[1].behaviourTypeId, "One Undo did not clear the bulk assignment");
        item<QAction>(w, "editorRedo")->trigger(); canvas->select(a); QApplication::processEvents();
        const auto effective = item<QLabel>(w, "editorEffectiveBehaviour")->text();
        require(effective.contains("heavy-vehicle: " + QString(kDefault) + "-copy") && effective.contains("class override") &&
                effective.contains("behaviour type default"), "Effective values are not distinguished");
        // A referenced behaviour is deleted only with a replacement; cancelling it keeps the entry.
        modal([&] { item<QAction>(w, "editorBehaviourLibrary")->trigger(); }, [](QDialog& d) {
            modal([&] { click(d, "editorBehaviours", "catalogDelete", std::string(kDefault) + "-copy"); }, [](QDialog& r) { r.reject(); });
            require(rows(d, "editorBehaviours") == 2, "Referenced behaviour deleted without a replacement");
            modal([&] { click(d, "editorBehaviours", "catalogDelete", std::string(kDefault) + "-copy"); }, [](QDialog& r) {
                require(item<QComboBox>(r, "editorReplacement")->currentData().toString() == kDefault, "Replacement not offered");
                confirm(r);
            });
            require(rows(d, "editorBehaviours") == 1, "Replacement delete did not remove the behaviour");
            confirm(d);
        });
        require(w.history().document().definition->linkBehaviourTypes[0].overrides[0].behaviourId == kDefault, "References not rewritten");
        item<QAction>(w, "editorUndo")->trigger();
        require(w.history().document().definition->behaviours.size() == 2, "One Undo did not restore the deleted behaviour");
        // EN/TH: every new control has text in Thai too.
        item<QComboBox>(w, "editorLanguage")->setCurrentIndex(1); QApplication::processEvents(); canvas->select(a); QApplication::processEvents();
        for (const char* key : {"editorBehaviourLibrary", "editorApplyBehaviourType"})
            require(!item<QAction>(w, key)->text().isEmpty(), "Untranslated action");
        require(!item<QComboBox>(w, "editorBehaviourType")->itemText(0).isEmpty(), "Untranslated inherit option");
        require(!item<QLabel>(w, "editorEffectiveBehaviour")->text().contains("class override"), "Effective label not retranslated");
        modal([&] { item<QAction>(w, "editorBehaviourLibrary")->trigger(); }, [](QDialog& d) {
            require(!d.windowTitle().isEmpty() && !item<QLabel>(d, "editorBehaviourError")->parentWidget()->windowTitle().isEmpty(), "Untranslated dialog");
            d.reject();
        });
        // Save and reopen keep the library and the assignment.
        const auto saved = documentJson(w.history().document());
        w.saveFile(path); w.openFile(path);
        require(documentJson(w.history().document()) == saved, "Library save/reopen failed");
        // D131: a w74 behaviour opens read-only -- a note instead of prototype fields; only its name
        // changes, and the file stays schema 22.
        auto withW74 = w.history().document();
        DriverBehaviour w74{"w74-set"};
        w74.w74 = W74Parameters{2, 2, 1, 2, .5, 16, 4, .5, .5, 20, 1, 1, 3, .25, .5, -1, .5, .25};
        putBehaviour(withW74, w74, ""); validateDocument(withW74);
        QFile g(path); require(g.open(QIODevice::WriteOnly), "Fixture write failed");
        g.write(QByteArray::fromStdString(documentJson(withW74).dump())); g.close(); w.openFile(path);
        modal([&] { item<QAction>(w, "editorBehaviourLibrary")->trigger(); }, [](QDialog& d) {
            modal([&] { click(d, "editorBehaviours", "catalogEdit", "w74-set"); }, [](QDialog& e) {
                require(!item<QLabel>(e, "editorBehaviourW74ReadOnly")->text().isEmpty(), "Missing w74 note");
                require(!e.findChild<QDoubleSpinBox*>("followingTime"), "Prototype fields shown for w74");
                item<QLineEdit>(e, "editorBehaviourName")->setText("Urban W74"); confirm(e);
            });
            confirm(d);
        });
        const auto& renamed = *w.history().document().definition;
        require(renamed.behaviourNames.at("w74-set") == "Urban W74", "w74 name not edited");
        require(std::find(renamed.behaviours.begin(), renamed.behaviours.end(), w74) != renamed.behaviours.end(), "w74 values changed");
        require(documentJson(w.history().document())["schemaVersion"] == 22, "w74 file not schema 22");
        std::cout << "behaviour library UI tests passed\n";
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
