# PROGRESS archive — 2026-09-18, M1.18 (the flush mouth)

Moved whole out of `docs/PROGRESS.md` when it passed the 500-line guard. Superseded in part by
M1.19, whose entry is in `docs/PROGRESS.md`.

---

## 2026-09-18 — The mouth meets the Link again, by sliding along the ribbon instead of cutting across it (M1.18)

**The owner's requirement, after seeing the square mouth in the editor:** every lane of a Connector
must be backed by a lane of the Link, and must *meet* it. A square end does not — it stood 4.7 cm
to 1.8 m clear at an oblique arrival, with the polygon overlapping the carriageway across that step.

**This is not M1.17 reinstated; its revert stands.** M1.17 moved the end vertices **laterally**,
onto the Link's lane edges. That re-aimed each boundary's last leg, let neighbouring boundaries
cross, and folded the mouth to a point. M1.18 moves them **longitudinally** instead: each boundary
slides along **its own offset curve** by `s = d·(c·u)/(c·n)`, decaying to zero over a transition
zone. Because no vertex changes its lateral offset, the boundaries keep their order and **cannot
cross each other** — the fold is structurally absent, not tested for and worked around. Measured
over the 288-case sweep of Link heading × arrival angle: **0 folds, 0 ring self-crossings.**

**The trade the owner chose, in numbers.** The mouth is *flush but wider*: each lane keeps its full
width square to the Connector, so the mouth spreads along the Link's cross-section by `|d|/|c·n|`
and its lane edges land outside the Link's. Worst mouth span for a 7 m ribbon, by arrival angle off
the Link's axis:

| arrival | worst mouth span | | arrival | worst mouth span |
|---|---|---|---|---|
| 0-15° | 7.60 m | | 45-60° | 16.64 m |
| 15-30° | 8.79 m | | 60-90° | 28.86 m (the clamp) |
| 30-45° | 11.19 m | | | |

`kMouthShiftLimit = 4` bounds it, exactly as `kMiterLimit = 4` bounds a corner that would spike to
infinity — same form, same reason. **If 28.9 m looks wrong in the editor, that one constant is the
dial**; the geometry below it is unchanged.

**Falling short is reported, not hidden.** `connectorMouthFit` returns each end's shift, transition
zone and **residual in metres** — how far the mouth still stands off its Link. The two ends share
the spine rather than taking half each, so an ordinary end beside a steep one still aligns exactly.
Residual is **0.00** on the reverse curve, gentle reverse, quarter turn, u-turn, and on a moved Link
at 30° and 60°; it is **1.75 m** at 90°, where the Connector arrives straight across the lane and a
cut on the cross-section lies along the ribbon itself. Worst over the whole sweep: 4.32 m.

**What did not change.** The body: a 3.5 m lane is still 3.5 m square to the road at every interior
point. A parallel arrival has `c·u = 0`, so `s = 0` and the ribbon is left **bit for bit** as the
plain offset drew it — which is most of every network, and why `lane_edge_tests.cpp`'s collinear
fixture is untouched. A lane tapered to nothing still closes **exactly** on its neighbour: the two
share an end point but not a curve, so sliding each by the same distance parted them by 5.4 mm until
`shearMouth` re-closed them. `trafficsim-cli 42` is unchanged at `meanDelay 29.249359418430977` —
`connectorBoundaries` is presentational and hit-test only, and that was verified, not assumed.

**Tests moved rather than loosened, and the measure changed for a reason.** `mouthSquareness` is
gone: asserting it zero *was* asserting a square cut, the very thing the owner asked to stop. The
new `tests/connector_mouth_tests.cpp` (a new `mouths` ctest group) asserts `mouthFlush` — distance
from each boundary end to the Link's own cross-section — at **1e-9**, plus width at 1e-9, the
fold-free ring, the bit-for-bit parallel case, a Connector too short for its arrival, and the
near-parallel clamp. Deleting the two `shearMouth` calls fails the suite with
`Numeric mismatch: 0.254907 vs 0.000000`.

**Where a bound replaced an equality, that is honest rather than convenient.** A mouth slide leaves
a lane's two edges at different stations, so on a curved or width-changing ribbon no index-for-index
measure of width is exact — on the 2-into-2 90° turn, readings of 2.87 m, 2.95 m and 3.01 m for the
same 3 m lane, depending on the measure. Those fixtures now bound the width and say so; the exact
assertions live on fixtures where an exact measure exists.

**Verification:** 24/24 CTest on Qt 6.4.2 under `xvfb`, file sizes green (`connector_shape_tests.cpp`
split at 483 lines, `ROADMAP.md` trimmed to 485 with M1.12.2's closed body archived).
*Linux only — no desktop verification claimed; the editor has not been driven by hand.*

### Next (superseded — M1.12.3 is closed by M1.19 above)

**M1.12.3 — the Link wins at the mouth for widths**, filed in `ROADMAP.md` with its done-condition.
An authored `Connector::laneWidths` still replaces the Link's width at both ends
(`road_boundaries.cpp:116-117`), so a Connector whose author typed a width does not match the lanes
it attaches to. Give `ConnectorLaneWidths` a third `body` vector, hold `source`/`target` at the
Link's width, and taper between them. **Not** done in this session on purpose: M1.18 moved where a
mouth sits, M1.12.3 moves how wide it is, and together a failing width test and a failing mouth test
are indistinguishable.

Then, still the only thing that closes M1: the owner's timed exercise in `docs/M1_ACCEPTANCE.md`.
