# M2 plan — and the M1 review it starts from

Written 2026-09-24 at the owner's request ("ตรวจสอบ M1 และวางแผนพัฒนา M2"). **§3 was the draft;
the owner ratified it as drafted the same day and it is now registered in `ROADMAP.md` §M2
(D34).** The same day, before any observation, the owner withdrew C0 and C3 and re-registered
the gate as C1 + C2 with C4 recorded (D38); §3 below is the registered version.

---

## 1. M1 review — where it actually stands

### Verified this session (Linux only)

| Check | Result |
|---|---|
| `cmake --preset desktop` + build, clean container | Green, after installing `nlohmann-json3-dev`, `qt6-base-dev`, `qt6-tools-dev`, `ninja-build` (the container had none of them) |
| `ctest --preset desktop` | **36/36 pass** (offscreen Qt) |
| `cmake --build build/desktop --target check` | Pass |
| `trafficsim-cli 42` | Runs, reports `"validation": "not-yet-validated"`, 0 safety clamps |
| CI on `main` | Run 207 (PR #54) green on Linux and Windows; run 211 (PR #55) was in progress at review time |

Windows was not exercised here. A green Linux run is not Windows evidence (CLAUDE.md).

### What is closed

Every numbered M1 sub-milestone except M1.22 and M1.23 is implemented: M1.1–M1.21.1, M1.24–M1.27,
M1.26.1 and the carve-outs M1.3.1, M1.5.1, M1.11.1, M1.12.1; M1.12.2 was a measurement error and
M1.12.3 was closed by M1.19. ROADMAP is the authority for each.

### What keeps M1 open

| # | Item | Owner | Blocks |
|---|---|---|---|
| G1 | The timed four-leg / aerial-image / reopen exercise (`M1_ACCEPTANCE.md`) — **every row is still "Pending"** | Owner | M1 closure |
| G2 | M0 plausibility observation (accelerate, queue at red, discharge at green) | Owner | M0 closure |
| G3 | Keyboard-only equivalent of the route and input gestures (M1.26 gate) | Engineering + owner | M1.26 gate |
| G4 | M1.22 (spline/arc, snapping, layer locks, multi-property inspector …) and M1.23 (import/export, culling/LOD) | Engineering | Their own gates; not the M1 done-condition |
| G5 | Windows-native appearance, display scaling and the M1.12 review checklist | Owner | Honest cross-platform claim |
| G6 | Name (Q5, D11) — trigger is live | Owner | Nothing technical |

### Engineering defects found or confirmed

1. **Duplicate-station refusal still unfixed** (`NEXT.md` item 0). `src/model/network/sections.cpp`
   still measures a second arrival at an already-cut station against
   `boundaries.back() + kMinSectionLength`, i.e. against itself, and marks the Connector
   unsectionable. Run is refused for a physically fine drawing.
2. **No fixture built a four-leg intersection — now one does (below, M2.0 item 2).** Until then
   M1's done-condition geometry and M2's done-condition network had never been compiled or run by
   any automated check; `crossing.json` was the only runnable scenario.

### Verdict

M1 is **implementation-complete for its done-condition and gate-open**. No merged code can close
it; G1 can. The four-leg fixture now shows such a drawing **runs** — but only drawn with its
turns staggered along each exit (M2.0.1), which is M2's first real question.

---

## 2. What M2 has to deliver, and what the engine can already say

ROADMAP M2: *vehicle inputs per interval, compositions, turning proportions; press Run, get average
delay and queue per movement. Done when the M1 intersection, loaded with counted volumes, runs and
produces a delay table.* Then the honesty gate.

| Need | Today | Gap |
|---|---|---|
| Volumes per interval | `VehicleInput` = one route, one type, one rate over `[start, end)`. Several inputs can emulate intervals. | No interval list on one input; no 15-min table an engineer can paste counts into |
| Compositions | One vehicle type per input | No composition object; emulation = one input per type, which multiplies inputs by types |
| Turning proportions | One input per route = absolute volume per movement | No "relative flow per destination" object; the engineer must pre-multiply counts |
| Lane choice at pockets | Emergent: `routeLaneChains` only expands to lanes whose Connectors reach the destination; inserted lane is fixed (no lane changing) | Unverified on a real four-leg drawing (defect 2) |
| Crossing movements | Not modelled — paths pass through each other; signals keep them apart | **Only fully protected signal phasing is honest in M2.** Permissive turns, unsignalised legs, conflict areas are M3 |
| Delay per movement | `SummaryAccumulator`: one completed-trip mean for the whole run | No movement definition, no per-movement table, no queue measurement |
| Several seeds | CLI takes one seed | Averaging and confidence intervals are **M5**, not M2 |

**Consequence for the gate:** the only study M2 can run honestly is a **signalised intersection
with fully protected phasing**. That is enough to show whether the tool carries a real study
from blank network to report table (`PROBLEM.md` §7.1).

---

## 3. Gate criteria (registered; C0 and C3 withdrawn by D38)

Proposal only. Numbers in **[brackets]** are the owner's to set. Copy the accepted version into
ROADMAP §M2 and commit it **before any M2.2+ code** — a criterion written after results exist is
not a test (D8).

- **C1 — Completion.** One real study from the owner's practice (signalised, protected phasing,
  counted 15-minute volumes, the owner's actual timing plan) is completed end to end in TrafficSim:
  network over its aerial image, volumes, composition, timing, Run, per-movement delay and queue
  table. Fail if it needs hand-edited JSON, a code change during the study, or outside help.
- **C2 — Effort.** Wall time in TrafficSim ≤ **[2.0]×** the time in the owner's current tool for
  the same study, both timed from a blank project. Record both regardless of outcome.
- **C4 — Plausibility, recorded, not scored.** For each movement, the TrafficSim delay next to the
  current tool's. Neither engine is validated against the other and bit-agreement is a non-goal
  (`PROBLEM.md` §5), so C4 cannot pass or fail the gate; a movement more than **[two LOS
  letters]** away opens a numbered investigation milestone, per ROADMAP rule 2.

**Pass = C1 and C2 pass.** Reported as *not disproven*, never
*confirmed* (D8). Evidence goes in a new `docs/M2_GATE.md`, shaped like `M1_ACCEPTANCE.md`.

---

## 4. Slices, in order — each one system, each after the criteria are committed

Numbering continues after the existing **M2.1**, which stays open and later (behaviour
parameters, distributions, road classes, lane types, dynamic and positioned routing). **Proposed
rescope, owner to confirm:** compositions and the static routing decision move from M2.1 into
M2.3/M2.4 below, because M2's done-condition needs them and M2.1's gate does not.

### M2.0 — Preconditions (engineering items allowed before the criteria, since they are M1 defects)

1. **Done 2026-09-24:** the duplicate-station refusal (NEXT item 0). The second arrival reuses
   the cut, and gives way to the first as well as to the lane (`derivedPriorityRules`);
   `two_connectors_arriving_at_one_station_share_one_cut` is the specification now.
2. **Done 2026-09-24:** `data/projects/four-leg-signalised.traffic.json`, built by
   `tools/four_leg_network.hpp` through the editor's own commands and regenerated with
   `trafficsim-four-leg-fixture`; `fourleg` in `trafficsim-tests` holds it. Left-hand traffic,
   four legs of a 2-lane approach → taper → 3-lane right-turn pocket (44 m) → split-phase heads →
   2-lane exit; 12 movements, 120 s cycle (30/30/20/20 green, 3 s amber, 2 s all-red), 2090 veh/h
   for 900 s. It opens in the editor, reopens identically, raises **no** diagnostic, compiles to
   16 runtime routes and 8 derived priority rules, delivers every movement (461 trips at seed 42,
   none left pending) and replays exactly. Movement delays of roughly 35–56 s came out of a
   throwaway probe, not the test, and are **unvalidated** — they include acceleration loss and are
   not HCM control delay. What it surfaced, each its own item:

   - **M2.0.1 — Done (D35):** Connectors meeting at a lane start are ordered by M3.1's derived
     rule in drawing order; the fixture is now the natural drawing. Original finding:
     **Turns meeting an exit at its start are refused (8 × `UNSUPPORTED_MERGE`).** That
     is how an engineer draws this intersection, and `fourleg.natural_drawing_is_refused…`
     pins it. The fixture runs only because its left and right turns join the exit 10 m and
     20 m along the body, which derives M3.1 rules; on screen that is a short overlap of
     Connector on Link. An engineer in the C1 study would not discover that without help, so
     either C1 allows it, or a **minimal arbitration for end-arrivals** is pulled forward — the
     M3.1 rule derived for a start-of-lane merge as it already is for a body merge. Under split
     phasing no two of those movements are green together, so the rule rarely binds. Conflict
     areas themselves stay M3. **Owner decision.**
   - **M2.0.2 — An implied Link→Link step silently drops an ambiguous lane. Fixed:** the lane
     is still dropped (guessing would author a route nobody chose) but reported as the advisory
     `AMBIGUOUS_ROUTE_STEP`; Run is not blocked. The fixture names the taper or pocket entry.
   - **M2.0.3 — 4 safety clamps in 900 s. Diagnosed; owner decision.** Every one is a vehicle
     under 2 m from its stop line at 4–12 m/s when its head turns amber — amber is treated as red
     with no stop-or-go decision (`SIMULATION.md`). Not the M3.1 rules, not the merges. The frozen
     TS baselines (seeds 43, 4294967295) hold six of the same clamps, so a decision changes
     frozen fixtures and needs the owner: keep it until M4, or accept it now with a logged new
     baseline. Its effect on M2.5 delay is small but real — a halted vehicle waits a red it would
     have cleared. `fourleg.every_safety_clamp_is_…` fails if a clamp has any other cause.

### M2.2 — Time-varying volumes · **Implemented 2026-09-24**

Done as below: `VehicleInput.intervals`, derived scalars (`deriveInputTotals`), `id/int-k`
expansion, schema 10, and a counts box in the input dialog (paste one count per interval, length
in minutes, default 15). Frozen fixtures and `trafficsim-cli 42` unchanged.

`VehicleInput` gains an ordered interval list (start, end, veh/h); a single-interval input is
exactly today's input. Schema 10 with migration; Poisson per interval with the existing PRNG and
draw order. **Gate:** the four frozen baselines and `trafficsim-cli 42` byte-identical; an interval
table in the input dialog accepts pasted 15-minute counts.

### M2.3 — Vehicle compositions · **Implemented 2026-09-24**

Done as below, with `urban-mixed` (95% car, 5% heavy vehicle) and `car-only` as content; still
schema 10, which had not been released. Open question for the owner: Thai counts are
motorcycle-heavy, and a motorcycle without lane-sharing behaviour would be a claim the engine
cannot back — so none is shipped.

Compositions as content under `data/compositions/` (type + relative flow). An input names a type
**or** a composition. **Gate:** single-type inputs draw nothing extra from the PRNG (fixtures
unchanged); sampled shares converge to the configured ones over a long run within a stated
binomial tolerance.

### M2.4 — Static turning proportions · **Implemented 2026-09-24**

Done as below: `routingDecisions` with `routingDecisionId` on an input, split into
`id/route-<route>` before compositions, periods and lanes; a Routing decisions tab and dialog,
and decisions in the input's route list. A multi-route decision drops `laneShares` (weights for
one route's lanes mean nothing on another's). Still schema 10.

A static routing decision on an origin Link: destination routes with relative flows, per interval.
It feeds the existing per-route inputs at compile time; it does **not** restore lane-specific
routes (that is M2.1's positioned decision, D25). **Gate:** flows compile to the same core
scenario as the equivalent hand-split inputs; tables and dialogs round-trip through save/reopen.

### M2.5 — Movement evaluation, single run · **Next**

In `src/eval/`, fed only by the event stream (`core/` unchanged):

- **Movement** = (entry Link, exit Link) pair derived from the compiled route, named by the Link
  names the author gave.
- **Delay** per movement = travel time between two cross-sections (approach and exit) minus the
  free-flow time at each vehicle's desired speed, plus source wait. Labelled as *simulated
  movement delay*, **not HCM control delay**, until M5/M6 say otherwise.
- **Queue** per approach = Vissim-style queue counter at the stop line: queue start/end speed
  and maximum gap as content in `data/`, reporting mean and maximum length in metres.
- Unserved demand (pending, active at end) reported beside every table.

Shown in the editor after Run and printed/exported by the CLI (CSV), always with the
not-yet-validated marker (rule 4). Several seeds, means, confidence intervals and LOS stay
**M5**. **Gate:** analytic fixtures — a single-lane approach under a fixed red with known arrivals,
whose delay and queue can be computed by hand — plus exact replay per seed.

### M2.6 — The gate study (owner)

Run C1, C2 and C4 exactly as committed; fill in `docs/M2_GATE.md`; close M2 only on a pass. **M3 may not
start before this** (ROADMAP §M2).

---

## 5. Owner decisions this plan needs

1. **Accept, edit or reject the criteria** (done: D34, then D38), fill the bracketed numbers, commit them into ROADMAP §M2.
2. **Confirm the M2.1 → M2.3/M2.4 rescope**, or keep compositions and routing decisions in M2.1.
3. **Seeds for the gate study:** run one seed (M2 scope) or pull a minimal multi-seed mean
   forward from M5 so C4 compares like with like. Averaging with CIs is still M5's either way.
4. **M2.0.3:** amber stays red until M4, or an amber decision now with a new logged baseline.
5. **M2.0.1:** accept staggered turn arrivals in the gate study, or pull start-of-lane merge
   arbitration forward before it.
6. **Order relative to G1:** recommended — do G1 first; its saved drawing *is* M2.0's fixture and
   M2's done-condition network, and it tests the editor on a network nobody has yet run.
