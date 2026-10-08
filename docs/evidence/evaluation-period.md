# M5.4: author-declared evaluation period — 2026-10-08

Owner answer to O8 ([D139](../decisions/RECORD.md#d139)). An **illustration** of what a warm-up
changes, not validation: the same unvalidated simulated movement delay, not HCM control delay
or LOS. The 300 s warm-up below is an example chosen for this record, not a recommended value;
the project files ship without a period.

## Inputs

- GCC 13.3.0, Linux, CMake `headless` preset (Debug); `trafficsim-cli --seeds 42-51`.
- `data/projects/four-leg-signalised.traffic.json` (900 s, dt 0.1), SHA-256 `3b0eb445…42c016`.
- The same file with `"evaluationPeriod": {"warmup": 300, "end": 900}` and schema 23 (written by a
  short script, not committed), SHA-256 `7fda62d7…92871c`.

## Result (10 seeds, mean ± 95 % Student-t half-width)

| Figure | Whole run (no period) | Warm-up 300 s, to 900 s |
|---|---|---|
| Run-level mean delay (s) | 50.11 ± 2.01 | 52.74 ± 2.57 |
| West approach → East exit, delay (s) | 52.81 ± 4.28 | 54.52 ± 5.86 |
| North approach → West exit, delay (s), n | 36.59 ± 13.67, n = 10 | 42.63 ± 20.97, **n = 9** |
| West right-turn pocket, mean queue (m) | 33.75 ± 4.91 | 38.79 ± 8.12 |

Seed 42 completed 451 trips; 114 arrived before 300 s (`outsideWindow`). Every seed's movement
vehicles + not-in-movement + `outsideWindow` equals its completed trips (`evalperiod` test).
The CSV marker reads "…; measured from 300 s (after the warm-up) to 900 s, trips by arrival time."

**Reading:** delay and queues are higher once the empty-network start is excluded, the effect O8
anticipated ("delay ... reads low"). With 600 s measured, one movement had no arrival in one seed;
it is left out (n = 9), not counted as 0. Without a period every output is byte-identical to before
(four-leg and T-junction `--project` JSON and CSV, seed 42; the batch JSON/CSV).

## Checks

- `evalperiod` group: arrival-time inclusion with both edges; queue samples windowed while the
  hysteresis runs on every state; codec (schema 23 only with a period, refusals, schema 22 with a
  period refused); History refusal of a duration that cuts the window; four-leg accounting; a
  batch refusing two windows.
- `batch-run-ui`: an empty checked period is refused; 300–900 s through Run settings gives a batch
  equal to `runSeeds`/`aggregate` with that window; Undo drops period and batch.
- Failure-first: counting by departure time fails the first test; sampling queues outside the
  window fails the second (the first version of that test used a stopped vehicle at every sample
  and missed it; it now ends with a moving one).
