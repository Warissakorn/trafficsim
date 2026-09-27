# Four-point Connector drawing mouth

Owner-authorized on 2026-09-27 after coordinate examples, including the correction that P3
projects onto the **Link**, not the Connector. This is a drawing contract, not a claim of
measured Vissim parity and not a change to vehicle paths or right-of-way.

## Points and orientation

At each end, `connectorSurface` builds a cap P1 → P2 → P3 → P4:

1. P1: near intersection of a local Connector edge and a Link range edge.
2. P2: midpoint between the attached lane range's two Link boundaries at its station.
   A multi-lane Connector's stored first-lane endpoint is not this midpoint.
3. P3: nearest point to P2 on the far **Link boundary polyline**. On a straight Link this
   is the perpendicular projection; on a finite segment it may be its endpoint.
4. P4: far intersection of the other local Connector edge and Link range edge.

The local Connector edges use the authored end-leg direction and total end width from
`connectorLaneWidths`. The Link edges use the selected lane range, `matchedStation`, and
the directed edge segment at that station. Intersections use the supporting lines; P3 uses
the finite Link boundary. Of the two pairings, use the one with smaller total reach from P2.
This switches Link-edge pairing beyond 90 degrees instead of extending a long miter.
Near/far are ordered looking outward from the Connector body (opposite directions at source
and target). Ties use boundary order, with a metre tolerance to avoid roundoff-driven flips.

For straight equal-width roads of width W and the acute angle phi between their axes:

- distance(P2,P3) = W/2;
- distance(P3,P4) = W/2 tan(phi/2) ≤ W/2.

The bound is not asserted for unequal widths, curved Link edges, or a projection clamped at
a Link endpoint. These are evaluated geometrically, not forced into the equal-width formula.

With the horizontal Link bounded by y=0 and y=4 and P1=(0,0):

| Angle | P2 | P3 | P4 |
|---|---|---|---|
| 45° | (-0.828427, 2) | (-0.828427, 4) | (-1.656854, 4) |
| 89° | (-1.965395, 2) | (-1.965395, 4) | (-3.930789, 4) |
| 90° | (-2, 2) | (-2, 4) | (-4, 4) |

## Rendering and limits

The existing body rails remain. Their outside endpoints move to P1/P4, and P2/P3 become
additional cap vertices. Caps are reversed as needed to assemble one perimeter, rather than
being appended in a fixed left/right order. The cap itself is not a painted stop line.
Coincident adjacent vertices are collapsed.

**Dividers (M3.2.9b, D74).** The mouth also yields one point per Link lane boundary of the
attached range, in the order of the Connector's own boundaries (reversed past 90°, as the
rails are): the range edges are P1 and P4, and each interior Link boundary's line crosses
P1 → P2 → P3. Interior Connector boundary k ends on the point after as many lanes as have
width at that end, so a surplus (added/dropped, D73) lane closes onto its neighbour's point, or
onto P1/P4 when it is outermost. Rails and dividers reach their points by the same bend: the
shift fades out (smoothstep) over the half of the boundary nearest that end, so the other
half is untouched. With a mouth present, dividers are **not** clipped: one may reach its point
across the P2–P3 notch, which is Link surface. Only the legacy cap still clips.

Parallel edge lines have no unique intersection and retain the existing cap. An intersection
whose combined reach exceeds four times the larger width is also unusable. If the new
perimeter crosses itself (for example on a short sharp bend), the whole Connector retains
the prior surface, with absent `source`/`target` mouth descriptors; the four points are never
silently deleted by loop trimming while still being reported as active.

Paint, hit testing, box selection, framing, copy/move/rotation previews and the rotation
bounds read the same `ConnectorSurface`. The existing value-keyed cache covers the Connector,
both Links and driving side, so previews and Undo/Redo invalidate the same result.

The legacy `connectorBoundaries` lane strips still drive conflict coverage and existing
mouth-fit/alignment diagnostics. Those diagnostics measure the lane strips, not the new
display cap. The surface is derived only: no project schema, persisted vertices, runtime
path lengths or conflict-priority rules change in this work.

## Verification

`tests/connector_surface_tests.cpp` checks the agreed numerical examples, perpendicular
projection onto the Link, the acute-angle shoulder bound, source/target ends, rotations,
reflections, driving sides, a two-lane range, unequal widths, and the parallel fallback.
`tests/connector_mouth_sweep_tests.cpp` checks every divider end against its Link boundary
over 1,512 cases (target 30–150°, source 0/±45/30/60/90/135°, 1–3 lanes each end, offset
ranges, unequal widths, both driving sides).
`tests/connector_surface_ui_tests.cpp` checks actual canvas paint, picking on both sides of
the cap, Link-dependent cache invalidation and Undo/Redo. Environment-specific execution
results are recorded in PROGRESS.md; a Linux run is not Windows evidence.
