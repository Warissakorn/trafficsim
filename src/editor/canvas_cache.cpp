#include "canvas.hpp"
namespace trafficsim {
namespace {
// A Connector whose Link is gone still has to compare equal to itself from one frame to the
// next, so a missing Link is a value like any other rather than a special case.
const Link kNoLink{};
const Link& linkOf(const Network& network,const std::string& id) {
    for(const auto& link:network.links)if(link.id==id)return link;
    return kNoLink;
}
}
EditorCanvas::CachedConnector& EditorCanvas::connectorEntry(const Connector& c) const {
    const auto& network=document_->network;
    const Link& from=linkOf(network,c.from.linkId);
    const Link& to=linkOf(network,c.to.linkId);
    auto& entry=connectorCache_[c.id];
    if(entry.connector==c && entry.side==network.drivingSide && entry.from==from && entry.to==to)
        return entry;
    entry={c,from,to,network.drivingSide,{},{}};
    return entry;
}
// Deleted Connectors would otherwise keep their entries for the life of the window. This is the
// ONLY place an entry is erased, and redraw() calls it before it draws anything: callers hold
// references into the map across several cached calls, and an erase in the middle of a frame
// would dangle one. std::map nodes are stable, so inserting during a frame is safe.
void EditorCanvas::pruneConnectorCache() {
    if(!document_)  { connectorCache_.clear(); return; }
    std::erase_if(connectorCache_,[&](const auto& entry) {
        for(const auto& c:document_->network.connectors)if(c.id==entry.first)return false;
        return true;
    });
}
const std::vector<ConnectorPath>& EditorCanvas::cachedPaths(const Connector& c) const {
    auto& entry=connectorEntry(c);
    // connectorPaths throws on a range that no longer fits its Link, and every caller already
    // handles that. Filling the optional only on success keeps a failed frame from caching an
    // answer, and leaves the next frame to throw identically.
    if(!entry.paths)entry.paths=connectorPaths(document_->network,c);
    return *entry.paths;
}
const std::vector<std::vector<Point>>& EditorCanvas::cachedBoundaries(const Connector& c) const {
    auto& entry=connectorEntry(c);
    if(!entry.boundaries)entry.boundaries=connectorBoundaries(document_->network,c);
    return *entry.boundaries;
}
}
