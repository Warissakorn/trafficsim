# ROADMAP — TrafficSim

Not a schedule. A **sequence**, so that any session can see where it sits and what closes
the milestone it is in.

**Two rules, inherited from a prior effort that broke both and paid for it:**

1. A milestone closes **only** when its done-condition is met and its gate has passed.
   Merged code is not a passed gate.
2. If a milestone must ship incomplete, the missing part is **carved out into a new numbered
   milestone in this file, immediately**, not left as a note. "We'll come back to it" is how
   a roadmap stops being true.

---

## M0 — Vertical slice

The smallest thing that is genuinely a traffic simulator, end to end. Every layer exists in
miniature, so wrong assumptions surface while they are still cheap.

**Scope:** two crossing links · one connector each way · one fixed-time signal · one vehicle
input · Wiedemann-style car-following · a native 2D view showing vehicles as dots · a headless run
that prints average delay.

**Done when:** `trafficsim-desktop` shows vehicles accelerating, queueing at red, and
discharging at green in a way a traffic engineer recognises as plausible — and CTest
proves the same seed produces the same run on a fixed engine/toolchain.

**Explicitly not in M0:** lane changing, editing anything, saving anything, LOS, multiple
seeds, priority control. The slice is about the *shape*, not the feature set.

---

## M0.1 — Native C++ migration (D15)

**Status:** implemented; Linux native/desktop checks passed. Owner M0 acceptance remains open.

**Technical scope:** C++20 core/model/evaluation and CLI, CMake/CTest, Qt Widgets harness,
strict JSON loading, four TypeScript baseline fixtures, and native developer checks.
The former TS application remains in Git history. All new application development is C++.

**Done when:** core/network behaviour passes the saved baseline comparisons and native
regressions; the desktop compiles and passes Run/Pause/Step/Reset/seed/language smoke tests;
and the native build/run instructions are usable. This does not close the owner's M0
plausibility gate, the M1 editor or M7 installer.

---

## M1 — Network editor

The Vissim modelling surface, natively: links are first class, connectors are real objects,
junctions are not something the user places.

**Status:** M1.1–M1.10 implementation is available. The owner acceptance in M1.7 remains open;
M1 is not closed until its timed gate passes.

**Done when:** an engineer draws a four-leg intersection with turn pockets from scratch, over
an aerial image, in under 10 minutes, without reading documentation — and reopening the file
gives back exactly what they drew.

**Includes, because D7 makes it non-optional:** left-hand / right-hand traffic as a project
setting that actually drives connector and conflict geometry. Retrofitting this later touches
every geometry routine.

---

### M1.1 — Document and commands

Implemented: a Qt-free versioned ProjectDocument, persistent revision/ID counter,
named atomic edits, 100-entry Undo/Redo, save-point tracking and failed-edit rollback.

### M1.2 — Canvas and background

Implemented: pan/zoom/fit, metric grid/snap, link selection, embedded local background
images, two-point image calibration, transform/opacity editing and distance measurement.

### M1.3 — Link and lane tools

Implemented: drawing, point/link dragging, insert/remove points, per-lane widths,
reference-safe deletion, splitting with continuity connectors, opposite carriageways,
and a downstream extra lane for a turn-pocket approach. Basic atomic save/open landed
with these tools so drawings are not disposable. See NETWORK_EDITOR.md for the exact limits.

### M1.3.1 — Split links carrying signal heads

Implemented. Heads are located on the original lane geometry, classified by their
projection onto the original centreline, and projected onto the owning upstream lane,
downstream lane or split connector path. Heads inside the 0.2 m connector span become
connector-mounted controls. Route order, head IDs and program IDs are preserved in
one undoable transaction, including both driving sides and turn-pocket splits.

### M1.4 — Connector editor

Implemented: lane-to-lane creation by endpoint picking or Properties, editable interior
curve points, straight/curve reset, selection, reference-safe deletion and retargeting,
and shared endpoint maintenance after Link/Lane/driving-side edits. All changes use
History and persisted polyline geometry (schema 1 is migrated to schema 2). Curves are sampled polylines, not
swept-path or turning-radius validation. See NETWORK_EDITOR.md.

### M1.5 — Inspection and diagnostics

Implemented: Links, Connectors and Signal heads tables with two-way selection, canvas
multi-selection by Shift-click and rubber band, delete-many as one undoable transaction, and
structured diagnostics whose rows name an object and jump to it. Draft validity still blocks
an edit; runnability against the M0 compiler is reported on demand and never blocks. Exact
limits: property and geometry edits act on one object, there is no group drag, and
vehicle-type/behaviour references are not judged without a catalog. See NETWORK_EDITOR.md.

### M1.5.1 — Demand object tables and editing

Implemented. Optional typed authoring demand replaces the untyped JSON field.
Routes, vehicle inputs, fixed-time programs and heads have validated atomic commands
and native dialogs; routes, inputs and programs have their own tables. Route deletion
cascades inputs; referenced program deletion and topology-changing retargets are rejected.
Vehicle compositions, turning proportions and movement evaluation remain M2.

### M1.6 — Complete persistence workflow

Implemented. Atomic, bounded save/open and asset validation are shared with 15-second
dirty-revision recovery copies. Per-window locks exclude active editors; restored
documents open untitled and dirty. Vehicle/behaviour catalogs can be embedded explicitly.
Schema 1 and bare M0 authoring files load without changing IDs; saves write schema 2,
with default ranges/levels/styles for older files. Unknown future versions are rejected.

### M1.7 — Run handoff and owner acceptance

Run handoff implemented: compile one document revision with resolved catalogs into a
detached network/scenario snapshot. Unsupported features are exposed before Run;
successful edits invalidate a run. The engine's existing capability guards remain.

**Owner acceptance remains open.** Perform the four-leg/aerial-image/ten-minute/reopen
exercise in [M1_ACCEPTANCE.md](M1_ACCEPTANCE.md). The M0 plausibility gate also remains
open. No automated test or implementation status closes either gate.

---

### M1.8 — Run inside the network editor

Implemented. Run/Pause (F5), Step (F6 or Space on the canvas), Reset, seed and playback
speed control operate in the editor. Vehicles and fixed-time heads render over the
drawn network; status identifies the document revision and seed. Every fixed step
uses the unchanged core and the not-yet-validated marker stays visible.

**Done when:** an engineer draws a network, authors demand and watches it run without
leaving the editor. The automated drawing/demand/run/replay workflow covers the software
path; the owner's hands-on exercise remains in M1.7.

**Explicitly not in M1.8:** movement results, control delay, LOS or relaxation of D5.

### M1.9 — Network Objects sidebar, Vissim gestures and shortcuts

Implemented. A permanent Network Objects sidebar selects the creation type.
Ctrl+right-drag opens link and connector data dialogs; one connector owns contiguous
lane ranges. Ctrl+right-click opens demand/control creation or inserts a geometry point
in Select mode. Connector corner drags resize unreferenced ranges. Left-click during
creation adds intermediate polyline points.

Shift-click extends selection; Ctrl+left-click duplicates selected links and their
internal connectors/heads with fresh IDs, preserving programs without doubling demand.
Delete removes selected objects; Ctrl+Delete removes a vertex. Tool shortcuts and Tab
overlap cycling are available. Ctrl+B toggles the background; Ctrl+Shift+O toggles tables.

The model derives stable per-lane paths for compilation, reanchoring and reference
cleanup. Unequal ranges may express merges in authoring; the core still rejects them.

**Done when:** a daily Vissim user draws the four-leg intersection without searching for
controls. Implementation and automated gesture checks do not replace this owner test.

**Explicitly not in M1.9:** new network types from the parity review, editable table cells,
group drag and rotation.

Connector reanchoring is no longer part of what makes those two expensive. VISSIM_PARITY
§1 and §6 item 10 cite it as a blocker, and that text stays as the 2026-09-14 assessment,
but reanchoring is now a similarity transform of the connector's endpoint chord, so
applying one transform as a sequence of single-object edits composes exactly. Measured on
a hand-edited curve between two links, a shared translation applied as two separate link
moves fell from 8.17 m of distortion to 2.6e-14 m, and a shared rotation from 5.53 m to
7.1e-15 m. What these gestures still need is selection-wide transform plumbing, and for
paste the ID allocation the parity review also names — not connector geometry.

### M1.10 — Levels and display types

Implemented. Links and connectors persist a level and named display type. Rendering,
vehicle/head overlays, hit testing and visible-level filtering use level order; Tab
can select an overlapping lower object. Levels and styles live in `data/levels/` and
`data/display-types/`; adding a style needs no C++ change. Unknown style IDs retain
their value and use the default appearance.

**Done when:** a grade-separated junction draws and selects correctly at different
zooms and a new display type is a data file. Automated gesture/persistence coverage is
included; the owner should inspect a representative junction as part of acceptance.

**Explicitly not in M1.10:** 3D or simulation effects from elevation. A drawn flyover
does not add right-of-way, merging or crossing-conflict logic.

---

## M2 — Demand, run, first numbers · **GATE**

Vehicle inputs per interval, compositions, turning proportions. Press Run, get average delay
and queue per movement.

**Done when:** the M1 intersection, loaded with counted volumes, runs and produces a delay
table.

**GATE — the honesty check.** Before M3 starts, a practising traffic engineer completes a
small **real** study in this tool and in their current tool, and answers directly whether the
free SUMO-based alternative would have been good enough for this job.

> **This gate is currently performed by the project owner alone (D8), which makes it weak** —
> the person judging is the person who chose to build an engine. It is therefore run as a
> **pre-registered** test: **the pass/fail criteria are written into this file and committed
> before any M2 implementation begins.** Criteria decided afterwards are not a test. A pass
> under these conditions means "not disproven", never "confirmed". Recruiting outside
> engineers later strengthens the gate and is never wasted effort.

**Pre-registered criteria: TO BE WRITTEN before M2 implementation starts.** Leaving this
line unfilled and starting M2 anyway voids the gate.

- If the answer is broadly *yes*, this project is the wrong answer to the problem —
  see `PROBLEM.md` §7.1 — and the honest move is to stop and reconsider, not to continue
  because effort has been spent.
- This gate exists because the whole justification for owning an engine (`PROBLEM.md` §2)
  is an argument, not yet an observation.

---

## M3 — Right-of-way: conflict areas and priority rules

**The differentiating milestone.** Everything a SUMO wrapper structurally cannot do.

- Conflict areas as **editable input**: at each conflict point, choose which movement yields,
  or make it undetermined.
- Priority rules with real **gap time and headway in seconds and metres**.
- Stop and yield control.
- Signal heads placed **anywhere on a link**, not only at a stop line.

**Done when:** an unsignalized T-junction with a minor-road left turn produces plausible,
tunable minor-road delay that responds correctly to changing the gap time — and the same
network is demonstrably not expressible in a SUMO wrapper.

---

## M4 — Signal control

Controllers, signal groups, programs, fixed-time and actuated, detectors, ring-barrier.

**Done when:** an eight-phase two-ring, two-barrier plan is built from a real timing sheet in
under 8 minutes with zero validation errors, and runs.

---

## M5 — Evaluation and reporting

The reason the whole project exists (`PROBLEM.md` §4).

- Movement-level delay, LOS, queue length, travel time.
- Multi-seed batch runs with means and confidence intervals.
- Report tables that go into an impact study without passing through a spreadsheet.
- LOS thresholds as swappable per-jurisdiction data, never compiled in.

**Done when:** one intersection, ten seeds, one command → a movement-level LOS table with
confidence intervals, ready to paste into a report.

---

## M6 — Calibration and validation · **GATE**

**Done when:** the engine reproduces published benchmark results — capacity and delay for a
signalized approach, gap-acceptance capacity for an unsignalized minor movement — within a
stated tolerance, and the tolerance is published in the docs and shown in the app.

**This is a hard gate.** Numbers from an unvalidated engine must never reach a regulator.
Until M6 passes, every results screen carries a permanent "not yet validated" marker.

---

## M7 — Desktop packaging

Offline install, native file dialogs, no server required.

**Done when:** a non-technical user installs from a single file on Windows and macOS, opens a
project by double-clicking it, and works with no network connection.

> The Qt desktop now exists from M0.1 (D15), with an automated controls smoke test.
> M7 still owns installers, clean-machine deployment, platform integration and file
> associations on Windows/macOS. A developer executable does not close M7.

---

## Later, with their own milestones — not to be started inline

Recorded so nobody starts them opportunistically:

- Pedestrian and public-transport modelling
- Regional/macroscopic assignment
- Scenario management and comparison
- 3D presentation
- Collaboration
