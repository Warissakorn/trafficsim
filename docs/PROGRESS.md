# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks whole into `docs/archive/` if this gets
long. Older entries are preserved whole there:

- [`archive/PROGRESS-2026-09-16.md`](archive/PROGRESS-2026-09-16.md) — 2026-09-16
- [`archive/PROGRESS-2026-09-14.md`](archive/PROGRESS-2026-09-14.md) — 2026-09-14
- [`archive/PROGRESS-2026-09-10--2026-09-15.md`](archive/PROGRESS-2026-09-10--2026-09-15.md) — 2026-09-10 to 2026-09-15

---

## 2026-09-17 — Documentation tidied: one archive folder, one naming rule

Housekeeping only; no source file was touched. The docs tree had grown four archive files at
the top level, named after the **day they were written** rather than the entries they hold, so
`PROGRESS-archive.md` covered 09-14 to 09-16 while `PROGRESS-archive-2026-09-14.md` covered
09-10 to 09-14 — overlapping ranges under names that implied the opposite. `PROGRESS.md` itself
sat at exactly 500 lines, one line from failing hard rule 6, with a `## Log` tail of 2026-09-14
entries that belonged in an archive.

What changed:

- Archives moved to `docs/archive/` and renamed for the range they **contain**:
  `PROGRESS-2026-09-16.md` (7 entries), `PROGRESS-2026-09-14.md` (15),
  `PROGRESS-2026-09-10--2026-09-15.md` (14), and `VISSIM_PARITY-2026-09-16.md`.
- `PROGRESS.md`'s `## Log` tail moved into those archives whole. `PROGRESS.md` is now
  **284 lines**, 216 of headroom, and its header lists the three archives as a table of
  contents instead of a run-on sentence.
- Relative links inside the moved files repointed one level up; a link sweep over every
  `docs/**/*.md` and `CLAUDE.md` reports **no dangling targets**.

**Verification:** all 36 archived entries were diffed body-for-body against `git show HEAD:`
of the four source files — `lost: set()`, `gained: set()`, `bodies differing: []`. Nothing was
edited or summarised, only relocated, which is what `PROGRESS.md`'s own rule requires.
`trafficsim-check-file-sizes .` green; largest doc is now `docs/VISSIM_PARITY.md` at 491.

**Left alone deliberately:** `VISSIM_PARITY.md` is 9 lines from the guard and should be split
next, but its 2026-09-16 follow-ups are numbered `second`/`third`/`fourth` in the archive and
unnumbered in the live file, so the true order is not recoverable from the headings alone.
Splitting it on a guess would scramble a record. Recover the order from `git log -p` on that
file first, then move the four 2026-09-16 blocks (lines 317-407) below the existing pointer.

---

## 2026-09-17 — M1.18 reverted: it broke opening older project files

The owner tested the editor, found a Connector still wrong, and said to review the whole
thing. Reviewing turned up a regression I had shipped an hour earlier and had not tested for.

**M1.18 changed what `Connector::geometry` MEANS in the saved file** — from the first lane's
path to the middle of the range — with no migration. Every `.traffic.json` holding a multi-lane
Connector is then rejected on load:

```
a file drawn before M1.18: REJECTED -- DISCONNECTED_GEOMETRY: connectors[0].from
```

My verification measured that 120 of 120 drawn and driven vertices were unchanged, which was
true and beside the point: **I never opened a file written by the previous build.** A change to
the meaning of stored data needs a load test of the old data, and "the owner said not to worry
about save files" is not that test — refusing to open a file is different from migrating it.

Reverted whole, back to `30a212a`, which the tree now matches byte for byte. The owner chose the
revert over adding a schema 6 migration.

**What is still open, and what is not mine:**

- The pinch the owner photographed is **not reproduced**. Four fixtures — a bent Connector on a
  Link body, arrival angles 90° down to 5°, a 2→1 taper, and a save/reopen round trip — all hold
  93–100% of the full 7.000 m. Whatever causes it is not in those shapes, and the next session
  should ask for the project file rather than guess again, as three rounds of guessing from the
  screenshots each went wrong.
- **A real defect found while looking:** a 2→2 Connector through a sharp bend bulges to 8.698 m
  of a 7.000 m width, 24% over, at one sample. Present at `30a212a`, so it predates M1.18 and is
  its own bug — the miter blowing out where the polygon turns hard.
- CI run 105 failed in `windows-core` **before any test ran**: vcpkg's `z-applocal` post-build
  step raced with itself on two targets linking in the same second (`ERROR_SHARING_VIOLATION`,
  exit 32). Not the code; `windows-desktop` and the three Linux jobs passed.

M1.18 is unbooked in `ROADMAP.md` again. The owner's rule that prompted it — the midpoint of the
opening should sit at the attachment — is still unimplemented and still correct; it needs a
migration to land.

---

## 2026-09-17 — The mouth is a wedge cut on the Link

The owner circled the joint on a Vissim screenshot — a Connector arriving on a Link **body** at
an angle — and confirmed what to match: *ปากทางเป็นลิ่มตาม Link*. Vissim cuts a Connector's mouth
on the cross-section of the Link it attaches to. Ours was square to the Connector.

**The commit that made it square was justified with the wrong numbers.** `e6dd394` cited 1.06 m
of a 3.50 m lane on a reverse curve, 1.96 m at 60 degrees and 0.46 m at 90. Those belong to a
different defect — **interpolating** the cross-section through the body — fixed one commit
earlier in `e81a591`, whose own message says of the end cut: *"only the joint ... is shorter
through the corner, as it is in Vissim."* So the wedge had already been judged correct, and
`e6dd394` discarded it along with the interpolation, trading it for an overlap of 0.12-0.29 m.

The restoration is the pre-`e6dd394` projection, six lines: each boundary's end is placed at
`spine.front() + from · offset`, where `from` is the Link's own cross-section direction, which
`endCross` was still computing and throwing all of away but its sign.

**A design I proposed first was wrong, and measurement said so.** I planned to *shear* each
boundary's end along its own direction until it met the cross-section line. That lands on the
line but not on the Link's lane edges: the mouth comes out `offset / sin θ` wide. Measured, it
drew a 7.00 m mouth as 7.24 m at 30 degrees and 7.60 m at 120. The projection is both simpler and
correct — measured 7.0000 m at every angle.

**The guard that matters.** The cut must not creep into the body, or it rebuilds the very thing
`e6dd394` removed. Dumped every boundary vertex across six fixtures, before and after: 90
vertices, **36 changed, and all 36 are the two end samples — zero interior vertices moved**. The
body test was tightened from `> 3.4 m` to `= 3.5 m at 1e-9` to hold that line, measured against
the opposite edge's *body*, since a wedge segment is not a lane edge and measuring across one
reads 1.3 cm short without the lane being short.

**The cap I was advised to add is a no-op, so it is not there.** The concern was a wedge deeper
than its own opening leg folding over and feeding `trimSelfIntersections`. Built it: a 14 m range
whose first leg is 2.07 m against a 7.00 m half-width — a wedge three times deeper than its
opening. Zero self-crossings, and the trim never touched the mouth. Recorded rather than coded.

Mouth-to-lane-edge went from 4.7-17.2 cm, 0.12-0.29 m and 0.88 m to **0, to 1e-9**, and the three
tests that asserted squareness now assert exact landing instead — tolerances replaced by
equalities, not relaxed. Three negative checks each broke named tests: no cut at all, cutting
every sample rather than the two ends, and dropping the sign so lane order mirrors at the mouth.

One behaviour outside drawing: `canvas_spatial.cpp` builds the selection and hit-test outline from
these boundaries, so clicking a Connector at its mouth now matches what is drawn.

Booked as M1.17.

---

## 2026-09-16 — Moving several objects at once

The second gap the owner picked from the audit. Left-dragging a multi-selection did nothing:
`canvas_input.cpp` said in as many words that geometry editing stays single-object. Vissim has
always moved a selection, and the reason ours could not — reanchoring every attached Connector
— stopped being true with M1.14, where reanchoring became "move the one poly point attached to
the Link that moved".

**What moves.** Only Links carry geometry of their own, so they are what a move actually moves.
A Connector whose two Links are both in the moving set travels whole, keeping the points the
author placed in their places within the junction; one with a single end moving is an ordinary
Link edit, and reanchoring already does the right thing with it. Signal heads ride a station,
so they need no moving at all and must not get any. A selection holding no Link is a gesture
with no meaning rather than a move of nothing, and says `EDIT_MOVE_TARGET`.

**The drag threshold earns its line.** A press and release on the same pixel is already a delta
of zero, so it is safe without any threshold. What is not safe is a two-pixel tremor during a
click: if those two pixels fall either side of a grid line, the snap makes them a metre apart
and a whole junction jumps. The threshold is the only thing standing between a click and that,
and the gesture test pins it with a press point proven live by a longer drag from the same
pixel.

The preview outline is the copy drag's, which already drew the whole selection plus everything
riding with it at an offset; it now takes the offset from whichever gesture is running.

Four negative checks each broke a named assertion: leaving a Connector behind when both its
Links move, dragging a Connector whole when only one end moves, removing the drag threshold,
and never starting the group branch at all. Booked as M1.16; `Alt`-drag rotation is still not
implemented and still not booked.

---

## 2026-09-16 — A Name on every object, and the audit that found it

The owner asked what else still differs from Vissim. §§1–6 of `VISSIM_PARITY.md` are a
2026-09-14 snapshot and several of their "Today" cells have gone stale, so the audit was done
against live code. It found five gaps; the full table is in that file's sixth follow-up. The
owner picked the first.

**Nothing could be named.** `Link`, `Connector` and `NetworkSignalHead` had no `name` member at
all, and no dialog anywhere offered one — an interchange of forty links was forty opaque ids,
where Vissim puts `Name` beside `No.` on every object dialog and in every list. All three now
carry one: free text, at most 200 characters, and explicitly **not a key** — two objects may
hold the same name and an empty one is the normal state, which is why nothing looks an object
up by it. It is ordered last in each struct so that every existing brace-initialisation keeps
meaning what it says.

One field in the inspector's *common* section names whichever object is selected, rather than
three fields on three tabs, because in Vissim Name is a property of an object, not of a kind of
object. It commits on Return and on focus loss, but only when the text actually changed:
`editingFinished` fires on every click out of the field, and committing there unconditionally
put an empty entry on the undo stack each time. The three object lists gained a Name column
next to ID; the column loop now reads `columnCount()` instead of the literal 4 it was written
with, so the problem table's four columns still work beside the objects' five.

Verified: a name reaches the model, the list and the project file, comes back on reopen, copies
with a duplicated object and undoes as one entry. Five negative checks each broke a named
assertion — dropping the field from the JSON, dropping the length limit, clearing the name on
copy, never committing the field, and not reloading it on refresh.

**Not taken, with reasons.** Integer `No.` (item 4) churns the file format and every reference
for a mostly cosmetic win, and naming buys most of the same benefit. Missing object types (item
5) each need engine behaviour first. Editable lists and group move (items 2 and 3) are real and
unbooked; item 3's old blocker, connector reanchoring, no longer exists.

---

## Next

**Review M1.12 and run the owner acceptance exercise.** Check both-side lane growth,
road boundaries and Ctrl-click/Ctrl-drag on Links, Connectors and Signal heads. Check the
2026-09-16 grip work with them: lane tabs on the road edge, geometry grips on the bundle
centreline, and dragging a Connector end onto another lane or another position along it,
including onto a narrower Link, where the range narrows to the lanes that are there. Check a
bent Link keeps its width through the corner, and that a drag from one lane creates a one-lane
Connector. M1.13 is implemented: check that a Connector stays where it was
drawn when you stretch its Link, that shortening a Link clamps rather than refuses, and that a
project saved by an older build opens with its Connectors in the same places. Check a U-turn Connector draws as one clean ribbon and that no dashed line
runs down a single-lane stretch. Check a Connector between ends with different lane counts: the
lane that continues should hold its width while the extra one closes as a taper, and the divider
should arrive on the edge of the merged lane, never in the middle of it. Check a reverse-curve
Connector holds its width through the bend rather than pinching in the middle, and that moving a
Link under a drawn Connector moves only the poly point attached to it while the ribbon keeps its
width. Check the 2026-09-16 point work: a new Connector shows five grips rather than thirteen,
`Intermediate points` in Properties raises and lowers the count without losing the shape you
drew, a count of 2 draws three straight legs with a corner on each point as Vissim does, and a
Connector drawn between two links that nearly touch stays inside the junction and is reported as
a tight radius rather than drawn as a crumpled wedge. Old `*.traffic.json` files predating that
change were deliberately not migrated and will open with all of their stored points as poly
points. Check the mouth: a Connector's ends should sit
on the Link's lane edges with the markings running straight through, at any arrival angle and
after moving a Link under it. Check the group move: select two Links with a Connector between
them, drag, and see the junction move as one shape that one Undo puts back; check that a
selection of only Connectors or heads refuses with a reason. Check the Name work
too: name a Link, a Connector and a signal head, see each in its list, reopen the file and find
them still there. **Next after the review: M1.11.1**, which can now split a lane
at an attachment station that no longer moves. Test the workflow on the owner's
Windows desktop before claiming usability acceptance. M1.11.1 separately owns runtime
lane sections for interior attachments; Run correctly blocks those networks today.

1. Require Linux headless/desktop/release and Windows core checks to pass on the branch head.
   Qt 6.4 and nlohmann/json are available from the Ubuntu archive; all 23 desktop tests
   passed locally. Local dependencies were extracted into scratch because the package
   cache was not writable; no dependency workaround was added to the repository.
2. Run the blind four-leg/aerial-image/under-ten-minute/reopen task in M1_ACCEPTANCE.md
   and fill in the observed result. M1.7 owns this remaining gate; M1 is not closed.
3. Record the M0 queue/red/green plausibility observation separately. Keep the
   not-yet-validated marker and the merge/internal-source/cyclic-route guards.
4. Fix concrete usability failures before claiming acceptance. Do not begin M2
   implementation until its pre-registered honesty-test criteria are committed.

Implementation: `src/model/demand/`, `src/model/network/`, `src/commands/`,
`src/project/`, `src/editor/` and `src/shell/`. Current behavior is in NETWORK_EDITOR.md.

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
