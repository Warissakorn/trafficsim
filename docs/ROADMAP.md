# ROADMAP — Velk

Not a schedule. A **sequence**, so that any session can see where it sits and what closes
the milestone it is in.

**Two rules, inherited from a prior effort that broke both and paid for it:**

1. A milestone closes **only** when its done-condition is met and its gate has passed.
   Merged code is not a passed gate.
2. If a milestone must ship incomplete, the missing part is **carved out into a new numbered
   milestone in this file, immediately**, not left as a note. "We'll come back to it" is how
   a roadmap stops being true.

---

## M0 — Vertical slice

The smallest thing that is genuinely a traffic simulator, end to end. Every layer exists in
miniature, so wrong assumptions surface while they are still cheap.

**Scope:** two crossing links · one connector each way · one fixed-time signal · one vehicle
input · Wiedemann-style car-following · a canvas showing vehicles as dots · a headless run
that prints average delay.

**Done when:** `npm run dev` shows vehicles accelerating, queueing at red, and discharging at
green in a way a traffic engineer would recognise as plausible — and `npm test` proves the
same seed produces the same run.

**Explicitly not in M0:** lane changing, editing anything, saving anything, LOS, multiple
seeds, priority control. The slice is about the *shape*, not the feature set.

---

## M1 — Network editor

The Vissim modelling surface, natively: links are first class, connectors are real objects,
junctions are not something the user places.

**Done when:** an engineer draws a four-leg intersection with turn pockets from scratch, over
an aerial image, in under 10 minutes, without reading documentation — and reopening the file
gives back exactly what they drew.

**Includes, because D7 makes it non-optional:** left-hand / right-hand traffic as a project
setting that actually drives connector and conflict geometry. Retrofitting this later touches
every geometry routine.

---

## M2 — Demand, run, first numbers · **GATE**

Vehicle inputs per interval, compositions, turning proportions. Press Run, get average delay
and queue per movement.

**Done when:** the M1 intersection, loaded with counted volumes, runs and produces a delay
table.

**GATE — the honesty check.** Before M3 starts, a practising traffic engineer completes a
small **real** study in this tool and in their current tool, and answers directly whether the
free SUMO-based alternative would have been good enough for this job.

> **This gate is currently performed by the project owner alone (D8), which makes it weak** —
> the person judging is the person who chose to build an engine. It is therefore run as a
> **pre-registered** test: **the pass/fail criteria are written into this file and committed
> before any M2 implementation begins.** Criteria decided afterwards are not a test. A pass
> under these conditions means "not disproven", never "confirmed". Recruiting outside
> engineers later strengthens the gate and is never wasted effort.

**Pre-registered criteria: TO BE WRITTEN before M2 implementation starts.** Leaving this
line unfilled and starting M2 anyway voids the gate.

- If the answer is broadly *yes*, this project is the wrong answer to the problem —
  see `PROBLEM.md` §7.1 — and the honest move is to stop and reconsider, not to continue
  because effort has been spent.
- This gate exists because the whole justification for owning an engine (`PROBLEM.md` §2)
  is an argument, not yet an observation.

---

## M3 — Right-of-way: conflict areas and priority rules

**The differentiating milestone.** Everything a SUMO wrapper structurally cannot do.

- Conflict areas as **editable input**: at each conflict point, choose which movement yields,
  or make it undetermined.
- Priority rules with real **gap time and headway in seconds and metres**.
- Stop and yield control.
- Signal heads placed **anywhere on a link**, not only at a stop line.

**Done when:** an unsignalized T-junction with a minor-road left turn produces plausible,
tunable minor-road delay that responds correctly to changing the gap time — and the same
network is demonstrably not expressible in a SUMO wrapper.

---

## M4 — Signal control

Controllers, signal groups, programs, fixed-time and actuated, detectors, ring-barrier.

**Done when:** an eight-phase two-ring, two-barrier plan is built from a real timing sheet in
under 8 minutes with zero validation errors, and runs.

---

## M5 — Evaluation and reporting

The reason the whole project exists (`PROBLEM.md` §4).

- Movement-level delay, LOS, queue length, travel time.
- Multi-seed batch runs with means and confidence intervals.
- Report tables that go into an impact study without passing through a spreadsheet.
- LOS thresholds as swappable per-jurisdiction data, never compiled in.

**Done when:** one intersection, ten seeds, one command → a movement-level LOS table with
confidence intervals, ready to paste into a report.

---

## M6 — Calibration and validation · **GATE**

**Done when:** the engine reproduces published benchmark results — capacity and delay for a
signalized approach, gap-acceptance capacity for an unsignalized minor movement — within a
stated tolerance, and the tolerance is published in the docs and shown in the app.

**This is a hard gate.** Numbers from an unvalidated engine must never reach a regulator.
Until M6 passes, every results screen carries a permanent "not yet validated" marker.

---

## M7 — Desktop packaging

Offline install, native file dialogs, no server required.

**Done when:** a non-technical user installs from a single file on Windows and macOS, opens a
project by double-clicking it, and works with no network connection.

> Desktop is **last as a milestone but first as a constraint.** The stack is chosen so this
> is packaging work, not a rewrite (decision D3). A smoke test that the app builds and boots
> in a desktop shell runs from M1 onward, so this never becomes a surprise.

---

## Later, with their own milestones — not to be started inline

Recorded so nobody starts them opportunistically:

- Pedestrian and public-transport modelling
- Regional/macroscopic assignment
- Scenario management and comparison
- 3D presentation
- Collaboration
