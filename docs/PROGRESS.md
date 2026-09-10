# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks to `PROGRESS-archive.md` whole if this gets
long.

---

## Next

**Set up the toolchain and get both commands green. No features.**

1. In the repo root: `npm create vite@latest . -- --template vanilla-ts`, then add `vitest`.
2. Turn on `strict: true` in `tsconfig.json`.
3. Create the directory skeleton from the Layout section of `CLAUDE.md`, each with a
   one-line `README.md` saying what belongs there.
4. Add a lint rule (or a test) that **fails if anything under `src/core/` imports from
   outside `src/core/`** — hard rule 1 has to be mechanically enforced from day one, not
   remembered.
5. Verify `npm run dev` and `npm test` both work on a clean checkout.
6. Update the Log below and commit.

**Done when:** a clean clone runs both commands successfully and the core-import test fails
if you deliberately add a bad import.

**Do not start the simulation engine in the same session.** M0's first real system is the
scenario data structure and one vehicle moving along one link, and it deserves a whole
session.

---

## Backlog (M0, in order)

- [ ] Toolchain + directory skeleton + core-import guard  ← **Next**
- [ ] `Scenario` type and a fixture: two crossing links, one connector each way
- [ ] Fixed-timestep loop; one vehicle traverses one link at constant speed
- [ ] Car-following (Wiedemann-style); vehicles queue behind each other
- [ ] Fixed-time signal; vehicles stop at red, discharge at green
- [ ] Vehicle input generating arrivals from a seeded stream
- [ ] Canvas view: vehicles as dots on links
- [ ] Headless run printing average delay; reproducibility test on the seed

Later milestones are in [`ROADMAP.md`](ROADMAP.md). Do not pull work forward from them.

---

## Open questions

Ask these before the milestone they block.

| # | Question | Blocks | Notes |
|---|---|---|---|
| Q1 | Is the audience Thailand-first, or international from the start? | Report formats, LOS tables, units, default language | Affects `data/los-tables/` shape but not the engine. Can be deferred to M5. |
| Q2 | Which lane-changing model? | M1 | MOBIL and Gipps are both defensible. Needs a short spike, not a debate. |
| Q3 | Who are the three engineers for the M2 gate? | M2 gate | **Line these up early.** The prior effort stalled a milestone for want of real users to measure with. |
| Q4 | Which published benchmarks define the M6 tolerance? | M6 | Decide before M5 so evaluation is built to be checkable against them. |

---

## Decisions

Non-obvious choices **and the reasoning**. Without the reasoning a later session will
"improve" a decision away and break something invisible.

| # | Date | Decision | Why | What would make it wrong |
|---|---|---|---|---|
| D1 | 2026-09-10 | **Own simulation engine, not a front end over an existing one** | A prior six-milestone effort built a Vissim-shaped UI over SUMO and hit walls that are in the engine, not the interface: conflict areas are output-only, gap times are not expressible, signal heads must sit at stop lines, and the two things the job is actually paid for — per-movement evaluation and multi-run averaging — had to be built from scratch regardless. See `PROBLEM.md` §2. | If the M2 gate finds practising engineers would be satisfied by the wrapper. This is the single most expensive decision in the project and it has an explicit test. |
| D2 | 2026-09-10 | **Link-based network model natively; junctions are derived, not authored** | This is how the audience thinks and it is the whole point of D1. Translating to a node–edge model would reintroduce the impedance the prior effort spent six milestones papering over. | If deriving junction geometry from links proves intractable at M1. |
| D3 | 2026-09-10 | **TypeScript everywhere to start; `core/` written so it can be ported to a compiled language later without touching anything above it** | Microsimulation is CPU-bound and a compiled core is probably where this ends up. But picking a stack the user cannot run today, to solve a performance problem not yet measured, is the classic way to stall at milestone 0. Hard rule 1 (`core/` imports nothing) makes the port a contained job later, and makes it measurable first. | If M0 cannot reach real-time on a single intersection — then port immediately rather than optimising TypeScript. |
| D4 | 2026-09-10 | **Desktop is a constraint from day one, a milestone at the end** | Web-first keeps the development loop fast; a framework-free core plus isolated rendering means desktop packaging is packaging, not a rewrite. A boot smoke test in a desktop shell runs from M1 so it never becomes a surprise. | If a required capability (native file dialogs, offline licensing) turns out to need a different shell architecture. |
| D5 | 2026-09-10 | **A results screen carries a "not yet validated" marker until M6 passes** | Numbers from this tool go into documents submitted to regulators. An unvalidated engine that looks authoritative is worse than no tool. | Nothing. This one is not negotiable before M6. |
| D6 | 2026-09-10 | **Project spine written before any code** | Only what is on disk survives a session boundary. The rules in `PRINCIPLES.md` §3 were measured by a prior effort and would otherwise have to be rediscovered by paying for them again. | — |

---

## Log

### 2026-09-10 — repository initialized, documentation spine written

Created a fresh repo for a new project, separate from the prior SUMO-wrapper effort.

**Written:** `PROBLEM.md` (who this is for, the engine-level walls that motivate D1, non-goals,
and what would make the project wrong), `PRINCIPLES.md` (hard rules, deliberate non-goals, and
measured discipline inherited from the prior effort), `ARCHITECTURE.md` (the five-layer map,
marked planned throughout), `ROADMAP.md` (M0–M7 with done-conditions and two hard gates),
`CLAUDE.md` (standing orders), this file.

**Decisions:** D1–D6 above. D1 is the one everything else rests on, and it has an explicit
falsification test at the M2 gate.

**No code was written.** The Systems table in `ARCHITECTURE.md` describes intent, not reality;
every row is marked `planned`.

**Next:** toolchain setup — see the `Next` section above.
