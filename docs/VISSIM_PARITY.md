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

The **simulation window binds no shortcut at all** — Run, Step and Reset are buttons only
(`src/shell/main_window.cpp`; no `setShortcut` anywhere in it).

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
| **Vehicle routes (static)** | `Ctrl`+right-click on the link/connector at the routing decision, then left-click the destination section | No authoring model — untyped JSON under `ProjectDocument::definition`. **M1.5.1** |
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
- The simulation window is a **separate** `MainWindow` that loads M0 scenario JSON
  (`src/shell/main_window.cpp`, `src/project/load.cpp`).
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
Here `src/eval/` produces a completed-trip mean delay and the simulation window shows it. The
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
| Opened by | The simulation window | The network editor |
| Runs? | Yes | No — it has no demand yet (§5) |

Both matched the same `*.json` filter, and the editor's default save name is
`network.traffic.json`. Picking a project in the simulation window therefore produced a parser
exception about a null, which described the JSON accurately and the user's situation not at all.

Now: `loadScenario` classifies the file before reading any field and fails with a named,
translated code (`SCENARIO_IS_PROJECT`, `SCENARIO_NO_DEFINITION`, `SCENARIO_NO_NETWORK`,
`SCENARIO_NOT_JSON_OBJECT`, `SCENARIO_FILE_READ`), the simulation window offers **Open in
Network Editor** for a project, and the dialogs default to `*.traffic.json` for projects.
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

## 2026-09-16 follow-up — Grips, end attachments and lane-count checks

The owner's annotated screenshot marked four things: the lane tabs looked unfinished, the
Link and Connector geometry points sat at a road edge instead of the middle, the Connector
lane counts were not checked against what the other end actually has, and the Connector
end points could not be moved.

Lane tabs are now edge-mounted rounded tabs with a stem and the resulting count inside them,
in place of a loose dot with a floating number. Geometry grips moved to the bundle centreline
(`linkCentreline`, `connectorCentreline`); the stored polylines are unchanged, and a drag maps
back through the same offset. Connector ends are draggable onto any lane or position along it:
the range is centred on the lane under the pointer, and it is narrowed when the new end has
fewer lanes than the Connector carries. Widening a Link still never widens a range on its own,
because how many lanes a movement carries is the author's decision.

Not changed, deliberately: unequal ranges remain legal drawings and still fail the M0 run
check as `UNSUPPORTED_MERGE` — the counts are now shown beside the Connector length so the
author sees a 3 → 2 drop without opening the inspector. Re-attaching a Connector used by a
route or head is still rejected. Group transforms and the remaining booked gaps are unchanged.

## 2026-09-16 follow-up — Bent carriageways, one-lane connectors, and where an attachment lives

Three more owner findings. Two were defects and are fixed; the third is a model decision,
recorded here and booked as M1.13.

**Bends.** Lane edges were offset along the corner's average normal by the full width, which
leaves them `width/2 * cos(theta/2)` from the centreline — the carriageway pinched at every
bend, 30% at the right angle in the owner's screenshot. Offsetting now uses the miter length,
so Links, Connectors and the diagnostic view all keep their width through a corner. No pinned
simulation baseline moved: every reference fixture is a straight network, and straight
polylines are unchanged bit for bit.

**One lane means one lane.** The creation dialog pre-filled both lane counts with every lane
from the picked one to the end of the Link, so a drag from a single lane authored a two- or
three-lane Connector complete with lane dividers. It now opens at one lane per end. The
Properties counts also reset to one when no Connector is selected, so a previous selection
cannot seed the next creation.

**Where an attachment lives.** Vissim attaches a connector to a lane at a position and drags
it with the link; so do we, and that is not negotiable — validation requires the end to sit on
its attachment, and the compiler builds lane → connector path → lane, so a Connector left
behind in world coordinates would be a network the engine cannot run and a vehicle would
teleport across the gap. What is wrong is the *unit*: a fraction of lane length means
stretching a Link slides every interior attachment, and with it the lane-section lengths a
future run would measure. Vissim stores a distance; so does our own `NetworkSignalHead`. M1.13
books that change, with the migration and identity constraints it has to respect.

## 2026-09-16 follow-up — M1.13 implemented: attachments are metres along the Link

The unit decision recorded above is now the model. `LaneReference::station` holds metres along
the Link's reference polyline, Vissim's `Pos`, rather than a fraction of the attached lane's
arclength. Stretching a Link no longer slides the Connectors attached part-way along it, and a
multi-lane range meets a curved Link on one square cross-section instead of fanning with the
per-lane arclength difference.

Two behaviours the owner should know, because Vissim does not spell them out either. Shortening
a Link past an attachment clamps the Connector to the new end rather than refusing the edit — a
Signal head in the same position still refuses, which is the older contract and deliberately
left alone. And the number in Properties is measured on the Link's own line, so on a curve it
differs slightly from the distance travelled in an outer lane; that is what makes it the same
number for every lane of a range.

Remaining gaps are unchanged: group transforms, `Alt`-drag rotation, editable table cells and
the M1.11.1 runtime lane sections, which this change exists to make tractable.

## 2026-09-16 follow-up — Merge markings and the shape of a tight turn

Two more owner findings, both about what a Connector looks like rather than what it stores.

**A divider down the middle of one lane.** Where a Connector's ends carry different lane counts,
the interior boundary was pinned at the narrow end to the *centre* of the single lane the paths
converge into, and drawn dashed for the whole length — a lane line down the middle of where
vehicles drive. Markings are now derived separately from the boundaries: an interior divider
covers only the stretch where the two lanes are at least half their full spacing apart and stops
at the merge. The ribbon itself was already right, tapering 7.0 m to 3.5 m across a two-into-one.

**A U-turn drawn at half the radius it needs.** The default curve reached `chord/3` for every
turn, which is only the correct value as the turn angle tends to zero. On the owner's U-turn that
produced a minimum radius of 2.29 m on a 13 m chord — 0.18 of the chord, tighter than the 3 m
lane it carries, so the ribbon's own inner edge crossed itself, and the even-odd fill punched the
overlap out as the hole visible in the screenshot. The reach is now the circular-arc value for
the actual turn angle: unchanged for gentle turns, 0.45 of the chord for a U-turn (5.90 m here).
Surfaces fill by winding rule, so a self-overlap that remains reads as road.

Vissim leaves an impossible turn to the author; we do the same but say so, with a non-blocking
`TIGHT_CONNECTOR_RADIUS` row. Stored geometry is never rewritten, so existing drawings are
untouched — only newly created curves and Reset curve use the new reach.

## 2026-09-16 second follow-up — A Connector carries lanes, not a ribbon

**Every lane narrowed instead of one tapering.** On a two-into-one the whole ribbon shrank
together: the lane that continues measured 2.62 m half way along and 1.75 m at the mouth, so
vehicles drove a lane that pinched. Vissim keeps the through lane at its own width and drops the
surplus one as a taper. Ours does now: the cross-section is assembled from the lane widths at
each end rather than interpolated between the two mouths, so the continuing lane holds the width
its links give it point for point, and the extra lane closes onto it as a wedge. The divider
between them is a lane edge for its whole length, which is why it now arrives on the *edge* of
the lane the two merge into instead of part way down its middle — the marking trim from the
previous round is no longer what keeps it off the traffic.

Vissim requires a connector's two ends to carry the same number of lanes and leaves the taper to
a separate lane drop. We allow the unequal range and draw the taper ourselves; the lane pairing
follows the ranges in lane order, so re-anchoring a range moves the taper to the other side.

**A reverse curve was read as no turn at all.** The control reach came from the angle between the
two tangents, which is zero for an S even when each end leaves the chord steeply; a measured S
bent to 0.18 of its chord. Reading each end against the chord instead gives 0.22 there and is
identical, to the last bit, for straight runs and symmetric turns.

## 2026-09-16 third follow-up — The cross-section follows the road

Review of the merged change found a regression it had introduced. The cross-section the lane widths
are measured across was interpolated between the two mouths, which says nothing about where the
Connector points in between: on a reverse curve the mouths are parallel, so it never turned while
the path swung 50-60 degrees away, and the lane was drawn its own width times the cosine of that
angle. Measured square to the road, a 3.50 m lane came out 1.06 m at its narrowest on a tight S —
worse than the 2.90 m the pre-change code drew, and a reverse curve is one of the shapes the owner
reported. It now takes the path's own normal, corrected onto each mouth, and measures 3.34 m there;
symmetric shapes (quarter turn, U-turn) are unchanged, and both mouths still meet their links
exactly. The test that was supposed to guard this measured width *along* the cross-section, which is
the lane width by construction at any angle; it now measures square to the road as well.

## 2026-09-16 fourth follow-up — One poly point, and a constant offset

Two answers from the owner, both now the rule here.

**"Vissim moves only the one poly point that is attached to the Link."** `reanchorConnector` used to
carry the whole curve rigidly through a similarity transform of its endpoint chord, so a Link edit
dragged points the author had placed by hand. It now moves the attached endpoint and nothing else.
Path independence, which the transform was written for, comes for free: the point returns to where
the lane puts it and no other point was ever touched.

**"Should the offset from the lane centreline be the same all along?"** Yes, and that is the Vissim
rule: the polygon is the axis offset by half the total width, measured square to the axis at every
point. Links already did this — measured 3.500 m of a 3.500 m lane at every bend from 30 to 170
degrees, because `offsetGeometry` miters each corner. Connectors now go through the same function,
with a per-point offset so a tapering lane keeps its neighbours at full width. Measured after a Link
was rotated 90 degrees under a drawn Connector: the body holds 3.500 m where the interpolated
cross-section drew 0.46 m; only the joint itself is shorter through the corner, which is the notch
Vissim shows there too.
