# Source first-step clearance — D108, M3.2.8a.1a

Development evidence, 2026-10-04. No M0/M3 owner acceptance or M6 calibration claim.
Baseline: `2a6081005d62800f25052d695bfb198bc22c7d09` (D107 Connector equation).
Same Linux/GCC 13.3 Release toolchain with strict floating-point options in both builds.
Contract, failure-first tests and [input hashes](source-first-step-inputs.json) were committed
in `6375ad2` before implementation and observed seeded outputs.

## Cause and contract

Baseline source entry admitted any snapshot leader gap ≥ standstillDistance. A newly
inserted vehicle starts at rest but can immediately request more distance than the small
positive clearance; the later hard movement cap then cuts it to zero and emits a clamp.

On the current D107 geometry, seed 48 at all three headways has one such event each:
vehicle 245, t = 805.7 s, leader speed 0.613002485 m/s, gap 2.000491391 m, buffer 2 m.
Available distance is 0.000491391 m; ordinary acceleration requests 0.002044434 m.
The earlier D105 source traces were on different geometry and retain their original counts.

Before entry, evaluate the existing following law from speed zero with the pending vehicle's
sampled desired speed/driverFactor and the candidate snapshot leader, then integrate for dt.
If requested distance exceeds gap − standstillDistance, keep that arrival pending.
Equality admits; the exact standstill boundary admits a zero-motion step under D105.
The queue record, ID, scheduled time and random state are unchanged; actual departure occurs
when room opens. Candidate order and one attempt per source/tick remain unchanged.
No following parameter, buffer, setback, maximum deceleration or event suppression changes.
The guard is deliberately local to source entry, before lane changes/signals/conflict phases;
new constraints from those phases still use normal braking and emergency reporting.
Moving merge anticipation remains M3.2.8a.1, not implemented by this source slice.

## Regression

Three `core` cases (A59–A61) test queue preservation/release, actual versus scheduled entry,
RNG/accounting/copied replay, exact first-step equality, just-below clearance and the exact
standstill boundary. Two assertions fail before and pass after. Existing moving-merge
impossible-stop reporting remains tested and passing.
Linux Release `check`: **54/54**, including frozen references, architecture/file-size guards.
Frozen baselines and project inputs are unchanged. Desktop/Windows CI is recorded in the PR.

## 120 checked T-junction cases per build

Seeds 42–81 × headways 3/7/12 m, gapTime 5 s. Both diagnostic sweeps exit successfully after
per-tick copied replay, vehicle accounting, shared-segment body separation, swept conflict
interval checks and reported maximum braking. These checks do not prove cross-path 2D safety.

| Measure | Before | After |
|---|---:|---:|
| Generated/completed vehicles | 32,730 | 32,730 |
| Active/pending at end | 0/0 | 0/0 |
| Safety clamps | 226 | 223 |
| Minor clamps while moving | 21 | 21 |
| Minor clamps at source entry | 3 | 0 |
| Identical before/after trajectory digests | — | 117/120 |

The three differing runs are seed 48, one per headway. All drain. Source-clamp-free is an
observation of this sweep, not a universal guarantee. Genuine moving clamps remain counted.
Full paired reports, trajectory digests and source traces: [stress evidence](source-first-step-tjunction.json).

## Four supplied projects × 40 seeds

All CLI JSON fields are compared recursively for seeds 42–81. Generated/completed/active/
pending and movement counts stay unchanged, as do mean-delay fields. Two projects' reports
remain exactly equal. Lab/M2.6 travel times change because actual entry can move later while
the original scheduled time is preserved; this is not a capacity or calibration improvement.
Queue values can change numerically at the recorded small scale.

| Project | Changed reports | Clamps before → after | Maximum mean-travel difference (s) | Maximum queue-mean difference (m) |
|---|---:|---:|---:|---:|
| four-leg-signalised | 0/40 | 220 → 220 | 0 | 0 |
| lane-change-lab | 13/40 | 19 → 1 | 0.000956938 | 0 |
| m2.6-study-template | 15/40 | 858 → 837 | 0.002857143 | 0.000000457 |
| t-junction-priority | 0/40 | 4 → 4 | 0 | 0 |

[Per-seed differences](source-first-step-projects.json) retain every run, including unfinished
four-leg/M2.6 traffic. No seed is removed to improve the figures.

## Reproduce

Build baseline and final with the same headless Release configuration. Run both executables
from the same repository checkout to use identical hashed projects/catalogs:

```sh
trafficsim-t-junction-clamps --sweep sweep.jsonl .
trafficsim-t-junction-clamps --trace-all trace.jsonl .
trafficsim-cli --seed 42 --project data/projects/lane-change-lab.traffic.json --data-dir data
```

Repeat CLI for four projects × seeds 42–81. Compare complete JSON structure and numeric
fields; pair sweep rows by seed/headway. This workspace needs a writable TMPDIR because
/tmp is unavailable. Neither command is an M5 batch API or an M6 validation experiment.
