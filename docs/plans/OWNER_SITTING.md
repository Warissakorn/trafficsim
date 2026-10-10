# Owner sitting — everything that waits on the owner, in one pass

The owner's answer to the 2026-10-09 review's R4 (D143): clear the queue in **one sitting** with
this sheet, on the owner's Windows machine, and carve out whatever the sitting does not reach.
This is the only list of owner items; [NEXT](../NEXT.md) points here instead of keeping a copy
(hard rule 3). A session prepares the build and prints this sheet; it records the owner's words
verbatim and fills in **no verdict itself**.

**Rules.**
- Each item names what to open, what to do, and its failure condition as a yes/no question.
  Write the owner's answer on its *Owner:* line with the date, then move the item's result to its
  ROADMAP/PROGRESS line and delete it here.
- **Carve-out (R4 b).** After the sitting, any item still unanswered and listed for more than 14
  days becomes a numbered carve-out in [ROADMAP](../ROADMAP.md) under its milestone (rule 2), and
  leaves this sheet. A look is never closed by a session (only a new D-row could change that).
- *Listed* is the date of the decision or merge that created the item.

Build for the sitting: `cmake --preset desktop`, `cmake --build --preset desktop`, then
`trafficsim-desktop`. Screen at 100 %, then 125/150/200 % where an item says so.

---

## A — Gates (first: a fail here changes what comes next)

1. **M0 plausibility** (listed 2026-09-11). Open `data/scenarios/crossing.json`, Run. Do vehicles
   accelerate, queue at red and discharge at green in a way an engineer recognises? A *no*
   triggers D13 (fix or replace car-following) before W74/W99 or M6. Observation only, not
   calibration. *Owner:*
2. **M3.2.7d T-junction exercise** (listed 2026-09-26). Fill the sheet in
   [M3_ACCEPTANCE](M3_ACCEPTANCE.md) §3 on `data/projects/t-junction-priority.traffic.json`. Does
   minor-road delay respond to gap time as an engineer expects? It is what M3.2 closes on (R7).
   *Owner:*
3. **M5.7 rehearsal** (listed 2026-10-08). The PROBLEM §4 sentence on a real aerial image and the
   M2.6 template with real volumes (B4), Run seeds 42-51, Copy into the report. Record file,
   commit, table and the image size against the 32 MiB cap; labelled partial until M6. Did any
   step need a workaround, hand-edited JSON or outside help? *Owner:*

## B — Decisions in chat (one line each)

1. **The name (Q5)** (listed at the end of M1, D49). D11's trigger ("end of M1") is live. `Velk` is the
   strongest recorded candidate; `Headway` and `MicroFlow Simulator` were rejected and `Veytrix`
   set aside (D11). *Owner:*
2. **D95/D101 discretionary lane changes** (listed 2026-10-02). Off by the owner's choice (ii).
   A53 fails (4.85 % / 5.09 % returns within 10 s against a 1 % cap,
   `evidence/m3.2.8c-discretionary.md`). Keep off, reopen by route (i) (look past the nearest
   vehicle, E1/E3), or move the cap (iii)? Now part of M3.4 (R7). *Owner:*
3. **`sharedMouth` and `level`** (listed before 2026-10-03). A different drawing `level` counts as no
   crossing, although M3_CONTRACT says level is not separation. Keep or change? *Owner:*
4. **The M2.6 template** (listed 2026-09-25). `data/projects/m2.6-study-template.traffic.json`
   has placeholder volumes, a guessed timing-to-approach mapping and no aerial image. Supply the
   real study's counts, timing sheet and image for M5.7 (A3)? *Owner:*
5. **Standstill cap** (listed 2026-10-08, [W74 clamp trace](../evidence/w74-clamps.md)). The hard
   cap treats a moving leader as standing; it clamps in every model. Amber itself is now M4.2
   (R2). Book the cap as its own slice, or leave it? *Owner:*
6. **Routing stations** (listed 2026-10-06). Continuous-time or several stations on one Link,
   beyond M2.1.3: wanted? *Owner:*
7. **Multi-level modelling parameters** (listed 2026-10-09, D145). The six questions in
   [MULTI_LEVEL_MODELLING](MULTI_LEVEL_MODELLING.md) §8: Meso formulation, default VDF and its
   citation, deterrence form and trip purposes, the mode set (with PROBLEM §8), convergence
   defaults, on-street parking in M8 or later. M10/M11 contracts wait on 1–5. *Owner:*
8. **The example's cool-down** (listed 2026-10-09, D146). `four-leg-signalised` stays schema 17
   with no cool-down, so its batch still warns on 11 of 12 movements; 300 s clears every window
   trip ([evidence](../evidence/m5.9-cooldown.md)). Give the example a cool-down (its tests'
   expected numbers change), or keep it as the before case? *Owner:*
9. **M6 benchmark choice (Q4)** (listed 2026-10-10, M6.0). Read the
   [option sheet](../evidence/m6-benchmark-options.md) and answer its §7 by letter: Q4's kind of
   benchmark, a primary and secondary option for S1 (saturation flow) and S2 (delay), and for U
   (minor-movement capacity) the option and whether U-e is booked as the R7 calibration path. The sheet holds no engine numbers; the tolerance comes after this
   answer and before any comparable run. Which options? *Owner:*

## C — Run view (the 125 % screen)

1. **D141 Run seeds** (listed 2026-10-09). On the M2.6 template: Results → Run seeds, Cancel part
   way, run again, Copy into Excel/Word, Export; Thai strings. Does the window stay responsive,
   and does the pasted table land column by column? *Owner:*
2. **D104 Results → CSV** (listed 2026-10-04). Run to the end, Export results (CSV). Does it open
   in a spreadsheet, marker line first? Greyed before the end? *Owner:*
3. **Rear-axle turning, phases 1–3** (listed 2026-10-05). Car and heavy vehicle on a curved
   Connector, a short Connector and an internal Link join; lane-change guidance at low zoom. Does
   the rear axle cut inside the curve with continuous heading, a stopped changer hold its body,
   and a second change continue the first? *Owner:*
4. **D102 lane-change display** (listed 2026-10-02, phase 3 since 2026-10-05).
   `data/projects/lane-change-lab.traffic.json`, Lane drop scene. Does a changer ease across and
   hold when stopped? *Owner:*
5. **D90 cooperative braking** (listed 2026-10-01). M2.6, East approach. Do lane-2/3
   left-turners slip into lane 1 with the vehicle behind slowing, not stopping dead? *Owner:*
6. **D93 downstream decision** (listed 2026-10-01). Copy of four-leg; a decision on the West
   pocket Link (East 3, North 1), routeless West input. Do pocket vehicles change toward their
   turn with no LANE_UNSERVED? (CLI on Windows: 118 : 41 : 0.) *Owner:*

## D — Right-of-way on the drawing

1. **D86/D72 conflict areas follow the drawing** (listed 2026-09-30).
   `t-junction-priority.traffic.json`, tool `A`. Drag Major eastbound along and across: do the
   area and its line follow, status stay "runs", no grey copy? Drag it clear: do area, Yield and
   line go, and Ctrl+Z restore them? Needing an area wider than the overlap is the failure. *Owner:*
2. **D109/D110 conflict controls, Bézier coverage** (listed 2026-10-05). Curved Connector after an
   interior drag; a 3 × 3 crossing: nine lane-pair band pairs, one row, group edit/Delete, Undo;
   two separated intersections stay two rows. *Owner:*
3. **D114 edited Connector lane centres** (listed 2026-10-06). Curved 3 → 3 Connector and both
   taper sides; drag an interior point, rerun. Do vehicles, heads, waiting bars and coverage follow
   the edited lanes? Zero points, Reset straight, widths, save/reopen, Undo/Redo. *Owner:*
4. **D115/D117/D118 overlap display** (listed 2026-10-06). Oblique crossing, a Connector landing
   mid-Link, two Connectors leaving one lane, both traffic sides, obtuse/curved mouths, a taper.
   Do bands follow each lane and leave rails readable? Do Crossing/Merge edit together and
   Branching stay read-only; separated sites stay separate? *Owner:*

## E — Drawing and gestures (100/150/200 %)

1. **D83/D84 controls and gestures** (listed 2026-09-29). Chevrons, stacked spin chevrons, Thai
   and English on one baseline, 24 px fields. Plain left click never authors; Ctrl+right draws.
   Is Ctrl+right too slow for heavy route/counter work (D84's failure)? *Owner:*
2. **Canvas grid** (listed 2026-10-03). At 125 %, too faint? Fix colour, not width. *Owner:*
3. **Interaction cleanup, PRs #73–#75** (listed 2026-09-29). Curved and overlapping roads, tabs
   at working zoom. Anything wrong by eye? *Owner:*
4. **D80 central axis and four-point mouths** (listed 2026-09-28). Read
   `reference/CONNECTOR_FOUR_POINT_MOUTH.md`. Both ends at 45/90/120/150/170/179°, 1–3 lanes,
   kerb/median lanes added or dropped; grips, save/reopen, Undo. Fixed lane-index pairing holds?
   *Owner:*
5. **D96 route overlay** (listed 2026-10-02). Route 2-lane Link → 1-lane Connector → 2-lane Link,
   end-mounted and mid-body. Both lanes of both Links shown, nothing upstream of a mid-body
   arrival? *Owner:*
6. **D100 Ctrl+A wireframe** (listed 2026-10-02). Is the line readable on the grid? Wanting
   vehicles on the line is the failure. *Owner:*
7. **M1.19/M1.20 Connectors** (listed 2026-09-24 or earlier). Any Connector deleted by a Link drag
   unexpectedly? Is half a lane width the right "off the Link" distance? Lanes meet middle on
   middle? *Owner:*
8. **D116 road crossbars and Route trace** (listed 2026-10-06). Curved/multi-lane roads,
   Connector-mounted heads; trace a branch, click the destination, Undo/Redo, Backspace, cancel,
   save/reopen. Signal Run bars stay at the stop position and change colour; Reset restores. *Owner:*
9. **D119 positioned routing** (listed 2026-10-06). Click a mid-Link source, hover to an exit, add
   a second Route from the line, edit weights/time/type rules, run routeless traffic. Recognition
   at the line, Undo/Redo, line drag, save/reopen, legacy files. Is the one-timestep recognition
   boundary acceptable for a study? *Owner:*

## F — Dialogs: demand and behaviour

1. **D142 volume from turning counts** (listed 2026-10-09). Type a 15-minute count sheet into a
   decision, tick the input's checkbox; total and rows right; edit a count and Undo; save/reopen;
   run. *Owner:*
2. **Demand slices 1–6** (listed 2026-10-05). Catalog periods and type overrides in the desktop;
   the stacked PRs reviewed in order (#106, #107, then time/type). *Owner:*
3. **D128 behaviour library** (listed 2026-10-07). Capture the catalogs, duplicate a behaviour,
   heavy-vehicle class, an urban type with an override on several roads, effective list,
   Undo/Redo, delete with replacement, Thai, save/reopen, run; also a `w74` run. *Owner:*
