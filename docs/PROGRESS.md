# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks whole into `docs/archive/` if this gets
long. Older entries are preserved whole there:

- [`archive/PROGRESS-2026-09-20-m1.21-authoring.md`](archive/PROGRESS-2026-09-20-m1.21-authoring.md) — 2026-09-20, M1.21, the supplied-spec audit and authoring foundation; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-18-connector-position.md`](archive/PROGRESS-2026-09-18-connector-position.md) — 2026-09-18, M1.20; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-18-m1.19-lane-middles.md`](archive/PROGRESS-2026-09-18-m1.19-lane-middles.md) — 2026-09-18, M1.19, the Connector lane middles land on the Link lane middles; moved out 2026-09-21 as the oldest live entry
- [`archive/PROGRESS-2026-09-18-square-mouth.md`](archive/PROGRESS-2026-09-18-square-mouth.md) — 2026-09-18, the square-mouth revert (M1.17 reverted); moved out 2026-09-21 when it was the oldest live entry and the parity audit superseded it
- [`archive/PROGRESS-2026-09-18-earlier.md`](archive/PROGRESS-2026-09-18-earlier.md) — 2026-09-18, the snapping audit, the engine profile and the M1.17 revert
- [`archive/PROGRESS-2026-09-18-flush-mouth.md`](archive/PROGRESS-2026-09-18-flush-mouth.md) — 2026-09-18, M1.18, the flush mouth
- [`archive/PROGRESS-2026-09-17-mouth.md`](archive/PROGRESS-2026-09-17-mouth.md) — 2026-09-17, the wedge mouth and the miter "bulge"
- [`archive/PROGRESS-2026-09-17.md`](archive/PROGRESS-2026-09-17.md) — 2026-09-17, later entries
- [`archive/PROGRESS-2026-09-17-early.md`](archive/PROGRESS-2026-09-17-early.md) — 2026-09-17, earlier entries
- [`archive/PROGRESS-2026-09-16.md`](archive/PROGRESS-2026-09-16.md) — 2026-09-16
- [`archive/PROGRESS-2026-09-14.md`](archive/PROGRESS-2026-09-14.md) — 2026-09-14
- [`archive/PROGRESS-2026-09-10--2026-09-15.md`](archive/PROGRESS-2026-09-10--2026-09-15.md) — 2026-09-10 to 2026-09-15

---

## 2026-09-22 — One window: the M0 harness window retired (M1.24)

**Request:** the owner asked whether the M0 simulation window could be removed from startup,
since the editor already runs, and clarified that this meant the **UI only** — `src/core/`, the
engine that produces the numbers, stays. Re-analysis found two of the session's earlier
objections wrong: `trafficsim-cli` already prints a superset of what that window showed
(`meanDelay`, `safetyClamps`, plus an event log `--events` the UI never had), and
`parseDocument` requires only `network` and reads `definition` when present, so the editor
opens a bare M0 scenario already. The owner then authorized all three parts together.

**Changed:** `trafficsim-desktop` opens the editor. `MainWindow` and `src/render/`
(`NetworkView`, which only that window used) are removed; `--scenario` opens either file kind
in the editor and `--editor` is accepted as a no-op so existing shortcuts keep working. The
editor's run status now carries the completed-trip mean delay and the safety-clamp count,
accumulated from `SimState::events` into the same `SummaryAccumulator` the CLI uses — every
path that advances the run funnels through one helper, because a step whose events are not
accumulated loses its clamps permanently. What the figures mean moved to the status tooltip;
the "not yet validated" marker was already on the editor's scope banner. Sixteen locale keys
that only the retired window used are deleted from both catalogs; `SCENARIO_*` codes stay
because `loadScenario` still produces them for the CLI. ROADMAP M0 now says where the
plausibility observation is made, without touching the gate itself.

**Verification:** `scenario-run-ui` replaces `desktop-controls` and is the stronger test: it
opens `data/scenarios/crossing.json` in the editor, steps 1800 fixed steps at seed 42 and
requires **31 completed trips and mean delay 29.249359418430977 within 1e-7** — the same
numbers CTest pins on `trafficsim-cli 42` — then that a finished run cannot step past its end,
that both figures appear in the status line, that Reset clears the summary, that an overflow
seed produces no run, and that the Thai status carries the delay. Linux GCC 13.3 / Qt 6.4.2
`check` passes **34/34** including architecture and file sizes; the headless preset passes
23/23. The finished run was rendered and visually inspected in both languages. Windows
evidence would come from CI, not these Linux results.

One regression was caught and fixed during the session: appending the delay caveat to the
editor's scope banner made that word-wrapped label taller, which shortened the canvas viewport
and moved `connector-ui`'s scene-mapped clicks onto the wrong lane. The caveat is a tooltip
instead. Any future addition to that banner will move the drawing the same way.

### Next

M1.22 remains open for geometry/snapping tools (including custom rotation pivots), layer
locks, bulk inspection, context menus and the keyboard-only owner exercise. **Perform the
owner's M0 plausibility observation in the editor** (open `data/scenarios/crossing.json`, Run,
watch queueing at red and discharge at green) and the timed four-leg/aerial-image/reopen
exercise in M1_ACCEPTANCE.md; automated checks do not close either gate. Exporting scenario
JSON from the editor is still not implemented — the editor reads M0 scenarios but saves
schema 7, so hand-writing is still the only way to make one; book it before promising it. The
shared-station runtime-section fix and the M3.2 conflict-policy follow-up remain separate work.
Do not fold simulation changes into the editor workflow.

---

## 2026-09-22 — Network Editor selection rotation (M1.22.2)

**Request:** continue improving the Network Editor after PR #47. That history/nudging change
is merged; its GitHub Linux and Windows matrix passed. The current main baseline passes all
32 desktop CTest suites locally. This session remains within the editor system.

**Changed:** Alt-left-drag rotates selected roads around their surface-bounds centre, with
an amber outline, pivot and angle preview; Shift quantizes to 15 degrees. The bilingual
Rotate selection action / Ctrl+Shift+R dialog accepts exact signed angles. Model helpers
share the affected object set, pivot and transform with the canvas. Each release or accepted
dialog commits one History transaction; zero/full turns preserve saved state and redo.

Internal Connectors rotate exactly once with both parent Links. Authored stations, frozen
lane blends, widths, markings and names survive; heads ride unchanged stations. Skipping a
station round trip for these rigidly carried Connectors avoids rewriting author numbers
through coordinate round-off. Partial edits use M1.20 reanchoring and existing reference
cleanup; Undo restores detached Connectors, dependent routes/inputs and heads together.
Clicks, centre singularities and cancelled gestures cannot commit stale releases. Help
explains the attachment cleanup and the fixed centre; no project-schema/runtime change.

**Verification:** four new model cases and a Qt rotation suite cover both driving sides,
curved unequal-width ranges, large-coordinate pivots, metadata/derived-boundary preservation,
partial attachment survival/removal, atomic undo/redo, invalid inputs, no-ops and save/reopen.
Canvas tests cover preview, coalesced release, Shift snapping, click thresholds, head-only
selection, cancellation, exact-angle dialog/shortcut, translation and run invalidation.
Linux GCC 13.3 / Qt 6.4.2 desktop `check` passes **34/34** suites, including architecture,
file sizes and the existing replay baselines. The rotation preview and wrapped Thai angle
dialog were rendered and visually inspected. Some local build executables needed their execute
permissions restored before the harness could start; the complete rerun passed. Windows evidence comes
from this PR's CI, not these Linux results; owner acceptance remains open.

### Next

M1.22 remains open for geometry/snapping tools (including custom rotation pivots), layer
locks, bulk inspection, context menus and the keyboard-only owner exercise. Perform the
owner's timed four-leg/aerial-image/reopen exercise in M1_ACCEPTANCE.md; automated checks do
not close M0/M1 acceptance. The shared-station runtime-section fix and M3.2 conflict-policy
follow-up remain separate work. Do not fold simulation changes into the editor workflow.

---

## 2026-09-21 — Network Editor history and keyboard workflow (M1.22.1)

**Request:** improve and fix the Network Editor. This session completes the history/nudging
slice of M1.22 and fixes two confirmed selection/gesture defects within the editor system.

**Changed:** bilingual History dock (Ctrl+Shift+H), named Undo/Redo, current/saved markers,
explicit Enter/double-click restoration, retained redo states and monotonically increasing
revision IDs after branching. The dock reads metadata from the existing History snapshots;
it introduces neither a second document store nor a new project schema. Restoring a state
clears a compiled run and refreshes all editor surfaces.

Arrow keys nudge Link/Connector selections by the snap grid, or 1 m with Snap off; Shift
multiplies the step by ten. The same group-translation command preserves M1.20 attachment
semantics and makes every step undoable. Head-only selections still ride their parent road.
Filtering levels drops hidden selections; explicit table/inspector selection reveals hidden
objects and synchronizes the filter. Unknown IDs are ignored. Active drags/copies/creation
cancel on focus loss, so late releases cannot commit; multi-click drafts survive ordinary
focus changes between clicks. Help/command names are translated in both catalogs.

**Verification:** Linux, GCC 13.3 / Qt 6.4.2. Original baseline suites pass (30/30); separate
regressions reproduce hidden selection and focus-lost drag commits against c69cad5 before
checking their fixed behavior. Desktop `check` passes **32/32** suites, including the new
history and workflow suites, all model cases, replay baselines, architecture and file sizes.
Coverage includes both driving sides, group/Connector nudges, save-point restoration,
branching, rejected/no-op edits, 100-entry eviction, run invalidation, field-focus isolation,
level/inspector synchronization, cancellation and Thai translation. The Thai History dock
was rendered and visually inspected. This is Linux evidence; Windows CI and owner acceptance
are separate. See [EDITOR_WORKFLOW.md](EDITOR_WORKFLOW.md) for exact controls.

### Next

M1.22 remains open for rotation, geometry/snapping tools, layer locks, bulk inspection,
context menus and the keyboard-only owner exercise; M1's timed gate remains open. The
previous session's shared-station runtime-section fix remains separate work, with its
existing regression and M3.2 conflict-policy follow-up. No runtime/curve-parameter or visual-
override work was folded into this editor change.

---

## 2026-09-21 — A toolchain, and the audit's §3.3 answered with a measurement

**Request:** the owner asked whether the toolchain was hard to install, whether VS Code could be
used, then *"do as recommended"* — install it and verify the build.

**The toolchain went into the WSL2 Ubuntu that was already on the machine.** No admin rights and
no reboot were needed, and the tight `C:` drive is irrelevant because WSL has 955 GB free:
`apt-get install g++ cmake ninja-build nlohmann-json3-dev qt6-base-dev` gives `g++ 15.2.0`,
`cmake 4.2.3`, `ninja 1.13.2`, Qt 6 Widgets and Qt 6 Test. This is the first time in this
workstream that anything was built at all.

| Preset | Build | Tests |
|---|---|---|
| `headless` | 65/65, exit 0 | **21/21 passed**, 4.30 s |
| `desktop` (Qt 6 Widgets) | 93/93, exit 0 | **30/30 passed**, 9.47 s |

`QT_QPA_PLATFORM=offscreen` is needed for the desktop suite on a displayless machine — the
checked-in `desktop` preset does not set it. **And ninja's default parallelism OOMs the Qt build
here**: the host has 7.66 GB and WSL is given 3.74 GB, so the first desktop build died with exit
code 15. Build with `-- -j 3`. That is this machine, not the project. The four gates that matter
for the audit all passed — `file-sizes` (so the `PROGRESS.md` archive move was required, not
cosmetic), `all-model-tests`, `architecture` and `reference`. **Linux only**, so this is not
cross-platform evidence; the Windows MSVC path is still unexercised.

**Finding 1 is answered, and the audit's first reading of it was wrong.** Two Connectors arriving
at the same station on one lane do **not** compile and then overlap — the second is **refused**.
`runtimeSections` (`sections.cpp:68`) tests each cut against `boundaries.back() + kMinSectionLength`,
and the boundary the *first* arrival just made is at that very station, so the second arrival is
measured against itself and lands in `table.unsectionable` → `UNSUPPORTED_CONNECTOR_POSITION`,
which blocks Run. Nothing is physically wrong with the pair: the cut the second needs is the one
the first already made. So the defect is the **refusal**, and "they do not yield to each other" is
a second question sitting behind it that this session never reaches.

**One test added**, `connectors.two_connectors_arriving_at_one_station_are_refused_though_one_cut_would_serve`
in `tests/connector_tests.cpp`. It asserts the behaviour **as it is**, not as it should be, and its
comment carries what it should assert once the refusal is fixed — so the fix turns the test into
the specification rather than deleting it. It follows the standing rule: the forcing comes first
(one interior arrival is clean, the cut at the drawn station survives, both ends really are inside
the body at the same metre), then the consequence.

**No production C++ was changed, and no milestone is closed.** The fix is a behaviour change to the
section table, which every other surface reads; it is one system and it is booked as M3.2 work
beside conflict areas. `CONNECTOR_PARITY_AUDIT.md` §3.3 is rewritten to the measurement and gains a
new §7 recording the verification above.

### Next

Two independent things, neither started. First, **the §3.3 fix**: make `runtimeSections` reuse an
existing cut when a second arrival lands on the same station within `kMinSectionLength`, instead of
rejecting it — the test above then flips to its commented-out expectation. That is a section-table
change, so it wants its own session and a check that nothing downstream regressed. Second, resume
the separately booked M1.22 features and the owner's M1.21.1 recheck against their original
`.traffic.json`. Do **not** start §3.2 curve parameters or §16 visual overrides. Keep M3.2
(conflict areas, lane changing) as the home for the §3.3 second half, §3.4 and §3.6.

---

## 2026-09-21 — Connector parity audit (documentation only; no code changed)

**Request:** the owner asked whether the Connector matches Vissim in every respect, then asked
for everything that could actually be done about it in the session.

**The audit itself** is `CONNECTOR_PARITY_AUDIT.md`. It is the first place in this repository that
states the **two benchmarks** side by side: the owner's supplied Thai specification (a *target*,
per `specs/README.md` and `SPEC_AUDIT.md`) and Vissim itself (**never measured here** — only the
owner's screenshots are on file). §1 of the audit lists what matches the specification, §2 what is
absent, §3 the defects, §4 what was deliberately not done. Do not quote §1's "matches" as a
Vissim claim.

**What was changed in this session** is documentation and comments only:

- `CONNECTOR_PARITY_AUDIT.md` — new.
- `VISSIM_PARITY.md` — a 2026-09-21 entry, plus a one-line inline "superseded" note on the
  2026-09-18 wedge entry, whose sentence *"a plain square end. That is now what is drawn"* has
  been false since M1.18.
- `connector_commands.hpp` — two header comments corrected against the code they declare:
  `changeConnectorGeometry` *does* move the endpoints (only the lane reference is fixed there),
  and `resampleConnectorPoints` splits the longest leg when **raising** the count rather than
  re-spacing evenly, because even re-spacing was measured to cut a hand-placed corner by 1.00 m.
- `CLAUDE.md` — a row in the "Read these before working" table.

**The stale `PROGRESS.md` entry was not rewritten — it was moved whole to
[`archive/PROGRESS-2026-09-18-square-mouth.md`](archive/PROGRESS-2026-09-18-square-mouth.md).** The
2026-09-18 entry *"The Connector mouth is a plain square end again"* is append-only history, so its
text is preserved exactly as written; only its **location** changed, and the reason is hard rule 6
— this entry pushed the live file to 504 lines, past the 500-line guard `tools/check_file_sizes.cpp`
fails on. It was the oldest live entry, so it is the one the file's own header says to move. This
entry is the correction of record: what is drawn today is the **M1.18 longitudinal slide onto the
Link's cross-section**, with a full-width square end kept only as the fallback for an arrival more
than roughly 75° off its spine (`road_boundaries.cpp:324-325`), reported as
`WARN_CONNECTOR_ALIGNMENT`.

**Three runtime findings are recorded, not fixed** — all need a build, and this session had no
C++ toolchain (`cmake`, `g++`, `cl`, `clang++`, `ninja` all absent), so under hard rule 7 nothing
was touched that could not be verified. Booked:

1. **Two Connectors arriving at the same station on one lane do not yield to each other.** The
   derived rule names only the upstream lane section, never another Connector's path
   (`sections.cpp:195`). Whether that permits a one-step overlap at the drawn station is
   **untested either way** — a test comes before any fix, and the fix belongs with conflict areas
   in M3.2.
2. **A merge at a lane's *start* is uncontrolled and unreported** (`sections.cpp:203`). Consistent
   with `UNSUPPORTED_MERGE`, but nothing tells the author.
3. **`buildScenario` can carry a zero-gap priority rule** when the catalog is unset; only the
   `compileScenario` path refuses it (`compile.cpp:95`, `core/validate.cpp:73`). Latent — no such
   caller exists today.

**No milestone is closed by this entry.** No gate is met by documentation. M0/M1 owner acceptance
and M6 validation remain open, and no test was added or run.

**Superseded the same day, in the entry above:** finding 1 was answered with a toolchain — the pair
is *refused*, not overlapped, so the audit's §3.3 has been rewritten and a test added. Findings 2
and 3 stand as recorded.

---

## 2026-09-21 — Network lifecycle correctness audit (M1.21.1)

**Request:** investigate and fix Network authoring failures, especially moving a Connector end
to the other side and perpendicular mouths becoming needles. This takes priority over the
previous M1.22 feature follow-up. `NETWORK_LIFECYCLE_AUDIT.md` records the state matrix,
confirmed defects and limits. The screenshots do not include the source project; fixtures
reproduce the mechanisms without claiming exact source coordinates.

**Changed:** shared explicit retarget rebuilds the directed curve at the existing point count,
clears stale blend/cross-section data where appropriate, and is used by the preview and command.
The two initial regressions failed against main: a backwards last leg after retarget, and a
stretched steep mouth. Near-perpendicular/backwards mouth fitting now uses full-width square
ends, never the ill-conditioned slide, and exposes a bilingual alignment advisory. Ordinary
mouth fitting and M1.20 Link-movement semantics remain distinct and preserved.

Range grips pick actual cross-section centres, including even counts/unequal widths. Group
drag and rectangle selection use release coordinates. Body tabs follow actual boundaries;
outward growth of a capacity-limited taper no longer contracts it. Direction sampling chooses
the incoming/outgoing segment at a vertex, rather than averaging a cusp to zero. Validation
rejects collapsed derived lanes/centrelines before the canvas/runtime can sample them;
collapsed drag previews remain drawable and reject atomically on release. Double-click
insertion cancels all transient drag state.

**Test coverage:** new lifecycle model/UI suites, 144 generated heading cases, 66 anchored
sharp-arrival fixtures, a 36-case range transition matrix, preview/commit equality, cancellation,
release-only input, offset collapse, serialization and exact Undo/Redo. The historical 288-case
mouth sweep now asserts a genuinely anchored fixture. The four `points` tests existed but
were not registered in CTest; they now run explicitly, and an unfiltered registry test prevents
that omission recurring. Core reference fixtures were not regenerated.

**Verification:** local Linux/Qt 6.5.3 desktop CTest passes **30/30**; the unfiltered model
registry includes 147 cases. See the associated PR for final Linux/Windows CI results. A local incremental
build left one generated test executable without its executable bit; a clean target rebuild
restored the normal build artifact. This was not a product assertion failure and no test was
disabled. Visual QA uses the real canvas/catalog with generated and 89/90/91-degree authored
joins on both driving sides. No automated result closes the owner's M0/M1 acceptance gates.

### Next

Review the lifecycle fixes against the owner's original `.traffic.json` if supplied, especially
previously authored near-perpendicular shapes: they are retained and diagnosed, not silently
regenerated on load. Explicit retarget/Reset curve produces a fresh directed turn. Then resume
the separately booked M1.22 authoring features; do not describe that feature scope as completed
by this bug-fix audit. Keep the finite coverage and the remaining manual acceptance explicit.

---

## Next

**One engineering item is open** — item 0 below, from the Connector parity audit. Everything else
here is the owner's.

**0. Fix the duplicate-station refusal in `runtimeSections`.** A second Connector arriving at a
station the lane is already cut at is rejected as unsectionable, because `sections.cpp:68` measures
its cut against `boundaries.back() + kMinSectionLength` and the boundary the FIRST arrival just made
is at that very station — so it is measured against itself, and Run is blocked for a pair that is
physically fine. Reuse the existing cut instead of rejecting it. Section table only, one system, and
the test already in `tests/connector_tests.cpp` flips to its commented expectation when it lands.
See the top entry of this file, and `CONNECTOR_PARITY_AUDIT.md` §3.3.

**1. Drive M1.19 and M1.20 in the desktop editor.** Both are measured at the model and command
layer only. Nobody has yet dragged a Connector off a Link with a mouse, or looked at a mouth on
screen. Watch for a Connector deleted by a Link drag the author did not expect to touch it (the
Undo is there; the surprise is the thing to judge), and for whether half a lane width is the right
distance for "off the Link" — one constant, in `laneContains`.

> **Carry into the acceptance exercise:** the mouth is the shape an author sees at every merge, and
> both times it has been wrong it was found from a render, not from a test. When a Connector is
> drawn onto a Link's body at a sharp angle, check the joint by eye: every lane of the Connector
> should meet the lane of the Link it feeds, middle on middle, and the mouth should span the Link's
> own carriageway rather than spreading past it. At arrivals steeper than about 60 degrees it
> cannot: `kMouthSpanFloor` in `road_boundaries.cpp` is the dial, and `connectorMouthFit` reports
> what a mouth could not reach, in metres.

Every M1 sub-milestone and carve-out is implemented: M1.1–M1.20, plus M1.3.1, M1.5.1, M1.11.1 and
M1.12.1, with M1.12.2 closed as a measurement error rather than a defect and M1.12.3 closed by
M1.19. `docs/ROADMAP.md` is
the authority on each; the bodies of the long-implemented ones live in
[`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md).

**2. Run the timed acceptance exercise** in [`M1_ACCEPTANCE.md`](M1_ACCEPTANCE.md). It is the only
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

**3. Record the M0 plausibility observation** separately — acceleration, queue at red, discharge at
green. Owner observation, not calibration, and not M6 validation. The not-yet-validated marker
stays either way.

**4. Decide the name (Q5).** D11 deferred it "until the end of M1", and that trigger is now live.
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
| D24 | 2026-09-22 | **Retire the M0 harness window; the editor is the only window** | The owner asked whether it could go, and the measured answer is yes: `trafficsim-cli` already prints a superset of the figures it showed (`meanDelay`, `safetyClamps`, plus `--events`), and `parseDocument` needs only `network`, so the editor opens bare M0 scenarios already. Keeping a second window meant a second renderer (`src/render/`), a second run loop and a second place for the delay figure to drift from the CLI. What makes the removal safe is not the deletion but the replacement: `scenario-run-ui` pins the editor's run of `crossing.json` to the CLI baseline exactly (31 trips, mean delay 29.249359418430977), so the M0 plausibility observation changed surface without changing meaning. The gate itself was not touched — only the sentence naming where the observation is made. | If a results screen ever needs to run a scenario without the authoring surface (a batch review window, M5), build it on `runSimulation` and the event stream, not by restoring `MainWindow`. If the editor ever stops opening bare M0 scenarios, this decision is void and the CLI becomes the only M0 surface. |
