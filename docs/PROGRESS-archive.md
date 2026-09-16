# PROGRESS archive — TrafficSim

Entries moved whole out of [`PROGRESS.md`](PROGRESS.md) to keep it inside the 500-line budget
(hard rule 6). Nothing here is edited or summarised — only relocated. Newest first.

The `Next` section, the backlog, the open questions and the decision table all stay in
`PROGRESS.md`; this file is log entries only.

---

### 2026-09-16 — Mitered bends, one-lane Connectors and the attachment-unit decision (M1.12)

Three more owner findings from an annotated screenshot.

**Bends pinched the carriageway.** `offsetGeometry` moved a corner vertex along the average
normal by exactly `offset`, which lands `offset*cos(theta/2)` from the original line, so both
lane edges pulled in and the road narrowed at every bend: 18% at 63 degrees, 30% at the right
angle in the screenshot. It now uses the miter vector `(n1+n2)/(1+d1.d2)`, whose length is
`1/cos(theta/2)` — exactly the distance to where the two offset legs meet. A turn sharper than
about 151 degrees is clamped to four times the offset so a hairpin cannot spike. One function
fixes Links, Connectors and the diagnostic view, because all of them derive from it.

Straight polylines reduce to the old single normal bit for bit, so nothing pinned moved: every
reference fixture and the pinned 29.249359418430977 run on straight-only networks. **No
baseline fixture was regenerated.** A measured sweep confirms the miter at 5/30/60/90/120/150
degrees and the clamp at 175. Known limitation, written into NETWORK_EDITOR and shared with
Vissim: a bend tighter than the offset still self-intersects on the inside.

**A Connector from one lane drew two.** The model and renderer were right — a 1/1 Connector
has one path, two boundaries and no dashed centre. The creation dialog pre-filled both counts
with every lane from the picked one to the end of the Link, so a single-lane gesture silently
authored a wide Connector. It opens at one lane per end now. The Properties counts also reset
to one when no Connector is selected, since those boxes double as the creation form and were
carrying a previous selection's width into the next gesture.

**Where an attachment lives (decision, no code change).** Connectors must keep following their
Link: validation requires the end to sit on its attachment and the compiler builds
lane -> connector path -> lane, so a Connector left behind in world coordinates is a network
the engine cannot run. The wrong part is the unit — a fraction of lane arclength slides every
interior attachment when a Link is stretched, and with it the lane-section lengths M1.11.1
will measure. Storing a distance, as `NetworkSignalHead::position` already does, is booked as
**M1.13** with the constraints that decide it: schema 5 must convert on read because
`parse.cpp` migrates by field presence with no version dispatch; the absent-optional sentinel
must survive or `connectorRuntimeIssues` flips existing Connectors into "Run blocked"; and the
duplicate-connection key contains the fraction.

Coverage: a 90-degree three-lane bend keeps 10.5 m across both legs on both driving sides,
each lane keeps its own width, the centreline stays the average of the outer edges, a hairpin
stays finite at the documented limit, and a straight link is untouched. A UI test drags a
Connector from one lane, accepts the dialog untouched, and requires one lane per end and
exactly two road markings. Both fixes were reverted in turn to confirm the new tests fail
without them. The Linux desktop build and all 23 CTest suites passed; the Thai editor
screenshot was inspected. M1.11.1, M1.13 and the owner's Windows/timed gates remain open.

---

### 2026-09-16 — Centred grips, end attachments and Vissim-style lane tabs (M1.12)

The owner's annotated screenshot marked four editor faults. Lane tabs were a loose orange
dot with a number floating beside it; they are now rounded tabs mounted on the road edge by
a short stem, carrying the resulting lane count inside the tab, and the held tab is darker.

Link and Connector geometry grips sat on the stored polyline, which is at one road edge as
soon as lanes are added to a single side, and on a Connector's first lane path. They now sit
on the centreline of the whole bundle: `linkCentreline` and `connectorCentreline` are the one
place that geometry is derived, and the direction arrow reuses the same function. Stored
geometry is unchanged; hit-testing and dragging map back through the same per-point offset,
so the point under the pointer is the point that moves.

Connector ends are draggable. Dropping an end grip on a lane re-attaches that end anywhere
along it; the range is centred on the lane under the pointer and slides to stay inside the
Link. Dropping off the network, or Esc, leaves the attachment alone, and one release is one
undo entry. `changeConnectorEndpoints` now narrows the range when the new end has fewer lanes
than the Connector carries, and validates the whole move before mutating, so a bad lane leaves
nothing half-moved. A wider Link never widens a range on its own. Properties shows both lane
counts beside the Connector length, so a 3 -> 2 drop is visible on the canvas.

`reanchorConnector` moved from commands into the model beside the attachments it reads, so
the canvas can preview exactly the re-attachment the command will perform. Referenced
Connectors still cannot be re-attached; unequal ranges remain legal drawings that fail the
M0 run check as `UNSUPPORTED_MERGE`.

Coverage: centreline equals the average of both road edges after one-sided growth; connector
centrelines equal the average of the outer boundaries and differ from the stored path; range
narrowing, no silent re-widening and rollback on an unknown lane; a UI test drags a Connector
end onto another lane at another fraction and checks Esc, an off-network drop and one-step
Undo. English/Thai help and NETWORK_EDITOR/VISSIM_PARITY describe the changed grips.
The Linux desktop build and all 23 CTest suites passed; the Thai editor screenshot was
inspected. M1.11.1 runtime lane sections and the owner's Windows/timed M0/M1 acceptance
gates remain open.

---

### 2026-09-16 — Fixed lane edges, road boundaries and Ctrl-drag copies (M1.12)

The owner's 1–3 lane examples exposed recentering and lane-centre dashes. Links now
have resize handles on both sides. `laneOffset` keeps the reference polyline and all
surviving lane positions fixed when one edge grows/shrinks, including curved links.
Inspector count changes and downstream pockets use the same edge anchoring. Opposite
carriageways retain the requested median gap after asymmetric growth/unequal widths.
Connectors have source, target and middle handles on both sides. Leading edits rebase
the first path with frozen `laneBlend` weights; surviving lane pairs keep their curves.
Schema 4 persists those values; schemas 1–3 retain their old zero-offset/arc-weight defaults.

Shared model boundaries supply solid road edges and dashed internal dividers in the
editor and diagnostic view. Picking, box selection and framing use road surfaces.
Ctrl-click adds selection, and Ctrl-drag of an already selected object previews and
commits one copy on release. Click jitter does not create copies or geometry edits.
Links copy internal Connectors/heads; standalone Connectors and Signal heads may copy
onto valid lanes at their original levels. Invalid drops roll back all selected objects.
Signal heads select themselves on canvas/tables and support copy/delete. Table selection
preserves multiple rows and Ctrl/Shift selection across object types. Demand is not copied.

Regression coverage includes curved/unequal-width links on both driving sides, fixed
opposite edges, Connector rebasing, migration, save/reopen, reference rejection and
atomic copy/delete. UI suites cover both-side handles, real Ctrl click/drag, release-only
copies, invalid drops, Escape, group dependencies, heads and one-step Undo. English/Thai
help and NETWORK_EDITOR describe the changed gestures. The Linux desktop build and all
23 CTest suites (seven UI suites) passed; the Thai editor screenshot was inspected.
M1.11.1 lane-section compilation
and the owner's Windows/timed M0/M1 acceptance gates remain open.

### 2026-09-15 — Ctrl-right release, body attachments and lane side handles

The owner reported a disappearing Ctrl+right-drag preview, endpoint-only Connectors,
and missing direct lane-count manipulation. The Select tool entered creation preview
but its release branch only supported Draw and Connect; release also trusted the last
mouse-move event. Select/Links now infer Link creation from empty space and Connector
creation from a lane, and commit the actual release position regardless of released Ctrl.
Esc, tool changes and dialog Cancel discard the gesture. Invalid targets report an error.

`LaneReference::fraction` stores an optional normalized lane-arclength attachment.
Missing values retain source-end/target-start semantics. Schema 3 persists positions and
rejects older readers; schemas 1/2 and bare M0 networks remain readable. Curve tangents,
reanchoring, per-lane paths, validation, duplication and split remapping use the same
attachment semantics. Distinct station pairs on the same lanes may own distinct
Connectors; duplicate pairs at the same stations remain rejected. A split through an
attachment within its 0.2 m continuity span is rejected before mutation.

Selected Connectors expose orange source/target side handles from one lane onwards.
The middle handle sets both ranges to the same count. Selected Links have a side handle
that adds/removes lanes while retaining existing widths. Counts and geometry preview
without changing History; one release commits one command, Esc cancels. Range limits,
referenced-lane/Connector guards, Undo/Redo and lane IDs retain their existing contracts.
The first lane is chosen in the dialog/Properties; the number of derived Connector paths
remains the maximum of its two ranges, not an independent internal lane topology.

Related review fixes: body picking honors visible levels; curve-handle z-order follows
its object; the inspector preserves precise fractions on unchanged Apply and bounds
counts by the selected lanes. Help now describes body picking, side handles and
Ctrl+Delete, and the tables footer correctly says Shift-click for multi-selection.

**Runtime boundary:** M0 still traverses whole lanes. `connectorRuntimeIssues` names and
selects interior attachments in Diagnostics, and compile/Run rejects them with
`UNSUPPORTED_CONNECTOR_POSITION`. Authoring and saving remain allowed. M1.11.1 books
lane-section compilation, route/control remapping and matching vehicle rendering;
no engine capability guard or fidelity marker was weakened to make a drawing runnable.

**Validation:** Linux Qt 6.4 desktop build and all 23 CTest suites passed (including seven
UI suites). New cases cover release without a preceding mouse-move, releasing Ctrl first,
Select-mode creation, two-click and drag body attachments, one-lane range growth,
independent end counts, middle/Link handles, invalid-target feedback, cancellation,
Undo/Redo, precise inspector Apply, schema round-trip, both driving sides, duplication,
link/width edits, splitting and runtime rejection. The existing four reference replays,
CLI result, architecture and file-size checks pass. A rendered Thai editor screenshot
was inspected. Local evidence is Linux only; Windows and other build presets are CI gates.
M0/M1 owner acceptance remains open.

### 2026-09-15 — Windows packaging build broken by a Linux-only test mechanism

`Package binaries` run 6 failed on `main` at 53f58e5: Windows x64, `m1-workflow`, "autosave
ran without a recovery lock". `Native C++` passed on every commit of the branch, and run 5 on
the previous `main` was green, so the break arrived with PR #15.

**Cause.** The lock-failure case added to `m1_ui_tests` forced `QLockFile::tryLock` to fail by
pointing `XDG_DATA_HOME` at a regular file. That variable is an XDG convention: Windows
resolves `AppLocalDataLocation` from `%LOCALAPPDATA%` and ignores it. On Windows the recovery
directory was therefore valid, the lock succeeded, autosave started, and the assertion that
autosave stays stopped fired — reporting a product bug that does not exist. The setup no-oped;
the product behaved correctly.

**Two failures, not one.** The mechanism was platform-specific, and the assertions were
ordered so that a no-op setup read as a product defect instead of a broken test. The test now
occupies the recovery directory's own path with a regular file, which no OS lets a file be
created inside, and checks `recoveryPath()` is empty *first*, failing with "could not force a
recovery lock failure on this platform" if the setup ever stops working. Verified both ways on
Linux: disabling the blocking file now reports the mechanism, not autosave.
`QStandardPaths::setTestModeEnabled(true)` also keeps the whole binary out of the real profile,
which it had been reading and deleting recovery copies from.

**Why it reached main.** `native.yml` gates every push and pull request, but its Windows job
configures `-DTRAFFICSIM_BUILD_DESKTOP=OFF`, so no UI suite ran there. The only Windows desktop
build lived in `package.yml`, which is `workflow_dispatch` and gates nothing. A Windows-only UI
regression could merge green by construction. `native.yml` now carries a `windows-desktop` job
building the full desktop under MSVC and running all 21 tests.

Recorded in CLAUDE.md so the next session does not repeat it: assert that a forced failure was
forced before asserting its consequence, never force one with a platform-specific mechanism,
and do not read a green Linux run as cross-platform evidence.

### 2026-09-15 — Connector reshaping made path-independent, and the cost of one edit

Review of M1.3/M1.4 found that `reanchor` displaced a connector's interior points relative to
its *current* geometry, so the transform composed across edits instead of depending only on
where the endpoints are. Moving a link away and back to exactly the same place left a
hand-tuned curve permanently deformed by 3.76 m on a 30 m connector, and two sequential moves
that together were a pure translation distorted it by 8.17 m. Undo was unaffected, because
History restores whole-document snapshots rather than replaying the command.

Interior points are now carried by the similarity transform mapping the old endpoint chord
onto the new one, written as the complex invariant z = (p-a)/(b-a). Returning the endpoints
restores the curve to 3.6e-15 m, sequential moves are rigid to 2.6e-14 m, and a simultaneous
move stays rigid as before. `reanchor` runs only from `changeGeometry`, `changeLanes` and
`changeDrivingSide`, never on load, so stored geometry is untouched until a project is edited.
`reanchor_preserves_points_and_lane_references` pinned the old displacement formula; it now
asserts chord-relative shape and an explicit away-and-back round trip, and fails against the
previous implementation.

`History::execute` serialised both documents to JSON on every edit to detect no-op commands.
Measured at 400 links that was 63 ms of roughly 83 ms, more than the command and its
validation together. The model types now carry value equality — `BackgroundImage` compares
image bytes rather than the shared pointer — and the check is a struct comparison. The second
full `validateDocument` is replaced by the revision-exhaustion test that was the only thing it
added. One edit on a 200-link network fell from 62.0 ms to 3.8 ms.

`changeConnectorGeometry` now rejects non-finite points, zero length and duplicate consecutive
points, matching `changeGeometry`; previously only the surrounding transaction caught them.
`changeLanes` no longer reports a pre-existing bad connector range as `EDIT_REFERENCED_LANE`.

Verified on both presets: 21 desktop tests including the six UI suites, 15 headless,
architecture and file-size guards. Two randomised invariant sweeps (300 trials each, both
driving sides, with signal heads, routes, inputs, splits, retargeting, curve edits and
deletions) report no dangling references and no detached connector endpoints before or after
every edit. The `cli` test still pins 29.24935, so replay is unchanged. M0 plausibility and the
M1 owner gate in `M1_ACCEPTANCE.md` remain open.

### 2026-09-15 — Recovery lock failure and the run overlay's style lookup

`buildRecovery` returned as soon as `QLockFile::tryLock` failed, which also skipped the two
toolbar actions built below it. A session that could not take its recovery lock therefore lost
`editorEmbedCatalogs`, an unrelated feature, and ran with autosave silently off after a single
error message. The lock now only gates autosave: both actions are always built, the failure
clears the stale recovery path, and the timer stays stopped rather than re-reporting the same
failure every 15 seconds. Recovery browsing is deliberately still offered, because
`recoverFile` acquires its own lock — adopting one now starts autosave through `startAutosave`,
so a window that began unlocked becomes protected as soon as it recovers a draft.

`drawRunItems` evaluated `runStyles_.at(location.segmentId)` as an argument to `marker`, so it
ran before `marker`'s own missing-segment guard. `style()` already falls back to the default
type, so the lookup is now total and an absent segment degrades exactly as one absent from the
geometry map. The underlying asymmetry is also gone: `clearRunFrame` cleared only the geometry
map while `setRunNetwork` clears all three, so the three maps now always hold the same keys.

Qt 6.4.2 and nlohmann/json are available from the Ubuntu archive, so this session built the
desktop preset and ran all 21 tests, including the six UI suites that earlier entries could
only send to CI. That also covers the previous entry's compilation change. A new case in
`m1_ui_tests` forces a lock failure by pointing `XDG_DATA_HOME` at a regular file, which
defeats the lock for root as well, and asserts the window keeps both actions, leaves autosave
stopped and stays editable. Against the previous code it fails with "lock failure removed
catalog embedding". The run overlay change has no dedicated test: reaching it needs a
segment-id desync that compilation and `setRunNetwork` currently make impossible.

M0 plausibility and the M1 owner gate in `M1_ACCEPTANCE.md` remain open.

### 2026-09-15 — Scenario compilation cost and split re-projection coverage

Review of the merged M1 branch found `buildScenario` resolving `connectorPaths` inside the
per-lane, per-connector loop. `connectorPaths` linear-scans `network.links` to resolve each
end, so compilation scaled roughly cubically in network size. This is not a batch-only path:
`refreshDemand` compiles on every model change and `validateDocument` compiles on every
15-second autosave, so a large network stalled the editor on ordinary edits.

Connector paths depend only on the network, so they are now derived once and indexed by
originating lane. Measured on a straight corridor with three lanes per link and three-lane
range connectors: 400 links fell from 10809 ms to 11.8 ms, and 800 links now compile in
44.6 ms where the old shape did not finish in reasonable time. Segment order, `next` order
and therefore replay are unchanged — the `cli` test still pins 29.24935 exactly.

`signal_bearing_split_preserves_control_and_routes` asserted which link or connector each
head lands on but never its re-projected station, leaving the lane-arc-length to
centreline-station conversion untested. It now pins the downstream station and, for the
non-pocket case, the exact world point on the gap connector. A pocket widens the downstream
link and shifts its lanes, so only the station is stable there; the world-point check is
deliberately scoped to the case where no lane moves. Verified non-vacuous by mutation: a
0.05 m drift applied only to the downstream head is caught here and by no pre-existing check.

Verified on the headless preset only. Qt 6 is unavailable in this environment, so the desktop
build and the four UI suites this branch added remain unverified locally. `nlohmann/json` is
also absent and was supplied through `TRAFFICSIM_JSON_INCLUDE_DIR`; no repository dependency
changed. M0 plausibility and the M1 owner gate in `M1_ACCEPTANCE.md` remain open.

The same review left two smaller Qt-side items, both since fixed — see the entry above.
---

### 2026-09-14 — M1 workflow completion and verification

Implemented the remaining M1 editor scope authorized by the owner: typed demand/control
commands and dialogs, revision-bound in-editor Run, controlled splits, schema migration,
locked recovery, connector lane ranges, sidebar gestures, levels and display catalogs.
The core and its capability/fidelity guards are unchanged. README, architecture, roadmap
and the editor guide now describe the implemented surface; M1_ACCEPTANCE.md supplies the
original timed acceptance task and a blank result record. M0/M1 owner gates remain open.

CI on ff4995a passed 19 of 20 desktop suites, including the complete drawing/demand/run/
replay/recovery workflow and new native range tests. The remaining table assertion still
expected unresolved catalogs; it now checks the catalog-resolved valid scenario. A new
offscreen gesture suite exercises Ctrl-right creation/cancellation, range corner resize,
Ctrl-left duplication, level order at two zooms, Tab, filtering and exact reopen.
The first gesture run exposed a test timer firing before mouse release opened its
modal; confirmation now waits for the dialog and never throws through a Qt callback.
Gesture tests explicitly reactivate the editor after a modal and release Ctrl before
sending the next canvas shortcut: the offscreen platform has no window manager.
A persistence review found that Undo to the saved revision could leave an older recovery
copy; the next checkpoint now removes it, with a UI regression covering that case.

Validation runs through GitHub Actions because the session executor is intermittently
unavailable and local Qt/CMake installation could not complete. No local interactive GUI
or owner timing result is claimed. The parity review is explicitly retained as a historical
assessment with a current implementation addendum.
---

### 2026-09-14 — M1 completion implementation in progress

The owner authorized the remaining M1 editor work together. The session executor is offline;
changes are prepared through the GitHub connector and verified by the repository's CI.
Base d456b121 passed Native C++ run 34824423877. No local desktop execution is claimed.

First slice replaces the document's untyped definition with optional typed authoring values,
retains version-1 JSON compatibility and explicit catalog override semantics, adds atomic
route/input/program/head commands, and introduces catalog resolution and revision snapshots.
Runtime limitations remain separate from draft validity. The second slice adds route/input/program/head dialogs and tables plus in-editor fixed-step Run/Pause/Step/Reset with seed and speed. Successful edits invalidate the run snapshot; frames repaint without rebuilding the scene. The first CI failure was a JSON-to-string comparison in the migrated regression test, corrected with explicit extraction. The third slice adds locked per-window recovery copies, atomic autosave, catalog embedding,
schema-1-to-2 loading and controlled-link splitting. Split heads are classified by their
original centreline station and projected onto the owning new lane or connector span.
The runtime core is unchanged. CI compiled the second slice, then the file-size gate caught
PROGRESS.md at 511 lines; older entries were moved whole to the existing archive.
An offscreen end-to-end workflow now covers drawing, demand dialogs, Run/Step/Reset,
seed replay, invalidation after Undo, recovery, Unicode persistence and Thai controls.
The fourth slice adds contiguous connector lane ranges, stable derived runtime path IDs,
level-aware scene ordering/hit-testing, data-driven display catalogs, the Network Objects
sidebar and creation/duplication/overlap gestures. Unequal ranges may author merges; M0
still rejects those at Run. Keyboard decisions: Shift extends selection, Ctrl-left-click
duplicates links and internal connectors/heads without demand, Ctrl+B toggles the image,
and Ctrl+Shift+O toggles object tables. Delete removes objects; Ctrl+Delete removes a vertex.
The fourth-slice CI passed Linux headless and Windows core. Desktop compilation passed;
three UI regressions exposed a topology-diagnostics early return, a fixture outside the
new viewport, and a seeded arrival later than the fixed sampling time. These are corrected
and range compilation, reference safety, duplication and migration regressions are added.
The remaining validation and acceptance work follows on the same branch; no milestone is closed by this checkpoint.
---

### 2026-09-14 — Scenario/project file-kind confusion, and the Vissim parity review

**Reported:** opening `network.traffic.json` in the simulation window failed with
`Could not open this scenario. … [json.exception.type_error.304] cannot use at() with null`.

**Cause, not a corrupt file.** `documentJson` always writes `definition`, and a network drawn
from scratch has none, so every such project saves `"definition": null` — correct for a
project. The simulation window's Open filter was `*.json`, which listed the editor's own
default save name `network.traffic.json`; `loadScenario` then called
`parseDefinition(value.at("definition"))` unguarded, landing on `.at("duration")` on a null.
The raw nlohmann text reached the user because the handler appended `e.what()` verbatim.

**Changed, in outcomes.** Picking an editor project in the simulation window now says what
kind of file it is, in English or Thai, and offers **Open in Network Editor** — one click and
the drawing opens in the window that can hold it. No load path can surface an nlohmann
exception any more: `loadScenario` classifies the file before reading a field
(`SCENARIO_IS_PROJECT`, `SCENARIO_NO_DEFINITION`, `SCENARIO_NO_NETWORK`,
`SCENARIO_NOT_JSON_OBJECT`, `SCENARIO_FILE_READ`, carried on a typed `ScenarioLoadError`), and
`parseDocument` guards the mirror-image holes a hand-edited project could hit
(`EDIT_NO_NETWORK`, `EDIT_BACKGROUND_INVALID`, and null `format`/`nextId`/`revision`). File
dialogs default to `*.traffic.json` for projects. Two new tests pin the reported shape itself:
a saved empty project must be *recognised*, not parsed and rejected.

**Second pass, same day.** A scrutiny round found the classifier had the same defect it was
added to prevent: `value.value("format", std::string{})` throws `type_error.302` when the key is
present but not a string — including null — so `{"network":{…},"format":null}` still leaked an
nlohmann message. Fixed, and `TEST(project, file_kind_classification_survives_broken_metadata)`
pins it (verified to fail against the previous classifier). A second finding: `what()` on a
classification failure is the bare code, and two paths showed it untranslated — startup
`--scenario` via `main.cpp`, and the editor-launch fallback. Both now route through one
`MainWindow::explain` / `EditorWindow::openFileOrReport`, and a `--scenario` the simulation
window cannot run is explained **in** the window, with the editor offered, instead of a fatal
modal carrying an identifier.

**Also:** `docs/VISSIM_PARITY.md` reviews the editor against Vissim — hand motions, keyboard,
window layout, objects, and the run/output story — and ranks the gaps. The three the owner
accepted are carved into `ROADMAP.md` as **M1.8** (Run inside the editor), **M1.9** (network
objects sidebar, Vissim gestures and shortcuts) and **M1.10** (levels and display types).
No milestone was closed and no editor feature work was done: 17/17 CTest green, 59/59 native.
---

### 2026-09-12 — native editor M1.1–M1.3 implemented (D16)

Added a version-1 ProjectDocument and a Qt-free command library. Every committed edit
validates a candidate before publishing it; a failed edit preserves both history and
the model. Undo/Redo keeps up to 100 document snapshots, persists the current revision
and ID counter in project files, and tracks the last saved revision. Embedded PNG data
is shared immutably between snapshots rather than copied for every gesture.

The independent Qt editor is available from the M0 window or `--editor`. It provides
metric grid/snap, pan/zoom/fit, single-link selection, point/link dragging, point insertion
and removal, lane count and individual widths, left/right driving side, opposite
carriageways, split links and an extra downstream pocket lane. Splits introduce a 0.2 m
continuity span with explicit lane connectors and remap existing routes. A link deletion
confirms its affected connectors/heads/routes/inputs and restores all of them on Undo.
Referenced lane removal is rejected. Connector endpoints reanchor on geometry edits.

Local background images are embedded, calibrated from two picked points and a known
real distance, positioned/rotated/scaled and given opacity. All background changes are
undoable. Basic Open/Save/Save As use a versioned JSON document and QSaveFile atomic
replacement; failed load/save preserves the current work. New/Open/Close ask about
unsaved changes. Native prompts and editor controls support English and Thai. The
inspector can be hidden, resized or detached. No new simulation behaviour was added.

Boundary enforcement now also rejects project-to-command/editor/Qt imports, with
negative fixtures. `docs/NETWORK_EDITOR.md` records operation, format and exact limits.
M1.3.1 explicitly tracks the unsupported signal-bearing-link split rather than moving
signal stationing silently. General connectors, tables, recovery and run handoff remain
M1.4–M1.7. Windows/macOS GUI execution and the owner's usability gate remain unverified.

**Verification:** GCC 13.3 / Qt 6.4.2 on Linux. Fresh isolated Debug and Release
builds passed all 13 CTest suites; a separate Qt-free build passed all 11 suites.
The native test executable contains 38 named cases (8 new editor model cases), and
Qt UI checks exercise actual mouse/keyboard drawing, dragging, insertion/removal,
pan/zoom, cancellation, per-lane widths, pockets, opposite carriageways, driving side,
confirmed deletion/Undo, image transforms/two-point calibration, Unicode paths,
failed save/load preservation, unsaved-work cancellation and Thai translation.
The existing four TS regression fixtures, deterministic core replay, CLI and M0
controls still pass. Architecture/negative checks, the 500-line limit and whitespace
checks pass. A Thai editor screenshot at 1000×760 was visually inspected. GUI behaviour
on Windows/macOS and GitHub-hosted runner execution are not claimed verified here.
