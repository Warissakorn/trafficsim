#include "editor_window.hpp"
#include "behaviour_library_dialog.hpp"
#include "../commands/behaviour_commands.hpp"
#include "../commands/demand_commands.hpp"
#include "../editor/canvas.hpp"
#include "../project/behaviour_library.hpp"
#include "../project/demand_catalog.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QStringList>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
namespace trafficsim {
// M3.3.2c (D128, DRIVING_BEHAVIOUR.md §9): the library dialog and the road assignment.
void EditorWindow::openBehaviourLibrary() {
    auto staged = history_.document();
    try {
        // The library needs project-owned behaviours and vehicle types. Capturing them is part of
        // the same transaction: Cancel leaves ownership exactly as it was, one Undo restores it.
        auto& def = demand(staged);
        if (def.externalBehaviours || def.externalVehicleTypes) {
            auto catalog = resolveDemandCatalog(def, data_);
            if (def.externalVehicleTypes) { def.vehicleTypes = catalog.vehicleTypes; def.vehicleTypeNames = catalog.vehicleTypeNames; }
            if (def.externalBehaviours) def.behaviours = catalog.behaviours;
            def.externalVehicleTypes = false; def.externalBehaviours = false;
        }
    } catch (const std::exception& e) { showError(e); return; }
    const auto captured = staged;
    const CatalogText tr = [this](const char* key) { return text(key); };
    // No library edit: no revision, and catalogs a project does not own yet stay external.
    if (!editBehaviourLibrary(this, staged, tr) || staged == captured) return;
    execute("editorBehaviourLibrary", [&](auto& d) { d.network = staged.network; d.definition = staged.definition; });
}
std::vector<std::string> EditorWindow::selectedRoads() const {
    std::vector<std::string> roads;
    const auto& n = history_.document().network;
    for (const auto& id : canvas_->selection())
        if (std::any_of(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; }) ||
            std::any_of(n.connectors.begin(), n.connectors.end(), [&](const auto& c) { return c.id == id; }))
            roads.push_back(id);
    return roads;
}
void EditorWindow::buildBehaviourInspector(QVBoxLayout* layout) {
    auto* form = new QFormLayout; form->setContentsMargins(0, 0, 0, 0); form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addLayout(form);
    behaviourType_ = new QComboBox(this); label(form, "editorBehaviourType", behaviourType_);
    // Every selected Link and Connector at once: the one property edit that acts on the selection.
    auto* apply = new QToolButton(this); apply->setToolButtonStyle(Qt::ToolButtonTextOnly);
    apply->setDefaultAction(action("editorApplyBehaviourType", {}, [this] {
        const auto roads = selectedRoads();
        const auto chosen = behaviourType_->currentData().toString().toStdString();
        const auto value = chosen.empty() ? std::optional<std::string>{} : std::optional<std::string>(chosen);
        execute("editorApplyBehaviourType", [&](auto& d) { for (const auto& id : roads) assignBehaviourType(d, id, value); });
    }));
    form->addRow(apply);
    effectiveBehaviour_ = new QLabel(this); effectiveBehaviour_->setObjectName("editorEffectiveBehaviour");
    effectiveBehaviour_->setWordWrap(true); form->addRow(effectiveBehaviour_);
}
void EditorWindow::refreshBehaviourInspector() {
    const auto& doc = history_.document();
    const auto roads = selectedRoads();
    const bool owned = doc.definition && !doc.definition->externalBehaviours && !doc.definition->externalVehicleTypes;
    std::optional<std::string> current;
    for (const auto& l : doc.network.links) if (!roads.empty() && l.id == roads.front()) current = l.behaviourTypeId;
    for (const auto& c : doc.network.connectors) if (!roads.empty() && c.id == roads.front()) current = c.behaviourTypeId;
    {
        const QSignalBlocker block(behaviourType_);
        behaviourType_->clear(); behaviourType_->addItem(text("editorBehaviourInherit"), QString());
        if (doc.definition) for (const auto& t : doc.definition->linkBehaviourTypes)
            behaviourType_->addItem(t.name.empty() ? QString::fromStdString(t.id)
                                                   : QString::fromStdString(t.name + " (" + t.id + ")"), QString::fromStdString(t.id));
        const auto value = QString::fromStdString(current.value_or(""));
        if (behaviourType_->findData(value) < 0) behaviourType_->addItem(value + " " + text("editorBehaviourMissing"), value);
        behaviourType_->setCurrentIndex(behaviourType_->findData(value));
    }
    const bool usable = owned && !roads.empty();
    behaviourType_->setEnabled(usable); actions_.at("editorApplyBehaviourType")->setEnabled(usable);
    if (roads.empty()) { effectiveBehaviour_->clear(); return; }
    if (!owned) { effectiveBehaviour_->setText(text("EDIT_EXTERNAL_CATALOG")); return; }
    // Effective vs inherited, by the same precedence the compiler uses.
    QStringList lines;
    try {
        for (const auto& r : effectiveRoadBehaviours(*doc.definition, current)) {
            const char* source = r.source == BehaviourSource::classOverride ? "editorBehaviourSourceOverride"
                               : r.source == BehaviourSource::typeDefault ? "editorBehaviourSourceDefault" : "editorBehaviourSourceInherited";
            lines << QString::fromStdString(r.vehicleTypeId + ": " + r.behaviourId) + " · " + text(source);
        }
    } catch (const std::exception& e) { lines << text(e.what()); }
    effectiveBehaviour_->setText(text("editorEffectiveBehaviour") + "\n" + lines.join("\n"));
}
}
