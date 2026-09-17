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
// Only interior points may change; the two endpoints are attached to their lanes.
void changeConnectorGeometry(ProjectDocument&, const std::string& id, const std::vector<Point>& geometry);
// Referenced connectors cannot be retargeted: doing so would change route topology.
void changeConnectorEndpoints(ProjectDocument&, const std::string& id, LaneReference from, LaneReference to);
void resetConnectorCurve(ProjectDocument&, const std::string& id, bool straight = false);
// Vissim's Intermediate points field. The Connector's current road is re-laid with `count`
// interior points at equal spacing along it, so raising or lowering the count re-fairs the shape
// the author already has rather than throwing it away for the default curve.
void resampleConnectorPoints(ProjectDocument&, const std::string& id, int count);
// Deletes affected routes and their inputs in the same undoable transaction.
void deleteConnector(ProjectDocument&, const std::string& id);
// Shared by Link/Lane/driving-side edits. Moves the one poly point attached to each Link, the
// way Vissim does, so reshaping depends only on where the endpoints are and never on the path
// taken to get there.
void reanchorConnectors(ProjectDocument&);
}
