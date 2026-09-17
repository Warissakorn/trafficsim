# VISSIM_PARITY archive — 2026-09-16

Follow-ups moved whole from [VISSIM_PARITY.md](../VISSIM_PARITY.md) to keep it below 500 lines.

## 2026-09-16 second follow-up — A Connector carries lanes, not a ribbon

**Every lane narrowed instead of one tapering.** On a two-into-one the whole ribbon shrank
together: the lane that continues measured 2.62 m half way along and 1.75 m at the mouth, so
vehicles drove a lane that pinched. Vissim keeps the through lane at its own width and drops the
surplus one as a taper. Ours does now: the cross-section is assembled from the lane widths at
each end rather than interpolated between the two mouths, so the continuing lane holds the width
its links give it point for point, and the extra lane closes onto it as a wedge. The divider
between them is a lane edge for its whole length, which is why it now arrives on the *edge* of
the lane the two merge into instead of part way down its middle — the marking trim from the
previous round is no longer what keeps it off the traffic.

Vissim requires a connector's two ends to carry the same number of lanes and leaves the taper to
a separate lane drop. We allow the unequal range and draw the taper ourselves; the lane pairing
follows the ranges in lane order, so re-anchoring a range moves the taper to the other side.

**A reverse curve was read as no turn at all.** The control reach came from the angle between the
two tangents, which is zero for an S even when each end leaves the chord steeply; a measured S
bent to 0.18 of its chord. Reading each end against the chord instead gives 0.22 there and is
identical, to the last bit, for straight runs and symmetric turns.

## 2026-09-16 third follow-up — The cross-section follows the road

Review of the merged change found a regression it had introduced. The cross-section the lane widths
are measured across was interpolated between the two mouths, which says nothing about where the
Connector points in between: on a reverse curve the mouths are parallel, so it never turned while
the path swung 50-60 degrees away, and the lane was drawn its own width times the cosine of that
angle. Measured square to the road, a 3.50 m lane came out 1.06 m at its narrowest on a tight S —
worse than the 2.90 m the pre-change code drew, and a reverse curve is one of the shapes the owner
reported. It now takes the path's own normal, corrected onto each mouth, and measures 3.34 m there;
symmetric shapes (quarter turn, U-turn) are unchanged, and both mouths still meet their links
exactly. The test that was supposed to guard this measured width *along* the cross-section, which is
the lane width by construction at any angle; it now measures square to the road as well.

## 2026-09-16 fourth follow-up — One poly point, and a constant offset

Two answers from the owner, both now the rule here.

**"Vissim moves only the one poly point that is attached to the Link."** `reanchorConnector` used to
carry the whole curve rigidly through a similarity transform of its endpoint chord, so a Link edit
dragged points the author had placed by hand. It now moves the attached endpoint and nothing else.
Path independence, which the transform was written for, comes for free: the point returns to where
the lane puts it and no other point was ever touched.

**"Should the offset from the lane centreline be the same all along?"** Yes, and that is the Vissim
rule: the polygon is the axis offset by half the total width, measured square to the axis at every
point. Links already did this — measured 3.500 m of a 3.500 m lane at every bend from 30 to 170
degrees, because `offsetGeometry` miters each corner. Connectors now go through the same function,
with a per-point offset so a tapering lane keeps its neighbours at full width. The two ends are cut square to the Connector as well,
not to the links, which is what the owner's own Vissim screenshot shows: a constant-width ribbon
whose end simply overlaps the link it meets. Measured after a Link was rotated 90 degrees under a
drawn Connector, every sample including the joint is 3.500 m, against 0.46 m from the interpolated
cross-section and a wedge at the joint from cutting the ends on the link. The joint gap that
replaces it is 0 on a straight connection and 0.12-0.29 m where the sampled curve leaves the lane
at an angle -- an overlap, not a missing lane.
