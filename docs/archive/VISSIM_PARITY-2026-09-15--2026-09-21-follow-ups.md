# VISSIM PARITY archive — dated follow-ups, 2026-09-15 to 2026-09-21

Moved whole out of [`../VISSIM_PARITY.md`](../VISSIM_PARITY.md) on 2026-09-29. Each is a dated finding; the
current state is §1a and §2 of the live file, and later work may have closed what these report.

## 2026-09-15 follow-up — Reported Network Editor failures

M1.9's endpoint-only Connectors failed body-to-body authoring, and Ctrl+right-drag in Select had no
Link commit and used stale move state. M1.11 fixed both: release-position commits, body attachment
fractions, source/target/middle range handles, a Link lane-count handle. A split through an
attachment is rejected; others are remapped onto the right child Link. Owner acceptance remains open.

> Earlier 2026-09-16 follow-ups are in
> [`VISSIM_PARITY-2026-09-16.md`](VISSIM_PARITY-2026-09-16.md).

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
[`CONNECTOR_PARITY_AUDIT.md`](../CONNECTOR_PARITY_AUDIT.md); it is the place to look, and it names
its own limits. Two points from it belong in this file:

**The 2026-09-18 entry below is stale on one sentence.** It says the Connector end is "a plain
square end. That is now what is drawn." What is drawn is the **M1.18 longitudinal slide onto the
Link's cross-section**, with a square end kept only as the fallback for an arrival more than
about 75° off its spine — reported as `WARN_CONNECTOR_ALIGNMENT`. The entry's reasoning about why
the wedge was withdrawn still holds; only the description of what replaced it is out of date.

**There are two benchmarks, and this file only has one of them.** Everything recorded here as a
Vissim gap comes from the owner's screenshots and dialog references. The other benchmark — the
owner's supplied Thai specification — is a *target*, and `../SPEC_AUDIT.md` and `../specs/README.md`
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
| **Snap to Links/Connectors** — dragging heads, stop signs, PT stops, vehicle inputs onto a lane | **routes and vehicle inputs are placed by pointer since M1.25** (`canvas_demand.cpp`: since M1.26 the click resolves to the Link or Connector it lands on, haloed before the click). Signal heads are placed by click at the pointer's station on a lane or Connector path and drag along their lane since M2.7a. Stop signs and PT stops are not object types | Partly closed. Heads are placed by pointer; the missing object types are §6 item 5, still blocked on engine behaviour |
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

---

**Archived.** The 2026-09-17 first follow-up (the wedge mouth and the square-cut misreading)
moved to [`VISSIM_PARITY-2026-09-17-wedge-mouth.md`](VISSIM_PARITY-2026-09-17-wedge-mouth.md)
on 2026-09-22 to keep this file inside the 500-line limit. M1.18 and M1.19 superseded it.
