# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the history and the
decision log; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Entries written before 2026-09-23 keep the `Next` they shipped with, as history.

---

## Immediate — the thread of work in progress

**2026-09-24: M1 reviewed and M2 planned — read [`M2_PLAN.md`](M2_PLAN.md) first.** M1 is
implementation-complete for its done-condition and gate-open; 36/36 tests and `check` pass on
Linux. The review found that **no fixture or test anywhere builds a four-leg intersection**, so
the network M1's gate draws and M2's done-condition runs has never been compiled or run. The plan
holds a *draft* of M2's gate criteria (C0–C4) and slices M2.0–M2.6. **The draft is not a
pre-registration:** nothing past M2.0 starts until the owner edits it into `ROADMAP.md` §M2 and
commits it.

**The four-leg fixture exists and runs** (`data/projects/four-leg-signalised.traffic.json`,
`fourleg` tests; M2_PLAN.md M2.0). It surfaced three items, each written up there: **M2.0.1**
turns meeting an exit at its start are refused as `UNSUPPORTED_MERGE` — the fixture staggers them
along the exit to run, and whether to pull start-of-lane arbitration forward is the owner's call;
**M2.0.2** an implied Link→Link route step silently drops a lane that reaches the next Link twice;
**M2.0.3** four safety clamps in 900 s, cause unknown.

**What a session may do now without the owner:** item 0 below (the duplicate-station refusal),
then M2.0.2 (report the dropped lane rather than lose it silently) and M2.0.3 (find the clamps).
All are M1-side defects, not M2 implementation, so they do not touch the gate. M2.0.1 waits on
the owner's decision.

PR #54's compact UI refresh is merged; native Windows appearance and display scaling ride on the
owner acceptance exercise below.

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

**Two engineering items are open** — items 0 and 0b below. Everything else here is the owner's.

**0b. The demand authoring gate, and what is still missing.** Routes and vehicle inputs are
authored by clicking and a route now names Links and Connectors (M1.25, M1.26 — see the top
entry), but the gate is the keyboard-only equivalent of both gestures plus the owner's timed
exercise below, and neither is done. **M1.26.1** (adjustable per-lane shares) is closed (D32, top
entry). Signal heads are the last object still placed only through a dialog;
`objectAt`/`inputPlaced` in `src/editor/canvas_demand.cpp` are the shape to copy, inside M1.22.
A routing decision as an object at a station along the link — what Vissim actually places, and
what would bring back the lane-specific route M1.26 gave up — is **M2.1**, and M2 may not start
until its pre-registered criteria are written into `ROADMAP.md`. Writing them is the owner's,
and it blocks all of M2.

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
check on the whole project's premise. A draft to start from is `M2_PLAN.md` §3 — its C0, the audit
of the owner's last studies against `PROBLEM.md` §2's walls, is best answered **now**, before any
M2 output exists to colour it.

**M3 is open.** M3.1 supplied merge arbitration only — a deterministic gap-time/headway threshold,
not a calibrated critical-gap model. Conflict areas as editable input, priority rules as an
authorable object with their own UI, stop and yield control, crossing conflicts and signal heads
anywhere on a link are all still M3's, and its done-condition about minor-road delay responding to
gap time is not met.
