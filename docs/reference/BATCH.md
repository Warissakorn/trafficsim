# Multi-seed batches (M5.2)

What `trafficsim-cli --project F --seeds LIST` and `src/runner/` compute. Decisions:
[D130](../decisions/RECORD.md#d130) (M5 first) and [D131](../decisions/RECORD.md#d131) (this
contract). Every figure is **simulated movement delay, not HCM control delay or LOS, and not
validated** (rule 4); LOS waits for travel-time sections ([M5_PLAN](../plans/M5_PLAN.md) M5.4–M5.5).

## 1. Runs

- One project is parsed and compiled once (`compileDocument`, `evaluationSpec`); every seed runs
  that same `Scenario`. `runSeed` is the CLI's single-run loop (create, observe, step and observe
  to `totalTicks`), so a one-seed batch's `MovementReport` equals `--project`'s exactly.
- `LIST` is `42-51`, `1,5,9` or a mix, in the order written; at most 1000 seeds. A reversed
  range, an empty item or a repeated seed is refused before anything runs.
- Seeds run one after another. Cancelling (through the `progress` callback) returns no runs, so
  nothing can claim N runs it did not finish.
- Per seed: `generated` (vehicles the inputs created, `nextVehicleId − 1`) =
  `completed + active + pending`, checked by test.

## 2. Aggregate

`aggregate` sorts the runs by seed before summing, so any order of the same runs gives the same
bytes. Rows are matched by position and name; runs with different movements or approaches are
refused.

For each quantity over the seeds that have a value (a seed with no vehicle on a movement has no
delay and leaves that row's `n`):

| Field | Definition |
|---|---|
| `n` | Seeds with a value |
| `mean` | Arithmetic mean of the per-seed values (each seed weighs the same, as Vissim's run average) |
| `sd` | Sample standard deviation (n − 1); null when n < 2 |
| `halfWidth95` | t(0.975, n − 1) · sd / √n; null when n < 2. The interval is mean ± halfWidth95 |

The t quantile is tabulated for 1–30, 40, 60 and 120 degrees of freedom (three decimals); between
rows the lower row is used, which widens the interval and never narrows it. 120 and above use 1.980.

## 3. Overloaded seeds

A seed is **overloaded** when its end-of-run `pending` exceeds 5 % of `generated`: demand the
network did not take, usually gridlock or an input above capacity. By the owner's choice
(2026-10-09) it **stays in every mean**, because dropping it would report the lower delay of the
seeds that coped. It is named in `overloadedSeeds`, in a `# WARNING` line of the CSV and on stderr.
Completed-trip delay itself under-reads a saturated movement, since stuck vehicles never
complete; the per-seed block shows `pending` and `active` so a reader can see it.

## 4. Output

JSON on stdout: `validated: false`, `measure`, `seeds`, `overloadedSeeds`, per-movement
`vehicles`/`meanDelay`/`meanTravelTime` and per-approach `meanLength`/`maxLength` as
`{n, mean, sd, halfWidth95}`, network `meanDelay`/`completed`/`pending`/`safetyClamps`, a
`perSeed` array, and provenance: `engineVersion`, `buildCommit` (`git rev-parse` at build time,
`-dirty` with local changes, `unknown` without Git) and `compiler`.

`--csv F` (never overwrites) writes the marker line, the warning line if any, then three blocks:
`movement,n,meanDelay_s,ci95_s,sd_s,vehicles_mean,meanTravelTime_s`;
`approach,n,meanQueue_m,ci95_m,maxQueue_m,maxQueue_ci95_m`; and
`seed,generated,completed,active,pending,safetyClamps,overloaded,meanDelay_s`. Cells use the
single-run CSV's formats (`csv_format.hpp`): quoted names, C-locale numbers with two decimals,
an empty cell for no value.

`--seeds` cannot be combined with a single seed, `--discharge` or the single-run diagnostic flags.
There is no warm-up yet; every trip completed in the run counts (M5.3 adds the period).
