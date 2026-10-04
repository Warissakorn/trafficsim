# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the history and the
decision log; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it.

---

## Session-checked on Windows, 2026-10-02 — both findings resolved; the owner's look is still open

A session ran every owner item below on this Windows machine. It used the UI suites on the real
`windows` platform at 100/150/200 %, screenshots and the CLI. It could not drive the live desktop.
Results: `docs/evidence/windows-session-check-2026-10-02.md`. **No owner item is closed by it.**
Its two findings are closed. The stale "no lane changing" text was reworded with the owner on
2026-10-03. Digits sitting high at 150/200 % are a recorded limit (D103): the cause is the 13 px face's
deep descent, in every body-font control, and the owner kept the 100 % row alignment. The
`design-system-ui-1.5x`/`-2x` entries hold it to 1.5 logical px.
The canvas grid was soft at every scale other than 100 %, and is now crisp (2026-10-03, PROGRESS).
**The owner looks** at the grid on the 125 % screen: one-device-pixel lines are half as thick, in
logical terms, at 200 % as at 100 %. If that reads too faint, the fix is the line colour, not the width.

Also measured: D90's 40-seed M2.6 numbers are identical on MSVC and Linux. (`scenario-run-ui`'s
cost on Windows is settled: 2026-10-04, PROGRESS.)

## Owner looks at D104 (Results → CSV) on Windows

Run a project to its end, then Results → Export results
(CSV). The file should open in a spreadsheet with the marker line first, then movements, then
approaches. The action stays greyed until the run ends.

## Immediate — owner checks D86 (conflict areas follow the drawing) on Windows

Open `data/projects/t-junction-priority.traffic.json`, Conflict area tool (`A`). Drag Major
eastbound a few metres along and across: "Minor crosses eastbound" and its waiting line move
with the overlap, the status column stays "runs", no grey passive copy appears. Drag it clear of
the turn: the area, its Yield and its line go; Ctrl+Z brings them back. Reshape the crossing
turn: same. Verified on Linux only (headless and offscreen Qt). If an author ever needs an area
deliberately wider than the overlap to survive an edit, that is D86's failure condition.
Later, not booked: a branching (diverge) kind, red-red status, front/rear gap and visibility, and
`sharedMouth` treating a different `level` as no crossing although M3_CONTRACT says drawing level
is not separation — decide that one before touching it.

## Then — owner looks at D83/D84 on Windows (PR #78)

D81–D84 compile and pass on Linux (Qt 6.10) and in CI (Qt 6.5.3, Linux and Windows); nobody has
looked at them on the owner's Windows machine. Check at 100/150/200 % scaling: the dropdown
chevron is visible, spin boxes show two small stacked chevrons, Thai and English share one face
with numbers on the labels' baseline, every field and button is 24 px with its text centred.
Then the gesture rule: a plain left click selects in every tool and never authors; Ctrl+right-click
or Ctrl+right-drag draws Links and Connectors, splits, places heads, inputs, routes and counters,
and sets conflict-area priority. If the owner finds Ctrl+right slow for heavy route/counter work,
that is D84's stated failure condition — record it, do not quietly restore left-click authoring.
Open gaps: `docs/UI_REDESIGN_AUDIT.md` §6 (dock-title tracking, remaining C-locale cell formats;
tabular digits are solved by the face, D84).

## Then — owner looks at the editor interaction cleanup (merged: PRs #73–#75, 2026-09-28/29)

The UI suite covers hover, empty-space/table clearing, mode changes, Escape, tabs at all Link
stations, Connector end tabs inside the mouth and following angled rails, visible P3–P4
boundaries, curved resize previews, Undo and 10 cm markings. Still by eye, in the desktop:
curved/overlapping roads and the tabs at working zoom levels.

## Then — review D80 central axis and four-point mouths

D80 replaces the legacy square/slide mouth system. Read CONNECTOR_FOUR_POINT_MOUTH.md and the
D80 entry (`archive/PROGRESS-2026-09-28-d80-selection.md`) before changing it. The owner explicitly requested fixed lane-index
pairing on both ends, including obtuse arrivals, and a central construction reference.
Linux/GCC 13.3 headless CTest passed 48/48; desktop/Windows review remains outstanding.

1. Review in the desktop on Windows: both ends at 45/90/120/150/170/179 degrees; 1–3 lanes,
   kerb/median added or dropped lanes; drag central grips, save/reopen and Undo/Redo.
2. Exactly parallel **distinct** edge lines (including exactly 180 degrees or parallel unequal
   widths) have no P1–P4 intersection: open selectable rails plus an alignment advisory are
   intentional. Any alternative bounded geometry needs an explicit construction rule.
3. Near 180 degrees, real supporting-line intersections can be hundreds of metres away. They
   are retained. Folded strips remain unsupported by conflict coverage; no square fallback or
   silent point deletion should be reintroduced to hide this.
4. Runtime paths remain the stored first-lane-derived paths; schema-17 files are compatible.
   Conflict extents use the new rails, so existing authored extents can require review.

The Windows `scenario-run-ui` timeout is closed. It now paints every 100th Step, not every Step
(2026-10-04): 2.8 s CPU on a P-core, 11.8 s on an E-core, Windows Debug.

## Then — M3.2.8c: after cooperative braking; the owner's M3.2.7d

**Done** (the reasoning and numbers are in `PROGRESS.md` under each D-number, the evidence in
`docs/evidence/`):
- D95 discretionary lane changes: contract, rows and implementation; off (see below).
- D93/D94 lane changes at a downstream routing decision (Windows/MSVC only).
- D92 `--wait-causes`: 93% of dead-end wait time starts beside a standing target lane.
- D90 cooperative braking with look-ahead; `maxDecelerationCooperativeBraking` default 3 (D92).
- D87–D89 diagnostics: the right-turn rise was a five-seed sample; the East rise is the left-turn
  dead-end waits.
- D71 (M3.2.8b), D69 (M3.2.8a), D68 automatic conflict areas, D54–D67 (M3.2.2a–M3.2.7c).
- **Still open from these:** the owner looks at D72 in the editor (T-junction, Conflict tool).

**Use at least 40 seeds (42–81) for any before/after comparison of one movement (D88).** A
right-turn movement's per-seed change has an SD of about 7 s. The `b472e05` "before" recipe: a
scratch worktree, copy the diagnostic's two files, add its source to `trafficsim_eval`, patch
`runProject` to observe it behind an environment variable, uncommitted, and `cmp` the default
output first.

**D95 / D101 — off, by the owner's choice (ii), 2026-10-02.** Both are implemented and **off**:
no shipped behaviour carries `discretionaryLaneChangeThreshold`, `acceptedDecelerationTrailingVehicle`
or `discretionaryLaneChangeHoldTime`, and every shipped output is unchanged (A51, A58). A53 (as
D101 rewrote it) fails: 4.85% / 5.09% returns within 10 s at threshold 1.5, hold 3 s, against a
proposed 1% cap (`docs/evidence/m3.2.8c-discretionary.md`). The cause is the incentive (PROGRESS
"Why D95's changes reverse"). **When the owner reopens it,** the booked route is (i): look past
the nearest vehicle (E1) and drop the regime edges for the comparison only (E3) — contract §2
item 2, rows, the lab, then A53. Moving the cap (iii) is the other option on record.

**The owner looks at D102 on Windows:** open `data/projects/lane-change-lab.traffic.json`, Run
with Play, watch the Lane drop scene. A changing vehicle should ease across the lane line over
3 s, nose turned slightly toward its new lane, not jump. Verified offscreen on Windows
(`lane-change-display-ui`).

**The lane-change lab (D98).** `data/projects/lane-change-lab.traffic.json`, built from
`tools/lane_change_network.hpp` and tested by `lanelab`. Its four scenes:
- Overtaking
- Three lanes
- Lane drop: the D96 case. It has **no** discretionary span, and the downstream second lane stays
  empty, so D95 does not close D96's run-side gap.
- Diverge

Any lane-change work iterates there first.

Also open, not this session's work:
- **The owner looks at D93 on Windows (desktop):** on a copy of the four-leg drawing, put a
  decision on the West pocket Link (East 3, North 1) with a routeless West input. Run it, and
  watch pocket-lane vehicles change towards their turn. Problems should show no
  LANE_UNSERVED. Session-checked by CLI on Windows (2026-10-02): the split is 118 : 41 : 0 and
  the run completes. Not looked at in the desktop.
- **The owner looks at D96 in the editor:** draw a route 2-lane Link → 1-lane Connector → 2-lane
  Link, once with the Connector on the end and once mid-body. The overlay should show both lanes
  of both Links, with nothing upstream of a mid-body arrival.
- **Linux replay of D91–D94:** the CI run is the evidence. WSL Ubuntu exists here now (3.7 GB);
  `ar` on `/mnt/c` can fail with "Cannot allocate memory" at full parallelism, so build with `-j 4`.

**Not booked, for later sessions:**
- **Split targetStanding (measurement only):** is the target lane standing at its own red, or
  in a queue spilling back from it? Add it to `--wait-causes` only if a behaviour row needs the
  split. That would be one that changes lanes earlier, before the queue reaches the stub's
  span. No row needs it yet.
- **Engine cost after D90 (from 8b):** a callgrind of the M2.6 hour on Linux/GCC 15.2 (another
   machine, 2026-10-01) put lane-change work at ≈3% (`courtesyHolds` 1.9%, `decideLaneChanges`
   1.0%) — not where the time goes. The 9.1% state copy is gone (D91). Still open: a
   **same-compiler** callgrind at `b472e05`, `f9964f1` and HEAD before any new baseline replaces
   4.35G (the 4.01G measured was GCC 15.2 against D70's 13.3, not comparable). Next candidates by
   that profile: the per-tick span rebuild (8.5%, `appendSpans` 6.8%) and `observe` (20%) —
   measure whether `observe` is the per-line walk over every vehicle or `queueLength` and its
   per-line `behind` allocation before changing it.

**The owner looks at D90 on Windows:** run M2.6 (Run in the desktop) and watch the East
approach. Left-turners on lanes 2/3 should slip into lane 1 as they approach the dead end. The
lane-1 vehicle behind slows to let them in, without stopping dead, and the long standing waits at
the dead end should be rarer. Its 40-seed numbers are identical on Windows/MSVC (2026-10-02);
not looked at in the desktop.

**The rest of M3.2.8c** (ROADMAP row), one system per session, rows and contract first:
1. Downstream decisions: done (D93/D94). Free walk with no decision stays lane-fixed, and the
   decision station is not modelled (contract §2, rule 7).
2. Discretionary changes: contract D95, rows A47–A55, implementation above. Visibility at areas
   and a between-lanes state are still unwritten. Today a change is instantaneous in the engine
   (the Run view draws it as a slide, D102), which the contract records as a limit.
3. `laneChangeDistance`, only on a network where changes are measured late (D87; D89 found the
   left-turners also change at the first tick allowed).
4. Vissim's cooperative lane change (a vehicle moving out of the way) is not modelled.

**Build on this machine:** MSVC 14.51, Ninja and Qt 6.8.3 on D: (vcvars64, then
`cmake --preset desktop`) — Windows evidence. WSL Ubuntu (GCC 15.2, Qt 6) gives Linux evidence.

**Open item from M3.2.8a:** five minor vehicles standing or at walking pace within 1 m of the
T-junction's merge line are still clamped in the congested headway arm (seeds 42 and 43). They
are not the commitment case, since their stopping distance is about zero, and they are not
diagnosed. Diagnose them before claiming the minor road clamp-free.

**M3.2.7d is the owner's, not a session's.** Carry out the exercise with the recording sheet in
`docs/M3_ACCEPTANCE.md` §3, on Windows. Until the owner reports it, the row stays pending.

M3.1 supplied merge arbitration only — a deterministic gap-time/headway threshold, not a
calibrated critical-gap model. Derived stop lines sit 1 m short of the join (D50). M3's
done-condition (minor-road delay responds to gap time) is shown on development evidence only;
no gate result is inferred.

## Engineering work that can proceed without the owner, if asked

- **Owner looks at Ctrl+A wireframe (D100) on Windows:** is the link-colour line readable on the
  grid, and do vehicles beside the line on multi-lane roads read well, or should they snap to it?
- Derive an input's interval volumes from its entry decision's turning counts, so a count sheet
  is typed once; today the two are entered separately.
- Per-lane shares (D32; one weight per entry-Link lane since D71): no canvas gesture sets one,
  and the input table row (`refreshDemand`, `src/shell/editor_demand.cpp`) shows the equal-split
  figure even when shares are set.
- Keyboard-only equivalents of the route and vehicle-input gestures (M1.25/M1.26) are still open.
- The entry-acceleration bias in movement delay needs travel-time sections (M5), not a
  correction factor.

## Do not retry without a new measurement

- **Removing the M3.2.8b courtesy (D71):** without cooperation M2.6 was 61.6 s with a 404 s wait.
- **`comfortableDeceleration` for commitment (D69):** it clamped the major road.
- **The Run view "slowing down" over a run (2026-10-02, Linux):** per manual Step 1.9 → ~3.8 ms over
  12,000 steps of M2.6 is the vehicle count warming up, not a leak — after Reset the cost restarts
  low and climbs the same way; scene items stay ≈270, the BSP index and the pending queue are
  ruled out, and ≈67% is Qt widget painting (callgrind). Play paints once per 16 ms frame.
  Windows (D99): painting was 93% of a Step; the grid cache and the hidden-Results skip halved
  12,000 Steps (107 → 57 s). What is left per Step: canvas ≈1.4 ms, window flush ≈1.1 ms, the
  run label's relayout ≈0.8 ms. Measure with `trafficsim-run-view-benchmark`, not by hand.
- **Setting the Run/Pause icon only when it changes (2026-10-02, Windows):** removed the button
  repaint and toolbar relayout, but total layout time stayed 2,439 → 2,438 ms per 3,000 Steps:
  the word-wrapped `editorRunInfo` label relayouts the window every Step anyway. Reverted. The
  lever there is that label's layout (a UI change for the owner), not the action.
- **`scenario-run-ui` wall time as a meter on Windows:** under ctest it swung 24–90 s on unchanged
  code (one set timed out at 90 s). Use the benchmark, or the process's CPU time **pinned to one
  core type**: on this i5-12450H the same Debug run is 26–29 s CPU on P-cores (`ProcessorAffinity`
  0xFF) and 89–104 s on E-cores (0xF00), 2026-10-04. An unpinned CPU time is not comparable across
  runs.
- **One `refreshRun()` per Step instead of two (2026-10-02, Linux):** F6/Space call `pauseRun()`
  then `stepRun()`, both refreshing. Removing the first measured 2,169 → 2,342 ms per 1,000 Steps
  (medians of 5, spreads overlapping) — no effect, reverted. Qt paints once per event-loop pass,
  so the repeat only rebuilds a few table items.
- **`redraw()` copying only the primary Link** (item 6 of the 2026-09-23 pass): −1.2% of
  `redraw()`, inside the clock's spread; a frame is dominated by `QGraphicsItem` construction.
  What is left in a frame is M1.23's culling and LOD.
- **Publishing inline when a scenario has no zone** (M3.2.3a): measured at 52.05M instructions
  against 51.29M for the three-phase loop; reverted. The split costs +4.1% of `stepSimulation`.
- **Not booked:** `compileDocument` (paid once per Run, not per tick) and the `push_back` work
  left in `occupiedSpans`. The per-tick fleet sort is already a merge.
- **Measure the engine with callgrind, not the clock,** when resolving a few percent: wall time
  swung ±7% on 2026-09-23. The editor's timings are steadier (±2%). 2026-09-26 baseline, Release,
  after the D70 pass: the M2.6 template's one-hour run is 4.35G instructions, median 0.45 s wall
  (0.44–0.66 s, five runs; one outlier). What is left there: `stepSimulation` 66%, `observe` 26% (mostly the
  per-line queue walk itself), `compileDocument` 4%.
- **Linux/WSL build (not done; WSL is not installed on the owner's Windows machine):** on the
  machine that measured it, a one-file edit waited 150 s, 134 s of it linking the Debug
  `trafficsim-tests` (101 MB) on `/mnt/c`, and an unbounded `-j` hung a 3 GB WSL. There: a build
  tree on ext4 via an untracked `CMakeUserPresets.json`, `CMAKE_JOB_POOLS=link=1` with
  `CMAKE_JOB_POOL_LINK=link`, `-j2`, and `--target` only what the change needs; lld only if
  that is not enough. Measure before and after.
- **`ctest -j` wall time:** `m26study` was split on 2026-09-30 (its merge case, which runs the
  hour twice, is `m26study_merge`); Linux Debug `-j4` went 35.9 s → 16.3 s median with the
  repaint fix. The longest test is now `m26study_merge` (≈6 s). Add every new test group to
  `TRAFFICSIM_TEST_GROUPS` or `all-model-tests` fails (D70).

## Standing — the owner's items

1. **Record the M0 plausibility observation** — acceleration, queue at red, discharge at green.
   Owner observation, not calibration or M6 validation; the not-yet-validated marker stays.
2. **Decide the name (Q5).** D11's trigger ("end of M1") is live. `Velk` is the strongest recorded
   candidate; `Headway`, `MicroFlow Simulator` and `Veytrix` were rejected (D11 row) — do not
   re-derive them. The owner's decision, not a session's.
3. **Drive M1.19/M1.20 in the desktop editor** — measured at model and command level only. Watch
   for a Connector deleted by a Link drag the author did not expect, and whether half a lane width
   is the right "off the Link" distance (`laneContains`). When checking a mouth by eye, every
   Connector lane should meet the Link lane it feeds, middle on middle; the D80 P1–P4 construction now fixes lane-index pairing at both ends; singular or folded
   geometry is reported without a square fallback.
4. **The M2.6 template** (`data/projects/m2.6-study-template.traffic.json`) has placeholder
   volumes, a guessed timing-window-to-approach mapping and no aerial image; replace them before
   using it for a real study.
5. **Open questions, none blocking:** motorcycles (not shipped; lane sharing is unmodelled and Thai
   counts are motorcycle-heavy); a routing decision's station along its Link (M2.1).

M1.22, M1.23 and M2.1 remain open milestones; `docs/ROADMAP.md` is the authority on each.
