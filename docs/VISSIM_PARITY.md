# VISSIM PARITY — how the network editor differs from the surface it is modelled on

The point of D1 is a tool that behaves like the modelling surface its audience already knows.
This file measures how far the native editor is from that, **feature by feature and gesture by
gesture**, so the gap is a list of decisions rather than a feeling.

Written against commit state 2026-09-14, after M1.1–M1.5. Every "today" row cites the code.

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

| | Vissim | Today | Gap |
|---|---|---|---|
| Choosing what you are about to create | Click a type in the **Network Objects** sidebar; the sidebar is the mode, and it stays visible | A `QComboBox` of six tools: `select, draw, split, measure, calibrate, connect` (`src/editor/canvas.hpp:10`, wired at `src/shell/editor_window.cpp:57-59`) | The current mode is a collapsed dropdown showing one line of text. Vissim's is a permanent list — you can see every object type you *could* be placing |
| Creating a link | Right-drag on empty space, then a dialog for lanes and width | Pick **Draw link**, set lane count and width in the inspector, click each centreline point, Enter or double-click to finish (`docs/NETWORK_EDITOR.md` §Draw) | Different but defensible — polyline-by-clicks suits tracing an aerial image better than Vissim's drag. Keep |
| Creating a connector | **Right-drag from the source link to the target link**, then one dialog choosing the lane range at each end | Two clicks: an orange circle at a source lane end, then a teal circle at a target lane start (`src/editor/canvas_connectors.cpp`), or two combo boxes in **Properties → Connectors** | The real cost: a four-lane-to-four-lane movement is **one** right-drag in Vissim and **four** two-click pairs here. This is the owner's third priority |
| Panning | Ctrl+right-drag; right-drag alone is object creation | Right-button **or** middle-button drag (`src/editor/canvas_input.cpp`) | Right-drag is spent on pan here, which is exactly why connector creation had to become two clicks. The two decisions are one decision |
| Zooming | Wheel | Wheel, around the pointer | Matches |
| Opening an object's properties | Double-click the object | Double-click **inserts a geometry point**; properties are a separate always-open inspector dock | Divergent, and mildly hostile: the Vissim reflex edits geometry here |
| Adding to a selection | Ctrl+click | Ctrl+click or Shift+click, plus rubber band on empty space (`src/editor/canvas_select.cpp`) | Matches |
| Deleting the selection | Delete key | Delete on the canvas removes a **geometry vertex** (`src/editor/canvas_input.cpp:135-137`); deleting objects is a toolbar button with a confirmation dialog | Divergent, and the most likely source of a wrong-thing-deleted moment. Worth fixing with M1.9 |
| Moving several objects | Drag the selection | Not possible — "There is no group drag" (`docs/NETWORK_EDITOR.md` §M1.5) | Known and documented. Reanchoring every attached connector is the reason; it is real work, not an oversight |

**Summary.** Selection is already Vissim-shaped. Creation and deletion are not, and both trace
back to one root choice: right-drag was given to panning.

---

## 2. Keyboard

Vissim users work with one hand on the keyboard. The current set is thin — this is the honest
inventory, not a curated one.

| Action | Today | Source |
|---|---|---|
| New / Open / Save / Save As | `Ctrl+N` / `Ctrl+O` / `Ctrl+S` / `Ctrl+Shift+S` | `src/shell/editor_window.cpp:44-51` |
| Undo / Redo | Platform defaults | `src/shell/editor_window.cpp:53-54` |
| Fit network | `F` | `src/shell/editor_window.cpp:61` |
| Properties dock | `Ctrl+I` | `src/shell/editor_inspector.cpp:76` |
| Objects and problems dock | `Ctrl+B` | `src/shell/editor_tables.cpp:59` |
| Cancel current gesture | `Esc` | `src/editor/canvas_input.cpp:135` |
| Finish the link being drawn | `Enter` | `src/editor/canvas_input.cpp:136` |
| Remove the selected geometry point | `Delete` | `src/editor/canvas_input.cpp:137` |

**Absent entirely:** any shortcut that selects a tool. Every mode change is a trip to the
dropdown with the mouse — the single most repeated motion in a drawing session, and the one
with no keyboard path at all.

Proposed with M1.9, in the shape Vissim users expect: a digit or letter per network object
type, `Delete` acting on the selected **objects** with vertex removal moved to a modifier,
`Ctrl+Shift+click` for the second endpoint, and `Space` to toggle the last two tools.

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

| # | Gap | Cost to the user | Booked as |
|---|---|---|---|
| 1 | No Run in the editor | Breaks the core loop; sends them to a window that rejects their file | **M1.8** (needs M1.5.1 + M1.7) |
| 2 | No Network Objects sidebar | Every mode change is a dropdown trip; the object vocabulary is invisible | **M1.9** |
| 3 | Connector creation is per-lane-pair | A four-lane movement costs four gestures instead of one | **M1.9** |
| 4 | `Delete` deletes a vertex, not the selection | Wrong-thing-deleted; contradicts the Vissim reflex | **M1.9** |
| 5 | No tool shortcuts at all | The most repeated action has no keyboard path | **M1.9** |
| 6 | No levels, no display types | Overlapping geometry cannot be ordered or styled | **M1.10** |
| 7 | Double-click inserts a point instead of opening properties | Minor; the inspector is always visible | M1.9, if it fits |
| 8 | Object tables are read-only | A Vissim user will try to type in them | Not booked — M1.5 limit, revisit with M1.5.1 |
| 9 | No group drag | Real, documented, and expensive (connector reanchoring) | Not booked |
| 10 | Missing object types (priority rules, stop signs, reduced speed areas, …) | Large, but each one needs engine behaviour first | Not booked — see §4 |

Items 8–10 are recorded deliberately without a milestone. Booking work the engine cannot yet
honour is how a roadmap stops being true (`ROADMAP.md` rule 2).

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
