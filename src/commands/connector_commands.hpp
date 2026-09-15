#pragma once
#include "history.hpp"

namespace trafficsim {
std::string addConnectorRange(ProjectDocument&, const LaneReference&, const LaneReference&, int fromCount, int toCount);
void changeConnectorRange(ProjectDocument&, const std::string&, int fromCount, int toCount);
bool connectorReferenced(const ProjectDocument&, const Connector&);
// Run mutations through History::execute for validation, rollback and Undo/Redo.
Connector& editableConnector(ProjectDocument&, const std::string& id);
std::string addConnector(ProjectDocument&, const LaneReference& from, const LaneReference& to);
// Only interior points may change; the two endpoints are attached to their lanes.
void changeConnectorGeometry(ProjectDocument&, const std::string& id, const std::vector<Point>& geometry);
// Referenced connectors cannot be retargeted: doing so would change route topology.
void changeConnectorEndpoints(ProjectDocument&, const std::string& id, LaneReference from, LaneReference to);
void resetConnectorCurve(ProjectDocument&, const std::string& id, bool straight = false);
// Deletes affected routes and their inputs in the same undoable transaction.
void deleteConnector(ProjectDocument&, const std::string& id);
// Shared by Link/Lane/driving-side edits; preserves interior points by weighted displacement.
void reanchorConnectors(ProjectDocument&);
}
