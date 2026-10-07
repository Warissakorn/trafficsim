# M3.3.2b front-segment behaviour selection evidence — 2026-10-07

Branch `claude/review-all-prs-kyycls` on `main` `72127d3` (after PR #121). Local host:
Linux, GCC 13.3.0, CMake 3.28.3, Ninja 1.11.1, nlohmann/json 3.11.3, Qt 6.4.2.
Native Linux/Windows CI for the PR is separate. Contract:
[DRIVING_BEHAVIOUR §8](../reference/DRIVING_BEHAVIOUR.md#8-implemented-runtime-selection-m332b-d127); decision D127.

## Fixtures (`tests/behaviour_selection_tests.cpp`, group `behaviourselection`)

| Row | Case | Asserts |
|---|---|---|
| BA10, BA11 | `assignment_precedence_and_every_section_and_path_inherit_their_owner` | Two-lane Link cut by a mid-link Connector (3 sections) and a two-path Connector: every section/path gets its owner's type; heavy class → override, car → default; unassigned Link has no entry; compiled table equals the compiler's, one entry per segment × type; no assignment → empty |
| BA12, BA15 | `front_boundary_downstream_rear_ignored_sink_last_and_legacy_without_table` | Segments declared out of canonical order; front 1e-9 before a join → upstream set, exactly at and after → downstream, rear on the upstream segment ignored, past the sink → last; a type without an entry → legacy; no table → empty index and legacy everywhere; `resolveRefs` gives leader and follower their own slots |
| — | `compiled_selections_are_validated` | `UNKNOWN_SEGMENT`, `UNKNOWN_VEHICLE_TYPE`, `UNKNOWN_BEHAVIOUR`, repeated pair `DUPLICATE_ID`; `createSimulation` refuses |
| BA16 | `entry_profile_holds_the_same_pending_vehicle_without_redrawing` | A 50 m standstill on the entry segment holds a pending vehicle that legacy inserts (6 m gap); the pending record is unchanged and no random draw happens |
| BA13, BA18 | `same_set_assignment_keeps_the_trajectory_and_a_different_set_changes_it` | four-leg-signalised with owned catalogs: every Link/Connector assigned the legacy set → 900 ticks of vehicles, events, RNG and completions identical to unassigned; a set with three times the following time changes the trajectory |
| — | `zone_waiting_room_covers_an_assigned_larger_standstill` | Waiting room unchanged without assignments, grows to an assigned larger standstill |

`behaviourlibrary.assigned_roads_compile_and_an_unassigned_library_changes_nothing`
replaces the M3.3.2a refusal case: an assigned road compiles to a non-empty table
with no runtime diagnostic; an unassigned library still compiles to an equal Scenario.

Mutations, each rebuilt and run: `resolveRefs` back to the type slot (2 cases fail),
`<=` at the join (1), legacy behaviour at source insertion (1), class override ignored
(1), Connector paths skipped (1), duplicate-pair check removed (1). A canonical sort
of the table survived because lookups are by id; it was removed as unneeded.

## Commands and results

`cmake --preset headless && cmake --build --preset headless && ctest --preset headless`:
65/65, including the four frozen TS reference baselines and trajectory digest
(never regenerated), architecture, file-size and documentation guards. Desktop
preset (Qt 6.4.2, offscreen): 95/95.

Seed-42 `trafficsim-cli --project … --stop-lines --discharge --lane-changes
--wait-causes` for four-leg-signalised, t-junction-priority, lane-change-lab and
m2.6-study-template equals a `main` `72127d3` Release build after removing the
compiler/version fields and input manifests.

`trafficsim-engine-benchmark` (12 intersections, 300 s, Release, three alternating
runs each): `main` 0.27–0.31 µs per vehicle-tick, candidate 0.27–0.31 µs — within
noise. The benchmark has no assignment; the assigned path's per-vehicle front lookup
is not yet benchmarked.

## Limits

BA14 (positioned routing) and BA17 (Stop service at an unchanged line) hold by
construction — references are rebuilt after source-zero routing and lane changes,
and a served Stop line is kept without reading the behaviour — but have no focused
fixture yet. No W74/W99, no UI, no calibration. Selection lags a boundary crossing
by up to one timestep; timestep sensitivity on short Connectors is not yet measured.
