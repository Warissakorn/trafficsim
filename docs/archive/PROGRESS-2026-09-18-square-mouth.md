# PROGRESS archive — 2026-09-18, the square-mouth revert (M1.17 reverted)

Moved whole out of [`PROGRESS.md`](../PROGRESS.md) on 2026-09-21, when a new entry pushed the live
file past the 500-line guard (hard rule 6). Nothing here was edited.

**Why this one:** it is the oldest live entry, and it is also the one the 2026-09-21 Connector
parity audit corrects. Its description of what `connectorBoundaries` draws has been false since
M1.18. The correction of record is in `PROGRESS.md` under *2026-09-21 — Connector parity audit*;
[`../CONNECTOR_PARITY_AUDIT.md`](../CONNECTOR_PARITY_AUDIT.md) §3.1 has the detail.

---

## 2026-09-18 — The Connector mouth is a plain square end again (M1.17 reverted)

**The owner asked for the first Connector geometry back:** the end of a Connector simply meets the
Link, with no realignment towards the Link's direction and a plain square end. Done, on
instruction — this is a deliberate revert of M1.17, not a defect fix.

`connectorBoundaries` now stops at `offsetGeometry`. The whole end-cut block is gone: the
fixed-distance cut onto the Link's cross-section, the bounded re-miter that stood in where that cut
folded, the fold test that chose between them, and the `legCrossing`/`remiter` helpers. `endCross`
stays — the source end's cross-section is still what fixes which way lane order runs, and nothing
else reads it now.

**What is unchanged:** the body. The cross-section is not interpolated between the two mouths, so a
3.5 m lane is 3.5 m at every interior point (interpolation drew 1.06 m on a reverse curve, 0.46 m at
a 90-degree arrival). Only the two end samples move.

**The step is the accepted shape now, not a bug.** A square end stands clear of the Link's lane
edges at an oblique arrival: 4.7-17.2 cm on a gentle join, 0.12-0.29 m where a Link has been
rotated under the curve, up to 0.88 m on a hard reverse curve, and up to ~1.8 m where the Connector
arrives square across the lane. The polygon overlaps the carriageway across that step.

**Tests were rewritten, not deleted.** The three that asserted the wedge now assert the square end:
`mouthLine` still checks the mouth is one straight line, a new `mouthSquareness` checks that line is
square to the boundaries' own end legs (exact where widths are constant, 1e-2 where a lane tapers,
because a tapering edge leans against the ribbon by construction), and `stepTo` bounds the step to
the Link's lane edge instead of demanding it be zero. 127 tests pass, 16/16 ctest.

### Next

The engineering side of M1 is unchanged by this: **the only open item is the owner's timed
four-leg / aerial-image / reopen exercise in `docs/M1_ACCEPTANCE.md`.** If the square mouth looks
wrong once seen in the editor, the wedge is in Git history at `55294fa` and its reasoning is in
[`archive/PROGRESS-2026-09-17-mouth.md`](PROGRESS-2026-09-17-mouth.md) — do not re-derive it.
*(It did look wrong; M1.18 above replaced it with a longitudinal slide, not the wedge.)*

*Verified on Linux, headless preset only (Qt not installed here); no desktop verification claimed.*