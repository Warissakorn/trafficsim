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
| `NetworkSignalHead` | Lane reference, stop-line position and signal-program ID |

Coordinates are planar Cartesian metres with positive Y upwards. Geographic projection
and coordinate-system metadata belong to the future project layer. Runtime segment
lengths are derived from the same lane-centre geometry the renderer uses. The link
centreline is offset using bounded vertex normals; it is an initial geometric
approximation, not a production road-surface or junction-construction algorithm.

Driving side changes lane ordering and offsets. Connector endpoints must match their
referenced lane endpoints within 0.01 m. Changing side on an authored multilane network
requires rebuilding its connector geometry; stale endpoints fail validation.
Signal heads can be located anywhere along a lane, including its endpoints.

IDs must be nonempty and unique across network objects. Validation reports duplicate
IDs, invalid geometry/widths, empty links, unknown lanes, duplicate connections,
disconnected endpoints and signal positions outside lanes. The compiler also checks
the runtime route, demand, vehicle and control references.

`compileScenario` makes a detached runtime snapshot: one segment per lane and one per
connector. This is derived data, not a second persisted representation of the network.
Desired speeds belong to the vehicle-type distribution, not the link.

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
- Amber is treated as red, with no stop-or-go decision. A vehicle too close to stop when its head
  turns amber is halted at the line by the safety clamp; every clamp in the four-leg fixture and
  in the frozen seed-43/4294967295 baselines is this case (M2_PLAN.md M2.0.3).
- Geometric crossings do not create conflicts automatically. Separate movement paths
  can intersect spatially; their interaction is **not** modelled. The demo uses separate
  fixed-time greens and clearance intervals, not a conflict-area solver. Arbitrary
  overlapping green plans have no crossing-collision protection.
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

Catalog content lives under `data/vehicle-types`, `data/driver-behaviour` and
`data/scenarios`. The compiled boundary allows editor/project work without importing its
types into the engine. Runtime indexes and state-copy costs can be improved after profiling; the current
vector searches and occupancy scans target a small M0 network. No performance gain
is claimed merely because the implementation is now C++.
