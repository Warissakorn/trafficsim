# Multi-level modelling — Micro, Meso and Macro (design, 2026-10-09)

The owner asked for a program that models traffic at **three levels — microscopic, mesoscopic and
macroscopic** — with a **full four-step Macro model**, parking lots and dynamic assignment, chosen
by what studies need from it ([D145](../decisions/RECORD.md#d145)). This document is the design:
what each level is for, what it must output, how it fits the existing boundaries, and the
milestones that build it. It is a plan, not evidence. [ROADMAP](../ROADMAP.md) owns the milestone
scopes and gates (M8–M12); [NEXT](../NEXT.md) owns the queue. The new milestones start **after
the D143 queue** (M5.9 → M4.2 → M6.0 → M5.8). Nothing here is implemented.

## 1. Why three levels

[PROBLEM](../PROBLEM.md) §1 describes an engineer who studies one intersection. Real impact work
asks three questions at three scales, and today each needs a different tool:

| Question the study asks | Scale | Level | Typical deliverable |
|---|---|---|---|
| How many trips will the development generate, where will they go, by which mode and which road? | City / region, hundreds of zones | **Macro** (four-step) | Trip tables, link volumes, V/C, mode shares, select-link |
| How does a corridor behave over the peak — where do queues form, spill back and clear? | Corridor / subnetwork, tens of junctions | **Meso** | Link travel times by time slice, queue and spill-back maps, network VHT/VKT/delay |
| What is the delay, LOS and queue of each movement at this junction? | Junction / short corridor | **Micro** (exists) | Movement delay/LOS/queue table with 95 % CI (M5) |

A TIA in practice chains them: the regional model distributes site trips, a corridor model checks
whether the network can carry them over time, and the intersection model produces the report
table. One program that keeps **one network and one demand** across the chain removes the
re-entry and the unsynchronised copies that make that chain error-prone. That is the case for
D145, and it is also its main risk (§9).

## 2. Study → output matrix

What each study type needs, against what exists on 2026-10-09. "Partial" names the gap.

| Output | Study types | Level | Today |
|---|---|---|---|
| Movement delay, LOS, queue, 95 % CI | Intersection, signal retiming, TIA | Micro | **Exists** (M5.2–M5.6), unvalidated |
| Approach and intersection LOS | Intersection, TIA | Micro | **Exists** (M5.5), section-bounded |
| Node (junction) aggregation of all movements | Intersection, corridor | Micro, Meso | Missing ([VISSIM_PARITY](../audits/VISSIM_PARITY.md) §6) |
| Network totals: VHT, VKT, total delay, stops, mean speed | Corridor, scenario comparison | All | Missing |
| Scenario comparison with Welch CI | Every study (base / with-project / mitigated) | Micro first | Booked (M5.8) |
| Car-park occupancy over time, gate queue, spill-back, waiting arrivals | Development access TIA | Micro, Meso | Missing — M8 |
| Path travel times and route shares under congestion | Corridor, network change, diversion | Micro, Meso | Partial: fixed weights only (D119) — M9 |
| Link volume, V/C, LOS by V/C, select-link, select-zone | Regional, TIA trip distribution | Macro | Missing — M10 |
| Trip generation, OD table, mode shares, skims | Regional, TIA | Macro | Missing — M10 |
| Link travel time and queue by time slice; spill-back map | Corridor, incident, work zone | Meso | Missing — M11 |
| Subarea OD cut from a regional model | Any study fed by a regional model | Macro → Meso/Micro | Missing — M12 |
| Emissions / fuel | EIA | All | Missing; not booked (needs a cited model, after M6) |
| Bus stops, transit lines, pedestrians | Multimodal | Micro | *Later* (D143 R8), unchanged |

## 3. Design decisions

### 3.1 One authoring network, three compiled views

The editor document stays the **only** source of truth (hard rule 3). Each level gets a compiler,
never a second editable network:

```text
                      authoring document (*.traffic.json)
          Links · Connectors · controls · Zones · OD matrices · Parking lots
                                    │
       ┌────────────────────────────┼─────────────────────────────┐
compileScenario (exists)    compileMesoNetwork (M11)      compileMacroNetwork (M10)
       │                            │                             │
  core Scenario             meso::Network                  macro::Network
 (lanes, stations)    (link queues, turn capacities)   (graph, capacity, VDF)
```

What a coarser view needs and the network does not hold yet is an **authored attribute** with a
stated default, never a parallel table: a Link gains `linkType` (data-driven: capacity per lane,
free speed, VDF, Meso jam density), and a junction gains its Node (M5.1's evaluation node, reused
as the Macro/Meso node). Values derivable from geometry (length, lane count, turns from
Connectors) are derived, never stored twice.

A Macro study may cover far more road than anyone draws lane by lane. The same Link object
therefore allows a **centreline-only** form (one polyline, lane count, link type) that the Macro
and Meso compilers accept and the Micro compiler refuses with a validation issue naming the Link.
The Micro area of a model is where Links carry lane geometry and Connectors.

### 3.2 Shared demand layer: Zones and OD matrices (M8)

New authoring objects, shared by every level:

```text
Zone          { id, name, polygon, attributes{ population, jobs, ... } }   // attributes as data
ZoneConnector { id, zone, link, position, direction (in/out/both), share }
OdMatrix      { id, period (start, end), demandClass (mode/vehicle class), zones[], cells[][] }
ParkingLot    { id, zone, access ZoneConnector(s), capacity, initialOccupancy,
                dwellDistribution (data ref), gateServiceTime }
```

- **Micro and Meso** consume an OdMatrix by expanding it into the inputs and routes they already
  run: each origin's ZoneConnectors become sources, each OD pair a route family through the
  routing machinery of [POSITIONED_ROUTING](../reference/POSITIONED_ROUTING.md). Path choice is
  fixed weights (M8) or assignment (M9).
- **Macro** consumes the matrix directly.
- Existing Link inputs, Routes and Routing decisions remain valid. Schema bumps; older files load
  unchanged and keep their bytes.

### 3.3 Parking lots are zones with capacity (M8)

A lot is the origin and destination of site trips. Runtime contract (to be written as M8.1):
an arriving vehicle enters if occupancy < capacity after the gate service; otherwise it **waits at
the gate**, and its queue spills back through ordinary following — **a vehicle never vanishes**.
Departures are drawn from the dwell distribution with the run's seeded RNG and leave through the
existing source insertion guard (D108). Outputs: occupancy time series, peak and mean occupancy,
gate queue length and wait, number of arrivals that waited, in/out flows per interval. Bays,
parking search and on-street manoeuvres blocking a lane are out of the first scope.

### 3.4 Three pure engines

Each engine obeys hard rules 1 and 2: standard C++ only, no I/O, no wall clock, no unordered
iteration, no thread-dependent floating point. `check_architecture` gains a rule and a negative
fixture per module, as M5.2 did for the runner.

| Module | Model | Inputs | Outputs |
|---|---|---|---|
| `src/core/` (exists) | Car-following, lane change, conflicts, signals | Scenario, seed | Vehicle events → `src/eval/` |
| `src/meso/` (M11) | Event-based link queue model: per lane group a free-flow travel time, storage capacity and discharge capacity; signals and priority as capacity windows; spill-back when storage is full. The exact formulation (link-transmission model vs queue-server) is an owner decision (§8) | meso::Network, demand paths, seed | Link entry/exit events → flows, travel times, queues by time slice |
| `src/macro/` (M10) | Four-step, below | macro::Network, zones, data parameters | Trip tables, link volumes, skims, convergence |

**Four-step Macro model** (all parameters are data under `data/`, hard rule 5):

1. **Trip generation** — productions and attractions per zone from zone attributes and trip-rate
   tables (`data/trip-rates/`), per purpose; attraction totals balanced to productions.
2. **Trip distribution** — doubly-constrained gravity model, deterrence function of the skim
   (exponential, power or combined; parameters in data), Furness balancing to a stated tolerance
   and iteration cap; non-convergence reported.
3. **Mode choice** — multinomial logit over the modes the project defines, utilities from skims
   and coefficients in data. Modes are data; the motorcycle question (PROBLEM §8, R3) decides
   whether motorcycles are a mode here and a vehicle class in Micro.
4. **Assignment** — static user equilibrium for each road mode, Frank–Wolfe first (bi-conjugate
   later as a measured speed-up), volume-delay functions (BPR and conical) in data; stopping
   criterion is a relative gap stated in the run; result carries the gap reached.

Iteration orders are fixed (zone index, then link index); ties are broken by ID. Same project +
data + parameters ⇒ the same bytes. The Macro model is deterministic and has no seed.

### 3.5 Dynamic assignment for Micro and Meso (M9, reused by M11)

An iterative **between-run** loop, in a runner layer, so the engines stay unchanged:

1. Iteration 0 uses free-flow path costs (or a supplied cost file).
2. Run the seeds; measure each path's travel time per interval from the existing sections and
   movement observers (`src/eval/sections.*`, `movement.*`).
3. New weights = logit (scale θ) or Kirchhoff over smoothed costs (method of successive
   averages); write them as the RouteDecision weights for iteration k+1.
4. Stop at a stated convergence criterion (maximum relative change of path times, or a gap) or
   an iteration cap; report the convergence table; flag non-convergence, never hide it.

Costs live in a separate `*.paths.json` beside the project, with the revision and iteration they
came from. Same project + seeds + iteration count + cost file ⇒ the same bytes. En-route
rerouting (a vehicle changing path mid-trip on current conditions) needs a core contract and is a
later slice, not part of M9.

### 3.6 Multi-resolution workflow (M12)

**Subarea cut:** the engineer selects an area of the Macro model; for each period the program
extracts the subarea OD from the assigned paths that cross it, creating gate zones where paths
enter and leave. The cut matrix is **derived data with provenance** (source revision, assignment
gap, period), regenerated rather than edited, and the Meso/Micro run records which cut it used.
The same subarea can be run in Meso for the corridor and in Micro for the junctions inside it.
Consistency checks between levels (e.g. Micro throughput vs the Macro link volume it was fed) are
reported, not forced.

### 3.7 Outputs and the validation marker

Every level's results reuse the M5.6 Results surface and the shared CSV/TSV formatters
(`src/project/batch_output.*`, `csv_format.hpp`). Macro adds link and matrix tables; Meso adds
time-sliced link tables; Micro adds network totals and node aggregation. **Every result of every
level carries the not-yet-validated marker** until that level's M6 benchmark passes (hard rule 4).
M6 gains benchmark kinds per level, chosen without engine numbers as M6.0 requires: Macro — test
networks with published equilibrium solutions; Meso — analytic queue and shockwave cases; Micro —
as today.

## 4. Module and dependency sketch

```text
src/core/   Micro engine            imports nothing
src/meso/   Meso engine             imports nothing                       (M11)
src/macro/  Four-step model         imports nothing                       (M10)
src/eval/   observers               core (and meso event types, M11)
src/runner/ seeds, DTA loop         core, eval, meso                      (M9, M11)
src/model/  authoring + compilers   core contracts; meso/macro network types
src/project/ load/save, outputs     model, eval, runner, macro; nlohmann/json
src/shell/  editor, Results         everything above
```

The Macro solver needs no simulation runner; `src/project/` calls it directly and formats its
results. Long Macro runs execute on the M5.6 worker-thread pattern (D141): value copies in,
results out, progress and cancel.

## 5. Milestones

All after the D143 queue. One system per session, interface and acceptance rows first.

| Milestone | Slices | Done when |
|---|---|---|
| **M8 — Zones, OD matrices and parking lots** | M8.1 contract + rows; M8.2 schema/codec/commands for Zone, ZoneConnector, OdMatrix; M8.3 OD → Micro inputs/routes; M8.4 ParkingLot runtime (capacity, gate, dwell); M8.5 lot outputs; M8.6 editor tools (`Z`, `P`), OD matrix table | A development-access study runs from a zone OD matrix through a capacity-limited lot; a full lot produces a gate queue and never deletes a vehicle; older files load byte-identical |
| **M9 — Dynamic assignment (Micro)** | M9.1 contract + analytic two-route equilibrium fixture; M9.2 path cost extraction; M9.3 runner loop + CLI `--assign N`; M9.4 Results/Export of path flows and convergence | Two-route fixture converges to the analytic split within the stated tolerance; same inputs ⇒ same bytes; non-convergence flagged |
| **M10 — Macro four-step model** | M10.1 `compileMacroNetwork` + link types + VDF data + UE assignment; M10.2 generation; M10.3 distribution; M10.4 mode choice; M10.5 skims, select-link, Results/Export; M10.6 centreline-only Links in the editor | Assignment reproduces a published test network's equilibrium within a stated gap; each step has a hand-computed fixture; parameters swap with no code edit |
| **M11 — Meso engine** | M11.1 model choice + contract + analytic queue/shockwave rows; M11.2 `compileMesoNetwork`; M11.3 engine; M11.4 eval and Results; M11.5 DTA reuse from M9 | Analytic fixtures pass; spill-back blocks the upstream link at storage capacity; same seed ⇒ same bytes |
| **M12 — Subarea workflow** | M12.1 contract; M12.2 cut extraction with provenance; M12.3 Meso/Micro from a cut; M12.4 cross-level consistency report | A cut from M10 runs in M11 and in Micro with recorded provenance and a reported volume comparison |

## 6. What stays out

Importing Visum/Vissim files, bit-exact parity with either, activity-based demand, transit
assignment and pedestrian modelling (still *Later*), 3D, collaboration. A Macro model is for the
study's region, not a national model.

## 7. Order and cost

Micro remains the first deliverable: the M5 table is what a study is paid for today, and the
D143 queue corrects its known biases. M8 comes first among the new milestones because zones and
OD feed every level and lots answer the most common TIA question. M9 reuses Micro and needs no new
engine. M10 is the largest new code but analytically testable. M11 needs M9's loop and M10's link
types. M12 needs all of them.

## 8. Owner decisions still open

1. Meso formulation: link-transmission model (cell/kinematic-wave based) or queue-server.
2. Default VDF (BPR α = 0.15, β = 4 or a local function) and where its citation comes from.
3. Deterrence function form and default trip purposes for distribution.
4. The mode set — car, motorcycle, bus, others — decided with PROBLEM §8 (R3).
5. Default convergence criteria: Macro relative gap, Furness tolerance, DTA iterations.
6. Whether on-street parking that blocks a lane joins M8 or stays later.

## 9. Risks

- **Effort starving the interface** ([PROBLEM](../PROBLEM.md) §7.3). Each level must ship with its
  editor surface and Results, or it is an engine nobody can use. Every milestone gate includes the
  editor and output slices.
- **Unvalidated numbers at a larger scale.** A Macro volume table looks authoritative. The
  marker rule applies to every level; M6 gains per-level benchmarks.
- **Scope creep inside a level.** Each milestone lists what it excludes; extensions get their own
  slice and contract.
- **Duplicated network data.** Any attribute two levels need lives once on the authoring object
  (hard rule 3); a compiler that needs a value the document lacks adds an authored attribute with a
  default, never a side table.
