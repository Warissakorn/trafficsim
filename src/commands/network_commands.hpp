#pragma once
#include "history.hpp"

namespace trafficsim {
Link& editableLink(ProjectDocument& document, const std::string& id);
std::string addLink(ProjectDocument&, const std::vector<Point>& geometry, int lanes, double width);
void changeGeometry(ProjectDocument&, const std::string& id, const std::vector<Point>& geometry);
void changeLanes(ProjectDocument&, const std::string& id, const std::vector<double>& widths);
void deleteLink(ProjectDocument&, const std::string& id);
void changeDrivingSide(ProjectDocument&, DrivingSide side);
std::string oppositeLink(ProjectDocument&, const std::string& id, double gap);
// Splits at a centreline distance, reconnects original lanes, remaps routes/heads.
// Optional extra downstream lane forms a turn-pocket approach; it has no invented demand.
std::string splitLink(ProjectDocument&, const std::string& id, double distance, bool pocket = false);
}
