# Archived PROGRESS entries

Moved whole from PROGRESS.md on 2026-10-05.

## 2026-10-04 — `scenario-run-ui` was the E-cores, not a regression; the test is now 10× cheaper

The ≈100 s CPU found below is scheduling, not code. The same executable at the same commit was
pinned by `ProcessorAffinity` on this i5-12450H (logical 0–7 = four P-cores with HT, 8–11 = four
E-cores), interleaved: P-cores 29.1 / 26.0 s CPU, E-cores 103.9 / 88.9 s. The 2026-10-02 28 s run
was on P-cores. The code between `b823cd8` and `eeb9c5c` could not have done it anyway: locale
text, a test, and `hairlinePen` width 1/dpr → 1, which is the same pen at the offscreen dpr of 1.
The earlier "machine idle" rested on one 3 % load sample; Windows still parks a Debug test on an
E-core with Zoom and Discord running.

The fragility was real, though: the test pumped the event loop after each of 1,800 Steps and
painted 1,800 offscreen frames. Stepping and `refreshRun()` run inside the action, and only
painting waits for the loop, so the bulk loop now triggers Step directly and pumps every 100 Steps
and once at the end. Every assertion stays, including hidden-Results-not-rebuilt, which reads
`isVisible()`. The loop still paints 18 frames, and frame cost is `trafficsim-run-view-benchmark`'s
job. After: P-cores 2.8 s CPU, E-cores 11.8 s (one run each), against `TIMEOUT 90`, which is
unchanged.

## 2026-10-04 — Results export to CSV from the editor (D104)

The Results tab has an "Export results (CSV)…" action on its own toolbar, also in the
Simulation menu and the command palette. It writes `movementCsv(*runReport())`, the function
behind `trafficsim-cli --csv`, so the editor's file is byte for byte the CLI's, with the
not-yet-validated marker as its first line. It is enabled only once the run has reached its end
(`runFinished()`, which the partial note in `refreshResults()` now shares). The enabled state is
set in `refreshRun()`, because `refreshResults()` skips work while its tab is hidden. The write
goes through a new `writeEditorBytes` (QSaveFile, no direct-write fallback); `writeEditorDocument`
now calls it. A failed export reports `EDIT_CSV_WRITE` rather than `EDIT_FILE_WRITE`, whose text
speaks of unsaved project changes. `scenario-run-ui` asserts the action is disabled before a run,
part way through it and after Reset; that `exportResults` refuses a part-way run and writes
nothing; and that the finished run's file equals `movementCsv` of the same report. Desktop 77/77
on Windows (Debug).

Found on the way, not caused by this change: `scenario-run-ui` is now ≈100 s CPU on Windows Debug
with the machine idle (101 s at `eeb9c5c` without this change, 109 s with it; one run each). The
2026-10-02 figure was 28 s. It times out at 90 s alone. See NEXT. *Corrected in the entry above:
E-core scheduling, not code.*
