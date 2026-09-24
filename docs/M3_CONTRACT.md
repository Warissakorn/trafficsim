# M3 right-of-way contract

**Design for implementation after the M2 gate, not an implemented API.** Prepared with
[M3_PLAN.md](M3_PLAN.md); evidence requirements are in [M3_ACCEPTANCE.md](M3_ACCEPTANCE.md).
All proposed names below describe one small initial capability: explicit, at-grade,
fixed-lane conflicts. No calibrated critical-gap, automatic collision detection over the
whole drawing, lane-changing or Vissim-fidelity claim follows from these types.

## 1. Units, ownership and references

All runtime distances are metres, times seconds, speeds m/s. Positions measure vehicle
front bumpers; rear position is front distance minus vehicle length. Values must be finite.
Numeric tolerances are named numerical constants, never concealed calibration parameters.

Authored controls belong to the project network. Runtime segments, route incidence,
occupancy and reservations are derived and are never persisted as editable network data.
Separate the existing core `PriorityRule` from a new authored priority-rule type.

| Proposed value | Fields and interpretation |
|---|---|
| `ControlPathRef` | A tagged Link lane (`linkId`, `laneId`) or Connector lane (`connectorId`, source and target authored lane IDs). Resolve exactly one path; do not persist `/sec-k`, `/lane-k` or scenario slots |
| `ControlPoint` | `path`, `station`. Link stations are metres on its reference polyline; Connector stations are metres on its base polyline. Convert to the selected lane/path with one shared geometry helper |
| `WaitingLine` | Globally unique `id`, `name`, `point` (`ControlPoint`). Shared by conflict sides, Stop/Yield and counters; its station is stored once |
| `ConflictSide` | `path`, `entryStation`, `exitStation`, `waitingLineId`. Entry precedes exit; the referenced line is upstream of entry on every applicable route and may lie on a preceding Link |
| `ConflictArea` | Globally unique `id`, `name`, `kind` (`crossing`/`merge`), two sides, and `priority` (`firstYields`/`secondYields`/`undetermined`) |
| `AuthoredPriorityRule` | Globally unique `id`, `name`, `conflictAreaId`, `gapTime`, `headway`. At most one per area; direction and waiting/major markers come from the area, not a second copy |
| `StopControl` | Globally unique `id`, `name`, `waitingLineId`, `mode` (`stop`/`yield`), and the conflict-area IDs it controls. Every referenced yielding side must name that same line |
| `AuthoredQueueCounter` | Globally unique `id`, `name`, and measurement lines. Each line is either a reference to an existing head/control/wait line or an explicit `ControlPoint`, never both |

A multi-lane gesture expands into explicit lane-pair areas through one command. Expansion
must display which pairs it controls. A Connector lane pair which resolves to zero or
multiple paths is unresolved; a changed lane count never silently retargets it by ordinal.
No route's Link/Connector authoring contract changes, and no lane-changing decision is added.

Link/Connector geometry edits preserve numeric stations on their reference geometry and
recompute the visual/runtime conversion. Out-of-range or no-longer-overlapping extents
remain visible invalid controls and block Run. Do not clamp an invalid extent and claim
the author chose the new conflict. Split remaps the affected stations; a span crossing a
split must be represented losslessly or the split rejects atomically in the first slice.

For crossing areas, validate that the selected swept lane surfaces actually overlap and
that the interval extents cover that overlap. A merge area must correspond to the paths'
common downstream topology. Changes invalidating this coverage block Run. Merely storing
two arbitrary numbers is insufficient to claim geometric collision protection. Tangencies,
self-intersections and ambiguous overlaps unsupported by the geometry resolver are named
capability blockers, not guessed areas. Drawing level is not physical separation.

## 2. Authoring and runtime validation are different

**Structural errors reject the edit/load atomically:** duplicate IDs, malformed references,
unknown enums, non-finite/negative parameters, invalid interval order, dangling object IDs,
multiple rules for one area or duplicate controls at the same line with conflicting modes.
The currently valid document remains unchanged. Unknown schema versions/fields are not dropped.

**Incomplete or unsupported geometry/control may remain a saved draft:** `undetermined`, a
stale lane-pair mapping after editing, invalid geometric coverage, ambiguous route incidence,
or a conflict-group topology without an implemented solver. Problems identifies the authored
object, and Run refuses with a specific capability diagnostic. This follows D18b's existing
edit-versus-Run boundary. Missing runtime references must never become an unchecked index.

`undetermined` has no implicit winner and no "both go" interpretation. It is editable and
persistable, but blocks Run for that explicit area in the first release. It is not an all-way
yield model. Such a model would need its own deterministic arbitration and acceptance tests.

An area with determined priority needs one explicit rule with finite `gapTime > 0` and
`headway > 0`. Creating it copies the current catalog defaults into that authored rule once;
afterward those values are the source of truth. A saved explicit control stays portable
without the defaults catalog. An unresolved draft need not have a rule yet.

## 3. Effective priority and legacy compatibility

Use one pure resolver for diagnostics and compilation:

```cpp
// Proposed model seam; names may follow local conventions during implementation.
RightOfWayResolution resolveRightOfWay(const Network&, const ScenarioDefinition&);
// Result owns compiled controls, source-object mappings and ValidationIssue values.
```

The compiler consumes its output only when there are no Run-blocking issues. The UI reads
the same issues. `core/` never calls this model function or reads authored geometry.

At each merge, form a local graph of incoming paths. An edge means "this path gives way to
that path". The effective graph must be acyclic and totally order the mutually competing
incoming paths: one path has highest priority, and no unresolved pair can enter together.
Counting `n-1` yielding paths is necessary in some cases but not sufficient. Reject reciprocal
rules, longer cycles and incomparable competing paths. Keep `UNSUPPORTED_MERGE` for a merge
with no usable arbitration; add object-linked diagnostics for a contradictory authored graph.

Resolution policy:

1. With no explicit override for a merge, preserve M3.1's exact drawing-order fallback and
   its catalog/default failure behavior. Existing legacy scenarios keep their behavior.
2. When a user overrides a merge group, materialize its effective pairwise controls in one
   transaction, prefilled from the fallback. Authored decisions then own the **whole group**.
   Omit the group's derived rules. A partially specified group is a Run-blocked draft.
3. Never append reverse authored rules beside the old derived rule, and never silently
   break a cycle by ID or drawing order. An unrelated merge retains its fallback.
4. Deleting an individual explicit rule leaves an incomplete group blocked. Returning the
   entire group to automatic priority is a separate named, undoable action. Do not make
   deletion secretly change right-of-way.

Explicit crossing controls have no automatic fallback. With no new controls, compilation,
RNG draw order and every frozen baseline stay unchanged. Legacy `PriorityRule` values may
still be supplied through the C++ scenario API; the existing project codec does not persist
them. Do not describe a nonexistent old JSON feature as a supported migration path.

## 4. Runtime contract and admission order

The compiled scenario contains only segment/route references and numeric positions. A
conflict side resolves to ordered segment intervals and route-relative waiting/entry/exit
distances. Resolve all routes reaching the major entry through upstream sections, not only
vehicles already in the segment containing the entry. Route incidence is indexed once.

The proposal extends state with owned, reproducible values for active grants and per-vehicle
stop service. No global/static mutable cache or UI state is part of a decision. Copying a
`SimState` must preserve independent future runs. No new RNG draw is needed for threshold
arbitration; any later stochastic acceptance model has its own seeded contract.

For each tick:

1. Take the immutable pre-step state after the existing arrival/insertion phase. Build
   front/rear occupancy and candidate travel subject to following, signals and served-stop
   constraints. Candidate calculation must not already assume conflicting grants.
2. Retain grants until the **rear** clears all reserved intervals. A front crossing an exit
   does not free the area. Route sinks must lie beyond the area's clearance distance for
   every supported vehicle type, or Run refuses; front-at-sink removal cannot clear a tail.
3. Collect requests whose proposed swept travel reaches a waiting/admission line. Resolve
   requests before applying any vehicle update, ordered by active occupancy, explicit
   priority, request tick, then stable vehicle ID for otherwise equal nonconflicting ties.
4. A grant needs acceptable gap/headway, all required areas free, and enough downstream
   receiving space for the whole vehicle plus standstill distance. Reserve the receiving
   space for competing same-tick requests as well. An existing grant cannot be revoked
   inside an area to admit newly arriving priority traffic.
5. Cap denied movement at its waiting line using the existing leader/stop-line mechanism.
   Apply the same physical safety constraints to **both** sides; a major vehicle must wait
   for an already admitted minor vehicle to clear. Green never bypasses physical occupancy.
6. Publish all next-state vehicles, grant/stop state and events together. Check swept
   intervals over the whole tick, not just end positions. Emit/count emergency clamps as
   today; do not disguise them as ordinary calibrated braking.

No compatible same-direction following vehicle is blocked merely because another vehicle
uses the same side; longitudinal spacing still decides. Opposing sides of one conflict
cannot occupy it simultaneously. For successive areas with no vehicle-length waiting space
between them, admission reserves the connected group atomically. If a group's topology
cannot be resolved safely, block Run until its solver exists. Do not resolve deadlock by
teleportation or deleting demand. Record unserved vehicles and diagnose the cause.

This conservative first solver may lose capacity by reserving an entire area/group. Measure
and disclose that limitation. Stable arbitration is reproducibility, not scientific validation
or a fairness guarantee under saturated major-road demand.

### Gap/headway boundaries

Preserve the existing M3.1 comparison convention for the threshold model:

- Let `d` be a major vehicle's front distance to conflict entry along its approaching route.
  `d <= headway` blocks even at zero speed; an already occupying vehicle blocks until its
  rear clears the exit.
- For `d > headway` and `v > 0`, block when `d / v < gapTime`. Equality satisfies the time
  threshold only; it does not bypass the headway, occupancy or swept-safety checks.
- A stopped vehicle farther than headway is not by itself a reason to block forever. If
  it starts moving, the common admission/occupancy solver still prevents simultaneous entry.
- Take the logical OR across all relevant major vehicles, and AND with every required
  area's admissibility. A later rule cannot erase an earlier denial.

This is a deterministic threshold, not a calibrated critical-gap distribution. No assertion
about expected capacity follows just from choosing seconds and metres.

## 5. Stop, Yield and signal interaction

`yield` requires gap/admission checks but no mandatory stop when the path is clear. `stop`
requires the vehicle front to reach the line (within the named position tolerance), have
zero speed, and remain served there for at least one complete simulation tick before it may
request admission. This one-tick minimum defines the initial model, not a jurisdiction's
legal stop duration. No arbitrary dwell-time calibration parameter is added in this slice.

A queued vehicle stopped behind its leader is **not** served. Service is tracked by vehicle
and control line; it survives waiting for a gap or a red signal, is cleared after passing,
and resets on a new run. No route has repeated segments in the current supported runtime.

At a co-located signal and stop/yield line, constraints compose: red/amber hold; green only
removes that signal's hold. Stop service, gap and physical occupancy still apply. These are
permanent controls in the first slice; time-dependent precedence is not silently inferred.
Any future "signal disables priority" option needs an explicit contract and regression tests.

The existing interior-head mechanism remains the source for signal positions, including a
head exactly on a cut belonging upstream. Add missing pointer/keyboard authoring and lifecycle
tests rather than introduce a second signal model. D36's amber behavior stays unchanged.

## 6. Persistence, editing and queue measurement

The next schema stores authored controls with stable IDs, parameters and reference stations.
It stores no occupancy, route slots, generated conflicts or reservations. Old schema loads
produce empty new collections and the unchanged fallback. Runtime-unsupported controls can
be saved as clearly diagnosed drafts; Save is not evidence that Run supports them.

Every control command uses `History`. Cancel or failed validation changes nothing. Successful
edits invalidate the run. Deleting an owning road cascades dependent controls and queue-line
references in the same transaction; Undo restores the entire relationship. A copy with all
owners selected remaps IDs once. Copying a control without its owning paths rejects atomically
in the first slice. A retarget or lane edit preserves identity or marks the control unresolved.

Use the same compiled measurement-line representation for signal-derived and explicit queue
counters. The evaluation layer can read segment/route positions from core contracts without
importing the model. A counter referencing a control uses that line's position; its project
file never repeats the coordinate. Deleting it cannot leave a silent zero-length queue row.

Keep the current queue hysteresis/maximum-gap catalog and approach aggregation. Empty
completed movements have null delay. Report completed, active, pending and clamps alongside
queues; complete-trip selection bias and entry acceleration are still present. Changing a
counter must not change simulated trajectories. The CLI and editor consume the same report.
