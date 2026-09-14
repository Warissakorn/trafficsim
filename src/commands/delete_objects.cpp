#include "network_commands.hpp"
#include "connector_commands.hpp"
#include <algorithm>

namespace trafficsim {
namespace {
bool isLink(const ProjectDocument& d, const std::string& id) {
    return std::any_of(d.network.links.begin(), d.network.links.end(), [&](const auto& l) { return l.id == id; });
}
bool isConnector(const ProjectDocument& d, const std::string& id) {
    return std::any_of(d.network.connectors.begin(), d.network.connectors.end(), [&](const auto& c) { return c.id == id; });
}
}
void deleteObjects(ProjectDocument& d, const std::vector<std::string>& ids) {
    // Reject the whole selection before touching anything, so the reported code names the object
    // the user actually chose rather than whatever a partial cascade left behind.
    for (const auto& id : ids)
        if (!isLink(d, id) && !isConnector(d, id)) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    // Links first: deleting one already removes its own connectors, so a connector named in the
    // same selection can legitimately be gone by the time its turn comes.
    for (const auto& id : ids) if (isLink(d, id)) deleteLink(d, id);
    for (const auto& id : ids) if (isConnector(d, id)) deleteConnector(d, id);
}
}
