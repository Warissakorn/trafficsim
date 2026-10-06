# Positioned routing decisions (D119)

## Authoring contract

`RoutingDecision.position` is optional metres along its Link reference polyline.
Schema 20 writes it only when present. Older files keep their scheduled-demand-time
routing, compilation order, IDs and schema choice. A schema below 20 containing the
field is rejected, including `position: 0`. Missing and zero have different semantics.

The Route tool's first plain click on a Link captures the projected physical station.
Hover remains a local preview; the destination commits the Route and its positioned
Routing decision in one History operation. Starting within the line's ten-pixel
station tolerance reuses its exact stored station and appends another Route. Existing
periods gain one default weight for the new Route; type rules lock destination membership.
Ctrl+right/Enter and starts on a Connector retain their legacy unpositioned Route semantics.

Select and drag the decision line along its Link, or edit `position` in the decision
dialog. Preview does not change History. Release commits once; Escape/focus/tool/level
changes cancel. Position must be finite, nonnegative and below the Link's reference
length. Shortening the Link beyond a stored station is rejected atomically.
The first slice retains one decision per Link. A second point on that Link is refused;
use its existing line or reposition it. Decisions on successive Links are supported.

The input gesture on a Link carrying a positioned decision offers a routeless Link
input. Explicitly assigning an input a fixed Route retains that fixed assignment;
station decisions affect the routeless streams, as legacy placed decisions do.
Inputs mark source lanes; decisions mark recognition stations. Demand preview labels
provisional path allocations as chosen at the routing line, without a booked destination.

## Runtime interface

Project compilation derives `RouteDecision { id, fromRouteId, at, intervals, choices }`.
Each choice names a compiled route, default weight and complete interval weights.
Type expansion supplies that type's weights before handoff. Core sees only route
coordinates, immutable alternatives and weights, never authoring Links or geometry.

Positioned destinations are enumerated with neutral weights to retain all possible
suffixes, including a destination disabled during one interval. Initial Poisson path
allocations are provisional; their rates conserve the source total and lane shares.
A provisional suffix is not a selected destination. Legacy decisions alone still
split source periods at their scheduled-time breakpoints.

All alternatives share the same physical prefix through the line. A point beyond
an alternative's departure from the Link is blocked. A line inside an active conflict
zone (from its wait/entry line through its exit) is refused, preserving admission. Lateral spans on the decision
Link begin no earlier than the recognition station, on both source and target lanes.
The existing whole-vehicle, conflict, gap-safety and dead-end rules still apply.

When the front reaches the line, the engine draws once in vehicle-ID order using its
seeded RNG, at the passage tick's simulation time. Half-open intervals and type
rules apply; an all-zero active interval falls back to default weights. Each vehicle
retains passed decision IDs across route changes and snapshots. Source waiting never
books a positioned destination. An arrival downstream of a line does not cross it.
`RoutingEvent` records time, decision and old/new Route IDs; checkpoints retain the
passed IDs. A source station of zero is recognized before its first movement.

### Numerical boundary and limits

Recognition is a discrete tick boundary, not a continuous-time substep solver.
A crossing tick advances only to the line, retains its calculated speed, and selects
before subsequent travel. The remaining distance proposed for that tick is discarded:
recognition time is quantized to the tick (up to one timestep), and its effect on
whole-trip travel time has not been measured. The next tick
runs following, signals and conflict admission on the selected suffix. The routing
line causes neither a zero-speed stop nor a SafetyClamp event. This conservative
boundary prevents travelling onto an unselected suffix under another Route's checks.
Near-line placement is not a promise that a lane change can finish before a turn;
the existing dead-end behaviour remains. Cyclic walks and repeated reselection,
multiple positioned decisions on one Link, Connector stations and continuously
integrated recognition remain outside this slice. No calibration or M6 claim follows.

## Acceptance

| Check | Regression |
|---|---|
| No selection before line; one selection when crossed; immutable replay | `stationrouting` |
| Passage time, half-open boundary, type overrides and interval fallback | `stationrouting` |
| Zero source station and arrivals downstream | `stationrouting` |
| Independent consecutive recognition lines | `stationrouting` |
| Shared prefix, valid weights/stations and complete interval matrices | `stationrouting` |
| Source totals/lane shares, clipped awareness spans, schema opt-in | `stationrouting` |
| Click station, shared point, local preview, atomic Undo, drag/cancel, reopen | `station-routing-ui` |

Automated Linux/Windows CI and an owner's desktop appearance review are separate.
M2.1.3 remains in progress until those checks pass. Existing M0/M2/M3/M6 gates stay open.
