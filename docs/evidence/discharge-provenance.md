# M3.3.1b2a local evidence — 2026-10-06

Candidate branch: `codex/discharge-provenance`, based on
`94dad267add06cabaeb691fca8df6045d5b984f8` (PR #118). The PR commit supplies the
candidate's exact identity. Parent PRs #117 and #118 have successful Native C++
workflow runs 455 and 456. Run 456's Linux desktop/headless/Release and Windows
core/desktop jobs passed. That is parent evidence, not candidate CI evidence.

Local host: Linux, GCC 13.3.0, nlohmann/json 3.11.3; CMake/Ninja/Qt unavailable.
No engine, schema, catalog content or frozen reference fixture changed.

## Failure-first passage fixtures

Before changing the observer, run the two added explicit snapshot tests against
the prior algorithm: **15 pass, 2 fail**. Shared-prefix recognition failed at
`row.unavailable.empty()`; diversion to a route sink failed at
`d.report()[0].crossings.empty()`. The fixes retain membership for identical
physical prefixes and suppress inference for diverted/lateral remaps.

The completed discharge group has **18 passing tests**. The third new case steps
a real positioned-route scenario for 220 ticks, asserts recognition and completion,
counts one queued front crossing, and compares all dynamic fields with an identical
unobserved copy. Boundary, sink, duplicate, filtering and unavailable tests remain
in that group. Lateral/insertion-tick reconstruction is not demonstrated; BA05 stays open.

## Input identity and compatibility

The new **6-test provenance group** covers empty/abc/56-byte/million-a SHA-256
answers, binary high-bit inputs at 11 padding/block boundaries (expected values
produced independently using Python hashlib), logical sorting/repeated-read counts,
changed-input rejection, exact project-byte digest and captured catalog/queue reads,
compiler output equality, and owned-catalog omission with an actual missing optional
priority directory/fallback. The SHA-256 implementation follows
[FIPS 180-4](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf); this is a
reproducibility helper, not a validated cryptographic module claim.

Fixture SHA-256:

- `tests/discharge_tests.cpp`:
  `a0833f9a57cc09df6c0e08889fbce0f906f4addc758277c0eec658c88d218a44`
- `tests/input_manifest_tests.cpp`:
  `b03a6954a4b49ef345a7c3ed2fd5a2844b04a5aba46f19e8093185428d8a55f0`

Directly compile the CMake-listed core/model/eval/project sources and test files
`main`, `discharge`, `input_manifest`, `core`, `project`, `reference`, `movement`.
Flags: `-std=c++20 -O0 -Wall -Wextra -Wpedantic
-Wno-missing-field-initializers -fno-fast-math -ffp-contract=off`, external json
includes, checkout `TRAFFICSIM_SOURCE_DIR`. Relevant unchanged parent objects were
retained; changed sources/callers were recompiled. The first link exposed a stale
`demand_preview` object with the old optional-parameter signature; rebuilding it
fixed the local harness. No repository source fix was needed for that link.

The six registered groups run **72 tests, all pass**, including the four frozen TS
reference scenarios. Initially two project filesystem tests could not find this
host's absent `/tmp`; configure `TMPDIR` to an existing writable scratch directory
and rerun the same binary. The final 72/72 result is with that environment setup.
The direct test executable's group guard reports six groups, zero unlisted.
This is not a full CMake/CTest or Qt desktop run.

## CLI integration

Link the CLI with the same project libraries, version `dev` and compiler label
`GCC-13.3.0`. Run `four-leg-signalised.traffic.json`, seed 42, with
`--stop-lines --discharge`. Removing only the new `inputManifest` leaves JSON
exactly equal to the PR #118 executable. Every one of the six actual file hashes
and byte counts matches independent Python hashlib results from retained inputs.
A second run and a `--discharge-type car` run produce identical manifests.
All file logical names are sorted and project/queue entries are present.

The unchanged project/queue input hashes are retained in
[the original evidence](discharge-measurement.md). Repository documentation,
architecture, file-size and whitespace guards passed before publishing.
Current candidate native Linux/Windows CI remains independent. No owner,
empirical calibration or BA05 gate is closed by these observations.
