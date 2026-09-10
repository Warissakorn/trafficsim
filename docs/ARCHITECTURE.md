# ARCHITECTURE — Veytrix

The map of this codebase. Read before adding a system; update when the map changes.

> **Status: planned, not built.** Nothing in this repo is implemented yet. The Systems
> table below is the *intended* map and is marked as such. Per the skill this repo follows,
> a row moves out of "planned" only when the system exists. **Do not treat a planned row as
> evidence something is there.**

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

## Systems (all planned)

| System | Owns | Location | Talks to | Status |
|---|---|---|---|---|
| Simulation core | Vehicle state, time stepping, car-following, lane changing, gap acceptance, conflict resolution, signal state | `core/` | nothing (pure) | planned |
| Network model | Links, connectors, lanes, conflict areas, priority rules, signal heads | `model/network` | commands, project | planned |
| Demand model | Vehicle inputs, compositions, routing decisions, OD | `model/demand` | commands, project | planned |
| Control model | Signal controllers, groups, programs, detectors | `model/control` | commands, project | planned |
| Command registry | Every mutation as a named, undoable, serializable command | `commands/` | model, project | planned |
| Project | File format, load/save, revisions, undo stack, validation | `project/` | model, commands | planned |
| Renderer | Drawing the network; nothing else | `render/` | model (read-only) | planned |
| Editor | Drawing tools, inspector, tables, signal editor | `editor/` | commands, render | planned |
| Shell | Window layout, panels, palette, translations, theme | `shell/` | editor | planned |
| Evaluation | Turning the event stream into measurements | `eval/` | core output | planned |
| Runner | Running N seeds, aggregating, confidence intervals | `runner/` | core, eval | planned |
| Report | Impact-study tables and export | `report/` | eval | planned |

---

## Interfaces that matter

Contracts other systems code against. Written **before** the implementations they describe,
because they are what lets a future session build against a system without reading its
insides. Shapes are illustrative until the first version lands.

```ts
// core/ — the whole engine surface. Deliberately tiny.
runSimulation(scenario: Scenario, seed: number, opts: RunOptions): EventStream
stepSimulation(state: SimState, dt: number): SimState      // pure, for tests

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
what any individual command means. A test enforces this.

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
