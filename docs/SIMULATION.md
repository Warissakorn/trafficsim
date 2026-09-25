# Simulation core and network model

M0 implementation reference. **Not yet validated.** This engine is a reduced,
Wiedemann-inspired prototype, not an implementation of W74/W99 and not calibrated to
Vissim. Lane changing, crossing-conflict resolution, general priority control and LOS are not
implemented. **Merge arbitration exists only as M3.1**: a deterministic gap-time/headway threshold,
described below — not a calibrated critical-gap model. M0 remains open until the
owner reviews the live traffic behaviour against its plausibility gate.

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
is not production project persistence; save/versioning remains M1.

## Authoring network

| Object | Meaning |
|---|---|
| `Network` | ID, driving side, links, connectors and signal heads |
| `Link` | Road centreline as an ordered polyline; ordered lanes; no authored junction |
| `Lane` | Globally unique ID and positive width; ordered from driving-side curb inward |
| `Connector` | Explicit lane-to-lane connection with its own polyline and runtime length |
| `NetworkSignalHead` | Lane reference, stop-line position, and a signal group (`controllerId` + `groupNumber`, M2.7b) or a legacy signal-program ID |

Coordinates are planar Cartesian metres with positive Y upwards. Geographic projection
and coordinate-system metadata belong to the future project layer. Runtime segment
lengths are derived from the same lane-centre geometry the renderer uses. The link
centreline is offset using bounded vertex normals; it is an initial geometric
approximation, not a production road-surface or junction-construction algorithm.

Driving side changes lane ordering and offsets. Connector endpoints must match their
referenced lane endpoints within 0.01 m. Changing side on an authored multilane network
requires rebuilding its connector geometry; stale endpoints fail validation.
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

`compileScenario` makes a detached runtime snapshot: one segment per lane and one per
connector. This is derived data, not a second persisted representation of the network.
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
- **Placed decision:** it acts when a routeless vehicle comes onto its Link. The station along
  the Link is not modelled. The vehicle takes a destination its lane can reach, by relative flow
  among those. A lane reaching none carries on routeless, with `ROUTING_DECISION_LANE_UNSERVED`.
  After the destination it is routeless again.
- **On the entry Link** a decision instead puts each destination's flow in the lanes that reach
  it, so typed proportions hold exactly and the input's lane weights are unused (D43).
- **No lane changing:** further downstream, a vehicle's lane fixes its reachable destinations,
  and the proportions shift towards what the lanes allow.
- **Refused on Run:** a revisited lane (`ROUTELESS_CYCLE`), more than 256 paths, an unknown Link,
  two decisions on one Link, or a destination no lane can reach. Inputs still start only on
  entry Links (`UNSUPPORTED_INTERNAL_INPUT`), as for routes.
- Results group these paths into movements by (first Link, last Link), as for authored routes.

**Per-interval turning proportions (M2.1.2, D45).** A routing decision, placed or not, may carry
`intervals` (ordered, non-overlapping `startTime`/`endTime`) and, on every entry, `intervalFlows`
with one relative flow per interval. Inside interval k an entry weighs `intervalFlows[k]`; outside
every interval it weighs `relativeFlow` (the dialog writes the count total there). The input is cut
at the interval boundaries and each piece is split at its own proportions, so the compiled volumes
are exact per piece. **The interval is chosen by the time a vehicle enters the network, not the
time it reaches the decision** — off by the travel time from entry to decision, seconds against
15-minute counts. A placed decision's paths are the union over the intervals, one runtime route
each. Refused: `ROUTING_DECISION_INTERVALS` (a row's flow count differs from the intervals),
`INVALID_INTERVAL`, and `INVALID_SHARE` for a negative flow.

**The counts are proportions of the input's volume (D46).** The input's own volume per period is
what is split; the decision's counts only set the proportions. Their totals, interval lengths and
start times need not match the input's — the input is cut at both sets of boundaries — and an
interval in which every flow is 0 (nothing counted) uses the whole-period `relativeFlow`.

## Runtime scope

- Routes explicitly list connected lane/connector segment IDs. Route order is meaningful.
  There is no routing algorithm, lane changing or repeated segment within a route yet.
  **The runtime route is compiled, never authored** (M1.26): an authored route names Links and
  Connectors, and `buildScenario` expands it into one runtime route per lane the drawing
  carries, keeping the authored id when there is exactly one.
- Sources must begin on segments with no predecessor. They are Poisson processes with
  a rate in vehicles/hour over `[startTime, endTime)`. Zero-rate inputs generate no cars.
  An authored input's volume is the **Link total** and is divided **equally** across its
  route's lanes at compile time. That split is an authoring convenience, not a lane-choice
  model — this engine has no lane changing — and like every figure here it is unvalidated.
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
  authoring who has priority is M3. With protected (split) phasing those movements are never green
  together, so the rule rarely binds; with permissive phasing it would decide, unvalidated.
  A turn that does not wait for green — the Thai left turn at all times — makes it bind every
  cycle. **A derived rule's stop line is 1 m short of the join** (D50): held on the join itself,
  the waiting vehicle's front is on the shared lane and the major vehicle behind it deadlocks.
- Amber is treated as red, with no stop-or-go decision. A vehicle too close to stop when its head
  turns amber is halted at the line by the safety clamp; every clamp in the four-leg fixture and
  in the frozen seed-43/4294967295 baselines is this case (M2_PLAN.md M2.0.3).
- Geometric crossings do not create conflicts automatically. Separate movement paths
  can intersect spatially; their interaction is modelled **only where an author placed a
  crossing conflict area** (M3.2.3a, D57). The demo uses separate fixed-time greens and
  clearance intervals. Overlapping green plans with no authored area have no crossing protection.
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
  - a route minor at one zone and major at another (`CONFLICT_MIXED_ROLES`). It could hold one
    zone while waiting at another whose holder waits on it; M3.2.3c.

  Authored merge areas run on their compiled `PriorityRule`s, the M3.1 mechanism.
- Signal phases and offsets must align to the fixed timestep. Phases are half-open;
  amber is conservatively treated as stop, without a dilemma-zone decision.
- The timestep is in `(0, 0.5]` seconds and duration must be an integer number of ticks.
  Duration and signal timing are validated before any simulation starts.

## Step and motion

1. Generate all arrivals due at the current tick, retaining their sampled properties.
2. Attempt safe source insertion in scheduled-arrival/vehicle-ID order. An arrival that
   cannot enter stays in an external queue. At most one entry per source lane per tick.
3. Build occupied lane intervals from the same pre-step vehicle state. A vehicle's rear
   continues to occupy upstream segments after its front enters a connector.
4. Find the nearest leader or red/amber head anywhere ahead along the route.
5. Compute acceleration, then integrate ballistic displacement. A stop within a step
   uses stopping distance, avoiding negative speed and backwards movement.
6. Apply a conservative displacement cap at the old leader rear or stop line. This
   ensures numerical non-overlap on supported longitudinal paths; it may override the
   configured deceleration in an emergency, emitting `safety-clamp` for inspection.
7. Carry residual travel through every crossed segment, remove front bumpers that reach
   the route sink, and emit events. Vehicle front distance never resets at a connector.

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
