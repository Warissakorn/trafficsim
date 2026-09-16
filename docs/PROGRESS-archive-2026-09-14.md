# PROGRESS archive — 2026-09-14

Entries moved whole from [PROGRESS.md](PROGRESS.md) to keep the active log below 500 lines.

### 2026-09-14 — hot path 1/3: route geometry resolved once per run

`routeParts` was recomputed for every vehicle on every tick — 1,867,548 calls in an
8-corridor profile — even though it is a pure function of an immutable `Scenario`. A
`ScenarioIndex` now resolves it once in `createSimulation` and is carried through `SimState`
as a `shared_ptr`, so per-tick state copies share it rather than duplicating it. Routes live
in a contiguous vector, so `partsFor` recovers a route's index from its own address in O(1)
with no extra lookup. The index is built from the **canonical** scenario, so part order
matches the sorted routes.

The uncached `routeParts`, `locateVehicle` and `occupiedSpans` overloads are retained for
`src/render/` and the existing tests; cached and uncached paths share one implementation each
so they cannot drift. `stepSimulation` tolerates a hand-built state without an index by
building one, rather than requiring every caller to change.

**Measured** (Release, GCC 13.3, median of 3, identical commands as the baseline entry above):
600 s of simulation on 1/2/4/8/16/32 corridors went 0.067/0.142/0.488/1.739/4.876/17.724 s to
0.036/0.079/0.285/1.157/3.257/11.327 s, i.e. **-33% to -46%, -36% at 466 vehicles**. The
O(V^1.85) growth is unchanged and is deliberately left to the next slice; this change removes
constant work per call, not the quadratic term.

No behaviour change was intended and none was observed: 12/12 headless CTest including the new
trajectory digest and the exact `29.24935` CLI pin, plus 12 multi-seed multi-size CLI runs
(4 network sizes x 3 seeds) byte-identical to the pre-change binary.

**Verification:** headless preset only. Qt is absent in this container, so the three desktop
suites were not built or run; `src/render/` compiles against the unchanged overloads but no
desktop verification is claimed.

---

### 2026-09-14 — core hot-path optimization: measurement baseline and trajectory guard

Profiling (Release, GCC 13.3, callgrind) of a synthetic multi-corridor scenario shows engine
cost growing at **O(V^1.85)** in active vehicle count: 466 vehicles take 17.7 s of wall time
for 600 s of simulation. Attribution: `__memcmp_avx2_movbe` 30.9% of all instructions (linear
`detail::byId` searches over `std::string` IDs), `closestVehicle` 39.3% inclusive (nested
`parts x spans` scan, the quadratic term), `routeParts` ~35% inclusive over **1,867,548 calls**
recomputing a value that is constant for an entire run.

Before changing any engine code, per-tick trajectory is now pinned. The four frozen TypeScript
baselines deliberately exclude `MovedEvent`, so positions between the every-100-tick checkpoints
were unguarded. `tests/reference/trajectory-digest.json` records weighted means over the full
`MovedEvent` stream for the same four seeds; means (not sums) keep magnitudes physical so the
existing 1e-7 tolerance applies unchanged, and order/segment/vehicle weights make a reordering
visible that plain sums would hide. The four TypeScript baselines were **not** touched.

The guard was verified non-vacuous: perturbing only the reported position in `locateVehicle`
by 1e-6 relative — which changes `MovedEvent` but not checkpointed `distance` — fails
`meanOrderWeightedPosition` on all four seeds. A 1e-9 relative perturbation of acceleration is
caught by the pre-existing checkpoint comparison. Both perturbations were reverted.

These digests characterize what the engine currently does. They are not a fidelity claim and
do not affect the not-yet-validated marker or any milestone gate.

**Verification:** headless preset, 12/12 CTest. Qt is not installed in this container, so the
three desktop suites were not built or run and no desktop verification is claimed.

---

### 2026-09-14 — CI packaging workflow for testable binaries

Added `.github/workflows/package.yml`, a manually dispatched (`workflow_dispatch`) and
`v*`-tag workflow that builds, tests and uploads runnable binaries so the owner can try a
build without a local toolchain. Linux uses the `release` preset with apt Qt 6 and
`ctest --preset release` under the offscreen platform; Windows uses MSVC 2022, vcpkg
nlohmann/json and an aqt-installed Qt 6.5.3, then `windeployqt` so the archive runs on a
clean machine. Both stage the existing `install()` rules into `dist/` (desktop, CLI, data
catalogs) and add a `RUN.txt` that repeats the not-yet-validated marker.

Existing `native.yml` push/PR verification is unchanged; packaging is deliberately a
separate workflow so a slow Qt install never sits in the pull-request path. These are
unsigned test builds — installer work still belongs to M7, and no milestone gate is
affected. No engine, model or UI code changed.

**Verification:** workflow YAML parsed locally; the build itself is proven by the CI run,
not by this container, which has neither Qt nor nlohmann/json installed.

---

### 2026-09-14 — MIT License added

Added a top-level `LICENSE` (MIT, copyright 2026 Warissakorn) and a README License section.
The bundled Noto Sans Thai font keeps its SIL OFL 1.1 terms and Qt keeps its own; the MIT
grant covers this repository's own source and documentation only. No code change.

---

### 2026-09-14 — M1.4 Connector editor implemented (D17)

Added general lane-to-lane creation using two canvas endpoint clicks or Properties.
Source/target markers, hover previews and cancellation are transient; committing creates
one History entry. Connectors can be selected on the canvas or by ID (including short
split connectors), reshaped by dragging/inserting/removing interior points, reset to a
lane-aligned sampled curve, or made straight. Endpoints remain attached to their lanes.
Properties now has Links, Connectors and Image tabs, with English/Thai controls.

Connector commands share endpoint maintenance with Link/Lane/driving-side edits and
route/input cleanup with link deletion. Retargeting preserves an unreferenced curve's
interior points by weighted displacement; referenced retargeting is rejected. Duplicate,
invalid and failed edits preserve revision, ID allocation, saved state and Redo. Confirmed
connector deletion restores related routes/inputs together on Undo. The existing format
persists exactly the edited polyline and IDs; no new schema, Qt dependency in the model,
simulation physics, demand or right-of-way behaviour was introduced.

**Verification:** GCC 13.3, Qt 6.4.2, nlohmann/json 3.11.3, CMake 3.28.3 on Linux.
The unchanged base first passed all 13 desktop CTest suites. The extended Debug and
Release desktop builds pass 15/15, and the independent Qt-free build passes 12/12.
There are 46 named native cases, including eight new Connector cases. UI workflows
exercise real endpoint picking, curve drags, insert/delete, cancellation and locked
endpoints, plus ID selection, Properties actions, reference-safe deletion/Undo, Unicode
save/reopen and Thai errors. Four TS baselines, seeded replay and M0 controls still pass.
Architecture/negative fixtures, the 500-line budget and whitespace checks pass. The Thai
Connector tab and curve were visually inspected at 1000×760.

M1.3.1 remains open, along with M1.5–M1.7 and the owner's M0/M1 acceptance gates.
These are local Linux results; Windows/macOS GUI execution and hosted CI are not
established by them. Curves are editable sampled polylines, not swept-path validation.

---

### 2026-09-14 — Scenario/project file-kind confusion, and the Vissim parity review

**Reported:** opening `network.traffic.json` in the simulation window failed with
`Could not open this scenario. … [json.exception.type_error.304] cannot use at() with null`.

**Cause, not a corrupt file.** `documentJson` always writes `definition`, and a network drawn
from scratch has none, so every such project saves `"definition": null` — correct for a
project. The simulation window's Open filter was `*.json`, which listed the editor's own
default save name `network.traffic.json`; `loadScenario` then called
`parseDefinition(value.at("definition"))` unguarded, landing on `.at("duration")` on a null.
The raw nlohmann text reached the user because the handler appended `e.what()` verbatim.

**Changed, in outcomes.** Picking an editor project in the simulation window now says what
kind of file it is, in English or Thai, and offers **Open in Network Editor** — one click and
the drawing opens in the window that can hold it. No load path can surface an nlohmann
exception any more: `loadScenario` classifies the file before reading a field
(`SCENARIO_IS_PROJECT`, `SCENARIO_NO_DEFINITION`, `SCENARIO_NO_NETWORK`,
`SCENARIO_NOT_JSON_OBJECT`, `SCENARIO_FILE_READ`, carried on a typed `ScenarioLoadError`), and
`parseDocument` guards the mirror-image holes a hand-edited project could hit
(`EDIT_NO_NETWORK`, `EDIT_BACKGROUND_INVALID`, and null `format`/`nextId`/`revision`). File
dialogs default to `*.traffic.json` for projects. Two new tests pin the reported shape itself:
a saved empty project must be *recognised*, not parsed and rejected.

**Second pass, same day.** A scrutiny round found the classifier had the same defect it was
added to prevent: `value.value("format", std::string{})` throws `type_error.302` when the key is
present but not a string — including null — so `{"network":{…},"format":null}` still leaked an
nlohmann message. Fixed, and `TEST(project, file_kind_classification_survives_broken_metadata)`
pins it (verified to fail against the previous classifier). A second finding: `what()` on a
classification failure is the bare code, and two paths showed it untranslated — startup
`--scenario` via `main.cpp`, and the editor-launch fallback. Both now route through one
`MainWindow::explain` / `EditorWindow::openFileOrReport`, and a `--scenario` the simulation
window cannot run is explained **in** the window, with the editor offered, instead of a fatal
modal carrying an identifier.

**Also:** `docs/VISSIM_PARITY.md` reviews the editor against Vissim — hand motions, keyboard,
window layout, objects, and the run/output story — and ranks the gaps. The three the owner
accepted are carved into `ROADMAP.md` as **M1.8** (Run inside the editor), **M1.9** (network
objects sidebar, Vissim gestures and shortcuts) and **M1.10** (levels and display types).
No milestone was closed and no editor feature work was done: 17/17 CTest green, 59/59 native.

---

### 2026-09-13 — native migration and editor branches integrated into `main`

Merged `codex/cpp-desktop-migration` (D15) and `codex/network-editor-m1-1-3` (D16) into
`main` as two explicit merge commits. The migration commit is an ancestor of the editor
commit, so both branches shared one merge base at the last TypeScript commit `70383db`
and neither merge produced a conflict. No source or documentation was edited to make the
integration succeed; `main` now carries the C++20/CMake/Qt tree exactly as reviewed on
the branches.

Verified on the `headless` configuration only: full build clean, CTest **11/11 passing**,
including the architecture boundary, its negative fixtures, file sizes and the CLI checks.
**The Qt desktop harness and `editor_ui_tests` were not built or run** — no Qt in the
integration environment — so no desktop verification is claimed, per `docs/BUILDING.md`.
Building also required `nlohmann-json3-dev`, which a clean checkout must install first.

Neither the M0 acceptance gate nor the M1 gate is closed by this merge; merged code is
not a passed gate. `Next` is unchanged apart from its base note.

---

### 2026-09-12 — native editor M1.1–M1.3 implemented (D16)

Added a version-1 ProjectDocument and a Qt-free command library. Every committed edit
validates a candidate before publishing it; a failed edit preserves both history and
the model. Undo/Redo keeps up to 100 document snapshots, persists the current revision
and ID counter in project files, and tracks the last saved revision. Embedded PNG data
is shared immutably between snapshots rather than copied for every gesture.

The independent Qt editor is available from the M0 window or `--editor`. It provides
metric grid/snap, pan/zoom/fit, single-link selection, point/link dragging, point insertion
and removal, lane count and individual widths, left/right driving side, opposite
carriageways, split links and an extra downstream pocket lane. Splits introduce a 0.2 m
continuity span with explicit lane connectors and remap existing routes. A link deletion
confirms its affected connectors/heads/routes/inputs and restores all of them on Undo.
Referenced lane removal is rejected. Connector endpoints reanchor on geometry edits.

Local background images are embedded, calibrated from two picked points and a known
real distance, positioned/rotated/scaled and given opacity. All background changes are
undoable. Basic Open/Save/Save As use a versioned JSON document and QSaveFile atomic
replacement; failed load/save preserves the current work. New/Open/Close ask about
unsaved changes. Native prompts and editor controls support English and Thai. The
inspector can be hidden, resized or detached. No new simulation behaviour was added.

Boundary enforcement now also rejects project-to-command/editor/Qt imports, with
negative fixtures. `docs/NETWORK_EDITOR.md` records operation, format and exact limits.
M1.3.1 explicitly tracks the unsupported signal-bearing-link split rather than moving
signal stationing silently. General connectors, tables, recovery and run handoff remain
M1.4–M1.7. Windows/macOS GUI execution and the owner's usability gate remain unverified.

**Verification:** GCC 13.3 / Qt 6.4.2 on Linux. Fresh isolated Debug and Release
builds passed all 13 CTest suites; a separate Qt-free build passed all 11 suites.
The native test executable contains 38 named cases (8 new editor model cases), and
Qt UI checks exercise actual mouse/keyboard drawing, dragging, insertion/removal,
pan/zoom, cancellation, per-lane widths, pockets, opposite carriageways, driving side,
confirmed deletion/Undo, image transforms/two-point calibration, Unicode paths,
failed save/load preservation, unsaved-work cancellation and Thai translation.
The existing four TS regression fixtures, deterministic core replay, CLI and M0
controls still pass. Architecture/negative checks, the 500-line limit and whitespace
checks pass. A Thai editor screenshot at 1000×760 was visually inspected. GUI behaviour
on Windows/macOS and GitHub-hosted runner execution are not claimed verified here.

---

### 2026-09-11 — native C++ migration implemented (D15)

Replaced the active TypeScript/Vite application with C++20 libraries for core, network,
scenario loading and evaluation, a native CLI, and a Qt 6 Widgets desktop harness.
CMake presets cover desktop, headless and Release. All executable developer checks
are now C++; JSON remains the catalog/locale/fixture format. The original application
is preserved at GitHub commit `70383db6ab884c718baef97a8ab81292fdc9d1b0`.

The port preserves fixed ticks, explicitly sequenced xorshift32 draws, canonical IDs,
source queues, upstream tails, red/amber stops and all unsupported-topology guards.
`SimState` is a value snapshot sharing a detached const scenario. Qt, JSON and I/O stay
outside the core. CLI diagnostics include engine/compiler versions, unfinished counts
and optional JSONL events. M0 fixture loading is read-only, not project persistence.

The Qt harness supports Run/Pause/Step/Reset, seed validation/reset, playback speed,
scenario loading and English/Thai switching. Bundled Noto Sans Thai (unmodified OFL 1.1
font with license) fixes missing Thai glyphs on minimal systems. Desktop file-dialog
paths use native wide paths on Windows. Manual screenshot inspection confirmed Thai
text and a queued crossing scene at 640 pixels wide.

**Verification:** GCC 13.3, Qt 6.4.2, nlohmann/json 3.12.0, CMake 4.4.3 on Linux.
The original 40 tests and production build passed before capture. Native tests include
30 named C++ cases, four frozen TS baseline seeds, full same-build event replay, core
and network safety/validation, strict JSON/seed handling and locale key agreement.
Debug and Release desktop builds passed all 11 CTest suites, including interactive
control actions and an entire desktop run matching CLI/baseline. Headless also built
and passed independently without Qt. Address/undefined-behaviour sanitizer tests passed;
LeakSanitizer was disabled because this container cannot inspect process tasks.
The architecture negative fixtures, 500-line check and `git diff --check` passed.
An installed CLI run from a different working directory found its adjacent data and
reproduced seed 42: 31 completed, 0 active, 0 pending, 0 safety clamps and mean delay
29.249359418430977 seconds. No performance or scientific fidelity claim is made.

Added GitHub Actions definitions for Linux desktop/headless/Release and Windows MSVC
headless builds. Windows desktop execution and macOS deployment have not been tested
in this Linux workspace. See `docs/BUILDING.md`, `docs/MIGRATION.md`, updated architecture,
simulation contracts and `tools/README.md` for setup and precise limitations.

**Status:** M0.1 technical migration checks passed on Linux. Owner M0 plausibility
acceptance remains open. No M1 editor, movement LOS, right-of-way model, calibration
or M7 installer is claimed complete. Next remains owner review, then one undoable link.

---

### 2026-09-11 — M0 simulation core and network model implemented

The repository previously contained documentation only. Added a strict TypeScript/Vite/
Vitest toolchain and lockfile, the directory skeleton, and an AST-based core dependency
guard that is tested against intentionally invalid imports.

**Network:** link/lane/connector authoring types; left/right driving-side lane geometry;
mid-link signal heads; geometry/reference/range validation; and a detached scenario
compiler. Junctions are not authored, and no second persisted network format was added.

**Core:** fixed timestep; explicit xorshift32 seed state; immutable snapshots and pure
steps; Poisson source arrivals with persistent external queues; reduced four-regime
following; fixed-time red/amber/green signals; route transitions with residual distance;
upstream vehicle-tail occupancy; and a streaming event interface. The final subinterval's
arrivals remain pending instead of disappearing at the run horizon.

**Integration:** a crossing scenario and vehicle/behaviour catalogs in data files; a
passive canvas harness with run/pause/step/reset, playback speed, seed reset and English/
Thai text; a headless CLI; and an explicitly unvalidated completed-trip delay diagnostic.
The engine still has no UI, model, I/O or wall-clock imports.

**Verification:** 40 automated tests cover replay (including a reference trajectory
fingerprint), pure stepping, source queues, conservation, signal timing, free acceleration,
red stops/green discharge, upstream tails, short connectors, invalid scenarios, authoring
geometry and the import boundary. Production type checking/build and the source-size
check pass. A Chromium 152 browser smoke test passed dev boot, single-step, run/pause,
seed reset, Thai translation, an entire run matching the headless output, invalid-seed
handling and a 375-pixel viewport with no horizontal overflow. No page errors occurred.
In a separate clean detached checkout, `npm ci --offline` (using the package cache),
all 40 tests, the production build, the headless example and file-size checks also passed.

**Reference run:** seed 42, 180 simulated seconds, 31 completed trips, 0 active and 0
pending at the horizon, 0 numerical safety clamps. Mean completed-trip delay is
29.249359418430977 s (includes source wait and acceleration, not HCM control delay).
This is a reproducibility fixture, not a capacity or fidelity benchmark.

**Limits remain explicit:** no lane changing, merge arbitration, geometric crossing-conflict
resolution, priority rules, full W74/W99, project persistence, movement LOS or batch
aggregation. Merges, internal inputs and repeated-route segments fail validation.
M0 remains open for the owner's plausibility acceptance. No later milestone was closed.

See D12–D14 and `docs/SIMULATION.md` for the reasoning and precise interfaces.

---

---

### 2026-09-11 — reverted to the working name TrafficSim, naming deferred (D11)

Three renames in two days with no code written. The owner called it: go back to the working
name and decide the real one once the program has shape.

Headings across `CLAUDE.md`, `README.md`, `ARCHITECTURE.md`, `ROADMAP.md` and this file are
back to `TrafficSim`, now explicitly marked as a working name so no future session reads it
as settled. D9 and D10 keep their full reasoning and are marked superseded — this file is
append-only, and the collision findings gathered over those rounds are the main thing worth
keeping from them, so they are consolidated into the D11 row. The next naming round starts
from evidence, not from zero.

**Two pieces of queued work are cancelled, not postponed:** the GitHub repository rename (the
repo is still `Warissakorn/trafficsim` and the remote already points there, so there is
nothing to do) and the npm/PyPI/domain registrations for `velk`.

**Two defects in this file were found and fixed while making this change**, both introduced by
earlier sessions of this conversation:

1. **The D10 log entry below was never actually written.** The edit that should have added it
   matched no text, and the guard around that edit only checked that *something* in the file
   had changed — which was true because other edits in the same batch succeeded. It has been
   reconstructed below from the commit message and the D10 row. Guards on edits to this file
   now assert an exact match count per edit.
2. **Entries were in oldest-first order**, contradicting this file's own header. Reordered
   newest-first. No entry text was altered.

Nothing about scope, architecture or the roadmap changed. D1–D8 stand.

---

### 2026-09-11 — renamed to Velk (D10)

*Reconstructed on 2026-09-11 — see defect 1 in the entry above.*

`Veytrix` replaced throughout the documentation. `velk` verified free on npm and PyPI with no
brand or company found using it; `velk.dev` and `velk.app` free, `velk.com` and `velk.io`
held — ordinary for a four-letter word and irrelevant to a repository or package name, so
accepted as a known risk.

`MicroFlow Simulator` was proposed first this session and dropped after its collision check:
`microflow` taken on npm and PyPI, at least seven GitHub projects carrying the name along
with two orgs and a GitHub Topic, and both obvious domains held. Recorded in D10 so it is not
raised again.

Also noted at the time: the MicroFlow brand write-up claimed "extends to both microscopic and
macroscopic" as a strength, which contradicts `PROBLEM.md` §5 where macroscopic assignment is
a non-goal. **§5 was left unchanged** — that is a scope decision, not a naming one.

---

### 2026-09-10 — named Veytrix (D9)

Working name `TrafficSim` replaced throughout the documentation. `veytrix` is free on npm
and PyPI; `veytrix.com` is taken and `Vectrix` (electric scooters) is phonetically close —
both recorded in D9 as accepted, known risks rather than discovered later.

**Still to do by hand:** the GitHub repository is still called `trafficsim`. Renaming it needs
repository-admin access, which this session's GitHub app does not have — the owner renames it
in the repository settings, after which the git remote here needs updating.

---

### 2026-09-10 — Q1 and Q3 answered (D7, D8)

- **Q1 → international from the start** (D7). Consequences recorded: HCM as the default LOS
  pack with jurisdictions as swappable data, metric internally with switchable display units,
  and **left-hand/right-hand traffic as a first-class setting from M1** — added to the M1
  scope in `ROADMAP.md` because retrofitting it touches every geometry routine.
- **Q3 → the project owner performs the M2 gate alone** (D8). Recorded honestly as a
  weakening of the gate, with a mandatory mitigation: the M2 pass/fail criteria must be
  written into `ROADMAP.md` and committed **before** M2 implementation starts. `ROADMAP.md`
  now carries an unfilled placeholder for those criteria; starting M2 without filling it
  voids the gate.
- **Q5 opened:** final product name. `Veytrix` is a placeholder. `Headway` was considered
  and rejected — `headwaymaps/headway` is an existing open-source maps stack, too close a
  neighbour in the same field.

---

### 2026-09-10 — repository initialized, documentation spine written

Created a fresh repo for a new project, separate from the prior SUMO-wrapper effort.

**Written:** `PROBLEM.md` (who this is for, the engine-level walls that motivate D1, non-goals,
and what would make the project wrong), `PRINCIPLES.md` (hard rules, deliberate non-goals, and
measured discipline inherited from the prior effort), `ARCHITECTURE.md` (the five-layer map,
marked planned throughout), `ROADMAP.md` (M0–M7 with done-conditions and two hard gates),
`CLAUDE.md` (standing orders), this file.

**Decisions:** D1–D6 above. D1 is the one everything else rests on, and it has an explicit
falsification test at the M2 gate.

**No code was written.** The Systems table in `ARCHITECTURE.md` describes intent, not reality;
every row is marked `planned`.

**Next:** toolchain setup — see the `Next` section above.
