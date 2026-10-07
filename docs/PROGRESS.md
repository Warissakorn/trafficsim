# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is the history and the reasoning** — what a session
reads to understand why the code is the way it is. What to do next is in
[`NEXT.md`](NEXT.md); the [decision log](decisions/RECORD.md) has its own indexed record. Never delete an entry;
move old blocks whole into `docs/archive/` if this gets long, and list each in
[`archive/README.md`](archive/README.md), which indexes every older entry.

---

## 2026-10-06 — Captured discharge inputs and physical-prefix recognition (D123)

M3.3.1b2a emits parsed-byte SHA-256/size/read counts for project, actual catalog
reads and queue definitions, with logical names and optional fallback scopes.
Changed repeated reads reject before stepping; no hashing/I/O reaches core/eval.
Positioned suffix choices on the same physical prefix retain queue membership;
diverted/lateral remaps suppress raw old-head crossing inference and stay unavailable.
Complete lateral/source reconstruction remains M3.3.1b2b; BA05 does not close.
See [contract](reference/DISCHARGE.md) and [evidence](evidence/discharge-provenance.md).

Validation: 72 focused/relevant legacy GCC/Linux tests pass, including frozen
reference fixtures; seeded CLI/hash parity and guards pass (see evidence). Parent PRs #117/#118 pass
native CI; the candidate's native/desktop CI is separate. CMake/Qt are absent locally.
No engine/schema/frozen data changed; no owner or calibration gate closes.

Follow-up: workflow 457 passed Linux but both Windows jobs failed the LF-only fixture hash.
The test now reads actual checkout bytes; controlled LF/CRLF JSON retains distinct hashes.
The old assertion fails on a CRLF copy; the corrected suite passes 73 Linux tests and
7 CRLF-copy provenance tests. See evidence; fresh native Windows CI remains required.

---

## 2026-10-06 — Declared discharge windows and type selection (D122)

M3.3.1b1 exposes CLI windows/warmup/ranks and repeatable vehicle-type selection.
Headways retain original follower/predecessor pairs and ranks; filtered samples
cannot bridge skipped types. Raw crossings stay intact. A mixed startup prefix
has a separate unavailable reason without discarding valid selected headways.
Unknown types and invalid/orphan controls reject before stepping the engine.
Input-hash output and complete remap/source passage tracking remain M3.3.1b2.
See [contract](reference/DISCHARGE.md) and [evidence](evidence/discharge-controls.md).

Validation: 15 focused GCC/Linux tests pass, including the existing trajectory
comparison. CLI integration and repository guards are recorded in the evidence.
Native/desktop Linux/Windows CI remains independent; no owner/calibration gate closes.

---

## 2026-10-06 — Queue discharge and startup measurement (D121)

M3.3.1a adds a stdlib-only observer, pure rank estimator and CLI `--discharge` JSON.
Lane/cycle records retain type identity and queued-at-Go membership; tracked sink
arrivals are counted. Windows, ranks, timestep, signed startup estimates and
unavailable reasons are explicit. StopLine output and engine code are unchanged.
BA03 class filters and BA05 complete remap/source support remain M3.3.1b; neither
measurement nor owner/calibration gates close. See [contract](reference/DISCHARGE.md)
and [local test evidence](evidence/discharge-measurement.md).

Validation: 10 GCC/Linux tests pass; direct full CLI link and seeded JSON parity;
documentation, architecture and file-size guards. CMake/Ninja/Qt are absent; native/desktop
CTest must run in CI. No frozen baseline was regenerated.

---

## 2026-10-06 — Driving behaviour contract before implementation (D120)

Owner authorized the staged plan following the supplied Driving Behavior design.
M3.3.0 records class/default/legacy precedence, compiler/runtime ownership,
front-segment tick selection, source and D119 routing integration, shared catalog
lifecycle, and the capability map separating prototype support from W74/W99/lateral
work. BA01–BA20 are pending failure-first rows, not passed runtime tests. Measurement
specifies lane/cycle/rank headways and startup estimation before changing diagnostic
code; presets and PCU target ranges remain unvalidated. No engine/schema/UI changed.

Validation: baseline and edited docs/navigation, file-size and architecture guards
compiled/run directly with GCC on Linux; all passed. `git diff --check` passed.
CMake/Ninja are absent on this host, so native/desktop CTest was not run locally;
CI is required independently. Existing prototype fixtures are unchanged. No owner,
M0 or M6 gate closes; current resolver remains type-based.

---

## 2026-10-06 — Routes recognized at clicked Link stations (D119)

Owner authorized choosing a destination when vehicles reach the first Route click.
An optional RoutingDecision.position stores a reference-polyline station; positioned
documents opt into schema 20, while absent positions retain legacy demand-time booking.
The Route gesture creates/reuses the station atomically with its traced Route. A shared
crossbar previews and selects the point; dragging commits one Undo step. The dialog can
opt existing decisions in. Link inputs use the decision; explicitly assigned Route inputs
retain their assignment. Pending demand preview destinations are labelled as deferred.

Neutral provisional route families preserve source volume and lane shares, retain all
period/type alternatives and distinguish different traces to the same end Link. At passage,
the core draws once using seeded RNG and passage-time half-open interval weights, records
the decision on the vehicle and emits a routing event. Compatible physical prefixes retain
distance and lane; generated lateral awareness starts at the station. Legacy downstream
bookings remain intact. Decisions in active conflict reservation spans are refused.

Recognition caps the crossing tick's displacement at the line and retains computed speed;
the selected suffix receives normal safety checks on the next tick. Excess proposed travel
is discarded, so recognition time is quantized by dt; whole-trip timing impact is unmeasured.
The initial slice retains one decision per Link and excludes Connector stations. The
contract and independent M2.1.3 platform/owner gates document these limits. Oldest complete
D105/D106 progress entries moved to the indexed archive to keep this file near 500 lines.

Validation: GCC 13.3/C++20 and Qt 6.4.2 desktop build on Linux. All **91/91 CTest groups**
pass after updating the existing future-schema rejection example from 20 to 999 and
rerunning that affected group. New core/compiler coverage includes nine station cases;
Qt offscreen coverage verifies gesture, overlay clipping, grouping, drag/cancel, Undo and
save/reopen. Frozen references and architecture/file-size/documentation guards pass.
Native Linux/Windows CI and owner appearance/fidelity remain separate gates.

---

## 2026-10-06 — Mixed conflict sites and P3–P4 continuation (D118)

Owner authorized grouping all three kinds for a connected owner-pair site, 0.5 m per-side
rail offsets and directional continuation through P3–P4. Groups now retain their full kind
list; Crossing/Merge share controls by owner ID, Branching stays derived/read-only. The
mixed table/dialog identifies it separately from editable parameters. Direct group commands
stage and validate the candidate and reject newly introduced merge-order cycles atomically;
History still publishes one Undo step. Merge takeover preserves complete topology controls.
Offsets are normal to rails, capped at 20% of normal local width for narrow/tapered lanes.
Shared band outlines feed paint/picking. Valid mouth caps are clipped to their attached Link
lanes; cap pieces support physical grouping and finite Link stations support directional
continuation. Caps can lie on either side of a join, so no source/target station-side clamp
is imposed. Stored entryStation–exitStation, schema, runtime paths and solver remain unchanged.
M3.2.4f is carved separately; physical-mouth admission remains M3.2.4e. Regression coverage
adds both traffic sides, oblique offsets, mixed real sites, atomic rejection, geometry Undo,
and Qt display/picking/one-Undo checks. Oldest roadmap session block moved whole to archive.
Validation: GCC 13.3/C++20 and Qt 6.4.2 on Linux; full local desktop CTest passes
**89/89 groups**, including Qt offscreen UI, frozen references and repository guards.
Native Linux/Windows CI and owner appearance remain separate; no owner/fidelity gate closes.

---

## 2026-10-06 — Conflict bands follow driving lanes with rail offsets (D117)

Owner refined D115 display to separate directional lane bands, with appropriate offset
from road edges. Exact intersection polygons and mouth classification are retained for
measurement/grouping; `conflictAreaGeometry` also exposes per-side measured station spans.
Canvas bands follow the painted rails through those spans, inset laterally by 0.30 m per
side capped at 20% of local width. Longitudinal cuts remain unchanged, keeping short
mouths visible. Picking uses the same visible band union. Merge bands use measured mouth
spans rather than the unchanged one-metre runtime admission extents. No schema or solver
rule changes. Updated oblique UI tests require two distinct directional bands and blank
offset margins to be unpickable; narrow 0.5 m lanes test adaptive offsets. Model regressions
compare merge band spans directly with measured Connector/Link mouth stations.
The oldest docs-tidy history block is moved whole into archive to retain live-file headroom.
Validation: GCC 13.3/C++20 and Qt 6.4.2; the complete local desktop CTest suite passes
**89/89 groups**, including Qt offscreen UI, frozen references and repository guards.
Native CI and owner desktop appearance remain separate; no owner/fidelity gate closes.

---

## 2026-10-06 — Road crossbars and hover-traced Routes (D116)

The owner requested a simpler common appearance: Routes, Vehicle inputs and signal
heads are lines across the road. A shared normal/rail-intersection helper places
these bars on actual road edges, including attached Link rails at Connector mouths.
Cosmetic pens keep the line readable at zoom; a wider hit area keeps it selectable.
Inputs mark contiguous served lanes and skip zero-share lanes. Edit heads are neutral
with a contrasting casing; Run heads reuse their stop geometry and show program colours.
Reset restores the Edit bars. Selected Routes tint their clipped road surfaces and
mark both ends, preserving D96's partial-Link rendering.

The Route tool now accepts click start, hover through the chosen branch, click destination.
Hover remembers a valid lane-connected chain without changing the document; revisiting
an earlier road trims it. Ambiguous/unreachable extensions are rejected. Backspace,
Escape, focus/tool/level changes and the existing Ctrl+right/Enter flow remain supported.
The destination creates one command, so one Undo removes the whole Route. English/Thai
hints and input labels describe the new interaction. Core behaviour and schemas are unchanged.

Validation: Linux Debug build with GCC 13.3 / Qt 6.4.2; offscreen regression coverage for
curves, both driving sides, Connector mouths, zoom/picking, zero-share inputs, branched
tracing, cancellation, one-command Undo and Edit/Run signal geometry. All 89 tests passed
across the full run and five affected-test reruns after fixing adjacent-lane hit spill;
architecture, file-size, documentation and whitespace checks passed. Native Windows CI
and owner appearance review remain separate; no simulation or owner gate is closed.

## 2026-10-06 — Exact conflict polygons and classified mouths (D115)

Owner authorized the geometry/type/display slice after inspecting why measured
Connector/Link mouth overlaps were excluded from crossings. Quad clipping now retains
convex polygon pieces beside both station intervals. Classification distinguishes crossing,
merge, branching and one-stream continuation without discarding measured mouth geometry.
Automatic crossing keys and topology-derived merge priorities/extents remain stable;
branching is derived/read-only and continuation adds no control. Two arriving Connectors
with only an edge contact can display their measured common receiving-lane mouths.

Canvas fill, hit testing and connected-place grouping use the retained polygon union;
the former full side strips and 0.3 m display/pick inset are superseded. Each lane pair
still has its own reservation and paired priority layers. Branching rows are translated
in English/Thai and reject priority authoring in both UI and commands, including a
disguised/stale suggestion. Polygons are recomputed from network geometry, never added
to the file schema. Geometry edits and Undo restore their derived shapes.
New/Open/Recover also invalidate the conflict cache across document revision resets.

Validation: GCC 13.3/C++20 local desktop build with Qt 6.4.2 and strict floating point;
all model tests pass **486 cases, 0 failures**. New analytical oblique-area, source/target
mouth, continuation, branching refusal, codec and edit/Undo regressions accompany Qt
offscreen canvas/picking/grouping tests. Qt/model edge comparisons allow only numerical
area tolerance; explicit outside-overlap probes must remain unpickable.
The complete local desktop CTest suite passes **88/88 groups**, including documentation,
architecture, file-size and frozen-reference checks.
Native Linux/Windows CI and owner appearance review remain separate gates. This is
M3.2.4d; full physical-mouth merge admission/clearance is M3.2.4e, not implemented here.
No cross-path swept-body, fidelity or owner/milestone closure claim is made.

---

## 2026-10-06 — Edited Connector lanes drive motion and controls (D114)

Owner reported that reshaping a Connector leaves cars on the endpoint-only cubic,
authorized the geometry-based correction, then explicitly requested the lane centres
and other systems use shared data. `connectorPaths` now supplies final adjacent-rail
midpoints with named lane-centre terminals and no endpoint-only runtime equation.
Lengths, front positions, rolling rear-axle guides, heads, route overlays and right-of-way
station mappings use those paths. The cubic remains the default drawing generator.
Zero points/Reset straight now drive straight; widths, ranges and interior edits can
change travel lengths and results. IDs, file schemas and core motion code remain unchanged.

`conflictSurface` is the shared source for overlap calculation, outlines and waiting bars,
using those same final painted rails and authored cross-sections. Physical setbacks map
to the runtime lane distance. Common mouth pieces remain topology joins even when P1–P4
cuts lie longitudinally before/after the Link attachment; a finite Link end can truncate
the painted terminal leg. A separate interior crossing of the attached lane remains a
crossing. Existing History following and Run invalidation handle edits/Undo/Redo.
The T-junction example is regenerated by its builder: ten control/extent numbers change;
network geometry and frozen reference fixtures are unchanged. Old authored conflict areas
are validated against current coverage on load, not silently repaired.

New fixed-end edit regressions cover both driving sides, both taper directions,
independently interpolated rail centres, compiled lengths, heads, waiting bars/setbacks,
file round trips and Undo/Redo. Updated canvas tests check edited lane fronts/body heading
and replacement of the same canvas's Run network. D107 motion-independence and D109
coverage-independence expectations are superseded, with their historical evidence retained.
Validation: GCC 13.3/C++20 Release, strict floating point, Linux headless `check` passes
**60/60 CTest groups**, architecture/file-size/documentation guards and unchanged frozen
references. Existing lane-centre sweeps cover 240 seeded curve/taper runs with replay,
accounting and segment-body separation. T-junction seeds 42–81 × headways 3/7/12 pass
**120/120** replay/accounting/body/swept-conflict-interval/braking checks;
[full rows](evidence/connector-edited-centres.jsonl) retain clamp counts and trajectory digests.
Reproduce with `trafficsim-t-junction-clamps --sweep out.jsonl REPO_ROOT`.
Local builds disable precompiled headers and use a writable TMPDIR; CMake/Ninja and
nlohmann/json are local build dependencies, not committed repository changes. Qt is
unavailable locally; native desktop CI and owner appearance remain separate. These checks
are interval/segment checks, not cross-path 2D collision proof. No owner/M0/M3/M6 gate or
fidelity claim is closed.

---

## 2026-10-05 — Current contracts separated from obsolete reference text

Owner requested removal of unnecessary/stale documentation content. The old migration
and M1.21 reference snapshots and the complete historical M0 backlog/question register
are retained in archive; current references no longer act as a second live queue.
NETWORK_EDITOR now states codec reads 1–19 and writes 17/18/19 by feature, matching
document.cpp. Unsupported topology is distinguished from supported merge/crossing
controls; owner gates link to their maintained status rather than repeating stale claims.
SIMULATION's summary, Connector equation/section geometry, downstream routing and
scheduled-demand semantics are reconciled with the existing contracts and code.
Route-only Link reversal is documented with its actual positional-reference blockers;
lane removal, markings and accent-colour guidance are corrected from commands/tests/style.
Dated audit findings remain dated, and all decision/gate/source-spec/evidence/fixture
records are preserved. No runtime code, gate result or task priority changes.
Validation: documentation links/anchors/index coverage, retained archive blocks and
regression evidence, source-spec/data/fixture hashes, file-size guard and diff whitespace.
Native CI runs separately; no local CTest claim (CMake/CTest are unavailable).

---

## 2026-10-05 — Repository housekeeping

Owner asked to clean the project files. Removed the committed local Qt installer log
from the root and ignored future installer output, temporary editor files and desktop
metadata. JSONL run output remains ignored by default, but `docs/evidence/*.jsonl`
is explicitly allowed so a new measurement record is not silently omitted from Git.
Existing evidence and frozen fixtures are retained. Decision-ID navigation is wrapped
for source readability without changing any link or decision row.
Validation: documentation guard, file-size guard, retained source-spec hashes,
Git ignore checks for generated output versus evidence and `git diff --check`.

---

## 2026-10-05 — Documentation folders and indexed decision record

Second part of the owner's authorized documentation plan. Current references/contracts,
milestone plans/gates and dated audits now have separate indexed folders. The complete
decision rows move once into decisions/RECORD with stable D-number anchors; PROGRESS's
Decisions heading remains a compatibility pointer. Historical backlog/questions remain
marked as dated context, not another live queue. Relative links and source-comment doc
pointers follow the moves; supplied spec parts, evidence data and frozen tests stay intact.
ROADMAP keeps its consolidated dated review, respecting the prior owner's instruction.
NEXT's Demand priority, owner checks and work ordering are unchanged.
Validation: Markdown links/anchors and index coverage, three retained source-spec hashes,
decision/session retention, unchanged fixtures/data, file-size guard and diff whitespace.
No local CTest claim; CMake/CTest are unavailable. Native CI and owner observations remain
separate, and no milestone is closed by this maintenance.
The Qt-free C++ documentation guard is included in `check`/CTest on Linux and Windows.
Its positive/negative fixtures cover broken paths, stale/duplicate/explicit anchors,
encoded fragments, fenced examples and missing index entries; GCC compilation and
direct guard/self-test execution pass locally.

---

## 2026-10-05 — Shared AI instructions and documentation authority

Owner authorized the documentation organization plan. AGENTS.md is the shared entry
point; CLAUDE.md delegates to it. Standing rules and model invariants are retained,
while detailed status is read from NEXT/ROADMAP. The documentation map now selects
context by task and the decision index locates existing D-numbers without copying
their reasoning. README's codec range is corrected to schemas 1–19, matching
`documentFromJson`; feature-dependent legacy save versions remain unchanged.
The owner's current task explicitly takes priority over the standing session queue.
No product scope, gate, owner review or engine behaviour is changed.
Three pre-existing archive links are repaired. The source-spec parts stay byte-identical:
Network Editor's existing D38 edit is now explicit in README/manifest, retaining the
original hash plus a retained-copy hash and its authorizing commit.
Validation: local Markdown links/anchors, source-spec hashes, file-size guard and
`git diff --check`. No local CTest claim: CMake/CTest are unavailable in this workspace.

---

## 2026-10-05 — Time-varying compositions and type-conditioned routing (D113)

Owner asked to continue after #107; all five of its native CI jobs passed. Slice 6
adds schema-19 composition periods and complete per-type routing matrices. Input
volume remains authoritative. Gaps use base composition; missing type rules inherit
default flows; zero counted totals use that type's whole-period weights. Splitting
by type precedes conditioned routing, including downstream decisions, at scheduled
demand time. Source queueing never reselects the destination. Legacy ordering, IDs,
file bytes and random stream are preserved when no new rules are present.
[DEMAND_TIME_TYPES.md](reference/DEMAND_TIME_TYPES.md) records interfaces and gates. Staged UI
edits commit through History; reference checks include periods and type rules.
Failure-first tests exposed missing serialization/validation. Breakpoint conservation,
queue retention, replay and UI verification accompany this change. No new core RNG,
dynamics, exact-count mode or reporting is added. Linux GCC / Qt 6.4.2 Debug:
the required `check` target passes all 86 checks, including 56 Demand cases and
the new time/type UI suite. Frozen fixtures remain unchanged. Native CI and owner
review remain separate. Two older dated entries moved whole for D82 headroom.

---

## 2026-10-05 — Project vehicle/composition catalogs (D112)

Owner asked to continue Demand improvements after PR #106. Its five native CI jobs
passed. Slice 5 captures types, behaviors and compositions into one undoable project
edit. Schema 18 adds owned compositions and authoring names; absence retains external
catalog behavior and schema-17 fixture bytes on save. All owned compositions, including unused ones, validate IDs, weights
and type references. Existing core checks validate dimensions, axles, speeds and
behavior references. The staging UI offers add/edit/delete, stable IDs, names and
engine/axle parameters. Cancel keeps ownership and History unchanged; referenced
entries cannot be deleted. Input selection, Run and Preview share the ownership resolver.
[DEMAND_CATALOGS.md](reference/DEMAND_CATALOGS.md) records the interface and gates. No dynamics
or report model is added. Regression tests first exposed missing serialization and
validation; portable compilation, rollback, ownership Undo/Redo and Qt staging are tested.
Linux GCC/Qt 6.4.2 Debug: all 85 desktop checks pass, including 44 Demand cases and
the new catalog UI suite. Frozen reference files remain unchanged; this slice's
native CI and owner appearance are separate.

---

## 2026-10-05 — Demand correctness, interval preservation and preview (D111)

Owner requested a detailed Demand-only review and authorized implementation.
[DEMAND_IMPROVEMENT.md](plans/DEMAND_IMPROVEMENT.md) records the contract and six slices;
this branch implements 1–4, with catalog and time/type extensions still planned.
Zero lane weights now exclude a lane; invalid weights cannot silently fall back.
Positive stale weights retain D32's fallback with an advisory. Decision/input dialogs
preserve irregular intervals and offer explicit period editing. Missing active count
cells are rejected. Preview compiles Run's snapshot and shows expected demand,
including composition and lane expansion. Scheduled-time routing remains unchanged.
New failure-first lane cases, conservation and Qt preservation/Undo tests accompany
these changes. Frozen fixtures remain untouched. Linux verification and Windows
native/owner review status are recorded with the branch; no M2 gate is reclosed.

---

## 2026-10-05 — Continuous lane-change guidance (phase 3)

Owner asked to continue after phase 2 (#104). Replaced the independent D102 lateral
slide/yaw with a composed front guide and the same rear rolling solution. Accepted
changes capture their source/target stations and pre-change speed in snapshot-owned,
runtime-only `laneChangeTrace`; following, lane admission and evaluation never read it.
Station remaps reconstruct longitudinal travel. Quintic blends compose with the ongoing
guide, so overlapping changes start from its position/tangent. Stopped vehicles hold
pose; body heading continues settling after the front reaches the lane. Blend length
is `3 * max(startSpeed, 5)` m, an uncalibrated display assumption replacing a time window.

Guide polyline sampling splits at route vertices, sections and blend endpoints, then
uses arc length in the existing RK4 rear model with the source's initial rolling heading.
Polyline tangents are walked once rather than repeatedly searched for every sample.
Actual Canvas cache keys include complete trace/type and immutable Scenario/Run geometry;
seeking rebuilds older journeys and arrivals discard paths. No revision cache. Numerical
sample bounds and missing/disconnected/inconsistent-input behaviour are documented in
VEHICLE_POSE.md, including acceptance rows P3.1–P3.6. Engine occupancy still changes in
one tick; this does not add swept-body conflict clearance or a between-lanes state.

Independent analytic-guide RK4/no-slip, two rigid vehicle sizes, differing station remaps,
overlapping changes, curved join, convergence, invalid input and query-order tests pass.
Crossing reports/events (seeds 0/42/43/4294967295) and four example-project reports,
lane-change diagnostics and CSVs (seed 42) match phase-2 `de9aee3` byte-for-byte.
Linux GCC 13.3 / Qt 6.4.2 Debug: 58/58 headless and 83/83 desktop suites pass;
final guide/Canvas reruns follow the tangent-walk repair. Architecture and size guards
pass. Windows CI results belong to the PR. Owner appearance review stays open. Two old display-scale entries moved whole to the archive.

---

## 2026-10-05 — Rear-axle display reference (phase 2)

Owner authorized the next turning slice after phase 1 merged (#103). The route still
prescribes the traffic front bumper; its scalar station/length contract is unchanged.
`RearAxlePath` solves the rear no-slip equation using the bumper-to-rear lever, with
rigid axle/body offsets. Canvas items now originate at the rear axle, with the nose
kept at its station, including low zoom. This is a bumper-guided approximation, not
front-wheel tracking, swept-body collision clearance or measured Vissim fidelity.

Optional complete type `axles` data is parsed, validated and retained on save. Missing
old-file data stays omitted and resolves 60/20/20 percent proportions. The shipped car
and rigid heavy dimensions are explicit modelling assumptions. Catalog/inline type
extension is additive; network schema remains 17. No engine motion equations changed.

Heading is solved on fixed spatial steps, split at joins/vertices and read without
vehicle history. One derived track per route/type is retained against immutable Scenario
ownership and Run-network replacement, never a document revision (D28). Memory is
bounded; disconnected/missing paths draw none. Existing lane-change slide/yaw remains
an overlay outside the no-slip equation. The full contract is in VEHICLE_POSE.md.

Original type catalog bytes are preserved beside the old sweep evidence. Its guard
permits only added display axles, rejecting changed traffic fields; no fixture/result
is regenerated. Linux GCC 13.3 / Qt 6.4.2 Debug: all 82 desktop suites pass across the
full run and four repair reruns; architecture/size guards pass. A chord substitution
fails the analytic-turn regression. Crossing logs/reports for four seeds and seed-42
reports/CSVs for four projects are byte-identical to main `5d823be`. The Run-view
benchmark completes 3000 M2.6 Steps in both versions; concurrent build/test load means
no timing claim. Headless and Windows CI results belong to the PR.
Owner appearance and subsequent engine slices remain in NEXT.

---

## 2026-10-05 — Continuous vehicle headings across route segments

Owner authorized phase 1 of the vehicle-position audit: repair heading discontinuities
before introducing axle kinematics. The old Run view used a front tangent when local
station was below vehicle length, then switched to a chord inside the segment. On the
90-degree test curve that switched by 4.28 degrees for a 4.5 m car and 11.52 degrees for
a 12 m vehicle; exiting onto the straight target also switched to its tangent.

`vehicle_pose.*` now samples the complete ordered route at front distance and one type
length upstream, using the existing Link geometry and direct Connector equation. The
initial tangent extends behind route entry; coincident samples use a finite front
tangent fallback. Missing geometry has no drawable pose. Canvas shares per-frame route
parts with its lane-change slide; it keeps the true type length for heading at low zoom.
The new Qt-free target has headless geometry regressions, and the Qt suite checks actual
scene-item poses at both joins and the former length thresholds for both shipped types.
Both the extracted legacy formula and the original Canvas fail the new join regression.

This remains a display chord approximation. The upstream arc sample is not the actual
rear bumper or axle, and no wheelbase, overhang, steering or articulated trailer model
is introduced. Core state/events, runtime equations, schema and frozen fixtures are
unchanged. Linux GCC 13.3 / Qt 6.4.2 Debug: headless 56/56 and desktop 81/81 pass,
including architecture and file-size guards. Against main `3403b72`, crossing event logs
and reports match byte-for-byte for seeds 0, 42, 43 and 4294967295; seed-42 reports/CSVs
also match for four-leg-signalised, t-junction-priority, lane-change-lab and m2.6-study-template.
Windows CI is pending; owner appearance and axle-model work remain open in NEXT.

---

## 2026-10-05 — Shared mouth classification and separate lane-pair paint (D110)

Owner reported Connector/Link endpoint conflicts and clarified that grouping shares priority,
not filled outlines. Reproduced: a one-lane internal join is an automatic merge, but Add
crossing creates a duplicate crossing there. `crossingOverlaps` now excludes actual rooted
mouths for automatic suggestions, Add, geometry following and Run coverage; raw geometric
measurements stay available. Neighbour-lane crossings and separate later intersections stay
eligible. Old explicit mouth crossings report CONFLICT_NO_OVERLAP until removed/recreated
as the actual merge. Canvas draws each lane pair's two original strips/insets; all share group
selection and priority edits, including one Undo. Nine pairs have eighteen painted sides.
Linux headless/desktop checks and regressions are recorded in the follow-up PR. No core,
schema, topology or frozen fixture changes; Windows and owner appearance checks remain open.

---

## Backlog (M0, in order)

The [historical checklist](archive/PROGRESS-M0-backlog-and-questions.md#backlog-m0-in-order)
is archived. Current work lives in [NEXT](NEXT.md); milestone gates live in [ROADMAP](ROADMAP.md).

## Open questions

The [historical register](archive/PROGRESS-M0-backlog-and-questions.md#open-questions)
is archived. Use [NEXT — Owner decisions](NEXT.md#2--owner-decisions) for current questions.

## Decisions

The complete [decision record](decisions/RECORD.md) now lives beside its
[topic index](decisions/README.md). This heading remains as a compatibility target
for existing `PROGRESS.md#decisions` links. Add new decisions to RECORD, not here.
