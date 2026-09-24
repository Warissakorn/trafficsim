# PROBLEM — what this is for

> Read this before ARCHITECTURE or ROADMAP. Everything downstream is an answer to
> something on this page. If a proposed feature does not trace back to a line here,
> it is out of scope until this document is changed first.

---

## 1. Who this is for

Traffic engineers and consultants who produce **traffic impact studies** — TIA, EIA,
intersection redesigns, signal retiming, development-access studies. Their working day is:

1. Draw a road network over an aerial image.
2. Enter counted volumes and turning proportions per time interval.
3. Set up priority control or signal timing.
4. Run the microsimulation, several times, with different random seeds.
5. Report **delay, level of service, and queue length per movement**, averaged across runs.
6. Do it again for the "with project" and "mitigated" scenarios.

Today that work is done in **PTV Vissim**. It works, and these users know it well.

## 2. The problem

**Vissim costs more than most of these users can justify, and no affordable tool gives them
the modelling surface and the output their studies require.**

A tool is usable for this work only if the engineer can express the intersection as they
actually analyse it, and get out the numbers the report needs:

| What the engineer needs | Why the study needs it |
|---|---|
| **Conflict areas** — pick which movement yields at each individual conflict point | Right-of-way at a real junction is a local engineering judgement, not something to be inferred from a junction type. The engineer must be able to set it and see its effect. |
| **Priority rules** — set gap time and headway in seconds/metres | These are the parameters minor-road delay is tuned with and justified by in a report. |
| **Signal heads anywhere on a link** | Mid-block crossings, staggered stop lines and pre-signals are ordinary study objects. |
| **Wiedemann car-following** | Calibration practice and published parameter sets are expressed in W74/W99 terms. |
| **Desired speed distributions** | Speed belongs to the vehicle and its driver, and every study dialog assumes it. |
| **Node evaluation** — delay/LOS/queue per movement | The table that goes into the report. |
| **Multi-run averaging with confidence intervals** | A single stochastic run is not a result. |

The last two are the deliverable of the entire job: **the two things the user is actually
paid to produce.** The first five are what makes those numbers trustworthy for a given
intersection.

## 3. What this project is

**A traffic microsimulator with its own simulation engine, built to the model Vissim users
already think in, usable in real engineering work**, with the evaluation output that traffic
impact reporting requires.

Its own engine, so that conflict areas, priority rules, signal placement, driver behaviour,
and per-movement evaluation are all first-class inputs rather than things worked around.

This is the expensive answer, chosen deliberately. See decision **D1** in
[`PROGRESS.md`](PROGRESS.md) for the reasoning, and **D38** for the current statement of
purpose.

## 4. What "done" looks like

The success condition for the whole project, in one sentence:

> A traffic engineer opens an aerial image, draws a four-leg signalized intersection,
> enters counted turning volumes, sets the signal timing, runs 10 seeds, and gets a
> movement-level delay and LOS table they can paste into a report — without opening
> Vissim and without opening Excel.

Every milestone in [`ROADMAP.md`](ROADMAP.md) is a slice of that sentence.

## 5. Non-goals

Stated now so that later sessions do not quietly drift into them:

| Not doing | Why |
|---|---|
| Bit-exact agreement with Vissim | Different code is different code. Every model must be recalibrated to local field data. Claiming parity would be false and would be believed. |
| Regional/macroscopic assignment (Visum-like) | A different product. Possible later, never inline. |
| 3D presentation | Presentation value only; enormous cost. |
| Multi-user collaborative editing | Needs its own architecture. Not before the single-user tool is good. |
| Importing Vissim `.inpx` files | Proprietary, unstable, and would tie the data model to someone else's. Users rebuild networks; that is accepted. |
| Being a general-purpose simulation platform | Scope is traffic impact study work. Depth over breadth. |

## 6. Why this can work

The gap is narrower than it looks:

- **Microsimulation is well-documented published science.** Wiedemann 74/99, gap acceptance,
  lane changing (MOBIL, Gipps), signal control — these are papers, not trade secrets.
- **The hard part of Vissim is not the physics, it is the modelling surface**: the link-based
  network, conflict areas, the object model engineers already know. That is design work,
  and it has already been mapped in detail by the prior effort.
- **The evaluation layer has to be written either way**, so it is not extra cost here.
- **Scope is narrow.** Intersections and corridors for impact studies. Not city-wide
  regional models, not rail, not freight logistics.

## 7. What would make this project wrong

Recorded honestly, so it can be checked rather than defended:

1. **If an engineer cannot complete a real study with it.** A tool that cannot carry one
   real impact study from blank network to report table, in reasonable time, is not usable in
   engineering work however good its engine is. **This is tested with a real study before
   milestone 3** — see the M2 gate in `ROADMAP.md`.
2. **If simulation fidelity cannot be validated.** An engine nobody trusts is worthless in a
   report that goes to a regulator. Validation against published benchmarks is a gate, not
   a nice-to-have.
3. **If the effort of the engine starves the UI.** The prior effort's lesson was that
   almost all of the value was in UX. An excellent engine behind a bad interface loses to
   Vissim on day one.
