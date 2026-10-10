# M5 plan — multi-seed results, LOS and report tables (2026-10-08)

The owner asked for the shortest path to real study use and answered the roadmap review's
O1, O3, O8 and O9 ([D130](../decisions/RECORD.md#d130)). This plan numbers the slices that
carry the back half of the [PROBLEM §4](../PROBLEM.md#4-what-done-looks-like) sentence:
"runs 10 seeds, and gets a movement-level delay and LOS table they can paste into a report".
[ROADMAP](../ROADMAP.md#m5--evaluation-and-reporting) owns the M5 scope and gate;
[NEXT](../NEXT.md) owns the live queue. A slice listed here is not implemented until its
own session records evidence.

## 1. Owner answers this plan rests on

| Review question | Answer | Consequence |
|---|---|---|
| O1 order | (a) M5 before further W74 (M3.3.3a) and editor polish | W74 is parked, not cancelled; owner looks stay queued but block nothing |
| O3 LOS basis | (b) section-bounded delay, HCM pack first | LOS letters need travel-time sections (M5.4) before M5.5; whole-route delay (D39) never gets a letter |
| O8 evaluation period | Warm-up default 900 s, editable per project; unfinished trips counted separately and warned | Schema change in M5.3 |
| O9 seeds | (c) default 10, always show n and the 95 % half-width | Batch output carries n per row |
| Q4 "decide before M5" | Explicitly waived for M5 by D130 | The M6 benchmark sheet is still owed before M6; every number keeps the not-yet-validated marker |

## 2. Slices

One system per session, interface first, build green at each commit.

| Slice | System | Interface first | Done when |
|---|---|---|---|
| **M5.2** · implemented 2026-10-09 (D131, [BATCH](../reference/BATCH.md)); native CI per PR | Batch runner and `trafficsim-cli --project F --seeds A-B [--csv F]` | `src/runner/`: `runSeeds(snapshot, spec, seeds)` → per-seed results; `aggregate(results)` → n, mean, SD and t-based 95 % half-width per movement and approach, in seed order. The project run loop in `tools/run_simulation.cpp` moves into a callable function the runner reuses with `compileDocument` and `evaluationSpec`. Each seed records generated/completed/active/pending and clamps; a gridlocked seed or one past the unfinished-trip threshold is flagged, never silently averaged. `engineVersion` becomes a build-stamped commit plus compiler | Hand-computed aggregate test; a permuted seed list gives the same result; a one-seed batch equals today's `--project` output byte for byte; generated = completed + active + pending per seed; frozen baselines unchanged; `check_architecture` gains a runner rule with a negative fixture; ARCHITECTURE records where batch formatting lives |
| **M5.3** · implemented 2026-10-09 (D132); native CI per PR | Evaluation period | `evaluation.warmup` (default 900 s) and `evaluation.end` in the project file, older files load unchanged; the movement observer ignores trips that entered before warm-up and counts unfinished trips per movement | Codec and migration tests; inspector edit with Undo; en/th strings; warm-up 0 reproduces today's numbers |
| **M5.4** · implemented 2026-10-09 (D133, [TRAVEL_TIME_SECTIONS](../reference/TRAVEL_TIME_SECTIONS.md)); native CI per PR | Travel-time (delay) sections | Contract and acceptance rows in `docs/reference/` first. A section is a start and end line on Links/Connectors; delay = section travel time − free-flow time at desired speed | Analytic fixtures (lone vehicle; one vehicle held by a red of known length); placement shares the counter gesture; save/reopen, Undo/Redo |
| **M5.5** · implemented 2026-10-09 (D134, [LOS](../reference/LOS.md)); native CI per PR | LOS pack as data | `data/los/` packs with A–F thresholds by control type (rule 5); pure `losLetter(delay, controlType, pack)`; control-type tag on sections (author-set, D134); approach (start Link) and intersection rows volume-weighted | Threshold-edge tests on both equality sides; a swapped pack changes letters with no code edit; LOS appears only on section delay, labelled "simulated section delay, not validated (M6)" |
| **M5.6** · implemented 2026-10-09 (D141, [BATCH](../reference/BATCH.md) §6); native CI per PR | Editor "Run N seeds", Copy and Export | Shell action over M5.2 on a worker thread with a snapshot copy, progress and cancel; an edit invalidates the result. Results shows n, mean, ±95 %, LOS, queue and seed flags. Copy (TSV) and Export (CSV) share the CLI formatter; the marker line comes first | UI tests: rows equal the CLI batch; cancel leaves no table claiming N runs; export bytes equal the CLI's; Linux and Windows jobs in `native.yml` |
| **M5.7** | Rehearsal with a real study | The owner runs the §4 sentence on a real aerial image and the M2.6 template with its placeholders replaced ([OWNER_SITTING](OWNER_SITTING.md) A3, B4) | File, commit and table recorded; image size against the 32 MiB cap noted; labelled partial until M6 |
| **M5.8** · booked by D143 (R1); M5.8a CLI implemented 2026-10-10 (D148, [BATCH](../reference/BATCH.md) §7); M5.8b editor next | Scenario comparison (PROBLEM §1 step 6: base, with-project, mitigated) | Contract first: two projects (or two batches) on one seed list; per movement, section and approach, the difference of means with a Welch 95 % CI; rows matched by name, unmatched rows listed, never silently dropped. No common-random-numbers claim (one random stream per run). CLI first (`--compare`), then the editor | Hand-computed difference and Welch interval; swapped order negates the differences; mismatched rows reported; marker line first; Copy/Export through the same formatters as M5.6 |
| **M5.9** · implemented 2026-10-09 (D146, [BATCH](../reference/BATCH.md) §5, [evidence](../evidence/m5.9-cooldown.md)); native CI per PR | Evaluation cool-down | Contract first: after the evaluation end, the run continues until every trip that entered inside the window has finished or a bound (author-set, default stated in the contract) is reached; only then is a trip unfinished. Delay rows still count trips by the window. Older files and a zero cool-down give today's bytes | `four-leg-signalised --seeds 42-51` no longer flags in-flight traffic as unfinished while a gridlocked movement still is; analytic fixture (one vehicle entering at the window end); codec/migration tests; frozen baselines unchanged |

## 3. Order after M5.7 (D143)

The owner's R2 answer: correct the known biases first, then the validation path.

1. **M5.9** evaluation cool-down (above) — implemented (D146).
2. **M4.2** amber stop-or-go ([ROADMAP](../ROADMAP.md) M4.2; D36 ran amber as red, which biased
   every signalised delay) — implemented (D147).
3. **M6.0** benchmark option sheet, docs only, no engine numbers; the marker stays until M6 passes
   — delivered 2026-10-10 ([sheet](../evidence/m6-benchmark-options.md)); the owner chooses in
   [OWNER_SITTING](OWNER_SITTING.md) B9.
4. **M5.8** scenario comparison (above) — M5.8a, the CLI `--compare`, implemented (D148); M5.8b, the
   editor, next.

Not booked here: W74's cited preset and W99 ([NEXT](../NEXT.md)); the live
[CONNECTOR_PARITY_AUDIT](../audits/CONNECTOR_PARITY_AUDIT.md) §3.5/§3.6 defects; motorcycles,
decided after M5.7 (R3, [PROBLEM](../PROBLEM.md) §8); the installer (M7).

## 4. Verification for every slice

`cmake --preset desktop`, `cmake --build --preset desktop`, `ctest --preset desktop` and the
`check` target (headless preset where Qt is missing, without a desktop claim). Frozen TS
baselines and same-build replay stay unchanged. Native Linux and Windows CI are separate
evidence; owner observation is separate again.
