# Archived roadmap review — 2026-10-03

Moved whole from ROADMAP on 2026-10-09, when the [2026-10-09 review](../audits/ROADMAP_REVIEW-2026-10-09.md) replaced it. Its findings describe `eeb9c5c`; O1–O10 keep their numbers. O1, O3, O8 and O9 were answered by D130; O5, O6 and O10's remaining items by D143 (as R5, R7, R8); O2 (Q4) becomes M6.0; O4 waits in the owner sitting.

## Review record — 2026-10-03

**Reading this dated record:** review sections §1–§10 refer to the numbered subsections below. Findings, counts and test results describe `eeb9c5c` on 2026-10-03; they are not a fresh verification of HEAD. O1–O10 and S0–S5 remain proposals unless a later decision says otherwise. Combining these documents on 2026-10-04 does not approve them or change any gate.

**Updates since the review:** D104 implements single-run Results CSV export with the CLI's bytes; clipboard copy and batch export in S3 remain open. S4's input-table lane-share display is done (`laneSplit`, 2026-10-03). `NEXT.md` was regrouped on 2026-10-04; its current queue supersedes the old section layout and counts described here. The historical findings below are retained with their date.

A read-only review of `ROADMAP.md`, `NEXT.md` and the status lines that repeat them, at `eeb9c5c`. It has two halves: an **accuracy audit** (does each status match the decision log, the code, the tests and git?) and a **strategic review** (what stands between the product and the `PROBLEM.md` §4 sentence, and in what order?). The owner chose the output: verified factual errors are corrected in this session; **everything that needs judgement is a proposal here, for the owner.** Nothing is closed, re-ordered or carved out by this review, and no gate result is inferred.

**Method.** Six audit slices (M0–M1 closed, M1.22/M1.23, M2, M3, M4–M7 with layout, NEXT and PROGRESS) each compared one part of the documents against `PROGRESS.md`'s decision log, the archives, the code, the tests and `git log`. Every finding was then checked by an independent skeptic told to refute it; the M0–M1 slice had three per finding (documents, code, safety of the fix). 87 findings survived: **63 factual, 17 owner decisions, 7 strategic**; none was refuted outright, but several had their wording corrected. Three independent strategy lenses (the success-sentence walk, risk and effort, dependencies and the owner queue) fed one synthesis, which two adversarial critics then attacked; §7 below carries their corrections. Desktop 77/77 on Windows (MSVC 19.51, Qt 6.8.3, Debug) before any edit; this session changed documents only.

---

### 1. Summary

1. **The front half of the success sentence works; the back half has no product code.** Aerial image, four-leg drawing, counted interval volumes, fixed-time timing and a one-run movement delay table all exist. "Runs 10 seeds", "LOS" and "paste into a report without Excel" do not: `src/runner/` and `src/report/` hold only a README, `data/` has no LOS pack, and the editor has no copy or export (§5).
2. **Effort since M2's gate has gone to M3.2 behaviour and editor work, not to M5.** Of 122 non-merge commits since 2026-09-25, 45 touch `src/editor` or `src/shell`, 12 touch `src/eval` (all M3.2 work but one M2.6 performance change), and **0** touch `src/runner` or `src/report` (§6).
3. **No rule forces that order; `NEXT.md` does.** M5 is not behind a gate that is unpassed (M0's and M3's gates do not precede it), but `NEXT.md` puts five sections of owner looks first and marks engineering work "if asked", and `CLAUDE.md` tells a session not to re-plan. The order is the owner's call (decision O1).
4. **The documents had drifted.** 63 statements were stale or wrong: CLAUDE.md's schema 16 (it is 17), ROADMAP's "M3.2.8c — back to the owner" (the owner ruled, D102), M3's "M2.6 is still unperformed", M4.1 listing fixed-time controllers as open, wrong ctest names, superseded "Linux only"/"Windows only" notes. All are corrected (§3).
5. **Three gaps have no owner at all:** W74/W99 car-following (a `PROBLEM.md` §2 row no milestone builds), the M6 benchmark (Q4, open since 2026-09-10, "decide before M5"), and the evaluation period (warm-up, window, unfinished trips) that any averaged or LOS figure depends on (§4, §8).

---

### 2. Where each milestone stands

| Milestone | ROADMAP says | Evidence shows |
|---|---|---|
| M0 | Gate open | Open since 2026-09-11; owner observation not recorded. A fail triggers D13 (fix or replace car-following) |
| M0.1 | Implemented | Implemented; Windows desktop suites in CI since `e57b1b5` (corrected) |
| M1 | Accepted by ruling (D49) | Accepted by ruling; the written exercise (pockets, aerial image) never passed; no carve-out records that (O5) |
| M1.12 | Implemented | Implemented; no owner check of its gestures on Windows is recorded (corrected wording) |
| M1.22 | Open | Partly delivered: History, nudging, rotation; Connector tapers, route-safe reversal, a fixed shortcut set and Ctrl+K palette exist. Remaining list overstated (O10) |
| M1.23 | Open | Not started; no GeoJSON/OSM/Shapefile/PNG code |
| M1.26 | **CLOSED** | Implemented; its own gate (keyboard-only route/input gestures) is open — contradicts rule 1 (O5) |
| M2 | GATE, no status | Done-condition met in code; gate passed (D53, owner's word, no table on record: `plans/M2_GATE.md`). Heading carries no status (O5) |
| M2.1 | Open | Open; desired speed is one uniform min/max per type; no Link limits, lane types, decision station |
| M2.7 | Open | Implemented 2026-09-25; whether the owner's M2.6 use met its done-condition is not recorded (O4) |
| M3 | Done-condition | Shown on development evidence only; M3.2.7d sheet empty |
| M3.2 | Open | Rows through A46 green; D95/D101 implemented and off (D102); gate names "diverges" with no booked work; "calibrated gap acceptance" in scope but needs M6 (O6) |
| M3.2.9 | Implemented | Connector geometry filed under a right-of-way milestone; no M3_PLAN or M3_ACCEPTANCE rows (O6) |
| M4 / M4.1 | Open | Fixed-time controllers exist (M2.7b); actuated, detectors, intergreens, amber stop-or-go absent |
| M5 | Done-condition only | Not started: no runner, no CI, no LOS, no slices numbered |
| M5.1 | Open | Not started; its scope (transit stops, crosswalks) contradicts "Later … not inline" (O10) |
| M6 | GATE | Not started; no benchmark chosen (Q4) |
| M7 | Not started | `package.yml` builds a portable Windows folder and a Linux tree; no installer, no macOS build, no file association |

---

### 3. Corrections made in this session

Each was confirmed by an independent verifier against the decision log, code, tests or git; the wording that verifiers corrected is the wording applied. Nothing here closes or re-orders work.

- **ROADMAP.md** — M0 scope (four Links, two heads, two inputs in `crossing.json`) and the `scenario-run-ui` pin ("to within 1e-7", not "exactly"); M0.1 platform line; M1 status list (the reverted M1.17 was inside "M1.1–M1.20"; M1.21–M1.27 were missing; D80 replaced M1.18/M1.19's mouth); M1.12 and the M1.12.1 heading (it spanned the open M1.22/M1.23); "owner M1 acceptance remains open" ×3 → D49; M1.26's gate wording; M2's gate text (C2/C4 withdrawn, D51/D52), status date, "M3 may start", "M2's gate still precedes this work"; M2.1.1's "per-interval flows" and "wait for lane changing" (D93/D94); M2.7's "before M2.6" and click → Ctrl+right-click (D84), and a missing blank line that made its body render as a heading; M3's Preparation paragraph; the M3.2 table: D84 gestures, D72 and D86 added, platform notes superseded by recorded evidence, "back to the owner" → D102, D75's "dropped lane" → "the lane the drag ended on", `attachment-ui` → `editor-attachments`, `gesture-ui` → `editor-gestures`, D80 added to M3.2.9h; M4.1 listing fixed-time controllers as open.
- **CLAUDE.md** — schema 17; NEXT owner item 2 (not 4) for the name; M3.2.8c and M3.2.9 status; the engine bullet (D93/D94); authoring-only fields do ride in `core/types.hpp`; hard rule 5's reasoning is D7, not PRINCIPLES; the Layout block (`src/editor`/`src/shell` contents, runner and report marked planned, `data/` contents, `tools/`).
- **NEXT.md** — the WSL contradiction (`wsl.exe -l -v`: not installed here); D86, D80 and D83/D84 platform notes after the 2026-10-02 session check; PRs #76–#78; Qt versions in CI; D103's limit at 150/200 %; D101 beside D95; `Veytrix` "set aside", not rejected (D11); the D100 look moved from "can proceed without the owner" to the owner's items; M2.7 listed among open milestones.
- **ARCHITECTURE.md** — D49; D91's `SimState&&` overloads; schema 17 (it said 8 and 7); the model/eval/CLI rows; the M3 seam "design only, behind the M2 gate" → landed; demand row; the `data/` catalogue list.
- **M3_PLAN, M3_CONTRACT, M3_8_CONTRACT, M3_ACCEPTANCE, M2_PLAN** — status headers written before the M2 gate; D95/D101 "not implemented"/"being implemented" → implemented and off (D102); D93 "Windows only"; the M2.7a gesture.
- **VISSIM_PARITY §1a/§2, NETWORK_EDITOR, EDITOR_WORKFLOW** — overlap cycling is `Ctrl+Tab` since `ed74268`; plain `Tab` moves focus. §2 gains `Q`, `Ctrl+K`, `Ctrl+Shift+T/F`, the Help key, `+`/`-`. The measured §1a row says it was measured before the chord changed.
- **SPEC_AUDIT** (conflicts 8 and 12 settled since), **UI_REDESIGN_AUDIT** §5 (24 px controls, width-1 hairlines), **archive/ROADMAP-M1-implemented** (two "body is below" pointers that pointed at nothing; the bodies are at `bcead5d:docs/ROADMAP.md`), and the READMEs of `src/eval`, `src/model/control`, `src/model/demand`, `src/project`, `src/shell`.

**Left alone on purpose:** dated `PROGRESS.md` entries and decision rows (history, not status; D24 says "exactly" and the 2026-10-03 entry names one CI Qt version — both are records of their day), `network_commands.hpp`'s reversal comment (code), and every dated assessment section (`VISSIM_PARITY` §1/§3/§6, `M2_PLAN` §1).

---

### 4. Verified, but the owner's call

Recommendations are this review's; the choice is the owner's. Grouped so one chat answer can take each line by option letter.

| # | Question | Options | Recommendation |
|---|---|---|---|
| O1 | Does M5 (runner → editor batch → export) go ahead of further M3.2.8c behaviour and editor polish? | (a) yes, NEXT names it next · (b) keep NEXT's order · (c) alternate | **(a).** PROBLEM §2 calls it "the deliverable of the entire job"; D95/D101 shipped switched off |
| O2 | Q4: which published benchmarks define M6's tolerance? Its note says "decide before M5" | (a) HCM analytic references · (b) a published field dataset · (c) local Thai measurements · (d) (a) now, (c) later | **(d)**, from an option sheet a session prepares first (S0). Or override "before M5" explicitly in O1 |
| O3 | LOS: which pack, and may a letter sit on today's whole-route delay? | (a) letters on simulated movement delay — requires amending M5.1's gate line and the Results note, which say it is "not HCM control delay or LOS" · (b) letters only on a section-bounded delay (travel-time sections, D39's "M5 measurement") · (c) another pack too · (d) none before M6 | **(b)**, HCM first (D7). HCM thresholds differ by control type, so movements need a control-type tag |
| O4 | Did the M2.6 study build its timing in the controller dialog with every head placed by pointer (M2.7's done-condition)? | (a) yes → record in `plans/M2_GATE.md`, M2.7 closes · (b) no/unsure → observe it in the sitting | One line from the owner |
| O5 | Closure wording (rule 1) | M2 closed with M2.1/M2.7 carried as open sub-milestones, or open · M1.26 "implemented; gate open" instead of CLOSED · per-approach queue (D40) satisfies M2, per-movement queue to M5 · M1's unpassed written exercise carved or recorded · M2.0's commits before the criteria (`c1b122f`, `4e1567e` before `811e0db`): an allowed exception or a recorded lapse | M2 closed; M1.26 relabelled; queue to M5; record M1's ruling; M2.0 an exception (no gate observation was affected) |
| O6 | What closes M3.2? | Diverges: a numbered slice, or a ruling with an **acceptance-fixture row** (the D98 lab is a development bed, not evidence) · calibrated gap acceptance carved into M6 · M3.2.8c's remainder (visibility, `laneChangeDistance`, between-lanes state, D95 route (i)) into a new milestone · M3.2.9 to an editor milestone · the five T-junction clamps a numbered item or a NEXT note | Carve gap acceptance to M6 and the M3.2.8c remainder out, so M3.2 can close on M3.2.7d and its gate rows |
| O7 | Who owns W74/W99 car-following (PROBLEM §2)? No milestone builds it | (a) a numbered milestone, a new behaviour preset, frozen baselines pinned to the prototype · (b) M6 validates the prototype and PROBLEM §2 changes · (c) Later | **(a)** after M0's observation, before M6. Its screens must say permanently that Vissim-calibrated parameter sets do not carry over (rule 4) |
| O8 | Evaluation period: warm-up, evaluation window, unfinished trips | No code has a warm-up or window; the M2.6 template starts empty and runs its demand period; delay counts completed trips only, which reads low when a movement saturates | Decide before any LOS column (O3) |
| O9 | Seeds: keep "ten seeds" in M5/PROBLEM §4, or a CI half-width target? | (a) fixed 10 · (b) target half-width · (c) default 10, always show n and half-width | **(c).** The seed-to-seed SD of one movement's delay is unmeasured; D88's 7 s is the SD of a *change* between engine versions and does not size a table. Scenario comparison stays under "Later" (one random stream, so same-seed pairing across scenarios is not common random numbers) |
| O10 | One-line rulings | Close Q2 (D71/D95) · Q5 name, and whether registration (Q6) follows · plain `Tab` → focus as a Vissim departure (no D-row for `ed74268`) · 3D: non-goal (PROBLEM §5) or "Later" · M5.1's transit/crosswalks vs "Later" · amber stop-or-go and intergreens written into M4's scope (D36, `plans/M2_PLAN.md`) · scenario-JSON export from the editor wanted? · canvas below `workspace-ui`'s floor at 1080p/150 % · narrow M1.22's list; refresh `SPEC_AUDIT` (it predates M3.2) before it gates M1.22 · what M1.23's "CSV" means · ~~count sheet typed once vs D46~~ answered 2026-10-09: an opt-in `volumeFromCounts` per input, D46 otherwise (D142) · NEXT's ordering, and whether a non-gate look may ever close without the owner (only as a new D-row) | — |

---

### 5. The success sentence, step by step

> A traffic engineer opens an aerial image, draws a four-leg signalized intersection, enters
> counted turning volumes, sets the signal timing, runs 10 seeds, and gets a movement-level
> delay and LOS table they can paste into a report — without opening Vissim and without opening Excel.

| Clause | Status | Owner | Gap |
|---|---|---|---|
| opens an aerial image | works | M1 | Never exercised with a real aerial (D49's attempt had none). Images are stored as base64 PNG under a 32 MiB cap; a large photographic aerial may hit it — untested |
| draws a four-leg signalized intersection | works | M1 | — |
| enters counted turning volumes | works | M2.1.2/M2.2 | Inputs and decision counts are typed separately (D46, by design). Motorcycle-heavy Thai counts cannot be represented |
| sets the signal timing | partial | M2.7, M4 | Fixed-time only; amber runs as red (D36), which biases delay and causes clamps |
| runs 10 seeds | **missing** | M5 | One seed per run in the editor (20× playback cap) and the CLI. Seed loops exist only in developer tools, three times over |
| movement-level delay table | works (one run) | M2.5 | Whole-route delay incl. source wait and ≈3 s entry acceleration (D39). Queue is per approach (D40). A route through two junctions has no per-junction row (no nodes) |
| … and LOS | **missing** | M5 | O3, O8, O2 |
| they can paste into a report | partial | M5 | CLI CSV only; no copy/export in the editor |
| without opening Excel | **missing** | M5 | Averaging and CIs are done by hand today |
| without opening Vissim | partial | M6 | Every number carries the not-yet-validated marker until M6; no benchmark |

PROBLEM §2's behaviour rows: conflict areas **partial** (no front/rear gap, safety factor, visibility or red-red status; Vissim parity never measured); priority rules works (deterministic threshold, not a calibrated critical-gap model); signal heads anywhere works; **W74/W99 unowned** (O7); desired speed distributions partial (M2.1, unbooked); node evaluation and multi-run averaging as above. No reduced-speed areas or curvature speed limit exist, which biases turning-movement delay — the quantity M6's signalised-approach benchmark measures.

---

### 6. Where the effort went

Counts are `git log --no-merges --since=2026-09-25` (122 commits) by path; a commit can count in several rows.

| Area | Commits | Note |
|---|---|---|
| `src/editor` or `src/shell` | 45 | Connector geometry (D73–D80), interaction and UI polish (D81–D84, D99–D103) |
| `src/model` / `src/project` | 23 / 18 | Mostly right-of-way and lane-change compile paths |
| `src/eval` | 12 | M3.2 work (queue counters, D71, M3.2.8c diagnostics, D95/D101) and one M2.6 performance change; movement report untouched since `d046c2c` (2026-09-27) |
| `src/core` | 10 | Commitment, lane changes, cooperation (D69, D71, D90, D91, D95, D101) |
| `src/runner`, `src/report` | **0** | Untouched since the 2026-09-11 migration |

Since D39/D40 (2026-09-24), no decision row designs multi-seed reporting, LOS or an M6 benchmark; D88 sets a seed count for development comparisons only. PROBLEM §7's risk 3 ("engine starves the UI") is not happening; its inverse is — refinement is starving the deliverable. The risk on the other side stays real: an M5 table behind a weak editor still loses to Vissim, so the owner's open UI looks (§8) are not to be dropped, only scheduled.

---

### 7. Proposed sequence — owner-free sessions, after O1

Proposals; the owner books them in `NEXT.md`. One system per session, interface first. The critics' corrections are folded in: the Q4 sheet comes first so the recorded "decide before M5" is honoured, and carries no engine figures (choosing a benchmark the engine already matches would undo pre-registration, as D34's "blind" C0 avoided).

| # | Session | Milestone | Interface first | Done when |
|---|---|---|---|---|
| S0 | **M6 benchmark option sheet** (docs only) | prepares Q4 | `docs/evidence/m6-benchmark-options.md`: per M6 benchmark kind (signalised approach capacity/delay; unsignalised minor-movement capacity) ≥ 2 published options — quantity, source, the command that would produce the comparable figure. No engine numbers, no tolerance | PROGRESS Q4 points to it; the owner can answer by letter |
| S1 | **Batch runner and `trafficsim-cli --project F --seeds A-B`** | M5 (number a new slice, e.g. M5.2; M5.1's gate items — event accounting, multi-run checks — apply) | `src/runner/`: `runSeeds(scenario, spec, seeds) → per-seed MovementReport`; `aggregate()` in seed order → n, mean, SD, 95 % half-width per movement and approach. **Provenance:** replace the constant `engineVersion` ("0.2.0-cpp-m0" since `ab3423d`) with a build-stamped commit, and record compiler, seed list and per-seed clamps/pending/active. A seed that gridlocks or leaves trips unfinished is flagged, never silently averaged | Analytic aggregate test; permuted seed list gives the same result; one-seed batch equals today's `--project`; generated = completed + active + pending per seed; baselines untouched; `check_architecture` gets a runner rule with a negative fixture; ARCHITECTURE says where batch formatting lives (`src/report/` as reserved, or `src/project/` beside `movementCsv` — decide and record) |
| S2 | **"Run N seeds" in the editor** | M5 | Shell action over S1's API on a copy of the run snapshot; worker thread, aggregation in seed order (rule 2); Results shows n / mean / ±95 % | UI test: Results rows equal the CLI batch; an edit invalidates; cancel leaves no table claiming N runs; marker and "not HCM control delay or LOS" stay |
| S3 | **Copy table / Export CSV from Results** | M5 | TSV/CSV formatters beside S1's, first line the marker; clipboard and save | Test asserts the clipboard was set first, then that rows parse back; export bytes equal the CLI's; exported numbers C-locale while display cells follow `UI_REDESIGN_AUDIT` §6 — deliberately different, documented |
| S4 | **Input table shows the share it runs** | M1.26.1 | `refreshDemand` reads `laneShares` | The row shows the authored split, not the equal one, when shares are set (today it displays a figure the run does not use) |
| S5 | **Fill the session-fillable M3_ACCEPTANCE §4 rows; diagnose the five T-junction clamps** | M3.2 | Measurement before any fix | Commit/build/platform, fixture hashes and same-build replay rows filled; each clamp has a recorded cause. NEXT's do-not-retry (`comfortableDeceleration` for commitment, D69) is listed as a risk; before/after uses 40 seeds (D88) |

After O3/O8: the LOS pack as data (`data/los/`), a pure delay → letter function keyed by control type, approach and intersection rows; then travel-time sections. After O7: W74/W99 as a preset. Keyboard-only route/input gestures (M1.26's gate) remain owner-free and can slot in anywhere.

---

### 8. The owner queue, in one sitting

NEXT lists about 21 items waiting on the owner; ROADMAP, PROGRESS and the evidence file hold about nine more (strategy-lens counts, not recounted). Only O1–O3 and the M0 and M3.2.7d gates block anything. A session prepares the build and printed sheets, records the owner's words verbatim, and fills in no verdict itself.

- **A — decisions in chat:** O1–O10 by letter.
- **B — gate:** M0 on `data/scenarios/crossing.json` (acceleration, queue at red, discharge at green). First, because a fail triggers D13 before any W74/W99 or M6 work.
- **C — Run view, on the 125 % screen:** D90 (M2.6 East approach), D93 (pocket decision), D102 (lane-change slide), the grid's faintness.
- **D — `t-junction-priority.traffic.json`:** M3.2.7d with the `plans/M3_ACCEPTANCE.md` §3 sheet; D86 (drag, clear, Undo); D72.
- **E — drawing:** D80 at 45–179°; D83/D84 at 100/150/200 %, timing Ctrl+right against left-click for route and counter work (D84's failure condition); the PR #73–#75 cleanup; D96; D100; M1.19/M1.20 and `laneContains`; M1.12's gestures; M2.7's dialog if O4 is "no".
- **After S1–S3:** one rehearsal of the §4 sentence with a real aerial image, recorded with file, commit and table — labelled **partial** while LOS is undecided, so it is not read as §4 or M5 met. Outside engineers remain the way to strengthen the M2 gate (D8: "never wasted effort").
- **Guard:** each session adds at most one owner look, dated, with its failure condition as a yes/no question. NEXT points to ROADMAP and PROGRESS lines rather than moving gate status out of them (ROADMAP stays the authority, rule 1).

---

### 9. Risks the next sessions carry

- **Single-platform evidence** still stands behind several rows (A44's byte comparison on MSVC only; numerical replays are not run in CI). CLAUDE.md forbids reading one platform as both.
- **Gridlock:** queue gridlock is not prevented (M3.2.3c); D50 recorded a template with 445 vehicles never entering. A batch mean over a gridlocked seed is survivor-biased (S1 flags it).
- **`CONNECTOR_PARITY_AUDIT`** §3.5 (a Connector at a lane's start or end draws but does not run) and §3.6 (a zero gap time passes validation) are still live; Vissim itself has never been measured.
- **Thai studies:** motorcycles and per-approach speeds cannot be modelled; both are open questions in NEXT, not booked.

### 10. What this review did not check

No CLI or benchmark run (figures such as the 0.45 s M2.6 hour are quoted; D91 measured 557 ms later); HCM tables and procedures against the publications; whether the sweep tools can be rewired to a runner with byte-identical output; QClipboard under offscreen CI; whether a large real aerial fits the image caps; CI status at HEAD; the owner's M2.6 file and table (not in the repository). Strategy-lens counts other than those in §6 were spot-checked, not recounted.
