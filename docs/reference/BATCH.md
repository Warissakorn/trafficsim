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

`--csv F` (never overwrites) writes the marker line, the warning lines if any, a
`# Evaluation period: W s to E s` line, then three blocks:
`movement,n,meanDelay_s,ci95_s,sd_s,vehicles_mean,meanTravelTime_s,unfinished_mean`;
`approach,n,meanQueue_m,ci95_m,maxQueue_m,maxQueue_ci95_m`; with a travel-time section
(M5.4, [TRAVEL_TIME_SECTIONS](TRAVEL_TIME_SECTIONS.md) §4) a `section,...` block in the movement
block's columns plus `controlType,los` and, with a control type, the LOS block
([LOS](LOS.md) §4); and
`seed,generated,completed,active,pending,safetyClamps,overloaded,meanDelay_s`. Cells use the
single-run CSV's formats (`csv_format.hpp`): quoted names, C-locale numbers with two decimals,
an empty cell for no value.

`--seeds` cannot be combined with a single seed, `--discharge` or the single-run diagnostic flags.
## 5. Evaluation period and unfinished trips (M5.3, D132)

Every seed uses the project's evaluation period ([SIMULATION](SIMULATION.md#movement-evaluation-m25)):
rows count trips that end inside it and queues average its ticks; run totals stay whole-run.
Each movement also reports `unfinished` (active or pending on its routes at the end) as an
Estimate. A movement whose mean unfinished exceeds 5 % of mean (completed + unfinished) is named
in `movementsWithUnfinished` and in a `# WARNING` CSV line: its completed-trip delay reads low,
because the stuck vehicles never arrive.

## 6. Editor batch (M5.6, D141)

The editor's Results tab runs the same batch as `--seeds`, for the project open in it.

- **Seeds.** A *Seeds* field beside *Run seeds* takes the CLI's `LIST` (`parseSeedList`), default
  `42-51` (ten seeds, O9). A bad list is refused with the CLI's message before anything runs.
- **Snapshot.** *Run seeds* compiles the current document (`compileDocument`, `evaluationSpec`) on
  the UI thread, then hands value copies to one worker thread that calls `runSeeds` and
  `aggregate`. The worker never reads the document, so editing during a batch cannot change it.
- **Progress and cancel.** The note shows seeds done of N. *Cancel* (and every edit, Undo, Redo,
  New, Open, Reset, a change to either seed field, or a single Run) stops the batch: the progress callback
  returns false, the worker's result is discarded by generation number, and no table is shown.
  A batch therefore never shows, copies or exports fewer runs than its n says. Closing the window
  cancels and waits for the seed in progress.
- **One table at a time.** A finished batch replaces the single-run tables until it is
  invalidated as above; starting a single run discards the batch.
- **Table.** Movements and sections: n, mean delay, ±95 % half-width, mean vehicles, mean travel
  time, mean unfinished; sections add control type and LOS ([LOS](LOS.md) §4). Approaches: n,
  mean queue ±95 %, max queue. The note names the measure, the evaluation period, the overloaded
  seeds and the movements with unfinished trips (§3, §5), under the not-validated marker.
- **Export** writes `batchCsv` (§4), byte for byte the CLI's `--csv` for the same project and seeds.
- **Copy** puts the same text on the clipboard as tab-separated values (`csvToTsv`): comment
  lines unchanged, each other line split at its unquoted commas, quotes removed. It pastes into a
  spreadsheet or a Word table column by column, marker line first. A single finished run copies
  `movementCsv` the same way.

### Acceptance rows

| Row | Check | Test |
|---|---|---|
| EB1 | The table's rows, n and means equal `aggregate(runSeeds(...))` of the same project and seeds | `batch-run-ui` |
| EB2 | Export bytes equal `batchCsv` of that aggregate (the CLI's `--csv`) | `batch-run-ui` |
| EB3 | Copy equals `csvToTsv(batchCsv(...))`, marker line first | `batch-run-ui`, `batch.tsv_*` |
| EB4 | Cancel part way leaves no batch table; Export and Copy refuse | `batch-run-ui` |
| EB5 | An edit after a finished batch discards it; the single-run view returns | `batch-run-ui` |
| EB6 | A bad seed list is refused before any run | `batch-run-ui` |
