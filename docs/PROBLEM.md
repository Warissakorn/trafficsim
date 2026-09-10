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

**Vissim costs more than most of these users can justify, and there is no free tool that
does step 5.**

The obvious answer is Eclipse SUMO: it is free, mature, actively developed, and its
simulation quality is not in question. So the obvious project is a friendly UI over SUMO
that gives Vissim users the workflow they already know.

**That project was attempted and it does not fully work.** A prior effort spent six
milestones building exactly that — a Vissim-shaped web UI over SUMO, with a real network
editor, signal editor with ring-barrier, scenarios, and reporting. The UI succeeded. What
it ran into was a set of walls that are **in the simulation engine, not in the UI**, and
no amount of interface work moves them:

| What the user needs | Why a SUMO wrapper cannot give it |
|---|---|
| **Conflict areas** — pick which movement yields at each individual conflict point | SUMO derives right-of-way from junction type and link priority. The `<request>` matrix is an output, not an input. You can read it; you cannot set a cell. |
| **Priority rules** — set gap time and headway in seconds/metres | Not expressible. There is no per-conflict gap-time parameter to write. |
| **Signal heads anywhere on a link** | Signals must sit at a junction stop line. A mid-block signal requires inventing a junction. |
| **Wiedemann car-following** | W99 exists as an approximation; W74 has no counterpart. Calibration values carried over from Vissim do not mean the same thing. |
| **Desired speed distributions** | In Vissim a link has no speed — speed comes from the vehicle. In SUMO speed lives on the link and the vehicle applies a multiplier. The mental models do not line up, and the difference surfaces in every dialog. |
| **Node evaluation** — delay/LOS/queue per movement | Does not exist anywhere in SUMO or its tool suite. Must be built from scratch regardless of the front end. |
| **Multi-run averaging with confidence intervals** | Not built in. Must be built from scratch regardless of the front end. |

The last two are the deliverable of the entire job. **The two things the user is actually
paid to produce are the two things the wrapper approach has to build itself anyway** —
while inheriting an engine whose right-of-way model it cannot reach.

## 3. What this project is

**A traffic microsimulator with its own simulation engine, built to the model Vissim users
already think in**, with the evaluation output that traffic impact reporting requires.

Not a SUMO front end. Not a SUMO fork. A separate engine, so that conflict areas, priority
rules, signal placement, driver behaviour, and per-movement evaluation are all first-class
inputs rather than things worked around.

This is the expensive answer, chosen deliberately. See decision **D1** in
[`PROGRESS.md`](PROGRESS.md) for the reasoning and for what would make it wrong.

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

1. **If a SUMO wrapper turns out to be good enough for the users we actually have.** The
   walls above are real, but a consultant who only ever models signalized intersections with
   standard control may never hit them. **This must be tested with real users before
   milestone 3.** See the M2 gate in `ROADMAP.md`.
2. **If simulation fidelity cannot be validated.** An engine nobody trusts is worthless in a
   report that goes to a regulator. Validation against published benchmarks is a gate, not
   a nice-to-have.
3. **If the effort of the engine starves the UI.** The prior effort's lesson was that
   almost all of the value was in UX. An excellent engine behind a bad interface loses to
   Vissim on day one.
