# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks whole into `docs/archive/` if this gets
long. Older entries are preserved whole there:

- [`archive/PROGRESS-2026-09-17.md`](archive/PROGRESS-2026-09-17.md) — 2026-09-17
- [`archive/PROGRESS-2026-09-16.md`](archive/PROGRESS-2026-09-16.md) — 2026-09-16
- [`archive/PROGRESS-2026-09-14.md`](archive/PROGRESS-2026-09-14.md) — 2026-09-14
- [`archive/PROGRESS-2026-09-10--2026-09-15.md`](archive/PROGRESS-2026-09-10--2026-09-15.md) — 2026-09-10 to 2026-09-15

---

## 2026-09-18 — The Connector mouth is a plain square end again (M1.17 reverted)

**The owner asked for the first Connector geometry back:** the end of a Connector simply meets the
Link, with no realignment towards the Link's direction and a plain square end. Done, on
instruction — this is a deliberate revert of M1.17, not a defect fix.

`connectorBoundaries` now stops at `offsetGeometry`. The whole end-cut block is gone: the
fixed-distance cut onto the Link's cross-section, the bounded re-miter that stood in where that cut
folded, the fold test that chose between them, and the `legCrossing`/`remiter` helpers. `endCross`
stays — the source end's cross-section is still what fixes which way lane order runs, and nothing
else reads it now.

**What is unchanged:** the body. The cross-section is not interpolated between the two mouths, so a
3.5 m lane is 3.5 m at every interior point (interpolation drew 1.06 m on a reverse curve, 0.46 m at
a 90-degree arrival). Only the two end samples move.

**The step is the accepted shape now, not a bug.** A square end stands clear of the Link's lane
edges at an oblique arrival: 4.7-17.2 cm on a gentle join, 0.12-0.29 m where a Link has been
rotated under the curve, up to 0.88 m on a hard reverse curve, and up to ~1.8 m where the Connector
arrives square across the lane. The polygon overlaps the carriageway across that step.

**Tests were rewritten, not deleted.** The three that asserted the wedge now assert the square end:
`mouthLine` still checks the mouth is one straight line, a new `mouthSquareness` checks that line is
square to the boundaries' own end legs (exact where widths are constant, 1e-2 where a lane tapers,
because a tapering edge leans against the ribbon by construction), and `stepTo` bounds the step to
the Link's lane edge instead of demanding it be zero. 127 tests pass, 16/16 ctest.

### Next

The engineering side of M1 is unchanged by this: **the only open item is the owner's timed
four-leg / aerial-image / reopen exercise in `docs/M1_ACCEPTANCE.md`.** If the square mouth looks
wrong once seen in the editor, the wedge is in Git history at `55294fa` and its reasoning is in the
2026-09-17 entries below — do not re-derive it.

*Verified on Linux, headless preset only (Qt not installed here); no desktop verification claimed.*

---

## 2026-09-17 — The sharp point at a Connector's mouth was the re-miter, not the wedge

**The owner circled a Connector's mouth on our own render: it narrowed to a point where it met the
Link.** Three readings were measured and two of them were wrong, which is the only reason the third
was found.

| reading | what the measurement said |
|---|---|
| the wedge cut runs a tongue across the Link | no — mouth exactly 3.500 m at every arrival 10°–170°, at a Link end and on a Link body alike |
| the 2→1 lane taper closes to a point at the mouth | it does, but the owner confirmed the Connector is **2 lanes → 2 lanes** |
| **the mouth re-miter overshoots** | **yes: 5.59, 9.41, 14.31 and 18.11 m of mouth on a 7.00 m Connector** |

**What the re-miter does and why it ran away.** Where the fixed-distance cut would fold, every
boundary extends its own last leg to meet the Link's cross-section line instead. At a strongly
oblique arrival that line lies near the ribbon's own axis, so the intersection lands many lane
widths out. The bound added in `1f2f014` allowed 1.5 times the mouth's own width of overshoot,
which is nowhere near tight enough. Past about 9 m the two outer boundaries cross each other, and
the surface is filled from a closed ring, so `trimSelfIntersections` closed that fold into a point.
**The spike was the fill trim doing its job on a shape that should never have been handed to it.**

**The bound is now the mouth itself, not a distance to pick.** The fixed-distance cut puts boundary
i exactly on the Link's lane edge, so the mouth's span across the road is the lane widths and
nothing else; a re-miter may only redistribute corners inside that span. Three shapes are tried in
order of how much each is the Link's own cross-section — the fixed-distance cut, then the bounded
re-miter, then the un-cut end square to the Connector — and **the first that does not fold is the
mouth**, taken whole.

**The square end had to come back as the last resort, and the measurement is why.** Bounding the
re-miter alone left the fixed-distance cut folding on its own: the ring trim ate up to **7.67 m** of
mouth. Past roughly 50° off the cross-section a ribbon cannot be cut on a line that near its own
axis without folding, whichever corner placement is used. The 0.12–0.29 m step a square end leaves
against the road is the step Vissim's own screenshot of this joint shows, and the owner's Vissim
reference for it is parallel-sided and stops at the attachment.

**Nothing an ordinary joint draws moved.** Every Link-end attachment and every near-tangential
merge still takes the fixed-distance cut, bit for bit: all 126 existing unit tests passed unchanged
at every step, including the ones pinning the mouth to the Link's lane edges at 1e-9.

**Verification.** 16/16 CTest, 127/127 unit tests, architecture and size guards green. Over 12 Link
headings × 24 arrival headings at a body attachment: every mouth lane exactly 3.500 m to 1e-9, no
ring self-intersection anywhere, and 0.0000 m of mouth lost to the trim — against 18.11 m of mouth
and 7.67 m lost before. The new test fails on the old code with `10.452885 vs 3.500000`.

**Left alone deliberately:** a Connector whose two ends carry different lane counts still closes its
surplus lane at the mouth. The owner was asked whether that taper should move into the middle of the
body and answered to keep it as it is.

**The lesson, again.** The first two readings were built from the render and from the record, and
both were plausible. Only the third survived a measurement. `docs/VISSIM_PARITY.md` already carried
*"ask for a picture of Vissim before reasoning about it"*; the other half of it is **ask for a
number before believing the picture.**

---

## 2026-09-17 — M1.12.2: the miter "bulge" was a measurement, not a defect

**I was about to fix something that was not broken, and measuring first is the only reason I did
not.** The record said a 2→2 Connector through a sharp bend "bulges to 8.698 m of a 7.000 m width,
24% over ... the miter blowing out where the polygon turns hard". I built the case and measured it
three ways, on 90.47° of deflection:

| how the width is measured | reading |
|---|---|
| along the cross-section, at the mitered vertex | **9.9403 m** (+42%) |
| perpendicular, point to the far polyline | 7.0425 m (+0.6%) |
| **projected across the leg the vertex lies on** | **7.000000 m** (exact) |

The first is `width / cos(φ/2)` — the corner-to-corner diagonal of a correctly mitered joint,
which is *what the intersection of two offset legs is*, and what a road painted round a kink
actually measures across its corner. The original 8.698 m is the same identity at a gentler bend
(`7.000 / cos(36.4°)`). The carriageway square to the road never moved.

**The two tests I suspected were both right, and one of them already said so.** I had flagged
`bends_keep_their_full_carriageway_width` for asserting `3.5*sqrt(2)` at a right-angle corner, and
the `8e-2` tolerance in the connector tests. Reading them properly: the first asserts the
carriageway is **10.5 m projected across each leg** *and* `3.5/cos(45°)` between adjacent
boundaries at the vertex — both halves, deliberately. The second's comment states the distinction
outright: "along the cross-section a mitered corner reads wide … square to the road it is the lane
width". A previous session had already worked this out and written it down; the M1.18-era note
calling it "a real defect" was a mis-diagnosis of the same numbers.

**So `offsetGeometry` was not touched.** `network_tests` pins the miter to 1e-9, and removing it
would reinstate the pinch it exists to fix — 18% at 63°, 30% at a right angle. Fixing this
"defect" would have broken every bend in the editor.

**What was genuinely missing is now there.** Width had only ever been bounded from **below**
(`least > .9*3.5`), which is how a claim of 24% over stood unchallenged for a session.
`a_bent_connector_holds_its_width_square_to_the_road_from_both_sides` now asserts it **exactly**,
to 1e-9, on every interior leg of a hard bend, with the along-cross-section reading asserted first
as the forcing so the test cannot pass on a gentle curve. M1.12.1's curved bound went from
`span < 5.7` to the same equality. Both catch a 0.1% width error, checked by inflating the width
in `connectorLaneWidths`.

**Two things measurement corrected mid-flight.** My first forcing assertion required
`minimumRadius < width/2`; the fixture's radius is 17.9 m, so the assertion was simply false — it
is now the deflection angle, which is the property that actually produces the wide reading. And the
exact width was 5.8 mm over until I excluded legs touching either end: those run to a vertex the
**wedge mouth** moved (M1.17), so their direction is the Link's cross-section, not the Connector's.

**`tests/connector_tests.cpp` passed the 500-line guard**, so it split on the seam it already had:
topology (creation, references, retargeting, deletion, history) stays, and shape (width, markings,
bend radius, the mouth) moved to `tests/connector_shape_tests.cpp` with the measuring helpers that
serve it. Test names diffed against `git show HEAD:` — none lost, none duplicated, and no test's
own code changed; the only body differences are the relocated helper block and the new comment.

**The lesson worth keeping:** a distance between two boundaries is a *width* only when it is
measured square to the road. `perpendicular()` carried that warning in its own comment; the 24%
figure was taken with `apart()`, which does not.

**Verification:** 23/23 CTest, 126/126 unit tests, architecture and size guards green. No source
file in `src/` changed at all — this milestone closed on a measurement and a test.

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

## Next

**M1's engineering side is finished. What remains is the owner's, and only the owner's.**

> **Carry into the acceptance exercise:** the mouth spike fixed above was found from a render, not
> from a test, and the shape it settles is the one an author sees at every merge. When a Connector
> is drawn onto a Link's body at a sharp angle, check the joint by eye: parallel-sided, no point, no
> line running past the surface onto the Link.

Every M1 sub-milestone and carve-out is implemented: M1.1–M1.17, plus M1.3.1, M1.5.1, M1.11.1 and
M1.12.1, with M1.12.2 closed as a measurement error rather than a defect. `docs/ROADMAP.md` is the
authority on each; the bodies of the long-implemented ones live in
[`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md).

**1. Run the timed acceptance exercise** in [`M1_ACCEPTANCE.md`](M1_ACCEPTANCE.md). It is the only
thing that closes M1 and it cannot be delegated: an engineer draws a four-leg intersection with
turn pockets over an aerial image, from a blank editor, **under 10 minutes, without documentation
or assistance**, then the file is reopened and compared field for field. Not one row of that record
has been filled in. Merged code does not close it (rule 1), and a rehearsed retry is not the first
observation.

**Do this on Windows.** Everything in these sessions was verified on Linux only. `native.yml` runs
the Qt suites on Windows too, and CI run 105 previously failed in `windows-core` on a vcpkg
`z-applocal` race before any test ran — so a green Linux run is not evidence about the platform the
owner actually draws on. The M1.12 review checklist from the 2026-09-16 sessions is the list to
work through first.

**2. Record the M0 plausibility observation** separately — acceleration, queue at red, discharge at
green. Owner observation, not calibration, and not M6 validation. The not-yet-validated marker
stays either way.

**3. Decide the name (Q5).** D11 deferred it "until the end of M1", and that trigger is now live.
`Velk` is the strongest recorded candidate — clean on npm, PyPI and a brand search, with
`velk.com`/`velk.io` held, which is ordinary for a four-letter word. Do **not** re-derive the
candidates that were already rejected: `Headway`, `MicroFlow Simulator` and `Veytrix` all have
findings in the D11 row. **This is the owner's decision, not a session's.**

**Not started, and deliberately:** M2 implementation. Its gate is pre-registered and
`ROADMAP.md`'s "Pre-registered criteria: TO BE WRITTEN before M2 implementation starts" is still
unfilled. Starting M2 before writing them **voids the gate** (D8), and that gate is the honesty
check on the whole project's premise. Write the criteria first.

**M3 is open.** M3.1 supplied merge arbitration only — a deterministic gap-time/headway threshold,
not a calibrated critical-gap model. Conflict areas as editable input, priority rules as an
authorable object with their own UI, stop and yield control, crossing conflicts and signal heads
anywhere on a link are all still M3's, and its done-condition about minor-road delay responding to
gap time is not met.

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
| D20 | 2026-09-17 | **Implement M3.1 — merge arbitration by gap time and headway — rather than carve M1.11.1's second half out** | A Connector arriving inside a lane body is structurally a merge: the section downstream of the arrival has two predecessors. D13 forbids accepting that without gap acceptance, so M1.11.1's done-condition ("leaves **and** enters") was unreachable. The owner, shown the choice, chose to build the necessary part of M3 instead of deferring. Scoped to the one M3 bullet it needs — a priority rule with real seconds and metres — reusing the red-signal clamp rather than a second braking path, and relaxing `UNSUPPORTED_MERGE` **by construction** (n−1 of n predecessors must yield to another) rather than by removal. Filed at its number; **M3 is not closed** and its done-condition is not met. | Nothing about the mechanism. But if a later session reads M3.1 as "M3 is underway", the gate discipline is lost: M3 still owns conflict areas, authorable priority rules, stop/yield control, crossing conflicts and heads anywhere on a link. |
| D21 | 2026-09-17 | **Lane sections are derived every compile, never persisted** | `ROADMAP.md` forbids persisted duplicate runtime networks, and sectioning changes no authored id, so it is a pure function of the drawing — unlike `splitLink`, which must rewrite routes because the ids an author stored really do change. `sectionId(laneId, 0)` returns `laneId`, so an uncut lane compiles to exactly the `Segment` it always did, which is what keeps the four frozen baselines valid; breaking that one identity fails 54 tests. Route expansion lives **inside** `buildScenario` because `validateAuthoredDemand` compiles on every save, and anywhere else would break saving. | If sectioning ever becomes expensive enough to matter, cache it beside the document — but never store it in the project file, and never let an authored route name a section id. |
| D22 | 2026-09-17 | **A Connector's `laneMarkings` is indexed per interior divider, not per lane** | Vissim's `Lanes` tab field is per lane, but a lane has two edges and there are `paths + 1` boundary lines, so per-lane does not map onto them unambiguously — any choice is a choice. Per divider is complete and unambiguous, and the two outer edges stay solid because they are the edge of the carriageway, not a lane divider. **Not verified against Vissim**, and recorded as a chosen representation rather than a parity claim (rule 4). | A look at real Vissim showing the field means something else. The change is small — the vector's length and one index — so it was not worth blocking M1 to confirm. |
| D23 | 2026-09-17 | **The reported miter "bulge" is closed as a measurement error; `offsetGeometry` is unchanged** | Measured on 90.47° of deflection: 9.9403 m along the cross-section at the mitered vertex, 7.0425 m perpendicular point-to-polyline, and **exactly 7.000000 m projected across the leg**. The first is `width/cos(φ/2)`, which is what the intersection of two offset legs is — the diagonal of a correct mitered joint, not a bulge. The 8.698 m on record is the same identity at a gentler bend. Removing the miter would reinstate the pinch it exists to fix (30% at a right angle), and `network_tests` pins it to 1e-9. What was actually missing was an **upper** bound on width; it is now asserted exactly on every interior leg, and catches a 0.1% error. | Nothing, unless Vissim is shown to cut corners rather than miter them. The standing lesson: a distance between two boundaries is a width only when taken square to the road — `perpendicular()` says so in its comment, `apart()` does not, and the 24% figure was taken with `apart()`. |
