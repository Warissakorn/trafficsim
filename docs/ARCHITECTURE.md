# ARCHITECTURE — TrafficSim

**Current stack: C++20, CMake, Qt 6 Widgets.** D15 supersedes the initial TypeScript stack.
M0 core/network functionality has been ported, with a native desktop harness and CLI.
The traffic-engineering acceptance gate remains open. M1.1–M1.3 and M1.4 editing are implemented; M1.3.1 and full M1 acceptance remain open.

## Boundaries

The authoring network is the source of truth. `compileScenario` derives a runtime scenario;
it is never persisted as a second editable network. `createSimulation` copies and
canonicalizes that scenario into `std::shared_ptr<const Scenario>`.

The engine cannot access the authoring model, JSON, Qt, files, wall clocks or threads.
Each step receives a const state and returns a new value; vehicle/input/event vectors
are independent copies. Old snapshots remain intact. States are C++ values, not objects
with JavaScript-style deep-freeze; callers must treat published states as snapshots.

| CMake target | Location | Dependencies | Status |
|---|---|---|---|
| `trafficsim_core` | `src/core/` | Standard C++ library only | M0 engine implemented |
| `trafficsim_model` | `src/model/network/` | Core contracts/validation | M0 authoring model and compiler implemented |
| `trafficsim_eval` | `src/eval/` | Core events | Completed-trip diagnostic only |
| `trafficsim_project` | `src/project/` | Model, evaluation types, nlohmann/json | M0 loading/output and version-1 authoring document codec |
| `trafficsim_commands` | `src/commands/` | Project document | Atomic named edits, Undo/Redo, Link/Lane/Connector operations |
| `trafficsim_shell` | `src/shell/`, `src/render/`, `src/editor/` | Commands, Qt Widgets | M0 harness and independent native editor |
| `trafficsim-cli` | `tools/run_simulation.cpp` | Project/core/eval | Headless seed runner and JSONL export |
| `trafficsim-desktop` | `src/shell/main.cpp` | Shell | Native desktop entry point |

Qt and JSON are not linked into the core. Set `TRAFFICSIM_BUILD_DESKTOP=OFF` to build
and test the engine, model and CLI on a machine without Qt.

## Contracts

```cpp
// core/simulation.hpp
SimState createSimulation(const Scenario&, std::uint32_t seed);
SimState stepSimulation(const SimState&);
SimState stepSimulation(const SimState&, double dt); // Must equal scenario.timeStep.
SimState runSimulation(const Scenario&, std::uint32_t seed,
                       const EventSink& sink = {}, bool includeMovementEvents = true);

// model/network/network.hpp
Scenario compileScenario(const Network&, const ScenarioDefinition&);
std::vector<ValidationIssue> validateNetwork(const Network&);

// project/load.hpp
LoadedScenario loadScenario(const std::filesystem::path& file,
                            const std::filesystem::path& dataDirectory);
```

`runSimulation` invokes a synchronous `std::function<void(const SimEvent&)>` sink and
returns the final state. The core owns no output stream. The callback decides whether to
accumulate statistics, write a file or discard events. `SimEvent` is a `std::variant` of
six typed event structs. States retain only the latest tick's events.

The desktop uses a Qt timer to schedule fixed steps. Playback time never enters the
engine. The M0 workload runs on the UI thread; playback credit is capped per callback.
A future worker handoff must retain snapshots and deterministic step order. First
parallelize independent batch seeds when M5 is implemented.

`NetworkView` reads model geometry and runtime snapshots. It owns no edits, signals,
arrival generation or simulation timer; Qt repaints only when data/exposure changes.
The current renderer is a QPainter diagnostic, not a performance-tested production
renderer or the M1 editor.

## Editor boundary

`ProjectDocument` owns the authoring network, optional M0 definition, background,
revision and ID counter. `History` commits a candidate only after validation and keeps
bounded before/after snapshots. Failed edits never mutate the published document.
Project never imports commands; a boundary check and negative fixtures enforce this.

`EditorCanvas` renders a const document and sends gesture callbacks. Drag previews are
transient and one release submits one command. `EditorWindow` composes native actions,
inspector controls, translation, save prompts and QSaveFile atomic replacement. It is
independent from the M0 simulation window; run handoff remains M1.7. Qt stays out of
project/model/core. Embedded background bytes are immutable and shared across history.
`connectorCurve` in the model returns a sampled cubic aligned to the endpoint lane
directions. Only its polyline is persisted; its interior points are the editable curve
handles. `connector_commands.hpp` defines creation, geometry, retargeting, reset and
deletion. Link/Lane/driving-side edits use the same `reanchorConnectors` path.
Retargeting a connector used by a route is rejected; deletion removes affected routes
and their inputs through shared command-side reference cleanup. No runtime merge or
right-of-way support is implied by authoring these connections.
See [NETWORK_EDITOR.md](NETWORK_EDITOR.md) for user controls and file semantics.

## Remaining systems

| System | Location | Required boundary |
|---|---|---|
| Extended commands | `src/commands/` | Multi-selection and future object edits use the same transaction path |
| Extended editor | `src/editor/` | Connector tools implemented; tables and diagnostics remain |
| Project persistence | `src/project/` | Versioned authoring file, transactions and revisions |
| Editable demand/control | `src/model/demand/`, `src/model/control/` | Model data compiles into core contracts |
| Movement evaluation | `src/eval/` | Events to delay, LOS, queues and travel times |
| Batch runner | `src/runner/` | Independent seeds, deterministic aggregation |
| Reports | `src/report/` | Format evaluated measurements, no new simulation logic |

Adding a command must never teach `project/` its implementation. The corresponding boundary check now runs with the M1 document/command implementation.

## Data and enforcement

- `data/scenarios/`: M0 authoring fixtures.
- `data/vehicle-types/`, `data/driver-behaviour/`: all JSON catalog files are loaded in
  filename order. Adding a new entry needs no C++ change. Explicit catalogs inside a
  scenario definition override the directory catalogs for that definition.
- `data/locales/`: English key source and Thai UI text, copied with runtime data.
- LOS packs remain planned and must be jurisdiction-specific data, never compiled constants.

`trafficsim-check-architecture` permits literal local headers and reviewed standard
headers in core/eval. It rejects Qt, I/O, unordered containers, wall-clock/RNG headers,
macro includes and module imports, with negative fixtures. This is a restricted source
check, not a full C++ parser; review remains responsible for unconventional preprocessor
constructs. CMake target dependencies provide an additional compile/link boundary.

No engine-abstraction layer, plugin framework or second persisted model is introduced.
