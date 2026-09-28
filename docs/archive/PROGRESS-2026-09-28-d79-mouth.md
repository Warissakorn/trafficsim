# Archived D79 mouth implementation

## 2026-09-28 — M3.2.9h: one P1–P4 construction at every angle (D79)

- The owner asked for P1–P4 without a separate case past 90°. The mouth used to try both
  pairings of Connector and Link edges and keep the shorter, which switched Link edges past 90°.
  Now the first rail's edge line always meets the range's first Link boundary (index order), and
  the shoulder keeps growing as W/2·tan(θ/2) (4 m lane: 2.00 m at 90°, 4.83 m at 135°, 7.46 m at
  150°). Past about 152° the existing reach limit (4× width) hands that end to the legacy cap.
  Offered the alternative (same edge side geometrically, today's picture); the owner chose index
  order.
- Which rail is on which side stays read off the rails. Deriving it from the driving side was
  tried and reverted: legacy strips at an obtuse end run lane 0 on the driver's left, so the cut
  crossed the rails and whole surfaces fell back (1,512-case sweep: 2,150 mouths, 367 Connectors
  with neither end, against 2,928 / 27 before).
- New in the rail/divider step: the stretch of a boundary that runs past its cut along the end
  direction is pulled back onto the cut before the bend. Past 90° a strip can run across the Link
  beyond its cut and the bend alone hooked it (a divider ended 1.83 m off its point; mutation
  check: removing the pull-back fails `mouth_sweep` again).
- Sweep now: 2,847 mouths, 7 Connectors with neither end (lost mouths: target 150°, where the
  reach limit is expected, and folded-strip cases at 30°, NEXT's known issue).
- Tests: `mouths.four_point_mouth_keeps_one_construction_beyond_90_degrees` (91–150° both sides,
  170° falls back) and `…sweep_follows_one_formula_at_every_angle` (5–355°, 60 placed).
- Windows desktop: 67/67. Linux not run.

---

