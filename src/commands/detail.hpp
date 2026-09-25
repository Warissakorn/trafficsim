#pragma once
#include "../project/document.hpp"
#include <map>
#include <set>

namespace trafficsim::detail {
void removeRoutesUsingSegments(ProjectDocument&, const std::set<std::string>& segments);
// M2.4. After any route removal: drop decision entries for missing routes, then decisions left
// empty and the inputs that named them. The one place both removal paths share.
// M2.1.1: also drops destination entries, placed decisions and routeless inputs whose Link is gone.
void pruneRoutingDecisions(AuthoringDefinition&, const Network&);
// M3.2.2b: authored right-of-way controls follow the objects they name, the way signal heads do
// (right_of_way_commands.cpp). Every one runs inside the caller's command, so it is one step.
//
// Deletes every area with a side, or a side's waiting line, on a removed Link or Connector, the
// areas' rules, the waiting lines on removed owners, and lines that served only those areas.
void removeControlsOn(ProjectDocument&, const std::set<std::string>& links, const std::set<std::string>& connectors);
// M3.2.5: drops the area ids that no longer exist from every Stop/Yield control, then any control
// left with no area or no line -- a control means nothing without both.
void pruneStopControls(RightOfWay&);
// True when a control names this Link: reversing it would turn every station around.
bool controlsNameLink(const Network&, const std::string& link);
// After splitLink cut `link` at `distance` and moved Connector ends: stations at or past the far
// side of the 0.2 m span move to `downstream` (lane ids through `lanes`), exactly as Connector
// ends do; a Connector lane pair whose end moved takes the downstream lane id. Throws
// EDIT_SPLIT_CONTROL, before changing anything, when a waiting line or a side lies in or across
// the span -- the first slice rejects rather than guess (M3_CONTRACT.md §1).
void checkSplitControls(const Network&, const std::string& link, double distance);
void splitControls(ProjectDocument&, const std::string& link, const std::string& downstream, double distance,
                   const std::map<std::string, std::string>& lanes);
// duplicateObjects: copies each waiting line, area and rule whose every owner was copied, with
// fresh ids and its references remapped once. A Connector lane pair needs both end Links copied,
// so its lane ids map. Anything else stays behind, as an unselected head does.
void copyControls(ProjectDocument&, const Network& source, const std::map<std::string, std::string>& links,
                  const std::map<std::string, std::string>& lanes, const std::map<std::string, std::string>& connectors);
}
