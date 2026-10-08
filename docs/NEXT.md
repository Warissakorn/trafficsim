# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it.

## Driving behaviour improvement — owner instruction, 2026-10-06

The owner authorized the staged plan (D120). M3.3.0 writes the
[assignment contract](reference/DRIVING_BEHAVIOUR.md) and
[delivery/acceptance design](plans/DRIVING_BEHAVIOUR.md); it implements no behavior.
M3.3.1a adds the [measurement observer](reference/DISCHARGE.md) and CLI `--discharge`.
M3.3.1b1 adds vehicle-type follower selection and CLI windows/warmup/ranks (D122);
see [evidence](evidence/discharge-controls.md). Do not treat inverse selected-type
headway as selected-type throughput; original ranks/predecessors remain intact.
M3.3.1b2a captures parsed-byte project/catalog/queue hashes and supports positioned
route recognition on identical physical prefixes (D123). Diversions suppress raw
old-head crossing inference; see [evidence](evidence/discharge-provenance.md).
M3.3.1b2b1 (D124) reconstructs proven source and post-remap longitudinal passages
without reading display traces; see [passage evidence](evidence/discharge-passage.md).
M3.3.1b2b2 (D125) scopes remap invalidation to vehicles queued at Go while ranks
1..steadyLast are open, and names same-tick source-sink types through core's pure
`upcomingArrivals` replay with an exact fingerprint; see
[identity evidence](evidence/discharge-identity.md). Measurement (M3.3.1) is complete
pending native CI on the merged stack; BA05 has focused evidence, not owner validation.
M3.3.2a (D126) stores the project-owned behaviour library in schema 21 — model-tagged
behaviours, vehicle classes, link behaviour types and Link/Connector assignment — with
History commands for put/duplicate/assign/delete-with-replacement; see
[contract §7](reference/DRIVING_BEHAVIOUR.md#7-implemented-library-and-codec-m332a-d126)
and [evidence](evidence/behaviour-library.md). M3.3.2b (D127) compiles road assignments into per-segment selections and every
consumer uses one `effectiveBehaviour` chosen by the front segment; unassigned runs
are unchanged and the interim Run refusal is gone. See
[contract §8](reference/DRIVING_BEHAVIOUR.md#8-implemented-runtime-selection-m332b-d127)
and [evidence](evidence/behaviour-selection.md). Confirm this PR's native
Linux/Windows CI independently. BA14 (positioned routing) and BA17 (Stop service
under a changing profile) now have focused fixtures in the same evidence.
M3.3.2c (D128) adds the *Driving behaviours* dialog on the Inputs toolbar (staged,
one History step, users-first edits, delete with replacement) and the inspector's
*Behaviour type* that assigns every selected Link/Connector at once and shows each
vehicle type's effective behaviour and its source; see
[contract §9](reference/DRIVING_BEHAVIOUR.md#9-implemented-editor-m332c-d128) and
[evidence](evidence/behaviour-editor.md). Confirm this PR's Windows UI jobs.
**Owner review (Windows):** open a project, capture the catalogs via the dialog,
duplicate a behaviour, make a heavy-vehicle class and an urban behaviour type with an
override, assign it to several roads, check the effective list, Undo/Redo, delete
with replacement, switch to Thai, save and reopen, then run.
The **M3.3.3a W74 contract** is written (D129, [W74](reference/W74.md)). The pure
`w74Acceleration`/`w74Thresholds` exist in `src/core/w74.hpp` with BA21–BA22 focused
evidence (D130, [evidence](evidence/w74-pure-function.md)); `Leader::acceleration` is
still 0 everywhere. The schema-22 codec and BA29 are done (D131,
[evidence](evidence/w74-codec.md)): a file stores `w74` behaviours, Run refuses one in use
(`UNSUPPORTED_BEHAVIOUR_MODEL_RUN`), the dialog shows it read-only. Next, one slice
each: the §5 trait hash (`W74Traits` on `PendingVehicle`, splitmix64 + Irwin–Hall, never
touching `randomState`) with `W74State` on `Vehicle`, and BA24–BA26; then composition and
BA28 (fill every `Leader` site first, then lift the Run refusal), and record BA27's
timestep sensitivity. W74 parameter editing in the dialog is a separate UI slice. Keep prototype runs
byte-identical (BA18); do not ship a preset in the same slice. The owner may want to
read §5 (traits) and §7 (sign hysteresis) before the runtime slices. PCU/calibration
gates remain open.
Keep the owner's existing reviews below; no observation/validation gate closes.

## Positioned routing decisions — owner instruction, 2026-10-06

D119 implements M2.1.3 ([contract](reference/POSITIONED_ROUTING.md)). Check this PR's
native Linux/Windows CI independently. On Windows, click a mid-Link source, hover to
an exit, add a second Route from the same line, edit weights/time/type rules, then run
routeless input traffic. Verify recognition at the line, downstream lane-change awareness,
Undo/Redo, line drag/cancellation, save/reopen and legacy files. Review the documented
one-timestep recognition boundary before using this feature in a study. Multiple points
on the same Link remain unsupported; a point may be repositioned. No owner or simulation
validation gate closes. Continue the existing queues below after this review.

## Road crossbars and Route trace — owner instruction, 2026-10-06

D116 implements the owner's common crossbar appearance and click/hover/click Route gesture.
Review this PR's native Linux/Windows CI. On Windows at 100/150/200 %, inspect curved and
multi-lane roads, Connector-mounted heads and overlapping Route/input endpoints. Trace a chosen
branch, click the destination, Undo/Redo, Backspace, cancel, and save/reopen. Signal Run bars
must stay at the stop position and change colour with the program; Reset restores Edit bars.
Input zero-weight lanes must remain unmarked. No owner or simulation-validation gate closes.
After this review, continue the conflict and Demand review queues below.

## Conflict overlap priority — owner instruction, 2026-10-06

The owner authorized the overlap/type/display plan. M3.2.4d (D115) retains exact polygon
clips and labels merge/branching/continuation mouths. The owner refined display in D117/D118:
separate bands follow each driving direction with 0.5 m normal rail offsets capped at
20% of local width; ends keep measured cuts. Picking follows the visible bands, grouping
uses physical polygons. Review the PR's native Linux/Windows CI separately. On the owner's desktop,
inspect an oblique crossing, a Connector landing mid-Link across another lane, and two
Connectors leaving the same lane: bands must follow each lane and leave its rails readable; branching
is selectable with no priority editing. At a connected site of the same Link/Connector pair,
all three kinds share one row; edit Crossing/Merge together while Branching stays read-only.
Inspect source/target P3–P4 band continuation on attached Link lanes for both traffic sides,
including obtuse/curved mouths, a taper and finite Link ends. Check geometry edits, Undo/Redo
and save/reopen. Separated sites must remain separate; rejected merge-order cycles must leave
all members unchanged.
Inspect a narrow/tapered lane and short mouth too; the adaptive offset must not erase a band.
D115's shared polygon fill is superseded by these directional bands. Existing merge
station extents/engine rules remain unchanged; M3.2.4e requires a separate admission contract
and failure-first clearance tests before widening runtime reservations to the physical mouth.
After this review, retain the Demand review queue below. No merge or owner gate is closed.

## Demand priority — owner instruction, 2026-10-05

The owner explicitly asked to start Demand improvements. This takes priority over
§0's unanswered ordering proposal. [DEMAND_IMPROVEMENT.md](plans/DEMAND_IMPROVEMENT.md)
defines slices 1–6: correctness, lossless interval editing, explicit periods and
compiled preview first; project-owned vehicle/composition catalogs and typed/time
rules next. Reporting, LOS and batch evaluation stay M5. Slices 1–4 (PR #106)
passed native Linux/Windows CI. Slice 5 (#107) adds project-owned catalogs and editing;
its five native jobs passed too. Slice 6 adds schema-19 time/type rules and staged
editors ([DEMAND_TIME_TYPES.md](reference/DEMAND_TIME_TYPES.md)); check this slice's native CI
and owner appearance independently. The six implementation slices are present;
M2.8 stays in progress until its own review gates pass. Next review the stacked PRs
in dependency order (#106, #107, then time/type), retarget after each merge, and
exercise catalog periods and type overrides in the owner's desktop. No automatic
merge or whole-Demand completion claim. Additional distributions, exact-count or
dynamic-routing work requires its own contract; reporting remains M5.

## 0 — The owner decides the order (roadmap review, 2026-10-03)

[ROADMAP.md — review record](ROADMAP.md#review-record--2026-10-03) corrected the stale status lines
and proposes an order, but **changes none.** Its finding is that the back half of the success
sentence has no product code and no commit since 2026-09-25: 10 seeds, LOS, and a table to paste
(M5). Most of the items below are owner looks. Its §4 asks the owner ten questions (O1–O10).
- **O1** decides whether the next sessions are its S0–S3 ahead of what follows here: the M6
  benchmark option sheet, then the M5 batch runner, an editor batch run, and Results
  copy/export.
- **D104 has done part of S3:** a single run's CSV export, with bytes equal to the CLI's. The
  clipboard copy and the batch export are still to do.
- Until the owner answers, this file's order stands.
- S4 (the input row's lane-share figure) is done (2026-10-03, `laneSplit`). S5 (the T-junction
  clamps, plus the session-fillable `plans/M3_ACCEPTANCE.md` §4 rows) needs no answer; it is in §3.

Sections: [0 Order](#0--the-owner-decides-the-order-roadmap-review-2026-10-03) · [1 Owner checks on Windows](#1--owner-checks-on-windows) ·
[2 Owner decisions](#2--owner-decisions) · [3 Session work](#3--session-work-without-the-owner) ·
[4 Working notes](#4--working-notes) · [5 Do not retry](#5--do-not-retry-without-a-new-measurement)

---

## 1 — Owner checks on Windows

Each item is merged and tested. Nobody has looked at it in the desktop on the owner's machine.
A session cannot close these. The 2026-10-02 session check ran the UI suites on the real
`windows` platform at 100/150/200 % (`docs/evidence/windows-session-check-2026-10-02.md`). It
closed no owner item.

In order:

- **Rear-axle turning (2026-10-05, phases 1–3):** inspect car and heavy-vehicle noses
  entering/leaving a curved Connector, a short Connector and an internal Link join.
  The front remains at its runtime station; the rear axle cuts inside a curve, with
  rigid wheelbase/overhangs and continuous heading. Check lane-change guidance
  and low zoom too: a stopped changer must hold its body, a second change must continue
  the first, and heading must settle after the front arrives. The heavy vehicle remains
  one rigid body, with no trailer joint.
  Automated Linux/Windows results belong to the PR; an owner look remains separate.

- **D109/D110 conflict controls and Bézier coverage:** inspect a curved Connector after an interior
  geometry drag: overlap highlights, coverage and physical waiting positions follow
  the edited lane rails/centres (D114). Inspect a 3 × 3 crossing: nine lane-pair band pairs, one row, nine
  lane-pair controls, group edit/Delete
  and Undo. Two separated intersections of the same owners must remain two rows. Check
  Windows native CI and owner appearance before claiming cross-platform verification.
  At internal and terminal Connector/Link attachments, compare Add with automatic areas:
  a mouth must remain topology/merge; a neighbouring lane genuinely swept stays a crossing.

- **D114 edited Connector lane centres:** run a curved 3 → 3 Connector and both taper
  sides, drag an interior point while keeping its attachments fixed, then rerun. Vehicles,
  heads, waiting bars and conflict coverage must follow the edited lanes. Check Zero points,
  Reset straight, widths, save/reopen and Undo/Redo. The old D107 point-count/motion invariance
  is superseded by this owner's instruction. Verify this branch's native Linux/Windows CI
  independently and inspect actual desktop appearance; headless tests close no owner gate.

1. **D104 Results → CSV.** Run a project to its end, then Results → Export results (CSV). It
   should open in a spreadsheet with the marker line first, then movements, then approaches.
   The action stays greyed until the run ends.
2. **Canvas grid at 125 %** (2026-10-03). Lines are one device pixel, so at 200 % they are half as
   thick, in logical terms, as at 100 %. If that reads too faint, fix the line colour, not the
   width. Digits sit up to 1.5 logical px high at 150/200 %. That is a recorded limit (D103),
   held by `design-system-ui-1.5x`/`-2x`.
3. **D86 conflict areas follow the drawing.** Open `data/projects/t-junction-priority.traffic.json`
   and the Conflict area tool (`A`).
   - Drag Major eastbound a few metres along and across. "Minor crosses eastbound" and its waiting
     line should move with the overlap, the status should stay "runs", and no grey passive copy
     should appear.
   - Drag it clear of the turn. The area, its Yield and its line should go, and Ctrl+Z should
     bring them back. Reshaping the crossing turn should behave the same.
   - An author needing an area wider than the overlap to survive an edit is D86's failure
     condition. Verified at command level on Linux and Windows (`conflict_follow`,
     `conflict-auto-ui`); not yet seen dragged in the desktop.
   - The same tool covers D72 (conflict areas on every overlap).
4. **D83/D84 controls and gestures (PRs #76–#78)**, at 100/150/200 %. A session ran their
   suites on this machine (2026-10-02); the owner has not looked.
   - Controls: the dropdown chevron is visible; spin boxes show two stacked chevrons; Thai and
     English share one face with numbers on the labels' baseline; every field and button is 24 px
     with centred text at 100 % (at 150/200 %, see item 2).
   - Gestures: a plain left click selects in every tool and never authors. Ctrl+right-click or
     -drag draws Links and Connectors, splits, places heads, inputs, routes and counters, and sets
     conflict-area priority.
   - If Ctrl+right is slow for heavy route/counter work, that is D84's failure condition. Record
     it; do not quietly restore left-click authoring.
   - Open gaps: `audits/UI_REDESIGN_AUDIT.md` §6 (dock-title tracking, remaining C-locale cell formats).
5. **Editor interaction cleanup (PRs #73–#75).** The UI suite covers the mechanics. Look by eye
   at curved and overlapping roads, and at the tabs at working zoom levels.
6. **D80 central axis and four-point mouths.** Read `reference/CONNECTOR_FOUR_POINT_MOUTH.md` and
   `archive/PROGRESS-2026-09-28-d80-selection.md` first. The owner asked for fixed lane-index
   pairing on both ends, including obtuse arrivals.
   - Check both ends at 45/90/120/150/170/179°, with 1–3 lanes and kerb/median lanes added or
     dropped. Drag the central grips, save and reopen, Undo and Redo.
   - Exactly parallel distinct edge lines have no P1–P4 intersection. Open rails plus an
     alignment advisory are intentional.
   - Near 180° the intersections can be hundreds of metres away, and are kept. Folded strips are
     unsupported by conflict coverage. Do not reintroduce a square fallback.
   - D107 runtime paths use the existing cubic equation independently of drawing points.
     Schema-17 files remain readable; mapped conflict extents may need review.
7. **D102 lane-change display, superseded by phase 3.** Open `data/projects/lane-change-lab.traffic.json`, Play,
   and watch the Lane drop scene. A changer should ease across with rear rolling on a
   spatial guide, hold when stopped and continue during repeated changes. Phase 3
   replaces the fixed three-second slide. Windows evidence belongs to its PR.
8. **D90 cooperative braking.** Run M2.6 and watch the East approach. Left-turners on lanes 2/3
   should slip into lane 1, and the lane-1 vehicle behind should slow without stopping dead.
   Long standing waits at the dead end should be rarer. Its 40-seed numbers are identical on
   MSVC and Linux.
9. **D93 downstream decision.** On a copy of the four-leg drawing, put a decision on the West
   pocket Link (East 3, North 1) with a routeless West input. Pocket-lane vehicles should change
   towards their turn, with no LANE_UNSERVED. The CLI on Windows gives a 118 : 41 : 0 split.
10. **D96 route overlay.** Draw a route 2-lane Link → 1-lane Connector → 2-lane Link, once with
    the Connector on the end and once mid-body. Both lanes of both Links should show, with nothing
    upstream of a mid-body arrival.
11. **D100 Ctrl+A wireframe.** Is the link-colour line readable on the grid? Do vehicles beside
    the line on multi-lane roads read well? Vehicles stay in their lanes by the owner's choice;
    wanting them on the line is D100's failure condition.
12. **M1.19/M1.20 in the desktop editor** (measured at model and command level only).
    - Watch for a Connector deleted by a Link drag the author did not expect.
    - Is half a lane width the right "off the Link" distance (`laneContains`)?
    - Every Connector lane should meet the Link lane it feeds, middle on middle.

## 2 — Owner decisions

1. **M0 plausibility observation:** acceleration, queue at red, discharge at green. This is an
   observation, not calibration or M6 validation. The not-yet-validated marker stays.
2. **M3.2.7d exercise:** use the recording sheet in `plans/M3_ACCEPTANCE.md` §3, on Windows. The row
   stays pending until the owner reports it.
3. **The name (Q5).** D11's trigger ("end of M1") is live. `Velk` is the strongest recorded
   candidate. `Headway` and `MicroFlow Simulator` were rejected and `Veytrix` set aside (D11
   row); do not re-derive them.
4. **D95/D101 discretionary lane changes:** off by the owner's choice (ii), 2026-10-02.
   - Both are implemented, and no shipped behaviour sets `discretionaryLaneChangeThreshold`,
     `acceptedDecelerationTrailingVehicle` or `discretionaryLaneChangeHoldTime` (A51, A58).
   - A53 fails: 4.85% / 5.09% returns within 10 s at threshold 1.5, hold 3 s, against a 1% cap
     (`evidence/m3.2.8c-discretionary.md`). The cause is the incentive (PROGRESS "Why D95's
     changes reverse").
   - If reopened, the booked route is (i): look past the nearest vehicle (E1) and drop the regime
     edges for the comparison only (E3). That means contract §2 item 2, rows, the lab, then A53.
     Moving the cap (iii) is the other option on record.
5. **`sharedMouth` and `level`:** a different drawing `level` counts as no crossing, although
   M3_CONTRACT says drawing level is not separation. Decide before touching it.
6. **The M2.6 template** (`data/projects/m2.6-study-template.traffic.json`) has placeholder
   volumes, a guessed timing-window-to-approach mapping and no aerial image. Replace them before
   using it for a real study.
7. **Open questions, none blocking:** motorcycles (not shipped; lane sharing is unmodelled and
   Thai counts are motorcycle-heavy); continuous-time or multiple same-Link routing stations beyond M2.1.3.

M1.22, M1.23, M2.1 and M2.7 remain open milestones (M2.7 closes on the owner's use in M2.6);
`ROADMAP.md` is the authority on each.

## 3 — Session work without the owner

Pick one per session, as the user asks. Rows and contract come first for engine work.

- **Vehicle turning after phase 3:** rear-axle display, optional axle data and continuous
  lane-change guidance are implemented ([contract](reference/VEHICLE_POSE.md)). Next engine work
  needs separate acceptance rows for swept-body conflict clearance and between-lanes
  occupancy. Audit where `occupiedSpans`/conflict reservations use scalar length;
  reproduce a rigid heavy vehicle whose displayed swept body exceeds that envelope
  before changing admission.
  Steering feasibility and articulated trailer joints are separate slices. Keep the
  traffic front-bumper distance contract distinct from displayed axle coordinates;
  owner appearance review does not validate swept paths or traffic behaviour.

- **M3.2.8a.1 — moving merge anticipation.** D108 completes the source first-step
  slice (M3.2.8a.1a): queue entry waits until ordinary following/integration fits the
  current leader clearance. On D107 geometry, 120 stress runs remove 3 source clamps;
  21 moving minor clamps remain. Read [source evidence](evidence/source-first-step.md),
  the original [diagnosis](evidence/m3.2.8a-clamps.md) and ROADMAP first. Trace the current
  moving cases, then write anticipation/failure-first rows before changing motion.
  Preserve the buffer, D50 setback, maximum deceleration and genuine emergency reporting.
  Post-entry lane changes can introduce a new leader; the source guard does not cover that.
  Do not claim clamp-free or close M0/M3 owner observations.
- **Volumes from turning counts.** Derive an input's interval volumes from its entry decision's
  turning counts, so a count sheet is typed once. Today both are entered separately.
- **Per-lane shares (D32; one weight per entry-Link lane since D71).** No canvas gesture sets one.
  The input table row shows set weights since 2026-10-03 (`laneSplit`).
- **Keyboard-only equivalents** of the route and vehicle-input gestures (M1.25/M1.26).
- **The rest of M3.2.8c** (ROADMAP row):
  - Discretionary changes (D95, rows A47–A55; hold D101, rows A56–A58; both off): visibility at
    areas and a between-lanes state are unwritten. A change
    is instantaneous in the engine; the Run view uses a continuous rear-rolling spatial guide (phase 3).
  - `laneChangeDistance`, only on a network where changes are measured late (D87; D89 found
    left-turners change at the first tick allowed).
  - Vissim's cooperative lane change (a vehicle moving out of the way) is not modelled.
  - Free walk with no decision stays lane-fixed, and positioned recognition now follows M2.1.3
    ([contract](reference/POSITIONED_ROUTING.md)).
- **Conflict areas, later, not booked:** a branching (diverge) kind, red-red status, front/rear
  gap and visibility.
- **Split targetStanding (measurement only).** Is the target lane standing at its own red, or in
  a queue spilling back? Add it to `--wait-causes` only when a behaviour row needs it.
- **Engine cost after D90.** Linux/GCC 15.2 callgrind (2026-10-01): lane-change work ≈3%; the
  9.1% state copy is gone (D91).
  - Before any new baseline replaces 4.35G, run a **same-compiler** callgrind at `b472e05`,
    `f9964f1` and HEAD. The 4.01G was GCC 15.2 against D70's 13.3, so not comparable.
  - 2026-10-08, MSVC Release on P-cores, M2.6 CLI run (wall median 634 → 568 ms): `observe`'s
    per-line walk and queue matching (155 → 97 ms) and publish's vehicle copy (98 → 88 ms) are
    done. Split now: compile ≈116, step ≈316, observe ≈97 ms.
  - Next candidates, measured but not changed: **compile** walks `routelessChains` 100 times per
    Run (≈0.9 ms each; `connectorPaths` is only 20 ms of it), because `validateDocument`,
    `routelessIssues` and `expandRouteless` each walk every input × time/type slice — share or
    memoise the walk. Then the double refs/spans build on ticks with a source candidate
    (`simulation.cpp` arrivals vs the main snapshot, ≈40 ms) and phase 1 (≈110 ms). The
    `MovedEvent::segmentId` string copy would need an event-interface decision.
- **Linux replay of D91–D94:** CI (`native.yml`) is the evidence.
- **`plans/M3_ACCEPTANCE.md` §4:** the rows a session can fill ([ROADMAP review §7, S5](ROADMAP.md#7-proposed-sequence--owner-free-sessions-after-o1)).
- The entry-acceleration bias in movement delay needs travel-time sections (M5), not a correction
  factor.

## 4 — Working notes

- **Seeds.** Use at least 40 seeds (42–81) for any before/after comparison of one movement (D88).
  A right-turn movement's per-seed change has an SD of about 7 s.
- **The `b472e05` "before" recipe:**
  1. Make a scratch worktree and copy in the diagnostic's two files.
  2. Add its source to `trafficsim_eval`.
  3. Patch `runProject`, uncommitted, to observe it behind an environment variable.
  4. `cmp` the default output first.
- **Lane-change lab (D98):** `data/projects/lane-change-lab.traffic.json`, built from
  `tools/lane_change_network.hpp`, tested by `lanelab`. Any lane-change work iterates there first.
  - Scenes: Overtaking, Three lanes, Lane drop, Diverge.
  - Lane drop is the D96 case. It has no discretionary span, and the downstream second lane stays
    empty, so D95 does not close D96's run-side gap.
- **Build on this machine:** MSVC 14.51, Ninja and Qt 6.8.3 on D: (vcvars64, then
  `cmake --preset desktop`) gives Windows evidence.
  - **WSL is not installed here** (`wsl.exe -l -v`, 2026-10-03). Linux evidence comes from CI
    (`native.yml`) or another Linux/WSL machine.
  - On a 3.7 GB WSL machine, `ar` on `/mnt/c` failed with "Cannot allocate memory" at full
    parallelism; use `-j 4` there. On another, a one-file edit waited 150 s (134 s linking the 101 MB Debug
    `trafficsim-tests` on `/mnt/c`), and an unbounded `-j` hung a 3 GB WSL. The remedy: a build
    tree on ext4 via an untracked `CMakeUserPresets.json`, `CMAKE_JOB_POOLS=link=1` with
    `CMAKE_JOB_POOL_LINK=link`, `-j2`, and `--target` only what the change needs.
- **Timing on this machine:** the i5-12450H is hybrid. The same Debug run is 26–29 s CPU on
  P-cores (`ProcessorAffinity` 0xFF) and 89–104 s on E-cores (0xF00), 2026-10-04. Pin to one
  core type before comparing. Wall time under ctest swung 24–90 s on unchanged code.
- **Tests:** add every new model test group to `TRAFFICSIM_TEST_GROUPS`, or `all-model-tests`
  fails (D70). The longest test is `m26study_merge` (≈6 s, Linux). `scenario-run-ui` paints
  every 100th Step (2.8 s CPU on a P-core).
- **What M3 shows:** M3.1 supplied merge arbitration only, a deterministic gap-time/headway
  threshold, not a calibrated critical-gap model. Derived stop lines sit 1 m short of the join
  (D50). M3's done-condition (minor-road delay responds to gap time) rests on development
  evidence only; no gate result is inferred.

## 5 — Do not retry without a new measurement

- **Removing the M3.2.8b courtesy (D71):** without cooperation M2.6 was 61.6 s with a 404 s wait.
- **`comfortableDeceleration` for commitment (D69):** it clamped the major road.
- **The Run view "slowing down" over a run (2026-10-02):** this is the vehicle count warming up,
  not a leak. Per manual Step, cost rises 1.9 → ~3.8 ms over 12,000 steps of M2.6, and after
  Reset it restarts low and climbs the same way.
  - Scene items stay ≈270; the BSP index and the pending queue are ruled out. ≈67% is Qt widget
    painting (callgrind, Linux).
  - Windows (D99): painting was 93% of a Step. The grid cache and the hidden-Results skip halved
    12,000 Steps (107 → 57 s).
  - Left per Step: canvas ≈1.4 ms, window flush ≈1.1 ms, run label relayout ≈0.8 ms. Measure
    with `trafficsim-run-view-benchmark`, not by hand.
- **Setting the Run/Pause icon only when it changes:** layout stayed 2,439 → 2,438 ms per 3,000
  Steps, because the word-wrapped `editorRunInfo` label relayouts the window every Step anyway.
  Reverted. The lever is that label's layout, a UI change for the owner.
- **One `refreshRun()` per Step instead of two:** 2,169 → 2,342 ms per 1,000 Steps, inside the
  spread. Reverted. Qt paints once per event-loop pass.
- **`redraw()` copying only the primary Link:** −1.2% of `redraw()`, inside the clock's spread.
  What is left in a frame is M1.23's culling and LOD.
- **Publishing inline when a scenario has no zone (M3.2.3a):** 52.05M against 51.29M instructions
  for the three-phase loop. Reverted.
- **Not booked:** `compileDocument` (paid once per Run) and the `push_back` work left in
  `occupiedSpans`. The per-tick fleet sort is already a merge.
- **Measure the engine with callgrind, not the clock,** when resolving a few percent: wall time
  swung ±7%.
  - Baseline (2026-09-26, Release, after D70): the M2.6 one-hour run is 4.35G instructions,
    median 0.45 s wall.
  - Split: `stepSimulation` 66%, `observe` 26%, `compileDocument` 4%.
