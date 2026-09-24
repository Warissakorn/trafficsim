#include "document.hpp"
#include "../core/validate.hpp"
#include "../model/network/diagnostics.hpp"
#include <cmath>
#include <set>
#include <limits>

namespace trafficsim {
namespace {
// Written as names, not as the enum's integers: a project file a human opens should say `dashed`,
// and an added marking kind must not silently renumber what older files meant.
Json markingNames(const std::vector<MarkingType>& markings) {
    Json result=Json::array();
    for(const auto m:markings)result.push_back(markingName(m));
    return result;
}
}

namespace {
Json points(const std::vector<Point>& ps) {
    Json out = Json::array();
    for (const auto& p : ps) out.push_back({{"x", p.x}, {"y", p.y}});
    return out;
}
Json reference(const LaneReference& r) {
    Json result={{"linkId",r.linkId},{"laneId",r.laneId}};
    if(r.station)result["station"]=*r.station;
    return result;
}
}
Json documentJson(const ProjectDocument& d) {
    Json network = {{"id", d.network.id}, {"drivingSide", d.network.drivingSide == DrivingSide::left ? "left" : "right"},
        {"links", Json::array()}, {"connectors", Json::array()}, {"signalHeads", Json::array()}};
    for (const auto& l : d.network.links) {
        Json lanes = Json::array();
        for (const auto& lane : l.lanes) lanes.push_back({{"id", lane.id}, {"width", lane.width}});
        network["links"].push_back({{"id", l.id}, {"geometry", points(l.geometry)}, {"lanes", lanes}, {"level",l.level}, {"displayType",l.displayType}, {"laneOffset",l.laneOffset}, {"name",l.name},
            {"boundaryMarkings",markingNames(l.boundaryMarkings)}});
    }
    for (const auto& c : d.network.connectors)
        network["connectors"].push_back({{"id", c.id}, {"from", reference(c.from)}, {"to", reference(c.to)}, {"geometry", points(c.geometry)}, {"fromLaneCount",c.fromLaneCount}, {"toLaneCount",c.toLaneCount},
            {"level",c.level}, {"displayType",c.displayType}, {"laneBlend",c.laneBlend}, {"name",c.name},
            {"laneWidths",c.laneWidths}, {"laneMarkings",markingNames(c.laneMarkings)}});
    for (const auto& h : d.network.signalHeads)
        network["signalHeads"].push_back({{"id", h.id}, {"lane", reference(h.lane)}, {"position", h.position}, {"programId", h.programId}, {"connectorId",h.connectorId}, {"name",h.name}});
    const auto& b = d.background;
    return {{"format", "TrafficSim"}, {"schemaVersion", 12}, {"nextId", d.nextId}, {"revision", d.revision}, {"network", network},
        {"definition", d.definition ? definitionJson(*d.definition) : Json(nullptr)}, {"background", {{"pngBase64", *b.pngBase64}, {"x", b.x}, {"y", b.y},
            {"metresPerPixel", b.metresPerPixel}, {"rotation", b.rotation}, {"opacity", b.opacity}}}};
}
void validateDocument(const ProjectDocument& d) {
    auto issues = validateNetwork(d.network);
    // blocksDraft owns the "an empty network is a legal draft" rule, for this and the editor alike.
    std::erase_if(issues, [](const auto& i) { return !blocksDraft(i.code); });
    if (!issues.empty()) throw ValidationError(issues);
    if (!d.nextId || d.nextId == std::numeric_limits<std::uint64_t>::max() || d.revision == std::numeric_limits<std::uint64_t>::max()) throw std::invalid_argument("EDIT_ID_LIMIT");
    const auto& b = d.background;
    if (!b.pngBase64 || !std::isfinite(b.x) || !std::isfinite(b.y) || !std::isfinite(b.rotation) ||
        !std::isfinite(b.metresPerPixel) || b.metresPerPixel <= 0 || !std::isfinite(b.opacity) ||
        b.opacity < 0 || b.opacity > 1 || b.pngBase64->size() > 32 * 1024 * 1024)
        throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
    validateAuthoredDemand(d);
}
ProjectDocument parseDocument(const Json& j) {
    ProjectDocument d;
    if (!j.is_object()) throw std::invalid_argument("EDIT_VERSION");
    if (j.contains("schemaVersion")) {
        // Every read here is guarded: a hand-edited null section must name itself, not surface
        // as an nlohmann type_error the user cannot act on.
        if (!present(j, "schemaVersion") || !j.at("schemaVersion").is_number_integer() || (j.at("schemaVersion") < 1 || j.at("schemaVersion") > 12) ||
            !present(j, "format") || j.at("format") != "TrafficSim")
            throw std::invalid_argument("EDIT_VERSION");
        if (!present(j, "nextId") || !j.at("nextId").is_number_unsigned() ||
            !present(j, "revision") || !j.at("revision").is_number_unsigned()) throw std::invalid_argument("EDIT_ID_LIMIT");
        d.nextId = j.at("nextId").get<std::uint64_t>();
        d.revision = j.at("revision").get<std::uint64_t>();
        if (!present(j, "background")) throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
        const auto& b = j.at("background");
        if (!b.is_object() || !b.contains("pngBase64") || !b.at("pngBase64").is_string()) throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
        for (const char* number : {"x", "y", "metresPerPixel", "rotation", "opacity"})
            if (!present(b, number) || !b.at(number).is_number()) throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
        d.background = {std::make_shared<const std::string>(b.at("pngBase64").get<std::string>()), b.at("x").get<double>(), b.at("y").get<double>(),
            b.at("metresPerPixel").get<double>(), b.at("rotation").get<double>(), b.at("opacity").get<double>()};
    }
    if (!present(j, "network")) throw std::invalid_argument("EDIT_NO_NETWORK");
    d.network = parseNetwork(j.at("network"), j.contains("schemaVersion") ? j.at("schemaVersion").get<int>() : 0);
    if (present(j, "definition")) {
        d.definition = parseAuthoringDefinition(j.at("definition"));
        // Schema 7 and earlier stored a route as lanes and Connector paths. Schema 8 stores the
        // Links and Connectors those belong to, so that narrowing a Connector cannot invalidate
        // a route. The mapping is idempotent, which is what lets it run on every read.
        migrateRoutesToObjects(d.network, *d.definition);
    }
    validateDocument(d);
    return d;
}
std::string allocateId(ProjectDocument& d, const std::string& prefix) {
    std::set<std::string> used{d.network.id};
    for (const auto& l : d.network.links) { used.insert(l.id); for (const auto& lane : l.lanes) used.insert(lane.id); }
    for (const auto& c : d.network.connectors)
        for(int i=0;i<std::max(c.fromLaneCount,c.toLaneCount);++i)used.insert(connectorPathId(c,i));
    for (const auto& h : d.network.signalHeads) used.insert(h.id);
    if (d.definition) {
        for (const auto& r : d.definition->routes) used.insert(r.id);
        for (const auto& i : d.definition->inputs) used.insert(i.id);
        for (const auto& p : d.definition->signalPrograms) used.insert(p.id);
    }
    for (;;) {
        if (d.nextId >= std::numeric_limits<std::uint64_t>::max() - 1) throw std::invalid_argument("EDIT_ID_LIMIT");
        auto id = prefix + "-" + std::to_string(d.nextId++);
        if (!used.contains(id)) return id;
    }
}
}
