#include "json.hpp"
#include <stdexcept>

namespace trafficsim {
namespace {
template<class T> T field(const Json& value, const char* name) {
    const auto& item = value.at(name);
    if constexpr (std::is_same_v<T, double>) {
        if (!item.is_number()) throw std::invalid_argument(std::string("Expected number: ") + name);
    }
    return item.get<T>();
}
const Json& array(const Json& value, const char* name) {
    const auto& items = value.at(name);
    if (!items.is_array()) throw std::invalid_argument(std::string("Expected array: ") + name);
    return items;
}
std::vector<std::string> strings(const Json& value, const char* name) {
    return array(value, name).get<std::vector<std::string>>();
}
std::vector<Point> points(const Json& value) {
    std::vector<Point> result;
    for (const auto& p : array(value, "geometry")) result.push_back({field<double>(p, "x"), field<double>(p, "y")});
    return result;
}
LaneReference reference(const Json& value) {
    return {field<std::string>(value, "linkId"), field<std::string>(value, "laneId")};
}
SignalColor color(const std::string& text) {
    if (text == "red") return SignalColor::red;
    if (text == "amber") return SignalColor::amber;
    if (text == "green") return SignalColor::green;
    throw std::invalid_argument("INVALID_SIGNAL_COLOR");
}
}
Network parseNetwork(const Json& value) {
    Network network;
    network.id = field<std::string>(value, "id");
    const auto side = field<std::string>(value, "drivingSide");
    if (side != "left" && side != "right") throw std::invalid_argument("INVALID_DRIVING_SIDE");
    network.drivingSide = side == "left" ? DrivingSide::left : DrivingSide::right;
    for (const auto& item : array(value, "links")) {
        Link link{field<std::string>(item, "id"), points(item), {}};
        for (const auto& lane : array(item, "lanes"))
            link.lanes.push_back({field<std::string>(lane, "id"), field<double>(lane, "width")});
        network.links.push_back(std::move(link));
    }
    for (const auto& c : array(value, "connectors"))
        network.connectors.push_back({field<std::string>(c, "id"), reference(c.at("from")), reference(c.at("to")), points(c)});
    for (const auto& h : array(value, "signalHeads"))
        network.signalHeads.push_back({field<std::string>(h, "id"), reference(h.at("lane")),
                                      field<double>(h, "position"), field<std::string>(h, "programId")});
    return network;
}
DriverBehaviour parseBehaviour(const Json& b) {
    return {field<std::string>(b, "id"), field<double>(b, "standstillDistance"),
        field<double>(b, "additiveSafetyDistance"), field<double>(b, "multiplicativeSafetyDistance"),
        field<double>(b, "followingTime"), field<double>(b, "speedThreshold")};
}
VehicleType parseVehicleType(const Json& t) {
    const auto& range = t.at("desiredSpeed");
    return {field<std::string>(t, "id"), field<double>(t, "length"), field<double>(t, "width"),
        {field<double>(range, "min"), field<double>(range, "max")}, field<double>(t, "maxAcceleration"),
        field<double>(t, "comfortableDeceleration"), field<double>(t, "maxDeceleration"), field<std::string>(t, "behaviourId")};
}
ScenarioDefinition parseDefinition(const Json& value) {
    ScenarioDefinition definition;
    definition.duration = field<double>(value, "duration");
    definition.timeStep = field<double>(value, "timeStep");
    for (const auto& r : array(value, "routes"))
        definition.routes.push_back({field<std::string>(r, "id"), strings(r, "segmentIds")});
    for (const auto& i : array(value, "inputs"))
        definition.inputs.push_back({field<std::string>(i, "id"), field<std::string>(i, "routeId"),
            field<std::string>(i, "vehicleTypeId"), field<double>(i, "vehiclesPerHour"),
            field<double>(i, "startTime"), field<double>(i, "endTime")});
    for (const auto& p : array(value, "signalPrograms")) {
        SignalProgram program{field<std::string>(p, "id"), field<double>(p, "offset"), {}};
        for (const auto& phase : array(p, "phases"))
            program.phases.push_back({field<double>(phase, "duration"), color(field<std::string>(phase, "color"))});
        definition.signalPrograms.push_back(std::move(program));
    }
    if (value.contains("vehicleTypes"))
        for (const auto& t : array(value, "vehicleTypes")) definition.vehicleTypes.push_back(parseVehicleType(t));
    if (value.contains("behaviours"))
        for (const auto& b : array(value, "behaviours")) definition.behaviours.push_back(parseBehaviour(b));
    return definition;
}
}
