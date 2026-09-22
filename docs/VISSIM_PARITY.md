# VISSIM PARITY — how the network editor differs from the surface it is modelled on

The point of D1 is a tool that behaves like the modelling surface its audience already knows.
This file measures how far the native editor is from that, **feature by feature and gesture by
gesture**, so the gap is a list of decisions rather than a feeling.

**Historical review:** sections 1–6 describe the 2026-09-14 pre-completion state after
M1.1–M1.5. Their "today" columns and line references are retained as the original gap
assessment, not current implementation claims. Use the status below and ROADMAP for
current work.

**M1 completion update (2026-09-14):** typed demand/control editing, in-editor Run with
revision snapshots, locked recovery, controlled-link splits, the Network Objects
sidebar, Ctrl+right creation, connector lane ranges/corner handles, duplication,
Delete/Ctrl+Delete, Tab overlap cycling, levels and data-driven display types now
exist. Ctrl+B toggles the image and Ctrl+Shift+O toggles tables; Shift extends selection.
[NETWORK_EDITOR.md](NETWORK_EDITOR.md) documents the exact controls and limitations.
Group drag/rotation, additional network object types and editable table cells were
not added to M1. Owner usability and engine-validation gates remain open.

**Owner correction (2026-09-15, M1.12):** Ctrl+click adds selection; Ctrl+drag already
selected objects duplicates them. This supersedes the historical Ctrl+click claim below.
Lane handles now work on both sides without recentering existing lanes. Outer road
markings and internal dividers replace lane-centre dashes. Independent Connector and
Signal head copies require valid attachments; see NETWORK_EDITOR for exact behavior.

**How to read it.** A gap is not automatically work. `PROBLEM.md` owns scope and `ROADMAP.md`
owns sequence; this file only tells the truth about the distance and proposes where each item
belongs. The gaps the owner accepted are carved into **M1.8, M1.9 and M1.10** in
[`ROADMAP.md`](ROADMAP.md) — anything else here is recorded, not booked.

This file describes authoring. It makes no claim about simulation fidelity; the
not-yet-validated marker (D5, hard rule 4) stands over everything below.

---

## 1. Working posture — what the hands do

This is the part that decides whether an engineer feels at home in the first ten minutes, and
it is where the editor diverges most.

**Source note.** The Vissim column below is the owner's reference material (2026-09-14),
not the PTV manual. Rows it states explicitly are marked ✔; rows carried over from the first
draft of this review and *not* confirmed by it are marked **?** and must be checked against
Vissim itself before M1.9 designs against them. An earlier revision of this file asserted
plain right-drag for creation and Ctrl+right-drag for panning — **that was backwards**, and it
invalidated the conclusion drawn from it. Do not restore it.

| | Vissim | Today | Gap |
|---|---|---|---|
| Choosing what you are about to create | Click a type in the **Network Objects** sidebar; the sidebar is the mode, and it stays visible ✔ | A `QComboBox` of six tools: `select, draw, split, measure, calibrate, connect` (`src/editor/canvas.hpp:10`, wired at `src/shell/editor_window.cpp:57-59`) | The current mode is a collapsed dropdown showing one line of text. Vissim's is a permanent list — you can see every object type you *could* be placing |
| Creating a link | **`Ctrl` + right-drag** from start to end on empty space, then a **Link Data** dialog for lane count and widths ✔ | Pick **Draw link**, set lane count and width in the inspector, click each centreline point, Enter or double-click to finish (`docs/NETWORK_EDITOR.md` §Draw) | Ours is a polyline of clicks, Vissim's is one drag plus a dialog. Defensible: tracing an aerial image wants per-point placement. But the **creation chord** should still be `Ctrl`+right-drag so the reflex transfers |
| Adding curve points to a link | **`Ctrl` + right-click** on the link ✔ | Double-click the centreline (`docs/NETWORK_EDITOR.md` §Draw) | Divergent, and it collides with the Vissim reflex below |
| Rotating a link | **`Alt` + left-drag** on the selection ✔ | Not possible at all | Absent. Cheap to add once group transforms exist; blocked by the same connector-reanchoring problem as group drag |
| Creating a connector | **`Ctrl` + right-drag** from the source link to the target link, then a **Connector** dialog choosing the lane range at each end; **left-click during the drag adds spline points** ✔ | Two clicks: an orange circle at a source lane end, then a teal circle at a target lane start (`src/editor/canvas_connectors.cpp`), or two combo boxes in **Properties → Connectors** | The real cost: a four-lane-to-four-lane movement is **one** `Ctrl`+right-drag in Vissim and **four** two-click pairs here. Vissim also shapes the curve in the same gesture; we require a separate editing pass. Owner's third priority |
| Adjusting lane count across a connector | **Corner drag points** on the connector when source and target lane counts differ ✔ | Not possible — a connector is one lane to one lane (`src/model/network/network.hpp`) | Structural, not cosmetic: our connector model has no lane *range*. M1.9 has to widen the model, not just the gesture |
| Panning | Plain right-drag is **not** spent on object creation — creation is `Ctrl`+right-drag ✔ | Right-button **or** middle-button drag (`src/editor/canvas_input.cpp`) | **No conflict.** Our right-drag pan can stay exactly as it is and `Ctrl`+right-drag creation can be added on top. This removes the only reason the two-click connector flow existed |
| Zooming | Wheel ✔ (implied) | Wheel, around the pointer | Matches |
| Opening an object's properties | Double-click the object **?** — unconfirmed by the reference, which shows dialogs opening on *creation* | Double-click **inserts a geometry point**; properties are a separate always-open inspector dock | Verify before acting. Our always-visible inspector may be the better design regardless |
| Adding to a selection | Not stated. The reference gives **`Ctrl` + left-click = duplicate the selection** ✔ | `Ctrl`+click or Shift+click adds/removes one object; rubber band on empty space (`src/editor/canvas_select.cpp`) | **Direct semantic collision on the same chord**: the gesture that extends a selection here *copies an object* there. A Vissim user reaching for `Ctrl`+click expects a duplicate. This is more dangerous than a missing feature |
| Cycling overlapping objects | **`Tab`** at the click position ✔ | Nothing — `hit()` returns the nearest object and there is no way to reach the one behind it (`src/editor/canvas.hpp:64`) | Absent, and genuinely needed: links and connectors overlap constantly at a junction. `hit()` already ranks by distance, so the ordered list this needs mostly exists |
| Deleting the selection | `Delete` **?** — not in the reference | `Delete` on the canvas removes a **geometry vertex** (`src/editor/canvas_input.cpp:135-137`); deleting objects is a toolbar button with a confirmation dialog | Divergent from near-universal convention regardless of Vissim. Worth fixing with M1.9 |
| Moving several objects | Drag the selection **?** | Not possible — "There is no group drag" (`docs/NETWORK_EDITOR.md` §M1.5) | Known and documented. Reanchoring every attached connector is the reason; it is real work, not an oversight |

**Summary, corrected.** Vissim's creation verb is **`Ctrl` + right-drag**, uniformly, for every
network object. That single fact changes the M1.9 design in two ways:

1. **Right-drag panning is not the obstacle.** The first draft of this review claimed the
   two-click connector flow was forced by giving right-drag to panning. It was not — the chord
   Vissim uses is still free here. Adopting it costs nothing we currently have.
2. **The blocker is the model, not the mouse.** One Vissim gesture creates a connector across a
   *lane range*; `Connector { from, to }` holds one lane pair. The gesture cannot be adopted
   honestly until the connector model carries a range, so M1.9 is a model change with a gesture
   on top — not a UI-only milestone. Sizing it as UI-only would be wrong.

The `Ctrl`+left-click collision is the one item here that can destroy work rather than merely
annoy, and it should be settled before any other gesture change.

---

## 2. Keyboard

Vissim users work with one hand on the keyboard. The current set is thin — this is the honest
inventory, not a curated one.

### What the editor binds today

| Action | Today | Source |
|---|---|---|
| New / Open / Save / Save As | `Ctrl+N` / `Ctrl+O` / `Ctrl+S` / `Ctrl+Shift+S` | `src/shell/editor_window.cpp:44-51` |
| Undo / Redo | Qt platform defaults — Redo is `Ctrl+Shift+Z` on Linux, `Ctrl+Y` on Windows | `src/shell/editor_window.cpp:53-54` |
| Fit network | `F` | `src/shell/editor_window.cpp:61` |
| Properties dock | `Ctrl+I` | `src/shell/editor_inspector.cpp:76` |
| Objects and problems dock | `Ctrl+B` | `src/shell/editor_tables.cpp:59` |
| Cancel current gesture | `Esc` | `src/editor/canvas_input.cpp:135` |
| Finish the link being drawn | `Enter` | `src/editor/canvas_input.cpp:136` |
| Remove the selected geometry point | `Delete` | `src/editor/canvas_input.cpp:137` |

The **simulation window bound no shortcut at all** — Run, Step and Reset were buttons only.
**Superseded by M1.24:** that window is removed; the editor's Run/Step carry F5 and F6/Space
(`src/shell/editor_run.cpp`), so this gap is closed rather than outstanding.

### Collisions with Vissim

Not "missing" — **bound to something else**. These are the rows that will actively mislead a
Vissim user, listed before the gaps because a wrong action is worse than an absent one.

| Chord | Vissim ✔ | Here | Severity |
|---|---|---|---|
| `Ctrl+B` | Show/hide the **background image** | Toggle the Objects and problems dock | **High** — the editor *has* a background image (M1.2), so both meanings are live and plausible in the same window |
| `Ctrl` + left-click | **Duplicate** the selection | Add/remove one object from the selection | **High** — see §1; the same chord, two incompatible verbs |
| `Ctrl+N` | Toggle simple network display | New project | Medium — `Ctrl+N` = New is near-universal outside Vissim. A deliberate choice to make, not an automatic change |
| `Ctrl+A` | Toggle wireframe / normal link display | Unbound | Low — free to take |
| `Esc` | **Stop the simulation** | Cancel the current drawing gesture | Deferred — harmless today, becomes live the moment M1.8 puts Run in this window |
| Redo | `Ctrl+Y` | Platform default (`Ctrl+Shift+Z` on Linux) | Low — Windows already matches; Linux does not |

### Absent, and worth taking

| Vissim ✔ | Purpose | Where it belongs |
|---|---|---|
| `F5` / `F6` / `Space` / `Esc` / `+` / `-` | Run continuously · single step · next step · stop · faster · slower | **M1.8** — adopt this set wholesale rather than inventing one; note `Esc` above |
| `Tab` | Cycle objects overlapping the click point | M1.9 |
| `Ctrl+C` / `Ctrl+V` | Copy / paste network objects | Not booked — needs an ID-allocation policy for pasted objects |
| `Ctrl+Q` | Quick mode (draw less, simulate faster) | M1.8, if the vehicle layer needs it |
| Per-object-type keys | Select the active network object type | M1.9 — today **no shortcut selects a tool at all**, so the most repeated action in a drawing session has no keyboard path |

`Ctrl+D` (3D), `Ctrl+U` (time format), `Ctrl+T` and the 3D navigation keys (`K` `I` `J` `L`
`Q` `A`) are out of scope: there is no 3D mode and no wall-clock display to toggle.

---

## 2b. Creation gestures for the objects we do not have yet

Recorded now because M1.5.1 is the **next** milestone and it authors two of these. Adopting the
gesture at the same time as the model is far cheaper than retrofitting it.

| Object | Vissim gesture ✔ | Status here |
|---|---|---|
| **Vehicle routes (static)** | `Ctrl`+right-click on the link/connector at the routing decision, then left-click the destination section | Typed since **M1.5.1**; the gesture is **M1.25**; **M1.26** makes the route name Links and Connectors, so it covers every lane of the carriageway the way Vissim's does, and narrowing a Connector no longer invalidates it. Still absent: the decision as a positioned object, relative flows, per-interval volumes and a lane-specific route — all M2.1 |
| **Nodes** | Right-click-drag a polygon over the junction, double-click the first point to close | Absent entirely. Nodes are how Vissim aggregates delay and queue per junction — the output a traffic impact study needs. Belongs with **M5 evaluation**, not the editor |
| **Signal controllers** | `Signal Control > Signal Controllers` table, right-click → **Add…**, then **Edit signal groups** | We have `NetworkSignalHead { programId }` and a flat `SignalProgram` — no controller, no signal groups, no head-to-group mapping. **M4** |
| **Parking lots** | **Car Park Creator** generates bays and their connectors from a drawn area | Absent. Not booked (§4) |

The pattern worth extracting: in Vissim **one chord creates everything**, and the object type
comes from the sidebar, not from the gesture. That is the actual argument for the sidebar in
M1.9 — it is not decoration, it is what makes a single creation chord unambiguous.

---

## 3. Window layout

| | Vissim | Today |
|---|---|---|
| Network object palette | Permanent left sidebar, one row per object type, the active row is the edit mode | None. A toolbar dropdown (§1) |
| Properties | Modal dialog on double-click, per object type | A dockable, always-available inspector with **Links / Connectors / Image** tabs; the tab follows the selection (`src/shell/editor_inspector.cpp`, `editor_window.cpp:78-81`) — **better than Vissim for tracing work**, keep it |
| Object lists | Dockable list windows per object type, editable in place | **Objects and problems** dock, four tabs: Links, Connectors, Signal heads, Problems; two-way selection with the canvas; **read-only** (`src/shell/editor_tables.cpp`, `docs/NETWORK_EDITOR.md` §M1.5) |
| Quick View | A small panel showing the selected object's key attributes | None; the inspector covers most of the need |
| Level selector | Levels order overlapping geometry (flyovers, underpasses) | None — no `level` anywhere in the model |
| Display types | Named, swappable draw styles per object | None — draw styles are fixed in `src/render/` and `src/editor/canvas.cpp` |
| Simulation controls | **In the network editor window** | A separate window entirely (§5) |

The tables being read-only is a deliberate M1.5 limit, not a defect — but in Vissim the list
window *is* a legitimate way to edit, and a user who tries will find nothing happens.

---

## 4. Network objects: what exists

The entire authoring model is five types (`src/model/network/network.hpp`):

```
Link { id, geometry, lanes[] }        Lane { id, width }
Connector { id, from, to, geometry }  NetworkSignalHead { id, lane, position, programId }
Network { id, drivingSide, links[], connectors[], signalHeads[] }
```

Commands over them (`src/commands/*.hpp`): add/delete link, change geometry, change lane
widths, delete many as one transaction, change driving side, opposite carriageway, split (with
or without a downstream lane), add/delete connector, change connector geometry or endpoints,
reset curve, reanchor.

**Present in Vissim, absent here:** conflict areas · priority rules · stop signs · reduced
speed areas · desired speed decisions · vehicle routes as first-class objects · parking lots ·
pedestrian areas and links · public transport stops and lines · data collection points ·
queue counters · travel time sections · nodes · levels · display types · link behaviour types.

Two of these are already booked and should not be re-litigated here:

- **Vehicle routes and inputs** exist only as untyped JSON inside the project document — they
  have no table and no commands. `ROADMAP.md` M1.5.1 owns this and says so plainly.
- **Conflict areas** are the whole reason D1 exists (`PROBLEM.md` §2). They belong with
  right-of-way in the engine, not with the editor; authoring them before the core can honour
  them would be a fidelity claim the code cannot support.

The rest are genuinely out of scope until the milestone that needs them. A network object with
no engine behind it is a drawing, and drawing it invites a user to believe it is modelled.

---

## 5. Running, and what comes out — the widest gap

**In Vissim the simulation runs in the network editor.** You draw, you press play, and vehicles
move over the network you just drew, in the same window, at a speed you control. That single
property is most of why the tool feels like one tool.

Today:

- The editor "does not simulate edited projects yet" (`docs/NETWORK_EDITOR.md:3`).
- The simulation window was a **separate** `MainWindow` that loaded M0 scenario JSON.
  **Superseded by M1.24:** it is removed and the editor opens both file kinds; `loadScenario`
  (`src/project/load.cpp`) survives for `trafficsim-cli`.
- Until this session, that window could not even *open* a project file: it read
  `definition` unguarded, and an editor project legitimately saves `"definition": null`, so it
  failed with a raw `json.exception.type_error.304`. It now names the file kind and offers to
  open the drawing in the editor instead (§7).

So the editor's Run story is not "rough" — it is **absent**, and the bug that prompted this
review is a direct symptom of the split. A user who draws a network and looks for the play
button finds a different window that rejects their file.

**What an in-editor Run actually requires**, in dependency order:

1. **A demand authoring model** — routes, vehicle inputs, signal programs as typed objects with
   undoable commands. This is M1.5.1 and nothing runs without it.
2. **Catalog resolution before Run** — vehicle types and driver behaviours live in `data/`, not
   in the project, so a project must resolve them at run time and say what is missing. M1.7.
3. **Revision → run snapshot** — compile one explicit document revision, so a run is always
   attributable to a revision and edits during a run cannot change it underneath. M1.7.
4. **A vehicle layer over the editor canvas** plus Run/Pause/Step/Reset and a speed control.
   This is the only genuinely new surface, and it is M1.8.

Steps 1–3 already exist on the roadmap; only step 4 needed booking. Hence M1.8 is the *surface*
and M1.7 stays the *plumbing* — see `ROADMAP.md`.

**Final output.** Vissim ends in evaluation: node results, movement delay, queue lengths, LOS.
Here `src/eval/` produces a completed-trip mean delay, shown on the editor's run status since
M1.24 (the simulation window that used to show it is gone). The
per-movement delay and LOS tables that a traffic impact study actually needs are M2 onwards and
are not attempted. **Whatever is built, the not-yet-validated marker stays** until M6 (D5).

---

## 6. Ranked gaps

Ranked by how much each one costs a Vissim user per hour of drawing, against the work it takes.
Re-ranked 2026-09-14 against the owner's gesture/hotkey reference: two items moved **up** because
they mislead rather than merely lack, and item 3 grew because it is a model change, not a gesture.

| # | Gap | Cost to the user | Booked as |
|---|---|---|---|
| 1 | No Run in the editor | Breaks the core loop; sends them to a window that rejects their file | **M1.8** (needs M1.5.1 + M1.7) |
| 2 | `Ctrl`+left-click extends the selection; in Vissim it **duplicates** | The one collision that can destroy work rather than annoy — settle it before any other gesture change | **M1.9**, first |
| 3 | Connector creation is per-lane-pair, and the model has no lane range | A four-lane movement costs four gestures instead of one `Ctrl`+right-drag — and cannot be fixed in the UI alone | **M1.9** (model + gesture) |
| 4 | No Network Objects sidebar | Every mode change is a dropdown trip; and without it a single creation chord has no way to say *what* it creates | **M1.9** |
| 5 | `Ctrl+B` toggles the Objects dock; in Vissim it toggles the **background image** | Both meanings are live in this window — the editor has a background image | **M1.9** |
| 6 | `Delete` deletes a vertex, not the selection | Wrong-thing-deleted; contradicts near-universal convention | **M1.9** |
| 7 | No tool shortcuts at all; no `Tab` to cycle overlapping objects | The most repeated action has no keyboard path; objects behind others are unreachable | **M1.9** |
| 8 | No levels, no display types | Overlapping geometry cannot be ordered or styled | **M1.10** |
| 9 | Object tables are read-only | A Vissim user will try to type in them | Not booked — M1.5 limit, revisit with M1.5.1 |
| 10 | No group drag, no `Alt`-drag rotate, no copy/paste | Real, documented, and expensive (connector reanchoring; ID allocation for pasted objects) | Not booked |
| 11 | Missing object types (nodes, priority rules, stop signs, reduced speed areas, parking lots, signal groups, …) | Large, but each one needs engine behaviour first | Not booked — see §4 and §2b |

Items 9–11 are recorded deliberately without a milestone. Booking work the engine cannot yet
honour is how a roadmap stops being true (`ROADMAP.md` rule 2).

**Nodes are the one omission worth re-reading later.** They are absent here and unbooked, but in
Vissim they are how delay and queue are aggregated per junction — which is the output a traffic
impact study is actually paid for (`PROBLEM.md`). They belong to M5 evaluation, not the editor,
but M5 should not rediscover them from scratch.

---

## 7. Two file kinds — the trap this review started from

Worth stating on its own, because it is a UX defect rather than a missing feature, and it is
fixed as of this session.

The project produces **two** JSON kinds that used to be indistinguishable to the user:

| | M0 scenario | Editor project |
|---|---|---|
| Typical name | `data/scenarios/crossing.json` | `network.traffic.json` |
| Root keys | `network`, `definition` | `format`, `schemaVersion`, `nextId`, `revision`, `network`, `definition`, `background` |
| `definition` | Required, complete | **Null** for any network drawn from scratch — that is correct, not corrupt |
| Opened by | The network editor, or `trafficsim-cli` (M1.24; formerly the simulation window) | The network editor |
| Runs? | Yes | No — it has no demand yet (§5) |

Both matched the same `*.json` filter, and the editor's default save name is
`network.traffic.json`. Picking a project in the simulation window therefore produced a parser
exception about a null, which described the JSON accurately and the user's situation not at all.

Now: `loadScenario` classifies the file before reading any field and fails with a named,
translated code (`SCENARIO_IS_PROJECT`, `SCENARIO_NO_DEFINITION`, `SCENARIO_NO_NETWORK`,
`SCENARIO_NOT_JSON_OBJECT`, `SCENARIO_FILE_READ`), and the dialogs default to
`*.traffic.json` for projects. The **Open in Network Editor** hand-off the simulation window
offered is moot since M1.24: the editor opens both kinds itself.
See `docs/NETWORK_EDITOR.md` §"Two file kinds".

The two kinds were **not** merged into one schema, deliberately. A single format would make
every drawing look runnable, which is the fidelity claim hard rule 4 exists to prevent. They
converge when M1.8 gives a project a real Run — not before, and by adding demand to projects,
not by blurring the formats.


## 2026-09-15 follow-up — Reported Network Editor failures

The earlier M1.9 endpoint-only behavior did not satisfy body-to-body authoring. The
Ctrl+right-drag preview in Select mode also had no Link commit branch, and release used
stale move state. M1.11 addresses these with release-position commits, body attachment
fractions, source/target/middle range handles and a Link lane-count handle.

Review also covered cancellation, hidden-level picking, inspector range bounds, shape-handle
z-order, duplicate connections at distinct positions, schema migration, duplication,
reanchoring, splits and runtime diagnostics. A split through an attachment is rejected;
other attachments are remapped onto the proper child Link. The current whole-lane runtime
cannot honor body attachments and explicitly blocks Run (M1.11.1). First-lane selection
remains in Properties/the creation dialog. The middle handle changes both ranges together;
there is no independent arbitrary Connector lane topology. Group transforms, editable
tables and additional object types retain their earlier status. Owner acceptance remains open.

> Earlier 2026-09-16 follow-ups are in
> [`archive/VISSIM_PARITY-2026-09-16.md`](archive/VISSIM_PARITY-2026-09-16.md).

## 2026-09-16 fifth follow-up — Intermediate points, and what the Connector dialog still lacks

**A Connector is a polyline through a settable number of intermediate points.** We stored a
13-point sample of a cubic, so every sample was a grip, dragging one put a corner in a shape the
author had no count over, and the dialog had no field for it.

The owner settled what the line actually is by sending a Vissim connector with `Intermediate
points` set to **2**: four dots, three straight legs, a mitered corner on each dot, a visible step
at each mouth where the polygon overlaps the link, and no tangency to the links at all. It is the
same rule a Link is drawn by. A first pass here read it as a spline and drew a smooth curve
through the points; that was wrong and is reverted. What survives is the model — a Connector
stores its two attachments and its intermediate points, nothing baked — and the field. The owner
set the default at 3.

Changing the count does not re-derive the default curve. Raising it splits the longest leg, so no
point the author placed is lost and the drawn line does not move; lowering it spaces the points
evenly along the shape that is there. `Reset curve` remains the one thing that goes back to the
arc, and laying more points along that arc follows the turn more closely: 2.29 m of sag at one
point, 0.60 m at three, under 0.10 m at fifteen.

**The default curve could leave its own junction.** The arc reach that shapes a new Connector is
`(2/3)·chord·tan(α/2)/sin(α)`, which is 0.67 of the chord at a right angle and 11.05 at 160
degrees. Drawn where two links nearly touch — the owner's picture — the curve ran to 11.0 times
its own chord. It is held at the 120-degree value, `(4/3)·chord`; every ordinary turn, U-turn
included, is unchanged to the last bit, and the hairpin now measures 1.9. It is still an
undrivable turn for a 3.5 m lane and still says so, as `TIGHT_CONNECTOR_RADIUS`.

**Reviewed against Vissim's Connector dialog, and still missing.** Booked here so the next
session does not have to rediscover them; none is in this slice.

| Vissim field | Ours | Verdict |
|---|---|---|
| `No.` | A generated id string, not an editable integer | Cosmetic, but ids are what a project file is read by. Not booked |
| `Name` | Present, on a Connector and on every other object | Done — M1.15, below |
| `Intermediate points` | Present, as of this entry | Done |
| `Link length` | Shown beside the lane counts, measured on the road | Done |
| `Link behavior type` | Not modelled anywhere | Already out of scope (§ "not modelled") |
| `Display type` | In the shared appearance row | Done |
| `from link / to link`, `At:` | Lane combos plus a metres position each | Done |
| `Lanes` tab — per-lane `Width` | **Closed (M1.12.1).** Authorable per lane path, schema 6; empty still means "derive from the links", and `connectorLaneWidths` is the one place a width is decided |
| `Lanes` tab — per-lane `MarkingType` | **Closed (M1.12.1)**, with one difference worth knowing: ours is indexed per **interior divider**, not per lane, because per-lane does not map unambiguously onto `paths + 1` boundary lines. The two outer edges are always solid. **This mapping was not checked against Vissim** — a chosen representation, not a measured parity claim (rule 4) |
| `Lanes` tab — `BlockedVeh`, `NoLnCh`, `Has overtaking lane` | Absent; lane-change behaviour is not modelled | Blocked on the lane-changing model (Q2), not on the dialog |
| `Reverse parking` | Absent | Parking is not modelled at all (§4) |

Group drag, `Alt`-drag rotate and copy/paste stay declined for the reason already on file.

---

## 2026-09-16 sixth follow-up — what else is not Vissim, audited against the code

The owner asked what else still differs. §§1–6 above are a 2026-09-14 snapshot and several of
their "Today" cells have since gone stale, so this pass was verified against live code rather
than against the table. Five gaps, ranked by gain ÷ (risk × effort):

| # | Gap | Evidence | Verdict |
|---|---|---|---|
| 1 | Nothing could be named | No `name` member on `Link`, `Connector` or `NetworkSignalHead`; no field in `editor_inspector.cpp` | **Done: M1.15** |
| 2 | Object lists are read-only | `editor_tables.cpp` sets `NoEditTriggers`; four (now five) fixed columns per tab | Vissim's Lists are its power-user surface — typed cells, sorting, multi-select-and-set. Not booked |
| 3 | No group move, no `Alt`-drag rotate | `canvas_input.cpp`: "Geometry editing stays strictly single-object" (`Ctrl`+drag duplicates, but a multi-selection cannot be moved) | Group move **done: M1.16** — the reanchoring that once blocked it now exists. `Alt`-drag rotate is not booked |
| 4 | `No.` is a string, not an integer | `allocateId(d,"link")` yields `link-1` | **Advised against for now:** it churns the file format and every reference for a mostly cosmetic win, and M1.15 buys most of the same benefit |
| 5 | Missing object types | 9 tools in `canvas.hpp` against Vissim's Network Objects palette; nodes, priority rules, conflict areas, reduced-speed areas, stop signs, parking | **Deliberately not booked:** each needs engine behaviour first (ROADMAP rule 2). Nodes are the one that matters for the deliverable, and belong to M5 |

The evidence here is code-level and visual, not timed: all five are things that are *absent*,
not things that are slow.

---

## 2026-09-21 — the Connector, audited end to end

The owner asked whether the Connector matches Vissim in every respect. The full audit is
[`CONNECTOR_PARITY_AUDIT.md`](CONNECTOR_PARITY_AUDIT.md); it is the place to look, and it names
its own limits. Two points from it belong in this file:

**The 2026-09-18 entry below is stale on one sentence.** It says the Connector end is "a plain
square end. That is now what is drawn." What is drawn is the **M1.18 longitudinal slide onto the
Link's cross-section**, with a square end kept only as the fallback for an arrival more than
about 75° off its spine — reported as `WARN_CONNECTOR_ALIGNMENT`. The entry's reasoning about why
the wedge was withdrawn still holds; only the description of what replaced it is out of date.

**There are two benchmarks, and this file only has one of them.** Everything recorded here as a
Vissim gap comes from the owner's screenshots and dialog references. The other benchmark — the
owner's supplied Thai specification — is a *target*, and `SPEC_AUDIT.md` and `specs/README.md`
both say so. The audit keeps them apart deliberately; do not let a section number from the
specification be read here as a measured Vissim behaviour.

---

## 2026-09-18 — snapping, audited against the owner's list of Vissim's four snaps

The owner listed what Vissim snaps to and asked for all of it. Audited against live code, only one
of the four is a snap we are missing; two are **interactions we do not have at all**, and one is a
file format we do not read. Recording the distinction because "adjust the snap" and "add pointer
placement for objects that are typed into a dialog today" are not the same size of work.

| Vissim | ours, in code | verdict |
|---|---|---|
| **Snap to Links/Connectors** — dragging heads, stop signs, PT stops, vehicle inputs onto a lane | **routes and vehicle inputs are placed by pointer since M1.25** (`canvas_demand.cpp`: since M1.26 the click resolves to the Link or Connector it lands on, haloed before the click). Signal heads still take `nearestLane()` and a dialog. Stop signs and PT stops are not object types | Partly closed. The head is the remaining pointer-placement gap; the missing object types are §6 item 5, still blocked on engine behaviour |
| **Snap to Points** — endpoints and intermediate points when connecting | endpoints yes; a station another Connector already attaches at, yes (added the same day); **intermediate points, now added** | **Done.** This was the one real snap gap |
| **Snap to CAD (DWG/DXF)** | `BackgroundImage` holds a base64 **PNG** and nothing vector | Needs an import path, a DXF/DWG reader, vector storage, rendering and vertex hit-testing. A milestone, not a session. **Not booked** — no done-condition written |
| **Snap to Vehicle Routes** — decision points with a snap radius | **M1.25/M1.26** draw routes on the canvas and resolve a click to the Link or Connector under it; a routing decision is still not a separate object with its own position along the link | Booked and half-closed. What remains is the decision point as an object, which is demand-model work (M2.1), not a snap radius |

**The owner's opening statement — that Vissim does not snap to a Link end — is not the one we
implemented, and the measurement is why.** The owner's own *Snap to Points* item lists the End
point, and in our model an attachment at a Link end is a distinct stored state (`station` absent)
that compiles to a departure rather than a cut. Measured on a 50 m Link, a Connector drawn short of
the end by:

| short by | station stored | result |
|---|---|---|
| 0.00 m (snapped) | *(absent)* | one section — the end attachment the author drew |
| 0.03 m | 49.9700 | **`unsectionable`, cannot Run** |
| 0.15 m | 49.8500 | **`unsectionable`, cannot Run** |
| 0.25 m | 49.7500 | runs, but as a body attachment with a 0.25 m stub section |

So "place it carefully by hand instead" reproduces exactly the 9 mm / 5.8 cm failure measured on
the owner's own four-leg drawing. The endpoint snap stays, and the reason is recorded here rather
than left as a silent disagreement with the instruction.

---

## 2026-09-18 — the wedge is withdrawn on the owner's instruction

The owner asked for the original Connector geometry: the end meets the Link and nothing more — no
turn towards the Link's direction, a plain square end. *(Superseded: see the 2026-09-21 entry
above. The end is the M1.18 longitudinal slide, with the square end kept as the steep-arrival
fallback.)* So the two entries
below record why the wedge was adopted and bounded, not what the editor does today. The
parity gap they close is reopened deliberately: our mouth stands 4.7 cm to 0.88 m clear of the road
where Vissim's lies on it, and that is the owner's call. Everything about the *body* in those
entries still holds.

---

## 2026-09-17 second follow-up — and the wedge has a limit the re-miter was hiding

The owner circled a mouth on our own render that narrowed to a **point** where it met the Link, and
sent a Vissim connector of the same kind for comparison: parallel-sided, constant width, stopping at
the attachment.

The wedge above is not what drew it and is unchanged. The cause was the **re-miter** that stands in
where the fixed-distance cut would fold: at a strongly oblique arrival it extends each boundary to a
cross-section line lying near the ribbon's own axis, and the intersection lands many lane widths
out — 5.59, 9.41, 14.31 and 18.11 m of mouth on a 7.00 m Connector. Past about 9 m the outer
boundaries cross, and the ring trim that fills the surface closed the fold into the point.

The mouth now takes the first of three shapes that does not fold: the fixed-distance cut, then a
re-miter bounded by the mouth's own span, then the un-cut end square to the Connector. **The square
end is back, but only as the last resort past roughly 50° off the cross-section**, where no corner
placement on that line avoids a fold. Every Link-end attachment and every ordinary merge still takes
the wedge, bit for bit — the entry above stands.

This does not reopen the square-cut question it settled. It bounds where the wedge is a valid cut at
all, which is a different statement, and the 0.12–0.29 m step the square end leaves is the step the
owner's own Vissim screenshot of this joint shows.

## 2026-09-17 — The mouth is a wedge, and the square cut was a misreading

The owner circled the joint on a Vissim screenshot: a Connector arriving on a Link **body** at an
angle, its mouth cut on the Link's cross-section. Ours was square to the Connector, from
`e6dd394`. Restored to the cut, which is what `e81a591` — the commit immediately before it — had
already judged Vissim-correct: *"only the joint, where the Connector arrives across the lane and
is cut on that lane's cross-section, is shorter through the corner, as it is in Vissim."*

`e6dd394` re-quoted the **interpolation's** numbers as if they were the end cut's, and traded a
Vissim-correct wedge for a non-Vissim overlap of 0.12-0.29 m. Measured after the restoration:
every mouth lands on its Link's lane edges to 1e-9, the mouth width along the cross-section is
exactly the Link's lane width at 30/60/90/120 degrees, and every interior boundary vertex is
unchanged bit for bit.

This is the second time in two days that a screenshot of the real thing overturned a reading of
Vissim taken from our own geometry — the first was the spline. The lesson is on the record:
**when a shape is meant to match Vissim, ask for a picture of Vissim before reasoning about it.**
