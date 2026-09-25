# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the history and the
decision log; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it.

---

## Immediate — M3.2.2

M1 usability is accepted by owner ruling (D49) and M2's gate passed by the owner's judgment (D53,
[`M2_GATE.md`](M2_GATE.md)). **Start M3.2.2:** authored references/model, codec, commands and the
effective-priority resolver, in that order, with A01–A08 evidence — the contract and evidence
design are in [M3_PLAN.md](M3_PLAN.md), [M3_CONTRACT.md](M3_CONTRACT.md) and
[M3_ACCEPTANCE.md](M3_ACCEPTANCE.md) (D41). Keep the new controls Run-blocked until M3.2.3
implements their runtime.

M3.1 supplied merge arbitration only — a deterministic gap-time/headway threshold, not a
calibrated critical-gap model; derived stop lines sit 1 m short of the join (D50). Conflict areas
as editable input, authorable priority rules, stop/yield control and crossing conflicts are
M3's; its done-condition (minor-road delay responds to gap time) is not met.

**Known limit, surfaced by the M2.6 template:** a Thai left turn at all times still queues behind
through traffic in a shared kerb lane, because there is no lane changing until M3.2.8.

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
- **Not booked:** `compileDocument` (paid once per Run, not per tick) and the `push_back` work
  left in `occupiedSpans`. The per-tick fleet sort is already a merge.
- **Measure the engine with callgrind, not the clock,** when resolving a few percent: wall time
  swung ±7% on 2026-09-23. The editor's timings are steadier (±2%). 2026-09-25 baseline, Release
  headless: the M2.6 template's one-hour run takes 0.78 s (0.77–0.86 s, five runs).

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
