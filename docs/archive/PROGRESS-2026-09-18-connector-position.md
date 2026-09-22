# Archived progress — 2026-09-18 Connector position

Moved whole from PROGRESS.md on 2026-09-22 to keep that file below 500 lines.

## 2026-09-18 — A Connector keeps its own position, and goes when it has nothing to connect (M1.20)

**The owner's requirement, in the same session as M1.19:** a Connector must store its position
itself rather than having both ends recomputed from its Links on every edit; it must be possible to
move it off a Link; and a Connector with no Link left to connect must disappear. Their answers to
the three questions that follow from that: **snap whenever the end is still on a Link**, **either
end coming off = delete**, and the mouth stays the flush, constant-width cut M1.19 just built.

**What was there.** `reanchorConnector` wrote `geometry.front()` and `geometry.back()` from the
lane references on every Link edit. A Connector could not be moved off a Link at all, because the
next edit put it back, and a Link edit reached into a shape the author had tuned by hand and moved
one end of it.

**What replaced it — two functions with two different inputs, which is the whole of the change.**

- `anchorConnectorEnds` is the old behaviour, kept for the edits whose input IS the reference:
  creating a Connector, moving an end onto another lane, and the edits that **re-lay** a Link's
  lanes without moving the road — a lane added or removed, a width changed, the driving side
  flipped. Nothing moved out from under anything there, so every Connector follows the lane it
  names. Deleting a Connector because the author added a lane to the Link beside it would be a
  surprise, not a rule.
- `reanchorConnector` is the new one, for the edits that **move** a road. Each end still on its
  Link's carriageway is snapped onto the middle of the lane under it and its station moved to
  match; an end that has come off is left exactly where it is, and the function returns false.
  `reanchorConnectors` then deletes that Connector with the routes and heads that named it, in the
  same transaction as the edit that moved it — so one Undo brings both back.

**Three things it has to get exactly right, each found by a test going red.**

- **Nothing moved under an end means nothing changes, to the last bit.** Without that guard, every
  Link edit anywhere re-derived every station through a polyline round trip and walked them an ulp
  at a time. A station is an author's number; an unrelated edit may not rewrite it.
- **A lane bundle edit slides the lanes, not the road.** An end standing still is then on its
  neighbour, so the search is over the Link's lanes, named lane first — never over other Links,
  which would be a topology change nobody asked for.
- **The end and the start of a Link keep meaning "the end" and "the start"**, read on the lane with
  a micron of slack, because measuring a polyline's own length back off it is not exact and
  `attachedAtLinkEnd` — and the M0 whole-lane runtime behind it — turns on that distinction.

**In the editor.** A Connector's body can be dragged like a Link's; its end handles still re-attach
it to another lane. `changeConnectorGeometry` accepts an end that has moved, because "keeps its own
position" is only true if the author can put that position anywhere — including off the Link, where
the Connector is deleted. It still refuses to name a different lane that way: that is
`changeConnectorEndpoints`, which guards route topology.

**What this costs, stated plainly.** Moving a Link now deletes the Connectors whose ends it leaves
behind — the `crossing.json` fixture loses both of its Connectors when `west` is moved 10 m across
3.5 m lanes. That is the owner's rule, not a side effect, and it is one Undo away. Shortening a Link
past an attachment deletes rather than clamps, for the same reason: clamping moved a Connector to
somewhere the author had not put it.

**Tests: six rewritten to the new contract, none loosened.** The `anchored` helpers in four files
asserted a Connector's ends sat on their lanes' ENDS; they now assert the ends sit on the lane
middle **at the station named**, which is the invariant that survives. `shortening_a_link_clamps_...`
became `shortening_a_link_past_an_attachment_deletes_the_connector_that_hung_off_it`;
`reanchor_preserves_points_and_lane_references` became
`a_link_edit_leaves_every_point_where_the_author_put_it`, with both halves in it — a Link stretched
under an end (the Connector holds, station 81 m) and a Link moved out from under one (deleted). The
group-move test keeps the junction-moves-rigidly half unchanged and states the other half the new
way, and the reshape test now separates an invalid shape (still refused, rolled back) from an end
moved off the Link (accepted, deletes).

**Verification:** 24/24 CTest on Qt 6.4.2 under `xvfb`, file sizes green. `trafficsim-cli 42`
unchanged at `meanDelay 29.249359418430977`. *Linux only — the editor has not been driven by hand,
so the drag gestures are asserted at the command layer, not through the canvas.*

### Next

**Drive the two changes by hand in the desktop editor.** Everything here and in M1.19 is measured at
the model and command layer; nobody has yet dragged a Connector off a Link with a mouse, or looked
at a mouth on screen. Do that first, then the owner's timed exercise in `docs/M1_ACCEPTANCE.md`.
Watch in particular for: a Connector deleted by a Link drag the author did not expect to touch it
(the Undo is there, but the surprise is the thing to judge), and whether half a lane width is the
right distance for "off the Link" — it is one constant, in `laneContains`.

---
