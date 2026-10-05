# Simulation core and network model

Current engine/model reference. **Not yet validated.** This engine is a reduced,
Wiedemann-inspired prototype, not an implementation of W74/W99 and not calibrated to
Vissim. Mandatory lane changing exists (M3.2.8b, below). Discretionary changes (D95) and their
hold (D101) are implemented but **off**: no shipped behaviour carries the fields, because A53
fails (vehicles change back within 10 s). Derived merge priority and authored crossing/merge
areas with Stop/Yield are implemented using deterministic thresholds and occupancy checks,
not calibrated critical-gap behaviour. LOS is not implemented. Milestone gates are maintained
in [ROADMAP](../ROADMAP.md); automated checks do not establish owner or scientific validation.

## Run

Use the C++ build described in `docs/BUILDING.md`:

```bash
cmake --preset desktop
cmake --build --preset desktop
ctest --preset desktop
./build/desktop/bin/trafficsim-desktop
./build/desktop/bin/trafficsim-cli 42
```

Use the `headless` preset to build core/model/CLI without Qt.

The editor provides Run/Pause (F5), single step (F6 or Space), reset, seed and playback
speed, and English/Thai text. Changing the seed clears the run, as does editing the drawing.
Playback speed changes the number of fixed steps scheduled by the UI; it never changes the
engine timestep. Pausing stops the Qt timer. The run status shows the completed-trip mean
delay and the safety-clamp count; both are diagnostics, not HCM control delay or LOS.
M1.24 retired the separate M0 harness window — the editor and `trafficsim-cli` are the two
surfaces that run a scenario, and they produce the same run for the same seed.

## Public contracts

```cpp
#include "src/core/simulation.hpp"
#include "src/model/network/network.hpp"
using namespace trafficsim;

// Caller supplies Network and ScenarioDefinition values.
auto issues = validateNetwork(network); // ValidationIssue { code, path }
auto scenario = compileScenario(network, definition); // Throws ValidationError.
auto state = createSimulation(scenario, 42);
auto next = stepSimulation(state); // state is unchanged.
auto final = runSimulation(scenario, 42, [](const SimEvent& event) {
    // Caller owns I/O, rendering and evaluation.
}, false); // Suppress movement callbacks; still simulate every fixed step.
```

`src/core/types.hpp` is the engine contract. `src/model/network/network.hpp` is the
authoring contract. Semantic validation accepts typed values. `src/project/load.hpp`
loads M0 JSON fixtures and catalogs, checking object/array/string/number fields and
rejecting invalid signal/driving-side enums before compilation. This read-only loader
is separate from the editor project codec; see
[Save, recovery and formats](NETWORK_EDITOR.md#save-recovery-and-formats) for persistence.

## Authoring network

| Object | Meaning |
|---|---|
| `Network` | ID, driving side, links, connectors and signal heads |
| `Link` | Road centreline as an ordered polyline; ordered lanes; no authored junction |
| `Lane` | Globally unique ID and positive width; ordered from driving-side curb inward |
| `Connector` | Explicit lane-to-lane connection with its own polyline and runtime length; unequal ends pair lane for lane, one lane added or dropped per side at most (D73) |
| `NetworkSignalHead` | Lane reference, stop-line position, and a signal group (`controllerId` + `groupNumber`, M2.7b) or a legacy signal-program ID |

Coordinates are planar Cartesian metres with positive Y upwards. Geographic projection
and coordinate-system metadata remain outside the current contract. Runtime Link geometry
uses mitered lane offsets (`offsetGeometry`, D23). Connector motion evaluates the direct
cubic equation from mapped lane attachments/tangents (D107), with integrated arc length;
intermediate drawing points and Connector widths do not define that motion. Display paint
can differ from the driving curve. See [Connector geometry](CONNECTOR_FOUR_POINT_MOUTH.md)
and [vehicle pose](VEHICLE_POSE.md) for the separate drawing/display contracts.

Driving side changes lane ordering and offsets. Connector endpoints must match their
referenced lane attachment positions within 0.01 m; attachments may be at an interior
station. Editor changes reanchor Connectors, and stale endpoints fail validation.
Signal heads can be located anywhere along a lane, including its endpoints. The head's station
is the stop line: a vehicle is held there while the head is not green. A signal group of a
fixed-time controller (M2.7b, D48) is compiled into one ordinary `SignalProgram` with id
`<controllerId>#<groupNumber>` — green from green start to green end in cycle seconds
`(t + offset) mod cycle`, then amber, red for the rest — so the engine runs exactly the programs
it always did.

IDs must be nonempty and unique across network objects. Validation reports duplicate
IDs, invalid geometry/widths, empty links, unknown lanes, duplicate connections,
disconnected endpoints and signal positions outside lanes. The compiler also checks
the runtime route, demand, vehicle and control references.

`compileScenario` makes a detached runtime snapshot, splitting Link lanes into sections
at body attachments and connecting the mapped Connector paths. Sections are derived on
compile (D21), not a second persisted representation of the network.
Desired speeds belong to the vehicle-type distribution, not the link.

### Routeless inputs and placed routing decisions (M2.1.1, D42, D43)

A vehicle input may name a Link (`linkId`) instead of a route, and a routing decision may be
placed on a Link (`linkId`), with rows that are destination Links (`destinationLinkId`) or
routes. Both are expanded at compile time (`routelessChains`, `expandRouteless`) into static core
routes `link:<id>/path-k` and one input per complete path, at volume × probability:

- **Free walk:** every Connector path leaving the vehicle's lane is one way out. Leaving the
  network at the lane end is one more when no path leaves from the end. A path leaving within
  `kRoutelessStubLength` (4.5 m) of the end counts as leaving from the end, because a shorter
  remainder cannot hold a vehicle (D44). Each way gets an equal share. A way out upstream of where the vehicle came onto the lane is behind it.
- **Placed decision:** its destination is booked during compilation using scheduled demand
  time, then its path family governs travel on the decision's Link. A station along that Link
  is not modelled. Relative flows apply among destinations the arrival lane serves via a
  complete path or a kept adjacent-lane stub. A lane serving none continues free walk, with
  `ROUTING_DECISION_LANE_UNSERVED`. After the destination it is routeless again.
- **On the entry Link** a decision instead spreads each destination's flow equally over every
  lane of the Link. A lane that cannot reach it changes lanes (M3.2.8b), so typed proportions
  hold exactly and the input's lane weights are unused (D43, D71).
- **Downstream decisions:** free walk with no decision stays lane-fixed. A decision downstream
  supports mandatory changes on its own Link (D93/D94, contract §2 "Downstream routing
  decisions", A40–A46). A kept stub needs a same-entry full route on adjacent lanes; it does
  not allow a change before arrival on that Link. Destination weights apply only to served
  paths; see the contract for stub pruning and family rules.
- **Refused on Run:** a revisited lane (`ROUTELESS_CYCLE`), more than 256 paths, an unknown Link,
  two decisions on one Link, or a destination no lane can reach. Inputs still start only on
  entry Links (`UNSUPPORTED_INTERNAL_INPUT`), as for routes.
- Results group these paths into movements by (first Link, last Link), as for authored routes.

**Per-interval turning proportions (M2.1.2, D45).** A routing decision, placed or not, may carry
`intervals` (ordered, non-overlapping `startTime`/`endTime`) and, on every entry, `intervalFlows`
with one relative flow per interval. Inside interval k an entry weighs `intervalFlows[k]`; outside
every interval it weighs `relativeFlow` (the dialog writes the count total there). The input is cut
at the interval boundaries and each piece is split at its own proportions, so the compiled volumes
are exact per piece. **Scheduled demand time selects the interval**, not eventual entry after
source waiting or arrival at the decision. A queued record keeps its booked route/type (D111–D113). A placed decision's paths are the union over the intervals, one runtime route
each. Refused: `ROUTING_DECISION_INTERVALS` (a row's flow count differs from the intervals),
`INVALID_INTERVAL`, and `INVALID_SHARE` for a negative flow.

**The counts are proportions of the input's volume (D46).** The input's own volume per period is
what is split; the decision's counts only set the proportions. Their totals, interval lengths and
start times need not match the input's — the input is cut at both sets of boundaries — and an
interval in which every flow is 0 (nothing counted) uses the whole-period `relativeFlow`.

## Runtime scope

- Routes explicitly list connected lane/connector segment IDs. Route order is meaningful.
  There is no routing algorithm or repeated segment within a route yet.
  **The runtime route is compiled, never authored** (M1.26): an authored route names Links and
  Connectors, and `buildScenario` expands it into one runtime route per lane of its entry Link
  (`route/lane-k`), keeping the authored id when there is exactly one.
- **Mandatory lane changing (M3.2.8b, D71; contract `M3_8_CONTRACT.md` §2).** A lane that cannot
  reach the route's end compiles to a **stub** route with a dead end; the compile step also
  emits lateral spans (`ScenarioDefinition::laneChanges`, `routeDeadEnds`, never serialized)
  between adjacent lanes of one Link. The core changes a stub vehicle as soon as it lies wholly
  inside a span and both it and its new follower can accept the gap at `comfortableDeceleration`
  without a clamp; nothing alongside, never inside a conflict area. It is instantaneous and
  deterministic (vehicle-id order, no RNG); a stub vehicle is held at its dead end and never
  arrives. Cooperation: the nearest target-lane vehicle that can stop comfortably holds back for a
  vehicle waiting at its dead end (D71). With `maxDecelerationCooperativeBraking` on its behaviour,
  one also brakes, at no more than that, for a changer still moving once its dead end governs its
  car-following (M3.2.8c, D90). Results count `laneChanges`; a stub is never a
  movement row.
- Sources must begin on segments with no predecessor. They are Poisson processes with
  a rate in vehicles/hour over `[startTime, endTime)`. Zero-rate inputs generate no cars.
  An authored input's volume is the **Link total** and is divided **equally** (or by
  `laneShares`, one weight per lane of the entry Link) across that Link's lanes at compile time.
  That split is an authoring convenience, not a lane-choice model — there are no discretionary
  changes in any shipped behaviour (D95 is off; see above) — and like every figure here it is unvalidated.
  **Since M2.2 an input may carry counted `intervals`** (start, end, veh/h), ordered without
  overlap; each becomes its own core input (`id/int-k`), so the core still sees one Poisson
  process per `[startTime, endTime)`. Restarting a Poisson stream at a boundary changes no
  statistics, and an input without intervals compiles exactly as before.
  **Since M2.3 an input may name a composition** from `data/compositions/` instead of one vehicle
  type; resolving the catalog splits it into one input per type (`id/type-<type>`, volume ×
  normalised share), which is exact for Poisson arrivals. `heavy-vehicle` is a plausible,
  **unvalidated** type added for it. Motorcycles are not offered: lane sharing is not modelled.
  **Since M2.4 an input may follow a static routing decision** — relative flows over routes that
  leave one Link — and is split into one input per route (`id/route-<route>`) before everything
  else. Turning proportions are then exact in expectation; lane choice is still fixed at entry.
- A segment with multiple predecessors is rejected with `UNSUPPORTED_MERGE` **unless the merge is
  arbitrated** (M3.1): it is accepted only when at least *n*−1 of its *n* predecessors carry a
  `PriorityRule` naming another of them, so exactly one has priority and the rest have somewhere
  to wait. The guard was narrowed by construction, never removed — a network that has not been
  through the priority model still reports it, and a segment may not give way to itself.
- A `PriorityRule` holds a stop line on the minor approach (`yieldSegmentId`, `yieldPosition`), a
  conflict point on the major one (`conflictSegmentId`, `conflictPosition`), a **gap time in
  seconds** and a **headway in metres**. A vehicle waits while any major vehicle is within the
  headway of the conflict point or would reach it within the gap time, and is held at its stop
  line by the same clamp a red signal head uses. A stopped major vehicle beyond the headway does
  **not** block: a queue that is not moving is a gap, and treating it as a block would deadlock
  the minor approach. **This is a threshold test, not gap acceptance as the literature defines
  it** — no distribution, no driver variation, nothing calibrated. Rule 4 applies.
  `yieldPosition` is metres along `yieldSegmentId`; since M3.2.2c it may be **negative**, a stop
  line on the approach before that segment, but no further back than its chain of single
  predecessors, so every route that reaches the yielding segment crosses the line (D56).
- Rules for a Connector arriving inside a lane body are **derived from the drawing**, never
  persisted, with their two numbers read from `data/priority-rules/`. Run refuses such a network
  with `EDIT_NO_PRIORITY_DEFAULTS` if those cannot be read, rather than defaulting to a zero gap
  time, which would be a merge nobody gives way at.
  Two Connectors arriving at the **same** station share one cut, and the one drawn later also gives
  way to each drawn earlier: lane, then first, then second — a strict order, never a cycle.
  **Since M2.0.1 (D35) the same drawn-order rule arbitrates Connectors meeting at a lane's start**
  — every turn into an intersection exit. The order is the drawing order, which is arbitrary;
  authored controls can take over that priority under [M3_CONTRACT](M3_CONTRACT.md).
  With protected (split) phasing those movements are never green
  together, so the rule rarely binds; with permissive phasing it would decide, unvalidated.
  A turn that does not wait for green — the Thai left turn at all times — makes it bind every
  cycle. **A derived rule's stop line is 1 m short of the join** (D50): held on the join itself,
  the waiting vehicle's front is on the shared lane and the major vehicle behind it deadlocks.
- **Commitment (M3.2.8a, D69; [`M3_8_CONTRACT.md`](M3_8_CONTRACT.md) §1).** A vehicle short of a
  derived rule's stop line or a zone's waiting line is committed when `v² > 2 · maxDeceleration ·
  gap`: it cannot stop there. Equality can stop.
  - A committed vehicle ignores the headway and gap-time parts of the test at that line.
  - It is still held by occupancy (a major vehicle inside the area, or across the conflict point),
    an unserved Stop, receiving space and the swept check. A clamp then is counted as before.
  - It is read off the snapshot, never stored.
  - It replaces the emergency clamp that a gap closing under a driver too close to stop used to
    fire. It does not apply to signal heads.
- Amber is treated as red, with no stop-or-go decision. A vehicle too close to stop when its head
  turns amber may be halted at the line by a counted safety clamp. Other emergency clamps
  remain possible at sources/merges or after a lane change; do not infer their cause from a
  historical fixture count (see NEXT and the clamp evidence).
- The editor derives passive crossing candidates from geometric overlaps (D68/D72); those
  are not enforced merely because they are drawn. Runtime protection comes from authored
  effective conflict controls compiled into zones. The core does not discover geometry or
  provide global collision avoidance. Overlapping greens with no authored protection can
  still conflict; see [M3_CONTRACT](M3_CONTRACT.md).
- A `ConflictZone` (M3.2.3a) is one crossing: a major and a minor side, each an `[entry, exit)`
  interval on one segment, a minor waiting line (`waitPosition`, may be negative as for
  `yieldPosition`), a gap time and a headway. A minor vehicle waits at its line while any major
  vehicle on a route through the major side is inside the area, within the headway of entry,
  or would reach entry sooner than the gap time. Equality passes, and a standing vehicle beyond
  the headway is a gap. It also waits while a standing leader leaves less than its length plus
  standstill distance past the exit. Past the line it **holds** the crossing until its rear clears
  the exit, and a major vehicle waits at entry meanwhile. The grant is read off positions and never
  revoked. A request crossing the line in a tick in which a major front reaches the area is capped
  before anything is published (the swept check). Validation refuses a route that ends within the
  longest vehicle of an exit (`CONFLICT_SINK_TOO_CLOSE`), and one that starts past the line
  (`CONFLICT_ROUTE_STARTS_PAST_LINE`). The whole area is reserved, so capacity is conservative.
  Rule 4 applies: a deterministic threshold, not calibrated gap acceptance.
  **Since M3.2.3b (D58)** a side is a chain of consecutive segments, so an area may lie over a
  section cut. A route turning off inside the area leaves it where it turns off. A major route
  joining the chain part way is in the area from the join. Minor zones along one route with less
  than one vehicle (longest type plus its standstill) between an exit and the next line form a
  **chain**: they share the first line and the last exit, so a vehicle is admitted to all of them
  or to none, and waits for receiving space past the last (A15). Refused by name:
  - a route that meets a minor chain part way (`CONFLICT_ROUTE_JOINS_INSIDE`);
  - **since M3.2.3c (D59)**, zones whose holds form a cycle (`CONFLICT_HOLD_CYCLE`).

  Holds are the edge in question. A route can hold zone M while waiting as a major vehicle at
  zone J's entry, when J's entry comes before it has room to stand clear of M. A cycle of such
  holds is a deadlock waiting to happen. An acyclic mix runs: priority falls along it, so the last
  holder in any chain of waits never waits itself.

  Since M3.2.3c, requests crossing their lines in one tick behind the **same standing leader**
  share its room. Lower vehicle ids are served first, and a request that would not fit is capped at
  its line.

  **Authored merge areas run on the zone solver**, the areas being the last metre of each incoming
  path, so the major side also waits at its entry for an admitted minor. Merges derived from the
  drawing stay on the M3.1 `PriorityRule`.

  Gridlock through queues is not prevented: an admitted vehicle can still stop inside an area
  behind a leader that stopped after admission. The run then records the vehicles as unserved.

  **Stop and Yield (M3.2.5a, D62).** A zone's `control` is `yield` or `stop`:
  - **Yield** is exactly the admission test above. A vehicle with a clear gap never stops.
  - **Stop** also requires each minor vehicle to serve the line before it may cross.

  Serving works like this:
  - The vehicle has come to the line when it is below walking pace (`kStoppedSpeed`, 0.1 m/s) with
    its front within `stopLineReach`. That is the gap car-following keeps behind a standing obstacle
    at that pace, plus 0.1 m.
  - The reduced model only approaches zero behind a line, ever slower. So the Stop finishes the
    stop: the vehicle rests at zero speed for the tick it came to the line and one whole tick more.
    This is ordinary braking of at most 1 m/s², not a safety clamp.
  - Service is kept per vehicle in `SimState::stopService`. It survives waiting for a gap or a red
    head, is cleared once the vehicle passes the line, and a new run starts with none.
  - A queued vehicle stands a vehicle length back, so it has not come to the line.

  A signal head at the same line composes with the Stop: green removes only the head's hold. A
  scenario without a Stop zone runs none of this, and the seed-42 output is unchanged.
- Signal phases and offsets must align to the fixed timestep. Phases are half-open;
  amber is conservatively treated as stop, without a dilemma-zone decision.
- The timestep is in `(0, 0.5]` seconds and duration must be an integer number of ticks.
  Duration and signal timing are validated before any simulation starts.

## Step and motion: a vehicle's input, process and output

**What a vehicle carries** (`Vehicle`, `types.hpp`): input, route and type slots;
`scheduledTime`, `enteredTime`; `desiredSpeed` and `driverFactor`, drawn once at generation; and
`distance` along its route, `speed`, `acceleration`, `mode`. It has no lane, no x/y and no memory
of past decisions: only `distance`, `speed` and `routeIndex` are read back next tick
(`acceleration` and `mode` are outputs only). Admission grants, commitment and courtesy are
re-derived from each snapshot. The one carried table is `SimState::stopService`, outside the vehicle.
`driverFactor` scales only the safety distance (below and `stopLineReach`); gap acceptance,
acceleration and lane changing are the same for every driver of a type.

**Birth.** In: a `VehicleInput`, the state's PRNG. Process: Poisson arrival times; then
insertion in scheduled-time/id order, the queue front of each input only, at most one entry per
source segment per tick, and only with at least `standstillDistance` to the vehicle ahead.
D108 additionally requires the ordinary first following/integration step from rest to fit
the snapshot clearance beyond that same standstill buffer. A denied entry keeps its sampled
vehicle and original scheduled time in the queue; no departure or clamp is emitted.
Equality admits, including a zero-distance step at the exact standstill boundary. The
check anticipates the current leader only, before lane changes/signals/conflict decisions;
those later constraints and genuine moving-vehicle clamps remain enforced.
Out: a `Vehicle` at `distance` 0, speed 0, and `departed`; a blocked arrival waits in its
input's queue (departure delay). Starting from rest is the ≈3 s of entry acceleration in delay.

Accepted changes also append a snapshot-owned `Vehicle.laneChangeTrace` containing
both stations/routes and pre-change speed, solely for display reconstruction
([VEHICLE_POSE.md](VEHICLE_POSE.md)). No physics/evaluation function reads that trace;
this does not introduce a between-lanes engine state.

**Each tick** (`stepSimulation`). In: the pre-step snapshot, the same for every vehicle. Process:
1. Insert arrivals, then build occupied intervals; a rear keeps occupying upstream segments.
2. Mandatory lane changes (M3.2.8b): `decideLaneChanges` moves a stub vehicle to its target
   route, then `courtesyHolds` names who holds back for a waiting one, or brakes cooperatively for
   a moving one. Nothing changes in its
   insertion tick: its rear is still behind the span start.
3. Every constraint becomes the **nearest standing obstacle** and a cap, `allowedDistance`: the
   vehicle ahead (`closestVehicle`); a red or amber head; a stub's dead end; a priority rule
   it must give way at (gap time/headway, M3.1; commitment, M3.2.8a); a conflict zone that does
   not admit it (`zoneHold`, M3.2.3, with Stop service). Courtesy is a **second** obstacle,
   since a nearer moving leader would otherwise hide it. Cooperative braking is a second leader
   at the changer's speed, with no cap, and its braking is bounded by
   `maxDecelerationCooperativeBraking`.
4. `followingAcceleration` behind that obstacle, then ballistic integration (no negative
   speed), with speed capped at `desiredSpeed`.
5. A move beyond `allowedDistance` is cut to it and stops: `safety-clamp`.
6. A vehicle resting at a Stop line is held at zero (`restsAtStop`), which is not a clamp.
7. Phase 2, only with zones: `resolveRequests` (swept check, shared receiving space) can only
   shorten a move.
8. Publish in id order: residual travel crosses segments, `segment-entered` and `moved`.

Out: new `distance`, `speed`, `acceleration`, `mode` and events. Front distance never resets at
a connector.

**Death.** In: front at the route's end, not on a stub. Out: `arrived` with `travelTime` (from
`enteredTime`), `departureDelay` (`enteredTime − scheduledTime`) and `freeFlowTime` (route
length ÷ `desiredSpeed`, no acceleration term).

A runtime vehicle identifies its input, route and vehicle type by **index into the canonical
`Scenario`**, not by id (D29). `createSimulation` sorts the scenario once and never changes it
again, so a slot names exactly what an id named; `ScenarioIndex` resolves each input's route and
type once, and nothing looks an id up per tick. Names reappear at the boundary and only there:
every event carries `routeId`, and a checkpoint carries `inputId`, `routeId` and
`vehicleTypeId`, read back from the scenario. A slot must never be written to a file or an
event — it means nothing outside the `Scenario` it indexes.

At the final tick, arrivals from the last subinterval are retained in the pending queue;
they are not silently lost merely because no step remains to insert them. No vehicle is
teleported out of a blocked input. Results must show active and pending vehicles as well
as completed trips to avoid hiding unserved demand.

The prototype desired gap is:

`standstillDistance + (additiveSafetyDistance + multiplicativeSafetyDistance * driverFactor) * sqrt(speed)`

D105: a vehicle at exactly zero speed with a leader gap at or below `standstillDistance`
never gets positive following acceleration. It waits until the gap opens, including when a
moving leader's rear is clipped to the start of a shared merge segment. This prevents an
unnecessary start followed by a zero-distance safety clamp; moving vehicles still use the
same acceleration and hard cap, and their emergency clamps remain counted. No tolerance,
new state, gap relaxation or commitment change is introduced.


`driverFactor` is a clipped normal draw with mean 0.5 and standard deviation 0.15, sampled
once per vehicle. The distance shape is inspired by the
[PTV Wiedemann 74 parameter documentation](https://cgi.ptvgroup.com/vision-help/VISSIM_2025_ENG/Content/4_BasisdatenSim/FahrverhaltensparameterFolgeverh_Wied74.htm).
Unlike that model, this prototype does not smear standstill distance or use its full
perception thresholds. Its free/approaching/following/braking regime thresholds and
acceleration equations are defined in `src/core/following.cpp`; they are our own reduced
approximation. Parameters must not be transferred from a calibrated Vissim model.

## Determinism, snapshots and events

The PRNG is xorshift32 with explicit state. Seeds are unsigned 32-bit integers; zero
maps to the fixed nonzero state `0x6d2b79f5`. Desired speeds are sampled uniformly within
the vehicle-type range. Scenario collections are sorted by ID, while geometric points,
route order and phase order are preserved. Stable IDs control simultaneous insertion.

`createSimulation` copies and canonicalizes the scenario into shared const ownership.
Each step returns an independent value state and does not mutate its argument. Config
edits cannot change an active run. Only the detached, immutable scenario is shared
between steps; vehicle/queue/event vectors are copied. Public state structs are editable
C++ values, so callers must treat published snapshots as read-only.

Simulation time is integer tick times fixed timestep. There is no wall-clock, framework,
I/O, JSON, unordered container or thread access inside `core/`. The restricted-include
checker, its negative fixtures and separate CMake targets protect this boundary. The
checker is not a full C++ parser; unconventional preprocessor constructs need review.

Same scenario, seed, engine version and toolchain reproduce the same C++ event stream.
`log`/`cos` are not promised to be bit-identical across JS engines or C++ math libraries.
Preserve engine/compiler/runtime versions with future report runs. Compiler settings
disable fast-math and floating-point contraction; draw order is explicitly sequenced.

The old TS seed-42 event-string fingerprint remains in Git history. Four saved TS
fixtures compare all non-movement events and full states every 10 seconds at a 1e-7
absolute tolerance on physical values. IDs, ticks, counts and categories agree exactly.
Native replay tests compare every event exactly on the same build. See `MIGRATION.md`.

| Event | Meaning |
|---|---|
| `signal` | Initial or changed signal colour |
| `departed` | Actual insertion, original scheduled time and sampled desired speed |
| `moved` | End-of-step segment position, speed and acceleration |
| `segment-entered` | Every segment boundary crossed during a step |
| `safety-clamp` | Numerical overlap prevention changed the proposed displacement |
| `arrived` | Sink reached; actual travel time, source delay and free-flow reference |
| `lane-change` | A stub vehicle moved to another route of its movement at the start of a step (M3.2.8b) |

Movement and arrival times are quantized to the ending tick. Signals for movement are
evaluated at the starting tick. `state.events` contains only the latest step; callers
use `runSimulation` callbacks to stream history without retaining all trajectory records in state.

## Evaluation and extension points

`src/eval/summary.cpp` is a completed-trip diagnostic. Mean delay is actual travel time plus
source wait minus route length divided by sampled desired speed. It includes acceleration
loss; it is **not HCM control delay or LOS**. An empty run returns `null`, not NaN. Incomplete
trips do not enter this mean and must be reported separately.

### Movement evaluation (M2.5)

`src/eval/movement.cpp` turns one run into a per-movement table and per-approach queues. It
reads `SimState` snapshots and their events only (`MovementAccumulator::observe`, called once
after `createSimulation` and once after every `stepSimulation`). `core/` does not know it exists.
`src/project/evaluation.cpp` supplies what it measures (`evaluationSpec`).

- **Movement:** an authored route's (first Link, last Link) pair, named from the Links' Names,
  in authored route order. Two routes with the same pair are one movement. Every runtime route
  (`id`, `id/lane-k`) maps through its authored id.
- **Delay** per movement is the mean over completed trips of the run summary's own term:
  `max(0, travelTime + departureDelay − freeFlowTime)` over the **whole route** (D39). The
  movements' trips plus `notInMovement` equal the run's completed trips.
  - The figure is labelled *simulated movement delay, one run* and is **not HCM control delay or
    LOS**.
  - **Known bias:** a vehicle enters the network from standstill, so an unimpeded trip already
    carries about `v/(2a)` of acceleration delay (≈3 s for the car type). The analytic test pins
    this rather than hiding it. Cross-section travel-time sections, which remove it, are M5's.
- **Queue** per approach follows Vissim's queue counter at each signal head's stop line (D40).
  - A vehicle enters queue state below `beginSpeed` and leaves it above `endSpeed`.
  - Walking upstream from the line along each route that crosses it, the queue ends at the first
    vehicle not in queue state or at a clear gap over `maxHeadway`. Its length runs to that last
    vehicle's rear.
  - An approach is the maximum over its lanes' heads at each step. The report gives the mean over
    every observed step and the maximum, in metres.
  - The conditions are content: `data/evaluation/queue-counter.json`, in km/h and m, with
    Vissim's defaults of 5, 10 and 20.
- **Unserved demand:** vehicles still in the network (`active`), vehicles waiting to enter
  (`pending`) and safety clamps are reported beside every table. Incomplete trips are not in any
  delay.

The editor shows this in the **Results** tab, and `trafficsim-cli --project FILE [--csv FILE]`
prints the same report as JSON and CSV. Both carry the not-yet-validated marker. Several seeds,
confidence intervals and LOS letters are M5.

Catalog content lives under `data/vehicle-types`, `data/driver-behaviour` and
`data/scenarios`. The compiled boundary allows editor/project work without importing its
types into the engine. Runtime indexes and state-copy costs can be improved after profiling; the current
vector searches and occupancy scans target a small M0 network. No performance gain
is claimed merely because the implementation is now C++.
