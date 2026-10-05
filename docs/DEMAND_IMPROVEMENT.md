# Demand improvement — 2026-10-05

Owner scope: review and improve Demand only. Reporting, LOS, seed batches,
warm-up and evaluation windows remain M5; vehicle dynamics remain M3.
This work extends M2 without reopening or claiming its observed gate.

## Contract for the first slice

1. Vehicle input volume is authoritative. Turning counts are relative weights,
   not a second source of vehicles. Composition and lane/route expansion conserve
   that volume, subject to the existing reachability gates.
2. Time intervals are half-open, sorted and non-overlapping. Gaps and unequal
   lengths are valid. Input gaps produce no demand. Turning gaps and all-zero
   turning intervals use whole-period weights, as before.
3. Timed routing uses scheduled demand time, before the source queue. Queued
   vehicles keep their compiled destination. Actual entry time and arrival at a
   downstream decision do not reselect it. This slice documents the existing
   semantics rather than changing the random stream or routing model.
4. Lane weights are finite, non-negative and have a positive finite total.
   Zero excludes a lane. Empty weights mean equal split. A positive stale vector
   retains D32's equal-split fallback, now with an advisory. Invalid stale vectors
   cannot evade validation. Entry decisions and unplaced multi-route decisions
   allocate lanes independently and ignore authored lane weights; the UI says so.
5. Opening, confirming, renaming or cancelling a decision preserves its interval
   matrix unless an interval/count/flow change is explicit. Input scalar totals
   derived from intervals cannot be edited as independent values.
6. Every active counted turning row has the same number of counts. Missing cells
   are rejected; zero counts must be explicit. Unused zero-flow rows may stay empty.
7. The explicit period dialog edits seconds and rates/turning weights, accepts gaps
   and unequal lengths, and validates ordering before acceptance. It remains local
   until the parent dialog commits through History. Pending changes to compact
   counts, flows or placement must be committed before opening it.
8. Preview compiles the same document/catalog snapshot as Run. It lists compiled
   input/type/route, entry link/lane, path endpoint, interval rate and expected
   vehicles. It does not generate arrivals, advance simulation or mutate History.
   Expected counts are rate integrals, not exact Poisson counts or entered/completed
   vehicle counts. A lane-change stub's last link is not its ultimate destination.
   Invalid demand blocks preview rather than producing an incomplete table.

## Delivery order and gates

| Slice | Deliverable | Acceptance |
|---|---|---|
| 1 | Validation and lane-share correctness | Zero lane stays empty; invalid weights rejected atomically; default and positive stale behavior preserved |
| 2 | Lossless interval editing | Irregular input/turn periods survive no-op, rename, Cancel, Undo and save/reopen |
| 3 | Explicit period authoring | Gaps and unequal lengths editable; overlaps rejected; incomplete count rows rejected |
| 4 | Compiled Demand preview | Rates and composition/lane/route totals match Run; immutable revision; invalid demand rejected |
| 5 | Vehicle type and composition authoring | Project-owned catalog editing, reference validation, migration and portable save/reopen before UI delivery |
| 6 | Time-varying composition and type-specific routing | Explicit interval/type rules, conservation across every breakpoint, backward compatibility and deterministic replay |

Slices 1–4 are PR #106. Slice 5 is implemented in the subsequent catalog branch;
[DEMAND_CATALOGS.md](DEMAND_CATALOGS.md) records its ownership and editing contract.
Slice 6 remains planned; native CI and owner appearance gates remain distinct.
Do not claim completion from the presence of an external JSON catalog or a combo box.

## Vehicle type and composition scope for the next slice

Keep the meanings distinct: a type describes one vehicle's physical/dynamic
parameters; a composition distributes an input over types. Current car/heavy
catalogs and fixed composition shares remain supported. Add catalog editing with
project ownership, IDs, names, references and checks for dimensions, speed,
acceleration/deceleration and driver behavior. Reuse the existing engine fields
rather than creating a second definition in the shell. Wheelbase/overhangs affect
appearance; changing them must not silently alter arrival or car-following models.

Motorcycle-heavy counts need a separately specified dynamics/lane-occupation
contract before a motorcycle type can be claimed as supported. Articulated vehicles
also need an articulation model; a heavy rigid body is not a trailer train.
Time-varying shares, type-conditioned destinations and desired-speed distributions
need their own authored schema, migration, validation and deterministic sampling
contracts. Decide exact arrival/count mode only after defining interaction with
Poisson arrivals and source queues; Preview must never imply exact counts today.

## Verification and limits

Regression cases first exposed three existing lane-weight failures. Tests cover
correctness, compiled conservation and Qt editing. Frozen reference fixtures are
not regenerated. Linux automated desktop checks use Qt's offscreen platform.
Windows native CI and the owner's desktop appearance review remain distinct gates.
No benchmark/calibration, LOS or Vissim-equivalence claim is added by this work.
