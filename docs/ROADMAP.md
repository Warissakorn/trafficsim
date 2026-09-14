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

Not implemented. Current split commands reject these links without changing the document.
Define stationing/remapping for heads on upstream/downstream portions and inside the
split connector span, then test control and route preservation together. This is an
explicit follow-up, not a claim that arbitrary controlled networks can already be split.

### M1.4 — Connector editor

Implemented: lane-to-lane creation by endpoint picking or Properties, editable interior
curve points, straight/curve reset, selection, reference-safe deletion and retargeting,
and shared endpoint maintenance after Link/Lane/driving-side edits. All changes use
History and the existing version-1 geometry format. Curves are sampled polylines, not
swept-path or turning-radius validation. See NETWORK_EDITOR.md.

### M1.5 — Inspection and diagnostics

Implemented: Links, Connectors and Signal heads tables with two-way selection, canvas
multi-selection by Ctrl-click and rubber band, delete-many as one undoable transaction, and
structured diagnostics whose rows name an object and jump to it. Draft validity still blocks
an edit; runnability against the M0 compiler is reported on demand and never blocks. Exact
limits: property and geometry edits act on one object, there is no group drag, and
vehicle-type/behaviour references are not judged without a catalog. See NETWORK_EDITOR.md.

### M1.5.1 — Demand object tables and editing

Not implemented. Routes and vehicle inputs have no authoring model — they are untyped JSON
inside the project document — so M1.5 lists and names them in diagnostics but gives them no
table and no commands. Define the authoring types and their undoable edits, then table them
alongside the network objects. This is an explicit follow-up, not a claim that demand can
already be authored; if M2 lands first, fold this into it.

### M1.6 — Complete persistence workflow

Autosave/recovery, robust asset/catalog handling and future-schema migration policy.
Basic version-1 atomic save/open and embedded images already exist.

### M1.7 — Run handoff and owner acceptance

Compile an explicit document revision into a run snapshot, expose unsupported simulation
features before Run, and perform the four-leg/aerial-image/ten-minute/reopen acceptance
exercise above. The M0 owner gate remains open; editor work does not waive it.

---

### M1.8 — Run inside the network editor

Not implemented. Today the editor cannot simulate and the simulation window is a separate
`MainWindow` that loads M0 scenario JSON — a user who draws a network and looks for a play
button finds a window that rejects their file. Vissim runs the simulation **in** the network
editor, and that single property is most of why it feels like one tool.

**Scope:** Run/Pause/Step/Reset and a speed control on the editor's own canvas, vehicles drawn
over the network as drawn, and the run attributed to one explicit document revision.

**Depends on:** M1.5.1 (demand has no authoring model, so there is nothing to run) and M1.7
(catalog resolution and revision-to-snapshot). M1.7 stays the plumbing; M1.8 is the surface.

**Done when:** an engineer draws a network, authors demand, presses Run without leaving the
editor, and watches vehicles traverse it — with the not-yet-validated marker still displayed.

**Explicitly not in M1.8:** results tables, per-movement delay or LOS (M2 onwards), and any
relaxation of D5. Running a network is not evidence that its numbers mean anything.

### M1.9 — Network Objects sidebar, Vissim gestures and shortcuts

Not implemented. The edit mode is a six-entry `QComboBox`, connectors are created one lane pair
at a time, `Delete` removes a geometry vertex rather than the selected objects, and no shortcut
selects a tool at all. See [`VISSIM_PARITY.md`](VISSIM_PARITY.md) §1–2 for the measured gap.

**Scope:** a permanent network-objects sidebar replacing the tool dropdown; connector creation
in one gesture across a lane range rather than per pair; `Delete` acting on the selection with
vertex removal moved to a modifier; a shortcut per object type.

**Done when:** an engineer who uses Vissim daily draws a four-leg intersection here without
looking for a control that is not where their hand expects it.

**Explicitly not in M1.9:** new network object types (§4 of the parity review), editable object
tables, and group drag.

### M1.10 — Levels and display types

Not implemented. There is no `level` anywhere in the model, so overlapping geometry — flyovers,
underpasses — cannot be ordered, and draw styles are fixed in rendering code.

**Scope:** a level on links and connectors that orders drawing and selection, and named display
types. Both are **content, not code** (hard rule 5): adding the fiftieth display type must not
need a code edit, so they live in `data/`.

**Done when:** a grade-separated junction draws and selects correctly at every zoom, and a new
display type is a data file.

**Explicitly not in M1.10:** 3D, and any change to how levels affect simulation — this is a
drawing and selection concern only.

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
