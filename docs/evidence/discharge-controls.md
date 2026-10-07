# M3.3.1b1 local evidence — 2026-10-06

Candidate: `codex/discharge-controls`, parent commit
`a1f6d55161d811c1af42771fc02790b297db0705` (PR #117). The PR commit supplies
the exact candidate identity. Linux GCC 13.3.0, C++20, nlohmann/json 3.11.3;
CMake/Ninja/Qt are unavailable locally. No engine, schema or frozen fixture changes.

## Contract and failure-first evidence

[Declared controls/type selection](../reference/DISCHARGE.md#declared-controls-and-vehicle-type-selection-m331b1)
retains original crossing ranks and each follower's actual predecessor. In a mixed
fixture, selecting cars must include rank 3's gap from the bus at rank 2. Reranking
selected cars would wrongly use the gap from rank 1 instead.

The interface and new tests were compiled before implementing estimator/JSON
selection. `type_filter_preserves_original_follower_gaps_and_ranks` failed at
`r.samples==2`, demonstrating that the old estimator still included all three
samples. The next test initially aborted on a missing const JSON key; its assertion
now checks key presence first so absence is a normal test failure. This was a test
harness correction, not engine behavior. No complete pre-implementation suite pass
or failure count is claimed.

## Focused suite

Compile `tests/main.cpp`, `tests/discharge_tests.cpp`, `src/eval/discharge.cpp`,
`src/project/discharge_json.cpp` and `src/core/*.cpp` directly using GCC flags
`-std=c++20 -O0 -Wall -Wextra -Wpedantic -Wno-missing-field-initializers
-fno-fast-math -ffp-contract=off`, external json includes and checkout source dir.
Run the binary with `discharge`: **15 pass, zero fail**.

The ten prior tests remain green, including complete dynamic-state equality over
220 copied seeded steps. Five added tests cover mixed-type original follower gaps,
raw-stream/sample-rank JSON, unavailable mixed startup, empty/unknown filters,
repeatable type options, declared windows/warmup/ranks, default end time and strict
invalid/missing/orphan option rejection. Startup and headway unavailable reasons
remain separate; no matching selected gaps cannot yield zero or infinite flow.

Fixture SHA-256 (`tests/discharge_tests.cpp`):
`1528873bd6961967dccdcbd08c84596b0d01a7db2767c458f74706ffa96e8cce`.

## CLI integration and limits

The CLI is compiled directly from the CMake-listed core/model/eval/project sources
with GCC C++20, `-O0 -fno-fast-math -ffp-contract=off`, version `dev`, compiler
label `GCC-13.3.0`. Compare the four-leg project, seed 42, with
`--stop-lines --discharge` against the parent executable. All existing top-level
output and default per-cycle values must match, allowing only added selection and
startup-reason metadata. Type selection must preserve every raw crossing and
include only original ranks whose follower has the selected type.

Also run with start 60, end 120, warmup 80, steady ranks 4–8 and startup ranks 1–3:
verify emitted controls and exclusion of complete cycles outside that window.
Unknown type, orphan options, no project, NaN seconds and invalid rank interval
must reject. Integration results: all checks passed. Existing output and default cycle values
match across 90 head/cycle rows. Filtering `car` preserves all raw crossings and
selects the expected original follower ranks. Declared window/rank metadata and
outside-window exclusions pass; all five invalid invocations reject.

Direct compilation initially produced an empty `connector_paths.cpp` object and
the link failed on its missing symbols. Recompiling that unchanged translation
unit serially and relinking passed; no repository source change was needed.
Documentation, architecture, file-size and whitespace guards pass.

Project/catalog input identity is unchanged from the
[parent evidence](discharge-measurement.md#fixture-identity) and its CLI integration
hashes. Automated input-hash emission and full route/lane/source passage tracking
remain M3.3.1b2. Current selection uses vehicle-type IDs, not future behavior
classes or PCU. BA05, empirical/owner gates and full native Linux/Windows desktop
CTest remain independent; local tests do not close those gates.
