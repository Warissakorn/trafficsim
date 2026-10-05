# TrafficSim

A traffic microsimulator with its own **C++20 engine, link-based network model and Qt 6
Widgets desktop interface**, built to the modelling surface PTV Vissim users already know and
aimed at the movement-level delay and queue output that traffic impact studies need.
`TrafficSim` is a working name; the name is the owner's decision (D11).

**Where it stands.** M1 (editor usability) was accepted by owner ruling (D49) and the M2 gate
passed by the owner's judgment (D51–D53). **M0 plausibility is still open**, and M3 right-of-way
and driver behaviour are under way. `trafficsim-desktop` opens the Network Editor:
- Links, Connectors and turn pockets, drawn on an aerial image.
- Routes, vehicle inputs, routing decisions and fixed-time signal controllers.
- Conflict areas with priority and Stop/Yield, and queue counters.
- Run, Pause, Step and Reset on the same canvas, then a Results tab with per-movement delay and
  per-approach queues, exportable as CSV.

The controls are in English and Thai, with a bundled Noto Sans Thai font. What is next lives in
[`docs/NEXT.md`](docs/NEXT.md); the milestone sequence is in
[`docs/ROADMAP.md`](docs/ROADMAP.md).

The supplied Link, Connector and Network Editor target specs are retained in
[`docs/specs/`](docs/specs/README.md), with a [code audit](docs/audits/SPEC_AUDIT.md). These documents
are not claims of Vissim parity.

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
| Simulation core | `src/core/simulation.hpp` | Fixed stepping, seeded arrivals, reduced car-following, fixed-time signals, conflict-area and merge right-of-way, commitment at waiting lines, mandatory lane changes with cooperative braking. Imports nothing. |
| Network and demand model | `src/model/` | Links, lanes, Connectors, routes, inputs, routing decisions, signal controllers, conflict areas and queue counters, compiled into core inputs |
| Project files | `src/project/` | Strict load and save of `*.traffic.json` (reads schemas 1–19; writes 17–19 by feature) and M0 scenario JSON, validation, movement evaluation and its CSV |
| Evaluation | `src/eval/` | Per-movement delay and per-approach queue from one run's event stream |
| Desktop | `src/shell/`, `src/editor/` | The Network Editor, Run view and Results tab, in English and Thai |
| CLI | `tools/run_simulation.cpp` | One seeded run of a scenario or project; summary JSON, optional events and `--csv` |

Out of scope today: multi-seed batches, confidence intervals and LOS (M5), and validation
against field data (M6). Read [`docs/reference/SIMULATION.md`](docs/reference/SIMULATION.md) for numerical behaviour.

## Migration evidence

The TypeScript baseline remains in Git at
[`70383db`](https://github.com/Warissakorn/trafficsim/commit/70383db6ab884c718baef97a8ab81292fdc9d1b0).
The C++ tests compare four baseline seeds: all non-movement events and complete state
checkpoints every 10 simulated seconds, with a 1e-7 absolute tolerance on physical values.
Same-build C++ replay compares the full event stream exactly.

Seed 42: 180 seconds, **31 completed, 0 active, 0 pending**, 0 safety clamps,
mean completed-trip delay **29.249359418430977 s**. This is a regression fixture, not
scientific validation or a performance benchmark. See [`docs/reference/MIGRATION.md`](docs/reference/MIGRATION.md).

## Project map

AI development starts at [AGENTS.md](AGENTS.md); read only the task-specific context
selected by the documentation map.

[`docs/README.md`](docs/README.md) lists every document by purpose. The ones to read first:

| Document | Purpose |
|---|---|
| [`docs/NEXT.md`](docs/NEXT.md) | The one live to-do |
| [`docs/PROBLEM.md`](docs/PROBLEM.md) | Audience, scope and why this owns an engine |
| [`docs/PRINCIPLES.md`](docs/PRINCIPLES.md) | Correctness and working rules |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Modules, dependencies and contracts |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | M0–M7 and acceptance gates |
| [`docs/PROGRESS.md`](docs/PROGRESS.md) | Session log and the decision record |
| [`AGENTS.md`](AGENTS.md) | Shared working instructions for humans and AI tools |
| [`CLAUDE.md`](CLAUDE.md) | Claude entry point, pointing to the shared instructions |

## Native network editor

Launch `trafficsim-desktop --language th`; the editor is what the application opens. The Network Objects sidebar provides Links, Connectors,
Routes, Vehicle inputs and Signal heads. Ctrl+right-drag creates links and connector
lane ranges; the Objects dock has demand and signal-program editing actions.

Draw a network, add a route and vehicle input, then Run (F5) in the editor. Step (F6),
Reset, seed and playback speed operate on one explicit document revision. Successful
edits invalidate that run. Save/Open preserves geometry, demand, embedded images,
levels and display types; locked recovery copies protect unsaved work.

See [the editor guide](docs/reference/NETWORK_EDITOR.md) for controls and file semantics. M1's acceptance
record is [`docs/plans/M1_ACCEPTANCE.md`](docs/plans/M1_ACCEPTANCE.md). Accepting the editor does not close M0
plausibility or engine validation.

## License

This project is released under the [MIT License](LICENSE). The bundled Noto Sans Thai
font in `data/fonts/` stays under the SIL Open Font License 1.1 (see
[`data/fonts/README.md`](data/fonts/README.md)), and Qt is used under its own terms —
see [`docs/BUILDING.md`](docs/BUILDING.md).
