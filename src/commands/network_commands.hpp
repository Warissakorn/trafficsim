#pragma once
#include "history.hpp"

namespace trafficsim {
Link& editableLink(ProjectDocument& document, const std::string& id);
std::string addLink(ProjectDocument&, const std::vector<Point>& geometry, int lanes, double width);
void changeGeometry(ProjectDocument&, const std::string& id, const std::vector<Point>& geometry);
void changeLanes(ProjectDocument&, const std::string& id, const std::vector<double>& widths);
void deleteLink(ProjectDocument&, const std::string& id);
// Deletes links and connectors together in one transaction, with their lanes, attached
// connectors, signal heads, and the routes/inputs those segments carry. An id already removed
// as a side effect of an earlier deletion in the same call is skipped, not an error, so the
// result does not depend on the order the user happened to select things.
void deleteObjects(ProjectDocument&, const std::vector<std::string>& ids);
void changeDrivingSide(ProjectDocument&, DrivingSide side);
std::string oppositeLink(ProjectDocument&, const std::string& id, double gap);
// Splits at a centreline distance, reconnects original lanes, remaps routes/heads.
// Optional extra downstream lane forms a turn-pocket approach; it has no invented demand.
std::string splitLink(ProjectDocument&, const std::string& id, double distance, bool pocket = false);
}
