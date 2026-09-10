# TrafficSim

A traffic microsimulator with its own simulation engine, built to the modelling surface PTV
Vissim users already think in, producing the movement-level delay and level-of-service output
that traffic impact studies require.

*Working name. No code yet — this repository currently holds the project's documentation spine.*

## Start here

| | |
|---|---|
| **[`docs/PROBLEM.md`](docs/PROBLEM.md)** | Who this is for, what is broken today, what "done" means, and what would make this project wrong |
| **[`docs/PRINCIPLES.md`](docs/PRINCIPLES.md)** | Rules that do not get relitigated, and hard-won discipline inherited from a prior effort |
| **[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)** | The intended map — five layers, dependencies pointing one way |
| **[`docs/ROADMAP.md`](docs/ROADMAP.md)** | M0–M7, each with a done-condition; two hard gates |
| **[`docs/PROGRESS.md`](docs/PROGRESS.md)** | Append-only log, decisions with reasoning, and the `Next` task |
| **[`CLAUDE.md`](CLAUDE.md)** | Standing orders for every working session |

## Why not just wrap an existing engine

That was tried, for six milestones, and the interface was not the problem. Conflict areas are
output-only in SUMO, gap times are not expressible, signal heads must sit at a stop line, and
the two outputs a traffic impact study is actually paid for — per-movement delay/LOS and
multi-run averaging — have to be built from scratch either way.

The full argument, and the test that would prove it wrong, are in
[`docs/PROBLEM.md`](docs/PROBLEM.md) §2 and §7.

## Status

**M0 — vertical slice.** Next task: toolchain setup. See `Next` in
[`docs/PROGRESS.md`](docs/PROGRESS.md).
