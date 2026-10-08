#include "behaviour_library_dialog.hpp"
#include "behaviour_editor.hpp"
#include "../commands/behaviour_commands.hpp"
#include "../core/validate.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
namespace trafficsim {
namespace {
QString q(const std::string& s) { return QString::fromStdString(s); }
void okCancel(QDialog& dialog, QLayout* layout, const CatalogText& text) {
    auto* b = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); layout->addWidget(b);
    b->button(QDialogButtonBox::Ok)->setText(text("editorConfirm")); b->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    QObject::connect(b, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(b, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
}
std::string usersText(const LibraryUsers& users, const CatalogText& text) {
    QStringList all;
    for (const auto* list : {&users.vehicleTypes, &users.behaviourTypes, &users.roads})
        for (const auto& id : *list) all << q(id);
    return (all.isEmpty() ? text("editorBehaviourUnused") : text("editorBehaviourUsedBy").arg(all.join(", "))).toStdString();
}
QLabel* note(QDialog& dialog, QFormLayout* form, const std::string& value, const char* object) {
    auto* label = new QLabel(q(value), &dialog); label->setObjectName(object); label->setWordWrap(true); form->addRow(label); return label;
}
bool editClass(QWidget* parent, VehicleClass& c, const AuthoringDefinition& d, const CatalogText& text) {
    QDialog dialog(parent); dialog.setObjectName("editorClassEditDialog"); dialog.setWindowTitle(text("editorBehaviourTabClasses"));
    auto* form = new QFormLayout(&dialog); form->addRow(text("editorColumnId"), new QLabel(q(c.id), &dialog));
    auto* label = new QLineEdit(q(c.name), &dialog); label->setObjectName("editorClassName"); form->addRow(text("editorColumnName"), label);
    auto* members = new QListWidget(&dialog); members->setObjectName("editorClassTypes"); form->addRow(text("editorBehaviourMembers"), members);
    for (const auto& t : d.vehicleTypes) {
        auto* item = new QListWidgetItem(q(t.id), members);
        item->setCheckState(std::find(c.vehicleTypeIds.begin(), c.vehicleTypeIds.end(), t.id) != c.vehicleTypeIds.end() ? Qt::Checked : Qt::Unchecked);
        // A type belongs to at most one class: members of another class are shown, not offered.
        for (const auto& other : d.vehicleClasses)
            if (other.id != c.id && std::find(other.vehicleTypeIds.begin(), other.vehicleTypeIds.end(), t.id) != other.vehicleTypeIds.end())
                item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }
    okCancel(dialog, form, text);
    if (dialog.exec() != QDialog::Accepted) return false;
    c.name = label->text().trimmed().toStdString(); c.vehicleTypeIds.clear();
    for (int i = 0; i < members->count(); ++i)
        if (members->item(i)->checkState() == Qt::Checked) c.vehicleTypeIds.push_back(members->item(i)->text().toStdString());
    return true;
}
QComboBox* behaviourChoice(QWidget* parent, const AuthoringDefinition& d, const std::string& current, const QString& inherit) {
    auto* combo = new QComboBox(parent);
    if (!inherit.isEmpty()) combo->addItem(inherit, QString());
    for (const auto& b : d.behaviours) {
        const auto name = d.behaviourNames.find(b.id);
        combo->addItem(name == d.behaviourNames.end() ? q(b.id) : q(name->second) + " (" + q(b.id) + ")", q(b.id));
    }
    combo->setCurrentIndex(std::max(0, combo->findData(q(current)))); return combo;
}
bool editType(QWidget* parent, LinkBehaviourType& t, const AuthoringDefinition& d, const std::string& users, const CatalogText& text) {
    QDialog dialog(parent); dialog.setObjectName("editorBehaviourTypeEditDialog"); dialog.setWindowTitle(text("editorBehaviourTabTypes"));
    auto* form = new QFormLayout(&dialog); note(dialog, form, users, "editorBehaviourUsers");
    form->addRow(text("editorColumnId"), new QLabel(q(t.id), &dialog));
    auto* label = new QLineEdit(q(t.name), &dialog); label->setObjectName("editorBehaviourTypeName"); form->addRow(text("editorColumnName"), label);
    auto* fallback = behaviourChoice(&dialog, d, t.defaultBehaviourId, {}); fallback->setObjectName("editorBehaviourTypeDefault");
    form->addRow(text("editorBehaviourDefault"), fallback);
    auto* overrides = new QTableWidget(static_cast<int>(d.vehicleClasses.size()), 2, &dialog); overrides->setObjectName("editorBehaviourTypeOverrides");
    overrides->setHorizontalHeaderLabels({text("editorBehaviourClassColumn"), text("editorBehaviourOverrides")});
    overrides->horizontalHeader()->setStretchLastSection(true); overrides->verticalHeader()->hide();
    for (int r = 0; r < overrides->rowCount(); ++r) {
        const auto& c = d.vehicleClasses[static_cast<std::size_t>(r)];
        overrides->setItem(r, 0, new QTableWidgetItem(q(c.id)));
        const auto o = std::find_if(t.overrides.begin(), t.overrides.end(), [&](const auto& x) { return x.classId == c.id; });
        overrides->setCellWidget(r, 1, behaviourChoice(overrides, d, o == t.overrides.end() ? std::string{} : o->behaviourId, text("editorBehaviourInherit")));
    }
    form->addRow(text("editorBehaviourOverrides"), overrides);
    okCancel(dialog, form, text);
    if (dialog.exec() != QDialog::Accepted) return false;
    t.name = label->text().trimmed().toStdString(); t.defaultBehaviourId = fallback->currentData().toString().toStdString();
    t.overrides.clear();
    for (int r = 0; r < overrides->rowCount(); ++r) {
        const auto chosen = qobject_cast<QComboBox*>(overrides->cellWidget(r, 1))->currentData().toString().toStdString();
        if (!chosen.empty()) t.overrides.push_back({overrides->item(r, 0)->text().toStdString(), chosen}); // empty: inherit
    }
    return true;
}
// A referenced entry is deleted only with a replacement the author picks (contract §1).
std::optional<std::optional<std::string>> replacement(QWidget* parent, const std::vector<std::string>& candidates,
                                                     const std::string& users, const CatalogText& text) {
    if (users.empty()) return std::optional<std::string>{};
    QDialog dialog(parent); dialog.setObjectName("editorReplacementDialog"); dialog.setWindowTitle(text("editorBehaviourReplacement"));
    auto* form = new QFormLayout(&dialog); note(dialog, form, users, "editorBehaviourUsers");
    auto* choice = new QComboBox(&dialog); choice->setObjectName("editorReplacement");
    for (const auto& id : candidates) choice->addItem(q(id), q(id));
    form->addRow(text("editorBehaviourReplacement"), choice); okCancel(dialog, form, text);
    if (dialog.exec() != QDialog::Accepted || choice->count() == 0) return std::nullopt;
    return std::optional<std::string>(choice->currentData().toString().toStdString());
}
template<class T> std::string nextId(const std::vector<T>& values, const std::string& prefix) {
    for (unsigned n = 1;; ++n) {
        const auto id = prefix + std::to_string(n);
        if (std::none_of(values.begin(), values.end(), [&](const auto& v) { return v.id == id; })) return id;
    }
}
template<class T> std::vector<std::string> others(const std::vector<T>& values, const std::string& id) {
    std::vector<std::string> result;
    for (const auto& v : values) if (v.id != id) result.push_back(v.id);
    return result;
}
}
bool editBehaviourLibrary(QWidget* parent, ProjectDocument& staged, const CatalogText& text) {
    QDialog dialog(parent); dialog.setObjectName("editorBehaviourDialog"); dialog.setWindowTitle(text("editorBehaviourLibrary"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* help = new QLabel(text("editorBehaviourHelp"), &dialog); help->setWordWrap(true); layout->addWidget(help);
    auto* tabs = new QTabWidget(&dialog); layout->addWidget(tabs);
    auto* error = new QLabel(&dialog); error->setObjectName("editorBehaviourError"); error->setWordWrap(true);
    const auto& d = *staged.definition;
    struct Page { QTableWidget* table{}; std::function<std::vector<std::pair<std::string, std::string>>()> rows; };
    std::vector<Page> pages;
    const auto refresh = [&] {
        for (auto& page : pages) {
            page.table->setRowCount(0);
            for (const auto& [id, name] : page.rows()) {
                const int at = page.table->rowCount(); page.table->insertRow(at);
                page.table->setItem(at, 0, new QTableWidgetItem(q(id))); page.table->setItem(at, 1, new QTableWidgetItem(q(name)));
            }
        }
    };
    // Each command edits `staged`; a refused one reports inline and leaves the rest of the session.
    const auto attempt = [&](const std::function<void()>& change) {
        try { change(); error->clear(); }
        catch (const ValidationError& e) { error->setText(text(e.issues.front().code.c_str()) + " · " + q(e.issues.front().path)); }
        catch (const std::exception& e) { error->setText(text(e.what())); }
        refresh();
    };
    const auto selected = [](QTableWidget* table) {
        const int row = table->currentRow();
        return row < 0 || !table->item(row, 0) ? std::string{} : table->item(row, 0)->text().toStdString();
    };
    struct Actions { std::function<void()> add, duplicate, edit, remove; };
    const auto page = [&](const char* key, const char* object, Page rows, const std::function<Actions(QTableWidget*)>& actions) {
        auto* body = new QWidget(tabs); auto* box = new QVBoxLayout(body);
        rows.table = new QTableWidget(0, 2, body); rows.table->setObjectName(object);
        rows.table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName")});
        rows.table->setEditTriggers(QAbstractItemView::NoEditTriggers); rows.table->setSelectionBehavior(QAbstractItemView::SelectRows);
        rows.table->setSelectionMode(QAbstractItemView::SingleSelection); rows.table->horizontalHeader()->setStretchLastSection(true);
        box->addWidget(rows.table);
        const auto a = actions(rows.table);
        for (const auto& [name, run] : std::vector<std::pair<const char*, std::function<void()>>>{
                 {"catalogAdd", a.add}, {"editorBehaviourDuplicate", a.duplicate}, {"catalogEdit", a.edit}, {"catalogDelete", a.remove}}) {
            auto* button = new QPushButton(text(name), body); button->setObjectName(QString(object) + name); box->addWidget(button);
            QObject::connect(button, &QPushButton::clicked, &dialog, run);
        }
        QObject::connect(rows.table, &QTableWidget::cellDoubleClicked, &dialog, [edit = a.edit](int, int) { edit(); });
        tabs->addTab(body, text(key)); pages.push_back(rows);
    };
    page("editorBehaviourTabBehaviours", "editorBehaviours", {nullptr, [&] {
        std::vector<std::pair<std::string, std::string>> rows;
        for (const auto& b : d.behaviours) { const auto n = d.behaviourNames.find(b.id); rows.push_back({b.id, n == d.behaviourNames.end() ? "" : n->second}); }
        return rows; }}, [&](QTableWidget* table) {
        const auto edit = [&, table](bool create) {
            const auto id = selected(table); if (!create && id.empty()) return;
            auto value = create ? (d.behaviours.empty() ? DriverBehaviour{} : d.behaviours.front()) : *std::find_if(d.behaviours.begin(), d.behaviours.end(), [&](const auto& b) { return b.id == id; });
            std::string name; if (const auto n = d.behaviourNames.find(value.id); !create && n != d.behaviourNames.end()) name = n->second;
            if (create) value.id = nextId(d.behaviours, "behaviour-");
            if (editBehaviour(&dialog, value, name, create ? "" : usersText(behaviourUsers(staged, id), text), d.behaviours, text))
                attempt([&] { putBehaviour(staged, value, name); });
        };
        return Actions{[edit] { edit(true); }, [&, table] { const auto id = selected(table); if (!id.empty()) attempt([&] { duplicateBehaviour(staged, id); }); },
            [edit] { edit(false); }, [&, table] {
                const auto id = selected(table); if (id.empty()) return;
                const auto users = behaviourUsers(staged, id);
                if (const auto with = replacement(&dialog, others(d.behaviours, id), users.empty() ? "" : usersText(users, text), text))
                    attempt([&] { deleteBehaviour(staged, id, *with); });
            }};
    });
    page("editorBehaviourTabClasses", "editorVehicleClasses", {nullptr, [&] {
        std::vector<std::pair<std::string, std::string>> rows; for (const auto& c : d.vehicleClasses) rows.push_back({c.id, c.name}); return rows; }},
        [&](QTableWidget* table) {
        const auto edit = [&, table](bool create) {
            const auto id = selected(table); if (!create && id.empty()) return;
            auto value = create ? VehicleClass{nextId(d.vehicleClasses, "class-"), "", {}}
                                : *std::find_if(d.vehicleClasses.begin(), d.vehicleClasses.end(), [&](const auto& c) { return c.id == id; });
            if (editClass(&dialog, value, d, text)) attempt([&] { putVehicleClass(staged, value); });
        };
        return Actions{[edit] { edit(true); }, [&, table] { const auto id = selected(table); if (!id.empty()) attempt([&] { duplicateVehicleClass(staged, id); }); },
            [edit] { edit(false); }, [&, table] {
                const auto id = selected(table); if (id.empty()) return;
                const auto users = vehicleClassUsers(staged, id);
                if (const auto with = replacement(&dialog, others(d.vehicleClasses, id), users.empty() ? "" : usersText(users, text), text))
                    attempt([&] { deleteVehicleClass(staged, id, *with); });
            }};
    });
    page("editorBehaviourTabTypes", "editorBehaviourTypes", {nullptr, [&] {
        std::vector<std::pair<std::string, std::string>> rows; for (const auto& t : d.linkBehaviourTypes) rows.push_back({t.id, t.name}); return rows; }},
        [&](QTableWidget* table) {
        const auto edit = [&, table](bool create) {
            const auto id = selected(table); if (!create && id.empty()) return;
            auto value = create ? LinkBehaviourType{nextId(d.linkBehaviourTypes, "behaviour-type-"), "", d.behaviours.empty() ? std::string{} : d.behaviours.front().id, {}}
                                : *std::find_if(d.linkBehaviourTypes.begin(), d.linkBehaviourTypes.end(), [&](const auto& t) { return t.id == id; });
            if (editType(&dialog, value, d, create ? "" : usersText(behaviourTypeUsers(staged, id), text), text))
                attempt([&] { putLinkBehaviourType(staged, value); });
        };
        return Actions{[edit] { edit(true); }, [&, table] { const auto id = selected(table); if (!id.empty()) attempt([&] { duplicateLinkBehaviourType(staged, id); }); },
            [edit] { edit(false); }, [&, table] {
                const auto id = selected(table); if (id.empty()) return;
                const auto users = behaviourTypeUsers(staged, id);
                if (const auto with = replacement(&dialog, others(d.linkBehaviourTypes, id), users.empty() ? "" : usersText(users, text), text))
                    attempt([&] { deleteLinkBehaviourType(staged, id, *with); });
            }};
    });
    refresh(); layout->addWidget(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm")); buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        // Dry run: an invalid library keeps the dialog open with the reason, nothing committed.
        try { validateDocument(staged); dialog.accept(); }
        catch (const ValidationError& e) { error->setText(text(e.issues.front().code.c_str()) + " · " + q(e.issues.front().path)); }
        catch (const std::exception& e) { error->setText(text(e.what())); }
    });
    dialog.resize(680, 560);
    return dialog.exec() == QDialog::Accepted;
}
}
