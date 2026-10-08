# M5.3: "Run N seeds" in the editor — 2026-10-08

ROADMAP S2 ([D138](../decisions/RECORD.md#d138)). Development evidence for the editor's batch, not
validation: every figure is the same unvalidated simulated movement delay as
`trafficsim-cli --seeds` (D136). The not-yet-validated marker stays.

## Inputs

- GCC 13.3.0, Qt 6.4.2, Linux, CMake `desktop` preset (Debug), `QT_QPA_PLATFORM=offscreen`.
- `data/projects/four-leg-signalised.traffic.json` (900 s, dt 0.1), seeds 42–44 and 42–51.
- Test: `tests/batch_run_ui_tests.cpp`, CTest `batch-run-ui`; an optional second argument
  writes a screenshot of the batch tab.

## What the test checks

| Check | How |
|---|---|
| Results equal the CLI batch | Seeds 42–44 through Simulation ▸ Run N seeds…; the window's `BatchReport` equals `aggregate(runSeeds(...))` over the same compiled document, `batchCsv` of both is byte-equal, and every cell of the movement, approach and per-seed tables equals the report formatted as shown |
| No partial table | While running: no rows, progress "k of N", Cancel shown, the action disabled |
| An edit drops the batch | A Run settings edit (dt 0.1 → 0.05) during a 10-seed batch empties the tab at once; after the dropped worker has stopped and been joined, nothing was published |
| Cancel | After the first seed: no rows, the note says cancelled and claims no seed count; after the worker is joined, still nothing |
| Marker | The note holds "Not yet validated" and "not HCM control delay or LOS" in English and "ไม่ใช่ HCM control delay หรือ LOS" in Thai |
| Range | First seed 4294967295 with two seeds is refused before anything runs |
| Close while running | The window is destroyed with a batch running; the destructor joins the worker after its current seed |

## Results

- `batch-run-ui` passes; whole test 18 s (Debug).
- A cancelled job stopped **1.5 s** after Cancel (about one four-leg seed in Debug; 0.13 s per seed in Release, D136).
- Failure-first: with both the stop flag and the publish guard removed, the test fails ("A dropped
  job published after the edit"). Removing either alone does not fail it: each layer alone keeps
  a dropped job from publishing (the flag stops the loop before the final post; the guard ignores
  a post from a job no longer held). A first version of the test waited a fixed 5 s and missed
  this; it now waits for the dropped worker's thread to be joined.
- Not covered: an absent mean or half-width shown as an empty cell. Seeds 42–44 give every movement
  n = 3; the absent rule is covered in the `runner` group (D136).
