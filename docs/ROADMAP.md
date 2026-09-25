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

**Where that observation is made (M1.24):** open `data/scenarios/crossing.json` in the
editor and use Run/Pause/Step/Reset there. Until M1.24 this said the same thing about a
separate M0 window; that window is gone, the observation is not. The editor reads a bare M0
scenario, steps the unchanged core, and its run status shows the mean trip delay and the
safety-clamp count the retired window showed. `scenario-run-ui` pins that this path
reproduces `trafficsim-cli 42` exactly — 31 completed trips, mean delay 29.249359418430977 —
so an engineer judging plausibility in the editor is judging the same run the CLI prints.
**The gate itself is unchanged and still open:** no automated test closes it, and the
plausibility judgement is the owner’s to make.

**Explicitly not in M0:** lane changing, editing anything, saving anything, LOS, multiple
seeds, priority control. The slice is about the *shape*, not the feature set.

---

## M0.1 — Native C++ migration (D15)

**Status:** implemented; Linux native/desktop checks passed. Owner M0 acceptance remains open.

**Technical scope:** C++20 core/model/evaluation and CLI, CMake/CTest, a Qt Widgets desktop
(originally a separate harness window; the editor since M1.24),
strict JSON loading, four TypeScript baseline fixtures, and native developer checks.
The former TS application remains in Git history. All new application development is C++.

**Done when:** core/network behaviour passes the saved baseline comparisons and native
regressions; the desktop compiles and passes Run/Pause/Step/Reset/seed/language smoke tests
(in the editor since M1.24, which retired the separate harness window);
and the native build/run instructions are usable. This does not close the owner's M0
plausibility gate, the M1 editor or M7 installer.

---

## M1 — Network editor

The Vissim modelling surface, natively: links are first class, connectors are real objects,
junctions are not something the user places.

**Status:** M1.1–M1.20 are implemented, including the carve-outs M1.3.1, M1.5.1, **M1.11.1** and
**M1.12.1**. **M1.12.2 is closed** (the reported miter bulge was measured along the cross-section
and is not a defect) and **M1.12.3 is closed by M1.19**. The owner's requirement that a Connector's
lanes meet the Link lanes they are assigned to ran M1.17 (reverted) → M1.18 (flush) → **M1.19**
(each lane on the lane it feeds); **M1.20** gives the Connector its own position. M1.7's owner
acceptance: **M1 usability accepted by owner ruling on 2026-09-25 (D49)** after one timed attempt
(9 min 40 s, no assistance, save/reopen exact) that did not include turn pockets or an aerial
image — see `M1_ACCEPTANCE.md` for what it did and did not show. M0 plausibility remains open.

M1.1–M1.6 and M1.8–M1.10 are implemented and their full bodies are in
[`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md); each keeps its heading and
a status line here so the sequence stays whole.

**Sub-milestones below are in numeric order, and a carve-out made under rule 2 is filed at its
number rather than at the end** — M1.11.1 inside M1.11, M1.12.1 and M1.12.3 inside M1.12 — where a
session looking for unfinished work will look for them.

**Done when:** an engineer draws a four-leg intersection with turn pockets from scratch, over
an aerial image, in under 10 minutes, without reading documentation — and reopening the file
gives back exactly what they drew.

**Includes, because D7 makes it non-optional:** left-hand / right-hand traffic as a project
setting that actually drives connector and conflict geometry. Retrofitting this later touches
every geometry routine.

---

### M1.1 — Document and commands

Implemented: a Qt-free versioned `ProjectDocument`, named atomic edits, 100-entry Undo/Redo, save-point tracking and failed-edit rollback.

### M1.2 — Canvas and background

Implemented: pan/zoom/fit, metric grid/snap, selection, embedded background images with two-point calibration and distance measurement.

### M1.3 — Link and lane tools

Implemented: drawing, point/link dragging, per-lane widths, reference-safe deletion, splitting with continuity connectors, opposite carriageways, turn pockets.

### M1.3.1 — Split links carrying signal heads

Implemented: heads are classified by projection onto the original centreline and reprojected onto the owning lane or split connector, in one undoable transaction.

### M1.4 — Connector editor

Implemented: lane-to-lane creation, editable interior curve points, straight/curve reset, reference-safe deletion and retargeting, shared-endpoint maintenance. Curves are sampled polylines, not swept-path validation.

### M1.5 — Inspection and diagnostics

Implemented: Links/Connectors/Signal-heads tables with two-way selection, multi-selection, delete-many as one transaction, and structured diagnostics that name and jump to an object. Draft validity blocks an edit; runnability only informs (D18b).

### M1.5.1 — Demand object tables and editing

Implemented: typed authoring demand replaces the untyped JSON field. Routes, inputs, programs and heads have validated atomic commands, dialogs and tables. Compositions and turning proportions remain M2.

### M1.6 — Complete persistence workflow

Implemented: atomic bounded save/open, asset validation, 15-second dirty-revision recovery copies, per-window locks, explicit catalog embedding. Schema 1 and bare M0 files load without changing IDs; unknown future versions are rejected.

### M1.7 — Run handoff and owner acceptance

Run handoff implemented: compile one document revision with resolved catalogs into a
detached network/scenario snapshot. Unsupported features are exposed before Run;
successful edits invalidate a run. The engine's existing capability guards remain.

**Owner acceptance: accepted by owner ruling, 2026-09-25 (D49)** — recorded, with what the attempt
did not show, in [M1_ACCEPTANCE.md](M1_ACCEPTANCE.md). The M0 plausibility gate remains open.

---

### M1.8 — Run inside the network editor

Implemented: Run/Pause (F5), Step (F6 or Space), Reset, seed and playback speed inside the editor; vehicles and fixed-time heads render over the drawn network and the status names the revision and seed. Every step uses the unchanged core and the not-yet-validated marker stays visible. **Explicitly not in M1.8:** movement results, control delay, LOS, or any relaxation of D5.

### M1.9 — Network Objects sidebar, Vissim gestures and shortcuts

Implemented: a permanent Network Objects sidebar; Ctrl+right-drag opens link/connector dialogs; Ctrl+right-click creates demand/control or inserts a geometry point; Ctrl-click extends selection and Ctrl+left-drag duplicates with fresh IDs; Delete removes objects, Ctrl+Delete a vertex; tool shortcuts, Tab overlap cycling, Ctrl+B background, Ctrl+Shift+O tables. **Done when** a daily Vissim user draws the four-leg intersection without searching for controls — an owner test that automated gesture checks do not replace. **Explicitly not in M1.9:** new parity object types, editable table cells.

### M1.10 — Levels and display types

Implemented: links and connectors persist a level and named display type; rendering, overlays, hit testing and visible-level filtering use level order, and Tab can select an overlapping lower object. Levels and styles live in `data/levels/` and `data/display-types/`, so adding a style needs no C++ change; unknown style IDs keep their value and use the default appearance. **Explicitly not in M1.10:** 3D, or any simulation effect from elevation — a drawn flyover adds no right-of-way, merging or crossing-conflict logic.

### M1.11 — Body attachments and direct lane resizing

Implemented in the Network Editor: Select/Links Ctrl+right-drag creates a Link from empty
space or a Connector between lane positions. Two-click connection also picks lane bodies.
Source/target positions persist in schema 3. Source, target and middle side handles resize
ranges from one lane; a Link side handle resizes its lane list. Undo/Redo, cancellation,
link edits, duplication and split attachment remapping share the existing command boundary.
The middle handle sets both ranges together; the Connector path count remains their maximum.
First-lane changes use the dialog or Properties. This does not claim independent arbitrary
internal Connector lane topology or close the owner's usability gate.

### M1.11.1 — Compile interior attachments into runtime lane sections

**Implemented.** `runtimeSections` cuts each lane at every station where a Connector attaches to
its body and `buildScenario` compiles the pieces, so an attachment part way along a lane runs in
both directions. **Leaving** is a diverge: the vehicle travels the drawn partial distance, and an
authored route stops at the section carrying the Connector it leaves by. **Arriving** is a merge,
arbitrated by a priority rule derived from the drawing (M3.1): the vehicle joins at the drawn
metre — the arriving path's successor is the section that *starts* at the cut, not the one that
ends there — gives way to traffic already on the lane, and is never charged for the stretch
upstream of where it came in.

Signal heads rebase onto the section they stand on, a head on a cut belonging to the upstream one
as `splitLink` already had it. Both render sites key geometry by section id from the same table
the scenario came from, and the route dialog still offers whole lanes via `authoringSegments`, so
no derived id reaches the project file.

Replay is unchanged and the four frozen baselines still pass, because `sectionId(laneId, 0)` is
`laneId`: a lane with nothing attached compiles to the `Segment` it always did. **One case still
cannot run:** a cut within `kMinSectionLength` (0.2 m) of a lane end or another
cut on the same lane, since the core accepts no zero-length segment. It reports
`UNSUPPORTED_CONNECTOR_POSITION`, reworded — the old string claimed the core runs only
end-to-start connectors, which is no longer true. A derived rule's gap time and headway come from
`data/priority-rules/`; their absence blocks **Run** with an object-linked
`EDIT_NO_PRIORITY_DEFAULTS` and does **not** block an edit, which is D18b's boundary.

**Done:** a vehicle leaves and enters at the drawn stations, travels the correct partial-lane
distances, obeys section-mounted signals, and replays deterministically on both driving sides.
The merge, internal-input and repeated-segment guards stand — `UNSUPPORTED_MERGE` was narrowed by
construction, not removed — and no persisted duplicate runtime network was introduced, because
sectioning changes no authored id and is a pure function of the drawing.

### M1.12 — Fixed lane edges, road markings and selection/copy gestures

Implemented following the owner's 1–3 lane examples: Link handles on both sides,
source/target/middle Connector handles on both sides, fixed surviving lane positions,
and shared edge/divider rendering in the editor and diagnostic view. Schema 4 stores
lane offsets and stable Connector interpolation weights. Ctrl-click adds selection;
Ctrl-drag selected Links, Connectors or Signal heads creates a single undoable copy.
Invalid attached-object drops roll back the entire edit. Tables select heads directly.
Owner Windows interaction and timed acceptance remain open.

---

### M1.12.1–M1.25 — Completed geometry, authoring, audit and editor iterations

**All closed**, with the originals and what none of them claims about Vissim parity in
[`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md): a Connector carries
its own `laneWidths` and `laneMarkings` in schema 6, `connectorLaneWidths` the single place a
width is decided (M1.12.1); the 24% miter "bulge" measured along the cross-section, where a
mitered corner's diagonal is `width / cos(φ/2)` by construction, so `offsetGeometry` was not
changed (M1.12.2); metre attachment stations (M1.13),
intermediate points (M1.14), names (M1.15), group moves (M1.16), flush mouths (M1.18),
lane-aligned mouths (M1.19, which closed M1.12.3 and left `TIGHT_CONNECTOR_RADIUS` alone) and
independent Connector placement with off-Link cleanup (M1.20); schema 7's supported subset
of the owner's specifications, where an unsupported field fails on load rather than vanishing on
save (M1.21); and the lifecycle audit's wrong-side retargets, pathological mouths, physical
range picking and coalesced releases (M1.21.1); the one-window editor, whose run status carries
the CLI's own delay figure (M1.24); and demand drawn by pointer (M1.25). M1.17's wedge remains
reverted, owner M1 acceptance remains open, and none of this closes the supplied target
specifications.

### M1.22 — Remaining authoring and interaction requirements

**Open.** Link spline/arc construction and curve parameters, extend/merge and safe referenced
reversal; tapered cross-sections, shoulders/median/sidewalk display; per-Link driving-side
semantics; snap priorities, alignment/angle constraints; layer locks, multi-property inspector,
context menus, shortcuts and accessibility. History, nudging and rotation are implemented below.
**Gate:** command/reference roundtrips plus both-side gesture tests and keyboard-only owner
exercise. Settle the conflicts in SPEC_AUDIT before changing geometry or gestures.

#### M1.22.1–M1.22.2 — History, keyboard editing and selection rotation

Implemented: a bilingual History dock with named Undo/Redo and saved-state markers, arrow-key
nudging through the group-move command, level-filtered selection, and Alt-drag rotation with a
pivot/angle preview, Shift steps and an exact-angle dialog — all preserving internal Connector
shapes, stations, lane metadata and carried heads in one Undo transaction. Full entries in
[`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md). These do not close
M1.22: geometry/snapping tools, custom pivots, layer locks, bulk inspection and the
keyboard-only owner exercise remain open, as does M1's timed gate.

### M1.23 — Interchange, document workflow and measured rendering

**Open.** Native-to-GeoJSON/CSV/PNG exports; GeoJSON/OSM/Shapefile import and CRS mapping;
recent files/tabs, spatial indexing/culling/LOD and optional renderer acceleration.
Competitor-format imports and 3D require an explicit scope revision before implementation.
**Gate:** known-coordinate import/export fixtures, multi-document recovery isolation and a
reproducible real-network benchmark. Do not claim 10k/100k-object performance in advance.

### M1.26–M1.27 — Carriageway demand, per-lane shares, optimization · **CLOSED**

Full entries in [`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md) (moved
2026-09-24). **M1.26:** a route names Links and Connectors and is compiled per lane; an input is
the Link total. **M1.26.1:** optional `laneShares` (D32). **M1.27:** build, redraw and engine
optimization, and the counted gesture walkthrough. **Gate still open for M1.26:** the
keyboard-only gestures and the owner's timed exercise in `M1_ACCEPTANCE.md`.

---

## M2 — Demand, run, first numbers · **GATE**

Vehicle inputs per interval, compositions, turning proportions. Press Run, get average delay and queue per movement.

**Done when:** the M1 intersection, loaded with counted volumes, runs and produces a delay
table.

**GATE — the honesty check.** Before M3 starts, a practising traffic engineer completes a
small **real** study in this tool and in their current tool, and the result shows whether the
tool is usable for real engineering work.

> **This gate is currently performed by the project owner alone (D8), which makes it weak** —
> the person judging is the person who chose to build an engine. It is therefore run as a
> **pre-registered** test: **the pass/fail criteria are written into this file and committed
> before any M2 implementation begins.** Criteria decided afterwards are not a test. A pass
> under these conditions means "not disproven", never "confirmed". Recruiting outside
> engineers later strengthens the gate and is never wasted effort.

**Pre-registered criteria — ratified by the owner on 2026-09-24, re-registered the same day
before any gate observation (D38), and again on 2026-09-25, still before any gate observation,
when the owner withdrew C2 (D51)** (record in [`M2_GATE.md`](M2_GATE.md)):

- **C1 — Completion.** One real study (signalised, protected phasing, counted 15-minute volumes,
  the owner's timing plan) completed end to end: network over its aerial image, volumes,
  composition, timing, Run, per-movement delay and queue table. Fails on hand-edited JSON, a code
  change during the study, or outside help.
- **C2 — withdrawn (D51).** No test times TrafficSim against Vissim or any other tool.
- **C4 — Plausibility, recorded, not scored.** Per movement, TrafficSim delay beside the current
  tool's; a movement more than **two LOS letters** apart opens a numbered investigation.

**Pass = the owner judges it passed (D51)**, on C1's study with C4 recorded — reported as *not
disproven*. (C0 and C3 were withdrawn by D38, C2 by D51; the remaining criteria keep their
numbers so earlier records still resolve.)

- If the owner judges it failed, the tool is not yet usable for the job it exists for — see
  `PROBLEM.md` §7.1 — and M3 does not start until what failed is fixed and the gate re-run.

---

### M2.0–M2.6 — Slices (detail in [`M2_PLAN.md`](M2_PLAN.md) §4)

**M2.0** preconditions: the four-leg fixture, same-station cuts, dropped-lane advisory, and
**M2.0.1** — Connectors meeting at a lane's start get M3.1's derived rule (D35; this extends M3.1,
it does not start M3). **M2.2** time-varying volumes · **M2.3** vehicle compositions · **M2.4**
static turning proportions (moved here from M2.1, D37) · **M2.5** movement delay and queue, one
run (implemented 2026-09-24, D39/D40) · **M2.6** the owner's gate study. Amber stays red until M4 (D36).
**Status 2026-09-24:** M2.0, M2.0.1, M2.2, M2.3, M2.4 and M2.5 implemented. M2's done-condition
(the M1 intersection with counted volumes runs and produces a delay table) is met in code. M2.6 is
next; the
gate itself is open and nothing here closes it.

### M2.1 — Link/lane/Connector behavior and demand extensions

**Open.** Resolve vehicle-owned desired speed versus Link limits/factors, then distributions,
inheritance, road classes, lane types/restrictions, and partial/dynamic/positioned routing
decisions (compositions and the static decision moved to M2.3/M2.4, D37). Persistence must not
imply runtime support; add explicit capability guards.
**Gate:** validated distributions and references, deterministic sampling, old seed fixtures
unchanged with defaults, observable runtime effects for every exposed behavior parameter.
M2's pre-registered owner gate still precedes this work.

#### M2.1.1 — Routeless inputs and placed routing decisions · **Implemented 2026-09-24** (D42, D43)

Owner request. An input on a Link needs no route; a decision placed on a Link sends routeless
vehicles to destination Links by relative flow; everything expands at compile time into static
routes (SIMULATION.md). Still open in M2.1: a decision's station along the Link, per-interval
flows, and partial/dynamic decisions. Proportions after the entry Link wait for lane changing.

#### M2.1.2 — Per-interval turning proportions · **Implemented 2026-09-24** (D45)

Owner choice for the next engineering item. A routing decision takes counted turning volumes per
interval (pasted per row in its dialog), schema 12; expanded at compile time per interval piece.
The interval is chosen by network entry time, not the time at the decision. Still open in M2.1:
a decision's station along the Link, and partial/dynamic decisions.

### M2.7 — Signal heads by pointer; fixed-time Signal Controllers · **Open** (owner request, before M2.6)
M2.7a (head = stop line, placed by click, D47) and M2.7b (controllers, signal groups, schema 13, D48) implemented; the owner's use in M2.6 closes it. Detail and done-condition: [`M2_PLAN.md`](M2_PLAN.md) §4.
---

## M3 — Right-of-way: conflict areas and priority rules

**The right-of-way model an engineer controls.**

- Conflict areas as **editable input**: at each conflict point, choose which movement yields,
  or make it undetermined.
- Priority rules with real **gap time and headway in seconds and metres**.
- Stop and yield control.
- Signal heads placed **anywhere on a link**, not only at a stop line.

**Done when:** an unsignalized T-junction with a minor-road left turn produces plausible,
tunable minor-road delay that responds correctly to changing the gap time.

**Preparation (2026-09-24):** [M3_PLAN.md](M3_PLAN.md), [M3_CONTRACT.md](M3_CONTRACT.md)
and [M3_ACCEPTANCE.md](M3_ACCEPTANCE.md) specify the ordered work and evidence. M2.6 is
still unperformed; this design neither starts runtime implementation nor waives its gate.
Interior signal positions already work in the model/runtime; M3 audits and completes the
authoring/interaction path, rather than introducing a second signal-position mechanism.

---

### M3.1 — Merge priority by gap time and headway

**Implemented, and it does not close M3.** The engine had no answer at a merge at all, which is
why `validateScenario` refused one outright: a place fed by two segments had no rule for who goes.
`PriorityRule{yieldSegmentId, yieldPosition, conflictSegmentId, conflictPosition, gapTime,
headway}` supplies one — Vissim's priority rule, with the two numbers an engineer tunes.

Car-following across a merge already worked, because `OccupiedSpan::segmentIndex` is a global
index into `Scenario::segments` and spans are bucketed globally, so two vehicles see each other
the moment they share a segment. What a rule adds is seeing the major approach *before* entering
it, which is not on the minor vehicle's own route and so is invisible to `closestVehicle`. A
vehicle that must give way is held at its stop line by the **same clamp a red signal head uses**,
not a second mechanism beside it. Rule-to-route incidence resolves once per scenario in
`ScenarioIndex`, mirroring `routeHeads`.

`UNSUPPORTED_MERGE` is loosened **by construction, never by removal**: a place fed by *n*
segments is runnable only when at least *n*−1 of them give way to another of them, so exactly one
has priority and the rest have somewhere to wait. A network that has not been through the
priority model reports it exactly as before — six tests break if the guard is deleted instead.
A segment may not give way to itself.

A standing queue upstream does **not** block: a stopped major vehicle further off than the
headway is a gap, and blocking on it would deadlock the minor approach instead of releasing it.

Gap time and headway for a **derived** rule live in `data/priority-rules/`, so changing them is a
data edit (hard rule 5). They are read best-effort, because a document carrying its own catalogs
must stay portable to a machine with no data directory; a rule that has to be derived without
them is refused at the point of use rather than given a zero gap time, which would be a merge
nobody gives way at, invented in silence.

**A deterministic threshold test, not a calibrated critical-gap model:** hard rule 4's
not-yet-validated marker stays and M6 still owns fidelity.

**Explicitly NOT in M3.1, and all still M3's:** conflict areas as editable input, priority rules
as an authorable object with their own UI, stop and yield control, crossing conflicts, and signal
heads placed anywhere on a link. M3's done-condition — an unsignalized T-junction whose
minor-road delay responds correctly to changing the gap time — is **not** met by this milestone
and M3 remains open.

---

### M3.2 — Lane changing and crossing-conflict control

**Open.** Explicit priority rules/conflict areas, lane-change distances/emergency stopping,
cooperation, visibility and calibrated gap acceptance. Specify signal/right-of-way interaction
without disabling collision constraints; account explicitly for any removed blocked vehicle.
**Gate:** controlled merges, diverges and crossing conflicts, congestion/no-overlap regression,
deterministic replay and the M3 owner exercise; scientific claims remain gated by M6.

#### M3.2.1-M3.2.8 — Ordered implementation slices

All implementation below requires the M2 gate to pass. Dependencies and exact contracts
are in [M3_PLAN.md](M3_PLAN.md); the live next action stays in `NEXT.md`.

| Slice | Scope | Status / gate |
|---|---|---|
| M3.2.1 | Contracts and acceptance design | Prepared; documentation only, no runtime claim |
| M3.2.2 | Authored controls, persistence, commands, compiler and effective-priority validation | Open; reference roundtrips, legacy compatibility, cycles/ties rejected |
| M3.2.3 | Crossing occupancy, admission, rear clearance and downstream space | Open; no-overlap, congestion accounting and exact replay |
| M3.2.4 | Conflict-area and priority-rule editor | Open; supported runtime effects, bilingual mouse/keyboard workflows |
| M3.2.5 | Stop/Yield with signal composition | Open; each vehicle serves Stop, Yield can pass, green retains physical safety |
| M3.2.6 | Signal-position workflow and unsignalised queue counters | Open; interior/cut positions, counter fixtures and CLI/editor agreement |
| M3.2.7 | T-junction fixture and owner evidence | Open; controlled gap response, completed/unserved reporting and owner exercise |
| M3.2.8 | Lane changing, cooperation, visibility and remaining behavior | Open; separate contract, controlled changes/conflicts, congestion and replay; calibrated gap acceptance still requires M6 evidence |

Passing M3.2.7 alone does not close M3.2 or its remaining booked scope. No gate result is
inferred from the preparation above, and the not-yet-validated marker remains.

---

## M4 — Signal control

Controllers, signal groups, programs, fixed-time and actuated, detectors, ring-barrier.

**Done when:** an eight-phase two-ring, two-barrier plan is built from a real timing sheet in
under 8 minutes with zero validation errors, and runs.

---

### M4.1 — Detectors and controller integration

**Open.** Detectors/DCP authoring and events, signal groups/controllers, actuated/adaptive logic,
phase validation and external-control interfaces. External integrations need explicit protocols
and deterministic recorded inputs; no wall-clock dependency in core.
**Gate:** passage/occupancy/aggregation fixtures, controller state-transition tests, dangling-
reference cleanup and the M4 real timing-sheet exercise.

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

### M5.1 — Additional network objects and evaluated outputs

**Open.** Evaluation nodes/stop lines, movement measurements/overlays and reports. Parking,
transit stops, crosswalks and multimodal behavior from the supplied documents require their own
runtime and calibration contracts, not merely stored object types.
**Gate:** event-accounting and known analytical scenarios, reference cleanup and multi-run
output checks. Keep completed-trip delay distinct from HCM control delay/LOS.

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
