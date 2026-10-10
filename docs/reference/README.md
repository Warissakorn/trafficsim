# Reference and contracts

Current behaviour, numerical contracts and file semantics. Plans and dated
acceptance records live in [plans](../plans/README.md); milestone status lives in
[ROADMAP](../ROADMAP.md). Read the applicable contract and decision, not every file.

| Document | Scope |
|---|---|
| [BATCH.md](BATCH.md) | Multi-seed batches: runs, aggregate, 95 % CI, overloaded seeds and output (M5.2); scenario comparison `--compare` (M5.8, §7) |
| [LOS.md](LOS.md) | Level of service from section delay: the HCM pack as data, control types, approach/intersection rows (M5.5) |
| [ZONES_AND_OD.md](ZONES_AND_OD.md) | Zones, zone connectors and OD matrices: schema 29, codes, expansion into Micro inputs by fewest-object chains, rows ZO1–ZO12 (contract, not yet implemented; M8.2–M8.3, D150) |
| [PARKING_LOTS.md](PARKING_LOTS.md) | Parking lots as zones with capacity: schema 30, gate hold, OD-driven exits, invariants, outputs, rows PL1–PL11 (contract, not yet implemented; M8.4–M8.5, D150) |
| [TRAVEL_TIME_SECTIONS.md](TRAVEL_TIME_SECTIONS.md) | Travel-time sections: lines, crossings, section delay, output and acceptance rows (M5.4) |
| [DISCHARGE.md](DISCHARGE.md) | Lane/cycle discharge and startup diagnostics, quantization and unavailable results |
| [DRIVING_BEHAVIOUR.md](DRIVING_BEHAVIOUR.md) | Proposed class/road behavior assignment and target capability map |
| [W74.md](W74.md) | W74 car-following equations, parameters, traits, state and model switch (contract, not yet implemented) |
| [POSITIONED_ROUTING.md](POSITIONED_ROUTING.md) | Positioned Route recognition and schema 20 |
| [SIMULATION.md](SIMULATION.md) | Simulation core and network model |
| [AMBER.md](AMBER.md) | Amber stop-or-go: the continuous check, `amberDeceleration`, M0 legacy and acceptance rows (M4.2) |
| [VEHICLE_POSE.md](VEHICLE_POSE.md) | Vehicle pose — rear axle reference and continuous lane changes |
| [NETWORK_EDITOR.md](NETWORK_EDITOR.md) | Native network editor |
| [NETWORK_EDITOR_CONNECTORS.md](NETWORK_EDITOR_CONNECTORS.md) | Native network editor — Connectors |
| [CONNECTOR_FOUR_POINT_MOUTH.md](CONNECTOR_FOUR_POINT_MOUTH.md) | Centred Connector axis and four-point mouths |
| [EDITOR_WORKFLOW.md](EDITOR_WORKFLOW.md) | History, keyboard editing and rotation |
| [AUTHORING_EXTENSIONS.md](AUTHORING_EXTENSIONS.md) | Link geometry actions, positional-reference safety and shared markings |
| [MIGRATION.md](MIGRATION.md) | Native entry points, frozen migration baselines and replay compatibility |
| [DEMAND_TIME_TYPES.md](DEMAND_TIME_TYPES.md) | Time and type Demand — slice 6 |
| [DEMAND_CATALOGS.md](DEMAND_CATALOGS.md) | Project Demand catalogs — slice 5 |
| [M3_CONTRACT.md](M3_CONTRACT.md) | M3 right-of-way contract |
| [M3_8_CONTRACT.md](M3_8_CONTRACT.md) | M3.2.8 behaviour contract |

Return to [the task reading map](../README.md#read-by-task).
