## 2026-10-05 — Bézier conflict coverage and connected editor groups (D109)

Owner requested runtime-equation coverage with painted highlights and one group for 3 × 3.
Crossing strips now refine the mapped cubic and its width rails independently of drawing
vertices. Broad-phase bounds use those rails; shared-lane exclusions apply only at the same
attachment mouth, not to distinct stations or later separated intersections. Waiting lines
retain physical setbacks under drawing edits. Grouping requires the same owner pair/kind
and connected surfaces on both sides; two locations remain separate. Table/canvas selection
and bulk commands share the group; nine runtime lane-pair reservations remain nine.
Schema/core/frozen baselines are unchanged. The editable T-junction example is rebuilt for
new extents. Linux model and Qt verification is recorded in the PR; Windows/owner gates
remain open. Existing saved coverage is validated, never silently migrated or widened.

---

