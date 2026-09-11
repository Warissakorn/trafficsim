# TypeScript baseline fixtures

Captured before removing the TypeScript runtime, from the source tree at GitHub main
`70383db6ab884c718baef97a8ab81292fdc9d1b0`. That snapshot passed its 40 tests and build.
The source blobs matched local commit `dfd84fac40ee2fc230bafc8aae831ce5880de6cc` used
for capture. Data values are unchanged by the native migration.

Each `seed-*.json` has `seed`, `summary`, every event whose kind is not `moved`, and
complete state checkpoints every 100 ticks. Checkpoint keys are `tick`, `randomState`,
`nextVehicleId`, `completed`, `vehicles`, and `inputs` (including pending queues).

To reproduce a capture in a separate baseline checkout, collect `runSimulation` events,
apply the old `summarize`, and independently step `createSimulation` to the final tick,
recording those checkpoint fields whenever `state.tick % 100 === 0`. The four seeds are
0, 42, 43 and 4294967295. Fixtures are stored compactly because full states are repetitive.

These are frozen migration evidence. Do not update them when a native test fails.
See `../../docs/MIGRATION.md` for tolerances and the limits of sampled trajectory checks.
