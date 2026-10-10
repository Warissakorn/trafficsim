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
## 5. Evaluation period, cool-down and unfinished trips (M5.3 D132, M5.9 D146)

Every seed uses the project's evaluation period ([SIMULATION](SIMULATION.md#movement-evaluation-m25)):
rows count trips that end inside it and queues average its ticks; run totals stay whole-run.
Each movement also reports `unfinished` (active or pending on its routes at the end) as an
Estimate. A movement whose mean unfinished exceeds 5 % of mean (completed + unfinished) is named
in `movementsWithUnfinished` and in a `# WARNING` CSV line: its completed-trip delay reads low,
because the stuck vehicles never arrive.

**Cool-down (M5.9, D146).** Without one, a run whose demand lasts to its last second reports
every trip still in flight as unfinished, so the warning fires where nothing is wrong. An author
sets `definition.evaluation.cooldown` (seconds, schema 27, written only when above 0):

- **Run length.** The run lasts `duration + cooldown`, always to that bound; it does not stop
  early when the window's trips have finished, which would change only run totals and would make
  the editor's Run and the CLI disagree. Inputs still end by `duration`, so no new demand is
  generated during the cool-down; vehicles already waiting at a source still enter.
- **The window selects trips by their demand.** With a cool-down a trip belongs to the window when
  its vehicle's `scheduledTime` (when its input released it, so source waiting is included) lies
  in `[warmup, end]` (`end` defaults to `duration`, never to the longer run). A window trip counts
  in its movement row when it arrives and in a section row when it crosses the section's end line,
  at any time up to the end of the run. A vehicle released after `end` counts nowhere.
- **Unfinished** is a window vehicle still in the network or still waiting at a source when the
  run ends: traffic the cool-down did not clear, such as a gridlocked movement, which the 5 % rule
  still names.
- **Queues** still average only the ticks in `[warmup, end]`. Run totals (`completed`, `pending`,
  `active`, `safetyClamps`, network `meanDelay`) stay whole-run and include the cool-down, so
  generated = completed + active + pending still closes.
- **Without a cool-down** (absent or 0) every rule above is D132's, byte for byte: rows count trips
  that end in the period, and every vehicle left at the end is unfinished.
- **Output.** JSON `evaluationPeriod` gains `cooldown`, the single-run CSV a `cooldown_s` line after
  `evaluationPeriod_s`, the batch CSV a `# Cool-down: N s after the demand ends` line after the
  evaluation period line, and the editor's notes say the same. None appears without a cool-down.
- **Valid** when finite, above 0 and on the time grid (`EVALUATION_PERIOD_INVALID`); the period's
  `end` must still lie inside `duration`. A new editor project starts with 900 s.
- **Limit.** A window vehicle that reaches a positioned routing decision after that decision's last
  interval uses its default weights, the rule [POSITIONED_ROUTING](POSITIONED_ROUTING.md) already
  states for any time outside the intervals.

| Row | Check | Test |
|---|---|---|
| CD1 | Schema 27 round-trips; a file without `cooldown` keeps its schema and bytes; the key below schema 27, zero, negative, non-finite or off-grid values are refused; clearing it restores the earlier bytes | `cooldown` |
| CD2 | One vehicle released exactly at `end` that arrives during the cool-down counts in its movement; without a cool-down it does not | `cooldown` |
| CD3 | A vehicle released after `end` counts in no row and not as unfinished | `cooldown` |
| CD4 | Gridlock: behind an all-red head, window vehicles stay unfinished and `movementsWithUnfinished` names the movement | `cooldown` |
| CD5 | No vehicle is released after `duration`; the cool-down adds ticks, not demand | `cooldown` |
| CD6 | A file without a cool-down gives the same batch CSV and JSON before and after a save through the new codec; four-leg CLI outputs are byte-identical across the change | `cooldown`, session `cmp` |
| CD7 | `four-leg-signalised` with a cool-down, seeds 42-51: no movement is warned, while CD4's network still is | `cooldown` |
| CD8 | A window vehicle's section trip that ends during the cool-down counts; section `unfinished` counts window vehicles only | `cooldown` |

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

## 7. Scenario comparison (M5.8, D148)

`trafficsim-cli --project BASE --compare ALTERNATIVE --seeds LIST [--csv FILE]` answers
[PROBLEM](../PROBLEM.md) §1 step 6: how much a with-project or mitigated scenario changes the
base. Every figure is a difference of **simulated** delays or queues, not HCM control delay or
LOS, and not validated (rule 4). The CLI comes first (M5.8a); the editor's Copy and Export are
M5.8b.

- **Runs.** Each project is parsed, compiled and run as its own batch (§1–§2) over the same
  `LIST`. Each run draws one random stream from its seed, and the two streams diverge as soon as
  the projects differ. So equal seeds are **not** common random numbers: the batches are treated
  as independent samples and nothing is paired.
- **Refused before any output.** `--compare` needs `--project` and `--seeds`, and refuses what
  `--seeds` refuses: a single seed, the single-run diagnostic flags, `--scenario` and `--events`.
  Also refused:
  - two batches whose seed lists differ;
  - evaluation periods that differ in warm-up, end or cool-down, because a difference over
    different windows is not the scenario's effect;
  - an existing `--csv` file.
- **Rows.** Movements, sections and approaches are matched by name, block by block, in the
  base's order. A name found in only one batch, or more than once in either, is not compared. It
  is listed as base-only, alternative-only or ambiguous, in JSON `unmatched` and on a
  `# Unmatched` CSV line; nothing is dropped silently. The network mean delay is one more row.
- **Quantity.** Mean delay for movements, sections and the network; mean queue length for
  approaches.
- **Statistic.** Each row uses the two batches' estimates over seeds (`n`, mean, SD):
  - difference `d = mean_alternative − mean_base`;
  - standard error `SE = √(s_b²/n_b + s_a²/n_a)`;
  - Welch–Satterthwaite degrees of freedom
    `ν = SE⁴ / [(s_b²/n_b)²/(n_b − 1) + (s_a²/n_a)²/(n_a − 1)]`;
  - 95 % half-width `tQuantile975(⌊ν⌋) · SE`. This takes the lower whole df, as §2's table takes
    the lower row: wider, never narrower.

  Edge cases:
  - A mean missing on either side (no vehicle in any seed) leaves `d` empty.
  - A side with `n < 2` has no SD: `d` is given, while the interval and `ν` are empty.
  - Both SDs 0 gives `SE = 0`: the half-width is 0 and `ν` is empty.
- **Reading it.** An interval that spans 0 means the difference cannot be told apart from seed
  noise at 95 %. The output claims nothing more, and gives no LOS letter to a difference.
- **JSON (stdout):**
  - `validated: false`, `measure`, `seeds`;
  - `base` and `alternative`, each with its file name, `overloadedSeeds` and
    `movementsWithUnfinished`;
  - `evaluationPeriod`;
  - `movements`, `sections` and `queues` rows: `{base: {n, mean}, alternative: {n, mean},
    difference, halfWidth95, degreesOfFreedom}`;
  - `network` and `unmatched`;
  - `engineVersion`, `buildCommit`, `compiler`.
- **CSV (`--csv`):**
  - the marker line first;
  - `# Base:` with both file names;
  - for each side, the overloaded-seed and unfinished-trip `# WARNING`s (§3, §5), when any;
  - `# Unmatched`, when any;
  - the evaluation period and cool-down;
  - then the blocks `movement`, `section` (only when either side has a section), `approach` and
    `network`, each with the columns `base_n, base_mean, alternative_n, alternative_mean,
    difference, ci95, df`, in units of s or m.

| Row | Check | Test |
|---|---|---|
| CMP1 | Welch by hand: `d`, `SE`, `ν`, `⌊ν⌋` and the half-width for explicit estimates; `ν` lies between `min(n) − 1` and `n_b + n_a − 2` | `compare` |
| CMP2 | Swapping base and alternative negates every `d` and leaves every half-width and `ν` unchanged | `compare` |
| CMP3 | Base-only, alternative-only and duplicate names are listed and not compared; matched rows keep the base's order | `compare` |
| CMP4 | Different seed lists and different warm-up, end or cool-down are refused; `n < 2` gives an empty interval; zero SDs give a zero half-width | `compare` |
| CMP5 | `four-leg-signalised` against itself (seeds 42–43): every `d` is exactly 0 and every half-width positive | `compare` |
| CMP6 | `four-leg-signalised` against a copy with one input's volume raised by half: every `d` equals the difference of the two batches' own means, bit for bit, and at least one is not 0 | `compare` |
| CMP7 | CLI: `--compare` without `--seeds` is refused; the CSV starts with the marker line; two invocations give identical bytes | `compare`, `cli-compare`, `cli-compare-conflict` |
