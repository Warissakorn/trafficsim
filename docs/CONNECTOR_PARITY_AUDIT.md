# Connector parity audit — 2026-09-21

Audited against `58647e9` on 2026-09-21, on the owner's question *"is the Connector like Vissim
in every respect?"*

This is a **reading of the code and the documentation**, not a run. No C++ toolchain was
available in the environment where this was written, so nothing here was built, executed or
measured. Every claim below cites the file and line it was read from, and every claim about
Vissim cites the document it came from. Where the two benchmarks disagree, this file says so
rather than picking one.

---

## 0. How to read this — there are two benchmarks, and they give different answers

| Benchmark | What it is | Verdict |
|---|---|---|
| **The supplied target specification** — [`specs/connector/`](specs/connector/part-01.md) | The owner's Thai requirement document, supplied 2026-09-20 | The **base** (§1–§4, §10–§15) is implemented; the **extension block** (§5–§9, §10.5, §16) is not |
| **Vissim itself** | The modelling surface this project is built to imitate (D1) | **Never measured.** No timed, counted or photographed comparison exists in this repository |

The distinction is not pedantry. [`specs/README.md`](specs/README.md) states the supplied
documents are *"requirements/proposals, not implementation or validation claims"* and that *"a
heading that says 'VISSIM-compatible' does not establish scientific or product parity"*.
[`SPEC_AUDIT.md`](SPEC_AUDIT.md) repeats it: they *"describe a target, not measured Vissim
parity"*.

So throughout this file, **"matches" means "matches the supplied target specification"**. It never
means "matches Vissim", and nothing here should be quoted as if it did. The one place this
repository holds real Vissim evidence is the owner's own screenshots, and the lesson already on
record in [`VISSIM_PARITY.md`](VISSIM_PARITY.md) is to ask for another one before reasoning about
a shape from our own geometry.

---

## 1. Implemented, and matching the supplied specification

Each row was checked in the code, not in the docs. Section numbers are the specification's.

| Spec | Requirement | Where it lives | State |
|---|---|---|---|
| §2 | `Connector` carries id, from, to, geometry, fromLaneCount, toLaneCount, level, displayType, laneBlend, name, laneWidths, laneMarkings | `network.hpp:33-51` | All twelve present, same names |
| §2 | `LaneReference{linkId, laneId, optional station}`, station in metres on the reference polyline | `network.hpp:24-32` | Present |
| §2 | Source without station = Link end; target without station = Link start | `connector_geometry.cpp:17` (`attachedAtLinkEnd`) | Present |
| §4.1 | `pathCount = max(fromLaneCount, toLaneCount)` | `connector_paths.cpp:72` | Present |
| §4.2 | Integer-division pairing | `connector_paths.cpp:76` | Present, formula identical to the spec |
| §4.3 | 1→1, 1→N, N→1, N→N, 2→3, 3→2 all derived, monotone | `connector_paths.cpp:59-92` | Present; not an independent routing matrix, as the spec itself limits |
| §4.4 | Path ids `<id>`, `<id>/lane-2`, … | `connector_paths.cpp:6-7` | Present |
| §3.1 | Stored as a polyline of 2 attachment points + 0–40 intermediate points | `connector_geometry.cpp:178` | Present; 0–40 enforced by `EDIT_CONNECTOR_POINTS` |
| §3.1 | Straight legs between points, mitered, **not** a persisted spline | `network.hpp:168-175`, `geometry.cpp:162-165` | Present, and confirmed against the owner's Vissim screenshot (2026-09-16) |
| §3.3 | Reset curve · Make straight · raise/lower intermediate points · drag interior point · drag endpoint · drag body | `connector_commands.cpp:98-134`, `canvas_input.cpp:77-242` | Present |
| §3.3 | Endpoint within 0.01 m of the reference lane centre | `validate.cpp:106` (`DISCONNECTED_GEOMETRY`) | Present |
| §10.1 | Width precedence authored > Link lane > 0 (taper) | `road_boundaries.cpp:121` | Present; `connectorLaneWidths` is the single decider |
| §10.2 | Mouth slid longitudinally onto the Link's cross-section, then transitioned; Link widths win at the joint | `road_boundaries.cpp:152-216`, `:420-436`; M1.18/M1.19 | Present |
| §10.3 | Outer edges always solid; interior dividers authored; a converging divider stops being drawn | `road_boundaries.cpp:449` | Present |
| §10.4 | `laneWidths` = pathCount values; `laneMarkings` = pathCount−1; empty = derived; range change clears both | `validate.cpp:82-86`, `connector_paths.cpp:54-55` | Present |
| §11.1 | Creation by the Connectors tool (C) and by Ctrl+right-drag | `editor_palette.cpp:16-21`, `canvas_input.cpp:14-29` | Both present |
| §11.2 | Snap order: Link endpoint → existing Connector attachment on that lane → Link intermediate point | `canvas_connectors.cpp:18-65` | Present, in that order |
| §11.2 | An exact endpoint pick stores **no** station | `canvas_connectors.cpp:62` | Present |
| §11.4 | Range handles change one edge, one undo per release, bounded by contiguous lanes and 12 | `canvas_lanes.cpp:80`, `connector_paths.cpp:61` | Present |
| §11.5 | `level` and `displayType` inherited from the source Link; id `connector-N` avoiding authored and derived ids | `connector_commands.cpp:46-51`, `document.cpp:91-107` | Present |
| §12.1 | The ten base commands | `connector_commands.hpp` | All ten present, same names |
| §12.2 | Referenced Connector: reshape yes; retarget no; range change no; delete cascades to heads, routes and inputs in one transaction | `connector_commands.cpp:39-45, 54, 95, 135-141` | Present |
| §14.1 | Compile: derive paths, cut lanes at body attachments, departure = diverge, arrival = merge | `sections.cpp:24-104` | Present |
| §14.2 | Derived section ids are never persisted | `sections.cpp:21`, `authoringSegments` at `:177` | Present |
| §14.3 | Interior arrival derives a priority rule yielding to the upstream target-lane section, gap time and headway from `data/priority-rules/` (3.0 s, 7.0 m) | `sections.cpp:195-220`, `data/priority-rules/default.json` | Present |
| §14.3 | Missing defaults **block Run**, not editing | `compile.cpp:61` (`EDIT_NO_PRIORITY_DEFAULTS`) | Present |
| §14.5 | `kMinSectionLength` 0.2 m; a violation saves but blocks Run as `UNSUPPORTED_CONNECTOR_POSITION` | `network.hpp:236`, `sections.cpp:68-71`, `compile.cpp:44` | Present |
| §15 | Every base diagnostic code | see §3 below | All present |
| §15 | `TIGHT_CONNECTOR_RADIUS` and `WARN_SHORT_CONNECTOR` are advisory, never blocking | `compile.cpp:64-94`, `diagnostics.cpp:102` | Present |

**Two things here are ours, not the specification's**, and are recorded so they are not mistaken
for parity: `MarkingType::doubleLine` (§10.3 names only solid/dashed/none), and
`WARN_CONNECTOR_ALIGNMENT` (`compile.cpp:77`), which reports the square-fallback mouth.

---

## 2. In the supplied specification, absent here

The whole extension block. These are **not silently dropped** — schema 7 rejects the fields on
load with `EDIT_UNSUPPORTED_FIELD` and a field path (`parse.cpp:16-22`, pinned by
`tests/authoring_tests.cpp:89-100`), which is the correct behaviour under hard rule 4: a field
that would change nothing at Run must not be accepted and then written back as if it had.

| Spec | Parameters | Code state |
|---|---|---|
| §3.2 Curve | `curveType`, `splineTension`, `controlReach`, `smoothingAngle`, `minRadius`, `sampleDensity` | Absent; no such member on `Connector` |
| §5 Behavior | `desiredSpeed`, `speedFactor`, `acceleration`, `deceleration`, `maxDeceleration`, `minHeadway`, `reactionTime`, `lookAheadDistance`, `lateralBehavior`, `cooperative`, `yieldToPedestrians` | Absent. `desiredSpeed` exists only as a vehicle-type property (`core/demand.cpp:29`), never per Connector |
| §6 Lane change | `laneChangeDistance`, `emergencyStopDistance`, `cooperativeLaneChange`, `aggressiveLaneChange`, `blockingTimeout` | Absent |
| §7.1 Conflict areas | `autoConflictArea`, `conflictPriority`, `conflictFrontGap`, `conflictRearGap`, `conflictVisibility` | Absent — **there is no ConflictArea object anywhere in the model** |
| §7.2 Priority | `priorityRuleType`, `minGap`, `maxWaitTime`, `stopLinePosition`, `visibilityDistance` | Absent; only `gapTime`/`headway` exist, and only on a derived rule |
| §7.3 | The ladder signal → priority rule → conflict area | Partially realisable; the third rung does not exist |
| §8 Signal | `signalGroupId`, `signalControllerId`, `stopLineOffset`, `signalHeadType`, actuated/adaptive | Absent. Heads do mount on Connector paths (`sections.cpp:221`), which is the §8.3 part that exists |
| §9.1 Routing decisions | `routingDecisionId`, `decisionPosition`, `destinations`, `routeType`, `lookAheadDistance` | Absent. Static routes naming path ids exist, which is §9.3 |
| §10.5 Mouth | `mouthTransitionLength`, `mouthBlendMode`, `mouthSharpness` | Absent; the transition zone is derived (`road_boundaries.cpp:171`, applied at `:190-216`) |
| §16.1 Visual | `color`, `lineStyle`, `lineWidth`, `showFlowLabels`, `showPriorityMarkers`, `showSignalMarkers`, `showLaneNumbers`, `highlightOnHover` | Absent |
| §16.2 | Zoom-level LOD | Absent |
| §16.3 | Double-click opens a dialog; right-click opens a context menu | Absent. Double-click inserts a geometry vertex (`canvas_input.cpp:161`, `:230`); there is no context menu |
| §12.1 | The six new commands | Absent; they depend on the fields above |
| §15 | The eight new diagnostic codes | Absent — verified by grep, none of `EDIT_LANE_CHANGE_DISTANCE`, `EDIT_SPEED_DISTRIBUTION`, `EDIT_PRIORITY_RULE`, `EDIT_SIGNAL_CONFLICT`, `EDIT_ROUTING_DECISION`, `EDIT_CONFLICT_PRIORITY`, `EDIT_CURVE_PARAMETERS`, `WARN_TIGHT_TURN` appears in `src/` or in the locales |
| §18 | `connector_behavior.cpp`, `connector_lane_change.cpp`, `connector_priority.cpp`, `connector_signal.cpp`, `connector_routing.cpp` | **Do not exist.** The source map lists proposals; `SPEC_AUDIT.md:44` already warns against describing them as delivered |

**Against the real Vissim dialog**, the fields the owner's own reference names and we still lack
are: an editable integer `No.` (ours is a generated string id), `Link behavior type`, the `Lanes`
tab's per-lane `BlockedVeh` / `NoLnCh` / `Has overtaking lane`, and `Reverse parking`. The
`Lanes` tab's `Width` and `MarkingType` are done (M1.12.1); `Name`, `Intermediate points`,
`Link length`, `Display type` and the attachment row are done. The outer two edges are
deliberately not authorable (`network.hpp:46-48`).

---

## 3. Confirmed defects and false records found by this audit

These are **not** "features not yet built". Each is either something the repository says about
itself that is no longer true, or a gap in behaviour that no test covers.

### 3.1 The recorded description of the mouth is stale in two files

`docs/PROGRESS.md:272` (entry "2026-09-18 — The Connector mouth is a plain square end again")
states that `connectorBoundaries` stops at `offsetGeometry` and that *"the whole end-cut block is
gone: the fixed-distance cut …, the bounded re-miter …, the fold test …, and the
`legCrossing`/`remiter` helpers"*. That entry's own closing line anticipates the correction —
*"the square end did look wrong; M1.18 above replaced it with a longitudinal slide"* — but the
body text still reads as current.

`docs/VISSIM_PARITY.md:423` says the same thing: *"a plain square end. That is now what is
drawn."*

**What the code actually does** (`road_boundaries.cpp:324-325`, `:402-410`, `:420-436`):

- an ordinary arrival is **slid along its own offset curve** until its end lands on the Link's
  cross-section, then the cross-section transitions back to the Connector's own width over a zone
  (`fitMouth` `:152`, `spendMouth` `:177`, `shearMouth` `:190`, `fitAndShear` `:420`).
  M1.18/M1.19.
- a **full-width square end** is kept only where the forward tangent dot product is below `0.25`
  — roughly 75° or more off the spine, or a reversed arrival — because no bounded slide can fix
  either without squeezing the ribbon or stretching it into a needle (`:324-325`). It is reported
  as `WARN_CONNECTOR_ALIGNMENT` (`compile.cpp:77`).

So the current end is **slide-to-flush, with a square end as the steep-arrival fallback** — not a
plain square end everywhere. Both files are corrected in this session; `PROGRESS.md` by a new
entry, since it is append-only.

### 3.2 Two comments in `connector_commands.hpp` contradict the code they declare

- `connector_commands.hpp:17` — *"Only interior points may change; the two endpoints are attached
  to their lanes."* `changeConnectorGeometry` (`connector_commands.cpp:85-90`) explicitly allows
  the ends to move, and says so in its own comment: *"The ends may move too … What they may NOT do
  is name a different lane this way; that is `changeConnectorEndpoints`."*
- `connector_commands.hpp:35-38` — describes `resampleConnectorPoints` as re-laying the road *"at
  equal spacing along it"*. Raising the count does **not** re-space: it splits the longest leg
  each time so no hand-placed corner is lost (`connector_commands.cpp:110-123`), and the code
  records the measurement that forced it (*"re-laying at even spacing instead cut a hand-placed
  corner by up to 1.00 m"*). Lowering the count does re-space (`:124-132`).

Both are corrected in this session. A header comment is a contract; a wrong one costs the next
session the same rediscovery twice.

### 3.3 Two Connectors arriving at the same station on the same lane do not yield to each other

`derivedPriorityRules` (`sections.cpp:195-220`) always names the **upstream lane section** as the
conflict segment. So each arriving path gives way to the traffic already on the lane, and never to
another arriving path. In Vissim this interaction is a conflict area (§7.1 of the supplied
specification); there is no ConflictArea object here, so the interaction has no counterpart.

The engine's own comment (`simulation.cpp:170-175`) explains why this matters: car-following
*"already works without any of this, because spans are bucketed by GLOBAL segment index, so two
vehicles see each other the moment they share a segment."* Two arrivals reaching the shared
downstream section **in the same step** share it only after moving, so a one-step overlap at the
drawn station is possible.

**Stated carefully:** no test covers this case, and this audit did not run one. It is reported as
an uncovered case, not as a measured overlap. It needs a test before it is called either way, and
the fix belongs with conflict areas in M3.2.

### 3.4 A merge at a lane's *start* is uncontrolled, and unreported

`sections.cpp:203` skips the rule when the joined section starts at 0 (`joined->start <= 0`).
That is deliberate — an arrival at the lane's start is the lane's own upstream boundary, not a
merge — and it is consistent: the merged section has only one predecessor, so `UNSUPPORTED_MERGE`
does not fire either (`core/validate.cpp:102`). The consequence worth recording is that a Connector
arriving at the very start of a lane adds traffic to it with no arbitration at all, and nothing in
the diagnostics says so.

### 3.5 A Connector leaving a lane's start, or arriving at its end, is authorable but not runnable

`attachedAtLinkEnd` (`connector_geometry.cpp:17`) recognises a departure at the lane **end**
and an arrival at the lane **start**. A departure at station 0, or an arrival at the lane's full
length, therefore falls through to the interior cut path and lands within `kMinSectionLength` of a
boundary, which is rejected and recorded in `table.unsectionable` (`sections.cpp:68-71`). The
author can draw it and save it; Run refuses it as `UNSUPPORTED_CONNECTOR_POSITION`.

The diagnostic wording covers it and the drawing is preserved, so this is a **documented limit
rather than a bug** — but it is a trap worth naming, because the drawing looks correct on screen
and the refusal is about a boundary the author did not draw. The snap that prevents the common
form of it is in the editor (`canvas_connectors.cpp:18-65`); this case is the one it cannot catch.

### 3.6 `buildScenario` can produce a zero-gap rule; only the Run path refuses it

`derivedPriorityRules` does not throw when `priorityDefaults` are unset, by design —
`buildScenario` (`compile.cpp:9`) is documented unchecked assembly for diagnostics that must not
throw, and a missing data catalog must not block an edit (D18b). The guard is
`priorityDefaultsIssues`, called from `compileScenario` (`compile.cpp:95`). A caller that
assembles through `buildScenario`
directly therefore gets a rule with `gapTime == 0, headway == 0`, and `validateScenario` accepts
those (`core/validate.cpp:73` passes `zero=true`). No such caller exists today; the hazard is
latent, and it is recorded so it is not created later.

### 3.7 The mouth has been re-decided seven times

`git log` on `road_boundaries.cpp` and `connector_geometry.cpp`:

`30a212a` wedge → `e6dd394` square → `4b116eb` re-miter → `1f2f014` bound the re-miter →
`f7b5039` bound the mouth → `9d8cb04` revert to square → `bcead5d` flush slide (M1.18) →
`cfa4dbc` square fallback for steep arrivals.

This is the highest-regression-risk surface on the object. Two of those reversals were caused by
reading Vissim from our own geometry rather than from a picture of Vissim — the lesson already on
record at `VISSIM_PARITY.md:469`. Any future change here should carry a Vissim screenshot with
it.

---

## 4. What this audit did not do, and why

- **No C++ was changed.** The environment had no `cmake`, `g++`, `cl`, `clang++` or `ninja`, so
  nothing could be built or tested. Hard rule 7 forbids leaving the build unverified, and hard
  rule 4 forbids claiming a fix that was not measured. Everything in §3 that needs code — 3.3,
  3.4, 3.6 — is booked, not fixed.
- **Nothing was closed.** No milestone gate is met by this file. M0/M1 owner acceptance and M6
  validation remain open.
- **Vissim was not measured.** No timing, counting or photography was done. §1's "matches" is
  against the supplied target specification only.

---

## 5. Recommended sequence

1. **The documentation corrections** in §3.1 and §3.2 — done in this session, since they are
   free and they are what the next session reads first.
2. **A test for §3.3**, before any fix. Two Connectors arriving at one station: does the engine
   allow an overlap? The answer decides whether this is M3.2 work or a defect.
3. **Conflict areas** — the largest real gap, and the reason this project exists
   ([`PROBLEM.md`](PROBLEM.md) §2). Until they exist, §3.3 and §3.4 have no home.
4. **The §5–§9 extension block**, behind its engine contracts, in the order
   [`SPEC_AUDIT.md`](SPEC_AUDIT.md) already books: behavior ownership at M2.1, lane changing and
   crossing conflicts at M3.2, signals at M4.1.
5. **Do not start §3.2 curve or §16 visual parameters.** They are decoration on an object whose
   runtime behaviour is not yet settled, and they would be the first fields accepted on load that
   change nothing at Run.

---

## 6. Where the evidence is

| Concern | Source |
|---|---|
| Object, paths, attachments, tangents, default curve | `network.hpp`, `connector_paths.cpp`, `connector_geometry.cpp` |
| Widths, boundaries, mouth, markings | `road_boundaries.cpp`, `markings.cpp` |
| Runtime cutting, merge rules, sections | `sections.cpp`, `compile.cpp`, `src/core/simulation.cpp` |
| Draft validation and codes | `validate.cpp`, `src/core/validate.cpp`, `data/locales/en.json` |
| Commands | `src/commands/connector_commands.{hpp,cpp}` |
| Editor: pick, snap, handles, gestures, inspector, table, tool palette | `src/editor/canvas_connectors.cpp`, `canvas_lanes.cpp`, `canvas_input.cpp`, `src/shell/editor_connectors.cpp`, `editor_tables.cpp`, `editor_palette.cpp` |
| Tests | `tests/connector_tests.cpp`, `connector_shape_tests.cpp`, `connector_point_tests.cpp`, `connector_mouth_tests.cpp`, `connector_ui_tests.cpp`, `attachment_tests.cpp`, `attachment_ui_tests.cpp`, `range_tests.cpp`, `network_lifecycle_tests.cpp` |
| The supplied target | [`specs/connector/part-01.md`](specs/connector/part-01.md), [`part-02.md`](specs/connector/part-02.md) |
| The recorded gap analysis | [`VISSIM_PARITY.md`](VISSIM_PARITY.md), [`SPEC_AUDIT.md`](SPEC_AUDIT.md), [`NETWORK_LIFECYCLE_AUDIT.md`](NETWORK_LIFECYCLE_AUDIT.md) |
