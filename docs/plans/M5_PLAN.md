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
| **M5.3** | Evaluation period | `evaluation.warmup` (default 900 s) and `evaluation.end` in the project file, older files load unchanged; the movement observer ignores trips that entered before warm-up and counts unfinished trips per movement | Codec and migration tests; inspector edit with Undo; en/th strings; warm-up 0 reproduces today's numbers |
| **M5.4** | Travel-time (delay) sections | Contract and acceptance rows in `docs/reference/` first. A section is a start and end line on Links/Connectors; delay = section travel time − free-flow time at desired speed | Analytic fixtures (lone vehicle; one vehicle held by a red of known length); placement shares the counter gesture; save/reopen, Undo/Redo |
| **M5.5** | LOS pack as data | `data/los/` packs with A–F thresholds by control type (rule 5); pure `losLetter(delay, controlType, pack)`; control-type tag on movements; approach and intersection rows volume-weighted | Threshold-edge tests on both equality sides; a swapped pack changes letters with no code edit; LOS appears only on section delay, labelled "simulated section delay, not validated (M6)" |
| **M5.6** | Editor "Run N seeds", Copy and Export | Shell action over M5.2 on a worker thread with a snapshot copy, progress and cancel; an edit invalidates the result. Results shows n, mean, ±95 %, LOS, queue and seed flags. Copy (TSV) and Export (CSV) share the CLI formatter; the marker line comes first | UI tests: rows equal the CLI batch; cancel leaves no table claiming N runs; export bytes equal the CLI's; Linux and Windows jobs in `native.yml` |
| **M5.7** | Rehearsal with a real study | The owner runs the §4 sentence on a real aerial image and the M2.6 template with its placeholders replaced (NEXT §2) | File, commit and table recorded; image size against the 32 MiB cap noted; labelled partial until M6 |

## 3. After M5, not blocking first use

- Amber stop-or-go (D36 runs amber as red, which biases delay) and the live
  [CONNECTOR_PARITY_AUDIT](../audits/CONNECTOR_PARITY_AUDIT.md) §3.5/§3.6 defects.
- W74 resumes from [NEXT](../NEXT.md) (M3.3.3a); if it lands after M5.3, it takes the next
  schema number rather than 22.
- The M6 benchmark option sheet and validation; the marker stays until M6 passes.
- Motorcycles for Thai counts and an installer (M7) need their own milestones.

## 4. Verification for every slice

`cmake --preset desktop`, `cmake --build --preset desktop`, `ctest --preset desktop` and the
`check` target (headless preset where Qt is missing, without a desktop claim). Frozen TS
baselines and same-build replay stay unchanged. Native Linux and Windows CI are separate
evidence; owner observation is separate again.
