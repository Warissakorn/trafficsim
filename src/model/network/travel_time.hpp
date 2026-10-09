#pragma once
// M5.4 (D133, docs/reference/TRAVEL_TIME_SECTIONS.md): what an authored travel-time section means.
#include "right_of_way.hpp"

namespace trafficsim {
// Blank or duplicate ids, a line on a Link that does not exist, a station that is not finite and
// >= 0, a start not before its end on one Link. Paths are "travelTimeSections[i]...".
std::vector<ValidationIssue> travelTimeSectionIssues(const Network&);
// Where one line lies at run time: one place per lane of its Link that the station resolves on,
// in lane order. Empty when the Link is gone or the station lies past every lane's end.
std::vector<ControlLocation> locateSectionLine(const Network&, const RuntimeSections&, const SectionLine&);
}
