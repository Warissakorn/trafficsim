# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the history and the
decision log; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Entries written before 2026-09-23 keep the `Next` they shipped with, as history.

---

## Immediate — the thread of work in progress

**M2 is under way (2026-09-24).** The owner ratified the gate (ROADMAP §M2, D34), then restated
the purpose — a simulator usable in real engineering work — and re-registered it as C1 + C2 with C4
recorded (D38). The owner also ruled on
M2.0.1 (D35), amber (D36) and the M2.1 rescope (D37). Implemented, each its own commit with
`core/`, the frozen fixtures and `trafficsim-cli 42` unchanged:

- **M2.0.1** Connectors meeting at a lane start are ordered by M3.1's derived rule in drawing
  order; the four-leg fixture is now the natural drawing and runs with no diagnostic.
- **M2.2** counted `intervals` on an input (paste a count column in the dialog), schema 10.
- **M2.3** `compositionId` from `data/compositions/` (`urban-mixed`, `car-only`) and a
  `heavy-vehicle` type — plausible, unvalidated.
- **M2.4** static routing decisions (Routing decisions tab), relative flows over routes leaving
  one Link.

- **M2.5** delay per movement and queue per approach for one run: the editor's **Results** tab and
  `trafficsim-cli --project FILE [--csv FILE]` (D39, D40; contract in `SIMULATION.md`).
- **M2.1.1** (owner request) a Vehicle input placed on a Link needs no route, and a routing
  decision can be placed on a Link with destination Links and relative flows, schema 11 (D42, D43).
  **For the gate study:** put counted turning volumes in a decision on the **entry** Link — there
  they hold exactly; further downstream the lanes limit them until lane changing exists.
- **M2.1.2** (owner choice) the decision dialog takes counted turning volumes per interval, one
  pasted row per destination, schema 12 (D45). The interval follows network entry time.

- **M2.7** (owner report, 2026-09-25) a Signal head is placed by a click at the pointer's station
  on a lane or Connector path, drags along its lane and is drawn as its stop line (D47); signal
  control is authored as fixed-time **Signal Controllers with Signal Groups** — a groups table,
  a timing-bar diagram, 2-/4-phase templates — compiled into ordinary core programs, schema 13,
  with schema 12 programs migrated colour for colour (D48). Verified locally on Linux only
  (offscreen UI tests and screenshots); `native.yml` runs the Windows UI suites.

**For the owner, before or during M2.6:** place the study's heads by pointer and type its timing
sheet into one controller. If a group needs something the dialog cannot say — a second green in
the cycle, red-amber, an intergreen check, detectors — that is M4, and it should be written down
in `M2_GATE.md` as friction, not worked around.

**M2's done-condition is met in code; the gate is not.** Next is **M2.6, the owner's gate study**
([`M2_GATE.md`](M2_GATE.md)): one real signalised study with protected phasing, from a blank
project to the Results table, timed against the owner's current tool (C1, C2), with C4 recorded.
Nothing in M3 starts before it passes.

**M3 preparation is recorded (2026-09-24, D41).** The owner asked to carry out the proposed
sequence. [M3_PLAN.md](M3_PLAN.md), [M3_CONTRACT.md](M3_CONTRACT.md) and
[M3_ACCEPTANCE.md](M3_ACCEPTANCE.md) now hold its contract and evidence design; no runtime
or schema change has started. The immediate action remains the owner's M2.6 observation,
not a request to approve the plan again. After C1/C2 pass and C4 is recorded in `M2_GATE.md`,
start M3.2.2: authored references/model, codec, commands and effective-priority resolver,
in that order, with A01-A08 evidence. Keep new controls Run-blocked until M3.2.3 implements
their runtime. Do not infer a gate pass from the instruction to follow the sequence.

**Engineering work that can proceed without the owner, if asked:**
- Deriving an input's interval volumes from its entry decision's turning counts, so a count sheet
  is typed once. Today the input's counts and the decision's are entered separately.
- An in-editor CSV export of the Results tab. The CLI has one; the editor only shows the table.
- A Results-tab refresh that skips work while the tab is hidden. It is cheap today (16 rows), so
  measure first.
- Removing the entry-acceleration bias needs travel-time sections (M5), not a correction factor.

**Open questions for the owner, none blocking:** motorcycles (not shipped; lane sharing is
unmodelled — Thai counts are motorcycle-heavy); per-interval turning proportions (M2.1).

**The measure-first optimization pass is finished.** Items 1–5 and 7 shipped; item 6 was built,
measured and **reverted**. Do not re-try any of these without a new measurement:

- **Item 6, reverted — do not retry.** `redraw()` copies every `Link` by value (`for (auto link
  : ...links)` in `src/editor/canvas.cpp`) so the primary can carry a drag preview. Copying only
  the primary was worth **−1.2% of `redraw()`** and −0.5% of the program, inside the wall
  clock's own spread, and it cost a scratch `Link` plus a reference-selecting ternary to read.
  A frame is dominated by `QGraphicsItem` construction — thousands of items against 159 Link
  copies — so the ratio does not improve at any network size. What is actually left in a frame
  is M1.23's culling and LOD work.
- **Deliberately not booked:** `compileDocument` (15.2% of instructions, but paid once per Run
  press, not per tick), and what is left inside `occupiedSpans`, which is the `push_back` work,
  not the search. The per-tick fleet sort is **done** — it is a merge now, −1.75% to −2.06%.

**Measure the engine with callgrind, not the clock, until this box settles.** Wall time swung
±7% on 2026-09-23, which cannot resolve a 3% change; instruction counts decided two calls that
day. The editor's timings are steadier (±2%) and the clock is fine there. Any harness driving Qt
must pump the event loop between iterations (D31) or it times Qt's deferred work instead.

**M1.26.1 is closed.** `VehicleInput.laneShares` (D32), the vehicle-input dialog's per-lane weight
fields, and the `demand-ui` test covering set/reopen/cancel are all in; see `docs/ROADMAP.md`'s
M1.26.1 entry. Not done by it: no gesture places a share from the canvas the way a route or an
input itself is placed by pointer, and the input table row (`refreshDemand`,
`src/shell/editor_demand.cpp`) still shows only the compiled equal-split figure even when shares
are set. If either turns out to matter before M1's acceptance exercise, they are small follow-ups
on the same dialog and table, not a new design question.

---

## Standing — the owner's items

**One engineering item is open** — item 0b below. Everything else here is the owner's.

**0b. The demand authoring gate, and what is still missing.** Routes and vehicle inputs are
authored by clicking and a route now names Links and Connectors (M1.25, M1.26 — see the top
entry), but the gate is the keyboard-only equivalent of both gestures plus the owner's timed
exercise below, and neither is done. **M1.26.1** (adjustable per-lane shares) is closed (D32, top
entry). Signal heads are placed by pointer since M2.7a (D47).
A routing decision as an object at a station along the link — what Vissim actually places, and
what would bring back the lane-specific route M1.26 gave up — is **M2.1**.

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

Every M1 sub-milestone and carve-out is implemented: M1.1–M1.20, plus M1.3.1, M1.5.1, M1.11.1,
M1.12.1, M1.21–M1.21.1, M1.24–M1.27 and M1.26.1, with M1.12.2 closed as a measurement error rather
than a defect and M1.12.3 closed by M1.19. M1.22 and M1.23 remain open. **M1.27.3 counted the
authoring gestures; a counted walkthrough is not the timed exercise in item 2 and closes nothing
of it.** `docs/ROADMAP.md` is
the authority on each; the bodies of the long-implemented ones live in
[`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md).

**2. Run the timed acceptance exercise** in [`M1_ACCEPTANCE.md`](M1_ACCEPTANCE.md). It is the only
thing that closes M1 and it cannot be delegated: an engineer draws a four-leg intersection with
turn pockets over an aerial image, from a blank editor, **under 10 minutes, without documentation
or assistance**, then the file is reopened and compared field for field. **2026-09-25:** the owner drew a
signalised four-leg in **9 min 40 s, no assistance**, and supplied the file: time and save/reopen
pass, but it has **no turn pockets and no aerial image**, so it is not the exercise as written. The
next attempt needs both; the owner still owes documentation use and first-attempt status. Merged code does not close it (rule 1), and a rehearsed retry is not the first
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

**M2's gate is registered** (ROADMAP §M2, D34, re-registered by D38 as C1 + C2 with C4 recorded).
M2.5 may proceed; the gate study itself (M2.6) is the owner's.

**M3 is open.** M3.1 supplied merge arbitration only — a deterministic gap-time/headway threshold,
not a calibrated critical-gap model. Conflict areas as editable input, priority rules as an
authorable object with their own UI, stop and yield control and crossing conflicts are still
M3's. Interior signal positions already exist in the model/runtime; their complete authoring
workflow and interaction evidence remain open. M3's done-condition about minor-road delay
responding to gap time is not met.
