# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks whole into `docs/archive/` if this gets
long. Older entries are preserved whole there:

- [`archive/PROGRESS-2026-09-17.md`](archive/PROGRESS-2026-09-17.md) — 2026-09-17
- [`archive/PROGRESS-2026-09-16.md`](archive/PROGRESS-2026-09-16.md) — 2026-09-16
- [`archive/PROGRESS-2026-09-14.md`](archive/PROGRESS-2026-09-14.md) — 2026-09-14
- [`archive/PROGRESS-2026-09-10--2026-09-15.md`](archive/PROGRESS-2026-09-10--2026-09-15.md) — 2026-09-10 to 2026-09-15

---

## 2026-09-17 — M1.12.1: a Connector carries its own lane widths and markings

The last M1 carve-out. A Connector's lane widths were read from the Links each end joins, so a
widening taper had to be authored on the Links instead, and every interior divider was dashed with
no choice about it. Both are now fields on the Connector, schema 6.

**The boundary that mattered more than the fields.** `laneWidthOf` was a file-local helper in
`road_boundaries.cpp`, and `compile.cpp` computed a Connector's width **a second time, its own
way**, for `TIGHT_CONNECTOR_RADIUS`. While both derived from the same Link lanes that was merely
duplication; the moment a width is authored it becomes a disagreement — the drawing would use the
authored width and the radius advisory would measure the old one. `connectorLaneWidths` is now the
one place a width is decided and both read it (hard rule 3). That refactor landed first and on its
own, and 23/23 stayed green across it, which is what says it was behaviour-neutral.

**Empty means derived, and that is the whole compatibility story.** Absent keys give an empty
vector, so schema 6 needs no conversion on read: nothing changed meaning, unlike M1.18. **The
old-file load test is not optional here** — a schema-5 document with a multi-lane Connector is
loaded, compared to the in-memory original, and its boundaries measured vertex for vertex to
1e-12. M1.18 was reverted for exactly this class of mistake, and its verification measured 120 of
120 drawn vertices unchanged, which was true and beside the point because it never opened a file
written by the previous build.

**A defect surfaced and I did not paper over it.** The first width test asserted the authored
5.5 m to 1e-6 on the curved fixture and read **5.529 m**. That is the miter widening the spacing
measured along the cross-section at a bend — the same defect as the 8.698 m of a 7.000 m width
already on record, at 0.5% instead of 24%. Loosening the tolerance would have written it down as
correct, which is precisely what I criticised two existing tests for doing. Instead the exact
assertion moved to a **straight** Connector fixture, where no miter is involved and it holds to
1e-9, and the curved case became a bound naming M1.12.2 for the session that tightens it. Booked
as **M1.12.2** with the cause identified and a "measure before changing" instruction, because it
changes drawn geometry at every bend.

**A judgement call to be honest about.** Vissim's `Lanes` tab has a per-**lane** `MarkingType`.
Ours is per **interior divider** (`paths − 1`), with the two outer edges always solid, because
per-lane does not map unambiguously onto `paths + 1` boundary lines and I would have been choosing
a mapping either way. **I did not check this against Vissim.** It is recorded in `ROADMAP.md` as a
chosen representation rather than a measured parity claim (rule 4), and it is a small change if
the owner's Vissim behaves differently.

**Two smaller decisions with reasons.** A resize that changes the path count **drops** the authored
arrays instead of padding them — an entry the author never typed is not a width they chose, and
the derived value is the honest fallback, the same reasoning that clears `laneBlend` on a geometry
change. And a partial list is rejected with `EDIT_LANES`: no field would say which lanes were
authored and which derived. Marking names are stored as `"solid"`/`"dashed"` rather than the enum's
integers, so a human reading the project file sees words and adding a kind cannot renumber what
older files meant.

**`docs/ROADMAP.md` needed room twice** and got it the way `PROGRESS.md` and `VISSIM_PARITY.md` did:
M1.1–M1.6 and then M1.8–M1.10 bodies moved whole into `docs/archive/ROADMAP-M1-implemented.md`,
each keeping its heading and a status line so the sequence stays whole. Both moves diffed against
`git show HEAD:` — no heading lost, archived bodies byte-identical, no kept body changed, no
dangling link. Note the status line referenced M1.12.2 before its section existed for a few
minutes; booking it in the same commit is what keeps the file internally true.

**Verification:** 23/23 CTest, 125/125 unit tests, architecture and size guards green. The exact
width holds to 1e-9 on a straight Connector; a Connector never given a width is unchanged to
1e-12, including one loaded from a schema-5 file. Linux only — `native.yml` also runs the Qt
suites on Windows and nothing in these sessions has been near it.

---

## 2026-09-17 — M1.11.1 closed: a vehicle enters at the drawn station too

With M3.1 in place, the target station joins `runtimeSections`' cut list and a Connector arriving
inside a lane body runs. M1.11.1's done-condition — "leaves **and** enters" — is met, with no
carve-out.

**Two things were not the two-line change I expected.**

*An arriving vehicle continues downstream of the cut, not upstream of it.* `sectionForStation`
resolves a station to the section that **ends** there, which is right for a signal head standing
on a cut and wrong for a vehicle joining at one — it would have put the arrival on the 40 m of
lane the vehicle never drives. `sectionStartingAt` is its mirror, and the two now sit next to each
other in the header saying which is for what.

*Route expansion had to learn where a route joins a lane.* An authored route `{a1, conn, b1}`
expanded from `b1`'s **first** section, so the chain read `… conn, b1` while `conn`'s successor was
`b1/sec-2` — `DISCONNECTED_ROUTE`, on a network that is perfectly legal. The walk now asks the
segment it just emitted where it leads on this lane and starts there. That is also what makes the
travelled distance right: 100 m of `a1`, the Connector, and 60 m of `b1` — not 140 m of `b1`.

**Where the missing-catalog check belongs, and where I first put it.** I had
`derivedPriorityRules` throw `EDIT_NO_PRIORITY_DEFAULTS` when the gap time was absent. Two
existing tests failed, and they were right to: `buildScenario`'s own header calls it "unchecked
assembly, for diagnostics that must not throw", and blocking an **edit** because a data catalog
was not resolved is exactly the boundary D18b exists to protect. The check moved to
`priorityDefaultsIssues`, with two callers — `compileScenario` throws on it, `runtimeDiagnostics`
reports it — so Run and the panel cannot disagree about it. Editing, saving and Undo of such a
Connector are untouched.

**The derived rule.** One per interior target: the arriving path gives way, its stop line at its
own downstream end, the conflict point where the upstream section ends — which is the drawn
station. Appended to any authored rules rather than replacing them, in path order, so it is
reproducible. Nothing is persisted: it is a function of the drawing, like the sections themselves.

**Measured, not assumed.** A probe on the failing case showed the arriving vehicle reaching 182 m
against a 180.002 m stop line by tick 20 and 236 m by tick 70 — it was crossing all along. My test
was reading `distanceOf` **after** the loop, by which point the vehicle had finished the route and
left the network, so it was comparing against a departed vehicle. The check now runs during the
loop. Worth recording because the assertion looked like a product failure and was a test bug.

**The negative check ran through the data file.** Setting `gapTime` and `headway` to 0.001 in
`data/priority-rules/default.json` breaks three named assertions, the held-at-the-stop-line one
among them. So the hold really comes from those two numbers, and the data path is live end to
end — not a value compiled in somewhere with the file for decoration.

**Verification:** 23/23 CTest, 119/119 unit tests, architecture and size guards green,
`trafficsim-cli 42` unchanged at `meanDelay 29.249359418430977` with the not-yet-validated marker.
The four frozen baselines are untouched. Linux only.

**`docs/ROADMAP.md` is now at exactly 500 lines, the hard rule 6 limit.** The next entry needs
room made first: the M1 block holds 21 sub-milestones, most long implemented, and archiving the
completed bodies the way `PROGRESS.md` and `VISSIM_PARITY.md` were archived is the move. That is
its own piece of work, not something to do while squeezing prose to fit.

---

## 2026-09-17 — M3.1: a merge is arbitrated by gap time and headway

`validateScenario` refused any merge outright, and rightly: a place fed by two segments had no
rule for who goes, and D13 forbids inventing one. The owner authorized implementing the necessary
part of M3 rather than carving M1.11.1's second half out, so `PriorityRule` now supplies the rule
— Vissim's, with the two numbers an engineer tunes.

**Most of a merge already worked, which is why this is small.** `OccupiedSpan::segmentIndex` is a
**global** index into `Scenario::segments` and spans are bucketed globally, so two vehicles on
different routes see each other the moment they share a segment — car-following across the merge
needed nothing. The single missing thing is seeing the major approach **before** entering it,
which is not on the minor vehicle's own route and so is invisible to `closestVehicle`. That is one
scan of one bucket, and the hold at the stop line is the **same clamp a red head uses**
(`simulation.cpp`), not a second braking path beside it.

**The guard is loosened by construction, never by removal.** A place fed by *n* segments is
runnable only when at least *n*−1 of them give way to another of them — so exactly one has
priority and the rest have somewhere to wait. Everything else still reports `UNSUPPORTED_MERGE`,
including a rule that points at the wrong segment and one that names its own segment. Checked by
deleting the guard instead of narrowing it: **six tests fail**, across `core`, `connectors`,
`diagnostics` and `ranges`. That is what tells me the relaxation is the shape I intended and not
a hole.

**A stopped queue is a gap, not a block.** The first version I reasoned through would have had the
minor approach yield to any major vehicle within the gap time — including one standing still 90 m
back, whose time-to-conflict is finite only because the arithmetic does not care that it is not
moving. That deadlocks the minor approach behind a queue that is never going to clear. A major
vehicle further off than the headway and not moving does not block, and there is a test for it.

**One test was vacuous and the negative check is what caught it.** The first "held at the stop
line" assertion used a major vehicle one second from the conflict point and ran 20 ticks. With the
clamp deleted it still passed — from 95 m at rest, two seconds is not enough to reach a stop line
5 m away, so the assertion was true whether the feature existed or not. Rewritten with a
thirty-second gap time and a major vehicle nine seconds out, over six seconds: now it fails first
when the clamp is removed, and it asserts the blocking actually happened rather than assuming it.

**Gap time and headway are data.** `data/priority-rules/default.json`, read by `resolveCatalogs`.
They are read **best-effort**, not unconditionally, and that was a correction: reading them as a
required catalog broke `catalog_overrides_and_missing_catalog_are_explicit`, which pins a real
contract — a document carrying its own vehicle types and behaviours is portable to a machine with
no data directory. So a missing file is not a load error; it is an error at the point of use,
where the alternative would be a zero gap time, which is a merge nobody gives way at, invented in
silence. `PriorityDefaults` is left zero-initialised on purpose for the same reason.

**What this is not.** A deterministic threshold test: wait while any major vehicle is inside the
headway or would arrive inside the gap time, go otherwise. Not a calibrated critical-gap
distribution, and hard rule 4's not-yet-validated marker stays. **M3 is not closed** — conflict
areas as editable input, priority rules as an authorable object, stop and yield control, crossing
conflicts and signal heads anywhere on a link are all still M3's, and its done-condition about
minor-road delay responding to gap time is not met by this. Booked as M3.1 at its number.

`src/core/` gained a type and a clamp and no dependency; the four frozen baselines are untouched
because an empty rule list changes nothing.

**Verification:** 23/23 CTest, 116/116 unit tests, architecture and size guards green. Two
deliberate negative checks each broke named assertions. Linux only.

---

## 2026-09-17 — M1.11.1, first half: a lane is cut where a Connector leaves its body

The engine's `Segment` is a whole traversable length, so a lane was all-or-nothing: a Connector
attached part way along one was authorable but `Run` refused it, because a vehicle turning off at
25 m of a 100 m lane would have been charged for all 100. `runtimeSections` now cuts each lane at
its interior attachments, and `buildScenario` compiles the pieces.

**The split that decides what this milestone can actually deliver.** Checked against
`src/core/validate.cpp:55-68` rather than assumed:

- A Connector **leaving** a lane body is a diverge — `s1.next = {s2, path}`, and `s2` and `path`
  each have one predecessor. Runnable, and now runs.
- A Connector **arriving** on a lane body is structurally a **merge**: the section downstream of
  the arrival has two predecessors, the upstream section and the path. `UNSUPPORTED_MERGE` fires,
  and inventing an arrival order to resolve it is exactly what D13 forbids.

So M1.11.1's done-condition ("leaves **and** enters") is half met. The owner chose to implement
the necessary part of M3 rather than carve the merge out, so the second half follows M3.1.

**What keeps the four frozen baselines valid.** `sectionId(laneId, 0)` returns `laneId` itself,
so a lane with nothing attached to its body compiles to exactly the `Segment` it always did —
same id, same length, same `next` vector in the same order. `polylineSpan(g, 0, length(g))`
returns `g` itself rather than rebuilding it from two `pointAlong` calls, and an uncut lane skips
the call entirely. Proven by breaking it on purpose: making `sectionId` always append a suffix
fails **54 tests**, all four TypeScript baselines among them. Restored, 110 pass.

**Two things I would have got wrong without reading the callers.**

- `validateAuthoredDemand` runs `validateScenario(buildScenario(...))` on **every save**. Had
  route expansion lived in `compileScenario`, saving any document with a sectioned lane would
  have broken, because the authored route still names the whole lane. Expansion is therefore
  inside `buildScenario`, which is also the one place all six callers go through.
- `validate.cpp:59` requires a segment length **> 0**. A cut at a lane end, or two cuts closer
  than epsilon, would have failed with a code that says nothing about Connectors. Hence
  `kMinSectionLength` at 0.2 m — the span `splitLink` already uses for the same question — and a
  degenerate cut that still blocks with an object-linked row.

**Route expansion adds no new error code.** It walks a lane's sections until one whose `next`
carries the following authored id. Running off the end leaves that id unreachable, which is
`DISCONNECTED_ROUTE` — reported by the core guard that already owns the question rather than by a
second check beside it.

**The diagnostic that was true and is now false.** `UNSUPPORTED_CONNECTOR_POSITION` told the user
"the current simulation core only runs end-to-start connectors". Leaving that string in place
while the code ran them would have been a message that misstates why Run is blocked, so it is
reworded to the narrow case it now means, and interior targets get their own
`UNSUPPORTED_ATTACHED_TARGET` in both locales. It also had to be added to `aboutTopology` in
`src/project/diagnostics.cpp`, or the row vanishes from a drawing with no demand authored — which
is how the inverted test caught it.

**One source of truth, four consumers.** `runtimeSections` feeds `buildScenario`, the signal-head
rebasing, `EditorCanvas::setRunNetwork` and `NetworkView::setNetwork`. The last two key their
geometry maps by section id, so a vehicle located on a section has geometry to be drawn at;
`network_view` resolves with a hard `.at()` and that only stays safe because both come from the
same table. The deliberate non-throwing fallback in `canvas_run.cpp` was left exactly as it was.

**The route dialog still offers whole lanes.** A route is stored in the project file, so offering
a derived section id would put a copy of derived data in it. `authoringSegments` collapses the
table back to one row per lane, with the union of its sections' successors minus its own sections
— that union is what makes the interior diverge selectable at all. The demand table's length
column reads the **compiled** route instead, so a turn off the middle of a link reports 25 m and
not 100.

**Tests.** Six new, each with its forcing assertion first: exact 25/75 section lengths and the
`next` order; route expansion stopping at the diverge and walking past it; the minimum-length
block with 5 m compiling and 30.1 m against 30 blocked; a head at 60 on a lane cut at 25
compiling to `a1/sec-2` at 35 while one at 10 stays on `a1` at 10; determinism across the
sectioned run with the shorter route proven to differ from the whole-lane one; and the authoring
view never yielding a `/sec-` id. The old
`draft_and_run_diagnostics_do_not_silently_run_wrong_lane_lengths` is inverted: its source half
runs, its target half asserts the object-linked row arrives **instead of** the generic
`UNSUPPORTED_MERGE`, not beside it.

**Verification:** 23/23 CTest, 110/110 in `trafficsim-tests`, architecture and size guards green,
`trafficsim-cli 42` unchanged. Linux only — `native.yml` also runs the Qt suites on Windows and
this has not been near it.

---

## 2026-09-17 — NETWORK_EDITOR.md: a section whose title covered a third of the file

`## Connector lane ranges` ran **159 lines, 37% of the manual**, and most of it was not about
connector lane ranges. Under that one heading sat Link lane-tab dragging, lane-edge mitering,
road surfaces and markings, geometry handles, end-handle re-attachment, the connector polyline
and its intermediate points, the wedge mouth, the cubic reach, attachment stations, Link splits,
the runtime limit, and deletion cascades. A reader looking for how markings are drawn had no
reason to open a section named after lane ranges, and would not have found it from the contents.

Split into five sections named for what each one holds:

| Section | Lines |
|---|---|
| Connector lane ranges | 43 |
| Lane edges, road surfaces and markings | 18 |
| Geometry and end handles | 29 |
| Connector shape: intermediate points and the mouth | 50 |
| Attachment stations, Link edits and deletion | 27 |

**Not a rewrite.** Only four heading lines and their blank lines were inserted; the prose was
checked byte-for-byte against `git show HEAD:` with those eight lines stripped back out, and it
is **identical**. The split points fall on existing paragraph breaks, so reading order is
unchanged — what changed is that the contents now tells the truth about where things are.
433 to 441 lines, the whole cost being the headings. No anchor link anywhere in the repo
pointed into this file, so no link broke; the sweep confirms none dangling.

**Still worth a later pass, deliberately not done here:** the paragraph now opening *Geometry
and end handles* is 22 lines and mixes the re-attachment gesture with how a Connector's
cross-section is built — two subjects in one block. Splitting it means rewriting sentences, not
moving lines, which is a content change and belongs in its own session with the behaviour in
front of it.

**Verification:** prose identical to HEAD, no dangling links, size guard green, 23/23 CTest.
No source file touched.

---

## 2026-09-17 — ROADMAP.md put back in sequence, and two stale status lines fixed

`ROADMAP.md` opens by calling itself "**a sequence**, so that any session can see where it sits".
It was not one. The M1 sub-milestones ran in the order they were *written*, so the carve-outs
made under rule 2 had piled up wherever the session that carved them happened to stop:

```
before: ... M1.10 M1.11 M1.12 M1.11.1 M1.13 M1.14 M1.15 M1.16 M1.17 M1.12.1
after:  ... M1.10 M1.11 M1.11.1 M1.12 M1.12.1 M1.13 M1.14 M1.15 M1.16 M1.17
```

Both misplaced entries are the two that are **still open**, which is the worst possible thing to
bury: M1.11.1 sat behind a done M1.12, and M1.12.1 sat last in the file behind five done
milestones, reading like the newest work rather than unstarted work. A carve-out is now filed at
its number, and the M1 preamble says so, so the next one lands in the right place.

**Two status lines were also simply wrong.** `ROADMAP.md` said "M1.1–M1.10 implementation is
available" and `CLAUDE.md` said M1 "covers M1.1–M1.10 ... M1.11 adds body attachments; M1.12
fixes both lane edges" — both written before M1.13–M1.17 shipped and never updated. A session
starting from either would have believed five milestones of work did not exist. Both now name
M1.1–M1.17 and, separately, the two open carve-outs. Note these are *not* a duplicated status
table: each milestone's own state still lives only in its own section, and the preamble names
which are open without restating why.

**Verification:** all 21 M1 sections diffed body-for-body against `git show HEAD:` —
`lost: set()`, `gained: set()`, `bodies differing: none`; everything outside the M1 block
byte-identical; line count unchanged at 434 by the reorder, 440 after the corrected preamble;
separator count unchanged at 14. 23/23 CTest, guards green. No source file touched.

A first attempt scored two false differences here, because my checker split the file only on
`### M1` and so let M2-M7 attach to whichever section came last. The file was fine; the check
was wrong. Bound a section comparison at both ends, not just the start.

---

## Next

**M1.12.2 — the miter widens a carriageway at a sharp bend.** The last engineering item in M1, and
the only one left that is mine. Booked in `ROADMAP.md` with the cause identified:
`offsetGeometry`'s miter vector has length `1/cos(θ/2)`, correct for the intersection of two offset
legs, but applied independently to each boundary with its own offset — so the spacing *along the
cross-section* between two boundaries grows by that factor at a sharp vertex. A 2→2 Connector
through a sharp bend reads **8.698 m of a 7.000 m width, 24% over**; M1.12.1 met the same thing at
0.5% (an authored 5.5 m lane measuring 5.529 m on a gentle curve).

**Measure before changing, and do not assume the existing tests are right.** Two of them may have
written the defect down as expected behaviour:

- `tests/network_tests.cpp` `bends_keep_their_full_carriageway_width` asserts `3.5*sqrt(2)` between
  adjacent boundaries at a right-angle corner — which *is* `1/cos(θ/2)` at 90°.
- `tests/connector_tests.cpp` allows an `8e-2` interior tolerance, commented "the miter (6.3 cm
  measured)".

Establishing which is Vissim's real behaviour is the first task, not the fix. Three earlier rounds
of guessing from screenshots each went wrong, so **ask the owner with a screenshot** rather than
guessing a fourth time. Useful measuring helpers already exist at `tests/connector_tests.cpp`
(`crossings`, `apart`, `perpendicular`, `mouthLine`, `narrowest`, `minimumRadius`) — `perpendicular`
measures square to the road, which is the honest measure here.

The gap that let 24% through: width is bounded only from **below** (`least > .9*3.5`). Add the
upper bound. Also tighten the curved-width bound M1.12.1 deliberately left loose in
`an_authored_width_is_exact_where_the_connector_is_straight`'s sibling
(`a_connector_carries_its_own_lane_widths`, currently `span < 5.7`) to an equality once the miter
is right.

**After that, M1's engineering side is done and only the owner's gate remains.** The timed
four-leg/aerial-image/reopen exercise in `M1_ACCEPTANCE.md` cannot be performed by anyone but the
owner; not one row of it has been filled in, and merged code does not close it (rule 1). The M1.12
review checklist from the 2026-09-16 sessions is worth running against a desktop build **on
Windows** first — `native.yml` runs the Qt suites there and nothing in these sessions has been near
it.

**D11's trigger is live.** Naming was deferred "until the end of M1" and Q5 is that decision.
`Velk` is the strongest recorded candidate — clean on npm, PyPI and a brand search, with
`velk.com`/`velk.io` held. **The owner names the project; do not pick one.**

**M3 is not closed.** M3.1 supplied merge arbitration only. Conflict areas as editable input,
priority rules as an authorable object with their own UI, stop and yield control, crossing
conflicts and signal heads anywhere on a link are all still M3's, and its done-condition about
minor-road delay responding to gap time is not met.

---

## Backlog (M0, in order)

- [x] Toolchain + directory skeleton + core-import guard
- [x] `Scenario` type and a fixture: two crossing movements with explicit connectors
- [x] Fixed-timestep loop; one vehicle traverses links with continuous route distance
- [x] Reduced Wiedemann-inspired car-following; vehicles queue behind each other
- [x] Fixed-time signal; vehicles stop at red, discharge at green
- [x] Vehicle input generating arrivals from a seeded stream, retaining blocked arrivals
- [x] Native Qt harness: vehicles as dots on links (former canvas preserved in Git history)
- [x] Headless completed-trip delay diagnostic and seeded replay regression
- [ ] Owner's M0 plausibility acceptance (still open)

Later milestones are in [`ROADMAP.md`](ROADMAP.md). The owner explicitly authorized M1.1–M1.3 in D16; all other milestone gates remain in force.

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

| D15 | 2026-09-11 | **C++20 throughout the application, Qt 6 Widgets desktop, CMake/CTest**; supersedes D3's initial stack, D4's web-first loop and D14's active TS tooling | The owner asked whether the whole program could move to C++, then authorized the proposed migration. Port the existing M0 core/model and harness together; preserve old source in Git and four frozen regression fixtures. Keep Qt/JSON outside the engine and existing modelling limitations explicit. This supersedes one-system scheduling guidance for the migration. | If behaviour diverges from the saved baseline or desktop controls cannot run, fix the port before M1. Cross-toolchain math uses an explicit tolerance; scientific fidelity still requires M6. |

| D16 | 2026-09-12 | **Implement M1.1–M1.3 together on the native C++ base** | The owner approved the editor plan and explicitly requested these three slices. This supersedes the earlier one-system scheduling and M0-only Next for this scoped work. Implement document/history, canvas/background and Link/Lane tools together with basic saving so drawings persist. Existing scientific and full M1 usability gates remain open. | If later connectors, demand or runtime behaviour are introduced without their own scope decision. |

| D17 | 2026-09-14 | **Continue M1 with the planned M1.4 Connector editor; preserve one version-1 geometry source** | The owner requested continued Network editor development. `Next` already called for M1.4, so this session implements that slice and retains M1.3.1's split guard. A lane-aligned cubic is sampled into editable polyline points, avoiding a second curve store and an unnecessary schema change. Referenced connectors may be reshaped but not retargeted; deleting one removes its affected routes and inputs atomically instead of inventing new paths. | If engineering workflows require persistent tangent handles or measured radius constraints, define their model/schema explicitly; do not claim the sampled curve provides those guarantees. |

| D18a | 2026-09-14 | **Diagnostics resolve object IDs in the model layer; `ValidationIssue` stays frozen** | Adding an `objectId` field to `ValidationIssue` in `src/core/types.hpp` would have compiled — no test compares a whole issue — but it is the wrong boundary. `core/` validates a `Scenario` whose only objects are segments, so the field would be empty for nearly every core code, and it would duplicate what the index path already locates (hard rule 3). Deriving the ID on read from the index path keeps `src/core/` untouched by M1.5 entirely, which is also what puts the four frozen TypeScript baselines and the trajectory digest structurally out of reach. | If an index path ever needs to survive an edit and be re-resolved later, a stored ID becomes the cheaper representation. It is not needed for a panel that is rebuilt per revision. |
| D18b | 2026-09-14 | **Draft validity blocks an edit; runtime supportability only informs, and only on demand** | These answer different questions and must not be merged. `validateNetwork` asks "is this a coherent drawing" and `History::execute` rightly refuses anything else. `validateScenario` asks "can the M0 core run this", and the authoring model deliberately expresses things the core cannot yet run — a merge is the standing example. Making the second blocking would forbid legal authoring; making the first advisory would let broken documents be saved. **A structural consequence the UI must own: because `validateDocument` throws on every non-`EMPTY_NETWORK` issue, a committed document can never carry a draft-invalid object, so the draft list is fed only by a blank document and by the issues of a *rejected* edit** — which is exactly where the object IDs earn their keep. | Nothing, unless the runtime gains merge arbitration, at which point `UNSUPPORTED_MERGE` stops being a finding. Do not "fix" the usually-empty draft list by loosening commit validation. |
| D18c | 2026-09-14 | **M1.5 tables cover network objects only; demand tables carve out to M1.5.1** | Routes and vehicle inputs have no model struct — they are untyped JSON under `ProjectDocument::definition` — so tabling them means designing an authoring demand model, which is M2's subject. Diagnostics still name routes and inputs by their own IDs, which is what the milestone actually required. Carved into a numbered milestone in the same session rather than left as a note, per hard rule 8. | If M2 demand authoring lands first, M1.5.1 is absorbed into it rather than done separately. |
| D18d | 2026-09-14 | **Vehicle-type and behaviour findings are withheld, not reported, when no catalog is loaded** | Those catalogs live in `data/`, not in the project file, so a document alone genuinely cannot resolve them. Reporting `UNKNOWN_VEHICLE_TYPE` for every input of an otherwise valid M0 fixture would blame the drawing for an absence that is by design, and would train users to ignore the panel. One `EDIT_NO_CATALOG` row says what was not checked instead. | When M1.7 resolves catalogs at run handoff, the check becomes real and the withholding should be removed rather than left as a permanent blind spot. |
| D19a | 2026-09-14 | **Keep M0 scenarios and editor projects as two formats; classify the file instead of merging them** | The reported 304 was a category error, not a corruption: a drawn network legitimately has no `definition`, and the simulation window had no way to tell a project from a scenario because both matched `*.json`. Merging the two schemas would have removed the failure by making every drawing claim it is runnable, which is precisely the fidelity claim hard rule 4 exists to prevent — a drawing has no demand, so it cannot run, and the format should keep saying so. Classifying the file before any field is read, and routing a project to the window that can open it, fixes the user's actual problem without that claim. | If M1.8 gives projects a real Run **and** demand authoring (M1.5.1) makes `definition` non-optional in practice, the distinction stops earning its keep and one format becomes honest. Converge then, not before. |
| D19b | 2026-09-14 | **Load errors carry a code on a typed exception, not a formatted message** | The shell already translates `EDIT_*` codes by locale key (`EditorWindow::showError`); the simulation window instead concatenated `e.what()`, which is how nlohmann's text reached a user running `--language th`. `ScenarioLoadError` carries file, code and detail separately so the shell can translate, show the path, and offer an action, while an unknown parser detail still falls back to raw text rather than a blank dialog. One error channel, one lookup, two windows. | If load errors ever need structured per-object issues the way `ValidationError` does, promote the code to an issue list rather than growing the string. |
| D19d | 2026-09-14 | **Error codes are resolved in exactly one place per window; no caller formats `what()` itself** | The first cut of D19b gave `ScenarioLoadError` a code but left three callers printing `what()`, which for a classification failure *is* the bare code — so `--scenario` on an editor project went from an unreadable nlohmann string to an unreadable identifier. Worse, not better: the user lost the one sentence the old message did contain. `MainWindow::explain` and `EditorWindow::openFileOrReport` are now the only places a code becomes text, and startup no longer dies on a file it could have explained. | If a third window appears, the two `text()` lookups become genuine duplication and should be hoisted to a shared locale helper rather than copied a third time. |
| D19c | 2026-09-14 | **The parity review books three milestones and deliberately leaves seven gaps unbooked** | `VISSIM_PARITY.md` §6 ranks ten gaps; only in-editor Run, the sidebar/gesture set and levels/display types are carved into `ROADMAP.md`. The rest — editable object tables, group drag, and the absent Vissim object types (priority rules, stop signs, reduced speed areas, conflict areas) — need engine behaviour that does not exist yet. Booking authoring for an object the core cannot honour would invite a user to believe it is modelled, and would put a date on work whose prerequisites are unscheduled. Recording them without a milestone keeps the roadmap true (`ROADMAP.md` rule 2) while keeping the finding. | When M3 lands right-of-way, conflict areas and priority rules stop being unhonourable and should be booked immediately — the review section is the list to work from. |
