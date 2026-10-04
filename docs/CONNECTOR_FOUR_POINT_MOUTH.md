# Centred Connector axis and four-point mouths

Owner instructions of 2026-09-28 (D80) supersede the legacy slide/square fallback and the
D79 reach cutoff. This is an owner-defined geometry contract, not measured Vissim parity.

## Axis before edges

`connectorCentreline` derives the whole-carriageway axis from `geometry`, translating each
point by the blend of the two offsets from the first lane's attachment to the attached range
centre. It does not read any boundary. `connectorBodyBoundaries` stacks the lane widths from
minus half the total to plus half the total around that axis, using the existing miter offset
for body corners. A surplus lane has zero width at the narrower end as before.

The serialized schema-17 `geometry` still denotes the first authored lane path. It is not
silently reinterpreted as a centreline: existing files, lane references and
authored station references remain compatible. D106 replaces the old runtime lane-path
derivation; lengths are compiled from the new paths. The *derived reference used to construct the road* is now
central. Changing lane widths does not move that reference.

Grips use the axis directly. An interior drag solves the inverse blend on [0,1] by 64 fixed
bisection steps, so the displayed point reaches the pointer after the command clears frozen
weights. Both canvas preview and release use that conversion; Undo and file round trips keep
ordinary stored geometry. The end grips remain P2 at the attached range centre.

## P1–P4 and lane order

At each end:

1. P1: near intersection of a local Connector edge and its corresponding Link boundary.
2. P2: midpoint of the attached Link range's outer boundaries at the attachment station.
3. P3: nearest point to P2 on the far **finite Link boundary polyline**.
4. P4: the other outer-edge intersection.

The local Connector directions come from the central axis. The normal's sign comes from
traffic handedness, never from a previously modified rail. Boundary k meets Link boundary
`first + k` at both ends, before and after 90 degrees. Near/far names and perimeter traversal
may reverse between ends; **lane pairing never does**.

P1/P4 use supporting lines, so an intersection can lie beyond a Link endpoint. Interior
boundaries use the same intersection rule, with their accumulated Connector lane widths.
A tapered lane closes onto its neighbour's intersection. There is no alternate cap crossing,
nearest-cap-vertex substitute, compression floor, square fallback, or maximum-reach cutoff.

For straight equal-width roads of width W, with P3's projection inside the Link edge:

- P2–P3 = W/2;
- P3–P4 = W/2 tan(theta/2), at source and target alike.

| Directed angle | P3–P4 for W = 4 m |
|---|---:|
| 45° | 0.828427 m |
| 90° | 2 m |
| 120° | 3.464102 m |
| 150° | 7.464102 m |
| 170° | 22.860105 m |
| 179° | 229.177300 m |

The old four-width cutoff rejected a symmetric mouth above 151.044976 degrees, independently
of the old 75.522488-degree square fallback. Both are removed. Near 180 degrees the formula
really does grow without bound; a retained intersection is not a claim of a drivable turn.
The body-corner miter limit remains; it is not a mouth-angle fallback.

## Singular lines and folds

Coincident parallel edge lines use the Link attachment station to define a continuous straight
join. Distinct parallel lines have no intersection: exactly reversed equal-width roads, or
parallel unequal-width roads, may therefore have no mouth. The numerical parallel tolerance
is 1e-12 on the unit-direction cross product. Non-finite intersections are also rejected.
If any required boundary intersection is undefined, that end's mouth is absent; the opposite
end's computed descriptor and rails are retained. No square cap is fabricated. With an
undefined end, no closed fill is drawn; open markings remain selectable and in rotation bounds.

A folded perimeter is retained and flagged by `selfIntersecting`. It no longer deletes both
mouths or swaps in a legacy polygon. `WARN_CONNECTOR_ALIGNMENT` reports missing intersections
or a fold, in both languages. Authoring remains possible. Conflict coverage still rejects
non-convex lane quads as unsupported instead of guessing a station through a fold.

## One geometry for consumers

Rails and dividers reach the mouth intersections with a common smoothstep weight measured
on the **original axis**, supported on the nearest half of that axis. No distance is measured
from a vertex that was already moved. On straight Link edges the symmetric outer cuts and
common weights keep the rail midpoint on the controlling axis, including at obtuse arrivals.
On curved Link edges the mouth need not be symmetric; P2 remains the attached range centre.

`ConnectorSurface` supplies the final boundaries, markings and outline. `connectorBoundaries`
returns those same boundaries for conflict coverage and lane handles. Paint, selection,
copy/move previews and rotation bounds use the same surface cache. Runtime lane paths (D106) take the midpoint of each adjacent rail pair at every interior
vertex. Their terminal legs join those midpoints to the named Link lane centres. Mouth
intersection midpoints can be displaced along the Link, and a zero-width taper ends on a
boundary, so neither is substituted for a runtime attachment. A two-point Connector remains
one straight leg between attachments. There is no new smoothing or persisted path.

Lane pairing and width derivation share a topology-only helper, independent of runtime
geometry; deriving a surface therefore cannot recurse through its own paths. The compiler,
run canvas and route overlays consume `connectorPaths` and its polyline lengths. Retained
authoring stations are mapped vertex-for-vertex with `matchedStation` onto these paths.
Waiting-line bars intersect the actual rails with the normal at that mapped runtime station.
Symmetric single-lane points within 1e-12 m of the stored vertex retain its exact bits.

The terminal legs are attachment transitions, not rail-midpoint lines. This includes the
merge into the receiving lane at a taper. D80's unbounded or folded authoring mouths still
do not establish a drivable turn; there is no new mouth cutoff or lane-change motion model.

## Verification

- `connector_surface_tests`: numerical examples, 5–355° sweep, rotation/reflection, widths.
- `connector_mouth_tests`: both ends and driving sides, 180 angle/rotation cases through
  179.9°, signed lane order, midpoint preservation, singular/fold reporting, centred unequal
  lane ranges, pointer inversion, save/reopen and Undo/Redo.
- `connector_mouth_sweep_tests`: 1,512 cases with 1–3 lanes, offsets, widths and driving sides;
  each divider is checked against its **own indexed** Link edge and the central-axis normal.
- Body-width tests measure `connectorBodyBoundaries`; end-intersection tests measure the final
  surface. Old assertions about a perpendicular/square legacy cap are superseded.
- `connector_lane_centre_tests`: 52 count/width/side combinations, curved body attachments,
  rotations, frozen weights, control stations/bars, exact single-lane paths, file/Undo/Redo,
  and 240 seeded curve/add/drop runs with replay, accounting and segment-body checks.
- Execution results and platform limitations are in PROGRESS.md. Linux is not Windows evidence.
