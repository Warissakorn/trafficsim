# TrafficSim

A traffic microsimulator with its own **C++20 engine, link-based network model and Qt 6
Widgets desktop interface**, aimed at the modelling workflow of traffic impact studies.
`TrafficSim` remains a working name until the end of M1 (D11).

**M0 native implementation available; owner acceptance remains open.** The desktop harness
shows seeded vehicles accelerating, queueing at fixed-time signals and crossing explicit
connectors. It has English/Thai controls, Run/Pause/Step/Reset, seed and playback speed,
and read-only scenario loading. A separate Network Editor now provides undoable
Link/Lane drawing, lane-to-lane connector editing, image calibration and basic project
save/open (M1.1–M1.3 and M1.4; see below).
A bundled Noto Sans Thai font provides offline Thai text rendering.

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

## Implemented

| Part | Entry point | Behaviour |
|---|---|---|
| Network model | `src/model/network/network.hpp` | Links, lanes, explicit connectors, driving-side geometry, mid-link signal heads, validation and scenario compilation |
| Simulation core | `src/core/simulation.hpp` | Detached const scenario, value snapshots, fixed stepping, seeded arrivals, following, signals, blocked-entry queues and event streaming |
| Desktop | `src/shell/main.cpp` | Qt Widgets controls and passive 2D network view, English/Thai UI |
| Scenario loading | `src/project/load.hpp` | Strict JSON shape checks, external vehicle/behaviour catalogs and scenario validation |
| Diagnostic | `tools/run_simulation.cpp` | Single-seed run, completed-trip delay, active/pending counts and optional JSONL events |

The runtime rejects merging paths, internal sources and cyclic routes. Lane changing,
crossing conflicts, priority rules, edited-project run handoff, batch evaluation and LOS
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

M1.1–M1.3 and M1.4 provide undoable Link/Lane drawing, lane-to-lane connectors with
editable curve points, background-image calibration and basic project save/open.
Launch `trafficsim-desktop --editor` or use the Network Editor entry in the simulation
window. Choose **Connect lanes**, then a source lane end and a target lane start.
See [the editor guide](docs/NETWORK_EDITOR.md) for the Properties workflow and controls.
Signal-bearing link splits (M1.3.1), tables/diagnostics, recovery and edited-network
simulation remain open, as does the full M1 usability gate.

## License

This project is released under the [MIT License](LICENSE). The bundled Noto Sans Thai
font in `data/fonts/` stays under the SIL Open Font License 1.1 (see
[`data/fonts/README.md`](data/fonts/README.md)), and Qt is used under its own terms —
see [`docs/BUILDING.md`](docs/BUILDING.md).
