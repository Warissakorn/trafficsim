# Native baseline and migration compatibility

D15 moved the application from TypeScript to C++20, CMake and Qt Widgets. The former
application is retained at `70383db6ab884c718baef97a8ab81292fdc9d1b0`; use a separate
checkout for its npm workflow. Do not restore a second active engine into this tree.
The [historical migration reference](../archive/MIGRATION-D15.md) preserves the original
mapping and planned horizon. Current modules are mapped in [ARCHITECTURE](../ARCHITECTURE.md),
build commands in [BUILDING](../BUILDING.md), and gates in [ROADMAP](../ROADMAP.md).

## Current entry points

| Surface | Maintained implementation |
|---|---|
| Core contract | `src/core/simulation.hpp` |
| Network model | `src/model/network/network.hpp` |
| Desktop shell and canvas | `src/shell/main.cpp`, `src/shell/editor_window.cpp`, `src/editor/canvas.cpp` |
| Headless runner | `tools/run_simulation.cpp` → `trafficsim-cli` |
| Project formats | [NETWORK_EDITOR — Save, recovery and formats](NETWORK_EDITOR.md#save-recovery-and-formats) |

The numbers below describe frozen migration fixtures, not today's capacity, safety of
every network, scientific validation or product performance. New features preserve the
legacy fixtures; feature-specific contracts require their own evidence.

## Regression evidence

The baseline's 40 tests and production build passed before migration. Four immutable
fixtures in `tests/reference/` were exported from that baseline with Node/TypeScript.
They contain every non-movement event, complete vehicle and pending-input state every
100 ticks (10 seconds in the demo), and the final completed-trip diagnostic.

| Seed | Completed trips | Mean trip delay (s) | Safety clamps |
|---:|---:|---:|---:|
| 0 | 35 | 25.144689692463636 | 0 |
| 42 | 31 | 29.249359418430977 | 0 |
| 43 | 38 | 31.250496995342825 | 4 |
| 4294967295 | 36 | 35.221348280901196 | 2 |

Comparison rules:

- Event types/order, identifiers, RNG states, ticks, counts and categorical state agree.
- Physical floating-point values use an absolute tolerance of **1e-7** in their SI unit.
- Same-build C++ replay compares **every** event, including movement, exactly.
- Cross-language fixtures do not demand the old JS JSON-string SHA-256. Property order,
  number formatting and transcendental math may differ by toolchain.
- Checkpoints sample trajectories; they are not a full cross-language trace comparison.
  Dedicated tests cover red stops, discharge, source conservation, upstream tails,
  connector residual travel, zero demand and unsupported topology rejection.

The native code explicitly sequences all PRNG draws; C++ operand evaluation order must
not alter the Gaussian sampler. The simulation still uses pre-step occupancy, stable ID
ordering, integer ticks and a fixed dt. No worker threads or algorithm change was added.

## Acceptance

Technical regression checks do not establish M0 plausibility, M6 scientific validation
or owner appearance acceptance. Read the current gate records through ROADMAP/NEXT.
