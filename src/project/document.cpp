#include "document.hpp"
#include "../core/validate.hpp"
#include <cmath>
#include <set>
#include <limits>

namespace trafficsim {
namespace {
Json points(const std::vector<Point>& ps) {
    Json out = Json::array();
    for (const auto& p : ps) out.push_back({{"x", p.x}, {"y", p.y}});
    return out;
}
Json reference(const LaneReference& r) { return {{"linkId", r.linkId}, {"laneId", r.laneId}}; }
}
Json documentJson(const ProjectDocument& d) {
    Json network = {{"id", d.network.id}, {"drivingSide", d.network.drivingSide == DrivingSide::left ? "left" : "right"},
        {"links", Json::array()}, {"connectors", Json::array()}, {"signalHeads", Json::array()}};
    for (const auto& l : d.network.links) {
        Json lanes = Json::array();
        for (const auto& lane : l.lanes) lanes.push_back({{"id", lane.id}, {"width", lane.width}});
        network["links"].push_back({{"id", l.id}, {"geometry", points(l.geometry)}, {"lanes", lanes}});
    }
    for (const auto& c : d.network.connectors)
        network["connectors"].push_back({{"id", c.id}, {"from", reference(c.from)}, {"to", reference(c.to)}, {"geometry", points(c.geometry)}});
    for (const auto& h : d.network.signalHeads)
        network["signalHeads"].push_back({{"id", h.id}, {"lane", reference(h.lane)}, {"position", h.position}, {"programId", h.programId}});
    const auto& b = d.background;
    return {{"format", "TrafficSim"}, {"schemaVersion", 1}, {"nextId", d.nextId}, {"revision", d.revision}, {"network", network},
        {"definition", d.definition}, {"background", {{"pngBase64", *b.pngBase64}, {"x", b.x}, {"y", b.y},
            {"metresPerPixel", b.metresPerPixel}, {"rotation", b.rotation}, {"opacity", b.opacity}}}};
}
void validateDocument(const ProjectDocument& d) {
    auto issues = validateNetwork(d.network);
    std::erase_if(issues, [](const auto& i) { return i.code == "EMPTY_NETWORK"; });
    if (!issues.empty()) throw ValidationError(issues);
    if (!d.nextId || d.nextId == std::numeric_limits<std::uint64_t>::max() || d.revision == std::numeric_limits<std::uint64_t>::max()) throw std::invalid_argument("EDIT_ID_LIMIT");
    const auto& b = d.background;
    if (!b.pngBase64 || !std::isfinite(b.x) || !std::isfinite(b.y) || !std::isfinite(b.rotation) ||
        !std::isfinite(b.metresPerPixel) || b.metresPerPixel <= 0 || !std::isfinite(b.opacity) ||
        b.opacity < 0 || b.opacity > 1 || b.pngBase64->size() > 32 * 1024 * 1024)
        throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
    if (!d.definition.is_null()) (void)parseDefinition(d.definition);
}
ProjectDocument parseDocument(const Json& j) {
    ProjectDocument d;
    if (j.contains("schemaVersion")) {
        if (!j.at("schemaVersion").is_number_integer() || j.at("schemaVersion") != 1 || j.at("format") != "TrafficSim")
            throw std::invalid_argument("EDIT_VERSION");
        if (!j.at("nextId").is_number_unsigned() || !j.at("revision").is_number_unsigned()) throw std::invalid_argument("EDIT_ID_LIMIT");
        d.nextId = j.at("nextId").get<std::uint64_t>();
        d.revision = j.at("revision").get<std::uint64_t>();
        const auto& b = j.at("background");
        d.background = {std::make_shared<const std::string>(b.at("pngBase64").get<std::string>()), b.at("x").get<double>(), b.at("y").get<double>(),
            b.at("metresPerPixel").get<double>(), b.at("rotation").get<double>(), b.at("opacity").get<double>()};
    }
    d.network = parseNetwork(j.at("network"));
    d.definition = j.value("definition", Json(nullptr));
    validateDocument(d);
    return d;
}
std::string allocateId(ProjectDocument& d, const std::string& prefix) {
    std::set<std::string> used{d.network.id};
    for (const auto& l : d.network.links) { used.insert(l.id); for (const auto& lane : l.lanes) used.insert(lane.id); }
    for (const auto& c : d.network.connectors) used.insert(c.id);
    for (const auto& h : d.network.signalHeads) used.insert(h.id);
    for (;;) {
        if (d.nextId >= std::numeric_limits<std::uint64_t>::max() - 1) throw std::invalid_argument("EDIT_ID_LIMIT");
        auto id = prefix + "-" + std::to_string(d.nextId++);
        if (!used.contains(id)) return id;
    }
}
}
