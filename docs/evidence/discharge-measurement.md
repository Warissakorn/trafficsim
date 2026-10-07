# M3.3.1a local measurement evidence — 2026-10-06

Candidate branch: `codex/discharge-measurement`, based on P0 commit
`3218efce9310e0dda7b2d1986853012c0f2a1342`. The PR commit identifies the exact
candidate; this record does not substitute for native Linux/Windows CI.

Host: Linux, GCC 13.3.0, C++20; nlohmann/json 3.11.3 obtained explicitly outside
this repository. CMake, Ninja and Qt are unavailable on this host. No frozen
fixture or engine source changed. Tests were written with the interface before
the implementation, but no pre-implementation failing execution was recorded.

## Focused checks

Compile `tests/main.cpp`, `tests/discharge_tests.cpp`, `src/eval/discharge.cpp`,
`src/project/discharge_json.cpp` and `src/core/*.cpp` directly with GCC:
`-std=c++20 -O0 -Wall -Wextra -Wpedantic -Wno-missing-field-initializers
-fno-fast-math -ffp-contract=off`, the external json include directory and
`TRAFFICSIM_SOURCE_DIR` set to the checkout. Run the resulting binary with
argument `discharge`. Ten tests pass, zero fail:

- Analytic queued crossings at 3, 5.5, 7.5, 9.5, 11.5 seconds: steady ranks 3–5
  produce 2 seconds and 1800 veh/h; startup ranks 1–2 produce 1.5 seconds (BA01).
- Empty/short, non-queued and partial cycles return unavailable; invalid ranks,
  duplicate IDs and zero gaps are rejected; negative startup remains visible (BA02).
- Whole-cycle window/warmup exclusions, boundary timestamps, separate lanes and
  exact type identity are asserted (partial BA03; no class filter yet).
- A copied seeded run is stepped 220 times. Every dynamic field is compared:
  vehicles, inputs, events, stop service, RNG, seed, tick/time, completed count and
  next ID; immutable scenario/index pointers remain shared (BA04).
- Tracked sink arrivals survive disappearance; duplicate observations count once;
  missing ticks reject; initial greens and final partial cycles are unavailable.
  Boundary exclusion is checked at timesteps 0.1 and 0.2 seconds. Tracked lane
  remaps and ambiguous source passages explicitly invalidate cycles (partial BA05;
  complete route/source tracking is not demonstrated).
- JSON includes ranks, raw type IDs and signed estimates; unavailable values are null.

CLI and serializer translation units also pass direct `-fsyntax-only` compilation.
The full CLI was then linked directly from the CMake-listed core/model/eval/project
sources with GCC C++20, `-O0 -fno-fast-math -ffp-contract=off`, version `dev` and
compiler label `GCC-13.3.0`. Running the four-leg project with seed 42 and
`--stop-lines`, once with and once without `--discharge`, produces exactly equal
existing JSON after removing the new field. The new output contains 90 head/cycle
rows, including valid estimates and explicit partial/queue/remap exclusions.
`--discharge` without `--project` rejects. These are CLI integration checks, not
CMake/CTest or desktop evidence.

Inputs retained from the parent commit, with SHA-256:

- `data/projects/four-leg-signalised.traffic.json`:
  `3b0eb4450904ed66ef6f698b24280f7318a6031d6423dc60e16a2ba11542c016`
- `data/evaluation/queue-counter.json`:
  `04bff220abb06e80db644e1b94afb53f3e0af790ffe2371e06eff5f2b82a1de9`

Documentation/navigation, file-size and architecture guards and `git diff --check`
are required separately. Full CTest/Qt/Windows, empirical flow calibration,
class filtering and complete route/source passage tracking remain open.

## Fixture identity

The deterministic fixture is embedded in `tests/discharge_tests.cpp`; it has no
external study dataset. Its SHA-256 is
`111872524bf0f3e703e18b2736c4c419756e4ed18e6ab3fe8059e1c1dae86b98`.
No universal PCU target or empirical capacity claim follows from these tests.
