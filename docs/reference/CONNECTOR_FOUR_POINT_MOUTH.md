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
authored station values remain readable. D114 derives runtime lane paths from the final
painted rails. Retained control stations map proportionally along the corresponding
authored/path legs. The *derived reference used to construct the road* is central.
Changing Connector lane widths does not move that reference, but can move lane centres.

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

## Drawing surface and runtime lane centres

Rails and dividers reach the mouth intersections with a common smoothstep weight measured
on the **original axis**, supported on the nearest half of that axis. No distance is measured
from a vertex that was already moved. On straight Link edges the symmetric outer cuts and
common weights keep the rail midpoint on the controlling axis, including at obtuse arrivals.
On curved Link edges the mouth need not be symmetric; P2 remains the attached range centre.

`ConnectorSurface` supplies the final boundaries, markings and outline. `connectorBoundaries`
returns those same boundaries for conflict coverage and lane handles. Paint, selection,
copy/move previews and rotation bounds use the same surface cache.

D114 supersedes D107's endpoint-only runtime equation at the owner's request: dragging
interior points must steer vehicles. `connectorPaths` uses the midpoint of adjacent final
rails at every interior vertex. Terminal vertices are the named Link lane-centre attachments,
so a displaced mouth cut or closed taper does not move a vehicle off its connected lane.
Vehicles interpolate by metre distance along this path; the compiler, run canvas, signal
heads, route overlays and right-of-way controls share the same path and length.
The rolling rear-axle display follows this path while retaining continuous body heading.

The existing cubic still generates the initial/default drawing. It is not a second runtime
path. Zero intermediate points and Reset straight now yield straight driving legs; adding
or removing points, reshaping, widths and range changes can change lane paths and travel times.
Point insertion preserves the authored axis; it need not preserve mouth-blended lane centres
because the surface is reconstructed at its new cross-sections.

Lane pairing and width derivation share a topology-only helper, independent of runtime
geometry; deriving a surface therefore cannot recurse through its own paths. Stored control
stations remain metres on `Connector::geometry`. `matchedStation` maps each authored leg's
fraction onto the corresponding runtime lane leg; the inverse is used when writing derived
controls. Signal-head positions are runtime lane metres. Existing files and IDs remain readable;
no schema change or analytic equation is persisted.

Conflict coverage and highlights both use the final painted lane rails, with the same authored
cross-section stations. Physical waiting-line setbacks use the mapped runtime path distance;
waiting bars intersect those rails with the runtime path normal. Geometry edits recompute
areas and waiting controls in the existing History transaction; Undo/Redo restore them together.
Common attachment mouths remain topology joins, including longitudinal cuts that extend
past a finite Link end; separate interior crossings of the same joined lane remain crossings.
The shell clears the old Run snapshot after a successful edit or Undo/Redo, and recompiles
before the next Run. Old stored conflict extents are validated against the current surface;
loading does not silently repair them.

D80's unbounded or folded mouths still do not establish a drivable turn. Non-convex conflict
quads remain unsupported; terminal transitions do not establish whole-body containment or
cross-path physical collision safety. No new mouth cutoff or measured Vissim fidelity is claimed.

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
  rotations, frozen weights, control stations/bars, single-lane guides, file/Undo/Redo,
  and 240 seeded curve/add/drop runs with replay, accounting and segment-body checks.
- `connector_equation_tests`: independent analytic arc length, station inversion, closest
  points, analytic cubic helpers and straight-runtime reset behaviour.
- `connector_edit_motion_tests`: fixed-end interior edits, both driving sides and taper
  directions, runtime/rail agreement, compilation, heads, waiting bars, file and Undo/Redo.
- Historical D107 execution details: [equation evidence](../evidence/connector-equation.md).
- Execution results and platform limitations are in PROGRESS.md. Linux is not Windows evidence.
