# PROGRESS archive — 2026-09-16

Entries moved whole from [PROGRESS.md](PROGRESS.md) to keep the active log below 500 lines.
Newest first, as in the active log.

### 2026-09-16 — The cross-section follows the road, not the line between the mouths

A review pass over the change merged an hour earlier found a regression in it, measured and fixed
here. No stored coordinate is involved either way.

**What was wrong.** `connectorBoundaries` took the direction across the road as a straight
interpolation between the two mouths' cross-sections. That says nothing about where the Connector
actually points: on a reverse curve the mouths are parallel, so the cross-section never turned
while the path swung 50-60 degrees away from it, and each lane was drawn its own width times the
cosine of that angle. Measured square to the road, a 3.50 m lane on a tight S (10 m of gap, 12 m of
offset) came out **1.06 m** at its narrowest — against **2.895 m** from the code before the rewrite,
so the rewrite made this shape worse, and a reverse curve is one of the two shapes the owner
reported. Symmetric shapes were unaffected: a quarter turn measured 3.492 m, a U-turn 3.500 m,
because there the interpolation happens to track the arc.

**Why the tests passed.** The width assertions measured the distance **along** the cross-section,
which is the lane width by construction at any angle the cross-section happens to sit at. They
could not fail. They now assert the distance square to the road as well, and a new case walks a
reverse curve, a gentle reverse, a quarter turn and a U-turn, each pinned above what the old
construction drew for it.

**The fix.** The widths now hang on the anchor path's own normal, corrected onto each mouth by two
end offsets, the second unwrapped against the first. On the same S that gives **3.341 m**, on the
whole measured grid it beats both the rewrite and the code before it, the quarter turn and U-turn
do not move, and both mouths still land on their link's lane edges to 4e-16. Taking the second
correction on its own instead — the obvious way to write it — picks the opposite way round the
circle and folds the ribbon flat; that is a negative check now, not a comment.

Two guards written with it turned out to be provably no-ops and were removed rather than left
unexercised: flipping the normal to match lane order, and unwrapping it sample by sample. Both are
angles modulo a full turn, so neither can change a cosine; verified by comparing geometry on a
270-degree loop ramp, identical to the last bit. What remains is the smallest form whose every line
fails a test when removed.

Also corrected: `trimSelfIntersections` earns its place on **Link** edges (a quarter turn of radius
3 m crosses once), not on Connector markings — no Connector boundary self-crosses any more,
including a three-lane U-turn at minimum radius, because they are no longer offsets of a path. The
call there stays for hand-dragged shapes; the previous entry's wording overstated it.

**Verification:** 23/23 CTest plus the architecture and file-size guards on Linux; Windows is
`native.yml`. Both remaining pieces of the construction were reverted separately and the new test
failed each time. No baseline fixture was regenerated; none could move.

---

### 2026-09-16 — A Connector carries lanes, not a ribbon that shrinks

Two more owner findings on the same screenshot, both about drawn geometry. Neither touches a
stored coordinate, so no existing project or pinned baseline can move.

**Every lane narrowed where one should have tapered.** `connectorBoundaries` interpolated every
boundary between the two end cross-sections, so a two-into-one shrank as a whole: measured point
by point, the lane that continues was 3.500 m at the source, 2.622 m half way along and 1.750 m
at the mouth. Vehicles drove a lane that pinched, and the interior boundary landed on the centre
of the single target lane, which is why a divider ran down the middle of it.

The cross-section is now assembled from the lane widths the Connector actually carries. A path
whose source or target lane repeats its neighbour's is the surplus one, its width at that end is
zero, and it closes as a wedge; every other lane holds the width its links give it, interpolated
only between its own two ends. The widths hang on the last path that is a real lane at both ends
and step out in both directions, so adding a lane at the leading edge still cannot move the far
edge — the property `endEdge` used to provide by pinning. On the same two-into-one the continuing
lane now measures exactly 3.000 m to 3.500 m along its whole length (the fixture's own lane
widths), the wedge falls 4.000 m to 0, and both mouths still land on their link's lane edges.
The divider is a lane edge for its full length, so it arrives on the edge of the merged lane.
Rendered: the through lane runs straight through and the extra lane closes onto it, the way
Vissim draws a lane drop; a one-into-two opens the mirror image.

**A reverse curve was read as no turn at all.** The control reach came from the angle between the
two tangents. For an S they are parallel, so the reach fell back to `chord/3` while each end was
still leaving the chord at 50°: a measured S bent to 3.12 m on a 17.2 m chord, 0.18 of it. The
reach is now the circular-arc value for **each end's** angle to the chord,
`(2/3)·chord·tan(α/2)/sin(α)`, which is the same number to the last bit for a straight run and
for a symmetric turn (the U-turn still measures 0.454 of its chord) and gives 3.75 m, 0.22, on
that S. Reverting it fails both curve tests.

**Offsets that loop.** A lane edge offset round a bend tighter than the offset crosses itself;
measured on a quarter turn of radius 3 m between two straights, the inner edge crossed once and
filled as a notch. `trimSelfIntersections` cuts the loop out at the crossing point and is applied
to drawn lines only — link boundaries in the editor and the diagnostic view, and Connector
markings — so `connectorBoundaries` keeps the vertex-for-vertex correspondence the model relies on.

**Verification:** 23/23 CTest plus the architecture and file-size guards on Linux; Windows is
`native.yml`. Each of the three changes was reverted on its own and the matching test failed.
No baseline fixture was regenerated; none could move, because only derived drawing geometry and
newly created curves changed.

---
