# Simulation core and network model

M0 implementation reference. **Not yet validated.** This engine is a reduced,
Wiedemann-inspired prototype, not an implementation of W74/W99 and not calibrated to
Vissim. Lane changing, merge arbitration, crossing-conflict resolution, priority control,
LOS and production project persistence are not implemented. M0 remains open until the
owner reviews the live traffic behaviour against its plausibility gate.

## Run

Use Node.js 22.12 or newer (tested with 24.19.0):

```bash
npm ci
npm test
npm run dev
npm run build
npm run simulate -- 42
```

The development harness provides Run/Pause, single step, reset, seed and playback speed,
and English/Thai text. Changing the seed resets the run. Playback speed changes the number
of fixed steps scheduled by the UI; it never changes the engine timestep. Pausing cancels
the animation callback. The canvas is a diagnostic view, not the network editor.

## Public contracts

```ts
import { compileScenario, validateNetwork } from './src/model/network';
import { createSimulation, stepSimulation, runSimulation } from './src/core';

// The caller supplies an authoring Network and a ScenarioDefinition.
const issues = validateNetwork(network); // { code, path }[], no UI strings
const scenario = compileScenario(network, definition); // throws on invalid input
let state = createSimulation(scenario, 42);
state = stepSimulation(state); // new immutable state; input state is unchanged

for (const event of runSimulation(scenario, 42, { includeMovementEvents: false })) {
  // Caller owns I/O, rendering and evaluation.
}
```

`src/core/types.ts` defines the complete engine contract. `src/model/network/types.ts`
defines the authoring contract. Validation functions accept these typed structures;
they are not general-purpose parsers for untrusted JSON. A future project loader must
check structural shape before passing data to these validators.

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
- Sources must begin on segments with no predecessor. They are Poisson processes with
  a rate in vehicles/hour over `[startTime, endTime)`. Zero-rate inputs generate no cars.
- A segment with multiple predecessors is rejected with `UNSUPPORTED_MERGE`. Choosing
  priority for competing entries would otherwise silently introduce a right-of-way model.
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
acceleration equations are defined in `core/following.ts`; they are our own reduced
approximation. Parameters must not be transferred from a calibrated Vissim model.

## Determinism, snapshots and events

The PRNG is xorshift32 with explicit state. Seeds are unsigned 32-bit integers; zero
maps to the fixed nonzero state `0x6d2b79f5`. Desired speeds are sampled uniformly within
the vehicle-type range. Scenario collections are sorted by ID, while geometric points,
route order and phase order are preserved. Stable IDs control simultaneous insertion.

`createSimulation` copies, canonicalizes and deeply freezes the scenario. Each step
returns a frozen state and does not mutate its argument. Config edits cannot change an
active run. Simulation time is integer tick times fixed timestep; there is no wall-clock,
framework or I/O access inside `core/`. The architecture test enforces this boundary,
including type imports, re-exports, dynamic imports and an intentionally bad-import test.

Same scenario, seed, engine version and JavaScript runtime reproduce the same event
stream. Transcendental arithmetic (`log`, `cos`) is not promised to be bit-identical across
different JS engines or future native ports. Persist the engine/runtime version alongside
the scenario and seed when reproducible report runs are introduced. The tests protect
current replay behaviour, not cross-version scientific reproducibility. The seed-42 demo
also has a committed SHA-256 event-stream fingerprint; intentional trajectory changes
must be reviewed and versioned rather than silently updating that reference.

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
use `runSimulation` to stream history without retaining all trajectory records in state.

## Evaluation and extension points

`eval/summary.ts` is a completed-trip diagnostic. Mean delay is actual travel time plus
source wait minus route length divided by sampled desired speed. It includes acceleration
loss; it is **not HCM control delay or LOS**. An empty run returns `null`, not NaN. Incomplete
trips do not enter this mean and must be reported separately.

Catalog content lives under `data/vehicle-types`, `data/driver-behaviour` and
`data/scenarios`. The compiled boundary allows editor/project work without importing its
types into the engine. Runtime indexes and native portability can be improved after
profiling; the current array searches and occupancy scans target a small M0 network.
