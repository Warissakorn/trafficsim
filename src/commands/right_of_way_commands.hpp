#pragma once
// M3.2.2: edits to the authored right-of-way controls. Each is a plain document change; the caller
// runs it inside History::execute, which validates the whole document and makes it one undoable
// step. A failed edit therefore changes nothing (docs/M3_CONTRACT.md §6).
#include "../project/document.hpp"

namespace trafficsim {
// Empty id allocates; the same id replaces. Structural checks run when the edit commits.
std::string putWaitingLine(ProjectDocument&, WaitingLine);
std::string putConflictArea(ProjectDocument&, ConflictArea);
std::string putPriorityRule(ProjectDocument&, AuthoredPriorityRule);
// Refused (EDIT_REFERENCED) while a conflict side still waits at it.
void deleteWaitingLine(ProjectDocument&, const std::string& id);
// Removes the area's rule with it: a rule means nothing without its area.
void deleteConflictArea(ProjectDocument&, const std::string& id);
// Leaves the area -- and so its merge group -- a Run-blocked draft. Deleting a rule never hands
// the group back to the drawing-order fallback behind the author's back (§3, policy 4).
void deletePriorityRule(ProjectDocument&, const std::string& id);

// Takes one merge over from the drawing-order fallback: one area, waiting line and rule per pair
// of its incoming paths, prefilled with the fallback's order and `defaults`' two numbers, so
// the result behaves exactly as the fallback did until the author changes it. `section` is a
// MergeGroup::section. Throws EDIT_NO_MERGE for a section no merge arrives on, EDIT_ALREADY_EXPLICIT
// when the group is already authored, EDIT_NO_PRIORITY_DEFAULTS when the numbers are unusable.
// Returns the area ids created.
std::vector<std::string> takeOverMerge(ProjectDocument&, const std::string& section, const PriorityDefaults&);
// The separate, named action that hands a merge back: removes every area covering two of its
// incoming paths, their rules, and the waiting lines nothing else uses.
void restoreAutomaticPriority(ProjectDocument&, const std::string& section);
}
