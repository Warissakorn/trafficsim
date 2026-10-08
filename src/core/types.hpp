#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace trafficsim {
// Runtime contract: metres, seconds, m/s and m/s². No UI or file-format types.
struct Segment { std::string id; double length{}; std::vector<std::string> next; };
struct Route {
    std::string id; std::vector<std::string> segmentIds;
    bool operator==(const Route&) const = default;
};
// M3.3.3a W74 (docs/reference/W74.md, D129). Every key is required data (hard rule 5); there
// are deliberately no defaults.
struct W74Parameters {
    double ax, bxAdd, bxMult, exAdd, exMult, cxAdd, cxMult, opdvAdd, opdvMult, dMax;
    double bMaxAdd, bMaxMult, bMaxSpeedRoot, bNullAdd, bNullMult, bMinAdd;
    double leaderAccelerationWeight, emergencyLeaderWeight;
    bool operator==(const W74Parameters&) const = default;
};
// Per-driver draws in [0, 1], fixed when the vehicle is generated (contract §5).
struct W74Traits {
    double zBx, zEx, zCx, zOp, zOsc;
    bool operator==(const W74Traits&) const = default;
};
enum class W74Regime : std::uint8_t { free, approaching, following, emergency };
// The previous tick's regime and the oscillation sign it used (0 outside following), §7.
struct W74State {
    W74Regime regime{};
    std::int8_t sign{};
    bool operator==(const W74State&) const = default;
};
struct DriverBehaviour {
    std::string id;
    double standstillDistance{}, additiveSafetyDistance{}, multiplicativeSafetyDistance{};
    double followingTime{}, speedThreshold{};
    // M3.2.8c: Vissim's "Maximum deceleration for cooperative braking" (m/s², positive). Without
    // it the behaviour does not brake cooperatively for a changer that is still moving (D90).
    std::optional<double> maxDecelerationCooperativeBraking;
    // M3.2.8c (D95): a vehicle on a full route changes to an adjacent one by choice when it can
    // accelerate harder there by at least the threshold (m/s², ours; Vissim has none), and the
    // trailing vehicle there brakes at no more than the accepted deceleration (m/s², positive).
    // Both or neither: without them the behaviour makes no discretionary change.
    std::optional<double> discretionaryLaneChangeThreshold, acceptedDecelerationTrailingVehicle;
    // D101: seconds after a vehicle's last lane change (either kind) during which it makes no
    // discretionary change. Ours; Vissim has none. Without it there is no hold.
    std::optional<double> discretionaryLaneChangeHoldTime;
    // Present iff the behaviour's model is `w74` (schema 25, D136); its prototype fields are then
    // unused and never written. Last, so every existing brace-initialisation keeps its meaning.
    std::optional<W74Parameters> w74;
    bool operator==(const DriverBehaviour&) const = default;
};
struct SpeedRange { double min{}, max{}; bool operator==(const SpeedRange&) const = default; };
struct VehicleAxles {
    double wheelbase{}, frontOverhang{}, rearOverhang{};
    bool operator==(const VehicleAxles&) const = default;
};
struct VehicleType {
    std::string id;
    double length{}, width{};
    SpeedRange desiredSpeed;
    double maxAcceleration{}, comfortableDeceleration{}, maxDeceleration{};
    std::string behaviourId;
    // Display kinematics only; traffic following/reservations still use length.
    std::optional<VehicleAxles> axles;
    bool operator==(const VehicleType&) const = default;
};
inline VehicleAxles vehicleAxles(const VehicleType& type) {
    // Nominal proportions for old catalogs, not calibrated vehicle dimensions.
    return type.axles.value_or(VehicleAxles{.6*type.length,.2*type.length,.2*type.length});
}
// One period of an input's volume (M2.2). A counted 15-minute table is a list of these.
struct VolumeInterval {
    double startTime{}, endTime{}, vehiclesPerHour{};
    bool operator==(const VolumeInterval&) const = default;
};
struct VehicleInput {
    std::string id, routeId, vehicleTypeId;
    double vehiclesPerHour{}, startTime{}, endTime{};
    // Relative weights, one per lane `routeLaneChains` currently expands `routeId` into, in that
    // same order (M1.26.1). Empty is the default and means "split equally" -- the M1.26 behaviour
    // -- and a size that no longer matches the route's lane count (the network was edited after
    // the shares were set) is treated the same as empty rather than misapplied to the wrong lane.
    // Normalised by their sum at compile time; the values themselves need not sum to 1.
    std::vector<double> laneShares;
    // Authoring only, like laneShares (M2.2): the volume per period, ordered and non-overlapping.
    // Empty means the one period [startTime, endTime) at vehiclesPerHour -- every input before
    // M2.2. When set it is the source, and startTime/endTime/vehiclesPerHour are DERIVED from it
    // (deriveInputTotals) so tables still read one figure; buildScenario expands it into one core
    // input per period and the core never sees it.
    std::vector<VolumeInterval> intervals;
    // Authoring only (M2.3): a composition from data/compositions/ instead of one vehicle type.
    // Non-empty means vehicleTypeId is unused; resolveCatalogs expands it into one input per type.
    std::string compositionId;
    // Authoring only (M2.4): a static routing decision instead of one route. Non-empty means
    // routeId is unused; the decision's routes and relative flows split the volume.
    std::string routingDecisionId;
    // Authoring only (M2.1.1): no route at all. Vehicles enter on this Link and follow the
    // network -- an equal share at every branch, and a placed routing decision's flows where they
    // meet one. buildScenario expands it into one core input per complete path.
    std::string linkId;
    bool operator==(const VehicleInput&) const = default;
};
enum class SignalColor { red, amber, green };
struct SignalPhase { double duration{}; SignalColor color{}; bool operator==(const SignalPhase&) const = default; };
struct SignalProgram {
    std::string id; double offset{}; std::vector<SignalPhase> phases;
    bool operator==(const SignalProgram&) const = default;
};
struct SignalHead { std::string id, segmentId; double position{}; std::string programId; };
// A minor approach giving way to a major one, at a point where two segments feed the same place.
// Vissim's priority rule: a stop line on the minor approach, a conflict marker on the major one,
// and the two numbers an engineer tunes -- a gap time in seconds and a headway in metres.
//
// This is a DETERMINISTIC THRESHOLD TEST, not a calibrated critical-gap distribution: a vehicle
// waits while any major vehicle is within `headway` of the conflict point or would reach it
// within `gapTime`, and goes otherwise. It makes a merge expressible and tunable. It does not
// make it validated -- see hard rule 4.
struct PriorityDefaults {
    double gapTime{}, headway{};
    bool operator==(const PriorityDefaults&) const = default;
};
struct PriorityRule {
    std::string id;
    std::string yieldSegmentId; double yieldPosition{};      // where the minor approach waits; <0 is before the segment
    std::string conflictSegmentId; double conflictPosition{}; // the point on the major approach
    double gapTime{};   // seconds: a major vehicle arriving sooner than this is not yielded to
    double headway{};   // metres: a major vehicle closer than this to the point blocks regardless
    bool operator==(const PriorityRule&) const = default;
};
// M3.2.3a. One crossing: the minor side gives way to the major side, and the two never occupy
// their areas at once. A side is a chain of consecutive segments (M3.2.3b): `entry` is metres
// along the first, `exit` along the last, so an area may run over a section cut. `waitPosition`
// is on the minor chain's first segment and may be negative -- on the single-predecessor
// approach before it (D56). A minor vehicle holds the crossing from passing its waiting line
// until its rear clears the exit; that grant is read off positions, so it needs no state (D57).
struct ZoneSide {
    std::vector<std::string> segmentIds; double entry{}, exit{};
    bool operator==(const ZoneSide&) const = default;
};
// M3.2.5 (contract §5). `yield` is admission by gap alone: a minor vehicle with a clear gap never
// has to stop. `stop` also requires each minor vehicle to serve the waiting line first -- stand at
// it for one complete tick -- before it may cross, gap or no gap.
enum class ZoneControl { yield, stop };
struct ConflictZone {
    std::string id;
    ZoneSide major, minor;
    double waitPosition{}; // on minor.segmentId: where a minor vehicle waits for admission
    double gapTime{};      // seconds, as PriorityRule
    double headway{};      // metres, as PriorityRule
    ZoneControl control{ZoneControl::yield};
    bool operator==(const ConflictZone&) const = default;
};
// M3.2.8b (docs/reference/M3_8_CONTRACT.md §2). A vehicle on route `fromRouteId` whose rear is at or past
// `fromStart` and whose front is at or short of `fromEnd` may change to `toRouteId`; distances map
// linearly between [fromStart, fromEnd] and [toStart, toEnd]. Compiled from the drawing -- two
// chains of one movement on adjacent lanes of a Link -- and never authored: the core sees route
// distances only, never a lane.
struct LaneChangeSpan {
    std::string fromRouteId, toRouteId;
    double fromStart{}, fromEnd{}, toStart{}, toEnd{};
    bool operator==(const LaneChangeSpan&) const = default;
};
// A route that cannot reach its movement's end on its own lane (a stub): a vehicle on it is held
// at `at` until it changes lanes, and never arrives on it.
struct RouteDeadEnd {
    std::string routeId; double at{};
    bool operator==(const RouteDeadEnd&) const = default;
};
// Compiled station routing. Alternatives share the exact physical prefix through `at`.
struct RoutingChoice {
    std::string routeId; double weight{}; std::vector<double> intervalWeights;
    bool operator==(const RoutingChoice&) const = default;
};
struct RouteDecision {
    std::string id, fromRouteId; double at{};
    std::vector<VolumeInterval> intervals; // only start/end are used
    std::vector<RoutingChoice> choices;
    bool operator==(const RouteDecision&) const = default;
};
// M3.3.2b (D127), COMPILED from Link/Connector behaviour-type assignments, never authored: the
// behaviour a vehicle type uses while its FRONT is on this segment. Ids, not slots, because
// createSimulation re-sorts segments, types and behaviours; the index resolves them once.
struct SegmentBehaviour {
    std::string segmentId, vehicleTypeId, behaviourId;
    bool operator==(const SegmentBehaviour&) const = default;
};
struct ScenarioDefinition {
    double duration{}, timeStep{};
    std::vector<Route> routes;
    std::vector<VehicleType> vehicleTypes;
    std::vector<DriverBehaviour> behaviours;
    std::vector<VehicleInput> inputs;
    std::vector<SignalProgram> signalPrograms;
    // Ordered last so every existing brace-initialisation of a definition keeps meaning what it
    // says. Empty is the normal state: a scenario with no merge needs no rule.
    std::vector<PriorityRule> priorityRules;
    // The two numbers a rule DERIVED from the drawing is given, read from data/priority-rules/ by
    // the project layer. Content, not code (hard rule 5). Left at zero here on purpose: a zero
    // gap time is an uncontrolled merge, so deriving a rule without loading these is refused
    // rather than silently allowed.
    PriorityDefaults priorityDefaults;
    // Last again, for the same reason. Empty for every scenario without an authored crossing,
    // which then runs exactly the code path it always did.
    std::vector<ConflictZone> conflictZones;
    // M3.2.8b, last again. COMPILED, never authored and never in a project file: the compile step
    // that expands routes into lane chains fills these alongside them (buildScenario for authored
    // routes, expandRouteless for routeless paths). Empty for every scenario without a stub,
    // which then runs exactly the code path it always did.
    std::vector<LaneChangeSpan> laneChanges;
    std::vector<RouteDeadEnd> routeDeadEnds;
    std::vector<RouteDecision> routeDecisions;
    // M3.3.2b, last again. Empty without an assigned road, which then runs exactly the legacy
    // type-only selection: same slots, same order of operations, same random draws.
    std::vector<SegmentBehaviour> segmentBehaviours;
    // Value equality, so callers can tell "this edit changed nothing" without serialising.
    bool operator==(const ScenarioDefinition&) const = default;
};
struct Scenario : ScenarioDefinition {
    std::vector<Segment> segments;
    std::vector<SignalHead> signalHeads;
};
struct RoutePart { std::string segmentId; std::size_t segmentIndex{}; double start{}, length{}; };
// Route geometry is a pure function of an immutable Scenario, so it is resolved once per run
// instead of per vehicle per tick. parts[i] corresponds to Scenario::routes[i] after
// canonicalisation; nothing here is derived from vehicle state.
// A signal head that lies on a route, with the start station of the FIRST route part
// carrying that head's segment - the part the per-vehicle scan used to search for.
struct RouteHead { std::size_t headIndex{}; double partStart{}; };
// The same idea for a priority rule: a rule whose yield segment lies on this route, with the start
// station of the first route part carrying it, so the stop line is a route coordinate.
struct RouteRule { std::size_t ruleIndex{}; double partStart{}; };
// A conflict zone one of whose sides lies on this route, in route distances resolved once.
// `entryAt`/`exitAt` bound the area along this route: a route joining the chain part way is in
// the area from where it joins, one leaving part way is out where it leaves. For a minor role,
// `waitAt` is where it waits and `clearAt` the exit its receiving space is measured past; a chain
// of zones with no room to wait between them shares the first line and the last exit (A15).
enum class ZoneRole : std::uint8_t { major, minor };
struct RouteZone {
    std::size_t zoneIndex{}; ZoneRole role{};
    double entryAt{}, exitAt{}, waitAt{}, clearAt{};
    bool joinsInside{}; // met the chain after its first segment
};
// A lateral span as the run uses it: the target route's slot and the same four distances.
struct RouteLaneChange { std::size_t target{}; double fromStart{}, fromEnd{}, toStart{}, toEnd{}; };
struct ScenarioIndex {
    std::vector<std::vector<RoutePart>> parts;
    // M3.2.8b, parallel to Scenario::routes. `remainingOfRoute` is how many changes a vehicle on the
    // route still has to make (0 for a full route); `deadEndOfRoute` is +infinity without one.
    bool laneChanges{};
    std::vector<std::vector<RouteLaneChange>> laneChangesOfRoute;
    std::vector<double> deadEndOfRoute;
    std::vector<std::uint32_t> remainingOfRoute;
    // D95: the full-to-full spans, apart from `laneChangesOfRoute` (stub spans only), in target-slot
    // order. `discretionary` is set only when some span exists AND some behaviour has the threshold.
    bool discretionary{};
    std::vector<std::vector<RouteLaneChange>> discretionaryOfRoute;
    std::vector<std::vector<RouteZone>> routeZones;  // parallel to Scenario::routes, in conflictZones order
    bool stopZones{};                                // any zone is a Stop: only then is service tracked
    std::vector<std::size_t> programOfHead;          // parallel to Scenario::signalHeads
    std::vector<std::vector<RouteHead>> routeHeads;  // parallel to Scenario::routes, in signalHeads order
    std::vector<std::vector<RouteRule>> routeRules;  // parallel to Scenario::routes, in priorityRules order
    // The segment index each rule watches, so the per-tick scan is a bucket lookup and never a
    // search by id. SIZE_MAX when the rule names a segment that does not exist; validation
    // rejects that scenario, but a hand-built index must not read out of bounds before it does.
    std::vector<std::size_t> conflictSegmentOfRule;  // parallel to Scenario::priorityRules
    // The behaviour a vehicle uses depends only on its TYPE, so it needs no per-vehicle lookup
    // at all once the type is known. The sorted routeOfId/typeOfId tables that used to live here
    // are gone with the string ids they existed to resolve: a vehicle carries its slots.
    std::vector<std::size_t> behaviourOfType;        // parallel to Scenario::vehicleTypes
    // M3.3.2b: segments x types, row-major, a behaviour slot each -- behaviourOfType overridden by
    // Scenario::segmentBehaviours. EMPTY when nothing is assigned: then the type alone decides.
    std::vector<std::size_t> behaviourOfSegmentType;
    // The slots a released vehicle is stamped with. An input's route and type never change, so
    // resolving them per input per tick -- which is what generateArrivals did once the vehicle
    // stopped carrying ids -- was the same lookup in a new place.
    std::vector<std::uint32_t> routeOfInput, typeOfInput; // parallel to Scenario::inputs
};
// Scenario lookups for one vehicle, resolved once per tick instead of once per use.
struct VehicleRefs { std::size_t route{}, type{}, behaviour{}; };
enum class FollowingMode { free, approaching, following, braking };
struct PendingVehicle {
    std::uint64_t id{};
    // SLOTS in the canonical Scenario, not ids. A Scenario is immutable and sorted by id from
    // createSimulation onwards, so an index names exactly the object an id named -- and a tick
    // copies, compares and sorts the whole vehicle list, which three std::strings per vehicle
    // made the most expensive thing the engine did. Resolution happens once, where the vehicle
    // is created; nothing downstream looks an id up again.
    //
    // kNoInput marks a vehicle placed directly rather than released by an input. Only tests do
    // that; nothing indexes `inputs` with it, and it serialises as an empty inputId.
    static constexpr std::uint32_t kNoInput = 0xffffffffU;
    std::uint32_t inputIndex{kNoInput}, routeIndex{}, typeIndex{};
    double scheduledTime{}, desiredSpeed{}, driverFactor{};
    bool operator==(const PendingVehicle&) const = default;
};
// D101: a vehicle's last lane change, of either kind -- the tick it started and the route it left.
// Inside SimState, so a copied state replays exactly; read only by the discretionary hold.
struct LastLaneChange {
    std::uint64_t tick{}; std::uint32_t fromRoute{};
    bool operator==(const LastLaneChange&) const = default;
};
// Display reconstruction only. Stations are captured BEFORE the route remap;
// physics, admission and measurements never read this trace. Owned by SimState
// so replay/seek and a second change can reconstruct the same rolling body.
struct LaneChangeTrace {
    std::uint32_t fromRoute{}, toRoute{};
    double fromDistance{}, toDistance{}, speed{};
    bool operator==(const LaneChangeTrace&) const = default;
};
struct Vehicle : PendingVehicle {
    double enteredTime{}, distance{}, speed{}, acceleration{};
    FollowingMode mode{FollowingMode::free};
    std::optional<LastLaneChange> lastLaneChange;
    std::vector<LaneChangeTrace> laneChangeTrace;
    std::vector<std::string> passedDecisions; // one selection per passage; routes cannot cycle
    bool operator==(const Vehicle&) const = default;
};
// Parallel to Scenario::inputs, one entry each and in that order: createSimulation builds it
// that way, and stepSimulation asserts it. There is deliberately no id here -- it would be a
// second copy of Scenario::inputs[i].id, free to disagree with it (hard rule 3).
struct InputState {
    std::optional<double> nextArrival;
    std::vector<PendingVehicle> queue;
    bool operator==(const InputState&) const = default;
};
struct SignalEvent {
    double time{}; std::string signalId; SignalColor color{};
    bool operator==(const SignalEvent&) const = default;
};
struct DepartedEvent {
    double time{}; std::uint64_t vehicleId{}; std::string routeId;
    double scheduledTime{}, desiredSpeed{};
    bool operator==(const DepartedEvent&) const = default;
};
struct MovedEvent {
    double time{}; std::uint64_t vehicleId{}; std::string segmentId;
    double position{}, speed{}, acceleration{};
    bool operator==(const MovedEvent&) const = default;
};
struct SegmentEnteredEvent {
    double time{}; std::uint64_t vehicleId{}; std::string segmentId;
    bool operator==(const SegmentEnteredEvent&) const = default;
};
struct SafetyClampEvent {
    double time{}; std::uint64_t vehicleId{};
    bool operator==(const SafetyClampEvent&) const = default;
};
struct ArrivedEvent {
    double time{}; std::uint64_t vehicleId{}; std::string routeId;
    double travelTime{}, departureDelay{}, freeFlowTime{};
    bool operator==(const ArrivedEvent&) const = default;
};
// M3.2.8b: a vehicle moved from one route to another at the start of a tick (contract §2).
struct LaneChangeEvent {
    double time{}; std::uint64_t vehicleId{}; std::string fromRouteId, toRouteId;
    bool operator==(const LaneChangeEvent&) const = default;
};
struct RoutingEvent {
    double time{}; std::uint64_t vehicleId{}; std::string decisionId, fromRouteId, toRouteId;
    bool operator==(const RoutingEvent&) const = default;
};
// New event alternatives append so every existing index stays stable.
using SimEvent = std::variant<SignalEvent, DepartedEvent, MovedEvent,
                              SegmentEnteredEvent, SafetyClampEvent, ArrivedEvent, LaneChangeEvent, RoutingEvent>;
// M3.2.5: one vehicle's service at a Stop line (contract §5). `line` is the route distance of the
// next Stop line it has not passed; `since` the tick whose start first found it standing there.
// It has served the line once a whole tick has run since then. Kept only while the line is ahead
// of or under the vehicle, so passing a line clears it; a new run starts with none.
struct StopService {
    std::uint64_t vehicleId{}; double line{}; std::uint64_t since{};
    bool operator==(const StopService&) const = default;
};
struct SimState {
    // Detached at createSimulation; copies share only this immutable scenario.
    std::shared_ptr<const Scenario> scenario;
    // Derived from scenario alone; shared, never copied per tick.
    std::shared_ptr<const ScenarioIndex> index;
    std::uint32_t seed{}, randomState{};
    std::uint64_t tick{}, nextVehicleId{1}, completed{};
    double time{};
    std::vector<InputState> inputs;
    std::vector<Vehicle> vehicles;
    std::vector<SimEvent> events; // Latest step only.
    std::vector<StopService> stopService; // sorted by vehicle id; empty without a Stop zone
};
struct ValidationIssue {
    std::string code, path;
    bool operator==(const ValidationIssue&) const = default;
};
// Walking pace: a stub vehicle this slow, at its dead end, is waiting for a gap (cooperation).
inline constexpr double kWaitingSpeed = 0.5;
// M3.2.8b, cooperation's "who is waiting" (M3_8_CONTRACT.md §2): a vehicle on a stub route at
// walking pace, as close to its dead end as car-following brings it. Declared with the contract,
// not in lanes.hpp, because evaluation (M3.2.8c step 1) reports the same waits and may see only
// this header: one definition for the engine and for the report. Defined in lanes.cpp.
bool waitingAtDeadEnd(const ScenarioIndex&, std::size_t route, const Vehicle&, const DriverBehaviour&);
// M3.2.8c (D90/D92), for the same reason -- cooperative braking and the wait-cause diagnostic:
// - deadEndGoverns: a stub vehicle's look-ahead -- its dead end, taken as a standing obstacle,
//   already governs its car-following. Cooperative braking helps a moving changer only then.
// - laneChangeTargetOf: the first span target a vehicle with this front and rear may change to,
//   in the index's order (fewest changes left, then the lower slot); null inside no span.
// - mappedOnto: a distance along a span's source, as a distance along its target.
bool deadEndGoverns(const ScenarioIndex&, std::size_t route, const Vehicle&, const VehicleType&, const DriverBehaviour&);
const RouteLaneChange* laneChangeTargetOf(const ScenarioIndex&, std::size_t route, double front, double rear);
double mappedOnto(const RouteLaneChange&, double at);
// M3.3.2b (D127, DRIVING_BEHAVIOUR.md §3): the one behaviour every consumer uses -- following,
// source clearance, own and trailing lane-change checks, courtesy, stop service, receiving space
// and the evaluation diagnostics. Chosen by the FRONT's segment on the vehicle's current route:
// a front exactly at a join belongs to the downstream segment, at or past the route end to the
// last. Without assignments it is behaviourOfType[type], unchanged. Defined in routes.cpp.
std::size_t effectiveBehaviour(const ScenarioIndex&, const Vehicle&);
// The route part a front at `distance` is on, by that same convention.
const RoutePart& frontPart(const std::vector<RoutePart>& parts, double distance);
// M3.3.1b2b2 (D125): the pending vehicles the next step generates at its start, in id order,
// replayed on a scratch copy -- the state is not changed. Lets an observer name the type of a
// vehicle generated, inserted and sunk within one tick. Defined in demand.cpp.
std::vector<PendingVehicle> upcomingArrivals(const SimState&);
}
