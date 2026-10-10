# Decision index

This is navigation, not a second decision log. The complete decisions and reasoning are
in [RECORD](RECORD.md). Read the applicable rows
and their evidence before reopening a choice. IDs remain stable even when records move.

| Topic | Start with these decisions |
|---|---|
| Product scope, stack, naming and owner gates | D145, D1–D17, D34, D38, D49, D51–D53 |
| Formats, validation and diagnostics | D18a–D18d, D19a–D19d, D21, D54–D56 |
| Link/Connector geometry and lifecycle | D20–D23, D26, D55, D72–D80, D86, D106–D110, D114–D118 |
| Demand and routing | D150, D142, D25, D32–D33, D37, D42–D46, D71, D93–D94, D111–D113 |
| Signals, conflicts, Stop/Yield and queues | D147 (supersedes D36 for projects), D35–D36, D40, D47–D48, D50, D57–D69 |
| Following, commitment and lane changes | D138, D137, D136, D135, D129, D127, D126, D125, D124, D123, D122, D121, D120, D69, D71, D87–D95, D98, D101–D102, D105, D108 |
| UI, gestures, display and results | D149, D148, D146, D144, D141, D139, D134, D133, D132, D131, D130, D128, D24, D30, D39–D40, D60–D65, D81, D83–D84, D96–D97, D100, D102–D104, D116 |
| Performance, checks and documentation | D143, D140, D27–D29, D31, D70, D82, D85, D91, D99 |

## Reading a historical choice

Check its date, reasoning and failure condition, then follow any later decision that
supersedes it. For example, D15 supersedes the original D3/D4 stack, D11 supersedes
the D9/D10 names, D107 replaced D106's runtime path choice, and D114 supersedes D107 after the owner
requested interior edits to steer vehicles. A historical row is
not automatically the current contract. Current interfaces live in the reference docs.

## All decision IDs

[D1](RECORD.md#d1) · [D2](RECORD.md#d2) · [D3](RECORD.md#d3) · [D4](RECORD.md#d4) · [D5](RECORD.md#d5) · [D6](RECORD.md#d6) · [D7](RECORD.md#d7) · [D8](RECORD.md#d8) · [D9](RECORD.md#d9) · [D10](RECORD.md#d10)
[D11](RECORD.md#d11) · [D12](RECORD.md#d12) · [D13](RECORD.md#d13) · [D14](RECORD.md#d14) · [D15](RECORD.md#d15) · [D16](RECORD.md#d16) · [D17](RECORD.md#d17) · [D18a](RECORD.md#d18a) · [D18b](RECORD.md#d18b) · [D18c](RECORD.md#d18c)
[D18d](RECORD.md#d18d) · [D19a](RECORD.md#d19a) · [D19b](RECORD.md#d19b) · [D19c](RECORD.md#d19c) · [D19d](RECORD.md#d19d) · [D20](RECORD.md#d20) · [D21](RECORD.md#d21) · [D22](RECORD.md#d22) · [D23](RECORD.md#d23) · [D24](RECORD.md#d24)
[D25](RECORD.md#d25) · [D26](RECORD.md#d26) · [D27](RECORD.md#d27) · [D28](RECORD.md#d28) · [D29](RECORD.md#d29) · [D30](RECORD.md#d30) · [D31](RECORD.md#d31) · [D32](RECORD.md#d32) · [D33](RECORD.md#d33) · [D34](RECORD.md#d34)
[D35](RECORD.md#d35) · [D36](RECORD.md#d36) · [D37](RECORD.md#d37) · [D38](RECORD.md#d38) · [D39](RECORD.md#d39) · [D40](RECORD.md#d40) · [D41](RECORD.md#d41) · [D42](RECORD.md#d42) · [D43](RECORD.md#d43) · [D44](RECORD.md#d44)
[D45](RECORD.md#d45) · [D46](RECORD.md#d46) · [D47](RECORD.md#d47) · [D48](RECORD.md#d48) · [D49](RECORD.md#d49) · [D50](RECORD.md#d50) · [D51](RECORD.md#d51) · [D52](RECORD.md#d52) · [D53](RECORD.md#d53) · [D54](RECORD.md#d54)
[D55](RECORD.md#d55) · [D56](RECORD.md#d56) · [D57](RECORD.md#d57) · [D58](RECORD.md#d58) · [D59](RECORD.md#d59) · [D60](RECORD.md#d60) · [D61](RECORD.md#d61) · [D62](RECORD.md#d62) · [D63](RECORD.md#d63) · [D64](RECORD.md#d64)
[D65](RECORD.md#d65) · [D66](RECORD.md#d66) · [D67](RECORD.md#d67) · [D68](RECORD.md#d68) · [D69](RECORD.md#d69) · [D70](RECORD.md#d70) · [D71](RECORD.md#d71) · [D72](RECORD.md#d72) · [D73](RECORD.md#d73) · [D74](RECORD.md#d74)
[D75](RECORD.md#d75) · [D76](RECORD.md#d76) · [D77](RECORD.md#d77) · [D78](RECORD.md#d78) · [D79](RECORD.md#d79) · [D80](RECORD.md#d80) · [D81](RECORD.md#d81) · [D82](RECORD.md#d82) · [D83](RECORD.md#d83) · [D84](RECORD.md#d84)
[D85](RECORD.md#d85) · [D86](RECORD.md#d86) · [D87](RECORD.md#d87) · [D88](RECORD.md#d88) · [D89](RECORD.md#d89) · [D90](RECORD.md#d90) · [D91](RECORD.md#d91) · [D92](RECORD.md#d92) · [D93](RECORD.md#d93) · [D94](RECORD.md#d94)
[D95](RECORD.md#d95) · [D96](RECORD.md#d96) · [D97](RECORD.md#d97) · [D98](RECORD.md#d98) · [D99](RECORD.md#d99) · [D100](RECORD.md#d100) · [D101](RECORD.md#d101) · [D102](RECORD.md#d102) · [D103](RECORD.md#d103) · [D104](RECORD.md#d104)
[D105](RECORD.md#d105) · [D106](RECORD.md#d106) · [D107](RECORD.md#d107) · [D108](RECORD.md#d108) · [D109](RECORD.md#d109) · [D110](RECORD.md#d110) · [D111](RECORD.md#d111) · [D112](RECORD.md#d112) · [D113](RECORD.md#d113) · [D114](RECORD.md#d114) · [D115](RECORD.md#d115)

For continuous vehicle-pose work, also read [VEHICLE_POSE](../reference/VEHICLE_POSE.md) and the
2026-10-05 phase entries in PROGRESS; those entries do not invent new D-numbers.

[D116](RECORD.md#d116) · [D117](RECORD.md#d117) · [D118](RECORD.md#d118) · [D119](RECORD.md#d119)

[D120](RECORD.md#d120) — staged behavior/measurement contracts and legacy assignment seam.

[D121](RECORD.md#d121) — queue discharge observer and incomplete measurement gates.

[D122](RECORD.md#d122) — original-rank vehicle-type selection and declared CLI measurement controls.

[D123](RECORD.md#d123) — captured input provenance and shared-prefix route passage tracking.

[D124](RECORD.md#d124) — proven post-remap/source motion and unresolved source-sink type identity.

[D125](RECORD.md#d125) — rank-scoped remap invalidation and exact same-tick source-sink type replay.

[D126](RECORD.md#d126) — schema-21 project-owned behaviour library and the Run refusal of assigned roads.

[D127](RECORD.md#d127) — front-segment behaviour selection through one compiled table and `effectiveBehaviour`.

[D128](RECORD.md#d128) — staged library dialog with one History step and whole-selection road assignment.

[D129](RECORD.md#d129) — W74 contract: explicit-constant parameters, hashed driver traits outside the run stream, state re-initialised on model entry.

[D130](RECORD.md#d130) — M5 first (O1a, O3b, O8, O9c): batch, evaluation period, section delay, LOS data, Copy/Export; W74 parked.

[D131](RECORD.md#d131) — M5.2 batches: seed-ordered aggregate, conservative t table, overloaded seeds flagged and kept, formatting in `src/project/`.

[D132](RECORD.md#d132) — M5.3 evaluation period (schema 22): trips ending in the period, whole-run totals, new projects 900 s + 1 h, per-movement unfinished.

[D133](RECORD.md#d133) — M5.4 travel-time sections (schema 23): Link-wide start/end lines, interpolated crossings, delay against the vehicle's desired speed.

[D134](RECORD.md#d134) — M5.5 LOS: HCM pack as data, author-set control type (schema 24), approaches by start Link, vehicle-weighted.
[D135](RECORD.md#d135) — W74 lands first as a pure, unwired function with type bounds and D105 inside it; `Leader::acceleration` added, 0 until BA28.

[D136](RECORD.md#d136) — schema-25 `w74` codec from one key table; Run refuses a `w74` behaviour in use until BA28; dialog read-only.

[D137](RECORD.md#d137) — W74 traits hashed at generation only when a scenario holds `w74`; `W74State` and BA24/BA25 move to the composition slice.

[D138](RECORD.md#d138) — W74 runs: one `follow` dispatcher and gap accessors for every consumer, `Leader` acceleration from the snapshot, state from the kept result; D136's refusal lifted.

[D139](RECORD.md#d139) — `w74` edited in the Driving behaviours dialog: model choice, empty-able text field per key, no code defaults.

[D140](RECORD.md#d140) — Routing walks read Connector paths from one lazy `ConnectorPathTable` per expansion, not a persistent cache.

[D141](RECORD.md#d141) — Editor Run N seeds: one worker thread over value copies, the CLI's batch formatter for Export and Copy (TSV), cancelled or edited-away batches discarded.

[D142](RECORD.md#d142) — Opt-in `volumeFromCounts`: an input's volume from its entry decision's turning counts (schema 26); D46 otherwise.

[D143](RECORD.md#d143) — Owner accepted R1–R8: queue M5.9 → M4.2 → M6.0 → M5.8 after M5.7; one owner sitting; M2 closed; M3.4 and M6 carve-outs; M1.22/M1.23/M5.1 scope.

[D144](RECORD.md#d144) — Results inner pages: Movements (or the batch), Queue discharge, Safety clamps; recovered from the unmerged PR #128.

[D145](RECORD.md#d145) — Three modelling levels (Micro, Meso, Macro four-step), zones/OD, parking lots and dynamic assignment; M8–M12 after the D143 queue; PROBLEM amended.

[D146](RECORD.md#d146) — M5.9 cool-down (schema 27): run past `duration` with no new demand; the window counts trips by release; new projects 900 s; no cool-down keeps D132's bytes.

[D147](RECORD.md#d147) — M4.2 amber stop-or-go: `amberDeceleration` (catalog 3.0), continuous check, commitment at red; M0 keeps amber as red; schema 28 for an owned field.

[D148](RECORD.md#d148) — M5.8a scenario comparison: `--compare` runs two independent batches on one seed list; alternative minus base with a Welch 95 % CI; unmatched rows listed; different windows refused before any run, different seeds before any output.
[D149](RECORD.md#d149) — M5.8b editor comparison: the open project (base) against a chosen file over the Seeds field on the batch worker; refusals before any run leave the results on show; Export/Copy are `comparisonCsv`; a dirty base is named `(unsaved edits)`.
[D150](RECORD.md#d150) — M8.1 contracts: zones, zone connectors at Link ends, OD matrices expanded by fewest-object chains (schema 29); parking lots with a gate and OD-driven exits, no dwell, no RNG (schema 30); docs only.
