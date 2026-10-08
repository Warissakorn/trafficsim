# Driving behaviour assignment contract

Design before implementation, 2026-10-06 (D120). This contract owns the proposed
M3.3 assignment seam, not a claim that W74/W99, motorcycles or these UI controls
already run. §7 records what M3.3.2a implemented (storage and editing), §8 the
M3.3.2b runtime selection and §9 the M3.3.2c editor. [ROADMAP](../ROADMAP.md#m33--driving-behaviour-library-and-models)
owns milestone status; [the delivery plan](../plans/DRIVING_BEHAVIOUR.md) owns
acceptance rows. Existing [SIMULATION](SIMULATION.md),
[M3_8_CONTRACT](M3_8_CONTRACT.md) and [POSITIONED_ROUTING](POSITIONED_ROUTING.md)
remain authoritative for the shipped engine.

## 1. Ownership and first implementation

The first implementation selects the **existing prototype** by physical road
context. W74/W99 require their own equations, transition/state/sampling contracts
and tests before implementation. Do not rename the prototype or accept CC fields
as aliases of its parameters. No model-independent engine adapter is proposed.

| Authored value | Owner and meaning |
|---|---|
| Driving behaviour | Project-owned ID/name, explicit following-model tag and its supported parameters; reuse existing catalog ownership rather than another catalog store |
| Vehicle type | Physical dimensions, speed distribution and acceleration/deceleration limits; retains legacy `behaviourId` fallback |
| Vehicle class | Named grouping of vehicle types for assignment; a type belongs to at most one class in the first slice; membership is stored once |
| Link behaviour type | Default behaviour ID and zero or one override per vehicle class |
| Road assignment | Optional behaviour-type ID on a Link or Connector; absence means legacy fallback |

The first slice has whole-Link/whole-Connector assignments. Lane overrides, station
subranges and time-varying assignments are deferred. Class membership does not
alter demand, PCU conversion or physical limits; a class is not a vehicle type.

Resolution order:

1. On an assigned road, use its behaviour type's matching class override.
2. Without a matching override, use that behaviour type's default.
3. On an unassigned road, use `VehicleType.behaviourId` exactly as today.

A missing referenced ID, duplicate class membership/override, unknown model or
invalid parameter rejects the candidate/load atomically. It is **not** a silent
fallback. A default is required on every behaviour type, including unused ones.
No override means inheritance; an empty string is not a valid referenced ID.

Duplicate creates a new independent ID and owned values. Editing a shared set
changes its users in one History transaction; the UI shows those users first.
Delete a referenced set/class/behaviour type only through an explicit validated
reassignment transaction, otherwise reject. Cancel/no-op changes no revision.
Successful edits invalidate the existing run through the normal document seam.

## 2. Compiler and runtime interface

The compiler resolves authoring IDs and road ownership once. Runtime inputs are
numeric segment/type-to-behaviour assignments, never Link/Connector objects,
polygons, file names or Qt types. All sections of one road inherit its assignment;
each derived Connector path inherits the Connector's own assignment, independently
of its attached Links. Shared physical segments cannot receive conflicting values
from different expanded routes. Report that inconsistency instead of guessing.

Proposed seam (names/layout to be finalized with failure-first tests):

```cpp
// model/compiler result, after references and model capabilities are validated
struct SegmentBehaviourAssignment {
    std::size_t segmentIndex;
    std::vector<std::size_t> behaviourByType; // parallel to canonical vehicleTypes
};
// core reads immutable Scenario/ScenarioIndex and the vehicle's physical location
std::size_t effectiveBehaviour(const Scenario&, const ScenarioIndex&,
                              const Vehicle&);
```

Canonicalize/remap segment, type and behaviour slots together. Omit the new table
entirely when no assignment is authored: preserve `behaviourOfType`, compilation
order, legacy RNG draw order and frozen event/checkpoint comparisons. A compiled
table is derived data, never another persisted editable network. Choose sparse
or dense storage by measured scale; do not build a generic cache in this slice.

## 3. Tick, routing and boundaries

For the prototype assignment slice, choose behaviour from the vehicle **front**
on its current physical segment at the pre-motion snapshot. At an exact internal
segment boundary, the downstream segment owns the front; at the route sink,
use the last segment while the vehicle still exists. Specify named numerical
tolerances and cover both sides of equality before code.

All consumers use this effective selection: following, source first-step
clearance, lane-change own/trailing checks, courtesy, stop service and the physical
standstill-buffer constraints that currently read a behaviour. A trailing check
uses the follower's own effective set, not the changer's. Stop service at an
unchanged line must not reset merely because the selected set changes.

Source candidates use the entry segment and retain the queued sampled vehicle,
ID, scheduled time and RNG state while waiting. Never redraw desired speed or
driverFactor when an assignment is resolved.

Recompute effective references after an accepted lane change or routing change,
where the current engine already rebuilds physical references. A suffix-only
change on an identical current prefix cannot change the current effective set.
D119 source-zero recognition occurs before lane-change/motion selection; its
crossing-tick cap and next-tick suffix checks remain unchanged. Passed decision
IDs and routing events stay owned by the copied state.

A prototype profile boundary does not itself introduce a movement cap. A tick
crossing one or several boundaries uses the pre-motion selection for that tick;
the next snapshot selects the new segment's set. Document up-to-one-timestep
selection lag and measure timestep sensitivity, especially on short Connectors.
Do not claim continuous within-tick switching. Same-set boundaries must leave
the legacy trajectory unchanged, including section cuts and positioned routing.

Vehicle identity, sampled desired speed, driverFactor and physical limits survive
a change of profile. New behavioral preferences must still compose with signal,
conflict, body-spacing and emergency constraints. Do not weaken physical checks
to make a newly selected profile appear smoother.

Switching following **models** is deferred until each model defines its state
initialization/transfer and driver traits; [W74](W74.md#7-state-and-model-switch)
defines them for `w74` (D129, not yet implemented). All future dynamic/attention/amber
state belongs in `SimState`; sampling cadence and stable processing order must
be explicit. Copy/replay never depends on paint order or wall clock.

## 4. Parameter capability map

This table describes baseline code at `07eb5cb`, not scientific validation.
"Planned" means the attached target needs new contracts/implementation; it must
not appear as a working setting just because the catalog can store a number.

| Target from owner design | Shipped capability / required work |
|---|---|
| DB_01–DB_07 library, Duplicate, region assignment | Owned behaviours exist; named library workflows/class/road assignment are M3.3 work |
| W74 ax, bx_add, bx_mult | Similar prototype fields exist; full W74 dynamics and traits are contracted in [W74](W74.md) (D129), not implemented |
| W99 CC0–CC9 | Planned following model; none are aliases of `followingTime` or other prototype fields |
| Desired speed / type length and width | Existing type fields; additional distribution families and motorcycle occupation are separate work |
| Transit acceleration CC8/CC9 | Type has scalar acceleration limits; speed-dependent model acceleration needs a contract; no transit-stop/articulation claim |
| Look ahead/back min/max; observed vehicles | Planned bounded perception; do not limit hard collision constraints with perception |
| Temporary attention duration/probability | Planned seeded state and cadence; copied replay and emergency behavior required |
| Smooth closeup; static-obstacle standstill gap | Planned obstacle-specific semantics; do not change ordinary vehicle buffer globally |
| Free lane selection | D95/D101 implemented but off under D102; acceptance A53 remains failed |
| Maximum/accepted deceleration own/trailing | Type limits and limited current checks exist; complete behavior-specific curves/thresholds planned |
| Deceleration distance (own/trailing) | Planned distance-dependent lane-change contract |
| Waiting time before diffusion | No vehicle-deletion feature booked; keep waiting vehicles/accounting and diagnostic reason |
| Minimum lane-change headway front/rear | Existing following-based checks are not this separate target parameter |
| Lane-change safety-distance reduction | Planned; never overrides physical overlap protection |
| Maximum cooperative braking | Existing optional positive-magnitude `maxDecelerationCooperativeBraking`; mapping is partial, not Vissim parity |
| Cooperative lane change; speed difference/collision time | Cooperation by changing lane is planned separately from braking |
| Advanced merging | Existing merge arbitration is not this complete behavior model |
| Rear lateral correction | Rear-axle display exists; physical lateral correction/occupancy not implemented |
| Desired lateral position; adjacent-lane observation | Planned lateral/perception model |
| Diamond queue; same-lane left/right overtaking | Planned 2D occupancy and motorcycle behavior |
| Turning-direction lateral placement | Planned; must respect D119 recognition rather than read an unselected suffix |
| Lateral distance at 0/50 km/h | Planned speed-dependent clearance; lane/type widths already exist |
| Collision-time gain; minimum longitudinal speed | Planned lateral maneuver decision rules |
| Time between lateral direction changes | Planned; not D101's discretionary hold parameter |
| Amber continuous check / one decision | Amber is red today; new signal behavior and state contract required |
| Amber alpha, beta1, beta2 | Apply to One decision in the referenced PTV semantics, not Continuous check |
| Red-amber Go | Planned distinct signal state/program; never means ordinary red permits travel |
| Signal reduced-gap factor/start/end distances | Planned local obstacle/following semantics |
| Front gap / Rear gap | Existing `gapTime` is a different deterministic threshold; separate Crossing/Merge semantics required |
| Conflict safety-distance factor | Planned Merge-specific behavior; never confuse with lane-change/signal reduction |
| Added gap at stop | Target ambiguous: define time margin/dwell independently of additional stop distance in metres |
| Anticipate routes / adjacent vehicles at conflict | Planned visibility/inference; no knowledge of unselected D119 routes |

Core distances/times/speeds/accelerations remain m/s-based. Deceleration settings
are positive magnitudes internally; UI signed conventions convert once at the
boundary. Unknown/unsupported model fields are rejected, never silently ignored.

## 5. Geometry, safety and calibration boundaries

Display bands/insets do not define physical admission. M3.2.4e must use physical
lane/path overlap and authored-to-runtime station adapters. Crossing clearance
uses front entry/rear exit; Merge needs compatible longitudinal sharing and
receiving-space rules, not a blanket exclusive polygon lock. Existing solver
behavior is unchanged until that separate contract and failure-first cases land.
Scalar vehicle-length clearance does not prove rigid swept-body or trailer safety.

Suggested Thai presets are experimental inputs, not calibrated defaults. The
CC1 capacity approximation requires speed/vehicle length and is not a guarantee
of saturation flow. PCU/h and vehicles/h require separate reporting and an
explicit conversion pack. Choose field/benchmark sources, measurement windows
and tolerances before reading the simulation's fit. 3D inspection is outside
this behavior slice and cannot substitute for numeric safety/validation tests.

Until M6 passes, retain the validation marker and disclose that parameters
calibrated in Vissim do not transfer automatically to this engine.

## 6. Source semantics

The owner supplied the target design in this conversation; its suggested values
are not empirical evidence. The public PTV pages explain target vocabulary, not
the proprietary implementation or a TrafficSim equivalence claim:

- [Driving behavior sets and class/road assignments](https://cgi.ptvgroup.com/vision-help/VISSIM_2026_EN-DE/en-us/Content/4_BasisdatenSim/Fahrverhaltensparameter_def.htm)
- [Signal behavior, One decision and red-amber](https://cgi.ptvgroup.com/vision-help/VISSIM_2026_EN-DE/en-us/Content/4_BasisdatenSim/FahrverhaltensparameterLichtsignalanlagen.htm)
- [Conflict types and driver planning](https://cgi.ptvgroup.com/vision-help/VISSIM_2026_EN-DE/en-us/Content/5_Netzbearbeiten/Konfliktflaechen_modellieren.htm)

## 7. Implemented library and codec (M3.3.2a, D126)

M3.3.2a stores and edits §1 in the project file and History; M3.3.2b (§8) compiles
and runs the assignments, replacing 2a's interim Run refusal. A library with no
assigned road leaves the compiled Scenario unchanged. The editor is §9.

Schema 21 is written only when a library feature is used; otherwise the previous
version rule and bytes are kept. A file below 21 carrying any key below is refused
(`EDIT_UNSUPPORTED_FIELD`), never read and dropped. From 21, every owned behaviour,
vehicle type, class, behaviour type and override rejects unknown keys.

| Location | Key | Meaning |
|---|---|---|
| `definition.behaviours[]` | `model` | Required from 21: `"prototype"`, or `"w74"` from schema 22 ([W74 §9](W74.md#9-persistence), D131). Anything else or absent → `UNSUPPORTED_BEHAVIOUR_MODEL` |
| `definition.behaviours[]` | `name` | Optional display name |
| `definition.vehicleClasses[]` | `id`, `name`, `vehicleTypeIds` | A type is in at most one class |
| `definition.linkBehaviourTypes[]` | `id`, `name`, `defaultBehaviourId`, `overrides[] {classId, behaviourId}` | Default required, at most one override per class |
| `network.links[]`, `network.connectors[]` | `behaviourType` | Optional behaviour-type id; absent means legacy `VehicleType.behaviourId` |

The library requires project-owned `behaviours` and `vehicleTypes`
(`EXTERNAL_BEHAVIOUR_CATALOG`; commands `EDIT_EXTERNAL_CATALOG`), so a file is
portable. Load and History reject atomically, unused entries included, with
`INVALID_ID`, `DUPLICATE_ID`, `UNKNOWN_VEHICLE_TYPE`, `DUPLICATE_CLASS_MEMBERSHIP`,
`MISSING_DEFAULT_BEHAVIOUR`, `UNKNOWN_BEHAVIOUR`, `UNKNOWN_VEHICLE_CLASS`,
`DUPLICATE_OVERRIDE` and `UNKNOWN_BEHAVIOUR_TYPE`. An empty referenced id is invalid.

`src/commands/behaviour_commands.hpp` provides put, duplicate, assign, users and
delete for behaviours, classes and behaviour types. Duplicate creates
`<id>-copy[-n]` with independent values; a class copy has no members. A referenced
entry is deleted only with an explicit replacement, rewritten everywhere in the
same transaction (`EDIT_REFERENCED_*`, `EDIT_INVALID_REPLACEMENT`); one Undo
restores it. Evidence: [behaviour-library](../evidence/behaviour-library.md).

## 8. Implemented runtime selection (M3.3.2b, D127)

`compileBehaviourAssignments` (project layer) turns road assignments into
`ScenarioDefinition::segmentBehaviours`: one `{segmentId, vehicleTypeId, behaviourId}`
per runtime segment of an assigned road and per vehicle type — the type's class
override, else the behaviour type's default. Every section of a Link (section cuts
included) inherits the Link's assignment; every lane path of a Connector inherits
the Connector's own. Each segment has exactly one owner, so expanded routes cannot
disagree. The table holds ids, because `createSimulation` re-sorts segments, types
and behaviours; `buildScenarioIndex` resolves it once into a dense segment × type
slot table, built only when the table is non-empty. `validateScenario` reports
`UNKNOWN_SEGMENT`, `UNKNOWN_VEHICLE_TYPE`, `UNKNOWN_BEHAVIOUR` and a repeated
segment/type pair (`DUPLICATE_ID`).

`effectiveBehaviour(index, vehicle)` (declared in `core/types.hpp`) is the one
selection. With no table it is `behaviourOfType[type]`, so unassigned runs keep
their exact slots, operation order and random draws. Otherwise it reads the
segment of the part holding the **front** on the vehicle's current route
(`frontPart`, shared with `locateVehicle`): a front exactly at a join is on the
downstream segment, at or past the route end on the last one; no tolerance. The
rear's position never matters.

Consumers: `resolveRefs` (following, courtesy and cooperative braking, own and
trailing lane-change checks — the trailing check reads the follower's own refs —
discretionary changes, Stop service, receiving space) and source insertion, which
evaluates the entry segment for the waiting pending vehicle without redrawing it.
References are rebuilt where the engine already rebuilds them: after source-zero
routing and after accepted lane changes, so a new road's set is used from the next
snapshot that sees the front there, up to one timestep after the crossing.
Positioned routing (D119) is applied before that snapshot; an unselected suffix is
never read. The evaluation diagnostics (`lane_changes`, `dead_end_waits`) use the
same function. Zone chaining's waiting room takes the largest standstill among the
legacy and assigned behaviours of each type. Physical safety checks are unchanged.

## 9. Implemented editor (M3.3.2c, D128)

**Library dialog.** *Driving behaviours* on the Inputs toolbar, beside the demand
catalog, opens three tabs: Behaviours, Vehicle classes and Link behaviour types,
each with Add, Duplicate, Edit and Delete. Every button edits a staged copy of the
document through `behaviour_commands`; nothing reaches History until Confirm.
Confirm dry-runs `validateDocument` and keeps the dialog open with the reason when
the staged library is invalid; otherwise the whole session commits as one History
step (`editorBehaviourLibrary`). Cancel discards it, and Confirm without an edit
makes no revision. A project still using the installed catalogs has its behaviours
and vehicle types captured into the staged copy (compositions are not touched), so
that capture is part of the same step and one Undo restores external ownership.

The behaviour editor shows who uses the set before any change (users first) and
labels every parameter with its project-file key, untranslated; optional parameters
have a checkbox (absent = not used). A class lists every vehicle type; a type already
in another class is shown but cannot be ticked. A behaviour type has a default and
one override choice per class, where *Inherit* means no override. Deleting an entry
that something uses asks for a replacement; Cancel there keeps the entry.

**Road assignment.** The inspector's *Behaviour type* choice applies to every
selected Link and Connector in one History step (`editorApplyBehaviourType`) — the
one property edit that acts on the whole selection. *Inherit* clears the assignment;
an id with no behaviour type is listed as missing. Below it, the primary road's
effective behaviour per vehicle type and its source (class override, behaviour type
default, or inherited from the vehicle type) come from `effectiveRoadBehaviours`,
the function the compiler uses (§8), so the display cannot disagree with the run.
Assignment is disabled while the catalogs are external. EN and TH texts are in
`data/locales`; the owner's visual review of the dialog is a separate gate.
