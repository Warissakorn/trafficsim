# Batch runner (M5.2, D136)

`runSeeds` runs one compiled scenario once per seed, measured exactly as a single
`trafficsim-cli --project` run; `aggregate` sorts the runs by seed and gives, per movement and
approach, n, mean, SD and the 95 % Student-t half-width. Pure: it includes only `src/core` and
`src/eval` (checked by `trafficsim-check-architecture`), has no I/O, clock or threads, and its
output is formatted by `src/project/batch_output.*`. Not validated; not HCM control delay or LOS.
