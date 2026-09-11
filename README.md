# TrafficSim

A traffic microsimulator with its own simulation engine, built to the modelling surface PTV
Vissim users already think in, producing the movement-level delay and level-of-service output
that traffic impact studies require.

*`TrafficSim` is a working name; naming remains deferred until the end of M1 (D11).*

The M0 simulation core and link-based network model now run together. The development
harness shows seeded vehicles accelerating, queueing at fixed-time signals and crossing
connectors, with English/Thai controls. **Not yet validated:** the car-following model is
a reduced Wiedemann-inspired prototype, not W74/W99 or a calibrated Vissim equivalent.

## Run

Requires Node.js 22.12 or newer.

```bash
npm ci
npm run dev
npm test
npm run build
npm run simulate -- 42
```

`npm run simulate` prints completed-trip diagnostics and active/pending vehicle counts.
Delay includes source waiting and acceleration; it is not HCM control delay or LOS.

## Implemented

| Part | Entry point | Behaviour |
|---|---|---|
| Network model | `src/model/network/index.ts` | Links, lanes, explicit connectors, driving-side geometry, mid-link signal heads, validation and scenario compilation |
| Simulation core | `src/core/index.ts` | Frozen snapshots, fixed stepping, seeded arrivals, longitudinal following, signals, blocked-entry queues and event streaming |
| Demo | `src/main.ts` | Run/pause, step, reset, seed, playback speed and a passive canvas view |
| Headless diagnostic | `tools/run-simulation.ts` | Single-seed run, mean completed-trip delay and unfinished-demand counts |

The runtime currently rejects merging paths and cyclic routes. Lane changing,
crossing-conflict resolution, priority rules, editing, saving, batch evaluation and LOS
remain future work. Read [`docs/SIMULATION.md`](docs/SIMULATION.md) for API contracts,
units, numerical behaviour, validation and limitations.

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

**M0 — vertical slice implementation available; acceptance gate remains open.** The owner
must review whether the live traffic behaviour is plausible before M0 closes. See `Next`
in [`docs/PROGRESS.md`](docs/PROGRESS.md).
