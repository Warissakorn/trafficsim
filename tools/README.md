# Native tools guide

All executable project tools are C++20. Run build commands from the repository root.
See `../docs/BUILDING.md` for Windows/Qt setup; no Node.js or Python is needed.

## Setup and quick reference

```bash
cmake --preset headless
cmake --build --preset headless
cmake --build build/headless --target check
```

| Source | Executable | Purpose |
|---|---|---|
| `run_simulation.cpp` | `trafficsim-cli` | Run a seeded M0 scenario without a window |
| `check_architecture.cpp` | `trafficsim-check-architecture` | Check core/eval include boundaries and forbidden nondeterministic APIs |
| `check_file_sizes.cpp` | `trafficsim-check-file-sizes` | Enforce the source-file line budget |

The executables are under `build/headless/bin/` or `build/desktop/bin/` for the Ninja
presets. On Windows add `.exe`. Multi-configuration generators add the configuration
name after `bin/`.

## Simulation runner

```bash
./build/headless/bin/trafficsim-cli
./build/headless/bin/trafficsim-cli 42
./build/headless/bin/trafficsim-cli --seed 123
./build/headless/bin/trafficsim-cli --scenario data/scenarios/crossing.json --data-dir data
./build/headless/bin/trafficsim-cli 42 --events run-42.jsonl
./build/headless/bin/trafficsim-cli --help
```

| Argument | Meaning | Default |
|---|---|---|
| Positional seed or `--seed N` | Unsigned integer 0–4294967295; specify once | 42 |
| `--scenario FILE` | M0 authoring network plus scenario definition | `data/scenarios/crossing.json` beside executable |
| `--data-dir DIR` | Vehicle/behaviour catalogs; desktop also reads locales here | Data beside executable, then working directory |
| `--events FILE` | Stream all events as JSON Lines | No trajectory file |
| `--help` | Usage without loading assets or running a simulation | — |

Stdout contains a JSON diagnostic; errors go to stderr with a nonzero exit code.
Invalid/overflow/fractional seeds are rejected. Missing files, wrong JSON field types,
unknown references, off-grid timing and unsupported topology fail before stepping.
`--events` requires a new filename and will not overwrite an existing file.

The loaded scenario can contain explicit `definition.vehicleTypes` and
`definition.behaviours` arrays. Otherwise every `.json` file in the corresponding
catalog directory is loaded in filename order. Adding the 50th vehicle type needs no
C++ edit. The authoring fixture format is documented in `../docs/SIMULATION.md`.

Seed 42 outputs 31 completed trips, 0 active, 0 pending, mean delay
29.249359418430977 seconds and 0 safety clamps after 180 simulated seconds.
This is an unvalidated completed-trip diagnostic, **not HCM control delay or LOS**.

| Output field | Interpretation |
|---|---|
| `validation` | Always `not-yet-validated` in M0 |
| `engineVersion` | Native engine generation; store with future report runs |
| `compiler` | Compiler identifier/version used to build the executable |
| `seed`, `time` | Input seed and final simulated time |
| `completed` | Vehicles whose front reached the route sink |
| `active` | Vehicles still in the network |
| `pending` | Sampled arrivals still waiting outside source lanes |
| `meanTravelTime` | Mean in-network time for completed trips, or null |
| `meanDelay` | In-network time plus source wait minus desired-speed reference, or null |
| `safetyClamps` | Numerical caps that prevented longitudinal overlap/stop-line crossing |

Event files may become large. Without `--events`, movement events are filtered at the
sink and no trajectory history is retained. The state still contains latest-step events.

## Architecture and size checks

```bash
./build/headless/bin/trafficsim-check-architecture .
./build/headless/bin/trafficsim-check-architecture --self-test
./build/headless/bin/trafficsim-check-file-sizes .
```

The architecture checker permits only literal same-directory headers and a reviewed
standard-library include list in core/eval; eval additionally reads the core event
contract. It rejects Qt, I/O, random-device/clock/unordered APIs, macro includes and
module imports. Negative fixtures ensure obvious forbidden imports fail. This is a
restricted source scanner, not a C++ AST parser; source review covers nonstandard
preprocessor forms. Do not expand the allowlist to hide a boundary violation.

The size checker scans C++ headers/sources, Markdown and CMake, skips build/output and
hidden directories, and fails above 500 lines.
Exit 0 means pass, 1 means a violation; command misuse returns 2.

## Tests and daily workflow

```bash
cmake --build --preset headless
ctest --preset headless
ctest --test-dir build/headless -R reference --output-on-failure
./build/headless/bin/trafficsim-tests core
cmake --build build/headless --target check
```

CTest covers core/network semantics, strict project input, four saved TS baselines,
architecture checks and CLI boot/errors. A desktop build adds controls, translation,
full-run agreement and failed-load preservation checks using Qt's offscreen platform.
Tests use explicit checks and remain active in Release builds.

Before committing: native build green, appropriate CTest suites green, source checks
green, `docs/PROGRESS.md` updated. Automated regression does not replace owner M0 review.

| Symptom | Action |
|---|---|
| CMake cannot find nlohmann/json | Install its development package or set `TRAFFICSIM_JSON_INCLUDE_DIR` |
| CMake cannot find Qt Widgets | Select the correct kit with `CMAKE_PREFIX_PATH`, or use the headless preset |
| Windows reports missing Qt DLLs | Add the matching Qt `bin` to PATH or run `windeployqt` |
| Cannot locate data | Keep `data/` beside executable or pass `--data-dir` |
| Baseline mismatch | Inspect the first differing event/checkpoint; do not regenerate fixtures to hide it |
| `UNSUPPORTED_MERGE` | M0 cannot arbitrate merging streams; right-of-way remains M3 |

## Editor regression coverage

The `editor` native test group covers ProjectDocument and command transactions without
Qt. The `editor-ui` CTest entry exercises the Qt editor through mouse/keyboard actions,
including saving, images and calibration. The architecture guard additionally rejects
project imports of commands/editor/shell/render and Qt, with negative fixtures.
The `connectors` native group covers lane mappings, editable curves, endpoint
maintenance, atomic failures and route/input preservation. The `connector-ui` CTest
entry exercises lane-end gestures, curve-point editing, ID/Properties controls,
reference-safe deletion/Undo, persistence and English/Thai feedback. Both UI entries
require a desktop build; headless results do not verify them.
See [NETWORK_EDITOR.md](../docs/NETWORK_EDITOR.md) for the editing workflow and limits.
