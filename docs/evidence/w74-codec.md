# M3.3.3a W74 schema-25 codec evidence — 2026-10-08

Branch `claude/dazzling-gates-df64jc` on `1d72ed1` (after D135). Local host: Linux, GCC 13.3.0,
CMake 3.28.3, Ninja 1.11.1, nlohmann/json 3.11.3, Qt 6.4.2 (offscreen). `--preset headless`
67/67 and `--preset desktop` 98/98 CTest. Windows evidence belongs to the PR's native CI.
Contract: [W74](../reference/W74.md) §5, §9; decision D136.

## Rows (`tests/w74_codec_tests.cpp`, ctest `w74codec`)

| Row | Case | Asserts |
|---|---|---|
| BA29 | Owned `w74` behaviour saved | `schemaVersion` 25; `model: "w74"`, its name, all 18 keys and optional cooperative braking; no prototype keys; other behaviours stay `prototype`; parse → equal document and identical bytes |
| BA29 | Same project without it | Plain file (schema 18) byte-identical |
| BA29 | Each of the 18 keys missing, then as text | `INVALID_BEHAVIOUR_PARAMETER` at `behaviours[i].<key>` |
| BA29 | One value just outside each range kind (12 keys) on an unused behaviour | `INVALID_BEHAVIOUR_PARAMETER` with path; each boundary value itself loads |
| BA29 | `followingTime`, `standstillDistance`, `discretionaryLaneChangeThreshold` on `w74`; `ax` on a prototype; unknown `cc0` | `EDIT_UNSUPPORTED_FIELD` with path |
| BA29 | `model: "w99"`; `w74` in schemas 21 and 24; schema 26 | `UNSUPPORTED_BEHAVIOUR_MODEL` with path; `EDIT_VERSION` |
| §9 | External catalog entry with `model: "w74"`, without `model`, and `"prototype"` | `w74` parameters read; the other two are the prototype |
| D136 | Unused `w74`; a vehicle type using it; a road behaviour type defaulting to it | Unused: seed-42 run vehicles and time equal the plain project's. Used: save/load works, `compileDocument` throws `UNSUPPORTED_BEHAVIOUR_MODEL_RUN` |

`behaviour-library-ui` adds: a `w74` behaviour's edit dialog shows the read-only note and
no prototype spin boxes; a rename commits; its values are unchanged; the file stays schema 25.
`behaviourlibrary`'s future-schema probe moved to 26 (25 now exists; 22–24 are M5's). Renumbered from schema 22 on 2026-10-09 when W74 was replayed on top of M5.

Seeded mutations each fail a case: dropping the below-25 `w74` refusal, the Run refusal,
its road-selection half, the prototype-key exclusivity, writing 21 instead of 22, a wrong
`bMinAdd` range, and skipping the dialog's `w74` branch.

## Byte guards

`trafficsim-cli 42` output is byte-identical to `48a3276`. The four shipped
`data/projects/*.traffic.json` parse and re-save to equal JSON at schema 17 (one-off local
check, not committed). Frozen TS baselines pass unchanged.

## Not shown

No W74 run, traits, state or model switch (BA23–BA28); no preset; no parameter editing UI.
No calibration or Vissim/SUMO equivalence; the not-yet-validated marker stays.
