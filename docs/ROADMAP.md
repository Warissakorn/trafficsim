# ROADMAP — TrafficSim

Not a schedule. A **sequence**, so that any session can see where it sits and what closes the milestone it is in.

This is the single roadmap and review document. Milestone scopes and gates below remain authoritative; the dated review records evidence and proposals, not additional approvals. `NEXT.md` remains the one live session queue.

Navigation: [M0](#m0--vertical-slice) · [M5](#m5--evaluation-and-reporting) · [M8–M12](#m8m12--multi-level-modelling-micro-meso-macro-d145) · [Reviews](#reviews): [2026-10-09](audits/ROADMAP_REVIEW-2026-10-09.md) (R1–R8) · [2026-10-03](archive/ROADMAP-review-2026-10-03.md) (O1–O10, S0–S5).

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

**Open, narrowed by D143 (R8) to what is unbuilt** (checked in code 2026-10-09): Link spline/arc construction and curve parameters; extending and merging Links; reversing a Link that carries references (`reverseLink` refuses one today); shoulders/median/sidewalk display; per-Link driving side (the Network has one); snap priorities and angle constraints; layer locks; a multi-property inspector beyond bulk behaviour assignment (D128); object context menus beyond the demand menu; shortcut customisation and an accessibility pass. Built and no longer listed: History, nudging, rotation, Connector tapers, route-safe reversal, the fixed shortcut set and the Ctrl+K palette. **Gate:** command/reference roundtrips plus both-side gesture tests and keyboard-only owner exercise. Settle the conflicts in SPEC_AUDIT before changing geometry or gestures.

#### M1.22.1–M1.22.2 — History, keyboard editing and selection rotation

Implemented: a bilingual History dock with named Undo/Redo and saved-state markers, arrow-key nudging through the group-move command, level-filtered selection, and Alt-drag rotation with a pivot/angle preview, Shift steps and an exact-angle dialog — all preserving internal Connector shapes, stations, lane metadata and carried heads in one Undo transaction. Full entries in [`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md). These do not close M1.22: geometry/snapping tools, custom pivots, layer locks, bulk inspection and the keyboard-only owner exercise remain open. M1 usability was accepted by owner ruling (D49), not by the written timed exercise.

### M1.23 — Interchange, document workflow and measured rendering

**Open.** Native-to-GeoJSON/CSV/PNG exports — "CSV" means the object tables (Links, Connectors, inputs, routes, decisions), as defined by D143 (R8); results CSV is M5's and exists (D104, D141); GeoJSON/OSM/Shapefile import and CRS mapping; recent files/tabs, spatial indexing/culling/LOD and optional renderer acceleration. Competitor-format imports and 3D require an explicit scope revision before implementation. **Gate:** known-coordinate import/export fixtures, multi-document recovery isolation and a reproducible real-network benchmark. Do not claim 10k/100k-object performance in advance.

### M1.26–M1.27 — Carriageway demand, per-lane shares, optimization · **M1.27 closed; M1.26 implemented, gate open**

Full entries in [`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md) (moved 2026-09-24). **M1.26:** a route names Links and Connectors and is compiled per lane; an input is the Link total. **M1.26.1:** optional `laneShares` (D32). **M1.27:** build, redraw and engine optimization, and the counted gesture walkthrough. **Gate still open for M1.26:** the keyboard-only equivalents of the route and vehicle-input gestures (NEXT.md). The owner's timed exercise in `plans/M1_ACCEPTANCE.md` was ruled on by D49: M1 usability accepted by owner ruling, not passed as written. D143 (R5) relabelled it: M1.26 is implemented with its gate open until the keyboard-only equivalents exist; M1's written exercise stays recorded as ruled on by D49, not passed.

---

## M2 — Demand, run, first numbers · **CLOSED** (gate passed, D53; closed by D143)

Vehicle inputs per interval, compositions, turning proportions. Press Run, get average delay and queue per movement.

**Done when:** the M1 intersection, loaded with counted volumes, runs and produces a delay table.

**Closed 2026-10-09 by the owner's R5 answer (D143).** The done-condition and the gate (D53) are
met; M2.1, M2.7 and M2.8 continue as open sub-milestones and do not reopen M2. The per-approach
queue (D40) satisfies M2; a per-movement queue belongs to M5. M2.0's commits made before the
criteria were registered (`c1b122f`, `4e1567e` before `811e0db`) are an allowed exception: no gate
observation was affected.

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
[schema-19 time/type rules](reference/DEMAND_TIME_TYPES.md). D142 (2026-10-09, schema 26) lets an
input take its volume from its entry decision's turning counts (owner's answer to O10's count-sheet
item; [§7](plans/DEMAND_IMPROVEMENT.md)). Existing M2 gate remains
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

**Open.** Explicit priority rules/conflict areas, lane-change distances/emergency stopping and cooperation. By D143 (R7), calibrated gap acceptance moved to M6 and the M3.2.8c remainder to M3.4; M3.2 closes on the M3.2.7d owner exercise and its gate rows, with a diverge acceptance-fixture row still to write. Specify signal/right-of-way interaction without disabling collision constraints; account explicitly for any removed blocked vehicle. **Gate:** controlled merges, diverges and crossing conflicts, congestion/no-overlap regression, deterministic replay and the M3 owner exercise; scientific claims remain gated by M6.

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

### M3.4 — Lane-change behaviour refinements

**Open; carved out of M3.2.8c by D143 (R7).** Visibility at conflict areas, a between-lanes state
(a change is instantaneous in the engine), `laneChangeDistance` where changes are measured late
(D87/D89), D95's booked route (i) if discretionary changes are reopened (owner, sitting B2), and
Vissim's cooperative lane change. **Gate:** contract and failure-first rows per item in the
lane-change lab (D98), then acceptance fixtures; prototype baselines change only by a recorded decision.

---

### M3.3 — Driving behaviour library and models

**Status:** M3.3.0 contracts delivered (D120); M3.3.1a queue discharge/startup observer and CLI delivered (D121), no engine behavior changed. **Scope/gates:** M3.3.1b1 type selection/CLI controls delivered (D122); M3.3.1b2a captured input hashes/shared-prefix recognition delivered (D123), with LF/CRLF checkout regression coverage; M3.3.1b2b1 proven lateral/source passage tracking delivered (D124); M3.3.1b2b2 rank-scoped remap invalidation and same-tick source-sink type identity delivered (D125; BA05 focused evidence, native CI per PR; [contract](reference/DISCHARGE.md)), M3.3.2a owned library/class/road-assignment storage and editing delivered (D126; BA06–BA09 focused evidence; Run refuses assigned roads), M3.3.2b compiled front-segment prototype selection delivered (D127; BA10–BA18 focused), M3.3.2c library dialog and bulk road assignment delivered (D128; BA19/BA20 automated, owner visual review open), M3.3.3a W74 equations/parameters/traits/state/model-switch contract written (D129; [W74](reference/W74.md), BA21–BA29 delivered with focused evidence), W74 implemented and runnable in schema 25, not validated (D135–D139; built in parallel with M5 and replayed on top of it 2026-10-09), next a cited preset, then the M3.3.3b W99 contract before code. [Delivery rows](plans/DRIVING_BEHAVIOUR.md) and [interface](reference/DRIVING_BEHAVIOUR.md) define evidence; native checks and owner reviews remain distinct. M0/M6 gates, D102's off state and legacy baselines remain unchanged; signals/lateral/batches retain their own milestones.

## M4 — Signal control

Controllers, signal groups, programs, fixed-time and actuated, detectors, ring-barrier.

**Done when:** an eight-phase two-ring, two-barrier plan is built from a real timing sheet in under 8 minutes with zero validation errors, and runs.

---

### M4.1 — Detectors and controller integration

**Open.** Detectors/DCP authoring and events, signal groups/controllers beyond M2.7b's fixed-time ones (D48), actuated/adaptive logic, phase validation and external-control interfaces. External integrations need explicit protocols and deterministic recorded inputs; no wall-clock dependency in core. **Gate:** passage/occupancy/aggregation fixtures, controller state-transition tests, dangling- reference cleanup and the M4 real timing-sheet exercise.

---

### M4.2 — Amber stop-or-go

**Implemented 2026-10-09 (D147, [AMBER](reference/AMBER.md), [gate evidence](evidence/m4.2-amber.md)); native CI per PR.**
Analytic rows AM1–AM8 cover both outcomes, the 40-seed before/after is recorded, and same-build
replay is byte-identical; the owner closes the milestone. Carved out by D143 (R2): D36 ran amber
as red, which biased every signalised delay an M5 table reports. Contract first: a stop-or-go decision at amber onset (can
the vehicle stop at a stated deceleration before the stop line?), intergreen handling, and
failure-first rows. It moves prototype trajectories, so frozen baselines and BA18 change only by a
recorded decision. **Gate:** analytic amber fixtures, both outcomes; before/after over at least 40
seeds (D88); same-build replay.

---

## M5 — Evaluation and reporting

The reason the whole project exists (`PROBLEM.md` §4). **Next, by owner instruction (D130, 2026-10-08):** slices M5.2–M5.7 in [`plans/M5_PLAN.md`](plans/M5_PLAN.md); O1 (a), O3 (b), O8 and O9 (c) are answered there. W74 (M3.3.3a) was built in parallel and now sits on top of them in schema 25 (D135–D139); it does not block M5.6. M5.2 (batch runner, `--seeds`, 95 % CI) is implemented (D131), and M5.3–M5.6 after it (D132–D134, D141: the editor's Run seeds, Copy and Export); M5.7, the owner's rehearsal, is next; by D143 sessions then take M5.9 (evaluation cool-down, implemented by D146: schema 27, new projects 900 s), M4.2 (implemented by D147), M6.0 (sheet delivered 2026-10-10) and M5.8 (CLI `--compare` implemented by D148; the editor next) ([M5_PLAN](plans/M5_PLAN.md) §3). The M5 gate stays open.

- Movement-level delay, LOS, queue length, travel time.
- Multi-seed batch runs with means and confidence intervals.
- Scenario comparison: base, with-project and mitigated on one seed list (M5.8, D143 R1).
- Report tables that go into an impact study without passing through a spreadsheet.
- LOS thresholds as swappable per-jurisdiction data, never compiled in.

**Done when:** one intersection, ten seeds, one command → a movement-level LOS table with confidence intervals, ready to paste into a report.

---

### M5.1 — Additional network objects and evaluated outputs

**Open.** Evaluation nodes/stop lines, movement measurements/overlays and reports. Transit stops, crosswalks and multimodal behaviour moved to *Later* by D143 (R8); parking is M8 since D145. **Gate:** event-accounting and known analytical scenarios, reference cleanup and multi-run output checks. Keep completed-trip delay distinct from HCM control delay/LOS.

---

## M6 — Calibration and validation · **GATE**

**Done when:** the engine reproduces published benchmark results — capacity and delay for a signalized approach, gap-acceptance capacity for an unsignalized minor movement — within a stated tolerance, and the tolerance is published in the docs and shown in the app.

**M6.0 — benchmark option sheet** (D143, R2): docs only, at least two published options per benchmark kind, no engine numbers and no tolerance, so the choice stays pre-registered. **Delivered 2026-10-10:** [m6-benchmark-options](evidence/m6-benchmark-options.md), kinds S1 (saturation flow and capacity), S2 (delay) and U (minor-movement capacity and gap acceptance). The owner chooses by letter ([OWNER_SITTING](plans/OWNER_SITTING.md) B9); the tolerance is set after that choice and before any comparable run. Calibrated gap acceptance (moved from M3.2 by R7) is validated here.

**This is a hard gate.** Numbers from an unvalidated engine must never reach a regulator. Until M6 passes, every results screen carries a permanent "not yet validated" marker.

---

## M7 — Desktop packaging

Offline install, native file dialogs, no server required.

**Done when:** a non-technical user installs from a single file on Windows and macOS, opens a project by double-clicking it, and works with no network connection.

> The Qt desktop now exists from M0.1 (D15), with an automated controls smoke test.
> M7 still owns installers, clean-machine deployment, platform integration and file
> associations on Windows/macOS. A developer executable does not close M7.

---

## M8–M12 — Multi-level modelling: Micro, Meso, Macro (D145)

**Booked by the owner on 2026-10-09; starts after the D143 queue** (M5.9 → M4.2 → M6.0 → M5.8).
Design, decisions and slices: [`plans/MULTI_LEVEL_MODELLING.md`](plans/MULTI_LEVEL_MODELLING.md).
One authoring network and one demand compile to three views; each engine imports nothing; every
result of every level carries the not-yet-validated marker until that level's M6 benchmark
passes. None is started; each slice begins with its contract and acceptance rows.

### M8 — Zones, OD matrices and parking lots

**Open, not started.** Zone, ZoneConnector and OdMatrix shared by every level; OD expanded into
Micro inputs and routes; ParkingLot as a zone with capacity, gate service and dwell time.
**Done when:** a development-access study runs from a zone OD matrix through a capacity-limited
lot; a full lot produces a gate queue and never deletes a vehicle; older files load
byte-identical; editor tools and the OD table ship in the milestone.

### M9 — Dynamic assignment (Micro)

**Open, not started.** Iterative between-run path-cost loop over the existing routing decisions;
costs in a separate file with provenance; convergence reported. En-route rerouting is excluded.
**Done when:** an analytic two-route fixture converges to its equilibrium split within the stated
tolerance; same inputs give the same bytes; non-convergence is flagged.

### M10 — Macro four-step model

**Open, not started.** Macro network compile with link types and volume-delay functions as data;
static user-equilibrium assignment; trip generation, gravity distribution with Furness
balancing, multinomial-logit mode choice; skims, select-link, Results and Export; centreline-only
Links in the editor. **Done when:** assignment reproduces a published test network's equilibrium
within a stated relative gap; each step passes a hand-computed fixture; parameters swap with no
code edit.

### M11 — Meso engine

**Open, not started.** Event-based link queue model (formulation is an owner decision), signals
and priority as capacity, spill-back, M9's assignment loop reused. **Done when:** analytic queue
and shockwave fixtures pass; spill-back blocks the upstream link at storage capacity; same seed
gives the same bytes.

### M12 — Multi-resolution subarea workflow

**Open, not started.** Subarea OD cut from an M10 assignment with gate zones and provenance,
run in M11 and Micro, with a cross-level consistency report. **Done when:** one cut runs at both
finer levels with recorded provenance and a reported volume comparison.

---

## Later, with their own milestones — not to be started inline

Recorded so nobody starts them opportunistically:

- Pedestrian and public-transport modelling, transit stops and crosswalks (from M5.1, D143;
  parking and regional/macroscopic modelling moved to M8–M12 by D145)
- Activity-based demand and transit assignment beyond M10's four-step model
- Scenario management beyond M5.8's comparison (a scenario manager, variant trees)
- 3D presentation
- Collaboration

---

## Reviews

Dated reviews record evidence and proposals, never approvals; the milestones above stay the
authority. Current: [2026-10-09](audits/ROADMAP_REVIEW-2026-10-09.md) (owner questions R1–R8).
Earlier: [2026-10-03](archive/ROADMAP-review-2026-10-03.md) (O1–O10, sessions S0–S5).
