# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the history and the
decision log; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it.

---

## Immediate — verify M3.2.8b on Linux/Qt, then M3.2.8c; the owner's M3.2.7d

**Done:**
- **M3.2.8b, mandatory lane changing with minimal cooperation (D71, `docs/M3_8_CONTRACT.md` §2,
  A27–A35).** Volume enters on every lane of the entry Link. A lane that cannot reach the end is
  a stub route that changes before its dead end. The nearest target-lane vehicle that can stop
  comfortably holds back for one waiting there. M2.6: all drained, mean delay 50.14 → 50.16 s,
  left turns down on all four approaches, right turns up on all four
  (`docs/evidence/m3.2.8b-mandatory.md`). **Without cooperation it was 61.6 s with a 404 s wait:
  do not remove the courtesy without a new measurement.**
- **M3.2.8a, commitment (D69, contract §1).** The owner's first choice, `comfortableDeceleration`,
  clamped the major road and was replaced by `maxDeceleration`: do not retry it without a new
  measurement.
- Conflict areas are automatic (M3.2.4c, D68), derived and never stored. M3.2.2a–M3.2.7c are
  D54–D67; the evidence is in `docs/M3_ACCEPTANCE.md` and `docs/evidence/`.

**First, before anything new (M3.2.8b was verified on Windows headless only):**
1. `cmake --preset desktop && cmake --build --preset desktop && ctest --preset desktop` on Linux.
   `src/shell/editor_demand.cpp` (input row: `routeLaneFamily(...).size()`) and
   `src/shell/editor_input.cpp` (share count: `routeLaneShareCount`) were edited but **never
   compiled**. Fix anything red before other work.
2. Re-run `trafficsim-cli 42 --project data/projects/m2.6-study-template.traffic.json` on
   Linux/GCC and compare with the evidence tables. A difference in the last digits is the
   toolchain. Record it in the evidence file either way.
3. Callgrind the M2.6 one-hour run (baseline 4.35G instructions, D70) and record the cost of
   `decideLaneChanges`, `courtesyHolds` and the span rebuild on a tick with a change.

**Then M3.2.8c** (ROADMAP row). Write its rows in `docs/M3_ACCEPTANCE.md` and its contract as
`M3_8_CONTRACT.md` §3 before the code. It is several systems, so pick one per session. In the
order the evidence argues for:
1. **Right turns rose on all four M2.6 approaches** (up to +13 s East). A right-turning vehicle
   entering on the kerb lane must now cross into the pocket lane. Measure first, before changing
   anything: dead-end waits per movement, and where on the Link the changes happen. Then decide
   whether `laneChangeDistance` (Vissim's look-ahead for a mandatory change) is the next piece.
2. Cooperation with a deceleration parameter and look-ahead, so a vehicle that is not yet
   stopped is helped too (today only a waiting vehicle is).
3. Lane changes after the entry Link: free-walk paths and placed decisions are still lane-fixed
   (SIMULATION.md, "No lane changing downstream").
4. Discretionary changes, visibility at areas, and a between-lanes state. Today a change is
   instantaneous, which the contract records as a limit.

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

- Derive an input's interval volumes from its entry decision's turning counts, so a count sheet
  is typed once; today the two are entered separately.
- In-editor CSV export of the Results tab (the CLI has one).
- Results-tab refresh that skips work while hidden — cheap today (16 rows), so measure first.
- Per-lane shares (D32; one weight per entry-Link lane since D71): no canvas gesture sets one,
  and the input table row (`refreshDemand`, `src/shell/editor_demand.cpp`) shows the equal-split
  figure even when shares are set.
- Keyboard-only equivalents of the route and vehicle-input gestures (M1.25/M1.26) are still open.
- The entry-acceleration bias in movement delay needs travel-time sections (M5), not a
  correction factor.

## Do not retry without a new measurement

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
- **`ctest -j` wall time is `m26study` (≈17 s Debug)**: split that group before chasing any other
  test, and add every new test group to `TRAFFICSIM_TEST_GROUPS` or `all-model-tests` fails (D70).

## Standing — the owner's items

1. **Record the M0 plausibility observation** — acceleration, queue at red, discharge at green.
   Owner observation, not calibration or M6 validation; the not-yet-validated marker stays.
2. **Decide the name (Q5).** D11's trigger ("end of M1") is live. `Velk` is the strongest recorded
   candidate; `Headway`, `MicroFlow Simulator` and `Veytrix` were rejected (D11 row) — do not
   re-derive them. The owner's decision, not a session's.
3. **Drive M1.19/M1.20 in the desktop editor** — measured at model and command level only. Watch
   for a Connector deleted by a Link drag the author did not expect, and whether half a lane width
   is the right "off the Link" distance (`laneContains`). When checking a mouth by eye, every
   Connector lane should meet the Link lane it feeds, middle on middle; past about 60° it cannot
   (`kMouthSpanFloor` in `road_boundaries.cpp`; `connectorMouthFit` reports the shortfall).
4. **The M2.6 template** (`data/projects/m2.6-study-template.traffic.json`) has placeholder
   volumes, a guessed timing-window-to-approach mapping and no aerial image; replace them before
   using it for a real study.
5. **Open questions, none blocking:** motorcycles (not shipped; lane sharing is unmodelled and Thai
   counts are motorcycle-heavy); a routing decision's station along its Link (M2.1).

M1.22, M1.23 and M2.1 remain open milestones; `docs/ROADMAP.md` is the authority on each.
