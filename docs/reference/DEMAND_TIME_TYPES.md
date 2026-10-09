# Time and type Demand — slice 6

## Interface and semantics

Schema 19 adds `Composition.intervals`: sorted, non-overlapping half-open periods
containing a complete `types` weight list. Base `types` applies outside the periods,
including gaps. Every list is nonempty, has unique type IDs and finite strictly
positive weights; omit a type to exclude it. An empty/zero period is an error,
including in an unused owned composition. Input volume periods remain authoritative, except for
an input with `volumeFromCounts`, whose periods are its decision's counted intervals (D142,
[DEMAND_IMPROVEMENT](../plans/DEMAND_IMPROVEMENT.md) §7).

`RoutingDecision.typeRules` is a list of complete overrides. Each rule has one
`vehicleTypeId`, `relativeFlows` in the decision's route order and route-major
`intervalFlows` on the decision's existing intervals. Weights are finite and
nonnegative; whole-period total must be positive. Every row/cell is explicit.
An all-zero counted interval uses that type's whole-period weights, matching
existing default routing semantics. A type without a rule uses default flows.
Rules do not create new destinations; target topology/reachability remains shared.

Catalog/type expansion precedes type-conditioned routing. Active input periods
are cut at composition and decision breakpoints, conserving their rates through
both splits. A legacy placed decision without `position`, including one downstream, uses the vehicle type
and scheduled demand time. Source queueing and time spent travelling do not change
its booked destination. Family routes for different types stay distinct.

There is no new random draw or exact-count mode. Compiled type/route/lane/period
inputs use the existing independent Poisson streams. Same document, seed, engine
and toolchain replays deterministically; enabling the new splits may change the
old random sequence. Files without new features use the old compilation order,
IDs and schema-17/18 save behavior, preserving frozen fixtures.

## Authoring and gates

Composition periods are edited within the staged catalog dialog, then captured in
one History operation. Type rules are edited locally within the decision dialog;
Cancel, no-op, Undo/Redo and save/reopen preserve both rule matrices and ownership.
Existing destinations must be saved before defining type rules. Remove overrides
before changing destination membership. Shared explicit periods edit default and
type-specific counts together. Preview and Run compile the same document snapshot.

Acceptance covers every breakpoint, gaps, exclusions, input-volume intersections,
placed/unplaced and downstream decisions, missing type rules, invalid/unused rules,
reference protection, atomic failure, portable load, deterministic replay, queued
records keeping their selection, and legacy fixture equality. Native CI and owner
appearance review are independent gates. M2's earlier observed gate is not reclosed.

Desired-speed distribution families, exact-count arrivals, dynamic reselection,
motorcycle lane occupation and articulation remain separate contracts. Reporting,
LOS and batch evaluation remain M5.

Positioned decisions opt into passage-time selection under schema 20; see
[POSITIONED_ROUTING](POSITIONED_ROUTING.md). Their intervals do not split source streams.
