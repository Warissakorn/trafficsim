# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks to `PROGRESS-archive.md` whole if this gets
long. Older entries have been moved whole to [`PROGRESS-archive.md`](PROGRESS-archive.md).

---

## 2026-09-15 — Connector reshaping made path-independent, and the cost of one edit

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

## 2026-09-15 — Recovery lock failure and the run overlay's style lookup

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

## 2026-09-15 — Scenario compilation cost and split re-projection coverage

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

## Next

**Run the owner acceptance exercise.** PR #14 is merged, its review is done and both code
findings from that review are fixed, so the owner gate is the only thing left open.

1. Require Linux headless/desktop/release and Windows core checks to pass on the branch head.
   Qt 6.4 and nlohmann/json install from the Ubuntu archive in this container, so the desktop
   preset and all 21 tests can be run locally; do that before relying on CI.
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

---

## Log

### 2026-09-14 — packaging workflow fixed: an action major that does not exist

The manually dispatched **Package binaries** run on `main` failed. The Windows x64 job died in
*Prepare all required actions*, before checkout or any compilation:

```
Unable to resolve action `jurplel/install-qt-action@v5`, unable to find version `v5`
```

Cause: `26d7399` ("ci: bump actions to Node 24 majors") rewrote five action references from
`@v4` to `@v5` across both workflows. Three were right — `actions/checkout@v5` and
`actions/upload-artifact@v5` are real Node 24 majors. **One was wrong: `jurplel/install-qt-action`
has no `v5`.** The bump was applied by pattern rather than by checking each action's own tags.
Reverted that one reference to `@v4`, with a comment naming this commit so the next
bump-everything pass does not redo it. The `actions/*` majors are left at `v5`.

Two things this was **not**: it was not M1.5, and it was not the main CI. `Native C++` is green
on all four jobs for `eb64079` — `linux (desktop)`, `linux (headless)`, `linux (release)` and
`windows-core` on MSVC — each running `--target check`, so the full suite passed on Windows too.
`native.yml` survived the same bump only because its Windows job uses vcpkg and never installs
Qt. The evidence that `@v4` works is in this repository: the packaging run 16 minutes earlier,
at `cf170d9`, built, tested, `windeployqt`-bundled and uploaded a Windows archive with it.

Also corrected a misplaced include found while tracing this: `src/editor/canvas_select.cpp`
uses `QLineF` for the rubber-band segment/rectangle test but did not include it, while
`src/editor/canvas.cpp`, which does not use it, did. It compiled only through transitive
inclusion from `<QGraphicsView>`. Moved to the file that uses it. No behaviour change.

**Verification:** 17/17 desktop CTest and 13/13 headless after the include move, `check` target
clean, `package.yml` parses. **The workflow fix itself is only proven by re-dispatching
Package binaries on `main`** — action resolution happens on GitHub's runners and nothing local
reproduces it.

### 2026-09-14 — M1.5 inspection and diagnostics implemented (D18)

Added a bottom Objects dock with Links, Connectors, Signal heads and Problems tables. Rows
are assembled on read from the document, carry the object ID they name, and select and frame
that object; the canvas selection is mirrored back into the tables. No selection state is
stored twice.

Canvas selection became an ordered list with the last-added object as primary. Ctrl- or
Shift-click toggles, and a drag on empty space rubber-bands links and connectors in network
order. `selected()` still returns the primary, so every existing single-object gesture —
vertex drags, point insertion and removal, connector endpoint locks, lane and split edits —
behaves exactly as before; the `editor-ui` and `connector-ui` suites pass unchanged.
**Group geometry dragging is deliberately not implemented**; property edits act on the
primary alone and say so. `deleteObjects` removes any number of links and connectors as one
History entry that one Undo restores whole, skipping IDs a link's own cascade already took.

Diagnostics are now structured and navigable. `validateNetwork` and `validateScenario` still
emit index paths; a model-layer resolver derives link, lane, connector and head IDs from
them on read, so `src/core/` was not touched at all. A rejected edit no longer throws its
`ValidationError::issues` away — they fill the Problems tab with the objects they name, and
selecting a row jumps to it. Runnability is separate and non-blocking: **Check runnability**
compiles the document and lists what the M0 core cannot run, such as an authored merge,
without blocking that edit or any later one. `compileScenario` was split so diagnostics can
assemble a scenario without throwing; its validate→build→validate order is unchanged.

A drawing with no demand is still checked for topology against a probe definition, with
demand findings dropped as meaningless rather than shown. Vehicle-type and behaviour
references are withheld with an explicit row when no catalog is loaded (D18d) instead of
being reported as unknown. 51 locale strings were added in both English and Thai, including
the 8 draft codes and 17 runtime codes that previously had no message at all.

**Verification:** GCC 13.3, Qt 6.4.2, nlohmann/json 3.11.3, CMake 3.28.3 on Linux. The
unchanged base first passed all 15 desktop CTest suites; the extended tree passes **17/17**
desktop and **13/13** on the independent Qt-free headless build. There are 57 named native
cases, including 8 new `diagnostics` cases and 3 new `editor` cases. The new `tables-ui`
suite drives real mouse and keyboard gestures: table-row selection, Ctrl-click, rubber
banding, cancelled and confirmed multi-delete with a single Undo, a rejected edit populating
Problems, jump-to-object from both a draft and a runtime row, and Thai tabs, headers and
messages. `TEST(diagnostics, every_emitted_code_has_a_translation)` derives its code list
from the validators rather than a hand-kept list. The four TS baselines, the trajectory
digest, seeded replay and the exact `29.24935` CLI pin still pass. Architecture and negative
fixtures, the 500-line budget and whitespace checks pass. A Thai screenshot at 1280×900 was
visually inspected.

**Not claimed:** no simulation, physics, right-of-way or demand behaviour changed. A clean
runnability check means the M0 core accepts the topology — it is not a fidelity claim, and
the not-yet-validated marker stands. These are local Linux results; Windows and macOS GUI
execution are not established by them. M1.3.1, M1.5.1, M1.6, M1.7 and the owner's M0 and M1
acceptance gates all remain open.

### 2026-09-14 — hot path 5: OccupiedSpan carries a segment index, not a segment name

After the previous slices, string copying was the largest remaining cost in the profile
(`_M_construct` 9.6%, string move-assign 5.5%, move-construct 4.2%). `OccupiedSpan::segmentId`
was the main source: a span is rebuilt for every vehicle on every tick, and since the
segment-bucketing slice nothing in `src/` read the field — `closestVehicle` uses
`segmentIndex`. The only reader left in the whole repository was one test assertion.

Dropping the string also removes a duplicated source of truth (hard rule 3): `segmentId` and
`segmentIndex` were two representations of the same fact, kept in step by hand. Callers that
want the name resolve it with `scenario.segments[segmentIndex].id`.

**This is an internal API shape change to `OccupiedSpan`**, recorded here deliberately rather
than slipped in: no observable output changes, `core/` has no consumers outside this
repository, and the compiler finds every use. `render/` and `eval/` never touched the field.

**Measured** (Release, GCC 13.3, median of 5): a uniform **-5.5%** across every network size;
1.499 s -> **1.416 s** at 466 vehicles. **The gain was much smaller than the 15-20% predicted
when this item was ranked.** The reason is short-string optimisation: ids like `road` and
`a0-1` fit inline, so the copies were never heap allocations, only inline byte moves. The
prediction was wrong in the plan and is corrected here so the mistake is not repeated.

Behaviour unchanged: 12/12 headless CTest including the trajectory digest and the exact CLI
value pin, plus 12 multi-seed multi-size CLI runs byte-identical to the pre-session binary.

**Verification:** headless preset only; Qt absent, so no desktop verification is claimed.

### 2026-09-14 — hot path 4: pending-vehicle insertion stops rebuilding every span

The pending-vehicle loop called `occupiedSpans` over the whole vehicle list **once per
candidate**, which was 42% of all span construction (122,061 of 288,400 `appendSpans` calls in
the profile). Spans and their buckets are now built once per tick and extended in place:
`appendVehicleSpans` adds exactly the spans a full rebuild would have appended for the newly
inserted vehicle, and the bucket fill is stable, so the grown structures are identical to what
a rebuild produced. Equivalence again rests on order, not on arithmetic.

**A first attempt built the structures unconditionally before the loop and was measurably
worse on small networks** — +21% at one corridor, +16% at four — because most ticks have no
arrival at all and previously did no span work whatsoever. Building lazily, only once a
candidate has survived the source filter, removes that cost: small networks return to parity
(+0.3% to +1.3%, inside run-to-run noise) and large ones keep the gain. The regression and its
cause are recorded here because the obvious eager version looks correct and is not.

**Measured** (Release, GCC 13.3, median of 5, identical commands):
0.028/0.049/0.117/0.292/0.670/**1.499 s** for 1/2/4/8/16/32 corridors — **-29.6% against the
previous slice at 466 vehicles**. Growth is now about **O(V^1.2)**.

Behaviour unchanged: 12/12 headless CTest including the trajectory digest and the exact CLI
value pin, plus 12 multi-seed multi-size CLI runs byte-identical to the pre-session binary.

**Verification:** headless preset only; Qt absent, so no desktop verification is claimed.

### 2026-09-14 — PROGRESS.md split, oldest entries archived

`PROGRESS.md` reached 513 lines and failed the 500-line budget (hard rule 6). Following this
file's own instruction, the naming-era entries of 2026-09-10/11 were moved **whole** into a new
`docs/PROGRESS-archive.md`; nothing was edited or summarised. `Next`, the backlog, the open
questions and the decision table all stay here, so a session with no memory still reads one
file to rejoin the work. The naming history those entries carry is already summarised in the
D9-D11 rows, which were not moved.

### 2026-09-14 — hot path 3/3: scenario lookups resolved once per tick

After the first two slices, string handling was still about half of all instructions, almost
all of it `detail::byId` doing a linear scan with an `std::string` compare per element. The
fix is to call it far less often rather than to make it cleverer:

- `resolveRefs` resolves each vehicle's route, type and behaviour **once per tick** into
  indices, replacing roughly six lookups per vehicle across the step loop, `occupiedSpans`
  and `locateVehicle`.
- `ScenarioIndex::routeHeads` precomputes, per route, the signal heads actually on it with the
  station of the first matching part — replacing an `std::find_if` over route parts with a
  string compare, run per head per vehicle per tick.
- `ScenarioIndex::programOfHead` plus a per-tick `headColors` vector evaluates each head's
  colour once per tick instead of once per head per vehicle; colour depends only on the tick's
  time, so every vehicle was recomputing the same answer.
- `locateOnParts` lets the step loop reuse the parts it already holds instead of looking the
  route up again.

Equivalence rests on order again: `routeHeads` is built in `signalHeads` order and records only
the first matching part, so each vehicle sees an identical sequence of heads and stations, and
the `allowedDistance`/leader updates fold in the same order as before. `byId` itself is
unchanged and still linear; it is simply no longer on the per-vehicle path.

**Measured** (Release, GCC 13.3, median of 3, identical commands):
0.028/0.048/0.117/0.298/0.740/**2.130 s** for 1/2/4/8/16/32 corridors — **-43% against the
previous slice at 466 vehicles**. Growth is now about **O(V^1.3-1.5)**.

**Cumulative for the three slices: 17.724 s -> 2.130 s at 466 vehicles, -88%**, and
0.067 s -> 0.028 s on the single-corridor case. Total instruction count on the profiling
scenario fell from 3.31 G to well under 1 G.

Behaviour unchanged throughout: 12/12 headless CTest including the trajectory digest and the
exact `29.24935` CLI pin, plus 12 multi-seed multi-size CLI runs byte-identical to the
pre-session binary at every slice.

**Verification:** headless preset only; Qt absent, so no desktop verification is claimed.

### 2026-09-14 — hot path 2/3: leader search grouped by segment

`closestVehicle` scanned every occupied span for every route part of every vehicle on every
tick, rejecting non-matching ones with an `std::string` segment comparison. That nested scan
was 39% of total instructions and carried the quadratic growth term.

Spans are now grouped by segment in a flat CSR layout (`start` offsets plus an `items` index
array), so a vehicle only ever looks at spans on the segments its own route actually uses.
`RoutePart` and `OccupiedSpan` carry a resolved `segmentIndex`, recovered during index
construction from the segment's address in the contiguous `segments` vector, so grouping needs
no string hashing. CSR rather than a vector-per-segment keeps this to three allocations
instead of one per segment, which matters because the pending-vehicle loop regroups per
candidate.

**Order is the correctness argument.** The bucket fill is stable, so each segment's spans keep
their original relative order, and the outer loop still walks route parts in order. The set and
sequence of spans that survive to the `gap < nearest->gap` test is therefore exactly what the
full scan produced, and that strict comparison keeps first-encountered-wins tie-breaking
unchanged. The dropped `span.segmentId != part.segmentId` test is now implicit in the bucket.

**Measured** (Release, GCC 13.3, median of 3, identical commands):
0.031/0.058/0.152/0.433/1.196/**3.731 s** for 1/2/4/8/16/32 corridors — **-67% against the
previous slice at 466 vehicles, -78.9% against the session baseline of 17.724 s**. Growth fell
from O(V^1.85) to about **O(V^1.6)**. The residual superlinear term is the per-candidate
`occupiedSpans` rebuild and the per-vehicle signal-head scan, both untouched here.

Behaviour unchanged: 12/12 headless CTest including the trajectory digest and the exact CLI
value pin, plus the same 12 multi-seed multi-size CLI runs byte-identical to the pre-session
binary.

**Verification:** headless preset only; Qt absent, so no desktop verification is claimed.

### 2026-09-14 — hot path 1/3: route geometry resolved once per run

`routeParts` was recomputed for every vehicle on every tick — 1,867,548 calls in an
8-corridor profile — even though it is a pure function of an immutable `Scenario`. A
`ScenarioIndex` now resolves it once in `createSimulation` and is carried through `SimState`
as a `shared_ptr`, so per-tick state copies share it rather than duplicating it. Routes live
in a contiguous vector, so `partsFor` recovers a route's index from its own address in O(1)
with no extra lookup. The index is built from the **canonical** scenario, so part order
matches the sorted routes.

The uncached `routeParts`, `locateVehicle` and `occupiedSpans` overloads are retained for
`src/render/` and the existing tests; cached and uncached paths share one implementation each
so they cannot drift. `stepSimulation` tolerates a hand-built state without an index by
building one, rather than requiring every caller to change.

**Measured** (Release, GCC 13.3, median of 3, identical commands as the baseline entry above):
600 s of simulation on 1/2/4/8/16/32 corridors went 0.067/0.142/0.488/1.739/4.876/17.724 s to
0.036/0.079/0.285/1.157/3.257/11.327 s, i.e. **-33% to -46%, -36% at 466 vehicles**. The
O(V^1.85) growth is unchanged and is deliberately left to the next slice; this change removes
constant work per call, not the quadratic term.

No behaviour change was intended and none was observed: 12/12 headless CTest including the new
trajectory digest and the exact `29.24935` CLI pin, plus 12 multi-seed multi-size CLI runs
(4 network sizes x 3 seeds) byte-identical to the pre-change binary.

**Verification:** headless preset only. Qt is absent in this container, so the three desktop
suites were not built or run; `src/render/` compiles against the unchanged overloads but no
desktop verification is claimed.

### 2026-09-14 — core hot-path optimization: measurement baseline and trajectory guard

Profiling (Release, GCC 13.3, callgrind) of a synthetic multi-corridor scenario shows engine
cost growing at **O(V^1.85)** in active vehicle count: 466 vehicles take 17.7 s of wall time
for 600 s of simulation. Attribution: `__memcmp_avx2_movbe` 30.9% of all instructions (linear
`detail::byId` searches over `std::string` IDs), `closestVehicle` 39.3% inclusive (nested
`parts x spans` scan, the quadratic term), `routeParts` ~35% inclusive over **1,867,548 calls**
recomputing a value that is constant for an entire run.

Before changing any engine code, per-tick trajectory is now pinned. The four frozen TypeScript
baselines deliberately exclude `MovedEvent`, so positions between the every-100-tick checkpoints
were unguarded. `tests/reference/trajectory-digest.json` records weighted means over the full
`MovedEvent` stream for the same four seeds; means (not sums) keep magnitudes physical so the
existing 1e-7 tolerance applies unchanged, and order/segment/vehicle weights make a reordering
visible that plain sums would hide. The four TypeScript baselines were **not** touched.

The guard was verified non-vacuous: perturbing only the reported position in `locateVehicle`
by 1e-6 relative — which changes `MovedEvent` but not checkpointed `distance` — fails
`meanOrderWeightedPosition` on all four seeds. A 1e-9 relative perturbation of acceleration is
caught by the pre-existing checkpoint comparison. Both perturbations were reverted.

These digests characterize what the engine currently does. They are not a fidelity claim and
do not affect the not-yet-validated marker or any milestone gate.

**Verification:** headless preset, 12/12 CTest. Qt is not installed in this container, so the
three desktop suites were not built or run and no desktop verification is claimed.


### 2026-09-14 — CI packaging workflow for testable binaries

Added `.github/workflows/package.yml`, a manually dispatched (`workflow_dispatch`) and
`v*`-tag workflow that builds, tests and uploads runnable binaries so the owner can try a
build without a local toolchain. Linux uses the `release` preset with apt Qt 6 and
`ctest --preset release` under the offscreen platform; Windows uses MSVC 2022, vcpkg
nlohmann/json and an aqt-installed Qt 6.5.3, then `windeployqt` so the archive runs on a
clean machine. Both stage the existing `install()` rules into `dist/` (desktop, CLI, data
catalogs) and add a `RUN.txt` that repeats the not-yet-validated marker.

Existing `native.yml` push/PR verification is unchanged; packaging is deliberately a
separate workflow so a slow Qt install never sits in the pull-request path. These are
unsigned test builds — installer work still belongs to M7, and no milestone gate is
affected. No engine, model or UI code changed.

**Verification:** workflow YAML parsed locally; the build itself is proven by the CI run,
not by this container, which has neither Qt nor nlohmann/json installed.

### 2026-09-14 — MIT License added

Added a top-level `LICENSE` (MIT, copyright 2026 Warissakorn) and a README License section.
The bundled Noto Sans Thai font keeps its SIL OFL 1.1 terms and Qt keeps its own; the MIT
grant covers this repository's own source and documentation only. No code change.

