# Travel-time sections (M5.4)

What a travel-time section is, how a run measures it and what it writes. Decisions:
[D130](../decisions/RECORD.md#d130) (LOS from section delay, owner answer O3 (b)) and
[D133](../decisions/RECORD.md#d133) (this contract). A section's delay is the quantity M5.5 will
give an LOS letter; until then every figure is **simulated section delay, not HCM control delay or
LOS, and not validated** (rule 4). Whole-route movement delay (D39) is unchanged and never gets a
letter.

## 1. The object (schema 23)

```json
"network": { "travelTimeSections": [
  { "id": "section-41", "name": "North left",
    "start": { "linkId": "link-3", "station": 40.0 },
    "end":   { "linkId": "link-9", "station": 25.0 } } ] }
```

- A **section line** is a cross-section of one **Link** at `station` metres along its reference
  polyline (the coordinate waiting lines and Connector attachments use). It spans every lane of
  that Link: a vehicle crosses it on whichever lane it drives. Connectors carry no section line in
  this slice: a turn is measured from its approach Link to its exit Link.
- The key is written only when there is a section, so a file without one keeps its bytes and
  schema. A file of schema 23 carries it; an older file that does is refused
  (`EDIT_UNSUPPORTED_FIELD network.travelTimeSections`). Readers accept schemas 1–24; schema 24 adds a section's
  `controlType` for LOS ([LOS](LOS.md), D134).
- Structural rules (the load or the edit is refused, the document unchanged): a non-blank id unique
  among all network ids (`INVALID_ID`, `DUPLICATE_ID`); each line's Link exists
  (`UNKNOWN_SECTION_LINK`); each station finite and ≥ 0 (`INVALID_POSITION`); on one Link the start
  lies before the end (`INVALID_SECTION_ORDER`). Any other key in a section or line is refused.
- Edits that move a station keep the line on the road: splitting a Link moves a line beyond the cut
  to the downstream child by the same arithmetic as other controls, and a line inside the 0.2 m cut
  span refuses the split (`EDIT_SPLIT_CONTROL`). Reversing a Link a section names is refused
  (`EDIT_REFERENCED_LINK`). Deleting a Link deletes every section with a line on it.

## 2. Binding to a run

At compile time each line becomes, per lane, the runtime segment and metres that
`locateControlPoint` gives for `{linkId, laneId, station}`. A lane where that fails (a station past
its end) contributes nothing. For each runtime route the line's **route distance** is its first
part on any of those segments plus the position, as for queue-counter lines. A section **applies**
to a route when both lines lie on it and the start comes first; on every other route it measures
nothing. Rows follow authored order, one per section, with 0 vehicles when nothing applies.

## 3. Measurement

All positions are a vehicle's front `distance` along its current route; each crossing time is
linear interpolation between the two observed states that bracket it.

| Event | Rule |
|---|---|
| Crossing | Line distance `a` on the route before and `a'` on the route after a step. Crossed when `d < a` before and `d' ≥ a'` after; time `t + Δt·(a − d)/((a − d) + (d' − a'))`. A lane change or route decision between the two states changes the route but not the rule, because each side is measured on its own route |
| Entry | A vehicle first seen enters at distance 0 at its `enteredTime`; a line at `0 ≤ a ≤ d'` is crossed at `enteredTime + (t' − enteredTime)·a/d'` (at `enteredTime` when `d' = 0`) |
| Exit | A vehicle that arrives (leaves the network) in a step is taken to have moved its last speed × Δt, and at least to its route's end, so an end line at the route's end is crossed inside that step; error under one time step |
| Trip | Crossing the start opens a trip on a route the section applies to; crossing the end closes it. A vehicle crossing the start again before the end restarts its trip |
| Travel time | `t_end − t_start` |
| Free-flow time | `(a_end − a_start) / desiredSpeed` on the route the vehicle holds at the end crossing: the section length at the vehicle's own desired speed, the term the whole-trip `freeFlowTime` uses (the engine has no speed limit below it) |
| Delay | `max(0, travel time − free-flow time)`, as `tripDelay` |
| Period | A trip counts when its **end** crossing is in the evaluation period (D132's rule for movements). With a cool-down (M5.9, D146, [BATCH](BATCH.md) §5) it counts when its vehicle was released in the window, whenever the end crossing falls |
| Unfinished | Vehicles still in the network with an open trip when the report is taken; with a cool-down, window vehicles only |

Rows: `name`, `vehicles`, `meanTravelTime`, `meanDelay` (null with no vehicle), `unfinished`.

## 4. Output

- Single run JSON: a `sections` array of the rows, written only when the project has a section.
  CSV: a block `section,vehicles,meanTravelTime_s,meanDelay_s,unfinished,controlType,los` (the last two
  from M5.5) after the approach block,
  only then. A project without sections writes exactly the bytes it wrote before M5.4.
- Batch (BATCH §4): `sections` with `vehicles`, `meanTravelTime`, `meanDelay`, `unfinished` as
  estimates, and the CSV block `section,n,meanDelay_s,ci95_s,sd_s,vehicles_mean,meanTravelTime_s,unfinished_mean,controlType,los`,
  again only with a section. Seeds must carry the same sections, by position and name.
- The marker line stays first. Letters and their own marker line are M5.5's ([LOS](LOS.md) §4).

## 5. Editing (M5.4b)

History commands `putTravelTimeSection` (empty id allocates `section-N`; the same id replaces) and
`deleteTravelTimeSection`, each one Undo step. The editor's Section tool reuses the queue-counter
gesture: Ctrl+right-click on a Link sets the start, a second Ctrl+right-click sets the end and
commits; Escape drops a half-placed section. A *Travel-time sections* tab lists id, name, start and
end, with rename and delete. The canvas draws each line across the Link's full width.

## 6. Acceptance rows

| Row | Fixture | Expected |
|---|---|---|
| TT1 | Lone vehicle, straight road, section 200–300 m | Crossing times equal the interpolation of the bracketing states computed by hand in the test; delay ≥ 0 and under 0.1 s |
| TT2 | Section from route start to route end | Free-flow time equals the trip's `freeFlowTime`; travel time within one Δt of the trip's travel time |
| TT3 | One vehicle held by a red of known length R | Delay at least the hold the red forces, `R − t_start − (line − start)/v`, and at most that plus the start-up loss bound stated in the test |
| TT4 | Period | A trip whose end crossing is before warm-up or after end is not counted; equality sides count |
| TT5 | Lane change | A vehicle that changes lane inside the section is timed once (two-lane fixture) |
| TT6 | Wrong order / other Link | A section no route passes in order reports 0 vehicles; same-Link start ≥ end is refused |
| TT7 | Unfinished | A vehicle stopped inside the section at report time is unfinished, not a row vehicle |
| TT8 | Codec | Schema-23 round trip; a file without sections keeps schema and bytes; schema 22 with the key, unknown keys and bad stations are refused |
| TT9 | Commands | Put/replace/delete with Undo; Link delete cascades; split moves a downstream line; reverse refused |
| TT10 | Outputs | No-section projects' single-run and batch bytes unchanged; batch aggregates a section by hand |
| TT11 | Editor (M5.4b) | Two Ctrl+right-clicks make one Undo step and one row; rename and delete through History; save/reopen |
