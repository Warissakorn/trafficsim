# Parking lots (M8.4–M8.5)

**Contract only, written 2026-10-10 (M8.1, [D150](../decisions/RECORD.md#d150)); nothing below is
implemented.** A parking lot is a zone with a capacity. Trips into its zone end at a **gate** and
park while there is space; trips out of its zone leave only while a vehicle is parked. Both
directions are ordinary OD demand ([ZONES_AND_OD](ZONES_AND_OD.md)): by the owner's choice (D150)
there is no dwell time and no new random draw. The rows close M8.4 (runtime) and M8.5 (outputs).
Every figure is **simulated, not validated** (rule 4); the lot answers whether the car park and its
access queue fit the site ([PROBLEM](../PROBLEM.md) §2), not how long people stay.

## 1. The object (schema 30)

```json
"network": {
  "parkingLots": [{"id": "lot-1", "name": "Site car park", "zoneId": "zone-1",
                   "capacity": 120, "initialOccupancy": 40, "gateServiceTime": 4}]
}
```

- `zoneId` names the lot's zone; a zone has at most one lot. The lot has no geometry of its own:
  its access is its zone's connectors.
- `capacity` is a whole number of vehicles, at least 1, of any type. Whether a motorcycle counts
  less is the open PROBLEM §8 question and is not settled here.
- `initialOccupancy` is a whole number from 0 to `capacity`, the vehicles parked at time 0.
- `gateServiceTime` is the seconds a vehicle stands at the gate before it may park (ticket,
  barrier), 0 or more and **on the time grid**, so service ends on an exact tick.
- **Schema 30**, introduced with the runtime (M8.4) rather than with zones (schema 29), so no file
  can carry a lot that Run would only refuse. The key is written only when non-empty; below
  schema 30 it is refused by a raw-JSON `rejectParkingBefore30` (`EDIT_UNSUPPORTED_FIELD`), and at
  30 any other key in a lot is refused (`knownFields`).

| Code | Refused when (structural, every load and edit) |
|---|---|
| `INVALID_ID` / `DUPLICATE_ID` | as for zones (network-wide id set) |
| `UNKNOWN_ZONE` | `zoneId` names no zone |
| `PARKING_ZONE_TAKEN` | a second lot names the same zone |
| `PARKING_CAPACITY_INVALID` | `capacity` is not a whole number of at least 1 |
| `PARKING_OCCUPANCY_INVALID` | `initialOccupancy` is not a whole number from 0 to `capacity` |
| `PARKING_GATE_INVALID` | `gateServiceTime` is negative, not finite or off the time grid; checked in `validateDocument`, where the time step is known, so changing the time step can be refused by it |

## 2. Which routes and inputs belong to a lot

A lot governs **routes and inputs, never places**, so that other traffic at the same road has a
rule:

- Its **gated routes** are the full chains the OD expansion builds into its zone, those whose
  destination connector belongs to the lot's zone. A stub never ends at a gate; it changes lanes
  onto a gated route first.
- Its **unpark inputs** are the OD inputs on its zone's origin connectors.
- Authored inputs and routes, routeless paths and other zones' trips never park and never draw
  occupancy.

Run refuses (`UNSUPPORTED_PARKING_ACCESS_SHARED`, on the lot's path, through `odRunIssues`) any
layout where a lot's traffic would share an end or a source with traffic it does not govern:
- a destination connector's Link with an outgoing Connector, so through traffic would queue behind
  the gate;
- a destination connector's Link on which any other compiled route ends (authored, routeless, or
  another zone's);
- an origin connector's Link that carries any other input;
- a Link that is a connector of the lot's zone and of another zone.

## 3. Runtime (M8.4)

**Core interface** (no I/O, no wall clock, no random draw):
- A **lot table** compiled into the core `ScenarioDefinition`, appended last as new collections
  are. Each lot has: id, capacity, initial occupancy, gate service ticks, gated route ids and
  unpark input ids. It is sorted by id in `canonicalScenario` and resolved to slots in
  `ScenarioIndex` (D29: slots inside, names in events and files).
- **Lot state** in `SimState`, one entry per lot (asserted parallel to the table): the occupancy,
  and the gate records `{vehicleId, lot, since}`, sorted by vehicle id.

**The gate is a hold at the end of a gated route**, the stop-line kind of hold (an obstacle at the
route end, as a red head is), **not** a dead end (`RouteDeadEnd` stops a vehicle short of a
connection it cannot make; a gated vehicle has reached where it was going):
- On a gated route the end-of-route sink (`distance ≥ routeLength`, SIMULATION Death) never fires.
  A vehicle clamped onto the gate is a safety clamp, not an arrival.
- A vehicle **has reached the gate** when it is below `kStoppedSpeed` with its front within
  `stopLineReach` of the route end, the test a Stop line uses. Its gate record is written on that
  tick (`since`) and kept until it parks; it is refreshed from the post-lane-change snapshot as
  stop service is (`refreshStops`).
- It is **served** once `gateServiceTime` has elapsed since `since` (an exact tick, §1). A served
  vehicle parks as soon as there is space; until then it keeps standing at the gate and the queue
  behind it forms by ordinary following, back along the Link and upstream.

**Order within one tick** (fixed, so a run is reproducible from the contract):
1. **Insertion**, in the existing `(scheduledTime, id)` candidate order ([SIMULATION](SIMULATION.md)
   Birth):
   - For an unpark input, the **lot test comes first**: a candidate whose lot has no parked vehicle
     is skipped *without* using up its source segment's one attempt this tick, so a candidate behind
     it on the same segment is still tried.
   - The test reads a running occupancy, lowered by each unpark inserted earlier in the same order.
   - The insertion guard (gap, D108) follows unchanged. A candidate refused there lowers nothing.
   - An inserted unpark emits `UnparkedEvent` immediately before its `DepartedEvent`.
2. **Admission**, after motion and before publish: served gate vehicles of each lot in
   `(since, vehicle id)` order, across all its gates, park while `occupancy < capacity`, each
   raising the occupancy by one. The occupancy already includes this tick's unparks, so a space
   freed at insertion can be taken in the same tick.
3. **Publish**: an admitted vehicle is removed where it stands. It emits the **unchanged**
   `ArrivedEvent` (travel time from entry, departure delay from release, free-flow time) and then
   `ParkedEvent`, and counts in `completed`. SIMULATION's Death paragraph links here for this one
   exception to "arrives at the route end".

**Events and checkpoints:**
- `ParkedEvent` and `UnparkedEvent`, each `{time, vehicleId, routeId, lotId}`, are appended to the
  `SimEvent` variant, so every existing index is stable. Their JSON kinds are `parked` and
  `unparked`, and SIMULATION's event table gains both rows.
- A checkpoint gains a `lots` array (id, occupancy, gate records) **only when the scenario has a
  lot**, so every existing checkpoint and the four reference fixtures keep their bytes (the W74
  "only when present" precedent).

**Invariants**, checked by test at every tick:
1. `0 ≤ occupancy ≤ capacity`;
2. `generated = completed + active + pending` (BATCH §1), unchanged;
3. per lot, `occupancy = initialOccupancy + parked − unparked`, counting its events so far;
4. every `parked` is in the same tick as an `arrived` of the same vehicle on one of its gated
   routes, and every `unparked` immediately precedes a `departed` on one of its unpark inputs.

**No vehicle is ever deleted.** A full lot holds its arrivals at the gate; an empty lot holds its
departures in their source queue. Neither creates nor removes a vehicle.

**Accounting and limits:**
- **Held at the gate** at the end of the run: active, and unfinished in their movement
  (BATCH §5).
- **Waiting for a parked vehicle**: pending. They count in the overloaded-seed test (BATCH §3),
  because they are demand the network did not serve, and each lot also reports them as
  `waitingForParked`, so a reader can tell a starved exit from a jammed road.
- **Delay includes the lot.** Movement and network delay of a trip into the lot include the gate
  stop, the service and any wait for space. An outbound trip's departure delay includes its wait
  for a parked vehicle. This is a stated bias, beside D39's source waiting; the lot outputs (§4)
  give those waits on their own so the delay can be split.
- **Gridlock through a lot is reported, not prevented**, as for any network (SIMULATION, runtime
  scope). For example, a full lot whose gate queue blocks its own exit path: nothing parks,
  nothing leaves, nothing is deleted.
- **A cool-down cannot empty a lot.** Outbound demand ends with the duration like all demand, so a
  lot full at the duration stays full, and its gate queue stays unfinished.

## 4. Outputs (M8.5)

- **Interval.** Lot time series use an interval length from a data pack,
  `data/evaluation/parking.json` (`intervalSeconds`, with its source note). It is read only when a
  lot exists, so other projects' input manifests and bytes do not change (the [LOS](LOS.md) pack's
  rule).
- **Per lot and interval:**
  - occupancy (mean and maximum over the interval's ticks);
  - entries (`parked`) and exits (`unparked`), by event time;
  - gate queue in vehicles (held at a gate or standing on a gated route behind one).
- **Per lot over the evaluation period:**
  - peak and mean occupancy, sampled on ticks in `[warmup, end]` as queues are;
  - arrivals that waited for space (held beyond `gateServiceTime`), with their mean and maximum
    wait;
  - departures that waited for a parked vehicle, with their mean and maximum wait;
  - `waitingForParked` and gate-held vehicles at the end.
  Waits are selected by the D132/D146 rule: by release time with a cool-down, by event time
  without.
- **Surfaces**, each present **only when a lot exists**, so every other output keeps its bytes,
  and each under the marker:
  - single-run JSON `lots` and a `lot` CSV block, after the section/LOS blocks;
  - the batch, each quantity as an Estimate `{n, mean, sd, halfWidth95}` with a batch CSV block;
  - `--compare`, rows matched by lot name, unmatched lots listed as BATCH §7 lists movements;
  - the editor's Results, a Lots table, with Copy and Export equal to the CLI's bytes.

## 5. Editor (M8.6)

- **Lot tool `G`** (`P` is cycle priority): click inside a zone with no lot to create one.
- **Defaults.** A new lot's capacity, initial occupancy and gate service time come from the data
  pack's defaults, not literals in code.
- **Inspector:** name, capacity, initial occupancy, gate service time.
- **Commands:** `putParkingLot` and `deleteParkingLot`, each one Undo step. A lot names its zone,
  so deleting the zone is refused while the lot exists (ZONES_AND_OD §4).

## 6. Not in this contract

- **Dwell time and trip chains** (a vehicle that parks and later leaves for another zone) are a
  later slice with its own contract (D150).
- **On-street parking** that blocks a lane is the owner's open design question §8 Q6.
- **Bays, parking search and in-lot circulation** are out of M8's scope.

## 7. Acceptance rows

| Row | Check | Test |
|---|---|---|
| PL1 | Schema-30 round trip; a file without lots keeps its schema and bytes; the key below 30 and any unknown lot key refused; every code of §1 one case each, including an off-grid service time and a time-step change it refuses | `parking` |
| PL2 | Every case of `UNSUPPORTED_PARKING_ACCESS_SHARED` on the lot's path; an authored route ending on the lot's destination Link is refused, not parked | `parking` |
| PL3 | Hand-computed, capacity 1, initial 1, service 2 s: one unpark and one arrival in the same tick; the arrival parks that tick after service; event order unparked, departed, …, arrived, parked; occupancy series exact | `parking` |
| PL4 | **Full lot**: first assert the lot is full and arrivals keep coming; then the gate queue grows, no vehicle is deleted, invariants 1–4 hold at every tick, and the queue spills back onto the upstream Link | `parking` |
| PL5 | **Empty lot**: an unpark candidate waits pending; the candidate behind it on the same source segment is still inserted that tick; `waitingForParked` counts it; the seed is overloaded when it should be | `parking` |
| PL6 | Gate service: a vehicle stopped at the gate parks exactly `gateServiceTime` after `since` when there is space; a clamp onto the gate is a safety clamp, not an arrival | `parking` |
| PL7 | Gridlock through a lot is reported and conserves vehicles; a cool-down leaves a full lot's gate queue unfinished | `parking` |
| PL8 | Same project and seed ⇒ same bytes; a checkpoint with lots round-trips; without lots, the four reference fixtures, checkpoints and every CLI output are byte-identical | `parking`, session `cmp` |
| PL9 | Outputs: interval series, summaries and wait selection with and without a cool-down; JSON and CSV only with a lot; batch Estimates; `--compare` by lot name with an unmatched lot | `parking`, `cli-compare` |
| PL10 | Editor: the G tool creates a lot from data defaults; the Lots table equals the CLI's batch; Copy and Export are the CLI's bytes; strings exist in `en.json` and `th.json` | `parking-ui` |
| PL11 | **M8 gate, end to end**: a development-access fixture in `data/projects/` whose only demand is an OD matrix through a lot over capacity runs through `--seeds` and the editor's Results, shows a gate queue on the access road, and loses no vehicle | `parking`, `parking-ui` |
