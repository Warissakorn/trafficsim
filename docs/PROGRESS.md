# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks to `PROGRESS-archive.md` whole if this gets
long. Older entries have been moved whole to [`PROGRESS-archive.md`](PROGRESS-archive.md).

---

## Next

**Implement M1.5.1 demand object tables, so M1.7 and then M1.8 have something to run.**

1. Build the desktop, run CTest, and launch `trafficsim-desktop --editor --language th`.
   A clean checkout needs `qt6-base-dev` and `nlohmann-json3-dev` installed first.
2. M1.5.1: routes and vehicle inputs are still untyped JSON under `ProjectDocument::definition`.
   Define the authoring structs beside `Link`/`Connector` in `src/model/network/network.hpp`,
   give them undoable commands in `src/commands/`, and table them next to the existing three
   tabs in `src/shell/editor_tables.cpp`. Reference-safety already exists for connectors —
   follow `deleteConnector`'s cascade, do not invent a second one.
3. Then M1.7 (catalog resolution and revision-to-run snapshot), then M1.8 (Run inside the
   editor). **M1.8 is the owner's first priority** but is blocked on 2 and 3: without demand
   there is nothing to run. Do not start M1.8 before M1.5.1 exists.
4. M1.6 (autosave/recovery, future-schema migration) is still open and independent; take it
   if demand authoring is blocked. Version-1 atomic save/open and embedded images exist.
5. Preserve the M1.3.1 signal-bearing-link split guard, pinned by
   `TEST(editor, signal_bearing_link_split_is_still_rejected)`. That follow-up still needs
   a stationing/remapping policy for heads in upstream/downstream and connector spans.
6. `docs/VISSIM_PARITY.md` §6 ranks the remaining editor gaps and says which are booked
   (M1.8/M1.9/M1.10) and which are deliberately not. Read it before proposing editor work.
   The M0 and full M1 owner gates remain open.

**Implementation:** `src/model/network/network.hpp`, `src/commands/`, `src/shell/editor_tables.cpp`.
Based on `main` commit `45ec8bf` (2026-09-14).

---

## 2026-09-14 — Scenario/project file-kind confusion, and the Vissim parity review

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

**Also:** `docs/VISSIM_PARITY.md` reviews the editor against Vissim — hand motions, keyboard,
window layout, objects, and the run/output story — and ranks the gaps. The three the owner
accepted are carved into `ROADMAP.md` as **M1.8** (Run inside the editor), **M1.9** (network
objects sidebar, Vissim gestures and shortcuts) and **M1.10** (levels and display types).
No milestone was closed and no editor feature work was done: 17/17 CTest green, 59/59 native.

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

### 2026-09-14 — M1.4 Connector editor implemented (D17)

Added general lane-to-lane creation using two canvas endpoint clicks or Properties.
Source/target markers, hover previews and cancellation are transient; committing creates
one History entry. Connectors can be selected on the canvas or by ID (including short
split connectors), reshaped by dragging/inserting/removing interior points, reset to a
lane-aligned sampled curve, or made straight. Endpoints remain attached to their lanes.
Properties now has Links, Connectors and Image tabs, with English/Thai controls.

Connector commands share endpoint maintenance with Link/Lane/driving-side edits and
route/input cleanup with link deletion. Retargeting preserves an unreferenced curve's
interior points by weighted displacement; referenced retargeting is rejected. Duplicate,
invalid and failed edits preserve revision, ID allocation, saved state and Redo. Confirmed
connector deletion restores related routes/inputs together on Undo. The existing format
persists exactly the edited polyline and IDs; no new schema, Qt dependency in the model,
simulation physics, demand or right-of-way behaviour was introduced.

**Verification:** GCC 13.3, Qt 6.4.2, nlohmann/json 3.11.3, CMake 3.28.3 on Linux.
The unchanged base first passed all 13 desktop CTest suites. The extended Debug and
Release desktop builds pass 15/15, and the independent Qt-free build passes 12/12.
There are 46 named native cases, including eight new Connector cases. UI workflows
exercise real endpoint picking, curve drags, insert/delete, cancellation and locked
endpoints, plus ID selection, Properties actions, reference-safe deletion/Undo, Unicode
save/reopen and Thai errors. Four TS baselines, seeded replay and M0 controls still pass.
Architecture/negative fixtures, the 500-line budget and whitespace checks pass. The Thai
Connector tab and curve were visually inspected at 1000×760.

M1.3.1 remains open, along with M1.5–M1.7 and the owner's M0/M1 acceptance gates.
These are local Linux results; Windows/macOS GUI execution and hosted CI are not
established by them. Curves are editable sampled polylines, not swept-path validation.


### 2026-09-13 — native migration and editor branches integrated into `main`

Merged `codex/cpp-desktop-migration` (D15) and `codex/network-editor-m1-1-3` (D16) into
`main` as two explicit merge commits. The migration commit is an ancestor of the editor
commit, so both branches shared one merge base at the last TypeScript commit `70383db`
and neither merge produced a conflict. No source or documentation was edited to make the
integration succeed; `main` now carries the C++20/CMake/Qt tree exactly as reviewed on
the branches.

Verified on the `headless` configuration only: full build clean, CTest **11/11 passing**,
including the architecture boundary, its negative fixtures, file sizes and the CLI checks.
**The Qt desktop harness and `editor_ui_tests` were not built or run** — no Qt in the
integration environment — so no desktop verification is claimed, per `docs/BUILDING.md`.
Building also required `nlohmann-json3-dev`, which a clean checkout must install first.

Neither the M0 acceptance gate nor the M1 gate is closed by this merge; merged code is
not a passed gate. `Next` is unchanged apart from its base note.
