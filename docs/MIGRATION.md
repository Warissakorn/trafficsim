# C++ migration — D15

The owner requested changing the whole application to C++, then authorized the project
adjustment. This supersedes D3's initial TypeScript stack and D4's web-first development
loop. It does not expand M0 into the M1 editor or close any traffic-engineering gate.

## Scope and mapping

| Previous entry | Native replacement |
|---|---|
| `src/core/index.ts` | `src/core/simulation.hpp` and typed C++ events |
| `src/model/network/index.ts` | `src/model/network/network.hpp` |
| `src/main.ts` and HTML/CSS | `src/shell/main.cpp`, `main_window.cpp` |
| Browser canvas | `src/render/network_view.cpp` using QPainter |
| `tools/run-simulation.ts` | `tools/run_simulation.cpp` → `trafficsim-cli` |
| TypeScript AST boundary guard | C++ restricted-include checker plus isolated CMake targets |
| Python size checker | `tools/check_file_sizes.cpp` |
| npm/Vite/Vitest | CMake/Ninja/CTest and C++ test executables |

The former application is retained in Git history at commit
`70383db6ab884c718baef97a8ab81292fdc9d1b0` (main before migration). Use a separate checkout
of that commit for the old `npm ci`, `npm test`, `npm run dev` workflow. Do not restore
a second active engine or parallel model store into this tree.

All authored data stays in JSON. Locale files moved to `data/locales/`. Scenario and
vehicle/behaviour fixtures retain their original values. The C++ loader adds structural
JSON checks before the semantic validators and can read all catalog files.

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

## Acceptance and remaining work

Technical migration checks are separate from the owner's M0 plausibility judgement.
Read-only fixture loading does not constitute production project persistence. A running
Qt window does not constitute the M1 editor or an M7 installer.

Continue with owner M0 review, then commands/undo/project contracts and one editable link
in M1. Crossing conflicts, merges, lane changing, true W74/W99, LOS, calibration and batch
aggregation remain explicit future work.
