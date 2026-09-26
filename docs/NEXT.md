# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the history and the
decision log; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it.

---

## Immediate — M3.2.8b, and the owner's M3.2.7d

**Done:**
- **M3.2.8a, commitment (D69, `docs/M3_8_CONTRACT.md` §1).** A driver who cannot stop at its line
  at `maxDeceleration` goes. The T-junction's right-of-way clamps are gone. No major vehicle is
  clamped at an area. The four-leg report is unchanged, and M2.6 moved by at most 0.13 s per
  movement (`docs/evidence/m3.2.8a-commitment.md`). The owner's first choice,
  `comfortableDeceleration`, clamped the major road and was replaced: **do not retry it without a
  new measurement.**
- Conflict areas are automatic (M3.2.4c, D68); any new right-of-way work must keep them derived,
  never stored.
- M3.2.2a–M3.2.7c (D54–D67); everything is in `docs/M3_ACCEPTANCE.md` and `docs/evidence/`.

**M3.2.7d is the owner's, not a session's.** Carry out the exercise with the recording sheet in
`docs/M3_ACCEPTANCE.md` §3, on Windows. Until the owner reports it, the row stays pending.

**Next session: M3.2.8b** (ROADMAP row): lane changing, cooperation and visibility. It is a new
system, so write its contract first, as `docs/M3_8_CONTRACT.md` §2 (the placeholder is there).
1. **Start from the known limit below.** A Thai left turn at all times queues behind through
   traffic in a shared kerb lane, because a vehicle keeps the lane it entered on. The smallest
   useful piece is a mandatory lane change toward the lane a route's next Connector leaves from.
   Measure it on the M2.6 template: its left turns, and whether the through movements' delay
   moves.
2. Then cooperation (a vehicle opening a gap for a merging one) and visibility at areas.
3. The contract must say how a lane change keeps replay exact (no new RNG draw without a seeded
   contract) and what a vehicle between two lanes occupies, for zones and car-following alike.

**Open item from M3.2.8a:** five minor vehicles standing or at walking pace within 1 m of the
T-junction's merge line are still clamped in the congested headway arm (seeds 42 and 43). They
are not the commitment case, since their stopping distance is about zero, and they are not
diagnosed. Diagnose them before claiming the minor road clamp-free.

M3.1 supplied merge arbitration only — a deterministic gap-time/headway threshold, not a
calibrated critical-gap model. Derived stop lines sit 1 m short of the join (D50). M3's
done-condition (minor-road delay responds to gap time) is shown on development evidence only;
no gate result is inferred.

**Known limit, surfaced by the M2.6 template:** a Thai left turn at all times still queues behind
through traffic in a shared kerb lane, because there is no lane changing until M3.2.8b.

## Engineering work that can proceed without the owner, if asked

- Derive an input's interval volumes from its entry decision's turning counts, so a count sheet
  is typed once; today the two are entered separately.
- In-editor CSV export of the Results tab (the CLI has one).
- Results-tab refresh that skips work while hidden — cheap today (16 rows), so measure first.
- Per-lane shares (D32): no canvas gesture sets one, and the input table row
  (`refreshDemand`, `src/shell/editor_demand.cpp`) shows the equal-split figure even when shares
  are set.
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
