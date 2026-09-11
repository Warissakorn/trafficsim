# ARCHITECTURE — TrafficSim

The map of this codebase. Read before adding a system; update when the map changes.

> **Status: M0 core/network subset implemented.** The development harness and headless
> runner exercise the two systems end to end. Later systems remain planned; see their
> explicit status below. The M0 acceptance gate is still open.

---

## Shape

Five layers, and the dependency arrows only ever point downward.

```
  ┌──────────────────────────────────────────────────────────────┐
  │  shell/          window, panels, command palette, i18n       │
  │  editor/         network editor · signal editor · tables     │
  │  render/         the network view (GPU), isolated            │
  └───────────────────────────┬──────────────────────────────────┘
                              │  reads model, dispatches commands
  ┌───────────────────────────┴──────────────────────────────────┐
  │  commands/       every mutation, undoable, one registry      │
  │  model/          the network + demand + control data model   │
  │  project/        load, save, revisions, validation           │
  └───────────────────────────┬──────────────────────────────────┘
                              │  scenario in, events out
  ┌───────────────────────────┴──────────────────────────────────┐
  │  core/           THE SIMULATION ENGINE — no I/O, no UI       │
  │                  vehicles · car-following · lane change ·    │
  │                  gap acceptance · conflict areas · signals   │
  └───────────────────────────┬──────────────────────────────────┘
                              │  event stream
  ┌───────────────────────────┴──────────────────────────────────┐
  │  eval/           measurements → delay, LOS, queue, travel    │
  │                  time, per movement, averaged across seeds   │
  │  report/         tables and exports for impact studies       │
  └──────────────────────────────────────────────────────────────┘
```

A user action becomes a **command**, which mutates the **model**, which is validated by
**project** and drawn by **render**. Pressing Run hands an immutable snapshot of the model to
**core**, which produces an event stream that **eval** turns into the numbers **report**
formats. The engine never reads the model directly and never writes results directly — it
receives a scenario and emits events.

**Why the arrows point one way:** it is what lets the engine run headless in a test, in a
worker thread, and in a batch of ten seeds at once, without any of those three knowing about
each other. See PRINCIPLES rules 2 and 3.

---

## Systems

| System | Owns | Location | Talks to | Status |
|---|---|---|---|---|
| Simulation core | Vehicle state, fixed stepping, reduced longitudinal following, seeded arrivals, signals and event stream | `core/` | internal modules only (pure) | M0 subset implemented; lane changing, gap acceptance and conflict resolution remain planned |
| Network model | Links, connectors, lanes, signal heads, geometry and scenario compilation | `model/network` | core contracts | M0 subset implemented; conflict areas and priority rules remain planned |
| Demand model | Vehicle inputs, compositions, routing decisions, OD | `model/demand` | commands, project | editable model planned; M0 fixed routes and Poisson inputs use core contracts |
| Control model | Signal controllers, groups, programs, detectors | `model/control` | commands, project | editable model planned; M0 fixed-time programs use core contracts |
| Command registry | Every mutation as a named, undoable, serializable command | `commands/` | model, project | planned |
| Project | File format, load/save, revisions, undo stack, validation | `project/` | model, commands | planned |
| Renderer | Drawing network and vehicle positions; nothing else | `render/` | model and core snapshots (read-only) | M0 passive canvas implemented; production renderer planned |
| Editor | Drawing tools, inspector, tables, signal editor | `editor/` | commands, render | planned |
| Shell | Window layout, panels, palette, translations, theme | `shell/` | editor | English/Thai demo translations implemented; full shell planned |
| Evaluation | Turning the event stream into measurements | `eval/` | core output | completed-trip diagnostic implemented; movement delay/LOS planned |
| Runner | Running N seeds, aggregating, confidence intervals | `runner/` | core, eval | batch runner planned; single-run CLI in `tools/run-simulation.ts` |
| Report | Impact-study tables and export | `report/` | eval | planned |

---

## Interfaces that matter

Contracts other systems code against. Written **before** the implementations they describe,
because they are what lets a future session build against a system without reading its
insides. Core and network contracts are implemented; commands, evaluation measures and
batch interfaces below remain illustrative.

```ts
// core/ — the whole engine surface. Deliberately tiny.
createSimulation(scenario: Scenario, seed: number): SimState
runSimulation(scenario: Scenario, seed: number, opts?: RunOptions): EventStream
stepSimulation(state: SimState, dt?: number): SimState     // dt must equal scenario.timeStep
compileScenario(network: Network, definition: ScenarioDefinition): Scenario
validateNetwork(network: Network): NetworkIssue[]

// commands/ — every mutation goes through here; nothing bypasses it
applyCommand(model: Model, cmd: Command): CommandResult     // returns inverse for undo
registerCommand(kind: string, handler: CommandHandler): void

// eval/ — event stream in, measurements out
evaluate(events: EventStream, measures: MeasureSet): Measurements

// runner/ — the reason multi-seed exists at all
runBatch(scenario: Scenario, seeds: number[]): AggregateResult  // mean + CI per measure
```

**The command registry is a hard boundary.** Adding a new kind of edit must not require
touching `project/`. `project/` owns transactions, undo, and revisions; it must never learn
what any individual command means. Its enforcement test must land with that system.

The existing `tests/architecture.test.ts` enforces the core import boundary and rejects
wall clocks and unseeded RNG. `core/types.ts` is the runtime contract; the network compiler
depends on it, and the engine never imports the authoring model. `docs/SIMULATION.md`
documents supported topology, event timing, immutable state, safety guards and limitations.

---

## Data layer

Content lives as data, not code. The diagnostic from the skill applies directly: **if adding
the 50th vehicle type requires editing code, the boundary is wrong.**

- `data/vehicle-types/` — dimensions, acceleration, desired-speed distributions
- `data/driver-behaviour/` — car-following parameter sets (Wiedemann 74/99 presets)
- `data/link-types/` — link behaviour templates (urban, freeway, ramp, pedestrian)
- `data/los-tables/` — LOS thresholds; these are **jurisdiction-specific** and must be
  swappable per project, never compiled in
- `data/validation/` — model-checking rules

---

## Deliberate non-goals

Do not add these by reflex. See `PROBLEM.md` §5 and `PRINCIPLES.md` §2.

- No engine-abstraction layer — there is exactly one engine.
- No plugin system.
- No second persisted representation of the network.
- No UI types reachable from `core/`.
- No wall-clock or unordered iteration anywhere in `core/` or `eval/` (breaks reproducibility).
