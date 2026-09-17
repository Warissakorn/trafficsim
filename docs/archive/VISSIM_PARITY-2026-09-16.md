# VISSIM_PARITY archive — 2026-09-16

Follow-ups moved whole from [`VISSIM_PARITY.md`](../VISSIM_PARITY.md) to keep it below 500
lines (hard rule 6). Nothing here is edited or summarised -- only relocated.

The seven sections below are the **first seven 2026-09-16 follow-ups, in the order they were
written**, recovered from `git log -p` on the live file: `0a4f26e` 02:42, `c76b4f8` 03:48,
`3c0766c` 04:23, `68ddf02` 07:44, `bb85a69` 08:40, `01a87f4` 09:53, `e81a591` 13:39. The
ordinals in the headings start at `second` only because the numbering was picked up partway
through; they are not a second ordering. The live file resumes at the fifth follow-up
(`a6b9ec8` 18:39).

## 2026-09-16 follow-up — Grips, end attachments and lane-count checks

The owner's annotated screenshot marked four things: the lane tabs looked unfinished, the
Link and Connector geometry points sat at a road edge instead of the middle, the Connector
lane counts were not checked against what the other end actually has, and the Connector
end points could not be moved.

Lane tabs are now edge-mounted rounded tabs with a stem and the resulting count inside them,
in place of a loose dot with a floating number. Geometry grips moved to the bundle centreline
(`linkCentreline`, `connectorCentreline`); the stored polylines are unchanged, and a drag maps
back through the same offset. Connector ends are draggable onto any lane or position along it:
the range is centred on the lane under the pointer, and it is narrowed when the new end has
fewer lanes than the Connector carries. Widening a Link still never widens a range on its own,
because how many lanes a movement carries is the author's decision.

Not changed, deliberately: unequal ranges remain legal drawings and still fail the M0 run
check as `UNSUPPORTED_MERGE` — the counts are now shown beside the Connector length so the
author sees a 3 → 2 drop without opening the inspector. Re-attaching a Connector used by a
route or head is still rejected. Group transforms and the remaining booked gaps are unchanged.

## 2026-09-16 follow-up — Bent carriageways, one-lane connectors, and where an attachment lives

Three more owner findings. Two were defects and are fixed; the third is a model decision,
recorded here and booked as M1.13.

**Bends.** Lane edges were offset along the corner's average normal by the full width, which
leaves them `width/2 * cos(theta/2)` from the centreline — the carriageway pinched at every
bend, 30% at the right angle in the owner's screenshot. Offsetting now uses the miter length,
so Links, Connectors and the diagnostic view all keep their width through a corner. No pinned
simulation baseline moved: every reference fixture is a straight network, and straight
polylines are unchanged bit for bit.

**One lane means one lane.** The creation dialog pre-filled both lane counts with every lane
from the picked one to the end of the Link, so a drag from a single lane authored a two- or
three-lane Connector complete with lane dividers. It now opens at one lane per end. The
Properties counts also reset to one when no Connector is selected, so a previous selection
cannot seed the next creation.

**Where an attachment lives.** Vissim attaches a connector to a lane at a position and drags
it with the link; so do we, and that is not negotiable — validation requires the end to sit on
its attachment, and the compiler builds lane → connector path → lane, so a Connector left
behind in world coordinates would be a network the engine cannot run and a vehicle would
teleport across the gap. What is wrong is the *unit*: a fraction of lane length means
stretching a Link slides every interior attachment, and with it the lane-section lengths a
future run would measure. Vissim stores a distance; so does our own `NetworkSignalHead`. M1.13
books that change, with the migration and identity constraints it has to respect.

## 2026-09-16 follow-up — M1.13 implemented: attachments are metres along the Link

The unit decision recorded above is now the model. `LaneReference::station` holds metres along
the Link's reference polyline, Vissim's `Pos`, rather than a fraction of the attached lane's
arclength. Stretching a Link no longer slides the Connectors attached part-way along it, and a
multi-lane range meets a curved Link on one square cross-section instead of fanning with the
per-lane arclength difference.

Two behaviours the owner should know, because Vissim does not spell them out either. Shortening
a Link past an attachment clamps the Connector to the new end rather than refusing the edit — a
Signal head in the same position still refuses, which is the older contract and deliberately
left alone. And the number in Properties is measured on the Link's own line, so on a curve it
differs slightly from the distance travelled in an outer lane; that is what makes it the same
number for every lane of a range.

Remaining gaps are unchanged: group transforms, `Alt`-drag rotation, editable table cells and
the M1.11.1 runtime lane sections, which this change exists to make tractable.

## 2026-09-16 follow-up — Merge markings and the shape of a tight turn

Two more owner findings, both about what a Connector looks like rather than what it stores.

**A divider down the middle of one lane.** Where a Connector's ends carry different lane counts,
the interior boundary was pinned at the narrow end to the *centre* of the single lane the paths
converge into, and drawn dashed for the whole length — a lane line down the middle of where
vehicles drive. Markings are now derived separately from the boundaries: an interior divider
covers only the stretch where the two lanes are at least half their full spacing apart and stops
at the merge. The ribbon itself was already right, tapering 7.0 m to 3.5 m across a two-into-one.

**A U-turn drawn at half the radius it needs.** The default curve reached `chord/3` for every
turn, which is only the correct value as the turn angle tends to zero. On the owner's U-turn that
produced a minimum radius of 2.29 m on a 13 m chord — 0.18 of the chord, tighter than the 3 m
lane it carries, so the ribbon's own inner edge crossed itself, and the even-odd fill punched the
overlap out as the hole visible in the screenshot. The reach is now the circular-arc value for
the actual turn angle: unchanged for gentle turns, 0.45 of the chord for a U-turn (5.90 m here).
Surfaces fill by winding rule, so a self-overlap that remains reads as road.

Vissim leaves an impossible turn to the author; we do the same but say so, with a non-blocking
`TIGHT_CONNECTOR_RADIUS` row. Stored geometry is never rewritten, so existing drawings are
untouched — only newly created curves and Reset curve use the new reach.

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
