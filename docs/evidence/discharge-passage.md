# M3.3.1b2b1 proven passage evidence — 2026-10-07

Candidate branch: `codex/discharge-passage`, based on
`cb1dfc97d7b72b30f055fcb8d6de64ca4f50a964` (PR #119). The PR commit supplies
candidate identity. Parent Native workflow 458 passed Linux desktop/headless/Release
and Windows core/desktop, including the corrected LF/CRLF provenance fixture.
Candidate CI is separate. Local host: Linux, GCC 13.3.0, JSON 3.11.3; CMake/Ninja/Qt
are absent. No core, schema, catalogs or frozen reference fixtures changed.

## Failure-first and supported fixtures

Compile the initial eight new `tests/discharge_passage_tests.cpp` cases with the
parent observer/production objects: **3 pass, 5 fail**. Failures are insertion-tick
survivor/sink raw passage, generated survivor type, source-at-head exclusion,
actual target-lane crossing and target-cycle ambiguity exclusion. They establish
source/remap setup before observing the forced outcome.

The final twelve cases additionally cover conflicting maps straddling a head,
insertion at the half-open green end, missing remap events and repeated-ID raw
suppression. `.1/.2` s real engine steps force both source survivor/sink passage
and mandatory lane remaps, with duplicate observations. Every dynamic field,
RNG/accounting and shared Scenario/index identity matches an unobserved copy;
subsequent lateral steps reach the sink without another crossing. The lateral
fixture safely handles both terminal outcomes rather than indexing an empty fleet.
The final passage test object enables `_GLIBCXX_ASSERTIONS` for container bounds.

The target head gets a proven longitudinal crossing after its start-of-tick remap;
the lane left behind gets none. Upstream membership changes invalidate estimates
on both affected heads. Station jumps already beyond a target head never count;
downstream changes leave an unrelated prior head eligible. Nonunique maps and
contradictory event/terminal evidence suppress inference and keep cycles unavailable.
The pre-existing explicit shared-prefix fixture now recognizes at its actual
18.5 m decision station, then passes 19 m on the following tick; its queue and
single crossing remain intact. Existing real routing and 220-tick copy cases pass.

## Commands and results

Directly compile changed CMake-listed eval/project/test sources using
`-std=c++20 -O0 -Wall -Wextra -Wpedantic -Wno-missing-field-initializers
-fno-fast-math -ffp-contract=off`, external JSON includes and checkout
`TRAFFICSIM_SOURCE_DIR`. Retain unchanged parent production/test objects.
Use `TMPDIR` pointing to an existing writable scratch directory on this host.
Link with test files main, discharge, discharge_passage, input_manifest, core,
project, reference and movement. Run unfiltered: **85/85 pass**, including 30
discharge cases, 7 provenance cases and four frozen TS reference scenarios.
`--check-groups discharge provenance core project reference movement` reports
six groups, zero unlisted. Documentation, architecture, file-size and whitespace
guards pass. This is not a full local CMake/CTest or Qt desktop run.

Recompile the CLI with the same libraries, version `dev`, compiler `GCC-13.3.0`.
Run parent and candidate for four-leg-signalised, t-junction-priority,
lane-change-lab and m2.6-study-template, seed 42, with `--stop-lines --discharge`.
Removing only `discharge` leaves each parsed JSON exactly equal; each inputManifest
also matches. The signalised fixtures have 90 and 363 cycles; the other two have
none. Raw discharge changes are diagnostics, not engine motion/capacity calibration.

## Retained source/fixture hashes

| File | SHA-256 |
|---|---|
| `tests/discharge_passage_tests.cpp` | `eef82df7927732b2b1e654eba89f751c3f2cfcafee777d6b47e3725473ec0ba7` |
| `tests/discharge_tests.cpp` | `1e838d2f98d3d385e58060947aac69b71821006af5223840887d1b662ce02587` |
| `src/eval/discharge.cpp` | `c22e5cc5129badc0b298b74b0e8a757646e0ddda82daf8f50ee1d60648a50332` |
| `src/eval/discharge_passage.cpp` | `71a33f37bc6e06da4e5fa8781777a366eb4ee8d1e58fd7f717579c90d706aa32` |

## Open gate

M3.3.1b2b2 still needs a producer/identity contract for a source generated and
arrived in one tick with neither survivor nor previous pending type evidence.
That forced case emits no guessed crossing and reports `untracked_source_passage`.
A route/input can contain multiple types, so nonunique type inference is forbidden.
BA05 remains partial/open, and candidate native Linux/Windows evidence is required.
No M0/M6, empirical or owner gate closes; no swept-body/reservation fidelity follows.
