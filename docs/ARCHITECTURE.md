# ARCHITECTURE — TrafficSim

**Current stack: C++20, CMake, Qt 6 Widgets.** D15 supersedes the initial TypeScript stack.
M0 core/network functionality has been ported, with the native desktop editor and CLI.
The traffic-engineering acceptance gate remains open. M1 editing and in-editor simulation are implemented; M1 usability was accepted by owner ruling (D49); owner M0 plausibility acceptance remains open.

Project-owned Demand catalogs (schema 18) store types, behavior snapshots, composition
weights and authoring names. `resolveDemandCatalog` supplies the UI and compiler;
`putDemandCatalog` captures ownership atomically through History. Names/compositions
stay outside core; only the expanded input rates and existing vehicle contracts run.
Schema 19 adds composition periods and complete type routing matrices.
`demand_time_types` supplies validation and expansion before conditioned routing;
placed decisions key route families by type. Scheduled demand time selects weights,
including downstream decisions; source queues never reselect. Legacy compilation
order and schema-17/18 bytes stay unchanged without the new fields.

## Boundaries

The authoring network is the source of truth. `compileScenario` derives a runtime scenario;
it is never persisted as a second editable network. `createSimulation` copies and
canonicalizes that scenario into `std::shared_ptr<const Scenario>`.

D108 source insertion checks ordinary following/integration from rest against snapshot
leader clearance before removing a queued arrival. The same sampled record waits until
its first step fits the unchanged buffer; later safety phases remain authoritative.

The engine cannot access the authoring model, JSON, Qt, files, wall clocks or threads.
Each step receives a const state and returns a new value; vehicle/input/event vectors
are independent copies. Old snapshots remain intact. The `SimState&&` overloads (D91) take the
previous state over instead; hot loops step with `std::move`, consuming it. States are C++ values, not objects
with JavaScript-style deep-freeze; callers must treat published states as snapshots.

Accepted lane changes capture a snapshot-owned `LaneChangeTrace` before station remapping.
Only the Qt-free Run pose target reads it, reconstructing a spatial front guide with
continuous rear rolling, including overlapping changes and stopped vehicles. Engine
occupancy still changes instantly; scalar following/conflict/evaluation contracts stay
unchanged. See [VEHICLE_POSE.md](reference/VEHICLE_POSE.md).

| CMake target | Location | Dependencies | Status |
|---|---|---|---|
| `trafficsim_vehicle_pose` | `src/editor/vehicle_pose.*`, `rear_axle_pose.*`, `lane_change_pose.*` | Model geometry, core route/type contracts; no Qt | Route front sampler, spatial rear-axle tracks and composed lane-change guides, independently testable headless |
| `trafficsim_core` | `src/core/` | Standard C++ library only | M0 engine implemented; crossing admission (`conflicts.*`, M3.2.3a); mandatory lane changes and cooperation (`lanes.*`, M3.2.8b) — spans and dead ends arrive as data, the core never sees a lane |
| `trafficsim_model` | `src/model/network/`, `src/model/demand/` | Core contracts/validation | M0 authoring model and compiler implemented; fixed-time Signal Controllers compiled to core programs (`signal_control.*`, M2.7b); authored right-of-way controls (`control.hpp`, `right_of_way.*`, M3.2.2a); lane families and lateral spans (`routeLaneFamily` in `routing.cpp`, `lane_family.cpp`, M3.2.8b) |
| `trafficsim_eval` | `src/eval/` | Core events and states | Completed-trip diagnostic; per-movement delay/travel time and approach queues for one run (M2.5); M3.2.8c diagnostics (lane changes, segment times, stop-line discharge, arrival phases, dead-end waits) |
| `trafficsim_project` | `src/project/` | Model, evaluation types, nlohmann/json | M0 loading/output; the schema-19 authoring codec (reads schemas 1–19); evaluation spec and report output; revision run snapshots |
| `trafficsim_commands` | `src/commands/` | Project document | Atomic named edits, Undo/Redo, network, demand, control and appearance operations |
| `trafficsim_shell` | `src/shell/`, `src/editor/` | Commands, Qt Widgets | The native editor — the application's only window since M1.24 |
| `trafficsim-cli` | `tools/run_simulation.cpp` | Project/core/eval | Headless single-seed runner, JSONL export, `--project` movement report and CSV, and the M3.2.8c diagnostic flags (`--lane-changes`, `--segment-times`, `--stop-lines`, `--arrival-phases`, `--wait-causes`) |
| `trafficsim-desktop` | `src/shell/main.cpp` | Shell | Native desktop entry point; opens the editor |

Connector widths and runtime paths share topology-only `connectorLanePairs`; the surface
never calls `connectorPaths`. D114 derives each runtime lane's interior vertices from adjacent
final painted-rail midpoints, and terminal vertices from named Link lane attachments.
The compiler, canvas, heads and controls read the same path and metre length. Stored control
stations map by corresponding leg fractions; conflict coverage uses the same rails and authored
cross-sections. The cubic remains the default drawing generator. Core sees lengths/stations only.

The Run view keeps the front bumper at route distance `d`, then derives a rigid body's
rear-axle pose with a prescribed-bumper-track/no-slip model. `RearAxlePath` integrates
heading by spatial RK4, independent of ticks and paint order; the rear axle is the
scene item's origin. Optional type `axles` data has nominal old-file defaults and does
not affect traffic following or reservations. Derived paths are shared per route/type,
cleared on Run-network replacement or a different immutable Scenario snapshot. Minimum
visible size does not alter physical axle dimensions. Lane changes compose continuous
front guides and use the same rolling solution; their cache keys include each vehicle's
complete change trace/type. Between-lanes engine occupancy remains unmodelled.
See [VEHICLE_POSE.md](reference/VEHICLE_POSE.md) for the equation, defaults, numerical budget and limits.

Qt and JSON are not linked into the core. Set `TRAFFICSIM_BUILD_DESKTOP=OFF` to build
and test the engine, model and CLI on a machine without Qt.

## Contracts

```cpp
// core/simulation.hpp
SimState createSimulation(const Scenario&, std::uint32_t seed);
SimState stepSimulation(const SimState&);
SimState stepSimulation(const SimState&, double dt); // Must equal scenario.timeStep.
SimState stepSimulation(SimState&&);                 // D91: takes the previous state over
SimState stepSimulation(SimState&&, double dt);
SimState runSimulation(const Scenario&, std::uint32_t seed,
                       const EventSink& sink = {}, bool includeMovementEvents = true);

// model/network/network.hpp
Scenario compileScenario(const Network&, const ScenarioDefinition&);
std::vector<ValidationIssue> validateNetwork(const Network&);

// model/network/right_of_way.hpp (M3.2.2a): the one resolver compile and diagnostics share
RightOfWayResolution resolveRightOfWay(const Network&, const RuntimeSections&, const PriorityDefaults&);
// ... and what a crossing area's extents are checked against (M3.2.2c, conflict_coverage.cpp)
SurfaceOverlap surfaceOverlap(const Network&, const ControlPathRef&, const ControlPathRef&);
// ... and one route's chains per entry lane, full or stub, with the spans between them (M3.2.8b)
std::vector<FamilyChain> routeLaneFamily(const Network&, const std::vector<std::string>& objectIds,
                                         std::vector<std::string>* ambiguous = nullptr);
void appendLaneChanges(const Network&, const RuntimeSections&, const std::vector<FamilyRoute>&,
                       std::vector<LaneChangeSpan>&, std::vector<RouteDeadEnd>&);

// project/load.hpp
LoadedScenario loadScenario(const std::filesystem::path& file,
                            const std::filesystem::path& dataDirectory);
```

`runSimulation` invokes a synchronous `std::function<void(const SimEvent&)>` sink and
returns the final state. The core owns no output stream. The callback decides whether to
accumulate statistics, write a file or discard events. `SimEvent` is a `std::variant` of
seven typed event structs (`LaneChangeEvent` appended last, M3.2.8b). States retain only the latest tick's events.

The desktop uses a Qt timer to schedule fixed steps. Playback time never enters the
engine. The M0 workload runs on the UI thread; playback credit is capped per callback.
A future worker handoff must retain snapshots and deterministic step order. First
parallelize independent batch seeds when M5 is implemented.

`EditorCanvas` is the only surface that draws a run. It reads model geometry and runtime
snapshots and owns no edits, signals, arrival generation or simulation timer. `src/render/`
held a second, simpler `NetworkView` for the M0 harness window; M1.24 removed both. The
current renderer is a QGraphicsView/QPainter diagnostic, not a performance-tested production
renderer.

## Editor boundary

`ProjectDocument` owns the authoring network, optional typed AuthoringDefinition, background,
revision and ID counter. `History` commits a candidate only after validation and keeps
bounded before/after snapshots. Failed edits never mutate the published document.
`History::states` exposes only revision/name/save-point metadata; the shell's History dock
does not duplicate document snapshots. `restore` navigates the existing Undo/Redo stacks,
rejecting expired revisions before mutation and preserving the monotonic next-revision counter.
Keyboard nudges submit the same group-translation command as a mouse drag. Filtering a level
prunes hidden selections; deliberate object selection reveals its level through a canvas
callback that keeps the shell's filter synchronized. Active mouse gestures are cancelled on
focus loss, while multi-click drawing/connection drafts remain available between clicks.
Project never imports commands; a boundary check and negative fixtures enforce this.

`src/model/network/rotation.cpp` supplies the affected object set, surface-bounds pivot and point
transform shared by the rotation preview and command. Alt-drag and the exact-angle dialog
submit `rotateObjects` through History. Internal Connectors retain authored references under
a rigid group rotation; only affected partial attachments are re-read. Detached Connectors
use the existing deletion cascade. No preview geometry is stored, and no schema changes.

`road_crossbar.*` is an editor-only presentation helper: actual rail intersections and
cosmetic stroke/hit geometry are shared by Routes, inputs and signal heads. Run snapshots
retain head bars by ID, so the program's colour changes on the same authored stop position.
Route hover traces remain local until one existing route command commits; no schema/core change.

`EditorCanvas` renders a const document and sends gesture callbacks. Drag previews are
transient and one release submits one command. `EditorWindow` composes native actions,
inspector controls, translation, save prompts and QSaveFile atomic replacement. It is
the only window, and runs its own compiled revision. Qt stays out of
project/model/core. Embedded background bytes are immutable and shared across history.
`connectorCurve` samples the existing cubic only for the persisted drawing and its
editable interior handles. `connectorPaths` derives driving lane centres from the resulting
editable surface; no second curve is persisted. `connector_commands.hpp` defines creation, geometry, retargeting, reset and
deletion. Explicit endpoint edits share model `retargetConnector` with the canvas preview:
they rebuild the directed turn at the current intermediate-point count and narrow ranges to
available lanes. Link geometry movement uses `reanchorConnectors`; lane-bundle/driving-side
edits use named-lane anchoring. These are deliberately different operations.
Retargeting/range changes to a connector used by a route or head are rejected; deletion removes affected routes
and their inputs through shared command-side reference cleanup. No runtime merge or
right-of-way support is implied by authoring these connections.
A connector stores one base polyline plus contiguous source/target lane counts.
`connectorPaths` derives stable per-lane paths; the first retains the connector ID,
then `id/lane-2`, `id/lane-3`, etc. Compilation, heads, validation and reference
cleanup share these IDs. No derived path is persisted as a second editable object.

`AuthoringDefinition` reuses core value contracts for routes, inputs and fixed-time
programs, with explicit flags distinguishing external catalogs from embedded overrides.
`compileDocument(document, dataDirectory)` resolves catalogs once and returns a
`RunSnapshot` containing revision, network and scenario values. The shell schedules
fixed steps and clears the snapshot after a successful edit, Undo/Redo, open or seed
change. Dynamic vehicle/head scene items are ordered by their authored level.

`editor_storage` shares bounded asset decoding and atomic QSaveFile replacement
between Save and recovery. Each editor owns a UUID recovery file and a QLockFile;
restoration validates before replacing the document and starts untitled and dirty.
Schema 1 loads with default one-lane connector ranges, level 0 and default display
type. Schema 1/2 endpoint references retain their default attachments; saves without catalog extensions keep schema 17; owned compositions/names use 18, time/type rules use 19.
`Link::laneOffset` positions the lane bundle independently of its reference polyline.
`replaceLaneBundle` anchors the edge opposite the edit; model lane geometry and road
boundaries share that offset, so resizing curved roads does not move surviving lanes.
Leading Connector range edits freeze `laneBlend` weights and rebase the first path;
derived surviving lane pairs retain their curves. Schema 1–3 default to zero offset
and legacy arclength weights. Geometry edits clear frozen weights when reshaping.
Canvas Ctrl-click selection is separate from Ctrl-drag copy, committed on release.
Link/group, independently attached Connector and Signal head copies all use History.
Optional `LaneReference::station` stores metres along the link's reference polyline, so a
stretched Link does not slide what is attached part-way along it; `matchedStation` maps that
one station onto any lane or boundary derived from the same reference.
`laneAttachment` is shared by curve construction, derived paths, validation and reanchoring.
The editor's side-resize gestures submit one Link/range command on release; preview data
never enters History. `runtimeSections` derives the runtime lane sections an interior attachment needs (M1.11.1) and is
the single source `buildScenario`, head rebasing and the canvas's vehicle layer read; a lane with
nothing attached yields one section carrying the lane's own id, so uncut networks compile
unchanged. `derivedPriorityRules` arbitrates the merge an arrival creates (M3.1).
`connectorRuntimeIssues` now blocks only an attachment too close to a lane end or another
attachment to leave a section. `connectorLaneWidths` is the single place a Connector's width is
decided (M1.12.1), read by both `connectorBoundaries` for drawing and `connectorShapeIssues` for
`TIGHT_CONNECTOR_RADIUS`, which previously derived it independently. Unsupported future versions
fail before mutation.

M1.21 stores shared Link boundary markings once per boundary in lane order. `markings.cpp`
derives the solid/dashed/none/double strokes the canvas paints; paint never modifies
lane/Connector surface geometry or runtime behavior. Schema 7 rejects unsupported network
object fields instead of dropping them on save. `link_geometry_commands.cpp` adds station
insertion, midpoint, straighten and unreferenced reverse through the existing History boundary.
Model validation checks imported Connector cross-section lists, not just command inputs.
Geometry advisories have their own severity and never enter the Run/save blocking checks.

The Network lifecycle audit supplies `directionAlong` for unambiguous vertex tangents and
validation of collapsed derived lanes. D80 replaces the old slide/square-mouth correction:
`connectorCentreline` derives a central axis from the compatible stored first-lane geometry;
`connectorBodyBoundaries` offsets every lane boundary around it; `connectorSurface` attaches
those boundaries with P1–P4. Both ends pair by lane index at every angle. `connectorBoundaries`
returns the same final rails used by paint and lane handles. D114 crossing coverage uses these same final rails, so a geometry edit changes both
motion and coverage. Authored cross-section stations map onto the derived lane paths.
D109 `conflictGroups` derives connected groups for one road-owner pair and kind; commands
author/edit/delete every member atomically, while the resolver keeps individual lane-pair zones.
D115 retains the convex polygon clips in `surfaceOverlaps`; `classifiedOverlaps` labels
mouths as merge, branching or continuation without discarding their geometry. The crossing-only
`crossingOverlaps` view remains shared by Add, following and Run. `conflictAreaPolygons`
supplies authored display/picking; automatic suggestions carry the same derived polygons.
Grouping uses physical polygon contact. Branching is a read-only derived editor kind;
codec/commands reject it as an authored control. Merge runtime extents/rules stay unchanged.
Separate locations remain separate groups. D114 restores geometry-derived runtime paths;
project schema remains unchanged, with explicit authoring-to-runtime station adapters.

The surface owns its computed mouths and a `selfIntersecting` flag. Singular intersections
and folds produce `WARN_CONNECTOR_ALIGNMENT`; they never select a square cap. Open markings
on an undefined join remain selectable. Waiting-line bars use the actual rails' intersection
with the normal at the mapped runtime lane station. See
[CONNECTOR_FOUR_POINT_MOUTH.md](reference/CONNECTOR_FOUR_POINT_MOUTH.md) for the contract and
[NETWORK_EDITOR.md](reference/NETWORK_EDITOR.md) for user controls/file semantics.

## Remaining systems

M3's right-of-way seam ([M3_CONTRACT.md](reference/M3_CONTRACT.md)) has landed in slices M3.2.2a–M3.2.6:
authored waiting lines, conflict areas, priority rules (gap time/headway), Stop/Yield controls and
queue counters are project-codec objects; `resolveRightOfWay` resolves them once into runtime
conflict zones and route incidence, admitted by `core/conflicts.*`. Derived merges still run on
M3.1 `PriorityRule`s (D59).

| System | Location | Required boundary |
|---|---|---|
| Extended commands | `src/commands/` | Multi-selection and future object edits use the same transaction path |
| Extended demand/control | `src/model/demand/` | M1 typed routes/inputs/fixed-time programs exist; M2 added intervals, compositions, routing decisions with per-interval turning proportions (M2.1.1–M2.4) and fixed-time Signal Controllers (M2.7b); partial/dynamic routing (M2.1) and actuated control (M4) remain |
| Movement evaluation | `src/eval/` | One-run delay and queues exist (M2.5); LOS, multi-seed means and travel-time sections remain |
| Batch runner | `src/runner/` | Independent seeds, deterministic aggregation |
| Reports | `src/report/` | Format evaluated measurements, no new simulation logic |

Adding a command must never teach `project/` its implementation. The corresponding boundary check now runs with the M1 document/command implementation.

## Data and enforcement

- `data/scenarios/`: M0 authoring fixtures.
- `data/vehicle-types/`, `data/driver-behaviour/`: all JSON catalog files are loaded in
  filename order. Adding a new entry needs no C++ change. Explicit catalogs inside a
  scenario definition override the directory catalogs for that definition.
- `data/levels/`, `data/display-types/`: ordered levels and named styles loaded by
  `loadDisplayCatalog`. Model objects store IDs/levels; rendering owns their appearance.
  These values never alter simulation topology or conflict handling.
- `data/compositions/` (M2.3), `data/priority-rules/` (gap time and headway for derived rules),
  `data/evaluation/queue-counter.json` (queue-counter conditions), `data/vehicle-appearance/`
  (display only, D97), `data/fonts/` (the bundled UI face) and `data/projects/` (shipped example
  projects that tests and the CLI run).
- `data/locales/`: English key source and Thai UI text, copied with runtime data.
- LOS packs remain planned and must be jurisdiction-specific data, never compiled constants.

`trafficsim-check-architecture` permits literal local headers and reviewed standard
headers in core/eval. It rejects Qt, I/O, unordered containers, wall-clock/RNG headers,
macro includes and module imports, with negative fixtures. This is a restricted source
check, not a full C++ parser; review remains responsible for unconventional preprocessor
constructs. CMake target dependencies provide an additional compile/link boundary.

No engine-abstraction layer, plugin framework or second persisted model is introduced.
