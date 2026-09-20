# TrafficSim

A traffic microsimulator with its own **C++20 engine, link-based network model and Qt 6
Widgets desktop interface**, aimed at the modelling workflow of traffic impact studies.
`TrafficSim` remains a working name until the end of M1 (D11).

**M0 native implementation available; owner acceptance remains open.** The desktop harness
shows seeded vehicles accelerating, queueing at fixed-time signals and crossing explicit
connectors. It has English/Thai controls, Run/Pause/Step/Reset, seed and playback speed,
and read-only scenario loading. The Network Editor provides undoable
Link/Lane drawing, connector lane ranges, image calibration, typed demand and signal
editing, project recovery, and simulation on the same canvas (M1 implementation).
The owner's timed M1 acceptance exercise remains open.
A bundled Noto Sans Thai font provides offline Thai text rendering.

The supplied Link, Connector and Network Editor target specs are retained in
[`docs/specs/`](docs/specs/README.md), with a [code audit](docs/SPEC_AUDIT.md).
[M1.21](docs/AUTHORING_EXTENSIONS.md) adds Link geometry actions, shared boundary markings,
stricter imported cross-section validation and schema-7 persistence. The remaining target
features are numbered in the roadmap; these documents are not claims of Vissim parity.

**Not yet validated:** the longitudinal model is a reduced Wiedemann-inspired prototype,
not W74/W99 or a calibrated Vissim equivalent. Diagnostics are completed-trip delay,
including source waiting and acceleration; they are **not HCM control delay or LOS**.

## Build and run

Requires a C++20 compiler, CMake 3.24+, Ninja and nlohmann/json 3.11+.
The desktop additionally requires Qt 6.4+ Widgets (Qt Test when tests are enabled).
There is no Node.js, npm, browser, Python or server dependency in the application.

Ubuntu 24.04 development setup:

```bash
sudo apt-get install g++ cmake ninja-build nlohmann-json3-dev qt6-base-dev
cmake --preset desktop
cmake --build --preset desktop
ctest --preset desktop
./build/desktop/bin/trafficsim-desktop
```

Windows instructions for Qt/Visual Studio and dependency paths are in
[`docs/BUILDING.md`](docs/BUILDING.md). Qt must match your compiler and architecture.

Headless build, with **no Qt dependency**:

```bash
cmake --preset headless
cmake --build --preset headless
ctest --preset headless
./build/headless/bin/trafficsim-cli 42
cmake --build build/headless --target check
```

Data is copied beside the executables at build time. Both programs can start from a
different working directory. For a custom data installation use `--data-dir <directory>`;
for another M0 fixture use `--scenario <file.json>`. The desktop accepts `--language th`.
See [`tools/README.md`](tools/README.md) for all CLI and developer-tool commands.

### Prebuilt binaries from CI

`.github/workflows/native.yml` builds and tests every push and pull request.
`.github/workflows/package.yml` produces downloadable builds for manual testing: open
**Actions → Package binaries → Run workflow** (or push a `v*` tag), then download the
`trafficsim-linux-x86_64` or `trafficsim-windows-x64` artifact. Each archive contains
`bin/trafficsim-desktop`, `bin/trafficsim-cli`, the `data/` catalogs and a `RUN.txt`.
The Windows build bundles its Qt runtime; the Linux build needs Qt 6 Widgets installed
(`sudo apt install libqt6widgets6`). These are unsigned test builds, not a release
installer — M7 owns installation.

## Implemented

| Part | Entry point | Behaviour |
|---|---|---|
| Network model | `src/model/network/network.hpp` | Links, lanes, explicit connectors, driving-side geometry, mid-link signal heads, validation and scenario compilation |
| Simulation core | `src/core/simulation.hpp` | Detached const scenario, value snapshots, fixed stepping, seeded arrivals, following, signals, blocked-entry queues and event streaming |
| Desktop | `src/shell/main.cpp` | Qt Widgets controls and passive 2D network view, English/Thai UI |
| Scenario loading | `src/project/load.hpp` | Strict JSON shape checks, external vehicle/behaviour catalogs and scenario validation |
| Diagnostic | `tools/run_simulation.cpp` | Single-seed run, completed-trip delay, active/pending counts and optional JSONL events |

The runtime rejects merging paths, internal sources and cyclic routes. Lane changing,
crossing conflicts, priority rules, batch evaluation and LOS
are future milestones. Read [`docs/SIMULATION.md`](docs/SIMULATION.md) for numerical behaviour.

## Migration evidence

The TypeScript baseline remains in Git at
[`70383db`](https://github.com/Warissakorn/trafficsim/commit/70383db6ab884c718baef97a8ab81292fdc9d1b0).
The C++ tests compare four baseline seeds: all non-movement events and complete state
checkpoints every 10 simulated seconds, with a 1e-7 absolute tolerance on physical values.
Same-build C++ replay compares the full event stream exactly.

Seed 42: 180 seconds, **31 completed, 0 active, 0 pending**, 0 safety clamps,
mean completed-trip delay **29.249359418430977 s**. This is a regression fixture, not
scientific validation or a performance benchmark. See [`docs/MIGRATION.md`](docs/MIGRATION.md).

## Project map

| Document | Purpose |
|---|---|
| [`docs/PROBLEM.md`](docs/PROBLEM.md) | Audience, scope and why this owns an engine |
| [`docs/PRINCIPLES.md`](docs/PRINCIPLES.md) | Correctness and working rules |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Modules, dependencies and contracts |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | M0–M7 and acceptance gates |
| [`docs/PROGRESS.md`](docs/PROGRESS.md) | Decisions, log and next task |
| [`CLAUDE.md`](CLAUDE.md) | Working instructions |

## Native network editor

Launch `trafficsim-desktop --editor --language th` or use the Network Editor entry
in the simulation window. The Network Objects sidebar provides Links, Connectors,
Routes, Vehicle inputs and Signal heads. Ctrl+right-drag creates links and connector
lane ranges; the Objects dock has demand and signal-program editing actions.

Draw a network, add a route and vehicle input, then Run (F5) in the editor. Step (F6),
Reset, seed and playback speed operate on one explicit document revision. Successful
edits invalidate that run. Save/Open preserves geometry, demand, embedded images,
levels and display types; locked recovery copies protect unsaved work.

See [the editor guide](docs/NETWORK_EDITOR.md) for controls and file semantics, and
[the acceptance exercise](docs/M1_ACCEPTANCE.md) for the remaining owner gate.
M1 implementation does not close M0/M1 owner acceptance or engine validation.

## License

This project is released under the [MIT License](LICENSE). The bundled Noto Sans Thai
font in `data/fonts/` stays under the SIL Open Font License 1.1 (see
[`data/fonts/README.md`](data/fonts/README.md)), and Qt is used under its own terms —
see [`docs/BUILDING.md`](docs/BUILDING.md).
