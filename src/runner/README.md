Multi-seed batches (M5.2): `runSeed`, `runSeeds` and `aggregate` in `batch.hpp`; scenario comparison
(M5.8a): `welch` and `compareBatches` in `compare.hpp`. Depends on core and
eval only — no model, project, Qt or I/O (`check_architecture`). Formatting lives beside the
single-run formatter in `src/project/batch_output.*`. Contract: [BATCH](../../docs/reference/BATCH.md).
