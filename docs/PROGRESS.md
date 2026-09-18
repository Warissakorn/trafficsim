# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks whole into `docs/archive/` if this gets
long. Older entries are preserved whole there:

- [`archive/PROGRESS-2026-09-18-flush-mouth.md`](archive/PROGRESS-2026-09-18-flush-mouth.md) — 2026-09-18, M1.18, the flush mouth
- [`archive/PROGRESS-2026-09-17-mouth.md`](archive/PROGRESS-2026-09-17-mouth.md) — 2026-09-17, the wedge mouth and the miter "bulge"
- [`archive/PROGRESS-2026-09-17.md`](archive/PROGRESS-2026-09-17.md) — 2026-09-17, later entries
- [`archive/PROGRESS-2026-09-17-early.md`](archive/PROGRESS-2026-09-17-early.md) — 2026-09-17, earlier entries
- [`archive/PROGRESS-2026-09-16.md`](archive/PROGRESS-2026-09-16.md) — 2026-09-16
- [`archive/PROGRESS-2026-09-14.md`](archive/PROGRESS-2026-09-14.md) — 2026-09-14
- [`archive/PROGRESS-2026-09-10--2026-09-15.md`](archive/PROGRESS-2026-09-10--2026-09-15.md) — 2026-09-10 to 2026-09-15

---

## 2026-09-18 — The middle of every Connector lane now lands on the middle of the Link lane it feeds (M1.19)

**The owner's requirement, after seeing M1.18's mouth in the editor:** each lane of a Connector must
line up with the lane of the Link it joins — not merely meet the Link's cross-section somewhere
along it.

**What M1.18 left.** It slid each boundary along its own curve until the mouth lay ON the Link's
cross-section, which made the mouth flush. But every boundary kept its full offset square to the
Connector, so resolved onto that oblique cut the lanes came out spread by `1/cos(arrival)`: the
mouth was the right line at the right angle and the wrong width, and each lane middle sat beside
the Link lane middle it feeds. Measured on a two-lane body attachment: **0.42 m out at the worst
arrival**, and a mouth spanning up to 28.9 m on a 7 m road at the clamp.

**What replaces it.** The offsets the boundaries leave each mouth at are no longer the Connector's
own stacked widths. They are read off **the Link's own lane boundaries at the attachment**,
projected onto the Connector's cross-section — one projection per boundary, then re-solved against
the end leg each boundary actually produces (`kMouthPasses`, a fixed 8 iterations; never a
convergence test, hard rule 2). The slide then lands each boundary exactly on the Link's own lane
boundary, so every lane middle coincides with the Link's, and the slide is `O(width)` rather than
`O(width/cos)` — the spike the `kMouthShiftLimit` clamp existed to bound no longer arises.

**Measured, on a two-lane Connector into a two-lane Link, worst lane middle off its Link lane's:**

| arrival | before | after | | mouth span (7 m road) | before | after |
|---|---|---|---|---|---|---|
| shallow | 0.42 m | 0.0001 m | | 0-60 degrees | up to 16.6 m | **7.000 m** |
| 45 deg | 0.16 m | 0.005 m | | 60-90 degrees | up to 28.9 m | 7.1-9.2 m |
| 75 deg | 0.05 m | 0.0006 m | | | | |

Over the 288-case heading x arrival sweep: **0 folds, 0 ring self-crossings** — unchanged, and
structurally so, because nothing here moves a vertex laterally past its neighbour. In the 84 cases
where the mouth's two outer edges land on the Link's outer edges, every interior divider and every
lane middle agrees with the Link's to **1 mm**.

**Three bounds, each there for a measured failure.** `kMouthSpanFloor` (0.25) stops the mouth being
compressed to a point where the arrival faces along the Link's cross-section and every Link lane
boundary projects onto the same place — the spike, in the other direction. `kMouthShiftLimit` now
also caps the offsets themselves: the solve is unbounded in that direction and produced a reading
of **3.6e7 metres** before the cap. And where the Connector's lane order runs opposite the Link's —
a Connector arriving from the far side — the mouth keeps the Connector's own cross-section, because
building it on the Link's would turn the ribbon over between its two ends: **120 of 288 sweep cases
self-crossed** until that fallback was added.

**M1.12.3 is closed by this, in the only way it can be.** The mouth is built from the **Link's**
widths, never an authored `laneWidths`: a lane laid at a width the Link does not have cannot have
both its middle on the Link's lane middle and its edges on the Link's edges — the two coincide only
when the widths do. The authored width takes over through the body, over a transition zone of one
carriageway width (capped at a quarter of the Connector each end). A Connector drawn as a single
straight segment has no interior vertex for its own cross-section to appear at, so its authored
width does not show; raise Intermediate points.

**Tests changed, not loosened.** The measure at a mouth is now the separation of the two edges
**along the cut** — which is what the mouth is — and that is the exact one: the taper fixture reads
3.000/4.000 m at its source and 3.500 m at its target to 1e-9, and the diverge's far mouth, bounded
at 2.8-3.2 m before, is pinned at 3 m. `squareWidth` at a mouth was dropped: it reads the lane over
the cosine of the arrival by construction, so asserting 3.5 m there was asserting a square cut.
Where a bound replaced an equality it carries its number: 2.8 mm where the slide is longer than the
boundary's own end leg, 3.1 cm inside a transition zone at a 90-degree arrival, 0.17 mm of solver
residue at the middle of a short Connector. `an_authored_width_is_exact_where_the_connector_is_
straight` was rebuilt on collinear Links so it has a body to measure, and now states both halves:
the author's width through the body, the Link's at each mouth.

**Verification:** 24/24 CTest on Qt 6.4.2 under `xvfb`, file sizes green.
`trafficsim-cli 42` unchanged at `meanDelay 29.249359418430977` — `connectorBoundaries` is
presentational and hit-test only. *Linux only — no desktop verification claimed.*

### Next

**The Connector keeps its own position** — the second half of what the owner asked for in the same
session: a Connector stores its geometry rather than having both ends recomputed from its Links on
every edit, can be dragged off a Link, and is deleted when either end comes off one. See the entry
below once it lands.

---

## 2026-09-18 — Snapping audited against Vissim: one gap closed, one instruction refused with numbers

The owner listed Vissim's four snaps and asked for all of them, opening with **"Vissim does not snap
to a Link end."** Audited against live code, the full table is in
[`VISSIM_PARITY.md`](VISSIM_PARITY.md). The short version: **one of the four was a snap gap and is
now closed; two are interactions we do not have; one is a file format we do not read.**

**Snap to Points is now complete.** `hitLanePosition` tries, in order, the lane's two endpoints, a
station another Connector already attaches at, then **the Link's own intermediate points** — the
new part. So a Connector meeting a Link at a bend lands on the bend. The vertex station is
accumulated with `polylineLength`'s own order and its own `hypot`, so the value stored is the one
measuring that prefix produces: the test asserts it with `==`, not a tolerance.

**The opening instruction was refused, and only a measurement earns that.** In our model an
attachment at a Link end is a distinct stored state — `station` absent — that `runtimeSections`
compiles to a departure rather than a cut. On a 50 m Link, drawn short of the end by:

| short by | station | result |
|---|---|---|
| 0.00 m | *(absent)* | one section, the end attachment drawn |
| 0.03 m | 49.9700 | **`unsectionable`, cannot Run** |
| 0.15 m | 49.8500 | **`unsectionable`, cannot Run** |
| 0.25 m | 49.7500 | runs, as a body attachment with a 0.25 m stub |

"Place it carefully by hand" is precisely the 9 mm / 5.8 cm failure measured on the owner's own
four-leg drawing the day before. Also: the owner's **own** *Snap to Points* item lists the End
point, so the opening line contradicts item 2 of the same list. Keeping the endpoint snap is the
Vissim-matching choice, not the deviation.

**Two of the four are not snapping at all**, and calling them snapping would have hidden their
size: heads, vehicle inputs and routes are **never placed by pointer** (`canvas_input.cpp:16-20`
takes `nearestLane()` and opens a dialog), and stop signs and PT stops are not object types —
§6 item 5, still blocked on engine behaviour. DWG/DXF snapping needs a vector import path where
`BackgroundImage` holds a base64 PNG: a milestone, deliberately **not booked**, no done-condition
written (hard rule 8 — do not start it without one).

**Verification:** 23/23 CTest on Qt 6.4.2 under `xvfb`, `trafficsim-cli 42` unchanged at
`meanDelay 29.249359418430977`. Deleting the new branch fails the suite with `A pick beside a
Link's intermediate point did not take it`. Linux only.

### Next

Unchanged and still the only thing that closes M1: the owner's timed exercise in
[`M1_ACCEPTANCE.md`](M1_ACCEPTANCE.md). The three snap-adjacent items above are scoped and
**not** booked — if the owner wants pointer placement for heads and inputs, or CAD snapping, each
needs a numbered milestone with a done-condition before any code.

---

## 2026-09-18 — A pick snaps to a station another Connector already attaches at

**The owner's own four-leg drawing would not Run, and the drawing was not wrong — the numbers
in it were.** `runDiagnostics` on it reported `UNSUPPORTED_CONNECTOR_POSITION` twice, and
`compileDocument` threw on the same two. Measuring what actually collided:

| lane | keeps its boundary | refused | apart |
|---|---|---|---|
| `lane-3` | `connector-30` (2-lane range) @ 49.376709 | `connector-32` @ 49.385893 | **0.009 m** |
| `lane-22` | `connector-46` @ 2.963768 | `connector-38` (2-lane range) @ 3.021435 | **0.058 m** |

Both pairs are two turning movements leaving or joining **one corner**, authored as separate
Connectors because they have different destinations — which is correct modelling. They differ
only by what a hand does with a mouse: 9 mm and 5.8 cm. `kMinSectionLength` is 0.2 m, so
`sections.cpp` rejects the second cut of each pair and `connectorRuntimeIssues` names its owner.
**The core is right and the guard is right**; nothing in `src/model` or `src/core` changed.

**The fix is at the point the mismatch is introduced.** `hitLanePosition` already snapped a pick
to a lane's two ends within `4/zoom` screen units; it now also takes the exact station of an
attachment already on that lane within the same radius. One branch, one file-local helper, in a
function both the two-click tool and the right-button range gesture already go through
(`canvas_input.cpp:23` and `:154`), so both ends of both workflows are covered by the one change.

**What it does not cover, stated rather than implied.** The radius is a screen distance, so past
roughly 20 pixels per metre it falls below `kMinSectionLength` itself and stops standing between
an author and a sub-0.2 m mistake. That is deliberate — an author zoomed that far in is asking
for fine placement — but it means the snap **reduces** this class of error, it does not make it
impossible. `UNSUPPORTED_CONNECTOR_POSITION` is still the backstop and still has to be.

**The test asserts its forcing first.** In `attachment_ui_tests.cpp`: the second pick really is a
distinct point, really is inside the radius, and the third really is outside it — all three
asserted before the equality, because without them the equality could hold vacuously. It then
asserts the near pick takes the first Connector's station **exactly** (`==` on the double, which
is meaningful because `attachmentStation` returns `*ref.station` verbatim), and that the far pick
**keeps its own** — a snap that always pulled to the nearest attachment would make two genuinely
distinct attachments unauthorable, and that is the failure this guards. Checked by deleting the
branch: the suite fails with `A pick inside the snap radius kept its own station`.

**Verification:** 23/23 CTest, architecture and size guards green, on a real Qt build (Qt 6.4.2
from the distribution) under `xvfb`. Linux only — `native.yml` also runs these suites on Windows
and nothing here has been near it.

### Next

The M1 position is unchanged: every sub-milestone is implemented and **only the owner's timed
exercise in `docs/M1_ACCEPTANCE.md` closes it.** Carry the file above into that exercise — it is
a real four-leg drawing that exercised a real authoring trap, and re-drawing those two corners on
a build with this snap is the cheapest check that the snap earns its place.

Two follow-ups were scoped and deliberately **not** done, so a later session does not re-derive
them:

1. **Name the conflict in the diagnostic.** Today `UNSUPPORTED_CONNECTOR_POSITION` says "move it
   0.2 m clear" without saying clear of *what* or by *how much* — I had to measure the table above
   with a throwaway tool. The enrichment belongs on `Diagnostic` (`diagnostics.hpp:8`), never on
   `core`'s `ValidationIssue` (D18a), but the data it needs is **discarded upstream**:
   `sections.cpp:66-71` keeps only the rejected Connector's own id, not the cut it lost to. So it
   is `sections.cpp` + `compile.cpp` + `diagnostics.{hpp,cpp}` — its own session, not an add-on.
2. **A station field in the inspector.** There is none (`editor_inspector.cpp`), so a station can
   only be set by dragging. Worth having for exact after-the-fact repair, but it is a new field,
   not a button.
---

## 2026-09-18 — An engine profile at scale: three measured optimizations, no behaviour change

**None of this is M1 work.** M1's engineering side is still done and the owner's timed gate in
`docs/M1_ACCEPTANCE.md` is still the only open item. This session profiled the engine and took
three wins that leave the trajectory byte-identical.

**The shipped scenario is too small to profile.** `crossing.json` is 31 trips in 180 s (57 ms), so a
12-intersection corridor at 900 s and a 40-intersection one at 120 s were generated by replicating
its validated pattern. Profiling a toy input finds toy problems.

**The profile said `std::string`, not simulation maths.** Of 1.55 G instructions, ~36% was string
handling: `memcmp` 9.6%, copy-ctor 5.8%, move-assign 5.2%, `operator==` 4.7%, plus dispose/traits.
Inclusive: `resolveRefs` 26.4%, per-tick copies 21.3%, the `Vehicle` sort 11.8%.

| # | change | 12-route | 80-route |
|---|---|---|---|
| 1 | `resolveRefs` through sorted id tables in `ScenarioIndex` | −6.2% | −33.5% |
| 2 | stop copying the vehicle list twice per tick | −5.6% | −6.1% |
| 3 | precompile `nlohmann/json` (build, not runtime) | −16% CPU | — |

Cumulative engine: **−11.5%** on the corridor, **−37.5%** on the 80-route network.

**Two things were predicted wrong and are worth remembering.** Item 1 was estimated at 25% and
delivered 6.2% on the corridor: the cost was never search asymptotics but the string comparison
itself, and with 12 routes and one vehicle type a binary search replaces ~6 compares with ~4. It
pays at 80 routes, which is the size that matters for a real study. And the first implementation
used `unordered_map`; `tools/check_architecture.cpp` rejected it immediately, correctly — core/ may
not use unordered containers at all, because their unspecified iteration order would leave
reproducibility to the standard library. Sorted vectors with `lower_bound` were the answer, with
ties ordered by position so a repeated id still resolves to its first occurrence.

**`trafficsim_shell` did not get the precompiled header.** It is the largest target at 23 sources
and would gain the most, but Qt is not installed here, so it was left alone rather than changed
without being compiled once.

**A clean checkout does not configure**: `nlohmann/json` is not found and CMake fails. `docs/BUILDING.md`
does list the dependency, so this may be the intended manual step, but the failure does not name the
package.

### Next

**Unchanged: the owner's timed four-leg / aerial-image / reopen exercise in `docs/M1_ACCEPTANCE.md`
is the only thing standing between here and M1.**

Two optimizations are diagnosed, measured and deliberately NOT done, in priority order:

1. **The precompiled header on `trafficsim_shell`** — one line, mirroring the three targets in
   `CMakeLists.txt`, but it needs a Qt-enabled machine to compile once before it is pushed.
2. **Take the three `std::string` ids off `Vehicle`.** This is the real fix for a small network, and
   it is what items 2 and 4 of the plan cannot reach: make a vehicle's identity its INDEX and derive
   `routeId`/`vehicleTypeId`/`inputId` from the scenario at the few points that need the name (event
   construction). That removes the per-tick compare, the per-tick copy and the 11.8% sort cost at
   once. It changes a core public type and every test that hand-builds a `Vehicle`
   (`tests/test.hpp:46`, `tests/core_tests.cpp:194`, `tests/attachment_tests.cpp:195`), so it is one
   system for one session, not a tweak.

The `std::sort` of vehicles by id (11.8%) was left alone on purpose: the element order it guarantees
is what makes a run reproducible, and it is not worth touching before the strings are gone.

*Verified on Linux, headless preset only (Qt not installed here); no desktop verification claimed.*

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
wrong once seen in the editor, the wedge is in Git history at `55294fa` and its reasoning is in
[`archive/PROGRESS-2026-09-17-mouth.md`](archive/PROGRESS-2026-09-17-mouth.md) — do not re-derive it.
*(It did look wrong; M1.18 above replaced it with a longitudinal slide, not the wedge.)*

*Verified on Linux, headless preset only (Qt not installed here); no desktop verification claimed.*

---

## Next

**One engineering item is open — M1.12.3 — and then M1 is the owner's alone.**

**0. M1.12.3 — the Link wins at the mouth for widths.** Filed in `docs/ROADMAP.md` with its
done-condition. An authored `Connector::laneWidths` still replaces the Link's width at *both* ends
(`road_boundaries.cpp:116-117`), so a Connector whose author typed a width does not meet the lanes
it attaches to even now that M1.18 puts its mouth on the Link. Give `ConnectorLaneWidths` a third
`body` vector, hold `source`/`target` at the Link's width, taper between them, and re-derive
`TIGHT_CONNECTOR_RADIUS` (`compile.cpp:73`) from a stated end of the profile.

> **Carry into the acceptance exercise:** the mouth is the shape an author sees at every merge, and
> both times it has been wrong it was found from a render, not from a test. When a Connector is
> drawn onto a Link's body at a sharp angle, check the joint by eye: it should now sit flush on the
> Link, spreading along its cross-section rather than stopping short of it — and never fold to a
> point. At very steep arrivals the mouth is deliberately long (up to 28.9 m for a 7 m ribbon);
> `kMouthShiftLimit` in `road_boundaries.cpp` is the one dial if that is too much.

Every other M1 sub-milestone and carve-out is implemented: M1.1–M1.18, plus M1.3.1, M1.5.1, M1.11.1
and M1.12.1, with M1.12.2 closed as a measurement error rather than a defect. `docs/ROADMAP.md` is
the authority on each; the bodies of the long-implemented ones live in
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
