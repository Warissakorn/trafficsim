#pragma once
#include "demand_catalog_dialog.hpp"
class QWidget;
namespace trafficsim {
// One behaviour's editor (D128, D139): its name, its model (prototype or w74) and that model's
// keys. `library` supplies prototype values when a w74 behaviour is switched back; a w74 page
// opened without values starts empty, because no key has a code default (hard rule 5).
// True when the author confirmed; `b` and `name` then hold the edit.
bool editBehaviour(QWidget* parent, DriverBehaviour& b, std::string& name, const std::string& users,
                   const std::vector<DriverBehaviour>& library, const CatalogText& text);
}
