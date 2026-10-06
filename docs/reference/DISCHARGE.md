# Queue discharge measurement (M3.3.1a)

`DischargeAccumulator` observes consecutive immutable `SimState` snapshots from
creation through the end of the run. `estimateDischarge` also accepts explicit
crossing records independently of the engine. This is an unvalidated diagnostic,
not calibrated saturation flow, PCU capacity, control delay or LOS (D121).
Existing `StopLineAccumulator` output is unchanged.

## Records and windows

Each signal head defines one lane (`segmentId`). Routes use the first occurrence
of that segment, as the existing stop-line diagnostic does. Repeated-segment routes
are outside this first slice's supported topology. Cycle Go is today's green
transition; red or amber ends it. Records contain vehicle ID, vehicle-type ID,
end-of-tick front-crossing time, and membership in the queue at Go. Classes are
not assigned yet: records retain every vehicle type; selection affects estimator
samples only, and no PCU conversion occurs.

At Go, upstream vehicles are sorted by distance to the head, then ID. The initial
queue is the contiguous prefix below `QueueDefinition.beginSpeed`, with each gap
from the head or preceding vehicle rear at most `maxGap`. This uses the project's
queue catalog. It is not an empirical saturation threshold.

Green intervals are half-open `[go,end)`. A crossing stamped at the green's end
is excluded. No interpolation is attempted: physical passage within a tick is
reported at its end, with up to one timestep of timestamp quantization; a gap
between two such timestamps can differ by up to one timestep. Simultaneous
tick crossings can yield zero gaps and are unavailable, not infinite flow.
Signal transition ordering follows the current engine's end-of-tick events.

Only complete greens wholly inside `[max(windowStart,warmup),windowEnd]` are
eligible. An initial observed green is conservatively unavailable, including
one starting at time zero; an unfinished last cycle is unavailable. Repeated
observation of the same tick is ignored; missing/out-of-order ticks or changed
Scenario identity throw. The caller must not replace a snapshot within a tick.
Lane changes or route choices that change the physical prefix through a head
make that whole cycle unavailable; shared-prefix route recognition is supported
as defined below. Source departures already beyond the head or arriving within the
insertion tick conservatively invalidate it because the prior position/type
snapshot is absent. Sink arrivals of previously upstream tracked vehicles are
counted explicitly, even without a survivor snapshot.

## Estimator

Ranks are one-based and declared before observation. For steady ranks `a..b`,
where `a >= 2`, average `t[r]-t[r-1]` over those ranks. The rate is `3600/h`
vehicles/hour for this lane/cycle. For startup ranks `1..k`, where `k < a`,
startup lost time is `t[k]-go-k*h`, retaining its sign. Every crossing through
rank `b` must have been in the initial contiguous queue. Extra crossings remain
in the raw output and do not change the declared rank interval.

Invalid windows/rank declarations throw. Partial/outside-window cycles,
insufficient crossings, non-queued ranks, invalid identities/timestamps,
nonpositive headways and conservative remap/source exclusions return an explicit
reason with absent numeric estimates. Consumers must display unavailable as such.

## CLI and remaining gate

`trafficsim-cli --project FILE.traffic.json --discharge` adds `discharge` JSON.
Defaults are the full run window, zero warmup, steady ranks 3–5 and startup ranks
1–2. All defaults, timestamp convention, timestep, sample count, crossings,
cycle completeness and reasons are emitted. Unavailable numeric fields are null.
CLI controls and vehicle-type selection are defined below. Future class assignment
remains separate. Existing top-level seed/compiler/engine-version output applies.
The CLI emits captured project/catalog hashes as defined below. Retain the
original inputs and exact executable commit/toolchain alongside that JSON.

BA03 has type-selection/window evidence; future vehicle classes are not assigned.
BA05 remains open: complete remap/source passage tracking is still pending. Native/desktop CI and empirical/owner validation are separate gates.
See [delivery rows](../plans/DRIVING_BEHAVIOUR.md) and
[local evidence](../evidence/discharge-measurement.md).

## Declared controls and vehicle-type selection (M3.3.1b1)

The CLI accepts `--discharge-start S`, `--discharge-end S`,
`--discharge-warmup S`, `--discharge-steady-first N`,
`--discharge-steady-last N`, `--discharge-startup-last N`, and repeatable
`--discharge-type ID`. These require `--discharge --project FILE`.
Seconds must be finite and nonnegative, ranks positive integers; scientific
window/rank restrictions above still apply, and the end cannot exceed run duration.
An unknown type ID rejects before the run. Empty type selection means all types.

Type selection keeps the full raw crossing stream and original ranks. A steady
sample is included only when its follower (the vehicle at rank r) has a selected
vehicle type; its predecessor remains the actual vehicle at rank r-1, of any type.
Skipped samples never connect nonadjacent selected vehicles into a fictitious gap.
Report the sampled ranks and count. No selected samples means unavailable.
`3600/h` is inverse selected-follower mean headway, not the selected type's hourly
throughput or a new approach capacity. These IDs are current vehicle types, not
future behavior/vehicle-class assignments, and no PCU conversion occurs.

Startup uses that selected steady reference only when every vehicle in original
startup ranks 1..k is selected. Otherwise headway can remain available while
startup is null with `mixed_type_startup_prefix`. The JSON emits this separate
startup reason and the selection/rate definitions. Unfiltered defaults retain the
previous numeric results. Captured input hashes and supported shared-prefix recognition are defined below.
Complete lateral/source reconstruction remains the next slice; BA05 stays open.

## Captured input provenance (M3.3.1b2a)

With `--discharge`, `inputManifest` records SHA-256, byte count and read count for
`project`, every catalog JSON file actually read by compilation/validation, and
`evaluation/queue-counter.json`. Names are logical relative paths, not host-specific
absolute paths. Entries are sorted by logical path. Project-owned catalogs remain
inside the project hash; unused external catalogs do not appear.

The parser and digest consume the same captured binary bytes, including whitespace,
BOM and line endings. JSON is not normalized or reserialized before hashing.
Repeated reads of a logical file must have identical bytes/digest; a mismatch
rejects before simulation. Best-effort priority-catalog fallbacks are recorded,
including files read before parsing fails. The collector is explicit and optional;
ordinary loading and simulation retain their previous contracts. No filesystem
or hashing code reaches core/eval.

This is the identity of actual file reads, not a directory snapshot or an archived
copy of inputs. Hashes do not recreate missing files, establish calibration, or
capture the executable's source commit. Retain the original files and exact build
commit alongside top-level engine/compiler/seed fields for replay.

## Supported route recognition and remaining passage limits

A positioned `RoutingEvent` can preserve the queue and crossings when source and
target routes have an identical physical segment prefix through this head.
Recognizing a different suffix does not move the front off that shared lane; keep
its original queued-at-Go membership and count a subsequent passage once.
Changing the physical prefix through a head, including diverting to a suffix
without that head, remains unavailable for that cycle. Such remaps suppress raw
crossing inference too: arrival on a diverted route is not passage of the old head.
Lane changes and ambiguous insertion-tick source passages remain conservative
unavailable cases. Complete lateral/source passage reconstruction is still BA05
work; this supported shared-prefix case does not close that gate.
