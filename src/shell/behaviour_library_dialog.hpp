#pragma once
#include "demand_catalog_dialog.hpp"
#include "../project/document.hpp"
class QWidget;
namespace trafficsim {
// M3.3.2c (D128, DRIVING_BEHAVIOUR.md §9): the driving-behaviour library dialog. Every button
// edits `staged` through behaviour_commands; nothing reaches History here. True only when the
// author confirmed and `staged` validates -- the caller then commits it as one transaction.
// `staged` must already own its behaviour and vehicle-type catalogs.
bool editBehaviourLibrary(QWidget* parent, ProjectDocument& staged, const CatalogText& text);
}
