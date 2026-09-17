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

---

## 2026-09-16 — The ends are square to the Connector, as Vissim draws them

The owner circled the joints in the render from the entry below and sent a Vissim screenshot beside
them: a constant-width ribbon whose ends are cut square to itself and simply overlap the link.

The joints were still being cut on the **links'** cross-sections. Where a Connector leaves or
arrives across a lane rather than along it -- which is exactly the state a moved Link leaves behind
-- that cut is nearly parallel to the road, so the last sample stretched into a slanted wedge. It is
the same mistake as the one below, surviving at the two end samples after being removed from the
body.

Both ends now take the same mitered offset as every other sample, so a Connector is one constant
width from end to end. Measured with a Link rotated 30/60/90 degrees under a drawn Connector: 3.500
m at **every** sample, joint included, against 1.96 m and 0.46 m before. Reverse curves, U-turns,
tapers and the merge/diverge wedges are unchanged or better. What replaces the wedge is an overlap
at the joint: nothing at all on a straight connection, and 0.12-0.29 m where the sampled curve
leaves its lane at an angle, because the square cut is square to the polyline the author actually
has. Vissim overlaps there too.

The tests now pin the rule rather than the old symptom: every boundary end has no component along
the Connector's own end direction (exact, to 1e-9), and lands within a joint's reach of the link's
lane edge. Reverting to the link-cut ends fails three of them.

**Verification:** 23/23 CTest plus the architecture and file-size guards on Linux; Windows is
`native.yml`. Two reverts each failed tests: cutting the ends on the links, and dropping the miter.
No stored geometry changes, so no baseline fixture could move and none was regenerated.

---

## 2026-09-16 — One poly point moves, and the offset is constant along the road

The owner traced a real interchange over an aerial image, moved a Link, and the Connectors came out
deformed. Two rules settled it, both theirs.

**"Vissim moves only the one poly point that is attached to the Link."** `reanchorConnector` carried
the whole curve through a similarity transform of its endpoint chord, so every Link edit dragged
points the author had placed by hand. It now sets the attached endpoint and leaves the rest alone.
The path independence the transform existed for is stronger this way, not weaker: the point returns
to where the lane puts it, and nothing else was ever touched. The test that pinned the old rule now
pins this one — interior points identical to 1e-12, endpoint on the lane to 1e-12.

**"Should a Connector and a Link both keep the same offset from the lane centreline all along?"**
Yes. A road's polygon is its axis offset by half its width, square to the axis at every point.
Links already did this: measured 3.500 m of a 3.500 m lane at every bend from 30 to 170 degrees,
because `offsetGeometry` miters each corner. Connectors did not — they stacked widths on a
cross-section that was only correct at the two mouths. Measured on the owner's case, a Connector
whose target Link had been rotated 90 degrees drew its 3.50 m lane at **0.46 m**.

Connector boundaries now go through `offsetGeometry` itself, via a new overload that takes an offset
per point so a tapering lane leaves its neighbours at full width; one implementation of "how a road
edge is offset" serves Links and Connectors alike. After the same 90-degree rotation the body holds
**3.500 m** at every sample, and only the last two — the joint, where the Connector arrives across
the lane and is cut on that lane's cross-section — are shorter through the corner, which is the
notch Vissim shows there too. Reverse curves came out 3.476-3.499 m against a 3.5 m nominal, better
than either earlier version. The mouths still land on their links' lane edges exactly.

**Verification:** 23/23 CTest plus the architecture and file-size guards on Linux; Windows is
`native.yml`. Three separate reverts each failed a test: interpolating the cross-section between the
mouths, dropping the miter, and dragging the whole curve on reanchor. Stored geometry only changes
where a Link edit moves an attachment, which is the edit itself; no baseline fixture was regenerated.

---

---

## 2026-09-16 — Intermediate points, and what a Vissim Connector's line actually is

The owner sent Vissim's Connector dialog: it counts **intermediate points**, and we had no such
field — we stored a 13-point sample of a cubic, so every sample was a grip and dragging one put a
corner in a shape the author had no count over.

**A first pass read the line as a spline. It is not.** The owner settled it by sending a Vissim
connector with the count set to **2**: four dots, three straight legs, a mitered corner on each
dot, a visible step at each mouth where the polygon overlaps the link, and no tangency to the
links anywhere. A Connector is drawn by the same rule a Link is. The spline is reverted; what
survives from that pass is the model it needed — a Connector stores its two attachments and its
intermediate points, and nothing baked — and the field.

**The field.** Properties → Connectors carries `Intermediate points`. It never re-derives the
default curve; `Reset curve` is the one thing that does. Raising it splits the longest leg each
time, so every point the author placed survives and the drawn line does not move at all —
re-laying at even spacing instead cut a hand-placed corner by 1.00 m, measured, which is why it
does not. Lowering it spaces the points evenly along the shape that is there, giving up only the
corners the lower count cannot hold; a 3 → 7 → 3 round trip leaves every point on the author's own
line, where a reset is 5.58 m away from it. A new Connector gets 3, the owner's number; 0 is legal
and leaves one straight leg. Laying more points along the arc follows the turn more closely: 2.29 m
of sag at one point, 0.60 m at three, under 0.10 m at fifteen.

**The curve that left its junction.** The arc reach laying those points,
`(2/3)·chord·tan(α/2)/sin(α)`, is 0.67 of the chord at a right angle and **11.05** at 160 degrees.
Drawn where two links nearly touch — the owner's second picture — it ran to 11.0 times its own
chord, which is how a 3.5 m ribbon ends up a crumpled wedge. Held at the 120-degree value,
`(4/3)·chord`, it measures 1.9, and the ordinary fixtures came out byte-identical. It is still an
undrivable turn for a 3.5 m lane and still reports `TIGHT_CONNECTOR_RADIUS`.

**What three points cost against thirteen.** A coarser polygon reads slightly wide along its
cross-section at a corner, because that is what a miter does — 6.3 cm, the same thing a Link's own
edges do at a bend, and square to the road it is still the lane width. The step at a mouth grows
with it: 4.7 to 17.2 cm on a gentle join, 0.88 m on a tight reverse curve. Both are recorded in
the tests with their measured numbers rather than tuned away.

**Old save files were deliberately not migrated.** The owner confirmed the project is still a test
bed and no drawing is being carried forward. No baseline fixture stores connector geometry, so
none could move and none was regenerated.

**Verification:** 23/23 CTest, the model suite up from 97 to 101 cases, plus the architecture and
file-size guards on Linux; Windows is `native.yml`. Three negative checks each failed a named test:
re-laying evenly when the count is raised, ignoring the count and always laying three, and letting
the arc reach run away again. A centripetal parameterization was tried while the spline reading
still stood and **removed**: it moved the measured numbers by 0.3 degrees and 3 mm.

---
