# PRINCIPLES — rules that do not get relitigated

> These are commitments, not preferences. Breaking one is allowed, but only by editing this
> file first and recording why in `PROGRESS.md`. A rule silently broken in code is how a
> project loses its shape.
>
> Rules 1–4 are load-bearing for correctness. Rules 5–9 are load-bearing for the project
> surviving across sessions. Section 3 is inherited from a prior effort in this domain —
> those were measured, not guessed.

---

## 1. Hard rules

### 1. One source of truth for the model, and it is a file we own
The project file is the model. No second store that has to be kept in sync with the first.
If two places ever need the same value, the boundary is drawn wrong — **move it, do not
sync it.** Views (a link table, a movement list, a conflict matrix) are assembled on read
and never persisted as their own truth.

### 2. The simulation core knows nothing about the UI, the file format, or the network editor
`core/` imports no framework, no I/O, no rendering. It takes a scenario struct, runs, and
emits events. This is what makes it testable headless, runnable in a worker, reusable in a
batch runner, and portable to a native build. **The day the core imports a UI type, the
project has lost its most valuable property.**

### 3. Anything a report depends on must be reproducible from a seed
Same scenario + same seed + same engine/toolchain = the same trajectory. No wall-clock,
no unordered iteration, no floating-point that depends on thread scheduling. Preserve
engine, compiler and runtime versions with future report runs so results remain
reconstructible. D15 introduces explicit cross-language tolerance for the native port;
it does not promise bit-identical transcendental math across JS and C++ libraries.

### 4. Never claim fidelity we have not measured
No screen, document, or marketing sentence may suggest results match Vissim, or that a
Vissim-calibrated parameter carries over unchanged. Where a model is an approximation, the
screen that exposes it says so, permanently, not in a dismissible tooltip.

### 5. The build is never left red
A session that cannot get to green reverts to the last green commit and records why in
`PROGRESS.md`. A red build costs the next session an hour before it can do anything.

### 6. Files stay near ~500 lines
Checked with `trafficsim-check-file-sizes .` before committing. Split along the seam
already in the file (rendering vs. logic, one concept per file), never at an arbitrary
midpoint.

### 7. UI strings come from the translation files only
`en` is the key source of truth; other languages follow. Data-borne text (scenario names,
validation messages) is stored bilingual with the data and picked at render time. A test
enforces this by switching language and failing on leftover characters from the other one.

### 8. Terminology follows Vissim, with the internal name alongside
The audience thinks in Vissim vocabulary — "link", "connector", "conflict area", "desired
speed distribution". Screens use those words. **Parameter names are never translated**,
because users must be able to find them in the project file and in the docs.

### 9. Every session ends with the log written and the work committed
Not as cleanup afterward — as part of finishing. A system that works but is not recorded
will be rewritten by a future session that cannot see it.

---

## 2. Deliberately not done

Recording the *absence* of something is as important as recording its presence, otherwise
each new session re-proposes it.

| Not doing | Reason |
|---|---|
| An abstraction layer for "any simulation engine" | There is one engine. Speculative generality written blind is the most expensive kind. |
| A plugin system | Nobody has asked. Data-driven content covers the real need. |
| Our own charting/table/canvas library | Solved problems. Spend the effort on the model. |
| Server-side accounts, cloud storage, sync | Single-user desktop-shaped tool first. |
| Real-time collaborative editing | Needs its own architecture; not before the single-user tool is good. |
| Importing competitor project formats | See `PROBLEM.md` §5. |

---

## 3. Inherited discipline — measured, not guessed

These come from a prior effort in this exact domain. They were paid for once already.

**On rendering**
- A network view redraws **only when something changes.** Leaving an engine's ticker running
  costs full CPU on a static map. Measured: 1.8 FPS vs 60 FPS on the same scene.
- Level of detail is chosen by **on-screen density** (`count ÷ zoom²`), not by zoom alone.
- Line cap style is not cosmetic. Round caps on a real road network measured **6.7× slower**
  than butt caps — curve endpoints cost far more than corners.
- **Cost tracks pixels filled, not shapes drawn.** Drawing lane surfaces on top of road
  bodies fills the same pixels twice. Before adding an LOD budget, ask whether something is
  being painted twice.
- Self-host fonts. A CDN font measured first-contentful-paint at 12,676 ms vs 256 ms.

**On measuring**
- **Benchmark on a real network, never only a synthetic one.** Generated grids have ~2 points
  per link; real roads have ~7. Every cost that scales with geometry is invisible on synthetic
  data.
- Total-throughput measurements swing ±10% on a dev machine. To compare anything smaller,
  toggle the thing on and off **within one run** and take the median of the differences.
- **A number that improves when nobody changed anything means the instrument broke.** A prior
  benchmark reported 60 FPS for days after a gesture change made its simulated drag stop
  redrawing anything. **When an interaction changes, re-check that the benchmark still
  exercises what users actually do.**
- Never hard-code screen coordinates in a test. Panel state changes the canvas size; a click
  at a fixed point lands outside it and passes or fails at random. Compute from the real
  bounding box.

**On state**
- Split application state into narrow contexts with a stable actions object. Measured: a
  single monolithic store caused 189–240 DOM mutations per action in regions that did not
  change.

**On input**
- **Trust measured events, not remembered specifications.** On the browsers tested: a second
  mouse button pressed while another is held fires a move event carrying the button, not a
  down event; a right-click without a drag ends in cancel, not up; and the context-menu event
  fires at press time on several platforms. Suppress a menu by **which gesture is in
  progress**, never by how far the pointer moved.

**On process**
- **Never close a milestone that has not met its own gate.** Two milestones in the prior
  effort were declared done with work outstanding; both had to be reopened later at higher
  cost, and the roadmap stopped being trustworthy in the meantime.
- **Merged code is not a passed gate.** If the gate is a measurement with real users, only
  that measurement closes it.
- The test suite must exercise the path users actually take, including the development-mode
  path, not only the production build. A prior effort shipped a full-screen freeze in dev
  mode with 30 green checks.
