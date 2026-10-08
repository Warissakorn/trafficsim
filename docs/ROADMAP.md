# ROADMAP — TrafficSim

Not a schedule. A **sequence**, so that any session can see where it sits and what closes the milestone it is in.

This is the single roadmap and review document. Milestone scopes and gates below remain authoritative; the dated review records evidence and proposals, not additional approvals. `NEXT.md` remains the one live session queue.

Navigation: [M0](#m0--vertical-slice) · [M5](#m5--evaluation-and-reporting) · [Review record](#review-record--2026-10-03) · [Owner decisions O1–O10](#4-verified-but-the-owners-call) · [Proposed sessions S0–S5](#7-proposed-sequence--owner-free-sessions-after-o1).

**Two rules, inherited from a prior effort that broke both and paid for it:**

1. A milestone closes **only** when its done-condition is met and its gate has passed. Merged code is not a passed gate.
2. If a milestone must ship incomplete, the missing part is **carved out into a new numbered milestone in this file, immediately**, not left as a note. "We'll come back to it" is how a roadmap stops being true.

---

## M0 — Vertical slice

The smallest thing that is genuinely a traffic simulator, end to end. Every layer exists in miniature, so wrong assumptions surface while they are still cheap.

**Scope:** two crossing one-way roads (four Links in `crossing.json`) · one connector each way · one fixed-time signal (two heads, one program each) · one vehicle input per road · Wiedemann-style car-following · a native 2D view showing vehicles as dots · a headless run that prints average delay.

**Done when:** `trafficsim-desktop` shows vehicles accelerating, queueing at red, and discharging at green in a way a traffic engineer recognises as plausible — and CTest proves the same seed produces the same run on a fixed engine/toolchain.

**Where that observation is made (M1.24):** open `data/scenarios/crossing.json` in the editor and use Run/Pause/Step/Reset there. Until M1.24 this said the same thing about a separate M0 window; that window is gone, the observation is not. The editor reads a bare M0 scenario, steps the unchanged core, and its run status shows the mean trip delay and the safety-clamp count the retired window showed. `scenario-run-ui` pins this path to the `trafficsim-cli 42` baseline — 31 completed trips exactly, mean delay 29.249359418430977 to within 1e-7 — so an engineer judging plausibility in the editor is judging the same run the CLI prints. **The gate itself is unchanged and still open:** no automated test closes it, and the plausibility judgement is the owner’s to make.

**Explicitly not in M0:** lane changing, editing anything, saving anything, LOS, multiple seeds, priority control. The slice is about the *shape*, not the feature set.

---

## M0.1 — Native C++ migration (D15)

**Status:** implemented 2026-09-11; Linux native/desktop checks passed then. Windows desktop suites have run in `native.yml` since `e57b1b5` (2026-09-15). Owner M0 acceptance remains open.

**Technical scope:** C++20 core/model/evaluation and CLI, CMake/CTest, a Qt Widgets desktop (originally a separate harness window; the editor since M1.24), strict JSON loading, four TypeScript baseline fixtures, and native developer checks. The former TS application remains in Git history. All new application development is C++.

**Done when:** core/network behaviour passes the saved baseline comparisons and native regressions; the desktop compiles and passes Run/Pause/Step/Reset/seed/language smoke tests (in the editor since M1.24, which retired the separate harness window); and the native build/run instructions are usable. This does not close the owner's M0 plausibility gate, the M1 editor or M7 installer.

---

## M1 — Network editor

The Vissim modelling surface, natively: links are first class, connectors are real objects, junctions are not something the user places.

**Status:** M1.1–M1.16, M1.18–M1.21.1, M1.22.1–M1.22.2 and M1.24–M1.27 (with M1.26.1) are implemented, including the carve-outs M1.3.1, M1.5.1, **M1.11.1** and **M1.12.1**; M1.17 was reverted. **M1.22 and M1.23 are open.** **M1.12.2 is closed** (the reported miter bulge was measured along the cross-section and is not a defect) and **M1.12.3 is closed by M1.19**. The owner's requirement that a Connector's lanes meet the Link lanes they are assigned to ran M1.17 (reverted) → M1.18 (flush) → **M1.19** (each lane on the lane it feeds); D80 (2026-09-28) has since removed M1.18's slide and M1.19's re-solve: the four-point P1–P4 mouth (`reference/CONNECTOR_FOUR_POINT_MOUTH.md`, D74/D79/D80) pairs lanes by index at both ends. **M1.20** gives the Connector its own position. M1.7's owner acceptance: **M1 usability accepted by owner ruling on 2026-09-25 (D49)** after one timed attempt (9 min 40 s, no assistance, save/reopen exact) that did not include turn pockets or an aerial image — see `plans/M1_ACCEPTANCE.md` for what it did and did not show. M0 plausibility remains open.

M1.1–M1.6 and M1.8–M1.10 are implemented and their full bodies are in [`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md); each keeps its heading and a status line here so the sequence stays whole.

**Sub-milestones below are in numeric order, and a carve-out made under rule 2 is filed at its number rather than at the end** — M1.11.1 inside M1.11, M1.12.1 and M1.12.3 inside M1.12 — where a session looking for unfinished work will look for them.

**Done when:** an engineer draws a four-leg intersection with turn pockets from scratch, over an aerial image, in under 10 minutes, without reading documentation — and reopening the file gives back exactly what they drew.

**Includes, because D7 makes it non-optional:** left-hand / right-hand traffic as a project setting that actually drives connector and conflict geometry. Retrofitting this later touches every geometry routine.

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

Implemented: lane-to-lane creation, editable interior curve points, straight/curve reset, reference-safe deletion and retargeting, shared-endpoint maintenance. Drawing curves are sampled polylines; D107 runtime paths evaluate the existing cubic directly, without swept-path validation.

### M1.5 — Inspection and diagnostics

Implemented: Links/Connectors/Signal-heads tables with two-way selection, multi-selection, delete-many as one transaction, and structured diagnostics that name and jump to an object. Draft validity blocks an edit; runnability only informs (D18b).

### M1.5.1 — Demand object tables and editing

Implemented: typed authoring demand replaces the untyped JSON field. Routes, inputs, programs and heads have validated atomic commands, dialogs and tables. Compositions and turning proportions remain M2.

### M1.6 — Complete persistence workflow

Implemented: atomic bounded save/open, asset validation, 15-second dirty-revision recovery copies, per-window locks, explicit catalog embedding. Schema 1 and bare M0 files load without changing IDs; unknown future versions are rejected.

### M1.7 — Run handoff and owner acceptance

Run handoff implemented: compile one document revision with resolved catalogs into a detached network/scenario snapshot. Unsupported features are exposed before Run; successful edits invalidate a run. The engine's existing capability guards remain.

**Owner acceptance: accepted by owner ruling, 2026-09-25 (D49)** — recorded, with what the attempt did not show, in [M1_ACCEPTANCE.md](plans/M1_ACCEPTANCE.md). The M0 plausibility gate remains open.

---

### M1.8 — Run inside the network editor

Implemented: Run/Pause (F5), Step (F6 or Space), Reset, seed and playback speed inside the editor; vehicles and fixed-time heads render over the drawn network and the status names the revision and seed. Every step uses the unchanged core and the not-yet-validated marker stays visible. **Explicitly not in M1.8:** movement results, control delay, LOS, or any relaxation of D5.

### M1.9 — Network Objects sidebar, Vissim gestures and shortcuts

Implemented: a permanent Network Objects sidebar; Ctrl+right-drag opens link/connector dialogs; Ctrl+right-click creates demand/control or inserts a geometry point; Ctrl-click extends selection and Ctrl+left-drag duplicates with fresh IDs; Delete removes objects, Ctrl+Delete a vertex; tool shortcuts, Tab overlap cycling, Ctrl+B background, Ctrl+Shift+O tables. **Done when** a daily Vissim user draws the four-leg intersection without searching for controls — an owner test that automated gesture checks do not replace. **Explicitly not in M1.9:** new parity object types, editable table cells.

### M1.10 — Levels and display types

Implemented: links and connectors persist a level and named display type; rendering, overlays, hit testing and visible-level filtering use level order, and Tab can select an overlapping lower object. Levels and styles live in `data/levels/` and `data/display-types/`, so adding a style needs no C++ change; unknown style IDs keep their value and use the default appearance. **Explicitly not in M1.10:** 3D, or any simulation effect from elevation — a drawn flyover adds no right-of-way, merging or crossing-conflict logic.

### M1.11 — Body attachments and direct lane resizing

**Implemented** — Link and Connector creation by Ctrl+right-drag, body attachments (schema 3) and side-handle lane resizing. Body in [`archive/ROADMAP-M1.11-M3.1-implemented.md`](archive/ROADMAP-M1.11-M3.1-implemented.md).

### M1.11.1 — Compile interior attachments into runtime lane sections

**Implemented** — `runtimeSections` cuts lanes at attachment stations so interior diverges and merges run (merges via M3.1); a cut within 0.2 m still reports `UNSUPPORTED_CONNECTOR_POSITION`. Body in [`archive/ROADMAP-M1.11-M3.1-implemented.md`](archive/ROADMAP-M1.11-M3.1-implemented.md).

### M1.12 — Fixed lane edges, road markings and selection/copy gestures

**Implemented** — fixed lane edges, road markings, two-sided handles (schema 4) and Ctrl-click/Ctrl-drag selection and copy. Timed acceptance: M1 usability accepted by owner ruling (D49), not by the written exercise. No owner check of these gestures on Windows is recorded; their UI suites (`editor-gestures`, `editor-attachments`, `tables-ui`) run on Windows in CI (`native.yml` `windows-desktop`). Body in [`archive/ROADMAP-M1.11-M3.1-implemented.md`](archive/ROADMAP-M1.11-M3.1-implemented.md).

---

### M1.12.1–M1.21.1, M1.24–M1.25 — Completed geometry, authoring, audit and editor iterations

**All closed** (M1.17 by revert, replaced by M1.18; M1.12.3 by M1.19); M1.22 and M1.23, which fall between them in number, are open below. The originals and what none of them claims about Vissim parity are in [`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md), except M1.17's and M1.18's full bodies: the archive holds stubs for those two, and the bodies are in Git history (`bcead5d:docs/ROADMAP.md`). In brief: a Connector carries its own `laneWidths` and `laneMarkings` in schema 6, `connectorLaneWidths` the single place a width is decided (M1.12.1); the 24% miter "bulge" measured along the cross-section, where a mitered corner's diagonal is `width / cos(φ/2)` by construction, so `offsetGeometry` was not changed (M1.12.2); metre attachment stations (M1.13), intermediate points (M1.14), names (M1.15), group moves (M1.16), flush mouths (M1.18), lane-aligned mouths (M1.19, which closed M1.12.3 and left `TIGHT_CONNECTOR_RADIUS` alone) and independent Connector placement with off-Link cleanup (M1.20); schema 7's supported subset of the owner's specifications, where an unsupported field fails on load rather than vanishing on save (M1.21); and the lifecycle audit's wrong-side retargets, pathological mouths, physical range picking and coalesced releases (M1.21.1); the one-window editor, whose run status carries the CLI's own delay figure (M1.24); and demand drawn by pointer (M1.25). M1.17's wedge remains reverted, M1 usability was accepted by owner ruling (D49; `plans/M1_ACCEPTANCE.md` keeps what the attempt did not show), and none of this closes the supplied target specifications.

### M1.22 — Remaining authoring and interaction requirements

**Open.** Link spline/arc construction and curve parameters, extend/merge and safe referenced reversal; tapered cross-sections, shoulders/median/sidewalk display; per-Link driving-side semantics; snap priorities, alignment/angle constraints; layer locks, multi-property inspector, context menus, shortcuts and accessibility. History, nudging and rotation are implemented below. **Gate:** command/reference roundtrips plus both-side gesture tests and keyboard-only owner exercise. Settle the conflicts in SPEC_AUDIT before changing geometry or gestures.

#### M1.22.1–M1.22.2 — History, keyboard editing and selection rotation

Implemented: a bilingual History dock with named Undo/Redo and saved-state markers, arrow-key nudging through the group-move command, level-filtered selection, and Alt-drag rotation with a pivot/angle preview, Shift steps and an exact-angle dialog — all preserving internal Connector shapes, stations, lane metadata and carried heads in one Undo transaction. Full entries in [`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md). These do not close M1.22: geometry/snapping tools, custom pivots, layer locks, bulk inspection and the keyboard-only owner exercise remain open. M1 usability was accepted by owner ruling (D49), not by the written timed exercise.

### M1.23 — Interchange, document workflow and measured rendering

**Open.** Native-to-GeoJSON/CSV/PNG exports; GeoJSON/OSM/Shapefile import and CRS mapping; recent files/tabs, spatial indexing/culling/LOD and optional renderer acceleration. Competitor-format imports and 3D require an explicit scope revision before implementation. **Gate:** known-coordinate import/export fixtures, multi-document recovery isolation and a reproducible real-network benchmark. Do not claim 10k/100k-object performance in advance.

### M1.26–M1.27 — Carriageway demand, per-lane shares, optimization · **CLOSED**

Full entries in [`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md) (moved 2026-09-24). **M1.26:** a route names Links and Connectors and is compiled per lane; an input is the Link total. **M1.26.1:** optional `laneShares` (D32). **M1.27:** build, redraw and engine optimization, and the counted gesture walkthrough. **Gate still open for M1.26:** the keyboard-only equivalents of the route and vehicle-input gestures (NEXT.md). The owner's timed exercise in `plans/M1_ACCEPTANCE.md` was ruled on by D49: M1 usability accepted by owner ruling, not passed as written. Whether "CLOSED" stands with that gate open is the owner's call (see [review §4, O5](#4-verified-but-the-owners-call)).

---

## M2 — Demand, run, first numbers · **GATE**

Vehicle inputs per interval, compositions, turning proportions. Press Run, get average delay and queue per movement.

**Done when:** the M1 intersection, loaded with counted volumes, runs and produces a delay table.

**GATE — the honesty check.** Before M3 starts, a practising traffic engineer completes a small **real** study in this tool, and the result shows whether the tool is usable for real engineering work. No criterion compares it with another tool (C2 withdrawn, D51; C4, D52).

> **This gate is currently performed by the project owner alone (D8), which makes it weak** —
> the person judging is the person who chose to build an engine. It is therefore run as a
> **pre-registered** test: **the pass/fail criteria are written into this file and committed
> before any M2 implementation begins.** Criteria decided afterwards are not a test. A pass
> under these conditions means "not disproven", never "confirmed". Recruiting outside
> engineers later strengthens the gate and is never wasted effort.

**Pre-registered criteria — ratified by the owner on 2026-09-24, re-registered the same day before any gate observation (D38), and again on 2026-09-25, still before any gate observation, when the owner withdrew C2 (D51) and C4 (D52)** (record in [`plans/M2_GATE.md`](plans/M2_GATE.md)):

- **C1 — Completion.** One real study (signalised, protected phasing, counted 15-minute volumes, the owner's timing plan) completed end to end: network over its aerial image, volumes, composition, timing, Run, per-movement delay and queue table. Fails on hand-edited JSON, a code change during the study, or outside help.
- **C2 — withdrawn (D51).** No test times TrafficSim against Vissim or any other tool.
- **C4 — withdrawn (D52).** No test compares TrafficSim's delays with Vissim's or any other tool's.

**Pass = the owner judges it passed (D51)**, on C1's study — reported as *not disproven*. (C0/C3 withdrawn by D38, C2 by D51, C4 by D52; the rest keep their numbers so old records resolve.)

- If the owner judges it failed, the tool is not yet usable for the job it exists for — see `PROBLEM.md` §7.1 — and M3 does not start until what failed is fixed and the gate re-run.

---

### M2.0–M2.6 — Slices (detail in [`plans/M2_PLAN.md`](plans/M2_PLAN.md) §4)

**M2.0** preconditions: the four-leg fixture, same-station cuts, dropped-lane advisory, and **M2.0.1** — Connectors meeting at a lane's start get M3.1's derived rule (D35; this extends M3.1, it does not start M3). **M2.2** time-varying volumes · **M2.3** vehicle compositions · **M2.4** static turning proportions (moved here from M2.1, D37) · **M2.5** movement delay and queue, one run (implemented 2026-09-24, D39/D40) · **M2.6** the owner's gate study. Amber stays red until M4 (D36). **Status 2026-09-25:** M2.0, M2.0.1, M2.2, M2.3, M2.4 and M2.5 implemented. M2's done-condition (the M1 intersection with counted volumes runs and produces a delay table) is met in code. **M2.6: the gate passed by the owner's judgment on 2026-09-25 (D51, D53)** — recorded in `plans/M2_GATE.md` as *not disproven*. M2.1 and M2.7 stay open; M3.2 implementation began 2026-09-25 (M3.2.2a, D54).

### M2.1 — Link/lane/Connector behavior and demand extensions

**Open.** Resolve vehicle-owned desired speed versus Link limits/factors, then distributions, inheritance, road classes, lane types/restrictions, and partial/dynamic/positioned routing decisions (compositions and the static decision moved to M2.3/M2.4, D37). Persistence must not imply runtime support; add explicit capability guards. **Gate:** validated distributions and references, deterministic sampling, old seed fixtures unchanged with defaults, observable runtime effects for every exposed behavior parameter. M2's pre-registered owner gate passed on 2026-09-25 (D53).

#### M2.1.1 — Routeless inputs and placed routing decisions · **Implemented 2026-09-24** (D42, D43)

Owner request. An input on a Link needs no route; a decision placed on a Link sends routeless vehicles to destination Links by relative flow; everything expands at compile time into static routes (SIMULATION.md). Still open in M2.1: a decision's station along the Link and partial/dynamic decisions (per-interval flows came in M2.1.2). Proportions after the entry Link waited for lane changing; since M3.2.8c (D93/D94, 2026-10-01) a decision after the entry Link holds its proportions where lanes are served, by lane changes on its own Link, and a lane with no same-entry path beside it stays lane-fixed (A43).

#### M2.1.2 — Per-interval turning proportions · **Implemented 2026-09-24** (D45)

Owner choice for the next engineering item. A routing decision takes counted turning volumes per interval (pasted per row in its dialog), schema 12; expanded at compile time per interval piece. The interval is chosen by network entry time, not the time at the decision. Still open in M2.1: a decision's station along the Link, and partial/dynamic decisions.

#### M2.1.3 — Recognition at an authored station · **In progress** (D119)

Owner authorized a click-positioned decision on 2026-10-06. Schema 20 and the
[station contract](reference/POSITIONED_ROUTING.md) define passage-time selection,
source/awareness separation, discrete recognition boundaries and legacy opt-in.
Automated platform checks and owner appearance review remain independent gates.
Multiple points on one Link and continuous-time substeps are outside this slice.
M2.1 stays open; partial/dynamic routing is not closed by this implementation.

### M2.7 — Signal heads by pointer; fixed-time Signal Controllers · **Open** (owner request, made before M2.6)
M2.7a (head = stop line, placed at the pointer's station, D47; by Ctrl+right-click since D84) and M2.7b (controllers, signal groups, schema 13, D48) implemented 2026-09-25, before the gate. The owner's use in M2.6 was to close it; `plans/M2_GATE.md` does not record whether that use met the done-condition. Detail and done-condition: [`plans/M2_PLAN.md`](plans/M2_PLAN.md) §4.

---

### M2.8 — Demand correctness and authoring improvements · **In progress**

Owner requested Demand-only improvement on 2026-10-05.
[DEMAND_IMPROVEMENT.md](plans/DEMAND_IMPROVEMENT.md) specifies validation, lossless and
explicit interval editing, and compiled preview (slices 1–4, PR #106). Slice 5 adds
[schema-18 project catalogs and editing](reference/DEMAND_CATALOGS.md); slice 6 adds
[schema-19 time/type rules](reference/DEMAND_TIME_TYPES.md). Existing M2 gate remains
observed; this extension needs its own automated and owner UI checks. Reporting,
LOS, warm-up and multiple-seed summaries belong to M5.

## M3 — Right-of-way: conflict areas and priority rules

**The right-of-way model an engineer controls.**

- Conflict areas as **editable input**: at each conflict point, choose which movement yields, or make it undetermined.
- Priority rules with real **gap time and headway in seconds and metres**.
- Stop and yield control.
- Signal heads placed **anywhere on a link**, not only at a stop line.

**Done when:** an unsignalized T-junction with a minor-road left turn produces plausible, tunable minor-road delay that responds correctly to changing the gap time.

**Preparation (2026-09-24):** [M3_PLAN.md](plans/M3_PLAN.md), [M3_CONTRACT.md](reference/M3_CONTRACT.md) and [M3_ACCEPTANCE.md](plans/M3_ACCEPTANCE.md) specify the ordered work and evidence. Written before M2.6; the M2 gate passed on 2026-09-25 (D53) and runtime work began with M3.2.2a (D54). Interior signal positions already worked in the model/runtime; M3.2.6a (D64, A21) audited the authoring/interaction path rather than adding a second signal-position mechanism.

---

### M3.1 — Merge priority by gap time and headway

**Implemented, and it does not close M3** — derived merge priority by gap time and headway (`PriorityRule`). M3's done-condition is not met by this milestone. Body in [`archive/ROADMAP-M1.11-M3.1-implemented.md`](archive/ROADMAP-M1.11-M3.1-implemented.md).

---

### M3.2 — Lane changing and crossing-conflict control

**Open.** Explicit priority rules/conflict areas, lane-change distances/emergency stopping, cooperation, visibility and calibrated gap acceptance. Specify signal/right-of-way interaction without disabling collision constraints; account explicitly for any removed blocked vehicle. **Gate:** controlled merges, diverges and crossing conflicts, congestion/no-overlap regression, deterministic replay and the M3 owner exercise; scientific claims remain gated by M6.

#### M3.2.1-M3.2.9 — Ordered implementation slices

M2 gate passed (D53). Contracts: [M3_PLAN.md](plans/M3_PLAN.md); next: `NEXT.md`. Passing M3.2.7 alone does not close M3.2; no gate result is inferred; the not-yet-validated marker remains.

| Slice | Scope | Status / gate |
|---|---|---|
| M3.2.1 | Contracts and acceptance design | Prepared; documentation only, no runtime claim |
| M3.2.2a | Waiting lines, conflict areas and priority rules: model, schema 14, commands, one effective-priority resolver (D54) | **Implemented 2026-09-25**; A01–A04, A06–A08 tested; every authored area Run-blocked until M3.2.3 |
| M3.2.2b/c | Reference lifecycle (A05, D55); a waiting line on a preceding Link and crossing-coverage check (D56) | **Implemented 2026-09-25**; `rightofway_lifecycle.*`, `rightofway_resolution.*`, curved Link, both driving sides |
| M3.2.3a/b | Crossing admission runtime (D57): gap time/headway, grants held until the rear clears, receiving space, swept check, sink clearance; sides over section cuts, chained zones admitted atomically (A15), authored merges run on their compiled rules (D58) | **Implemented 2026-09-25**; A09–A17, A25 in `conflict_zone.*`, `conflict_chain.*`; `rightofway_runtime.*` |
| M3.2.3c | Receiving space shared by same-tick requests behind one standing leader; authored merges on the zone solver (the major waits for an admitted minor); hold cycles between zones refused, acyclic role mixes run (D59) | **Implemented 2026-09-25**; `conflict_chain.*`, `rightofway_runtime.*`; derived merges stay on M3.1 rules, queue gridlock not prevented |
| M3.2.4a/b/c | Conflict-area editor. **a** (D60): Conflict areas tab, add-crossing / take-over / restore / edit / delete, canvas display, Problems jump, Run protection note, en/th — **implemented 2026-09-25**. **b** (D61): Conflict area tool (`A`) — click picks, Ctrl+right-click (a plain click until D84) or `P` cycles priority, drag a waiting line; yielding side hatched; save/reopen through the UI — **implemented 2026-09-25** **c** (D68, the owner's ruling): automatic conflict areas — every at-grade overlap a passive area, every merge with its derived priority, derived and never stored; Ctrl+right-click or `P` authors one (D84), Delete makes a crossing passive again — **implemented 2026-09-26**. D72: one area per overlap piece, only the joined lane is a Connector's mouth, drawn 0.3 m inset (display only) — **implemented 2026-09-27**. D86: an authored area follows its overlap after any drawing edit and is removed with its rule and Stop/Yield when the overlap goes — **implemented 2026-09-30**; the owner's look at both is pending (NEXT). | `rightofway_editor.*`, `priority-ui`, `priority-canvas`; c: `automatic_conflict.*`, `conflict-auto-ui`, `conflict_follow.*` |
| M3.2.4d | Measured overlap polygons/spans, inset directional-band display/picking, classified mouths (D115/D117) | Implemented 2026-10-06; engine rules and stored extents unchanged. Native CI and owner desktop review are separate gates; see NEXT. |
| M3.2.4f | P3–P4 directional mouth continuation, 0.5 m normal rail offsets and connected mixed-kind group controls (D118) | Implemented 2026-10-06; Branching remains read-only, group changes atomic with new merge-cycle refusal. Native CI/owner review remain open; no solver/schema change. |
| M3.2.4e | Merge admission/clearance covering the complete physical mouth overlap | Planned separately from M3.2.4d; write station/receiving-space and no-simultaneous-occupancy acceptance rows before changing the D50/D59 solver. No swept-body or validation claim from display polygons. |
| M3.2.5a/b | Stop/Yield. **a** (D62): `StopControl`, schema 15, per-vehicle stop service in core, signal composition, lifecycle — **implemented 2026-09-25**. **b** (D63): Stop/Yield on the conflict dialog and table, line marks on the canvas, en/th, save/reopen; one waiting line per lane — **implemented 2026-09-25** | `stop_control.*`, `stop_control_model.*`, `priority-canvas` |
| M3.2.6a/b/c | Signal positions and queue counters. **a** (D64): A21 evidence, before/on/after a cut, stretch/split/copy, both sides — **implemented 2026-09-25**. **b** (D64): `AuthoredQueueCounter`, schema 16, place-based evaluation, one row per approach — **implemented 2026-09-25**. **c** (D65): counter tool, Queue counters table, en/th, save/reopen — **implemented 2026-09-25** | a/b: `signal_position.*`, `queue_counter.*`; c: `queue-counter-ui` |
| M3.2.7a–d | T-junction evidence (A26). **a** (D66): fixture through commands (both sides, Yield/Stop, blocked exit) and controlled gap/headway/movement cases — **implemented 2026-09-25**. **b** (D66): diagnostic seeded sweep, metadata committed first — **implemented 2026-09-25**. **c** (D67): signal-composition variant on the fixture; congested major road so headway decides, and its headway arm (metadata first) — **implemented 2026-09-26**. **d**: the owner exercise (§3, Windows) — **pending, owner** | a: `tjunction.*`, `tjunction_controlled.*`; b: `docs/evidence/m3.2.7-sweep.*`; c: `tjunction_signal.*`, `docs/evidence/m3.2.7c-headway.*`; d open |
| M3.2.8a/b/c | Behaviour ([M3_8_CONTRACT.md](reference/M3_8_CONTRACT.md)). **a** (D69): commitment at a waiting line — a driver who cannot stop at `maxDeceleration` ignores headway/gap time, never occupancy, Stop, receiving space or the swept check; zones and derived merges — **implemented 2026-09-26**. **b** (D71, the owner's ruling): mandatory lane changing, Vissim-style — volume on every entry lane, a stub changes before its dead end at `comfortableDeceleration`, plus one cooperation rule (hold back for a vehicle waiting at its dead end) — **implemented 2026-09-27** (Windows; Linux/GCC replayed its evidence digit for digit, 2026-09-30). **c**: the remaining lane-change behaviour, rows before code — discretionary changes, visibility, `laneChangeDistance`, a between-lanes state — open; lane changes at a downstream routing decision (D93, A40–A46) **implemented 2026-10-01** (A44 byte comparison on Windows/MSVC only; tests green in Linux CI since PR #84; free walk with no decision stays lane-fixed: no destination); cooperative braking with a deceleration parameter and look-ahead (D90, A36–A39) **implemented 2026-10-01** (Linux; 40-seed M2.6 numbers identical on Windows/MSVC, 2026-10-02); discretionary lane changes (free lane selection, acceleration-gain threshold, stricter trailing safety) — contract D95, rows A47–A55, **implemented and off**; the hold after a change (D101, A56–A58) **implemented and off**; A53 rewritten by D101 **fails** on four-leg and M2.6 (≈5% of changes return within 10 s at threshold 1.5, hold 3 s) — off by the owner's choice (ii), 2026-10-02 (D102); route (i) if reopened (NEXT) | a: `commitment.*`, `docs/evidence/m3.2.8a-*`; b: `lanechange.*` (A27–A35), `docs/evidence/m3.2.8b-mandatory.md`; c: measurement steps 1–5 (D87–D89, `docs/evidence/m3.2.8c-*.md`), cooperative braking `lanechange.*` (A36–A39), `docs/evidence/m3.2.8c-cooperative-braking.md`; calibrated gap acceptance still requires M6 evidence |
| M3.2.9a/b/c | Lane correspondence across a Connector (D73, the owner's ruling). **a**: one-to-one pairing over the narrower end, at most one lane added/dropped per side, `laneChangeSide` (schema 17), the added lane is the tapering one — **implemented 2026-09-27** (Windows desktop; Linux in CI). **b** (D74): each divider ends on its own Link boundary point on the mouth; rails bend onto P1/P4; the 1,512-case probe is `mouth_sweep.*` — **implemented 2026-09-27** (Windows desktop; Linux in CI). **c**: "Lane change side" (Kerb/Left/Right) in the Connector Inspector, enabled only for a one-lane difference, one undoable edit; the lane tabs stop at a difference of 2 (no message, owner's call) — **implemented 2026-09-27** (Windows desktop; Linux in CI) | a: `lane_correspondence.*`; b: `mouth_sweep.*`; c: `connector-ui` (lane-change side step), `editor-attachments` (capped drag) |
| M3.2.9d–h | Connector end fixes (owner requests). **d** (D75): a range across more than a two-lane difference is created narrowed, centred on the lane the drag ended on — **implemented 2026-09-27** (Windows desktop; Linux in CI). **e** (D76): dividers find their mouth point like P1/P4 — **implemented 2026-09-27** (Windows desktop; Linux in CI). **f** (D77): end grips on the Link range centre — **implemented 2026-09-27** (Windows desktop; Linux in CI). **g** (D78): moving an end along the same lanes keeps the curve — **implemented 2026-09-27** (Windows desktop; Linux in CI). **h** (D79): one P1–P4 pairing at every angle — **implemented 2026-09-28** (Windows desktop; Linux in CI); D80 (2026-09-28) replaced its construction: a centred whole-carriageway axis, lane-index pairing at both ends, no square/slide/reach fallbacks — the owner's Windows review is pending (NEXT) | d: `lane_correspondence.*`, `editor-gestures`; e: `mouth_sweep.*`; f: `mouth_sweep.an_end_grip…`, `editor-attachments`; g: `lifecycle.sliding_an_end…`, `editor-attachments`; h: `mouths.*`, `mouth_sweep.*`; D107 (2026-10-04): direct existing cubic runtime equation, integrated/inverted arc stations; D106 midpoint guides are display only, `equation.*` / `lane_centres.*`; desktop owner look pending |

---

### M3.2.8a.1 — Remaining moving merge and source-clearance clamps

**Open (D105/D108, 2026-10-04).** D105 removed stationary zero-allowance false starts. **M3.2.8a.1a source first-step clearance is implemented (D108):** preserve the sampled arrival in its queue until ordinary following/integration from rest fits the snapshot leader clearance. Three current D107 source clamps disappear in 120 stress runs; 21 moving minor clamps remain. The earlier D105 geometry had different counts, retained as historical evidence. [Original diagnosis](evidence/m3.2.8a-clamps.md); [source verification](evidence/source-first-step.md).

**Scope:** define how a minor approaching a merge anticipates the shared leader's standstill buffer before its span appears, and how a source insertion fits its first acceleration step into a small positive clearance. Keep the 1 m derived setback (D50), buffer and maximum deceleration until an explicit new contract says otherwise; do not suppress events or relax collision checks to remove counts.

**Gate:** contract and failure-first regression cases before code; trace every remaining cause; no body/swept crossing overlap or unreported excessive braking; accounting and same-build replay; before/after movement comparison over at least 40 seeds. No clamp-free claim or M3/M6 closure follows from D105 alone.

---

### M3.3 — Driving behaviour library and models

**Status:** M3.3.0 contracts delivered (D120); M3.3.1a queue discharge/startup observer and CLI delivered (D121), no engine behavior changed. **Scope/gates:** M3.3.1b1 type selection/CLI controls delivered (D122); M3.3.1b2a captured input hashes/shared-prefix recognition delivered (D123), with LF/CRLF checkout regression coverage; M3.3.1b2b1 proven lateral/source passage tracking delivered (D124); M3.3.1b2b2 rank-scoped remap invalidation and same-tick source-sink type identity delivered (D125; BA05 focused evidence, native CI per PR; [contract](reference/DISCHARGE.md)), M3.3.2a owned library/class/road-assignment storage and editing delivered (D126; BA06–BA09 focused evidence; Run refuses assigned roads), M3.3.2b compiled front-segment prototype selection delivered (D127; BA10–BA18 focused), M3.3.2c library dialog and bulk road assignment delivered (D128; BA19/BA20 automated, owner visual review open), M3.3.3a W74 equations/parameters/traits/state/model-switch contract written (D129; [W74](reference/W74.md)); W74 implemented and running, not validated (D130–D133, BA21–BA29 focused evidence; clamps traced), editable in the Driving behaviours dialog (D134); Results shows queue discharge and the safety-clamp list (D135). Next: a cited `w74` preset or the M3.3.3b W99 contract before code. [Delivery rows](plans/DRIVING_BEHAVIOUR.md) and [interface](reference/DRIVING_BEHAVIOUR.md) define evidence; native checks and owner reviews remain distinct. M0/M6 gates, D102's off state and legacy baselines remain unchanged; signals/lateral/batches retain their own milestones.

## M4 — Signal control

Controllers, signal groups, programs, fixed-time and actuated, detectors, ring-barrier.

**Done when:** an eight-phase two-ring, two-barrier plan is built from a real timing sheet in under 8 minutes with zero validation errors, and runs.

---

### M4.1 — Detectors and controller integration

**Open.** Detectors/DCP authoring and events, signal groups/controllers beyond M2.7b's fixed-time ones (D48), actuated/adaptive logic, phase validation and external-control interfaces. External integrations need explicit protocols and deterministic recorded inputs; no wall-clock dependency in core. **Gate:** passage/occupancy/aggregation fixtures, controller state-transition tests, dangling- reference cleanup and the M4 real timing-sheet exercise.

---

## M5 — Evaluation and reporting

The reason the whole project exists (`PROBLEM.md` §4).

- Movement-level delay, LOS, queue length, travel time.
- Multi-seed batch runs with means and confidence intervals.
- Report tables that go into an impact study without passing through a spreadsheet.
- LOS thresholds as swappable per-jurisdiction data, never compiled in.

**Done when:** one intersection, ten seeds, one command → a movement-level LOS table with confidence intervals, ready to paste into a report.

**Status (2026-10-08):** the owner answered O1 with (a), M5 ahead. **M5.2 (S1) delivered:**
`trafficsim-cli --project F --seeds A-B` gives per movement and approach n, mean, SD and the 95 %
Student-t half-width, with per-seed accounting and the build's commit (D136, D137). **S2
delivered (D138):** Simulation ▸ Run N seeds… shows the same batch in Results. Still open: LOS
(thresholds as data), copy/export of the batch from Results (S3). **O8 answered and delivered (D139):**
an author-declared warm-up and measurement window. On O3 the owner asked whether HCM belongs at
the analytical level; the recommendation is HCM as M6's computed reference beside the simulation,
not letters on simulated delay; O3 stays open.

---

### M5.1 — Additional network objects and evaluated outputs

**Open.** Evaluation nodes/stop lines, movement measurements/overlays and reports. Parking, transit stops, crosswalks and multimodal behavior from the supplied documents require their own runtime and calibration contracts, not merely stored object types. **Gate:** event-accounting and known analytical scenarios, reference cleanup and multi-run output checks. Keep completed-trip delay distinct from HCM control delay/LOS.

---

## M6 — Calibration and validation · **GATE**

**Done when:** the engine reproduces published benchmark results — capacity and delay for a signalized approach, gap-acceptance capacity for an unsignalized minor movement — within a stated tolerance, and the tolerance is published in the docs and shown in the app.

**This is a hard gate.** Numbers from an unvalidated engine must never reach a regulator. Until M6 passes, every results screen carries a permanent "not yet validated" marker.

---

## M7 — Desktop packaging

Offline install, native file dialogs, no server required.

**Done when:** a non-technical user installs from a single file on Windows and macOS, opens a project by double-clicking it, and works with no network connection.

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

---

## Review record — 2026-10-03

**Reading this dated record:** review sections §1–§10 refer to the numbered subsections below. Findings, counts and test results describe `eeb9c5c` on 2026-10-03; they are not a fresh verification of HEAD. O1–O10 and S0–S5 remain proposals unless a later decision says otherwise. Combining these documents on 2026-10-04 does not approve them or change any gate.

**Updates since the review:** D104 implements single-run Results CSV export with the CLI's bytes; clipboard copy and batch export in S3 remain open. S4's input-table lane-share display is done (`laneSplit`, 2026-10-03). `NEXT.md` was regrouped on 2026-10-04; its current queue supersedes the old section layout and counts described here. The historical findings below are retained with their date.

A read-only review of `ROADMAP.md`, `NEXT.md` and the status lines that repeat them, at `eeb9c5c`. It has two halves: an **accuracy audit** (does each status match the decision log, the code, the tests and git?) and a **strategic review** (what stands between the product and the `PROBLEM.md` §4 sentence, and in what order?). The owner chose the output: verified factual errors are corrected in this session; **everything that needs judgement is a proposal here, for the owner.** Nothing is closed, re-ordered or carved out by this review, and no gate result is inferred.

**Method.** Six audit slices (M0–M1 closed, M1.22/M1.23, M2, M3, M4–M7 with layout, NEXT and PROGRESS) each compared one part of the documents against `PROGRESS.md`'s decision log, the archives, the code, the tests and `git log`. Every finding was then checked by an independent skeptic told to refute it; the M0–M1 slice had three per finding (documents, code, safety of the fix). 87 findings survived: **63 factual, 17 owner decisions, 7 strategic**; none was refuted outright, but several had their wording corrected. Three independent strategy lenses (the success-sentence walk, risk and effort, dependencies and the owner queue) fed one synthesis, which two adversarial critics then attacked; §7 below carries their corrections. Desktop 77/77 on Windows (MSVC 19.51, Qt 6.8.3, Debug) before any edit; this session changed documents only.

---

### 1. Summary

1. **The front half of the success sentence works; the back half has no product code.** Aerial image, four-leg drawing, counted interval volumes, fixed-time timing and a one-run movement delay table all exist. "Runs 10 seeds", "LOS" and "paste into a report without Excel" do not: `src/runner/` and `src/report/` hold only a README, `data/` has no LOS pack, and the editor has no copy or export (§5).
2. **Effort since M2's gate has gone to M3.2 behaviour and editor work, not to M5.** Of 122 non-merge commits since 2026-09-25, 45 touch `src/editor` or `src/shell`, 12 touch `src/eval` (all M3.2 work but one M2.6 performance change), and **0** touch `src/runner` or `src/report` (§6).
3. **No rule forces that order; `NEXT.md` does.** M5 is not behind a gate that is unpassed (M0's and M3's gates do not precede it), but `NEXT.md` puts five sections of owner looks first and marks engineering work "if asked", and `CLAUDE.md` tells a session not to re-plan. The order is the owner's call (decision O1).
4. **The documents had drifted.** 63 statements were stale or wrong: CLAUDE.md's schema 16 (it is 17), ROADMAP's "M3.2.8c — back to the owner" (the owner ruled, D102), M3's "M2.6 is still unperformed", M4.1 listing fixed-time controllers as open, wrong ctest names, superseded "Linux only"/"Windows only" notes. All are corrected (§3).
5. **Three gaps have no owner at all:** W74/W99 car-following (a `PROBLEM.md` §2 row no milestone builds), the M6 benchmark (Q4, open since 2026-09-10, "decide before M5"), and the evaluation period (warm-up, window, unfinished trips) that any averaged or LOS figure depends on (§4, §8).

---

### 2. Where each milestone stands

| Milestone | ROADMAP says | Evidence shows |
|---|---|---|
| M0 | Gate open | Open since 2026-09-11; owner observation not recorded. A fail triggers D13 (fix or replace car-following) |
| M0.1 | Implemented | Implemented; Windows desktop suites in CI since `e57b1b5` (corrected) |
| M1 | Accepted by ruling (D49) | Accepted by ruling; the written exercise (pockets, aerial image) never passed; no carve-out records that (O5) |
| M1.12 | Implemented | Implemented; no owner check of its gestures on Windows is recorded (corrected wording) |
| M1.22 | Open | Partly delivered: History, nudging, rotation; Connector tapers, route-safe reversal, a fixed shortcut set and Ctrl+K palette exist. Remaining list overstated (O10) |
| M1.23 | Open | Not started; no GeoJSON/OSM/Shapefile/PNG code |
| M1.26 | **CLOSED** | Implemented; its own gate (keyboard-only route/input gestures) is open — contradicts rule 1 (O5) |
| M2 | GATE, no status | Done-condition met in code; gate passed (D53, owner's word, no table on record: `plans/M2_GATE.md`). Heading carries no status (O5) |
| M2.1 | Open | Open; desired speed is one uniform min/max per type; no Link limits, lane types, decision station |
| M2.7 | Open | Implemented 2026-09-25; whether the owner's M2.6 use met its done-condition is not recorded (O4) |
| M3 | Done-condition | Shown on development evidence only; M3.2.7d sheet empty |
| M3.2 | Open | Rows through A46 green; D95/D101 implemented and off (D102); gate names "diverges" with no booked work; "calibrated gap acceptance" in scope but needs M6 (O6) |
| M3.2.9 | Implemented | Connector geometry filed under a right-of-way milestone; no M3_PLAN or M3_ACCEPTANCE rows (O6) |
| M4 / M4.1 | Open | Fixed-time controllers exist (M2.7b); actuated, detectors, intergreens, amber stop-or-go absent |
| M5 | Done-condition only | Not started: no runner, no CI, no LOS, no slices numbered |
| M5.1 | Open | Not started; its scope (transit stops, crosswalks) contradicts "Later … not inline" (O10) |
| M6 | GATE | Not started; no benchmark chosen (Q4) |
| M7 | Not started | `package.yml` builds a portable Windows folder and a Linux tree; no installer, no macOS build, no file association |

---

### 3. Corrections made in this session

Archived whole: [the 2026-10-03 corrections list](archive/ROADMAP-review-2026-10-03-corrections.md).

---

### 4. Verified, but the owner's call

Recommendations are this review's; the choice is the owner's. Grouped so one chat answer can take each line by option letter.

| # | Question | Options | Recommendation |
|---|---|---|---|
| O1 | Does M5 (runner → editor batch → export) go ahead of further M3.2.8c behaviour and editor polish? | (a) yes, NEXT names it next · (b) keep NEXT's order · (c) alternate | **(a).** PROBLEM §2 calls it "the deliverable of the entire job"; D95/D101 shipped switched off |
| O2 | Q4: which published benchmarks define M6's tolerance? Its note says "decide before M5" | (a) HCM analytic references · (b) a published field dataset · (c) local Thai measurements · (d) (a) now, (c) later | **(d)**, from an option sheet a session prepares first (S0). Or override "before M5" explicitly in O1 |
| O3 | LOS: which pack, and may a letter sit on today's whole-route delay? | (a) letters on simulated movement delay — requires amending M5.1's gate line and the Results note, which say it is "not HCM control delay or LOS" · (b) letters only on a section-bounded delay (travel-time sections, D39's "M5 measurement") · (c) another pack too · (d) none before M6 | **(b)**, HCM first (D7). HCM thresholds differ by control type, so movements need a control-type tag |
| O4 | Did the M2.6 study build its timing in the controller dialog with every head placed by pointer (M2.7's done-condition)? | (a) yes → record in `plans/M2_GATE.md`, M2.7 closes · (b) no/unsure → observe it in the sitting | One line from the owner |
| O5 | Closure wording (rule 1) | M2 closed with M2.1/M2.7 carried as open sub-milestones, or open · M1.26 "implemented; gate open" instead of CLOSED · per-approach queue (D40) satisfies M2, per-movement queue to M5 · M1's unpassed written exercise carved or recorded · M2.0's commits before the criteria (`c1b122f`, `4e1567e` before `811e0db`): an allowed exception or a recorded lapse | M2 closed; M1.26 relabelled; queue to M5; record M1's ruling; M2.0 an exception (no gate observation was affected) |
| O6 | What closes M3.2? | Diverges: a numbered slice, or a ruling with an **acceptance-fixture row** (the D98 lab is a development bed, not evidence) · calibrated gap acceptance carved into M6 · M3.2.8c's remainder (visibility, `laneChangeDistance`, between-lanes state, D95 route (i)) into a new milestone · M3.2.9 to an editor milestone · the five T-junction clamps a numbered item or a NEXT note | Carve gap acceptance to M6 and the M3.2.8c remainder out, so M3.2 can close on M3.2.7d and its gate rows |
| O7 | Who owns W74/W99 car-following (PROBLEM §2)? No milestone builds it | (a) a numbered milestone, a new behaviour preset, frozen baselines pinned to the prototype · (b) M6 validates the prototype and PROBLEM §2 changes · (c) Later | **(a)** after M0's observation, before M6. Its screens must say permanently that Vissim-calibrated parameter sets do not carry over (rule 4) |
| O8 | Evaluation period: warm-up, evaluation window, unfinished trips. **Answered 2026-10-08:** a warm-up and window the user declares, no code default (D139, delivered) | No code has a warm-up or window; the M2.6 template starts empty and runs its demand period; delay counts completed trips only, which reads low when a movement saturates | Decide before any LOS column (O3) |
| O9 | Seeds: keep "ten seeds" in M5/PROBLEM §4, or a CI half-width target? | (a) fixed 10 · (b) target half-width · (c) default 10, always show n and half-width | **(c).** The seed-to-seed SD of one movement's delay is unmeasured; D88's 7 s is the SD of a *change* between engine versions and does not size a table. Scenario comparison stays under "Later" (one random stream, so same-seed pairing across scenarios is not common random numbers) |
| O10 | One-line rulings | Close Q2 (D71/D95) · Q5 name, and whether registration (Q6) follows · plain `Tab` → focus as a Vissim departure (no D-row for `ed74268`) · 3D: non-goal (PROBLEM §5) or "Later" · M5.1's transit/crosswalks vs "Later" · amber stop-or-go and intergreens written into M4's scope (D36, `plans/M2_PLAN.md`) · scenario-JSON export from the editor wanted? · canvas below `workspace-ui`'s floor at 1080p/150 % · narrow M1.22's list; refresh `SPEC_AUDIT` (it predates M3.2) before it gates M1.22 · what M1.23's "CSV" means · count sheet typed once vs D46 (the input's volume is the authority by owner request) · NEXT's ordering, and whether a non-gate look may ever close without the owner (only as a new D-row) | — |

---

### 5. The success sentence, step by step

> A traffic engineer opens an aerial image, draws a four-leg signalized intersection, enters
> counted turning volumes, sets the signal timing, runs 10 seeds, and gets a movement-level
> delay and LOS table they can paste into a report — without opening Vissim and without opening Excel.

| Clause | Status | Owner | Gap |
|---|---|---|---|
| opens an aerial image | works | M1 | Never exercised with a real aerial (D49's attempt had none). Images are stored as base64 PNG under a 32 MiB cap; a large photographic aerial may hit it — untested |
| draws a four-leg signalized intersection | works | M1 | — |
| enters counted turning volumes | works | M2.1.2/M2.2 | Inputs and decision counts are typed separately (D46, by design). Motorcycle-heavy Thai counts cannot be represented |
| sets the signal timing | partial | M2.7, M4 | Fixed-time only; amber runs as red (D36), which biases delay and causes clamps |
| runs 10 seeds | **missing** | M5 | One seed per run in the editor (20× playback cap) and the CLI. Seed loops exist only in developer tools, three times over |
| movement-level delay table | works (one run) | M2.5 | Whole-route delay incl. source wait and ≈3 s entry acceleration (D39). Queue is per approach (D40). A route through two junctions has no per-junction row (no nodes) |
| … and LOS | **missing** | M5 | O3, O8, O2 |
| they can paste into a report | partial | M5 | CLI CSV only; no copy/export in the editor |
| without opening Excel | **missing** | M5 | Averaging and CIs are done by hand today |
| without opening Vissim | partial | M6 | Every number carries the not-yet-validated marker until M6; no benchmark |

PROBLEM §2's behaviour rows: conflict areas **partial** (no front/rear gap, safety factor, visibility or red-red status; Vissim parity never measured); priority rules works (deterministic threshold, not a calibrated critical-gap model); signal heads anywhere works; **W74/W99 unowned** (O7); desired speed distributions partial (M2.1, unbooked); node evaluation and multi-run averaging as above. No reduced-speed areas or curvature speed limit exist, which biases turning-movement delay — the quantity M6's signalised-approach benchmark measures.

---

### 6. Where the effort went

Counts are `git log --no-merges --since=2026-09-25` (122 commits) by path; a commit can count in several rows.

| Area | Commits | Note |
|---|---|---|
| `src/editor` or `src/shell` | 45 | Connector geometry (D73–D80), interaction and UI polish (D81–D84, D99–D103) |
| `src/model` / `src/project` | 23 / 18 | Mostly right-of-way and lane-change compile paths |
| `src/eval` | 12 | M3.2 work (queue counters, D71, M3.2.8c diagnostics, D95/D101) and one M2.6 performance change; movement report untouched since `d046c2c` (2026-09-27) |
| `src/core` | 10 | Commitment, lane changes, cooperation (D69, D71, D90, D91, D95, D101) |
| `src/runner`, `src/report` | **0** | Untouched since the 2026-09-11 migration |

Since D39/D40 (2026-09-24), no decision row designs multi-seed reporting, LOS or an M6 benchmark; D88 sets a seed count for development comparisons only. PROBLEM §7's risk 3 ("engine starves the UI") is not happening; its inverse is — refinement is starving the deliverable. The risk on the other side stays real: an M5 table behind a weak editor still loses to Vissim, so the owner's open UI looks (§8) are not to be dropped, only scheduled.

---

### 7. Proposed sequence — owner-free sessions, after O1

Proposals; the owner books them in `NEXT.md`. One system per session, interface first. The critics' corrections are folded in: the Q4 sheet comes first so the recorded "decide before M5" is honoured, and carries no engine figures (choosing a benchmark the engine already matches would undo pre-registration, as D34's "blind" C0 avoided).

| # | Session | Milestone | Interface first | Done when |
|---|---|---|---|---|
| S0 | **M6 benchmark option sheet** (docs only) | prepares Q4 | `docs/evidence/m6-benchmark-options.md`: per M6 benchmark kind (signalised approach capacity/delay; unsignalised minor-movement capacity) ≥ 2 published options — quantity, source, the command that would produce the comparable figure. No engine numbers, no tolerance | PROGRESS Q4 points to it; the owner can answer by letter |
| S1 | **Batch runner and `trafficsim-cli --project F --seeds A-B`** | M5 (number a new slice, e.g. M5.2; M5.1's gate items — event accounting, multi-run checks — apply) | `src/runner/`: `runSeeds(scenario, spec, seeds) → per-seed MovementReport`; `aggregate()` in seed order → n, mean, SD, 95 % half-width per movement and approach. **Provenance:** replace the constant `engineVersion` ("0.2.0-cpp-m0" since `ab3423d`) with a build-stamped commit, and record compiler, seed list and per-seed clamps/pending/active. A seed that gridlocks or leaves trips unfinished is flagged, never silently averaged | Analytic aggregate test; permuted seed list gives the same result; one-seed batch equals today's `--project`; generated = completed + active + pending per seed; baselines untouched; `check_architecture` gets a runner rule with a negative fixture; ARCHITECTURE says where batch formatting lives (`src/report/` as reserved, or `src/project/` beside `movementCsv` — decide and record) |
| S2 | **"Run N seeds" in the editor** | M5 | Shell action over S1's API on a copy of the run snapshot; worker thread, aggregation in seed order (rule 2); Results shows n / mean / ±95 % | UI test: Results rows equal the CLI batch; an edit invalidates; cancel leaves no table claiming N runs; marker and "not HCM control delay or LOS" stay |
| S3 | **Copy table / Export CSV from Results** | M5 | TSV/CSV formatters beside S1's, first line the marker; clipboard and save | Test asserts the clipboard was set first, then that rows parse back; export bytes equal the CLI's; exported numbers C-locale while display cells follow `UI_REDESIGN_AUDIT` §6 — deliberately different, documented |
| S4 | **Input table shows the share it runs** | M1.26.1 | `refreshDemand` reads `laneShares` | The row shows the authored split, not the equal one, when shares are set (today it displays a figure the run does not use) |
| S5 | **Fill the session-fillable M3_ACCEPTANCE §4 rows; diagnose the five T-junction clamps** | M3.2 | Measurement before any fix | Commit/build/platform, fixture hashes and same-build replay rows filled; each clamp has a recorded cause. NEXT's do-not-retry (`comfortableDeceleration` for commitment, D69) is listed as a risk; before/after uses 40 seeds (D88) |

After O3/O8: the LOS pack as data (`data/los/`), a pure delay → letter function keyed by control type, approach and intersection rows; then travel-time sections. After O7: W74/W99 as a preset. Keyboard-only route/input gestures (M1.26's gate) remain owner-free and can slot in anywhere.

---

### 8. The owner queue, in one sitting

NEXT lists about 21 items waiting on the owner; ROADMAP, PROGRESS and the evidence file hold about nine more (strategy-lens counts, not recounted). Only O1–O3 and the M0 and M3.2.7d gates block anything. A session prepares the build and printed sheets, records the owner's words verbatim, and fills in no verdict itself.

- **A — decisions in chat:** O1–O10 by letter.
- **B — gate:** M0 on `data/scenarios/crossing.json` (acceleration, queue at red, discharge at green). First, because a fail triggers D13 before any W74/W99 or M6 work.
- **C — Run view, on the 125 % screen:** D90 (M2.6 East approach), D93 (pocket decision), D102 (lane-change slide), the grid's faintness.
- **D — `t-junction-priority.traffic.json`:** M3.2.7d with the `plans/M3_ACCEPTANCE.md` §3 sheet; D86 (drag, clear, Undo); D72.
- **E — drawing:** D80 at 45–179°; D83/D84 at 100/150/200 %, timing Ctrl+right against left-click for route and counter work (D84's failure condition); the PR #73–#75 cleanup; D96; D100; M1.19/M1.20 and `laneContains`; M1.12's gestures; M2.7's dialog if O4 is "no".
- **After S1–S3:** one rehearsal of the §4 sentence with a real aerial image, recorded with file, commit and table — labelled **partial** while LOS is undecided, so it is not read as §4 or M5 met. Outside engineers remain the way to strengthen the M2 gate (D8: "never wasted effort").
- **Guard:** each session adds at most one owner look, dated, with its failure condition as a yes/no question. NEXT points to ROADMAP and PROGRESS lines rather than moving gate status out of them (ROADMAP stays the authority, rule 1).

---

### 9. Risks the next sessions carry

- **Single-platform evidence** still stands behind several rows (A44's byte comparison on MSVC only; numerical replays are not run in CI). CLAUDE.md forbids reading one platform as both.
- **Gridlock:** queue gridlock is not prevented (M3.2.3c); D50 recorded a template with 445 vehicles never entering. A batch mean over a gridlocked seed is survivor-biased (S1 flags it).
- **`CONNECTOR_PARITY_AUDIT`** §3.5 (a Connector at a lane's start or end draws but does not run) and §3.6 (a zero gap time passes validation) are still live; Vissim itself has never been measured.
- **Thai studies:** motorcycles and per-approach speeds cannot be modelled; both are open questions in NEXT, not booked.

### 10. What this review did not check

No CLI or benchmark run (figures such as the 0.45 s M2.6 hour are quoted; D91 measured 557 ms later); HCM tables and procedures against the publications; whether the sweep tools can be rewired to a runner with byte-identical output; QClipboard under offscreen CI; whether a large real aerial fits the image caps; CI status at HEAD; the owner's M2.6 file and table (not in the repository). Strategy-lens counts other than those in §6 were spot-checked, not recounted.
