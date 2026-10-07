#include "test.hpp"
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/routes.hpp"
#include "../src/core/validate.hpp"
#include "../src/project/behaviour_library.hpp"
#include "../src/project/run.hpp"
#include <fstream>
#include <map>
using namespace trafficsim;
// M3.3.2b (D127): compiled road assignments and one effective behaviour per vehicle (BA10-BA16).
namespace {
const std::string kDefault = "wiedemann-inspired-prototype";
DriverBehaviour behaviour(const std::string& id, double standstill = 2) { return {id, standstill, 2, 3, 1.5, .2}; }
// a (10 m) -> b (10 m); the legacy behaviour everywhere, `slow` on b when assigned.
Scenario joined(bool assigned) {
    Scenario s; s.duration = 30; s.timeStep = .1;
    s.segments = {{"b", 10, {}}, {"a", 10, {"b"}}}; // declared out of order: canonical order must not matter
    s.routes = {{"r", {"a", "b"}}};
    s.behaviours = {behaviour("slow", 5), behaviour("legacy")};
    s.vehicleTypes = {{"car", 4, 2, {10, 10}, 2, 3, 6, "legacy"}, {"bus", 10, 2.5, {8, 8}, 1, 2, 5, "legacy"}};
    if (assigned) s.segmentBehaviours = {{"b", "car", "slow"}};
    return s;
}
std::size_t slot(const std::vector<DriverBehaviour>& items, const std::string& id) {
    for (std::size_t i = 0; i < items.size(); ++i) if (items[i].id == id) return i;
    throw std::runtime_error("Missing fixture behaviour");
}
Vehicle at(const SimState& state, const std::string& type, double distance) {
    Vehicle v; v.id = 1; v.routeIndex = 0; v.distance = distance;
    for (std::uint32_t i = 0; i < state.scenario->vehicleTypes.size(); ++i)
        if (state.scenario->vehicleTypes[i].id == type) v.typeIndex = i;
    return v;
}
// Two Links joined by a Connector, owned catalogs, a heavy class and two road behaviour types.
ProjectDocument project() {
    ProjectDocument d; const auto a = addLink(d, {{0, 0}, {200, 0}}, 2, 3.5), b = addLink(d, {{220, 0}, {400, 0}}, 2, 3.5);
    const auto& A = d.network.links[0]; const auto& B = d.network.links[1];
    addConnectorRange(d, {a, A.lanes[0].id, {}}, {b, B.lanes[0].id, {}}, 2, 2);
    const auto c = addLink(d, {{100, -40}, {100, -200}}, 1, 3.5); // fed from the middle of a: a section cut
    addConnector(d, {a, d.network.links[0].lanes[0].id, 100.0}, {c, d.network.links[2].lanes[0].id, {}});
    const auto route = putRoute(d, {"route", {a, d.network.connectors[0].id, b}}); changeRunSettings(d, 60, .1);
    putDemandCatalog(d, resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data"));
    putInput(d, {"in", route, "car", 600, 0, 60});
    putBehaviour(d, behaviour("slow", 4), "Slow"); putBehaviour(d, behaviour("rural", 3), "");
    putVehicleClass(d, {"heavy", "Heavy", {"heavy-vehicle"}});
    putLinkBehaviourType(d, {"urban", "", kDefault, {{"heavy", "slow"}}});
    putLinkBehaviourType(d, {"open", "", "rural", {}});
    assignBehaviourType(d, a, "urban"); assignBehaviourType(d, d.network.connectors[0].id, "open");
    validateDocument(d);
    return d;
}
std::map<std::pair<std::string, std::string>, std::string> table(const std::vector<SegmentBehaviour>& entries) {
    std::map<std::pair<std::string, std::string>, std::string> result;
    for (const auto& e : entries) CHECK(result.emplace(std::pair{e.segmentId, e.vehicleTypeId}, e.behaviourId).second);
    return result;
}
}
TEST(behaviourselection, assignment_precedence_and_every_section_and_path_inherit_their_owner) {
    const auto d = project(); const auto sections = runtimeSections(d.network);
    const auto selected = table(compileBehaviourAssignments(d.network, *d.definition));
    const auto& a = d.network.links[0].id;
    std::size_t aSections = 0, paths = 0;
    for (const auto& s : sections.sections) {
        const bool assigned = s.linkId == a;
        aSections += assigned;
        // BA10: the class override for its members, the default for everyone else.
        CHECK(selected.contains({s.id, "car"}) == assigned);
        if (assigned) { CHECK(selected.at({s.id, "car"}) == kDefault); CHECK(selected.at({s.id, "heavy-vehicle"}) == "slow"); }
    }
    CHECK(aSections == 3); // BA11: lane 1 cut by the mid-link Connector, plus lane 2
    for (std::size_t p = 0; p < sections.paths.size(); ++p) {
        const bool assigned = sections.pathConnector[p] == d.network.connectors[0].id;
        paths += assigned;
        CHECK(selected.contains({sections.paths[p].id, "car"}) == assigned);
        if (assigned) CHECK(selected.at({sections.paths[p].id, "heavy-vehicle"}) == "rural");
    }
    CHECK(paths == 2);
    CHECK(selected.size() == (aSections + paths) * d.definition->vehicleTypes.size());
    const auto compiled = compileDocument(d, test::root() / "data").scenario;
    CHECK(table(compiled.segmentBehaviours) == selected); // one owner per segment: no conflict
    auto none = d; for (auto& l : none.network.links) l.behaviourTypeId.reset();
    for (auto& c : none.network.connectors) c.behaviourTypeId.reset();
    CHECK(compileBehaviourAssignments(none.network, *none.definition).empty());
}
TEST(behaviourselection, front_boundary_downstream_rear_ignored_sink_last_and_legacy_without_table) {
    const auto state = createSimulation(joined(true), 42); const auto& index = *state.index;
    const auto& behaviours = state.scenario->behaviours;
    const auto legacy = slot(behaviours, "legacy"), slow = slot(behaviours, "slow");
    CHECK(state.scenario->segments[0].id == "a"); // canonical order differs from the authored one
    CHECK(effectiveBehaviour(index, at(state, "car", 10 - 1e-9)) == legacy);
    CHECK(effectiveBehaviour(index, at(state, "car", 10)) == slow);      // exactly at the join: downstream
    CHECK(effectiveBehaviour(index, at(state, "car", 11)) == slow);      // rear still on a
    CHECK(effectiveBehaviour(index, at(state, "car", 25)) == slow);      // past the sink: last segment
    CHECK(effectiveBehaviour(index, at(state, "bus", 15)) == legacy);    // no entry for this type
    const auto plain = createSimulation(joined(false), 42);
    CHECK(plain.index->behaviourOfSegmentType.empty());
    for (double d : {0., 10., 25.}) CHECK(effectiveBehaviour(*plain.index, at(plain, "car", d)) == legacy);
    // BA15: each vehicle resolves its own front's set; a trailing check reads the follower's refs.
    auto lead = at(state, "car", 12), follower = at(state, "car", 5); follower.id = 2;
    const auto refs = resolveRefs(*state.scenario, {lead, follower}, index);
    CHECK(refs[0].behaviour == slow && refs[1].behaviour == legacy);
}
TEST(behaviourselection, compiled_selections_are_validated) {
    auto s = joined(true); s.segmentBehaviours.push_back({"c", "car", "slow"});
    s.segmentBehaviours.push_back({"a", "truck", "slow"}); s.segmentBehaviours.push_back({"a", "car", "fast"});
    s.segmentBehaviours.push_back({"b", "car", "legacy"});
    std::map<std::string, std::string> found;
    for (const auto& i : validateScenario(s)) found[i.path] = i.code;
    CHECK(found["segmentBehaviours[1].segmentId"] == "UNKNOWN_SEGMENT");
    CHECK(found["segmentBehaviours[2].vehicleTypeId"] == "UNKNOWN_VEHICLE_TYPE");
    CHECK(found["segmentBehaviours[3].behaviourId"] == "UNKNOWN_BEHAVIOUR");
    CHECK(found["segmentBehaviours[4]"] == "DUPLICATE_ID");
    test::throws([&] { createSimulation(s, 42); }, "UNKNOWN_SEGMENT");
}
TEST(behaviourselection, entry_profile_holds_the_same_pending_vehicle_without_redrawing) {
    for (const bool assigned : {false, true}) {
        auto s = joined(false); s.inputs = {{"in", "r", "car", 0, 0, 30}};
        // A 50 m standstill on the entry segment: the 6 m gap that legacy accepts is refused there.
        s.behaviours.push_back(behaviour("cautious", 50));
        if (assigned) s.segmentBehaviours = {{"a", "car", "cautious"}};
        auto state = createSimulation(s, 42);
        Vehicle leader = at(state, "car", 10); leader.id = 1; leader.desiredSpeed = 0;
        PendingVehicle pending; pending.id = 2; pending.inputIndex = 0; pending.routeIndex = 0;
        pending.typeIndex = leader.typeIndex; pending.desiredSpeed = 10; pending.driverFactor = .5; pending.scheduledTime = .05;
        state.vehicles = {leader}; state.inputs[0].queue = {pending}; state.nextVehicleId = 3;
        const auto random = state.randomState;
        state = stepSimulation(std::move(state));
        const bool departed = std::any_of(state.events.begin(), state.events.end(),
                                          [](const auto& e) { return std::holds_alternative<DepartedEvent>(e); });
        CHECK(departed == !assigned);
        if (assigned) { CHECK(state.inputs[0].queue.size() == 1); CHECK(state.inputs[0].queue[0] == pending); }
        CHECK(state.randomState == random); // waiting draws nothing
    }
}
TEST(behaviourselection, same_set_assignment_keeps_the_trajectory_and_a_different_set_changes_it) {
    auto base = parseDocument(Json::parse(std::ifstream(test::root() / "data/projects/four-leg-signalised.traffic.json")));
    putDemandCatalog(base, resolveDemandCatalog(*base.definition, test::root() / "data"));
    const auto run = [](const ProjectDocument& d, int ticks) {
        auto state = createSimulation(compileDocument(d, test::root() / "data").scenario, 42);
        std::vector<SimState> trace;
        for (int i = 0; i < ticks; ++i) { state = stepSimulation(std::move(state)); trace.push_back(state); }
        return trace;
    };
    const auto assign = [&](const std::string& behaviourId) {
        auto d = base;
        putLinkBehaviourType(d, {"everywhere", "", behaviourId, {}});
        for (const auto& l : base.network.links) assignBehaviourType(d, l.id, "everywhere");
        for (const auto& c : base.network.connectors) assignBehaviourType(d, c.id, "everywhere");
        return d;
    };
    for (const auto& t : base.definition->vehicleTypes) CHECK(t.behaviourId == kDefault);
    const auto legacy = run(base, 900), same = run(assign(kDefault), 900);
    CHECK(!compileDocument(assign(kDefault), test::root() / "data").scenario.segmentBehaviours.empty());
    for (std::size_t i = 0; i < legacy.size(); ++i) {
        CHECK(legacy[i].vehicles == same[i].vehicles); CHECK(legacy[i].events == same[i].events);
        CHECK(legacy[i].randomState == same[i].randomState && legacy[i].completed == same[i].completed);
    }
    auto relaxed = base.definition->behaviours.front(); relaxed.id = "relaxed"; relaxed.followingTime *= 3;
    auto other = assign("relaxed"); putBehaviour(other, relaxed, "");
    const auto changed = run(other, 900);
    bool differs = false;
    for (std::size_t i = 0; i < legacy.size() && !differs; ++i) differs = !(legacy[i].vehicles == changed[i].vehicles);
    CHECK(differs);
}
TEST(behaviourselection, zone_waiting_room_covers_an_assigned_larger_standstill) {
    auto s = joined(false); test::near(waitingRoom(s), 10 + 2); // the bus with the legacy set
    s.behaviours[0].standstillDistance = 20; test::near(waitingRoom(s), 12); // unassigned: unused
    s.segmentBehaviours = {{"b", "car", "slow"}}; test::near(waitingRoom(s), 4 + 20);
}
