# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is the history and the reasoning** — what a session
reads to understand why the code is the way it is. What to do next is in
[`NEXT.md`](NEXT.md); the decision log is at the bottom of this file. Never delete an entry;
move old blocks whole into `docs/archive/` if this gets long. Older entries are preserved there:

- [`archive/PROGRESS-2026-09-25-m1-timed-drawing.md`](archive/PROGRESS-2026-09-25-m1-timed-drawing.md) — 2026-09-25, the M1 timed drawing (9 min 40 s); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m2.7b-controllers.md`](archive/PROGRESS-2026-09-25-m2.7b-controllers.md) — 2026-09-25, M2.7b, fixed-time Signal Controllers (D48); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m2.7a-heads.md`](archive/PROGRESS-2026-09-25-m2.7a-heads.md) — 2026-09-25, M2.7a, a Signal head placed where clicked (D47); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-24-m2.1-demand.md`](archive/PROGRESS-2026-09-24-m2.1-demand.md) — 2026-09-24, M2.1.1 and M2.1.2 (routeless inputs, counted turning proportions); moved out 2026-09-25 as the oldest live entries
- [`archive/PROGRESS-2026-09-24-m2-slices.md`](archive/PROGRESS-2026-09-24-m2-slices.md) — 2026-09-24, M1.26.1 through the M3 contract, and the superseded `## Next` blocks; moved out 2026-09-25 as the oldest live entries
- [`archive/PROGRESS-2026-09-23-editor-benchmark.md`](archive/PROGRESS-2026-09-23-editor-benchmark.md) — 2026-09-23, the editor benchmark was measuring itself (D31); moved out 2026-09-24 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.27.3-reflexes.md`](archive/PROGRESS-2026-09-22-m1.27.3-reflexes.md) — 2026-09-22, M1.27.3, the reflexes counted; moved out 2026-09-24 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.27.2-slots.md`](archive/PROGRESS-2026-09-22-m1.27.2-slots.md) — 2026-09-22, M1.27.2, a vehicle carries scenario slots; moved out 2026-09-24 as the oldest live entry
- [`archive/PROGRESS-2026-09-23-compact-workspace.md`](archive/PROGRESS-2026-09-23-compact-workspace.md) — 2026-09-23, the compact desktop workspace; moved out 2026-09-24 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.27.1-redraw.md`](archive/PROGRESS-2026-09-22-m1.27.1-redraw.md) — 2026-09-22, M1.27.1, the connector cache; its numbers corrected 2026-09-23; moved out 2026-09-23 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.27-build-stage.md`](archive/PROGRESS-2026-09-22-m1.27-build-stage.md) — 2026-09-22, M1.27 build stage, the PCH and json_fwd work; moved out 2026-09-23 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.26-carriageway-routes.md`](archive/PROGRESS-2026-09-22-m1.26-carriageway-routes.md) — 2026-09-22, M1.26, a route belongs to the carriageway; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.24-one-window.md`](archive/PROGRESS-2026-09-22-m1.24-one-window.md) — 2026-09-22, M1.24, the one-window editor; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.25-pointer-demand.md`](archive/PROGRESS-2026-09-22-m1.25-pointer-demand.md) — 2026-09-22, M1.25, demand drawn by pointer; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.22.2-rotation.md`](archive/PROGRESS-2026-09-22-m1.22.2-rotation.md) — 2026-09-22, M1.22.2, selection rotation; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-21-m1.22.1-keyboard.md`](archive/PROGRESS-2026-09-21-m1.22.1-keyboard.md) — 2026-09-21, M1.22.1, editor history and the keyboard workflow; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-21-toolchain-and-3.3.md`](archive/PROGRESS-2026-09-21-toolchain-and-3.3.md) — 2026-09-21, the toolchain install and the §3.3 measurement; moved out 2026-09-22
- [`archive/PROGRESS-2026-09-21-connector-parity-audit.md`](archive/PROGRESS-2026-09-21-connector-parity-audit.md) — 2026-09-21, the Connector parity audit; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-21-m1.21.1-lifecycle.md`](archive/PROGRESS-2026-09-21-m1.21.1-lifecycle.md) — 2026-09-21, M1.21.1, the Network lifecycle correctness audit; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-20-m1.21-authoring.md`](archive/PROGRESS-2026-09-20-m1.21-authoring.md) — 2026-09-20, M1.21, the supplied-spec audit and authoring foundation; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-18-connector-position.md`](archive/PROGRESS-2026-09-18-connector-position.md) — 2026-09-18, M1.20; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-18-m1.19-lane-middles.md`](archive/PROGRESS-2026-09-18-m1.19-lane-middles.md) — 2026-09-18, M1.19, the Connector lane middles land on the Link lane middles; moved out 2026-09-21 as the oldest live entry
- [`archive/PROGRESS-2026-09-18-square-mouth.md`](archive/PROGRESS-2026-09-18-square-mouth.md) — 2026-09-18, the square-mouth revert (M1.17 reverted); moved out 2026-09-21 when it was the oldest live entry and the parity audit superseded it
- [`archive/PROGRESS-2026-09-18-earlier.md`](archive/PROGRESS-2026-09-18-earlier.md) — 2026-09-18, the snapping audit, the engine profile and the M1.17 revert
- [`archive/PROGRESS-2026-09-18-flush-mouth.md`](archive/PROGRESS-2026-09-18-flush-mouth.md) — 2026-09-18, M1.18, the flush mouth
- [`archive/PROGRESS-2026-09-17-mouth.md`](archive/PROGRESS-2026-09-17-mouth.md) — 2026-09-17, the wedge mouth and the miter "bulge"
- [`archive/PROGRESS-2026-09-17.md`](archive/PROGRESS-2026-09-17.md) — 2026-09-17, later entries
- [`archive/PROGRESS-2026-09-17-early.md`](archive/PROGRESS-2026-09-17-early.md) — 2026-09-17, earlier entries
- [`archive/PROGRESS-2026-09-16.md`](archive/PROGRESS-2026-09-16.md) — 2026-09-16
- [`archive/PROGRESS-2026-09-14.md`](archive/PROGRESS-2026-09-14.md) — 2026-09-14
- [`archive/PROGRESS-2026-09-10--2026-09-15.md`](archive/PROGRESS-2026-09-10--2026-09-15.md) — 2026-09-10 to 2026-09-15

---

## 2026-09-25 — M3.2.5a: Stop and Yield at the waiting line (D62)

M3.2.5 was split: **a** is the runtime and the model; **b** is the editor (ROADMAP row).

**Core** (`src/core/conflicts.*`, `simulation.cpp`):
- `ConflictZone::control` is `yield` or `stop`. Yield is the existing admission test, unchanged.
- A Stop holds each minor vehicle at its line until it has served it.
- `refreshStops` keeps `SimState::stopService`, at most one entry per vehicle for its next Stop
  line. The entry is carried while it is the same line and made when the vehicle has come to the
  line.

"Come to the line" had to be defined, because the reduced car-following model never reaches zero
behind a line. Measured: 0.04 m/s after 29 s, still creeping. So:
- **the line:** below `kStoppedSpeed` (0.1 m/s), with the front within `stopLineReach`, the gap the
  model keeps behind a standing obstacle at that pace plus 0.1 m;
- **the stop:** the Stop itself brings the vehicle to rest for that tick and one whole tick more.
  This is at most 1 m/s² of ordinary braking, published without a safety-clamp event.

A red head at the same line keeps its own hold, and green removes only that. Nothing composes by
special case: both are holds and the minimum applies.

**Cost:** `stepSimulation` measures 51.39M instructions against 51.24M at the base commit
(callgrind, `trafficsim-engine-benchmark 12 60`, Release). The first version cost +2.2%:
- **+1.02M:** GCC stopped inlining `vector<Vehicle>::push_back` in the grown function. Publishing
  with `push_back(std::move(...))` got it back.
- **+0.5M:** per-vehicle branches. They were moved out of the hot loop, behind `stopZones`.

**Model:**
- `StopControl{id, name, waitingLineId, mode, conflictAreaIds}` (contract §1), in schema 15. It is
  written only when present, so the two fixtures changed only `schemaVersion`, regenerated by
  their tools.
- **Structural issues:**
  - `DUPLICATE_STOP_CONTROL`: a second control on a line, or an area named twice;
  - `INVALID_STOP_CONTROL`: no area;
  - `UNKNOWN_*`.
- **Runtime:** `CONFLICT_STOP_LINE_MISMATCH` when an area does not give way at the control's line.
- The resolver marks the Stop areas' zones.

**Commands:**
- `putStopControl`, `deleteStopControl`.
- `setAreaControl(area, stop | yield | none)`: the mode belongs to the line, so it applies to every
  area the line controls.
- Changing an area's priority clears its control, because it now waits at another line.

**Cascades:**
- Deleting areas or roads prunes controls; `deleteWaitingLine` refuses a controlled line.
- Duplicating copies a control with its line and copied areas.
- A Problems row naming a control opens its area.

**A test slip worth remembering:** a vehicle's `routeIndex` indexes the *canonical* scenario the
state holds (sorted by id), not the scenario passed in. "route-10" sorts before "route-9", and the
first version of the seam test followed major vehicles.

Test runs: Linux headless 28/28 (289 unit tests), desktop 44/44 offscreen. Seed 42 is unchanged.

## 2026-09-25 — M3.2.4b: conflict areas on the canvas (D61)

The interaction was written first (`VISSIM_PARITY.md` §2b). Vissim picks conflict areas only
while *Conflict Areas* is the active object type; here the **Conflict area tool** (`A`) is that
type. Only this tool hit-tests areas, so a click at a junction under Select still picks the Link.
With the tool:
- a click selects the area's row;
- a click on the highlighted area, or `P`, cycles its priority: firstYields → secondYields →
  undetermined;
- a drag on a waiting line slides it along its own path;
- `Tab` picks the next area under the last click.

New code:
- Commands (`src/commands/conflict_authoring.cpp`): `cycleConflictPriority`, which keeps the
  name and the rule's numbers, and `moveWaitingLine`.
- Model: `controlPathPolyline`, the polyline a station is measured on. A drag projects onto
  exactly what `waitingLineBar` and the resolver read.
- Canvas (`src/editor/canvas_conflicts.cpp`): `conflictsAt`, `waitingLineAt`, the press and the
  drag, following the head drag. A line wins over an area.
- Readability: the side that gives way (the second while undetermined) is hatched and drawn
  above the solid side, so both show where a crossing's two sides cover the same square.

Evidence: `rightofway_editor.*` (2 new tests) and a new `priority-canvas` suite. It covers:
- Select still picks the Link, with the forcing that the point is inside an area;
- pick, click-cycle and `P`, each one Undo step, and a click on empty space;
- hatch and z order;
- a 5 m line drag along its lane with sideways drift, one step, and a jitter that moves nothing;
- Save with no dialog, then reopen: the same `rightOfWay`, rows, drawn items and "Runs".

Four mutations each fail their own assertion:
- Select hitting areas;
- no hatch;
- an area winning over the line;
- no commit on release.

`Tab` among overlapping areas has no test. Crossing areas never overlap, so it needs a taken-over
merge.

**Seen, not changed:** on a two-lane crossing, the minor side's waiting line for the far lane's
area stands inside the near lane's area. `kCrossingSetback` is measured from each area's own
entry. The resolver chains the two (A15) and the areas run, as the reopen check shows. Whether
the gesture should set the line before the first area is a question for M3.2.5's Stop/Yield
work.

Test runs: Linux headless 28/28, desktop 44/44 offscreen. Windows is CI's. Seed 42 is unchanged
(`1243e5361a7174d1ecc56b67d7c92b12`).

## 2026-09-25 — M3.2.4a: the Conflict areas tab (D60)

New commands in `src/commands/conflict_authoring.cpp`:
- `addCrossingAreas`: one area per crossing lane pair, extents from `surfaceOverlap`, lines 1 m
  short of entry, rule from the catalog.
- `setConflictControl`, `takeOverMergesOf`, `restoreAutomaticPriorityOf`, `removeConflictArea`.

Model helpers:
- `conflictSideOutline` and `waitingLineBar` draw from the resolver's own strips.
- `mergeSectionsOf` and `mergeSectionOfArea` name a merge by its section.

Shell (`src/shell/editor_priority.cpp`), the tenth objects tab:
- toolbar actions; Enter or double-click opens the dialog;
- a "Run status" column from the resolver;
- row selection highlights the area on the canvas (`src/editor/canvas_conflicts.cpp`);
- Problems rows for right-of-way objects open the tab;
- an `editorRunProtection` note shows with every run.

The validation banner no longer says "no conflict resolution", which stopped being true at
M3.2.3a. It now says conflicts are resolved only where authored, and the not-yet-validated marker
stays. All 49 right-of-way codes have en and th text; a test scrapes them from the sources.

Test runs: Linux headless 28/28, desktop 43/43 offscreen (new `priority-ui` suite). Seed 42 is
unchanged. A screenshot checked the drawing.

## 2026-09-25 — M3.2.3c: shared receiving space, merges on the solver, hold cycles (D59)

Phase 2 of `stepSimulation` is now `resolveRequests` (`src/core/conflicts.*`). Besides the swept
check, it serves requests crossing their lines behind one standing leader in vehicle-id order, and
caps any the room cannot hold. The leader's id comes from `closestVehicle` through an optional
out-parameter, so `Leader` itself is unchanged. An earlier version that added the id to `Leader`
cost +1.5% of `stepSimulation`; this one measures 51.20M against 51.29M before.

Core validation:
- The route check `CONFLICT_MIXED_ROLES` is replaced by `CONFLICT_HOLD_CYCLE`, a cycle in the
  waits-for graph between zones.
- A zone ending on two feeding segments arbitrates a merge, as a rule does.

The resolver compiles each complete authored merge group to zones instead of `right-of-way/` rules.

Tests changed deliberately, from authored rules to zones: `rightofway.a06`, `a07`, the lane-two
curved take-over, the short-section take-over, three `rightofway_resolution` tests and the
lifecycle `clean` helper. The D58 replay-equality test no longer holds, by design, and is replaced:
a take-over now carries the fallback's line and numbers but also holds the major side.

New tests are in `conflict_chain.*` (3) and `rightofway_runtime.*` (a three-way merge). Disabling
the room reservation or the cycle check each fails its own test. Test runs: Linux headless 28/28
and desktop 42/42 offscreen. Seed 42 is unchanged.

## 2026-09-25 — M3.2.3b: spans, atomic chains, authored merges run (D58)

`ZoneSide` is now a chain of segment ids. `zoneIncidence` (`src/core/conflicts.*`) resolves each
route's zones once, in route distances (`entryAt`, `exitAt`, `waitAt`, `clearAt`); the run index
and validation share it. The admission functions now read those instead of `partStart`.

The resolver:
- builds each side's chain (`sideChain`);
- drops `UNSUPPORTED_CONFLICT_SPAN` and `UNSUPPORTED_CONFLICT_GROUP`;
- drops `UNSUPPORTED_CONFLICT_RUNTIME` from merge areas, so no code emits it any more.

New core refusals are `CONFLICT_ROUTE_JOINS_INSIDE` and `CONFLICT_MIXED_ROLES`.

Tests changed deliberately, because they pinned the blockers this slice lifts: `rightofway.a04`,
`rightofway.a06`, the `clean`/`noIssues` helpers, and the M3.2.3a refusal test. The last is now
three run tests.

New tests: `tests/conflict_chain_tests.cpp` (4) and `rightofway_runtime.*` (5 in all). Disabling
chaining fails the A15 test; disabling chain-following fails the span test. A taken-over body
merge replays the untouched fallback's seed-42 event stream exactly, and reversing it changes the
stream, so the rule binds. No-zone cost is unchanged: `stepSimulation` measures 51.29M
instructions, as at M3.2.3a. Test runs: Linux headless 28/28 and desktop 42/42 offscreen. Seed 42
is unchanged.

## 2026-09-25 — M3.2.3a: one authored crossing runs (D57)

The core gained a `ConflictZone` (`src/core/types.hpp`) and an admission module
(`src/core/conflicts.*`). An authored crossing area with nothing reported against it now
compiles to a zone (`resolveRightOfWay(...).zones`) and runs. It no longer carries
`UNSUPPORTED_CONFLICT_RUNTIME`.

`stepSimulation` now runs in three phases:
1. Compute every candidate move from the snapshot.
2. Cap any minor request whose line crossing coincides with a major front reaching the area in the
   same tick.
3. Publish in vehicle order.

Without zones the arithmetic and event order are the old ones: seed 42 is byte-identical and the
four TS baselines pass.

Named blockers for what this slice does not run:
- `UNSUPPORTED_CONFLICT_SPAN`: an area over a section cut.
- `UNSUPPORTED_CONFLICT_GROUP`: a segment in two areas.
- Merge areas keep `UNSUPPORTED_CONFLICT_RUNTIME`.

Core validation adds `CONFLICT_SINK_TOO_CLOSE` and `CONFLICT_ROUTE_STARTS_PAST_LINE`.

Tests: `tests/conflict_zone_tests.cpp` (11) and `tests/right_of_way_runtime_tests.cpp` (3). With
admission disabled (holds and swept caps stubbed out) six core tests and the gap-time response
test fail. With only the swept check disabled, its own test fails. On the model crossing, the
minor road's mean travel time rises with the rule's gap time, 1 s against 6 s. Test runs: Linux
headless 28/28 and desktop 42/42 offscreen. Seed 42 is unchanged.

Cost with no zones: callgrind on `trafficsim-engine-benchmark 12 60` (Release) puts
`stepSimulation` at 51.29M instructions against 49.29M before, +4.1%. That is the three-phase
split. Wall clock is 0.36–0.37 µs per vehicle-tick against 0.35–0.37, inside the clock's spread.
Two measured attempts to win it back:
- Skipping the zone call for routes with no zone recovered 2.7 points and was kept.
- Publishing inline when there is no zone, through a lambda, cost more (52.05M) and was reverted.

## 2026-09-25 — M3.2.2c: waiting lines upstream, crossing coverage measured (D56)

The resolver now places a waiting line wherever it stands upstream of its side. It walks the
runtime graph backwards from the side's entry segment, along single predecessors only, and
compiles the line as metres along the yielding segment: negative when the line stands before that
segment. A line some route can go round is `CONFLICT_WAITING_LINE_BYPASSED`; a line nowhere
upstream of the side is `CONFLICT_WAITING_LINE_NOT_UPSTREAM`. Both replace
`CONFLICT_WAITING_LINE_UNSUPPORTED`. The core accepts a negative `yieldPosition` down to the length
of that same chain of single predecessors (`src/core/validate.cpp`), so the check lives in both
places. The simulation already measured the stop line in route distance, so no engine code
changed; a new core test shows a vehicle held at a line on the approach.

A `crossing` area is checked against `surfaceOverlap` (`src/model/network/conflict_coverage.cpp`),
which intersects the two lane surfaces quad by quad and reads the overlap back as authored stations.
The codes are:
- `CONFLICT_NO_OVERLAP`: nothing to protect, and a shared edge is not a crossing.
- `CONFLICT_EXTENT_UNCOVERED`: an extent, named by side, falls short of the overlap.
- `CONFLICT_GEOMETRY_UNSUPPORTED`: two separate overlaps, or a strip folded on a tight bend.

Found and fixed on the way: a waiting line earlier on the side's own lane, in an earlier section,
was compiled at the end of the yielding section without a word (0.6 m instead of −30 m in the test).

New tests are in `tests/right_of_way_resolution_tests.cpp` (`rightofway_resolution.*`, 5) and
`core.a_stop_line_may_stand_on_the_one_approach_before_the_yielding_segment`. Five of the six fail
on the pre-M3.2.2c resolver and core validation. The sixth checks the new measurement against an
independent ray-casting reading of the geometry, on a curved two-lane Link with both driving sides.
Test runs: Linux headless 28/28 and desktop 42/42 offscreen. Seed 42 is byte-identical.

## 2026-09-25 — M3.2.2b: controls follow their owners (D55)

`src/commands/detail.hpp` gains `removeControlsOn`, `controlsNameLink`, `checkSplitControls`/
`splitControls` and `copyControls`; `deleteLink`, `deleteConnector` (so every reanchor deletion
too), `splitLink`, `duplicateObjects` and `reverseLink` call them. The tests are in
`tests/right_of_way_lifecycle_tests.cpp` on a curved two-lane Link with a body merge, both driving
sides; shared fixtures moved to `tests/right_of_way_fixture.hpp`. The interim pinning test
`rightofway.until_m3_2_2b_*` is removed on purpose — M3.2.2b is the change it guarded. Six of the
eight new tests fail on the pre-M3.2.2b commands; the other two lock behaviour that was already
right (lane edits keep ids; a shorter Link reports, never clamps). Linux headless 28/28, desktop
42/42 offscreen; seed 42 and both project fixtures unchanged.

## 2026-09-25 — M3.2.2a scrutinised: four defects fixed before M3.2.2b (D55)

An outsider review of M3.2.2a found, and a test now forces each (all four fail on `44380b1`):
1. A take-over placed its waiting line on the Connector's stored polyline, so lane 2 of a curved
   range compiled 47.255 m instead of the fallback's 47.239 m — D54's "exactly the fallback"
   held only for a path that IS that polyline. Stations are now chosen on the runtime segment
   and converted to authored coordinates (`sideOf`).
2. `resolveRightOfWay` could throw inside `buildScenario` (unchecked, must not throw):
   `sectionForStation` threw `UNKNOWN_LANE`. `locate` now reports unresolved instead.
3. A waiting line off its path's end was silently replaced by the segment end; it is now
   `CONFLICT_UNRESOLVED_PATH` on the line, and its station is kept (no clamping).
4. A side on an upstream section shorter than 1 m resolved into the previous section; the entry
   is now at most half the segment back.
Recorded, not fixed: id uniqueness is checked against network ids only, not route/input ids
(`allocateId` already avoids clashes for commands).

## 2026-09-25 — M3.2.2a: authored right-of-way controls at the file/model seam (D54)

**What exists now.** `src/model/network/control.hpp` — `ControlPathRef` (a Link lane, or a
Connector lane named by its source/target lane ids, never an ordinal), `WaitingLine`,
`ConflictArea` (`crossing`/`merge`, `firstYields`/`secondYields`/`undetermined`) and
`AuthoredPriorityRule`, held in `Network::rightOfWay`. `right_of_way.cpp` — structural
checks (in `validateNetwork`, so a bad edit or file is refused whole), path resolution, merge
groups and `resolveRightOfWay`, which `buildScenario`, `compileScenario`, `priorityDefaultsIssues`
and `runtimeDiagnostics` all call. Schema 14 writes `network.rightOfWay` only when non-empty;
older files load with it empty, and the key in a schema-13 file fails. Commands:
`put`/`delete` for each object, `takeOverMerge` (materialise a merge's fallback as authored
areas, one transaction) and `restoreAutomaticPriority` (the separate named hand-back).

**Evidence.** `rightofway.*` covers A01, A03, A04, A06, A07, A08 and A02 in part (see
`M3_ACCEPTANCE.md`). The four baselines and `trafficsim-cli 42` are unchanged; the two project
fixtures differ only in `schemaVersion`. Linux headless 28/28, desktop 42/42 (offscreen);
Windows is `native.yml`'s.

**Not here:** reference lifecycle (M3.2.2b), any runtime for an authored area (M3.2.3), the
editor surface (M3.2.4), Stop/Yield (M3.2.5), counters (M3.2.6). Structural checks do not yet
prove that a crossing's extents cover the real overlap — every crossing is Run-blocked anyway.

## 2026-09-25 — Docs pass: stale instructions out, session-start reading cut

Owner-approved optimization pass, docs dimension, after a scrutiny of the first plan (which
dropped a "broken links" item — every hit was a false positive — and required the D28/D31
guardrails to move into Working rules before CLAUDE.md's history paragraphs were cut).
Estimated tokens (`docs_audit.py`): CLAUDE.md 3,837 → 2,515; NEXT.md 2,890 → 1,116; PROGRESS.md
18,344 → 16,242 — the decision log is two thirds of it and stays. The oldest 2026-09-24 entries and
two superseded `## Next` blocks moved whole to `archive/PROGRESS-2026-09-24-m2-slices.md`. No
code changed; engine speed was measured (0.78 s Release for the M2.6 template's hour) and left.

## 2026-09-25 — M2 gate passed by the owner (D53)

The owner reported M2.6 passed. Recorded in `M2_GATE.md` with the verdict as the owner's
(D51) and every detail the owner did not supply marked "not stated". ROADMAP §M2, NEXT and
CLAUDE.md now point at M3.2.2. No code changed.

## 2026-09-25 — C2 and C4 withdrawn; the owner judges the gate (D51, D52)

Then C4 too (D52): no delay comparison with Vissim in the gate. M6 validation is unaffected.

Owner ruling: cancel the timing comparison with Vissim in every test; the owner decides pass or
fail. ROADMAP §M2, `M2_GATE.md`, `M2_PLAN.md`, `M3_PLAN.md`, NEXT and CLAUDE.md now say so. The
only timing-against-another-tool criterion anywhere was M2's C2; M3's acceptance has none. C1 (the
study) and C4 (delay beside the other tool's, recorded) stand — the owner asked only about timing.
No code changed.

## 2026-09-25 — M1 accepted by owner ruling (D49); the M2.6 template; a merge deadlock fixed (D50)

**M1.** The owner ruled M1 usability accepted on the evidence of the one attempt: an engineer who
has used another simulator models an ordinary intersection here in well under 10 minutes (D49).
ROADMAP and `M1_ACCEPTANCE.md` say "accepted by owner ruling" and keep the unshown items listed.
M0 plausibility stays open. D11's naming trigger ("end of M1") is now live.

**The M2.6 template** (owner request). `tools/m26_study_network.hpp` builds
`data/projects/m2.6-study-template.traffic.json` from the four-leg fixture: right-turn pockets, a
new `FourLegOptions::leftBypass` (6 m) so the left turn leaves the kerb lane before its head, the
owner's timing windows (cycle 120 s), one routeless `urban-mixed` input per approach over four
15-minute intervals, and a placed decision per entry Link with per-interval turning counts. **The
volumes are placeholders** (the fixture's), and there is no aerial image. It cannot be C1/C2
evidence — C1 is the owner building from blank. `m26study` pins file = builder. The default
`leftBypass = 0` leaves the four-leg fixture byte-identical.

**The deadlock (D50).** With the free left turn, the East and South kerb lanes locked within a
minute: a left turn held exactly at the exit's start had its front on the exit lane, the through
vehicle behind it stopped 2 m from the join, inside the 7 m headway, and each waited on the other.
M2.0.1's note that the rule "rarely binds" under split phasing is true only while turns wait for
their own green. Fix: the derived stop line is 1 m short of the join. `m26study` forces the old
placement and asserts the deadlock returns, so the test can fail.

**Found, not fixed:** the left turn still waits behind the through queue in the shared kerb lane —
without lane changing (M3.2.8) the bypass only helps when the queue is shorter than 6 m. The
template's left-turn delays (34–60 s) show it.

## Backlog (M0, in order)

- [x] Toolchain + directory skeleton + core-import guard
- [x] `Scenario` type and a fixture: two crossing movements with explicit connectors
- [x] Fixed-timestep loop; one vehicle traverses links with continuous route distance
- [x] Reduced Wiedemann-inspired car-following; vehicles queue behind each other
- [x] Fixed-time signal; vehicles stop at red, discharge at green
- [x] Vehicle input generating arrivals from a seeded stream, retaining blocked arrivals
- [x] Native Qt harness: vehicles as dots on links (former canvas preserved in Git history)
- [x] Headless completed-trip delay diagnostic and seeded replay regression
- [ ] Owner's M0 plausibility acceptance (still open)

Later milestones are in [`ROADMAP.md`](ROADMAP.md). The owner explicitly authorized M1.1–M1.3 in D16; all other milestone gates remain in force.

---

## Open questions

Ask these before the milestone they block.

| # | Question | Blocks | Notes |
|---|---|---|---|
| ~~Q1~~ | ~~Thailand-first or international?~~ | — | **Answered 2026-09-10: international from the start.** See D7. |
| Q2 | Which lane-changing model? | M1 | MOBIL and Gipps are both defensible. Needs a short spike, not a debate. |
| ~~Q3~~ | ~~Who are the three engineers for the M2 gate?~~ | M2 gate | **Answered 2026-09-10: the project owner performs the gate alone.** This materially weakens it — see D8 and the mitigation in `ROADMAP.md` M2. |
| Q4 | Which published benchmarks define the M6 tolerance? | M6 | Decide before M5 so evaluation is built to be checkable against them. HCM is the likely baseline now that D7 makes the tool international. |
| Q5 | Final product name | Nothing | Deferred until the end of M1 by D11; **M1 is accepted (D49), so it is now the owner's call** (NEXT item 2). Candidates and collision findings are in the D11 row; reuse them. |
| ~~Q6~~ | ~~Register `velk` on npm and PyPI~~ | — | **Withdrawn 2026-09-11 as moot** — no settled name to register. The registration question returns with the name at M1. |

---

## Decisions

Non-obvious choices **and the reasoning**. Without the reasoning a later session will
"improve" a decision away and break something invisible.

| # | Date | Decision | Why | What would make it wrong |
|---|---|---|---|---|
| D1 | 2026-09-10 | **Own simulation engine, not a front end over an existing one** | Engineering use needs conflict areas and priority rules as authored inputs, signal heads anywhere on a link, and vehicle-owned driver behaviour, and the two things the job is actually paid for — per-movement evaluation and multi-run averaging — have to be built regardless. Owning the engine makes all of them first-class. See `PROBLEM.md` §2. | If the M2 gate (D38) shows an engineer cannot complete a real study with it. This is the single most expensive decision in the project and it has an explicit test. |
| D2 | 2026-09-10 | **Link-based network model natively; junctions are derived, not authored** | This is how the audience thinks and it is the whole point of D1. Translating to a node–edge model would reintroduce the impedance the prior effort spent six milestones papering over. | If deriving junction geometry from links proves intractable at M1. |
| D3 | 2026-09-10 | **TypeScript everywhere to start; `core/` written so it can be ported to a compiled language later without touching anything above it** | Microsimulation is CPU-bound and a compiled core is probably where this ends up. But picking a stack the user cannot run today, to solve a performance problem not yet measured, is the classic way to stall at milestone 0. Hard rule 1 (`core/` imports nothing) makes the port a contained job later, and makes it measurable first. | If M0 cannot reach real-time on a single intersection — then port immediately rather than optimising TypeScript. |
| D4 | 2026-09-10 | **Desktop is a constraint from day one, a milestone at the end** | Web-first keeps the development loop fast; a framework-free core plus isolated rendering means desktop packaging is packaging, not a rewrite. A boot smoke test in a desktop shell runs from M1 so it never becomes a surprise. | If a required capability (native file dialogs, offline licensing) turns out to need a different shell architecture. |
| D5 | 2026-09-10 | **A results screen carries a "not yet validated" marker until M6 passes** | Numbers from this tool go into documents submitted to regulators. An unvalidated engine that looks authoritative is worse than no tool. | Nothing. This one is not negotiable before M6. |
| D7 | 2026-09-10 | **International audience from the start, not Thailand-first** | Nothing in the engine is jurisdiction-specific, and the parts that are — LOS thresholds, report layouts, units — are data, not code, so building them swappable costs little now and a retrofit costs a lot. Three concrete consequences: HCM is the default LOS pack with others as swappable data; metric internally with display units switchable; **left-hand and right-hand traffic is a first-class network setting from M1** (Thailand, UK, Japan, Australia all drive left — a prior effort never implemented it at all). | If it turns out every real user is in one jurisdiction and the generality is unused weight. |
| D8 | 2026-09-10 | **The M2 gate is performed by the project owner alone, not three independent engineers** | The owner is a practising traffic engineer and no outside participants are available. Accepted with eyes open: this is **a materially weaker test than the one D1 needs**, because the person judging whether the tool is usable for a real study is the same person who chose to build it. Mitigation, mandatory: **the pass/fail criteria are written down and committed before M2 implementation starts**, so the judgement cannot be rationalised after the fact. Adding outside engineers later strengthens the gate and is never wasted. | Nothing makes it wrong; it is simply weak. Treat a pass as "not disproven", not as "confirmed". |
| D9 | 2026-09-10 | **The project is named Veytrix** | Chosen by the owner after working through several naming directions (domain jargon, borrowed engineering terms, abstract coinages, Thai-rooted feminine names). Verified free on npm and PyPI. **Two known flags, accepted:** `veytrix.com` is already resolving to something, and **Vectrix** is an existing electric-scooter company that is phonetically close. Neither blocks a repository or package name, but both are reasons a trademark search would be worth doing before any commercial use. | A trademark conflict surfacing later. Renaming is cheap while the repo is documentation only and gets steadily more expensive after that. **Superseded by D10 (Velk) on 2026-09-11.** |
| D10 | 2026-09-11 | **The project is named Velk**, superseding D9 | Coined, one syllable, no meaning in any major language — the owner's stated requirement. Verified free on npm and PyPI, and a brand/company search found nothing using it. `velk.dev` and `velk.app` are free; `velk.com` and `velk.io` are held, which is ordinary for a four-letter word and irrelevant to a repository or package name — accepted as a known risk. **`MicroFlow Simulator` was considered first and rejected on collision grounds** (`microflow` taken on npm and PyPI, ≥7 GitHub projects plus two orgs and a GitHub Topic, both obvious domains held) — do not re-propose it. | A trademark conflict, or the name proving so anonymous that people cannot find the project. Both are cheap to fix now and expensive once source code, packages and links exist. **Superseded by D11 on 2026-09-11.** |
| D11 | 2026-09-11 | **Keep the working name `TrafficSim`; defer naming until the end of M1** | The project was renamed three times in two days (TrafficSim → Veytrix → Velk) with several further candidate sets explored, and no code was written in that time. A name is far easier to judge against a working program than against a specification, and each further round costs a session without moving the project. Deferring also cancels work already queued: no GitHub repository rename, and no package or domain registrations to make and then undo. **Trigger to revisit: the end of M1**, when there is a working network editor to name. **Names already examined — start from these findings, do not re-derive them:** `Headway` rejected (`headwaymaps/headway`, an OSM maps stack, same field); `MicroFlow Simulator` rejected (`microflow` taken on npm and PyPI, ≥7 GitHub projects plus two orgs and a GitHub Topic, both obvious domains held); `Veytrix` set aside (`veytrix.com` held, `Vectrix` phonetically close); `Velk` set aside while clean on every channel checked (npm, PyPI, brand search; `velk.dev`/`velk.app` free) and therefore the strongest candidate to return to. | Drifting past M1 without ever deciding. The trigger exists to prevent exactly that. |
| D6 | 2026-09-10 | **Project spine written before any code** | Only what is on disk survives a session boundary. The rules in `PRINCIPLES.md` §3 were measured by a prior effort and would otherwise have to be rediscovered by paying for them again. | — |

| D12 | 2026-09-11 | **Implement the simulation core and network model together, including prerequisite tooling** | The owner explicitly requested both systems in this session. That supersedes the earlier toolchain-only Next and one-system scheduling guidance. The boundary still remains strict: model compiles a snapshot; core imports only its own modules. | If later features are pulled forward without a separate scope decision. M0 is still the boundary. |
| D13 | 2026-09-11 | **M0 uses a clearly labelled reduced Wiedemann-inspired longitudinal model; unsupported merges are rejected** | A small auditable prototype is enough to exercise the M0 architecture and queue/discharge behaviour. The published W74 safety-distance shape is an inspiration, not permission to claim a faithful W74/W99 implementation. Accepting merging paths without gap acceptance would silently invent unsafe right-of-way semantics. | If M0 plausibility fails, fix or replace the approximation before closing M0; do not remove the unvalidated marker. |
| D14 | 2026-09-11 | **Pin TypeScript 5.9.3 and the dependency lockfile** | The boundary guard uses the TypeScript compiler AST API, including type imports and dynamic imports. The initially resolved TypeScript 7 package lacks that API. Pinning the compatible compiler makes the guard executable, with deliberate negative tests. | When the guard is migrated to a supported replacement AST API and verified against the same forbidden-import fixtures. |

| D15 | 2026-09-11 | **C++20 throughout the application, Qt 6 Widgets desktop, CMake/CTest**; supersedes D3's initial stack, D4's web-first loop and D14's active TS tooling | The owner asked whether the whole program could move to C++, then authorized the proposed migration. Port the existing M0 core/model and harness together; preserve old source in Git and four frozen regression fixtures. Keep Qt/JSON outside the engine and existing modelling limitations explicit. This supersedes one-system scheduling guidance for the migration. | If behaviour diverges from the saved baseline or desktop controls cannot run, fix the port before M1. Cross-toolchain math uses an explicit tolerance; scientific fidelity still requires M6. |

| D16 | 2026-09-12 | **Implement M1.1–M1.3 together on the native C++ base** | The owner approved the editor plan and explicitly requested these three slices. This supersedes the earlier one-system scheduling and M0-only Next for this scoped work. Implement document/history, canvas/background and Link/Lane tools together with basic saving so drawings persist. Existing scientific and full M1 usability gates remain open. | If later connectors, demand or runtime behaviour are introduced without their own scope decision. |

| D17 | 2026-09-14 | **Continue M1 with the planned M1.4 Connector editor; preserve one version-1 geometry source** | The owner requested continued Network editor development. `Next` already called for M1.4, so this session implements that slice and retains M1.3.1's split guard. A lane-aligned cubic is sampled into editable polyline points, avoiding a second curve store and an unnecessary schema change. Referenced connectors may be reshaped but not retargeted; deleting one removes its affected routes and inputs atomically instead of inventing new paths. | If engineering workflows require persistent tangent handles or measured radius constraints, define their model/schema explicitly; do not claim the sampled curve provides those guarantees. |

| D18a | 2026-09-14 | **Diagnostics resolve object IDs in the model layer; `ValidationIssue` stays frozen** | Adding an `objectId` field to `ValidationIssue` in `src/core/types.hpp` would have compiled — no test compares a whole issue — but it is the wrong boundary. `core/` validates a `Scenario` whose only objects are segments, so the field would be empty for nearly every core code, and it would duplicate what the index path already locates (hard rule 3). Deriving the ID on read from the index path keeps `src/core/` untouched by M1.5 entirely, which is also what puts the four frozen TypeScript baselines and the trajectory digest structurally out of reach. | If an index path ever needs to survive an edit and be re-resolved later, a stored ID becomes the cheaper representation. It is not needed for a panel that is rebuilt per revision. |
| D18b | 2026-09-14 | **Draft validity blocks an edit; runtime supportability only informs, and only on demand** | These answer different questions and must not be merged. `validateNetwork` asks "is this a coherent drawing" and `History::execute` rightly refuses anything else. `validateScenario` asks "can the M0 core run this", and the authoring model deliberately expresses things the core cannot yet run — a merge is the standing example. Making the second blocking would forbid legal authoring; making the first advisory would let broken documents be saved. **A structural consequence the UI must own: because `validateDocument` throws on every non-`EMPTY_NETWORK` issue, a committed document can never carry a draft-invalid object, so the draft list is fed only by a blank document and by the issues of a *rejected* edit** — which is exactly where the object IDs earn their keep. | Nothing, unless the runtime gains merge arbitration, at which point `UNSUPPORTED_MERGE` stops being a finding. Do not "fix" the usually-empty draft list by loosening commit validation. |
| D18c | 2026-09-14 | **M1.5 tables cover network objects only; demand tables carve out to M1.5.1** | Routes and vehicle inputs have no model struct — they are untyped JSON under `ProjectDocument::definition` — so tabling them means designing an authoring demand model, which is M2's subject. Diagnostics still name routes and inputs by their own IDs, which is what the milestone actually required. Carved into a numbered milestone in the same session rather than left as a note, per hard rule 8. | If M2 demand authoring lands first, M1.5.1 is absorbed into it rather than done separately. |
| D18d | 2026-09-14 | **Vehicle-type and behaviour findings are withheld, not reported, when no catalog is loaded** | Those catalogs live in `data/`, not in the project file, so a document alone genuinely cannot resolve them. Reporting `UNKNOWN_VEHICLE_TYPE` for every input of an otherwise valid M0 fixture would blame the drawing for an absence that is by design, and would train users to ignore the panel. One `EDIT_NO_CATALOG` row says what was not checked instead. | When M1.7 resolves catalogs at run handoff, the check becomes real and the withholding should be removed rather than left as a permanent blind spot. |
| D19a | 2026-09-14 | **Keep M0 scenarios and editor projects as two formats; classify the file instead of merging them** | The reported 304 was a category error, not a corruption: a drawn network legitimately has no `definition`, and the simulation window had no way to tell a project from a scenario because both matched `*.json`. Merging the two schemas would have removed the failure by making every drawing claim it is runnable, which is precisely the fidelity claim hard rule 4 exists to prevent — a drawing has no demand, so it cannot run, and the format should keep saying so. Classifying the file before any field is read, and routing a project to the window that can open it, fixes the user's actual problem without that claim. | If M1.8 gives projects a real Run **and** demand authoring (M1.5.1) makes `definition` non-optional in practice, the distinction stops earning its keep and one format becomes honest. Converge then, not before. |
| D19b | 2026-09-14 | **Load errors carry a code on a typed exception, not a formatted message** | The shell already translates `EDIT_*` codes by locale key (`EditorWindow::showError`); the simulation window instead concatenated `e.what()`, which is how nlohmann's text reached a user running `--language th`. `ScenarioLoadError` carries file, code and detail separately so the shell can translate, show the path, and offer an action, while an unknown parser detail still falls back to raw text rather than a blank dialog. One error channel, one lookup, two windows. | If load errors ever need structured per-object issues the way `ValidationError` does, promote the code to an issue list rather than growing the string. |
| D19d | 2026-09-14 | **Error codes are resolved in exactly one place per window; no caller formats `what()` itself** | The first cut of D19b gave `ScenarioLoadError` a code but left three callers printing `what()`, which for a classification failure *is* the bare code — so `--scenario` on an editor project went from an unreadable nlohmann string to an unreadable identifier. Worse, not better: the user lost the one sentence the old message did contain. `MainWindow::explain` and `EditorWindow::openFileOrReport` are now the only places a code becomes text, and startup no longer dies on a file it could have explained. | If a third window appears, the two `text()` lookups become genuine duplication and should be hoisted to a shared locale helper rather than copied a third time. |
| D19c | 2026-09-14 | **The parity review books three milestones and deliberately leaves seven gaps unbooked** | `VISSIM_PARITY.md` §6 ranks ten gaps; only in-editor Run, the sidebar/gesture set and levels/display types are carved into `ROADMAP.md`. The rest — editable object tables, group drag, and the absent Vissim object types (priority rules, stop signs, reduced speed areas, conflict areas) — need engine behaviour that does not exist yet. Booking authoring for an object the core cannot honour would invite a user to believe it is modelled, and would put a date on work whose prerequisites are unscheduled. Recording them without a milestone keeps the roadmap true (`ROADMAP.md` rule 2) while keeping the finding. | When M3 lands right-of-way, conflict areas and priority rules stop being unhonourable and should be booked immediately — the review section is the list to work from. |
| D20 | 2026-09-17 | **Implement M3.1 — merge arbitration by gap time and headway — rather than carve M1.11.1's second half out** | A Connector arriving inside a lane body is structurally a merge: the section downstream of the arrival has two predecessors. D13 forbids accepting that without gap acceptance, so M1.11.1's done-condition ("leaves **and** enters") was unreachable. The owner, shown the choice, chose to build the necessary part of M3 instead of deferring. Scoped to the one M3 bullet it needs — a priority rule with real seconds and metres — reusing the red-signal clamp rather than a second braking path, and relaxing `UNSUPPORTED_MERGE` **by construction** (n−1 of n predecessors must yield to another) rather than by removal. Filed at its number; **M3 is not closed** and its done-condition is not met. | Nothing about the mechanism. But if a later session reads M3.1 as "M3 is underway", the gate discipline is lost: M3 still owns conflict areas, authorable priority rules, stop/yield control, crossing conflicts and heads anywhere on a link. |
| D21 | 2026-09-17 | **Lane sections are derived every compile, never persisted** | `ROADMAP.md` forbids persisted duplicate runtime networks, and sectioning changes no authored id, so it is a pure function of the drawing — unlike `splitLink`, which must rewrite routes because the ids an author stored really do change. `sectionId(laneId, 0)` returns `laneId`, so an uncut lane compiles to exactly the `Segment` it always did, which is what keeps the four frozen baselines valid; breaking that one identity fails 54 tests. Route expansion lives **inside** `buildScenario` because `validateAuthoredDemand` compiles on every save, and anywhere else would break saving. | If sectioning ever becomes expensive enough to matter, cache it beside the document — but never store it in the project file, and never let an authored route name a section id. |
| D22 | 2026-09-17 | **A Connector's `laneMarkings` is indexed per interior divider, not per lane** | Vissim's `Lanes` tab field is per lane, but a lane has two edges and there are `paths + 1` boundary lines, so per-lane does not map onto them unambiguously — any choice is a choice. Per divider is complete and unambiguous, and the two outer edges stay solid because they are the edge of the carriageway, not a lane divider. **Not verified against Vissim**, and recorded as a chosen representation rather than a parity claim (rule 4). | A look at real Vissim showing the field means something else. The change is small — the vector's length and one index — so it was not worth blocking M1 to confirm. |
| D23 | 2026-09-17 | **The reported miter "bulge" is closed as a measurement error; `offsetGeometry` is unchanged** | Measured on 90.47° of deflection: 9.9403 m along the cross-section at the mitered vertex, 7.0425 m perpendicular point-to-polyline, and **exactly 7.000000 m projected across the leg**. The first is `width/cos(φ/2)`, which is what the intersection of two offset legs is — the diagonal of a correct mitered joint, not a bulge. The 8.698 m on record is the same identity at a gentler bend. Removing the miter would reinstate the pinch it exists to fix (30% at a right angle), and `network_tests` pins it to 1e-9. What was actually missing was an **upper** bound on width; it is now asserted exactly on every interior leg, and catches a 0.1% error. | Nothing, unless Vissim is shown to cut corners rather than miter them. The standing lesson: a distance between two boundaries is a width only when taken square to the road — `perpendicular()` says so in its comment, `apart()` does not, and the 24% figure was taken with `apart()`. |
| D24 | 2026-09-22 | **Retire the M0 harness window; the editor is the only window** | The owner asked whether it could go, and the measured answer is yes: `trafficsim-cli` already prints a superset of the figures it showed (`meanDelay`, `safetyClamps`, plus `--events`), and `parseDocument` needs only `network`, so the editor opens bare M0 scenarios already. Keeping a second window meant a second renderer (`src/render/`), a second run loop and a second place for the delay figure to drift from the CLI. What makes the removal safe is not the deletion but the replacement: `scenario-run-ui` pins the editor's run of `crossing.json` to the CLI baseline exactly (31 trips, mean delay 29.249359418430977), so the M0 plausibility observation changed surface without changing meaning. The gate itself was not touched — only the sentence naming where the observation is made. | If a results screen ever needs to run a scenario without the authoring surface (a batch review window, M5), build it on `runSimulation` and the event stream, not by restoring `MainWindow`. If the editor ever stops opening bare M0 scenarios, this decision is void and the CLI becomes the only M0 surface. |
| D25 | 2026-09-22 | **An authored route names Links and Connectors; the per-lane routes are compiled** | The owner's ruling: *Routing เป็นการสร้างบนทุกช่องจราจร … ไม่ได้สร้างเป็นรายช่อง*. A lane-level route tied demand to a Connector's lane count, so narrowing one either invalidated the route or had to be refused — and it was refused, which left an author unable to correct their own drawing. Naming the objects makes the lane chains derived data, which is where they belong: `routeLaneChains` recomputes them on every compile, the reference guards disappear because nothing a route names can vanish, and a vehicle input becomes what Vissim's is, a Link total split across the lanes that carry it. The single-lane expansion deliberately keeps the authored id, which is what lets the four frozen baselines replay unchanged. | What it costs is the lane-specific route: a turn pocket where only the left lane may turn cannot be expressed. If that has to come back before M2.1's positioned routing decision, the honest fix is an explicit lane restriction on the route, not a return to lane ids — the ids were the thing that made every Connector edit fragile. |
| D26 | 2026-09-22 | **A Connector gesture connects every lane of both Links by default** | This reverses the M1.12 default of one lane per drag, which existed for a good reason at the time: pre-filling the maximum authored a wide Connector from a single-lane gesture, and the author could not narrow it afterwards once a route used it. D25 removes that trap, and the owner asked for the wide default explicitly — *ให้เชื่อมจำนวนช่องตามจำนวนช่องใน Link ทั้งหมดก่อน ค่อยปรับลงภายหลัง*. | If authors start drawing single-lane turns more often than full carriageways, make the default follow the drag width instead of flipping it back: the dialog already has both counts, and the gesture knows which lane it started on. |
| D27 | 2026-09-22 | **A precompiled header holds third-party headers only** | `trafficsim_shell` compiles 26 Qt translation units and re-read the same 2.46 s of `<QGraphicsView>` in every one of them; precompiling it and sharing that PCH with the eleven single-source UI test executables through `REUSE_FROM` took a clean build from 86 s to 64 s. What is deliberately *not* in it is any project header: a PCH over a header that changes turns every edit to it into a full-target rebuild, which is the cost this exists to remove — measured, touching `canvas.hpp` still rebuilds 38 objects and no `.gch`. Adding `<nlohmann/json.hpp>` was tried and reverted: it grew the `.gch` by 2 s on `src/shell`'s critical path and cancelled the saving (258 edge-seconds against 243 without). | If the shell is ever split into two targets, give each its own PCH rather than one shared across different flag sets — GCC rejects a `.gch` whose macro state differs, silently, and the only symptom is the speed-up quietly disappearing. |
| D28 | 2026-09-22 | **Derived geometry is cached against the values it is derived from, never against a revision** | The canvas recomputed every Connector's ribbon on every frame, which callgrind put at 67% of a run. What makes the cache safe is not the speed-up but the key: `connectorPaths` and `connectorBoundaries` read the Connector, the two Links it names and the driving side and nothing else — checked in the source, not assumed — so comparing those four values is exactly as strong as recomputing. `ProjectDocument::revision` was rejected as a key for two independent reasons: `History::undo` restores an older revision, so the number is not monotonic, and `History::reset` continues from the file's own revision, so two documents can share one. A preview object differs by value and misses, which is the behaviour a drag needs anyway. | If a future `connectorBoundaries` starts reading a third object — a neighbouring Connector at a shared station, say, which §3.3 may force — this key silently goes stale. The defence is the test that widens a Link the Connector does not name: extend it the same way for whatever the new input is, and make it fail first. And do not generalise the pattern by reflex: the same cache for Link geometry was measured and reverted, because recomputing a polyline is cheaper than proving the cache is still valid. |
| D29 | 2026-09-22 | **A runtime vehicle names its scenario objects by SLOT; only the boundary uses names** | A `Scenario` is immutable and canonically sorted from `createSimulation` onwards, so an index identifies exactly what an id identified — and a tick copies, sorts and compares the whole vehicle list, which three `std::string`s per vehicle made the engine's largest cost (17.4% string copying, 11.4% the sort, 10.3% `resolveRefs`). Slots halved the run. What keeps it honest is that ids become slots only through `detail::byId`, whose first-occurrence rule is exactly what the lookups it replaced returned, and that **names survive at the boundary**: events carry `routeId`, `pendingJson` takes the `Scenario` and emits the same three strings, so the frozen fixtures are byte-identical and a human still reads names. The reverse direction — a slot escaping into a file or an event — is the thing to refuse: a slot means nothing outside the Scenario it indexes. | If a `Scenario` ever becomes mutable after `createSimulation`, or if anything re-sorts one mid-run, every slot in flight is wrong at once and silently. That is the invariant to defend, not the indices. `stepSimulation` already asserts `state.inputs` is parallel to `scenario.inputs`; add the same kind of assertion for anything else that starts indexing the scenario. |
| D30 | 2026-09-22 | **`Ctrl`+left-click stays a dead end; it does not duplicate** | The owner's ruling, asked in user-facing words during M1.27.3 and answered *ไม่มีอะไรเกิดขึ้น*. `VISSIM_PARITY.md` had ranked this High since 2026-09-14 as a *collision* — the chord that duplicates in Vissim extending the selection here — but `trafficsim-gesture-walkthrough` measured it doing **nothing at all** on an already-selected object, which is the only case a Vissim user's hand reaches for. So the choice offered was not "take a verb away" but "fill an empty slot", and the owner declined to fill it. Nothing is lost: `Shift`+click extends a selection and `Ctrl`+drag duplicates. | Do not re-open it from the §1/§2 text alone — those rows describe the 2026-09-14 editor. Re-run the walkthrough first. If a future session gives `Ctrl`+left-click any verb, it must not be one that edits the drawing without a visible result, which is what made this chord dangerous on paper.
| D31 | 2026-09-23 | **A Qt benchmark pumps the event loop between iterations, or it is measuring Qt's deferred work instead of the code** | `QGraphicsScene::clear()` defers reclaiming its index entries to the event loop. A tight timing loop that clears and refills the scene therefore measures an index growing without bound: six identical batches climbed 28 → 159 ms, and the same loop with `processEvents()` stayed flat at 11.5. The harness's answer depended on how many repetitions it was asked for, which is the signature of this class of bug. | Any future harness that drives a Qt object must pump the loop, and any number produced by one that did not must be re-measured before it is quoted. The tell is a result that changes with the repetition count — check that before trusting a Qt timing, the way an interleaved A/B is checked against the noise floor. |
| D32 | 2026-09-24 | **Per-lane demand shares live on `VehicleInput` itself, as optional weights in `routeLaneChains` order** | `VehicleInput` is not wrapped by a separate editor type — `AuthoringDefinition` reuses `ScenarioDefinition`'s `VehicleInput` list directly (`src/model/demand/definition.hpp`) — so it is the only place both the editor and `buildScenario` already read, and the "schema-and-signature decision" the milestone named. Rejected: keying by lane id (the chain's starting lane), which is more robust to the network's lane count changing under an authored route (M1.26's whole point) but adds indirection nothing here asked for yet. Chose the plain positional vector instead, with a deliberately cheap safety net — a size that no longer matches the route's current lane count, or any non-positive weight, degrades to the M1.26 equal split rather than misapplying a weight to the wrong lane. Persisted only when non-empty (`definitionJson`), so an unedited input's saved file and compiled volumes are byte- and bit-identical to before this landed; schema bumped to 9 since the field is new, even though nothing here is version-gated the way network parsing is. `buildScenario` normalises by the weights' sum, so they need not sum to 1. The editor dialog landed later the same session (D33). | If a route's lane count turns out to change often enough in practice that the size-mismatch fallback fires constantly and authors find their shares silently reset, key by the chain's starting lane id instead — the rejected alternative above, not a redesign. The two tests holding the gate (`project.m1_26_1_lane_shares_round_trip_and_stay_out_of_an_unedited_file`, `editor.m1_26_1_lane_shares_weight_the_split_and_degrade_when_stale`) are the ones to extend, not replace, if that happens. |
| D33 | 2026-09-24 | **The vehicle-input dialog writes `laneShares` only when a field was actually touched, never merely displayed** | The dialog has to show *something* in each lane's weight box, and the natural seed for an unset input is the equal split (1 each) — but if accepting the dialog always wrote whatever was showing, opening an unedited input and clicking OK would write an explicit `{1,1,...}` where D32 depends on empty meaning "equal split" for the bit-identical guarantee, and would defeat the size-mismatch fallback the first time anyone merely looked at the dialog after a network edit. A `sharesDirty` flag, set only by `QDoubleSpinBox::valueChanged` and reset whenever the fields are rebuilt (seeded via `QSignalBlocker` so the seeding itself never sets it), makes "touched" the actual question asked, not "was the dialog opened". Rebuilding the fields on every route-combo change was the other half: the lane count is the selected route's, not the input's, so switching routes mid-dialog must not silently resize the wrong array. | If a future field in this dialog needs the same "only write if changed" contract, copy the flag-plus-`QSignalBlocker` pattern rather than inferring dirtiness by comparing values — a user typing the same number back is not a meaningful distinction to make, but it also is not wrong to treat as dirty, and the flag is simpler than either comparison. |
| D34 | 2026-09-24 | **M2's gate criteria are registered: C0–C4 as drafted in `M2_PLAN.md` §3** | Ratified by the owner in session, answering "accept as drafted", before any M2.2+ code. Amendment chosen by the owner the same day: C0, the portfolio audit, is answered before M2.5 produces its first delay figure rather than before any M2 code — its purpose is to be answered blind to TrafficSim's numbers, and until M2.5 there are none. The record lives in `M2_GATE.md`. | If a delay figure is produced before C0 is filled in, C3 stops being blind and the gate is weakened again; the stop point in NEXT exists for that. **C0 and C3, and the C0-before-M2.5 amendment, superseded by D38.** |
| D35 | 2026-09-24 | **M2.0.1 extends M3.1's derived rule to Connectors meeting at a lane's start; it does not start M3** | Asked "can the necessary part of M3 be done first", the owner chose to run the natural four-leg drawing. What that needs is the already-implemented M3.1 rule applied where it was skipped (a joined section starting at 0), ordered by drawing order, as same-station body arrivals already are. Conflict areas, crossing conflicts and authorable priority rules stay M3, behind the M2 gate. | If a study needs a different priority than drawing order, that is authorable priority rules — M3, not a patch here. |
| D36 | 2026-09-24 | **Amber stays red until M4** | Every safety clamp in the four-leg run and six in the frozen TS baselines are vehicles caught at the line by amber; a stop-or-go decision would change frozen fixtures. The owner chose to keep it; M2.5 shows the clamp count beside its figures. | When M4 builds signal control, or a study's delays are visibly driven by it. |
| D37 | 2026-09-24 | **Compositions and the static routing decision move from M2.1 to M2.3/M2.4** | M2's done-condition (counted volumes on the M1 intersection) needs both; M2.1's gate (validated distributions, behaviour parameters) does not. Both expand at compile time into the core's existing inputs, so `core/` and every frozen fixture are untouched. | If a positioned or per-interval decision is needed for the gate study — that remains M2.1. |
| D38 | 2026-09-24 | **The project is a simulator usable in real engineering work; the M2 gate is re-registered as C1 + C2, with C4 recorded** | Owner ruling in session: every reference to comparing against another simulator is removed from the repository, every file and line (git history keeps it), and the purpose is stated positively — a traffic simulation program engineers can use for real work. C0 (portfolio audit) and C3 (the comparison question) existed only to answer that comparison, so both are withdrawn; C1, C2 and C4 keep their numbers. Changed **before any gate observation** — C0 was never answered and M2.5 had produced no figure — so this is a re-registration, not a criterion moved after the fact (D8). M2.5 is unblocked. The owner's supplied spec copies in `docs/specs/` were edited too, on the same instruction. | An observation made before this date that the change could have been tailored to — none exists. |
| D39 | 2026-09-24 | **Movement delay is the run summary's whole-route term, grouped by (entry Link, exit Link)** | Smallest version that adds up: the movements' trips plus `notInMovement` equal the run's completed trips, and each movement's route starts on its approach and ends on its exit anyway. It includes source wait and the entry acceleration from standstill (≈3 s for a car), which is stated beside the table and pinned by a test. | When an engineer needs delay between two cross-sections (a travel-time section), or the entry bias matters to a figure. That is M5's measurement, and it removes the bias. |
| D40 | 2026-09-24 | **A queue counter at every signal head; an approach reports the maximum over its lanes** | Vissim's queue counter at the stop line with its default conditions (5 km/h, 10 km/h, 20 m) as data. It is measured along each route that crosses the line, so a queue spilling back past the pocket into the upstream Link is counted. Mean over every step, and the maximum. | An unsignalised approach (M3) needs a counter with no head, so counters become authorable objects then. |
| D41 | 2026-09-24 | **Prepare the ordered M3 contract without inventing an M2 gate result** | The owner asked to carry out the plan, which explicitly retains the M2 prerequisite. The contract replaces overridden merge groups atomically, detects cyclic/incomplete priority, reserves finite crossing extents through rear clearance and gives unsignalised queues real measurement lines. M3.2.1-M3.2.8 retain the full existing scope. | M2.6 remains the prerequisite for implementation. No owner observation, gate waiver or calibration result is inferred from this instruction. |
| D42 | 2026-09-24 | **A routeless input and a placed decision are expanded at compile time into static routes, one per complete path** | Every choice is random, independent and fixed, so a path's probability is the product along it, and splitting a Poisson stream by fixed probabilities is exact (as for M2.3/M2.4). The engine keeps one static route per vehicle. The rules: an equal share at each way out of a lane (the owner's choice); a decision acts where its Link begins (its station is not modelled); a lane that can reach no destination carries on routeless with an advisory; after its destination a vehicle is routeless again. A revisited lane, or more than 256 paths, blocks Run. | Lane changing (M3.2.8) makes a vehicle's lane a choice rather than a given, so decision legs must then be chosen per vehicle at run time, and this expansion is replaced. |
| D43 | 2026-09-24 | **A decision on the entry Link places each destination's flow in the lanes that reach it** | Measured: on the four-leg drawing, decisions after the entry left the counted proportions unreachable (136:16:39 against 500:120:100), because without lane changing the equal lane split decides the turns. Drivers sort themselves by lane before the junction; putting the flow in those lanes is what that achieves. It ignores the input's lane weights, which the table stops showing. Decisions further downstream still split per lane and are documented as such. | When lane changing exists, and the lane choice becomes the drivers' own. |
| D44 | 2026-09-24 | **A lane remainder shorter than 4.5 m after the last way out is not a network exit** | The owner's network had Connectors clicked 0.35–2.58 m short of Link ends, and the equal-split walk sent a third of the traffic out through the stub. 4.5 m is the shortest shipped vehicle (car), so no vehicle can be meant to drive into less. The owner chose this compile-time rule over snapping in the editor, which would restrict where a Connector can be drawn. A constant in `routeless.hpp`, not data: it is not a vehicle parameter a study tunes. | When a station on a decision is modelled, or a real exit this short turns up. |
| D45 | 2026-09-24 | **A per-interval turning proportion is chosen by the time a vehicle enters the network** | The engine carries one static route per vehicle, so the choice must be made at compile time, where only the entry time is known. The error is the travel time from entry to decision -- seconds against 15-minute counts. Outside every counted interval the whole-period flow applies, so a count table shorter than the run is not refused. A choice at the decision itself needs route choice in `core/`. | When `core/` gets runtime route choice, or a study's decision sits minutes downstream of its entry. |
| D46 | 2026-09-24 | **A decision's counts are proportions of the input's volume, and an uncounted interval uses the whole-period ones** | Owner request: turning counts and approach counts come from different sheets and rarely agree. The input's volume is the authority for how many vehicles enter; the turning counts say only where they go. Refusing an interval with no turn counted would block Run on ordinary data, and sending its vehicles nowhere would lose them. | If a study needs the input derived from the turning counts instead. |
| D47 | 2026-09-25 | **A Signal head is placed at the clicked station, one per lane, and is drawn as its stop line** | Owner report and choice (Vissim: one head per lane). The runtime already stops traffic at the head's station, so the stop line is the head, not a second object; the editor had simply discarded the click. One pick (`nearestHeadSlot`) serves click, hover and Ctrl-drag copy. | If a separate stop-line object is needed (e.g. a stop line away from the head, or unsignalised stop control in M3). |
| D48 | 2026-09-25 | **Signal control is authored as fixed-time controllers with signal groups and compiled into ordinary core programs; schema 12 programs shaped like a group migrate** | Owner report and choice (Vissim's model). A group is what a timing sheet lists; compiling it keeps `core/` and every fixture unchanged. Migration groups programs by cycle length into one controller with offset 0, because that is the only grouping the old file implies; colours are proved identical at every tick. Programs of any other shape stay legacy rather than being refused. | When M4 adds actuated control, intergreens or conflict checks, or if a file's programs of one cycle belong to different junctions and must be split. |
| D49 | 2026-09-25 | **M1 usability accepted by owner ruling, not by the written exercise** | After one timed attempt (9 min 40 s, no assistance; save/reopen exact; no turn pockets, no aerial image), the owner ruled: an engineer who has used another traffic simulator can model an intersection of ordinary complexity in this program in well under 10 minutes. The owner holds the gate, so M1 usability is accepted on that ruling; the record keeps what the attempt did not show (pockets, aerial image, first-attempt and documentation status) so no one reads it as the written exercise passed. M0 plausibility is a separate observation and stays open. | A later attempt by someone new to the program, or one that fails the pocket/aerial task, reopens the question. |
| D50 | 2026-09-25 | **A derived priority rule's stop line is 1 m short of the join, not on it** | Held exactly at the join, the waiting vehicle's front is on the shared lane; the major vehicle behind it stops within the headway and each waits for the other for ever. A Thai left turn at all times arrives at speed during the cross street's green and deadlocked the M2.6 template in its first minute (445 vehicles never entered). 1 m is geometry, not behaviour, so it is a constant (`kYieldClearance`, `sections.cpp`), not a `data/` value. The four-leg fixture's run and `trafficsim-cli 42` are byte-identical before and after. | An authored conflict area (M3.2) places its own stop line and supersedes this for authored rules. |
| D51 | 2026-09-25 | **No test times TrafficSim against Vissim; the owner judges pass or fail** | Owner ruling: withdraw the timing comparison with Vissim from every test, and let the owner assess the result. Made before any M2 gate observation, so it is a re-registration (as D38 was), not a criterion changed after the fact (D8). C2 is withdrawn and keeps its number; C1 still defines the study and C4 is still recorded, since it compares delays, not time. The verdict line in `M2_GATE.md` is the owner's. M1's 10-minute limit was absolute, never against Vissim, and M1 is already accepted (D49). | A later owner ruling. A judgment by the builder alone is the weakness ROADMAP §M2 names; outside engineers still strengthen it. |
| D52 | 2026-09-25 | **C4 withdrawn: no test compares delays with Vissim** | Owner ruling, following D51. Still before any M2 gate observation, so a re-registration, not a changed criterion (D8). C4 was never scored, only recorded; with it goes the "two LOS letters apart → investigation" trigger for the gate. What remains is C1 — the study completed end to end — and the owner's verdict. The Results table is still produced and recorded; it is simply not set beside another tool's. Plausibility against real-world data stays M6's (validation), untouched. | A later owner ruling. |
| D53 | 2026-09-25 | **The M2 gate is passed on the owner's word** | Under D51 the verdict is the owner's, and the owner reported "M2.6 passed". Recorded as *not disproven* (D8). The study's site, counts, file and Results table were not supplied; the record says so rather than filling them in. M3 may start. | Evidence that the study did not meet C1, or a later owner ruling. |
| D54 | 2026-09-25 | **M3.2.2 is split: M3.2.2a ships the authored model, schema 14, commands and the resolver; the reference lifecycle is M3.2.2b** | One system per session. The file/model seam — types, strict codec, History commands and ONE effective-priority resolver used by both compile and diagnostics — is testable on its own (A01–A04, A06–A08). Lifecycle (split/copy/retarget/resize/delete remapping, A05) touches every geometry command and is its own slice; until it lands, deleting a Link or Connector a control names is refused whole (safe but blunt), and a lane change leaves a stale, Run-blocked draft; `rightofway.until_m3_2_2b_*` pins both. Every authored area is Run-blocked (`UNSUPPORTED_CONFLICT_RUNTIME`) until M3.2.3, even an explicit merge the M3.1 mechanism could already run, because the plan says new controls stay blocked until their runtime is implemented. A taken-over merge compiles to exactly the fallback's rule (same 1 m waiting line, D50), so reversing it is the only change an author makes. Stop controls and queue counters are left to M3.2.5/M3.2.6, where their runtime lands. | M3.2.2b, or the M3.2.3 admission solver changing what a compiled area needs. |
| D55 | 2026-09-25 | **Authored controls follow their owners the way signal heads do; a split through one is refused; lane edits never retarget** | Deleting a Link or Connector (including a drag that detaches a Connector) cascades the areas on it, their rules, the lines on it and lines that served only those areas, in the same command, so Undo restores the whole relationship. A split moves stations by the same arithmetic as Connector ends and maps lane ids through the split's replacements; a Connector lane pair whose end moved downstream takes the new lane id. A control in or across the 0.2 m span is refused (`EDIT_SPLIT_CONTROL`) rather than guessed (contract §1, first slice). Copy takes a control only when every owner was copied — a Connector lane pair needs both end Links, because only then do its lane ids map. Lane-count and retarget edits are allowed and keep ids: a pair that no longer matches is a Run-blocked `CONFLICT_UNRESOLVED_PATH`, never an ordinal pick (§6). Reverse is refused like a head. The M3.2.2a scrutiny fixes ride under this number too. A waiting line on a preceding Link and crossing coverage are resolver work, carved to M3.2.2c. Recorded, not fixed: control ids are unique against network ids only. | An editor surface (M3.2.4) that lets an author select a control, which would need copy/delete of the control itself. |
| D62 | 2026-09-25 | **A Stop is served by coming to the line below walking pace and then resting at zero for one whole tick, which the Stop itself enforces; Yield is the existing gap test; the mode belongs to the waiting line** | Contract §5 asks for zero speed at the line, but the reduced car-following model only approaches zero behind an obstacle (0.04 m/s after 29 s), so a literal test never fires. Accepting 0.1 m/s within the gap the model keeps at that pace, then holding the vehicle at zero, keeps the one-tick minimum with no dwell parameter; the rest is ordinary braking, not an emergency clamp, so clamp counts stay honest. One control per line because a physical line cannot be Stop for one area and Yield for another; changing who gives way clears the area's control rather than leaving it on the wrong line |
| D61 | 2026-09-25 | **Conflict areas are picked by their own tool; a click on the selected one cycles priority without a passive state; a dragged line is kept and reported, not clamped; the yielding side is hatched** | Hit-testing areas under Select would steal the Link at every junction, which is the object an author clicks most there; Vissim avoids the same collision with its object-type sidebar. There is no passive state because an unauthored crossing is not an area (M3_PLAN §2); deleting the area is how an author gets one back. Clamping a waiting line at its entry would hide a draft that the resolver already names (`CONFLICT_WAITING_LINE_AFTER_ENTRY`), and authoring does not refuse what Run refuses. A crossing's two sides cover the same square, so one of them must let the other show through |
| D60 | 2026-09-25 | **The conflict-area editor ships table-and-dialog first; canvas gestures follow as M3.2.4b** | Every authored control is reachable by keyboard: the table, Enter, and a dialog whose fields keep the file's parameter names. That route reaches the same commands the pointer does, so A24's "pointer and keyboard submit the same commands" holds for what exists. Add-crossing expands to every overlapping lane pair and says how many before committing (contract §1). Its waiting lines stand 1 m short of entry, the D50 setback, so the result runs without further editing. The canvas draws areas and lines from the resolver's own geometry, so a picture cannot disagree with what runs. Clicking areas, cycling priority and dragging lines need hit-testing that competes with Link selection at every crossing: a separate interaction design, carved to M3.2.4b. | Canvas gestures once the Vissim conflict-area interaction is written up (VISSIM_PARITY §2); drawing two overlapping sides so both stay readable. |
| D59 | 2026-09-25 | **Authored merges run on the zone solver while derived merges stay on M3.1 rules; only hold cycles are refused; requests behind one standing leader share its room** | Contract §4 asks the major side to wait for an admitted minor, which the M3.1 rule cannot express. The solver does, with the same line and thresholds, so an authored merge moves to it. The D58 replay-equality test therefore no longer holds, and the take-over keeps the fallback's numbers rather than its trajectory. Derived merges stay on the rule, because moving them would change the published four-leg and M2.6 numbers (D39); that needs its own decision. A route minor at one zone and major at another deadlocks only if the holds close a loop, and a total priority order cannot close one. So the static waits-for graph replaces the blanket refusal, and a taken-over three-way merge runs. When requests share one standing leader, serving them in vehicle-id order is the contract's stable last tie-break; each takes its length plus standstill. | Queue gridlock, where an admitted vehicle stops inside an area behind a leader that stopped later: admission would have to reserve the downstream space, not only check it. Derived merges on the solver, with the owner's agreement to re-publish the four-leg numbers. |
| D58 | 2026-09-25 | **Zones are resolved per route in route distances; a chain of zones shares one line; authored merges run on their rules; mixed roles are refused** | Route distances make a section cut invisible to admission, as contract §4 asks. A route that turns off frees the area where it leaves, and a major route joining part way is seen from the join. A vehicle has nowhere to wait between two zones when there is less than the longest vehicle plus its standstill between them. Giving both the first line and the last exit makes admission atomic with no new state: it waits until every zone admits, then holds them all. This is conservative, because the later zone's majors wait from the first line. Authored merges already compiled to `PriorityRule`s (D54), the mechanism derived merges run on, so blocking them only withheld a runtime that exists. The evidence is an exactly equal event stream after take-over. A route minor at one zone and major at another can deadlock by mutual hold, and a minor route joining its chain part way never passes the line. Both are refused rather than half-run. | Arbitration that breaks mutual holds (M3.2.3c); merges moved to the zone solver so the major waits for an admitted minor. |
| D57 | 2026-09-25 | **M3.2.3's first slice runs one isolated crossing; a grant is read off positions, not stored; the whole area is reserved** | A minor vehicle past its waiting line with its rear short of the exit holds the crossing. Routes are fixed and positions monotone, so this is the contract's "retain until the rear clears", never revoked, with nothing added to `SimState`: a copied state replays exactly (A25), and stop service arrives with M3.2.5. A major vehicle waits at entry while another vehicle holds. A minor vehicle waits at its line on the M3.1 threshold (inside, within headway, or sooner than gap time; equality passes). It also waits while a standing leader leaves no room past the exit, because an admitted vehicle stopping inside the area would block the major road. The swept check runs over all candidate moves before publishing, so the result does not depend on vehicle order. Reserving the whole area loses capacity and is disclosed. Merges, connected groups (A15), areas over a section cut and receiving space shared between zones are carved to M3.2.3b and refused by name rather than half-run. | Measured capacity loss on the T-junction (M3.2.7); a moving leader that stops after admission leaves the vehicle inside the area — admission reserving the receiving space, not only checking it. |
| D56 | 2026-09-25 | **A waiting line is compiled as metres along the yielding segment, negative on the approach; it resolves only along single predecessors; crossing extents must contain the measured overlap** | "Upstream on every applicable route" (contract §1) is read off the runtime graph instead of the routes. Walking back from the side's entry, a segment with a second predecessor means some vehicle can reach the conflict without crossing the line. That is a named blocker, not a guess. A diverge is harmless: the core applies a rule only to routes through the yielding segment. Keeping the rule on the yielding segment, with a negative position, is what makes it apply to exactly those routes. Putting it on the line's segment would hold vehicles that turn away. The core's bound follows the same chain, so the two cannot disagree. Coverage uses the lane strips vertex for vertex with the authored polyline, so an overlap station is the cross-section `matchedStation` names. A larger extent is the author's choice; a smaller one, no overlap, two overlaps or a folded strip block Run. | Route-aware incidence (M3.2.3) that proves a bypassing route never reaches the area; a folded strip handled by `trimSelfIntersections`-style repair instead of refusal. |
