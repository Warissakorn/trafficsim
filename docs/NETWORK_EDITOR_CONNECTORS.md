# Native network editor — Connectors

The Connector sections of [`NETWORK_EDITOR.md`](NETWORK_EDITOR.md), split out on 2026-09-29 so both files stay
under the 500-line limit. Current, not archived; the same scope and the same caveats apply.

## Connector lane ranges

Select **Connectors (C)**, then Ctrl+right-drag from a source link lane to a target link
lane. The dialog chooses the first lane and contiguous lane count at each end. Left-click
during creation can add intermediate points. One completed gesture creates one connector
object even when it carries several lanes.

Both ends can attach anywhere on the **body** of their Link; endpoints remain valid.
The two-click workflow (C) also picks positions on lane bodies. Picking **snaps to a point**,
which is Vissim's *Snap to Points*: the lane's two endpoints, then a station another Connector
already attaches at on that lane, then the Link's own intermediate points — the first of those
within the radius wins, and the station taken is that point's exactly. Two movements leaving one
corner are therefore authored at one cross-section rather than centimetres apart, which is under
`kMinSectionLength` and would be refused at Run, and a Connector meeting a Link at a bend lands on
the bend. The radius is a screen distance, so zooming in past roughly 20 pixels per metre narrows
it below that limit and fine placement wins, which is what an author zoomed that far in is asking
for. The endpoint snap is not cosmetic: a Connector at a Link end is stored with **no** station at
all and compiles to a departure, while one a few centimetres short is a body attachment whose stub
section is under `kMinSectionLength` and cannot Run. Hidden levels cannot be picked. Properties → Connectors exposes
actual link/lane IDs and `from.station` / `to.station` in metres from the Link's start, as
Vissim stores a position. One station names one cross-section, so every lane of a range meets
the Link square even on a curve, and the number means the same thing whichever lane is picked.
The dialog opens at **one lane per end** — a gesture that starts on a single lane authors a
single-lane Connector, and a wider range is raised deliberately, within the lanes each end has.
Changing lanes preserves the selected stations, and so does adding or removing lanes: a
station is measured on the reference polyline, which `laneOffset` keeps fixed. Releasing outside a target Link reports
why nothing was created. Esc and Cancel leave the document and history unchanged.

A connector stores a base polyline and source/target lane counts. Its lane paths are
derived in monotone order, with stable IDs: the first uses the connector ID and subsequent paths use `id/lane-2`, `id/lane-3`, etc. Routes and signal heads can reference those paths.
Unequal counts pair lane for lane over the narrower end, with at most one lane added or dropped on each side, so the counts differ by at most 2 (M3.2.9a, D73). For a one-lane difference
`laneChangeSide` (schema 17, `"left"`/`"right"`, the driver's view; the Inspector's "Lane change side", M3.2.9c) picks the side, absent = kerb side; that lane is the one drawn tapering. Lane tabs stop at a two-lane difference. A Connector asked for across a larger difference (a 2-lane Link dragged onto a 5-lane one) is created narrowed to it: the wider end gets `narrower + 2` lanes, centred on the lane the drag ended on (M3.2.9d, D75). Ranges are limited by the existing lanes, at most 12 per end.

In Select (S), rectangular **lane tabs** exist even on a one-lane Connector. Each sits directly
against the carriageway edge, without a stem or a numeric label:

- Source handles on both sides: grow/shrink the contiguous source lane range.
- Target handles on both sides: grow/shrink the contiguous target lane range independently.
- Middle handles on both sides: set both ends to the same count, limited by available lanes.
- Each selected Link has tabs at the start, middle and end on **both sides** to add/remove lanes (up to 12). Existing
  widths and world positions are retained. Added lanes use the width of the dragged edge lane.

Drag outward to add lanes and inward to remove them; the road geometry previews the result
during the drag. One release is one undo entry. Esc cancels. Each tab changes
its own edge, leaving the opposite edge fixed. The first-side handles add/remove lanes
before the current first lane; the other handles change the last lane. Surviving lane
IDs and positions stay fixed, including on curved Links. Connector paths whose lane pair
survives a range edit retain their curve; unequal ranges can intentionally change lane mappings.
Properties count edits and downstream pocket creation expand the last-lane side.

## Connector shape: intermediate points and the mouth

A Connector is stored and drawn the way Vissim's is: its two attachments and a few
**intermediate points**, joined by **straight legs and mitered at each point**, exactly as a Link
is. It is not smoothed — a Connector with two intermediate points is three straight legs with a
corner at each one, which is what Vissim draws. The count is what decides how closely that polygon
follows the turn, and it is Vissim's own `Intermediate points` field, in Properties → Connectors.

Changing the count never re-derives the default curve; `Reset curve` is the button for that.
Raising it splits the longest leg each time, so every point already there survives and the drawn
line does not move at all. Lowering it spaces the points evenly along the shape that is there,
giving up only the corners the lower count cannot hold. A new Connector gets 3; 0 leaves one
straight leg between the attachments. `Reset curve` lays 3 along the arc, and laying more along
it follows the turn more closely — 2.29 m of sag from the arc at one point, 0.60 m at three,
under 0.10 m at fifteen.

**The mouth must not become a needle.** Ordinary forward arrivals use the bounded M1.19
projection/slide onto the Link's cross-section. Near-perpendicular arrivals (forward tangent
dot product below 0.25), including backwards approaches, cannot be aligned this way without
collapsing or reversing lane order. Those ends retain their full-width square cross-section
and do not slide. `WARN_CONNECTOR_ALIGNMENT` flags the fallback or a residual gap over 1 cm.
The warning is advisory: the drawing can be saved, but exact lane-edge alignment is not
claimed. Use `Reset curve` or adjust intermediate points/attachments to approach with traffic.

An endpoint grip sits on the middle of the Link lanes that end joins (the mouth's P2, D77), where
a drop is measured too. Dragging it along the lanes it joins keeps the curve, every point shifted
by its blend weight (D78); onto other lanes, or where a kept end leg would run against its lane
(the old wrong-way elbow), it rebuilds the turn at its point count. One undoable edit either way. Moving a Link still follows the separate world-position/deletion contract described
above. Merely opening a file does not regenerate any authored curve.

Interior points are editable; dragging one moves that corner and nothing else. Reset curve to
lane directions lays the points along a cubic. Each control point reaches `(2/3)·chord·tan(α/2)/sin(α)`, where α is the angle
between **that end's** lane direction and the chord — the cubic that stands in for a circular
arc leaving at that angle. It is `chord/3` as α tends to zero, the constant every turn used to
get, `(2/3)·chord` for a U-turn, which used to be drawn at less than half the radius it needs
(0.18 of the chord instead of 0.45), and it is the only reading that catches a reverse curve,
whose two ends are parallel while each still leaves its chord steeply (0.18 of the chord to
0.22 on a measured S). That reach is held at its 120-degree value, `(4/3)·chord`: past there it
runs away — 11.05 times the chord at 160 degrees — and a Connector drawn where two links nearly
touch left the junction altogether, measured at 11.0 times its own chord and now 1.9. Every
ordinary turn, U-turn included, is unchanged to the last bit. A Connector that still turns tighter than its own width is reported in Objects and
issues as `TIGHT_CONNECTOR_RADIUS`; the drawing is kept and Run is not blocked.
Make straight retains only endpoints. There are no separate persisted
Bézier handles and arbitrary edits need not remain smooth, though a reshaped curve is
held to the same geometry rules as a link: no non-finite coordinates, no repeated
consecutive points and a positive total length. Coincident endpoints cannot
generate a default curve; leave a positive gap. Duplicate lane-pair connections at the same source/target stations are
rejected, including pairs already covered by another connector range. Separate stations
on the same lane pair may own separate Connectors.
