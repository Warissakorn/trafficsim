# Roadmap review — 2026-10-09

A review of whether [ROADMAP](../ROADMAP.md) still fits [PROBLEM](../PROBLEM.md), six days after the
[2026-10-03 review](../archive/ROADMAP-review-2026-10-03.md). Findings describe `fcabcef` (branch
`claude/optimistic-lovelace-fakdpv`: `main` at `c7628e8` plus D142). Same method as last time, by
the owner's earlier choice: **verified factual errors are corrected; everything that needs judgement
is a proposal for the owner** (§8, R1–R8, answerable by letter). Nothing is closed, re-ordered or
carved out here, and no gate result is inferred.

**Evidence used.** Decision rows D104–D142; `git log --no-merges --since=2026-10-03` (76 commits);
Linux, GCC 13, Qt 6.4.2 offscreen, Debug: `check` 110/110 at `fcabcef`; two 10-seed CLI batches run
for this review (§3, §5). Windows was not run; no owner observation was made.

**Corrections made with this review.** The 2026-10-03 record moved whole to the archive (ROADMAP
was at the 500-line limit, so this review could not sit beside it); ROADMAP's navigation and NEXT's
two links into the old record now point there; M2.8 now names D142.

---

## 1. Summary

1. **The back half of the success sentence now has product code.** Ten seeds, n and ±95 %, LOS
   letters, a table that copies into a spreadsheet and a CSV with the CLI's bytes all exist
   (D131–D134, D141), and a count sheet is typed once (D142). None has been used by the owner yet:
   that is M5.7, and it is the next gate that matters.
2. **The owner queue is the bottleneck, and it is growing.** Since 2026-10-03 39 decisions were
   recorded (D104–D142) and no owner look or gate was closed. NEXT now holds about 30 owner items;
   the M0 plausibility gate has been open since 2026-09-11 (§5).
3. **Three PROBLEM lines have no milestone that delivers them** (§4): "do it again for the
   with-project and mitigated scenarios" (PROBLEM §1 step 6) sits under *Later*; motorcycle-heavy
   Thai counts have no owner; desired-speed distributions are partial and unbooked.
4. **The biggest bias in a signalised study is still unbooked.** Amber runs as red (D36); every
   signalised delay in an M5 table carries it, and it was listed as "after M5" (M5_PLAN §3) without
   a slice number.
5. **A shipped example shows the unfinished-trip warning on 11 of its 12 movements** (§5): demand
   that runs to the end of the simulation always leaves trips in flight, so the 5 % rule cannot tell
   normal in-flight traffic from a stuck movement on a short run.

---

## 2. Where each milestone stands

| Milestone | ROADMAP says | Evidence at `fcabcef` |
|---|---|---|
| M0 | Gate open | Unchanged since 2026-09-11; no owner observation recorded |
| M1 | Accepted by ruling (D49) | Unchanged; M1.22/M1.23: no commit since 2026-10-03 |
| M2 | Gate passed (D53) | Unchanged; M2.1.3 positioned routing added (D119); M2.7: O4 unanswered |
| M2.8 | In progress | Slices 1–6 plus D142 (schema 26); the stacked PRs' owner reviews are open |
| M3 / M3.2 | Open | Display and Connector work D109–D118 (10 decisions); O6 unanswered, so nothing can close M3.2; M3.2.7d owner exercise not performed |
| M3.2.8a.1 | Open | D105/D108; 21 moving minor clamps remain |
| M3.3 | Contracts and W74 delivered | 15 decisions (D120–D129, D135–D139); W74 runs, uncalibrated, no cited preset |
| M4 / M4.1 | Open | No commit since 2026-10-03; amber stop-or-go and intergreens unbooked |
| M5 | Next by D130 | M5.2–M5.6 implemented in two days (D131–D134, D141); M5.7 owner rehearsal open |
| M5.1 | Open | Unchanged; transit/crosswalk scope still contradicts *Later* (O10) |
| M6 | Gate | Not started. Q4 waived for M5 by D130, still owed before M6; the S0 option sheet was never written |
| M7 | Not started | Unchanged: no installer, no macOS build, no file association |

## 3. The success sentence, step by step

| Clause | 2026-10-03 | Now | Remaining gap |
|---|---|---|---|
| opens an aerial image | works | works | Never exercised with a real aerial; 32 MiB cap untested on a photo (M5.7 records it) |
| draws a four-leg signalised intersection | works | works | — |
| enters counted turning volumes | works, typed twice | works, typed once if chosen (D142) | Motorcycles cannot be represented (§4) |
| sets the signal timing | partial | partial | Fixed-time only; amber as red (D36) biases delay |
| runs 10 seeds | **missing** | works (D131, D141) | Verified: `--seeds 42-51` on four-leg, 10 runs, 11 s Debug |
| movement-level delay table | works (one run) | works, n and ±95 % | Whole-route delay; section delay for LOS (D133) |
| … and LOS | **missing** | works on sections (D134) | Author must draw sections and set control type; whole-route delay never lettered |
| paste into a report | partial | works (Copy TSV, Export CSV) | Owner has not pasted one yet (M5.7) |
| without opening Excel | **missing** | works | Mean, SD, CI computed in the tool |
| without opening Vissim | partial | partial | Not validated until M6; no benchmark chosen |

Every clause now has code. The sentence is still **unproven**: no engineer has carried a real study
through it, and every number carries the not-yet-validated marker.

## 4. Fit with PROBLEM — what the roadmap does not deliver

| PROBLEM line | Roadmap | Finding |
|---|---|---|
| §1 step 6: "do it again for the with-project and mitigated scenarios" | *Later: scenario management and comparison* | The core workflow has three scenarios; the roadmap has one. A user can copy a file today, but nothing compares two batches. Placing it in *Later* contradicts the user's working day as PROBLEM describes it (R1) |
| §2 desired speed distributions | M2.1, "partial", unbooked | One uniform min/max per vehicle type; no Link speed limits or reduced-speed areas, which biases turning delay (2026-10-03 §5) |
| §2 Wiedemann car-following | M3.3 | W74 runs (D138); W99 contract next; no cited parameter preset, so no study can use W74 honestly yet |
| §2 node evaluation | M5 | Section-based delay and LOS by control type (D133–D134); no node object. Adequate for one intersection; a corridor needs one section per movement |
| Users are Thai consultants (NEXT §2 item 8) | no milestone | Motorcycle-heavy counts and lane sharing are unmodelled; PROBLEM does not mention it either. Either PROBLEM gains a line or the gap is a recorded non-goal (R3) |
| §7.1 "an engineer cannot complete a real study" | M2 gate passed, owner alone (D8) | Outside engineers were called "never wasted effort" (D8); none recruited. M5.7 is the same single judge |
| §7.2 fidelity validation | M6 | Nothing started; every batch figure is still unvalidated |

## 5. Risks

- **Owner-evidence debt.** About 30 items wait on the owner in NEXT (§1: 15 looks; §2: 8 decisions;
  the 2026-10-05/06 and M5 sections: about 7 reviews). None closed since 2026-10-03, while 39
  decisions were added. Each new owner look is cheap to add and expensive to clear; the queue's age
  makes old looks less likely to be done (R4).
- **The unfinished-trip warning on short runs.** `four-leg-signalised` (900 s, demand until the end,
  no evaluation period, schema 17) run with `--seeds 42-51`: 11 of 12 movements carry the
  `# WARNING: unfinished trips over 5 %` line; mean unfinished per movement ranges 0.2–18.1 vehicles. The
  60-minute M2.6 template gives none. On any run whose demand stops at the simulation end, trips in
  flight count as "unfinished", so the warning reads as a fault where there is none. Vissim practice
  is a cool-down after the evaluation window (R6).
- **Single-platform evidence.** This review and the last three slices were verified on Linux only;
  Windows CI runs per PR, and the owner's machine is Windows.
- **Gridlock** is still not prevented (M3.2.3c); batches flag overloaded seeds (D131) but the means
  include them by the owner's choice.
- **Live defects** in [CONNECTOR_PARITY_AUDIT](CONNECTOR_PARITY_AUDIT.md) §3.5 (a Connector at a
  lane's start or end draws but does not run) and §3.6 (a zero gap time passes validation).

## 6. Where the effort went, 2026-10-03 to 2026-10-09

76 non-merge commits. By path (a commit counts in several rows): `docs` 69, `src/project` 24,
`src/shell` 21, `src/model` 21, `src/core` 15, `src/editor` 14, `src/eval` 13, `src/runner` 4,
`src/report` 0 (batch formatting lives in `src/project/`, D131). By decision: driving behaviour 15
(D120–D129, D135–D139), conflict display and Connectors 10 (D109–D118), M5 6 (D130–D134, D141),
clamps 4 (D105–D108), other 4 (D104, D119, D140, D142). M5 moved from zero to implemented in two days once the
owner ordered it (D130): the ordering rule, not capacity, was the constraint.

## 7. Proposed sequence after M5.7 (proposals; the owner books them in NEXT)

| # | Work | Why now | Owner-free? |
|---|---|---|---|
| P1 | M5.7 rehearsal, prepared by a session (checklist, template copy ready for real volumes) | It is the first real use of everything in §3 | Prepare yes, close no |
| P2 | Evaluation cool-down / unfinished-trip rule (contract first) | §5: the shipped example warns on every row | Yes, after R6 |
| P3 | Amber stop-or-go (D36), its own contract and failure-first rows | Biases every signalised delay the M5 table reports | Yes, after R2 |
| P4 | M6 benchmark option sheet (the 2026-10-03 S0), docs only, no engine numbers | Q4 is still owed before M6, and M6 is what lets a number reach a regulator | Yes |
| P5 | Scenario comparison: two batches, one difference table | PROBLEM §1 step 6 (R1) | Yes, after R1 |
| P6 | W74 cited preset, then W99 contract | M3.3's own next step; needs a published source | Yes |
| P7 | Motorcycles: a PROBLEM line, then a milestone, or a recorded non-goal | Thai counts (R3) | No: owner decision first |
| P8 | M7 installer | A non-developer cannot install today | Yes, when the owner wants outside users |

## 8. Owner questions (answer by letter)

**Answered 2026-10-09: every recommendation accepted (R1 a, R2 a then b, R3 c, R4 a with b, R5 a,
R6 a, R7 a, R8 a), recorded as [D143](../decisions/RECORD.md#d143).**

| # | Question | Options | Recommendation |
|---|---|---|---|
| R1 | Scenario comparison (PROBLEM §1 step 6) | (a) a numbered M5 slice now · (b) stay in *Later* and amend PROBLEM §1 · (c) after M6 | **(a)**: the user's job has three scenarios |
| R2 | What follows M5.7? | (a) P2+P3 (correct the numbers) · (b) P4 (validation path) · (c) P5 (scenarios) · (d) P6 (W74/W99) · (e) P8 (installer) | **(a) then (b)**: fix known biases before choosing a benchmark the engine must meet |
| R3 | Motorcycles and lane sharing | (a) PROBLEM line + milestone · (b) recorded non-goal for now · (c) decide after M5.7 | **(c)**, but record the question in PROBLEM, not only NEXT |
| R4 | Owner-look debt | (a) one sitting with a printed sheet, as the 2026-10-03 review §8 proposed · (b) looks older than 14 days become carve-outs in ROADMAP (rule 2) · (c) keep as is | **(a)**, with (b) for whatever the sitting does not reach |
| R5 | Closure wording still unanswered from O5 | (a) apply the 2026-10-03 recommendation (M2 closed with M2.1/M2.7 carried; M1.26 relabelled) · (b) leave open | **(a)** |
| R6 | Unfinished trips on short runs | (a) cool-down: simulation runs past the evaluation end until trips that entered in the window finish (bounded) · (b) count only vehicles stopped or waiting at the source · (c) keep and reword the warning | **(a)**, as Vissim studies do; contract and rows first |
| R7 | What closes M3.2 (O6) | (a) carve calibrated gap acceptance to M6 and the M3.2.8c remainder into a new milestone, close on M3.2.7d and its rows · (b) leave open | **(a)** |
| R8 | M1.22/M1.23 and M5.1 scope (O10) | (a) narrow M1.22 to what is unbuilt, define M1.23's CSV, move M5.1's transit/crosswalks to *Later* · (b) leave | **(a)** |

## 9. What this review did not check

Windows; the owner's M2.6 file; HCM tables against the publication; whether a real aerial fits the
caps; Vissim behaviour; any performance figure beyond the 11 s Debug batch above.
