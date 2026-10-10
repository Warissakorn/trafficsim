# Zones and OD matrices (M8.2–M8.3)

**Contract only, written 2026-10-10 (M8.1, [D150](../decisions/RECORD.md#d150)); nothing below is
implemented.** It defines the authoring objects that state demand as trips between zones, how a
matrix expands into the inputs and routes the Micro engine already runs, and the rows that close
M8.2 (schema, codec, commands) and M8.3 (expansion). Parking lots, which are zones with a capacity,
are [PARKING_LOTS](PARKING_LOTS.md). The booked design is
[MULTI_LEVEL_MODELLING](../plans/MULTI_LEVEL_MODELLING.md) §3.2; D150 records where this contract
departs from it. Every result a matrix feeds is **simulated, not validated** (rule 4), and a
matrix states expected trips, not counts.

## 1. The objects (schema 29)

```json
"network": {
  "zones": [{"id": "zone-1", "name": "Site", "polygon": [{"x": 0, "y": 0}, {"x": 40, "y": 0}, {"x": 40, "y": 30}]}],
  "zoneConnectors": [
    {"id": "zc-1", "zoneId": "zone-1", "linkId": "link-7", "direction": "origin", "share": 1},
    {"id": "zc-2", "zoneId": "zone-1", "linkId": "link-8", "direction": "destination"}
  ]
},
"definition": {
  "odMatrices": [{"id": "od-am", "name": "AM peak", "startTime": 0, "endTime": 3600,
                  "compositionId": "urban-mixed", "zones": ["zone-1", "zone-2"],
                  "trips": [[0, 120], [80, 0]]}]
}
```

- **Zone** `{id, name, polygon}`. The polygon (at least three finite points, metres in network
  coordinates) is where the zone is drawn and picked; Micro uses no other zone property. Land-use
  attributes (population, jobs) belong to M10.2's contract and are not stored now: nothing would
  read them, and a shape chosen blind would be reshaped later.
- **ZoneConnector** `{id, zoneId, linkId, direction, share}`. `direction` is `"origin"` (trips
  leave the zone onto the Link) or `"destination"` (trips end at the Link). An **origin attaches at
  the Link's start, a destination at its end**; there is no position along the Link (§5).
  `share` is the connector's relative weight among its zone's connectors of the same direction,
  above 0, written only when not 1.
- **OdMatrix** `{id, name, startTime, endTime, vehicleTypeId | compositionId, zones, trips}`.
  `trips[o][d]` is the **expected number of trips** from `zones[o]` to `zones[d]` released in
  `[startTime, endTime)`, seconds of the run. The demand class is exactly one vehicle type or one
  composition, as for an input. A matrix is one period and one class; a day is several matrices.
- **Schema and bytes.** The three keys are written only when non-empty, so a file without them
  keeps its schema and bytes (the rule of [NETWORK_EDITOR](NETWORK_EDITOR.md)'s schema list). A
  file with any of them is schema 29: 29 heads the schema ternary in `documentJson`
  (`document.cpp`), and the reader's upper bound becomes 29.
- **Refused on load** (the open document is kept):
  - below schema 29, including an M0 file (version 0): any of the three keys, by a raw-JSON
    `rejectZonesAndOdBefore29`, as `rejectCooldownBefore27` does (`EDIT_UNSUPPORTED_FIELD` with the
    key's path);
  - at 29: any other key in these objects (`knownFields`, `parse.cpp`), as for every network
    object.

### Structural rules

Checked on every load and every edit (`validateDocument`); the edit or the load is refused and the
document is unchanged. Paths name the authored object (`zones[i]`, `zoneConnectors[i]`,
`odMatrices[i]`).

| Code | Refused when |
|---|---|
| `INVALID_ID` | an id is blank, or a zone, connector or matrix id contains `/` or `>` (they separate the parts of generated ids, §3) |
| `DUPLICATE_ID` | a zone or connector id is used by any other network object; a matrix id by another matrix |
| `ZONE_POLYGON_INVALID` | fewer than three points, a non-finite coordinate, or zero area |
| `UNKNOWN_ZONE` | a connector or a matrix's `zones` names no zone |
| `UNKNOWN_ZONE_CONNECTOR_LINK` | a connector names no Link (not `UNKNOWN_LINK`, which is already a routeless Run code) |
| `ZONE_CONNECTOR_DIRECTION` | `direction` is neither `origin` nor `destination` |
| `ZONE_CONNECTOR_SHARE_INVALID` | `share` is not finite or not above 0 |
| `OD_MATRIX_SHAPE` | `zones` is empty or repeats a zone, or `trips` is not `zones × zones` |
| `OD_CELL_INVALID` | a cell is negative or not finite |
| `OD_PERIOD_INVALID` | not `0 ≤ startTime < endTime ≤ duration`, or a bound off the time grid. Shortening the duration below a matrix's end is refused with this code, as an input's interval is |
| `OD_PERIOD_OVERLAP` | two matrices of the same class overlap in time (they would add silently) |
| `OD_DEMAND_CLASS` | not exactly one of `vehicleTypeId` and `compositionId`, or the one given is blank |
| `INVALID_ID` (route, input) | in a schema-29 file, an authored route or input id starting with `od:`, the prefix §3 reserves. Older files cannot carry a matrix, so their ids are never refused |

Whether the named type or composition exists is checked on Run, with the existing
`UNKNOWN_VEHICLE_TYPE` and `UNKNOWN_COMPOSITION` on the matrix's path; `compositionIssues` and the
catalog's "used" rule (`resolveCatalogs`) count a matrix that names a composition, and deleting a
type or composition that a matrix names is refused like one an input names.

## 2. Demand and the Run checks

- **A matrix is demand.** A matrix with at least one positive cell counts as authored demand
  wherever inputs do: `onlyRunSettings` (`diagnostics.cpp`), the `EDIT_NO_INPUTS` guard in
  `compileDocument` and its row in `runDiagnostics` and `documentDiagnostics`. A project whose only
  demand is a matrix therefore runs. Its locale text says "no inputs or OD matrix".
- **Run checks, one function.** `odRunIssues(network, definition)` reports every OD failure on an
  authored path. `compileDocument` refuses on it before expanding; `runDiagnostics` and
  `documentDiagnostics` list it as runtime rows; `validateNetwork` and `validateAuthoredDemand`
  never emit it, so authoring tolerates an incomplete demand as it tolerates an unconnected
  route (D18b). No Problems row ever names a generated `inputs[N]` or `routes[N]`.

| Code | Run refuses when | Path |
|---|---|---|
| `OD_ORIGIN_LINK_INTERNAL` | an origin connector's Link has an incoming Connector: an engine source must start on a segment with no predecessor (`UNSUPPORTED_INTERNAL_INPUT`, `core/validate.cpp`) | `zoneConnectors[i]` |
| `OD_INTRAZONAL_TRIPS` | a diagonal cell is above 0: such trips use no road in Micro and would otherwise vanish | `odMatrices[i].trips[o][o]` |
| `OD_ZONE_NO_CONNECTOR` | a row (column) with a positive cell belongs to a zone with no origin (destination) connector | `odMatrices[i].zones[o]` |
| `OD_PAIR_UNREACHABLE` | a pair of connectors with flow (§3) has no drivable chain | `odMatrices[i].trips[o][d]` |
| `OD_PATH_SEARCH_LIMIT` | the chain search stopped at its limit, not at the topology (§3) | `odMatrices[i].trips[o][d]` |
| `OD_NOT_EXPANDED` | M8.2 only, removed by M8.3: a matrix has a positive cell, so storing matrices before the expansion exists never drops a trip silently | `odMatrices[i]` |

## 3. Expansion into Micro inputs and routes (M8.3)

One function, `withOdDemand(network, definition)`, returns the definition with the matrices
expanded into **ordinary authored Routes and VehicleInputs**, appended after the authored ones so
every authored `routes[i]` and `inputs[i]` path still names the same object. It runs **first** in
all four callers: `compileDocument`, `runDiagnostics`, `documentDiagnostics` and
`validateAuthoredDemand`. So it runs before `EDIT_NO_INPUTS`, `compositionIssues`,
`resolveCatalogs` (compositions, then routing decisions) and `expandRouteless`, and the authoring
checks, the Problems tab, the demand preview and Run see the same inputs. The cool-down is still
added last (`run.cpp`).

**Volume.** For matrix `m`, origin zone `o`, destination zone `d`, an origin connector `c` of `o` and
a destination connector `k` of `d`:

```text
vph(c, k) = trips[o][d] · 3600 / (endTime − startTime) · share(c) / Σ share(origin connectors of o)
                                                        · share(k) / Σ share(destination connectors of d)
```

Each `(m, o, d, c, k)` with `vph > 0` becomes one input on `c`'s Link over `[startTime, endTime)`
with the matrix's class. A zero cell or a pair that rounds to no flow emits nothing; there is no
minimum.

**Path: fewest objects (D150).** Until M9 assigns paths by cost, a trip takes the chain through the
fewest Links and Connectors, the rule a placed routing decision already uses to reach its
destination:

1. When `c` and `k` are on the same Link, the chain is that Link alone.
2. Otherwise the chains are `routeShortestChains(network, table, c.link, k.link)`
   (`model/network/routing.cpp`), breadth first, kept only when `routeLaneChains` gives at least
   one single-lane chain. A level whose chains are all undrivable is the answer: the search does
   not fall back to a longer level, so that pair is `OD_PAIR_UNREACHABLE`, as it is for a placed
   decision.
3. **Limits.** The search stops at 20 objects deep or 4 096 partial chains on one level. Stopping
   at a limit is `OD_PATH_SEARCH_LIMIT`, never `OD_PAIR_UNREACHABLE`, so the author can tell a
   network too large for the rule from a disconnected one.
4. **Lanes.** Each lane of `c`'s Link takes the first of the equally short chains (in the search's
   order) that it can drive in full; a lane that can drive none takes the first chain it can reach
   as a stub and changes lanes there, by the M3.2.8b rules
   ([M3_8_CONTRACT](M3_8_CONTRACT.md) §2). A lane with neither carries none of the pair's flow. The
   input splits **equally over the lanes that carry it**, not over all the Link's lanes.
5. The route ends at the end of `k`'s Link, where the vehicle arrives. Nothing walks on past it.
6. **No decision acts on an OD trip.** Static, placed and positioned routing decisions and their
   type rules apply to authored inputs only; an OD vehicle passing a positioned decision keeps its
   route.

**Ids and order (part of the random-number contract).**
- One generated Route per `(c.link, k.link)` pair, shared by every matrix and cell that uses it:
  `od:route/<cLink>><kLink>`.
- One input per `(m, o, d, c, k)`: `od:<matrix>/<origin zone>><destination zone>/<c>><k>`. The
  §1 id rules make both injective.
- Order: matrices in file order, then origin by `zones` index, then destination by `zones` index,
  then origin connectors, then destination connectors, each in network order.
- Every input draws its arrivals from the run's one random stream in canonical id order
  ([SIMULATION](SIMULATION.md#determinism-snapshots-and-events)). A file **with** a matrix may
  therefore draw differently from the same demand authored as inputs. A file without one draws
  exactly as before (DEMAND_TIME_TYPES's precedent).

**Cost.** Chains are resolved through one `ConnectorPathTable` and one memo per expansion, keyed by
`(c.link, k.link)`, as `DestinationChainMemo` does for routeless destinations (D140). Validation
on every edit then walks each distinct pair once.

**What the expansion feeds.** The generated inputs are ordinary inputs, so the rest of the
pipeline applies unchanged:
- a composition class expands to `…/type-<type>` inputs (`expandCompositions`);
- the lane split and periods compile as for any input (`buildScenario`);
- the trips' count in a seed is Poisson around the expected trips (exact counts are a
  [DEMAND_IMPROVEMENT](../plans/DEMAND_IMPROVEMENT.md) contract, not this one);
- movement rows group trips by first and last Link and merge with authored routes on the same pair
  ([SIMULATION](SIMULATION.md#movement-evaluation-m25)).

## 4. Edits and commands (M8.2)

- **Commands.** `putZone`, `putZoneConnector`, `putOdMatrix` (empty id allocates `zone-N`,
  `zc-N`, `od-N`; the same id replaces) and `deleteZone`, `deleteZoneConnector`, `deleteOdMatrix`,
  each one Undo step through `History::execute`.
- **Cascades:**
  - Deleting a **Link** deletes its connectors in the same step, as its sections are deleted
    (`removeControlsOn`).
  - Deleting a **zone** deletes its connectors in the same step. It is refused with
    `EDIT_REFERENCED_ZONE` while a matrix or a parking lot names the zone, so no matrix row is
    silently dropped.
  - **Reversing** a Link that carries a connector is refused with `EDIT_REFERENCED_LINK`, whose
    text is extended to name connectors (`controlsNameLink`).
  - **Splitting** a Link keeps an origin connector on the first piece and moves a destination
    connector to the last (`checkSplitControls`, `splitControls`).
  - Copy, duplicate and the opposite Link never copy connectors ("demand is not copied").
- **Registries every new kind joins:**
  - `allocateId`'s used-id set;
  - the network-wide id set in each issue function;
  - the Link cascades above;
  - `objectIdForPath` (zones, connectors) and `definitionId` (matrices), so a Problems row selects
    its object;
  - `deleteObjects` (the canvas Delete);
  - a locale entry in `en.json` and `th.json` for every code above.
- **Editor (M8.6).**
  - **Zone tool `Z`** (free; `P` is cycle priority): click the polygon's points, close it, then
    click a Link's start or end to add an origin or destination connector.
  - **OD matrix table**: zones × zones cells, period and class, with row and column totals.
  - Zones are selected, moved and deleted like other objects. A zone polygon moves with
    translate and rotate; its connectors stay on their Links.

## 5. Limits of this contract

- **No position.** A connector attaches at a Link end because the engine's sources start on a
  segment with no predecessor and lanes are cut only where Connectors attach. A connector part
  way along a Link needs a core source contract of its own.
- **No `both`.** A Link is one-way, so a zone that sends and receives on one road uses two
  connectors on two Links.
- **No assignment.** Fewest objects ignores congestion and length; M9 replaces it, starting from
  these chains.
- Not in this contract: exact counts, centreline-only Links (M10.6), zone attributes (M10.2), and
  a cut from a Macro model (M12).

## 6. Acceptance rows

| Row | Check | Test |
|---|---|---|
| ZO1 | Schema-29 round trip of all three keys; a file without them keeps its schema and bytes after a save through the new codec; each key below 29 (including version 0) and any unknown key in the objects is refused with its path | `zones` |
| ZO2 | Every structural code of §1, one case each, including an id containing `>`, an id clash with a travel-time section, overlapping same-class periods, and an authored `od:` route id in a schema-29 file (accepted in a schema-28 file) | `zones` |
| ZO3 | Every Run code of §2 on its authored path; no Problems row names a generated input or route; authoring accepts each case | `zones` |
| ZO4 | A project whose only demand is one matrix compiles and runs; a matrix of zero cells counts as no demand | `zones` |
| ZO5 | Hand-computed expansion: two origin and two destination connectors with unequal shares give the four `vph` values, ids and order of §3; a zero cell emits no input | `zones` |
| ZO6 | `c` and `k` on one Link give a one-Link route; two equally short chains with one lane able to drive only the second split as §3 step 4; an undrivable shortest level is `OD_PAIR_UNREACHABLE` even though a longer drivable chain exists | `zones` |
| ZO7 | A pair 21 objects apart is `OD_PATH_SEARCH_LIMIT`, not unreachable | `zones` |
| ZO8 | A composition-class matrix expands to `…/type-<type>` inputs; a positioned decision on an OD vehicle's Link does not change its route | `zones` |
| ZO9 | Byte-identical without matrices: the four reference fixtures (events, checkpoints, summary, trajectory digest), and single-run, `--seeds` and `--compare` JSON and CSV for the four shipped projects, before and after M8.2 and M8.3 | `zones`, session `cmp` |
| ZO10 | Commands and cascades of §4, each one Undo step: Link delete, zone delete with connectors, zone named by a matrix refused, reverse refused, split moves the destination connector | `zones` |
| ZO11 | Same project and seed ⇒ same bytes with matrices; the expansion runs once per distinct `(c.link, k.link)` (memo) | `zones` |
| ZO12 | Editor (M8.6): the Z tool draws a zone and adds both connector kinds; the OD table edits a cell through History; every new code and UI string exists in `en.json` and `th.json` | `zones-ui`, `project` |
