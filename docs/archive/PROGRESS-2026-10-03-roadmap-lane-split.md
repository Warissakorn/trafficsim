# Archived PROGRESS entries

## 2026-10-03 — Roadmap review: 63 stale statements corrected, the order left to the owner

At the user's request, a session reviewed the roadmap for accuracy and direction. Report:
[ROADMAP.md — review record](ROADMAP-review-2026-10-03.md) (consolidated 2026-10-04). Six audit slices compared ROADMAP, NEXT and the status
lines that repeat them against the decision log, archives, code, tests and git. An independent
skeptic checked each finding, and 87 survived: 63 factual, 17 owner decisions, 7 strategic.
- **Corrected (docs only):** CLAUDE.md said schema 16 (it is 17) and owner item 4 for the name
  (it is 2). ROADMAP still said "M2.6 is still unperformed" (M3), "back to the owner" (M3.2.8c,
  ruled by D102), "owner M1 acceptance remains open" (D49), listed fixed-time controllers as open
  (M4.1), and cited ctest names that do not exist (`attachment-ui`, `gesture-ui`). ARCHITECTURE said
  schema 8 and called the M3 seam "a design only". NEXT said WSL both exists here and does not
  (`wsl.exe`: it does not). Overlap cycling has been `Ctrl+Tab` since `ed74268`, and four docs said
  `Tab`. The full list is in the report's §3.
- **Not corrected, on purpose:** dated PROGRESS entries and decision rows, which are records of
  their day; one code comment; and anything that closes, carves or re-orders a milestone. Rule 1
  and hard rule 8 make those the owner's. The ten owner questions are the report's §4.
- **Why the order is not changed:** the strategic finding is that M5 (multi-seed runs, CIs, LOS,
  export) has no product code and no commit since 2026-09-25, while 45 of 122 commits touched the
  editor or shell. No gate holds M5 back. NEXT's order does, and CLAUDE.md forbids a session to
  re-plan, so moving M5 up is O1, the owner's.
- **Critics' corrections kept in the report:** M5 is not "gate-free". The M0 and M5.1 gate lines
  apply, but neither precedes M5. D88's 7 s SD is a per-seed *change* between engine versions, and
  does not size a single table's CI. The Q4 option sheet comes before the runner and carries no
  engine figures, so the benchmark is not picked by what the engine already matches.
Desktop 77/77 on Windows before the edits (MSVC, Qt 6.8.3, Debug). This session changed no code.

## 2026-10-03 — The input table shows the lane split the run gets (`laneSplit`)

With per-lane weights set, the Vehicle inputs row still printed the equal split ("1800 = 2 × 900.0"),
because it had its own copy of the split rule. The rule now lives once, in
`laneSplit(chainLanes, shareCount, laneShares)` (network.hpp, compile.cpp). It is used by the compiler,
by routeless inputs (`demand_paths.cpp`, every Link lane a "chain") and by `refreshDemand`, which
prints "1800 = 1200.0 + 600.0" when weights apply and the old "N × v" when they do not. It returns
`weighted` as well as the fractions, because the equal split must stay `total / n`: `total * (1/n)`
can differ in the last bit, which would move a Poisson spawn and break byte-identical replay. Seed 42
CLI output is `cmp`-identical before and after for the four shipped projects, the D93 scratch case,
and two scratch cases with weights set (routed {2,1}, routeless {3,1}). Tests: `laneSplit`
fractions and fallbacks (editor group), and `demand-ui` asserts the row before and after the 2:1
weight, after asserting that the compiled split really is 1200/600. Desktop 76/77 on Windows:
`scenario-run-ui` timed out at 90 s under machine load, and an alternating A/B of the unchanged
and changed builds measured both at 110–116 s CPU (NEXT).
