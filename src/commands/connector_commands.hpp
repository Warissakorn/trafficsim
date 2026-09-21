#pragma once
#include "history.hpp"

namespace trafficsim {
std::string addConnectorRange(ProjectDocument&, const LaneReference&, const LaneReference&, int fromCount, int toCount);
void changeConnectorRange(ProjectDocument&, const std::string&, int fromCount, int toCount, bool leading = false);
// Vissim's Lanes tab: a width in metres for every lane path, and a MarkingType for every interior
// divider. Both empty restores the derived behaviour, which is what a Connector whose lanes were
// never given a width has always done. A partial list is rejected: no field would say which lanes
// were authored and which were derived.
void changeConnectorLanes(ProjectDocument&, const std::string& id, const std::vector<double>& widths,
                          const std::vector<MarkingType>& markings);
bool connectorReferenced(const ProjectDocument&, const Connector&);
// Run mutations through History::execute for validation, rollback and Undo/Redo.
Connector& editableConnector(ProjectDocument&, const std::string& id);
std::string addConnector(ProjectDocument&, const LaneReference& from, const LaneReference& to);
// Any point may move, endpoints included: a Connector keeps its own position, so its ends may be
// dragged off the Link, where the Connector is then deleted. What this may NOT do is name a
// different lane -- that is changeConnectorEndpoints, which guards route topology.
void changeConnectorGeometry(ProjectDocument&, const std::string& id, const std::vector<Point>& geometry);
// Referenced connectors cannot be retargeted: doing so would change route topology.
void changeConnectorEndpoints(ProjectDocument&, const std::string& id, LaneReference from, LaneReference to);
void resetConnectorCurve(ProjectDocument&, const std::string& id, bool straight = false);
// Vissim's Intermediate points field. Raising the count splits the longest leg each time, so no
// point the author placed is lost and the road keeps the shape it already had; lowering it
// re-spaces the points evenly along that same shape, giving up only the detail the lower count
// cannot hold. Either way the author's road is re-faired rather than replaced by the default
// curve. Re-laying at even spacing on the way UP was measured to cut a hand-placed corner by up
// to 1.00 m, which is why the two directions differ.
void resampleConnectorPoints(ProjectDocument&, const std::string& id, int count);
// Deletes affected routes and their inputs in the same undoable transaction.
void deleteConnector(ProjectDocument&, const std::string& id);
// For an edit that MOVES a Link. Each Connector keeps its own position: an end still on its Link
// re-reads which station it now sits at, and a Connector with an end off its Link is deleted,
// with the routes and heads that named it, in this same transaction.
void reanchorConnectors(ProjectDocument&);
// For an edit that RE-LAYS a Link's lanes without moving the road -- a lane added or removed, a
// width changed, the driving side flipped. Nothing moved out from under anything, so every
// Connector follows the lane it names to where that lane now is. Deleting a Connector because
// the author added a lane to the Link beside it would be a surprise, not a rule.
void anchorConnectors(ProjectDocument&);
}
