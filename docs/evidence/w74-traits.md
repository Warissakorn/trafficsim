# M3.3.3a W74 driver-trait evidence — 2026-10-08

Branch `claude/dazzling-gates-df64jc` on `312f402` (after D136). Local host: Linux, GCC 13.3.0,
Clang 18.1.3, CMake 3.28.3, Ninja 1.11.1, Qt 6.4.2 (offscreen). `--preset headless` 68/68,
`--preset desktop` 99/99. Windows evidence belongs to the PR's native CI. Contract:
[W74](../reference/W74.md) §5; decision D137.

## Rows (`tests/w74_traits_tests.cpp`, ctest `w74traits`)

| Row | Case | Asserts |
|---|---|---|
| BA26 | `w74Traits` at (42, 1, 0.5), (0, 7, 0.3), (2³²−1, 2⁴⁰, 1.0) | Bit-equal (`==`) to an independent Python implementation of §5 (below); seed 0 is not remapped |
| BA26 | 2,000 ids at seed 42 | Each `u` in [0, 1), `zOp` in [0, 1], `zBx` is `driverFactor`, repeatable, distinct per id and seed |
| BA26 | `test::straight` vs the same plus an unused `w74` behaviour, seed 42, every tick to the end | Equal `randomState`, `nextVehicleId`, events, vehicles and queues once traits are removed; prototype vehicles carry none; every mixed vehicle carries `w74Traits(42, id, driverFactor)`; `upcomingArrivals` predicts the traits the step creates |
| BA26 | State copied at tick 300 and both stepped 300 more; two full runs at seed 7 | Equal vehicles, traits included |
| §9 | Checkpoints of both runs | `w74Traits` only in the mixed one, with the vehicle's values |

Golden values (hex doubles; zBx, zEx, zCx, zOp, zOsc) from `python3` with 64-bit masking:

```
(42, 1, 0.5)            0x1p-1 0x1.713d41f672900p-7 0x1.0732c71a88a40p-1 0x1.6a008383d7572p-2 0x1.6dfa0f552f5c4p-3
(0, 7, 0.3)             0x1.3333333333333p-2 0x1.6414d5f0fa298p-3 0x1.e29e59f004107p-1 0x1.3ac1386f8586dp-1 0x1.d5213525c2277p-1
(4294967295, 2^40, 1.0) 0x1p+0 0x1.720a8f641f508p-2 0x1.9a15efd9987f0p-2 0x1.24199bbfca707p-1 0x1.54b63d1dc2237p-1
```

## Two compilers

A one-off driver (not committed) printed 1,000 traits (seeds 0, 42, 81, 2³²−1; ids
7919·1..250) as `%a` after compiling `src/core/w74.cpp` with g++ 13.3.0 and clang++ 18.1.3
(`-std=c++20 -O2 -fno-fast-math -ffp-contract=off`): `cmp` identical, SHA-256 prefix
`bba4674213044336`. MSVC is CI's.

Seeded mutations each fail a case: `upcomingArrivals` without the seed copy (found and fixed
while writing these rows), traits on every scenario, `>> 12` instead of `>> 11`, a trait taken
from `randomState`, and checkpoints without the key. `w74codec`'s unused-behaviour run now
compares vehicles without traits and asserts they are present.

## Byte guards and limits

`trafficsim-cli 42` byte-identical to `48a3276`; frozen TS baselines unchanged. Nothing reads
the traits yet; no W74 run, state or model switch (BA23–BA25, BA27, BA28 come with composition).
