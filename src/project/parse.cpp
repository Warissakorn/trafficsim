#include "json.hpp"
#include <stdexcept>

namespace trafficsim {
bool present(const Json& value, const char* name) {
    return value.is_object() && value.contains(name) && !value.at(name).is_null();
}
const Json& section(const Json& value, const char* name) {
    if (!present(value, name)) throw std::invalid_argument(std::string("Missing section: ") + name);
    return value.at(name);
}
namespace {
// Every accessor goes through this: reaching .at() on a null or a non-object is how an
// nlohmann type_error escapes to the user instead of a sentence naming the missing field.
const Json& member(const Json& value, const char* name) {
    if (!value.is_object()) throw std::invalid_argument(std::string("Expected an object containing: ") + name);
    if (!value.contains(name)) throw std::invalid_argument(std::string("Missing field: ") + name);
    if (value.at(name).is_null()) throw std::invalid_argument(std::string("Field is null: ") + name);
    return value.at(name);
}
template<class T> T field(const Json& value, const char* name) {
    const auto& item = member(value, name);
    if constexpr (std::is_same_v<T, double>) {
        if (!item.is_number()) throw std::invalid_argument(std::string("Expected number: ") + name);
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (!item.is_string()) throw std::invalid_argument(std::string("Expected text: ") + name);
    }
    return item.get<T>();
}
const Json& array(const Json& value, const char* name) {
    const auto& items = member(value, name);
    if (!items.is_array()) throw std::invalid_argument(std::string("Expected array: ") + name);
    return items;
}
std::vector<std::string> strings(const Json& value, const char* name) {
    std::vector<std::string> result;
    for (const auto& item : array(value, name)) {
        if (!item.is_string()) throw std::invalid_argument(std::string("Expected strings: ") + name);
        result.push_back(item.get<std::string>());
    }
    return result;
}
int integer(const Json& value,const char* key,int fallback) {
    if(!value.contains(key))return fallback;
    const auto& v=member(value,key);
    if(!v.is_number_integer() || v < -1000 || v > 1000)throw std::invalid_argument("EDIT_DISPLAY_VALUE");
    return v.get<int>();
}
std::vector<Point> points(const Json& value) {
    std::vector<Point> result;
    for (const auto& p : array(value, "geometry")) result.push_back({field<double>(p, "x"), field<double>(p, "y")});
    return result;
}
LaneReference reference(const Json& value,int schemaVersion) {
    LaneReference result{field<std::string>(value,"linkId"),field<std::string>(value,"laneId")};
    const char* key=schemaVersion>=5?"station":"fraction";
    const char* wrong=schemaVersion>=5?"fraction":"station";
    if(value.contains(wrong))throw std::invalid_argument("EDIT_VERSION");
    if(value.contains(key))result.station=field<double>(value,key);
    return result;
}
// Below schema 5 the stored number is a fraction of the lane's own arclength. Convert it to a
// station on the link once every link is parsed, so the attachment keeps the world position it
// was drawn at. Signal heads share LaneReference, so they are walked too.
void migrateAttachments(Network& network) {
    const auto convert=[&](LaneReference& ref,bool outgoing) {
        if(!ref.station)return;
        for(const auto& link:network.links)if(link.id==ref.linkId) {
            const auto lane=laneGeometry(link,ref.laneId,network.drivingSide);
            const double station=matchedStation(lane,link.geometry,*ref.station*polylineLength(lane));
            const double reference=polylineLength(link.geometry);
            // An attachment that was exactly at the end keeps meaning "the end" as the link changes.
            if(station<=0 && !outgoing)ref.station.reset();
            else if(station>=reference && outgoing)ref.station.reset();
            else ref.station=std::clamp(station,0.,reference);
            return;
        }
    };
    for(auto& c:network.connectors){convert(c.from,true);convert(c.to,false);}
    for(auto& h:network.signalHeads)convert(h.lane,false);
}
SignalColor color(const std::string& text) {
    if (text == "red") return SignalColor::red;
    if (text == "amber") return SignalColor::amber;
    if (text == "green") return SignalColor::green;
    throw std::invalid_argument("INVALID_SIGNAL_COLOR");
}
}
Network parseNetwork(const Json& value, int schemaVersion) {
    Network network;
    network.id = field<std::string>(value, "id");
    const auto side = field<std::string>(value, "drivingSide");
    if (side != "left" && side != "right") throw std::invalid_argument("INVALID_DRIVING_SIDE");
    network.drivingSide = side == "left" ? DrivingSide::left : DrivingSide::right;
    for (const auto& item : array(value, "links")) {
        Link link{field<std::string>(item, "id"), points(item), {}};
        for (const auto& lane : array(item, "lanes"))
            link.lanes.push_back({field<std::string>(lane, "id"), field<double>(lane, "width")});
        link.level=integer(item,"level",0);
        if(item.contains("laneOffset"))link.laneOffset=field<double>(item,"laneOffset");
        if(item.contains("displayType"))link.displayType=field<std::string>(item,"displayType");
        if(present(item,"name"))link.name=field<std::string>(item,"name");
        network.links.push_back(std::move(link));
    }
    for (const auto& c : array(value, "connectors")) {
        network.connectors.push_back({field<std::string>(c, "id"), reference(member(c, "from"),schemaVersion), reference(member(c, "to"),schemaVersion), points(c),
            integer(c,"fromLaneCount",1),integer(c,"toLaneCount",1),integer(c,"level",0),
            c.contains("displayType")?field<std::string>(c,"displayType"):"default"});
        if(present(c,"name"))network.connectors.back().name=field<std::string>(c,"name");
        if(c.contains("laneBlend"))for(const auto& t:array(c,"laneBlend")) {
            if(!t.is_number())throw std::invalid_argument("INVALID_GEOMETRY");
            network.connectors.back().laneBlend.push_back(t.get<double>());
        }
    }
    for (const auto& h : array(value, "signalHeads")) {
        network.signalHeads.push_back({field<std::string>(h, "id"), reference(member(h, "lane"),schemaVersion),
                                      field<double>(h, "position"), field<std::string>(h, "programId"),
                                      present(h,"connectorId")?field<std::string>(h,"connectorId"):std::string{}});
        if(present(h,"name"))network.signalHeads.back().name=field<std::string>(h,"name");
    }
    if(schemaVersion<5)migrateAttachments(network);
    return network;
}
PriorityDefaults parsePriorityDefaults(const Json& value) {
    return {field<double>(value, "gapTime"), field<double>(value, "headway")};
}
DriverBehaviour parseBehaviour(const Json& b) {
    return {field<std::string>(b, "id"), field<double>(b, "standstillDistance"),
        field<double>(b, "additiveSafetyDistance"), field<double>(b, "multiplicativeSafetyDistance"),
        field<double>(b, "followingTime"), field<double>(b, "speedThreshold")};
}
VehicleType parseVehicleType(const Json& t) {
    const auto& range = member(t, "desiredSpeed");
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
