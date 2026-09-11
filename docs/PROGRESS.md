# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks to `PROGRESS-archive.md` whole if this gets
long.

---

## Next

**Review the M0 acceptance gate in the running development harness.**

1. Run `npm ci`, `npm test`, `npm run build`, then `npm run dev`.
2. Inspect the default seed 42 crossing: acceleration, the western approach queue at red,
   green discharge, and retained source demand.
3. Record the owner's plausibility judgement here. Automated replay and collision checks
   do not close the traffic-engineering gate.
4. If a behaviour defect is found, add the smallest failing scenario and fix the core.
   Do not add lane changing or priority control to disguise an M0 defect.
5. Only after the owner passes M0, start M1 with the command/model/project contracts and
   one undoable link edit. Keep the authoring model outside `core/`.

**Implementation reference:** `docs/SIMULATION.md`; core API in `src/core/index.ts`;
network API in `src/model/network/index.ts`.

---

## Backlog (M0, in order)

- [x] Toolchain + directory skeleton + core-import guard
- [x] `Scenario` type and a fixture: two crossing movements with explicit connectors
- [x] Fixed-timestep loop; one vehicle traverses links with continuous route distance
- [x] Reduced Wiedemann-inspired car-following; vehicles queue behind each other
- [x] Fixed-time signal; vehicles stop at red, discharge at green
- [x] Vehicle input generating arrivals from a seeded stream, retaining blocked arrivals
- [x] Canvas development harness: vehicles as dots on links
- [x] Headless completed-trip delay diagnostic and seeded replay regression
- [ ] Owner's M0 plausibility acceptance  ← **Next**

Later milestones are in [`ROADMAP.md`](ROADMAP.md). Do not pull work forward from them.

---

## Open questions

Ask these before the milestone they block.

| # | Question | Blocks | Notes |
|---|---|---|---|
| ~~Q1~~ | ~~Thailand-first or international?~~ | — | **Answered 2026-09-10: international from the start.** See D7. |
| Q2 | Which lane-changing model? | M1 | MOBIL and Gipps are both defensible. Needs a short spike, not a debate. |
| ~~Q3~~ | ~~Who are the three engineers for the M2 gate?~~ | M2 gate | **Answered 2026-09-10: the project owner performs the gate alone.** This materially weakens it — see D8 and the mitigation in `ROADMAP.md` M2. |
| Q4 | Which published benchmarks define the M6 tolerance? | M6 | Decide before M5 so evaluation is built to be checkable against them. HCM is the likely baseline now that D7 makes the tool international. |
| Q5 | Final product name | Nothing before M1 | **Deferred until the end of M1** by D11 — not a blocker on any current work. Candidates and collision findings are in the D11 row; reuse them. |
| ~~Q6~~ | ~~Register `velk` on npm and PyPI~~ | — | **Withdrawn 2026-09-11 as moot** — no settled name to register. The registration question returns with the name at M1. |

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
| D7 | 2026-09-10 | **International audience from the start, not Thailand-first** | Nothing in the engine is jurisdiction-specific, and the parts that are — LOS thresholds, report layouts, units — are data, not code, so building them swappable costs little now and a retrofit costs a lot. Three concrete consequences: HCM is the default LOS pack with others as swappable data; metric internally with display units switchable; **left-hand and right-hand traffic is a first-class network setting from M1** (Thailand, UK, Japan, Australia all drive left — a prior effort never implemented it at all). | If it turns out every real user is in one jurisdiction and the generality is unused weight. |
| D8 | 2026-09-10 | **The M2 gate is performed by the project owner alone, not three independent engineers** | The owner is a practising traffic engineer and no outside participants are available. Accepted with eyes open: this is **a materially weaker test than the one D1 needs**, because the person judging whether a free SUMO-based tool would have sufficed is the same person who chose to build an engine instead. Mitigation, mandatory: **the pass/fail criteria are written down and committed before M2 implementation starts**, so the judgement cannot be rationalised after the fact. Adding outside engineers later strengthens the gate and is never wasted. | Nothing makes it wrong; it is simply weak. Treat a pass as "not disproven", not as "confirmed". |
| D9 | 2026-09-10 | **The project is named Veytrix** | Chosen by the owner after working through several naming directions (domain jargon, borrowed engineering terms, abstract coinages, Thai-rooted feminine names). Verified free on npm and PyPI. **Two known flags, accepted:** `veytrix.com` is already resolving to something, and **Vectrix** is an existing electric-scooter company that is phonetically close. Neither blocks a repository or package name, but both are reasons a trademark search would be worth doing before any commercial use. | A trademark conflict surfacing later. Renaming is cheap while the repo is documentation only and gets steadily more expensive after that. **Superseded by D10 (Velk) on 2026-09-11.** |
| D10 | 2026-09-11 | **The project is named Velk**, superseding D9 | Coined, one syllable, no meaning in any major language — the owner's stated requirement. Verified free on npm and PyPI, and a brand/company search found nothing using it. `velk.dev` and `velk.app` are free; `velk.com` and `velk.io` are held, which is ordinary for a four-letter word and irrelevant to a repository or package name — accepted as a known risk. **`MicroFlow Simulator` was considered first and rejected on collision grounds** (`microflow` taken on npm and PyPI, ≥7 GitHub projects plus two orgs and a GitHub Topic, both obvious domains held) — do not re-propose it. | A trademark conflict, or the name proving so anonymous that people cannot find the project. Both are cheap to fix now and expensive once source code, packages and links exist. **Superseded by D11 on 2026-09-11.** |
| D11 | 2026-09-11 | **Keep the working name `TrafficSim`; defer naming until the end of M1** | The project was renamed three times in two days (TrafficSim → Veytrix → Velk) with several further candidate sets explored, and no code was written in that time. A name is far easier to judge against a working program than against a specification, and each further round costs a session without moving the project. Deferring also cancels work already queued: no GitHub repository rename, and no package or domain registrations to make and then undo. **Trigger to revisit: the end of M1**, when there is a working network editor to name. **Names already examined — start from these findings, do not re-derive them:** `Headway` rejected (`headwaymaps/headway`, an OSM maps stack, same field); `MicroFlow Simulator` rejected (`microflow` taken on npm and PyPI, ≥7 GitHub projects plus two orgs and a GitHub Topic, both obvious domains held); `Veytrix` set aside (`veytrix.com` held, `Vectrix` phonetically close); `Velk` set aside while clean on every channel checked (npm, PyPI, brand search; `velk.dev`/`velk.app` free) and therefore the strongest candidate to return to. | Drifting past M1 without ever deciding. The trigger exists to prevent exactly that. |
| D6 | 2026-09-10 | **Project spine written before any code** | Only what is on disk survives a session boundary. The rules in `PRINCIPLES.md` §3 were measured by a prior effort and would otherwise have to be rediscovered by paying for them again. | — |

| D12 | 2026-09-11 | **Implement the simulation core and network model together, including prerequisite tooling** | The owner explicitly requested both systems in this session. That supersedes the earlier toolchain-only Next and one-system scheduling guidance. The boundary still remains strict: model compiles a snapshot; core imports only its own modules. | If later features are pulled forward without a separate scope decision. M0 is still the boundary. |
| D13 | 2026-09-11 | **M0 uses a clearly labelled reduced Wiedemann-inspired longitudinal model; unsupported merges are rejected** | A small auditable prototype is enough to exercise the M0 architecture and queue/discharge behaviour. The published W74 safety-distance shape is an inspiration, not permission to claim a faithful W74/W99 implementation. Accepting merging paths without gap acceptance would silently invent unsafe right-of-way semantics. | If M0 plausibility fails, fix or replace the approximation before closing M0; do not remove the unvalidated marker. |
| D14 | 2026-09-11 | **Pin TypeScript 5.9.3 and the dependency lockfile** | The boundary guard uses the TypeScript compiler AST API, including type imports and dynamic imports. The initially resolved TypeScript 7 package lacks that API. Pinning the compatible compiler makes the guard executable, with deliberate negative tests. | When the guard is migrated to a supported replacement AST API and verified against the same forbidden-import fixtures. |

---

## Log

### 2026-09-11 — M0 simulation core and network model implemented

The repository previously contained documentation only. Added a strict TypeScript/Vite/
Vitest toolchain and lockfile, the directory skeleton, and an AST-based core dependency
guard that is tested against intentionally invalid imports.

**Network:** link/lane/connector authoring types; left/right driving-side lane geometry;
mid-link signal heads; geometry/reference/range validation; and a detached scenario
compiler. Junctions are not authored, and no second persisted network format was added.

**Core:** fixed timestep; explicit xorshift32 seed state; immutable snapshots and pure
steps; Poisson source arrivals with persistent external queues; reduced four-regime
following; fixed-time red/amber/green signals; route transitions with residual distance;
upstream vehicle-tail occupancy; and a streaming event interface. The final subinterval's
arrivals remain pending instead of disappearing at the run horizon.

**Integration:** a crossing scenario and vehicle/behaviour catalogs in data files; a
passive canvas harness with run/pause/step/reset, playback speed, seed reset and English/
Thai text; a headless CLI; and an explicitly unvalidated completed-trip delay diagnostic.
The engine still has no UI, model, I/O or wall-clock imports.

**Verification:** 40 automated tests cover replay (including a reference trajectory
fingerprint), pure stepping, source queues, conservation, signal timing, free acceleration,
red stops/green discharge, upstream tails, short connectors, invalid scenarios, authoring
geometry and the import boundary. Production type checking/build and the source-size
check pass. A Chromium 152 browser smoke test passed dev boot, single-step, run/pause,
seed reset, Thai translation, an entire run matching the headless output, invalid-seed
handling and a 375-pixel viewport with no horizontal overflow. No page errors occurred.
In a separate clean detached checkout, `npm ci --offline` (using the package cache),
all 40 tests, the production build, the headless example and file-size checks also passed.

**Reference run:** seed 42, 180 simulated seconds, 31 completed trips, 0 active and 0
pending at the horizon, 0 numerical safety clamps. Mean completed-trip delay is
29.249359418430977 s (includes source wait and acceleration, not HCM control delay).
This is a reproducibility fixture, not a capacity or fidelity benchmark.

**Limits remain explicit:** no lane changing, merge arbitration, geometric crossing-conflict
resolution, priority rules, full W74/W99, project persistence, movement LOS or batch
aggregation. Merges, internal inputs and repeated-route segments fail validation.
M0 remains open for the owner's plausibility acceptance. No later milestone was closed.

See D12–D14 and `docs/SIMULATION.md` for the reasoning and precise interfaces.

### 2026-09-11 — reverted to the working name TrafficSim, naming deferred (D11)

Three renames in two days with no code written. The owner called it: go back to the working
name and decide the real one once the program has shape.

Headings across `CLAUDE.md`, `README.md`, `ARCHITECTURE.md`, `ROADMAP.md` and this file are
back to `TrafficSim`, now explicitly marked as a working name so no future session reads it
as settled. D9 and D10 keep their full reasoning and are marked superseded — this file is
append-only, and the collision findings gathered over those rounds are the main thing worth
keeping from them, so they are consolidated into the D11 row. The next naming round starts
from evidence, not from zero.

**Two pieces of queued work are cancelled, not postponed:** the GitHub repository rename (the
repo is still `Warissakorn/trafficsim` and the remote already points there, so there is
nothing to do) and the npm/PyPI/domain registrations for `velk`.

**Two defects in this file were found and fixed while making this change**, both introduced by
earlier sessions of this conversation:

1. **The D10 log entry below was never actually written.** The edit that should have added it
   matched no text, and the guard around that edit only checked that *something* in the file
   had changed — which was true because other edits in the same batch succeeded. It has been
   reconstructed below from the commit message and the D10 row. Guards on edits to this file
   now assert an exact match count per edit.
2. **Entries were in oldest-first order**, contradicting this file's own header. Reordered
   newest-first. No entry text was altered.

Nothing about scope, architecture or the roadmap changed. D1–D8 stand.

### 2026-09-11 — renamed to Velk (D10)

*Reconstructed on 2026-09-11 — see defect 1 in the entry above.*

`Veytrix` replaced throughout the documentation. `velk` verified free on npm and PyPI with no
brand or company found using it; `velk.dev` and `velk.app` free, `velk.com` and `velk.io`
held — ordinary for a four-letter word and irrelevant to a repository or package name, so
accepted as a known risk.

`MicroFlow Simulator` was proposed first this session and dropped after its collision check:
`microflow` taken on npm and PyPI, at least seven GitHub projects carrying the name along
with two orgs and a GitHub Topic, and both obvious domains held. Recorded in D10 so it is not
raised again.

Also noted at the time: the MicroFlow brand write-up claimed "extends to both microscopic and
macroscopic" as a strength, which contradicts `PROBLEM.md` §5 where macroscopic assignment is
a non-goal. **§5 was left unchanged** — that is a scope decision, not a naming one.

### 2026-09-10 — named Veytrix (D9)

Working name `TrafficSim` replaced throughout the documentation. `veytrix` is free on npm
and PyPI; `veytrix.com` is taken and `Vectrix` (electric scooters) is phonetically close —
both recorded in D9 as accepted, known risks rather than discovered later.

**Still to do by hand:** the GitHub repository is still called `trafficsim`. Renaming it needs
repository-admin access, which this session's GitHub app does not have — the owner renames it
in the repository settings, after which the git remote here needs updating.


### 2026-09-10 — Q1 and Q3 answered (D7, D8)

- **Q1 → international from the start** (D7). Consequences recorded: HCM as the default LOS
  pack with jurisdictions as swappable data, metric internally with switchable display units,
  and **left-hand/right-hand traffic as a first-class setting from M1** — added to the M1
  scope in `ROADMAP.md` because retrofitting it touches every geometry routine.
- **Q3 → the project owner performs the M2 gate alone** (D8). Recorded honestly as a
  weakening of the gate, with a mandatory mitigation: the M2 pass/fail criteria must be
  written into `ROADMAP.md` and committed **before** M2 implementation starts. `ROADMAP.md`
  now carries an unfilled placeholder for those criteria; starting M2 without filling it
  voids the gate.
- **Q5 opened:** final product name. `Veytrix` is a placeholder. `Headway` was considered
  and rejected — `headwaymaps/headway` is an existing open-source maps stack, too close a
  neighbour in the same field.

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
