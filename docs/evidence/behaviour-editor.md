# M3.3.2c behaviour library editor evidence — 2026-10-07

Branch `claude/review-all-prs-kyycls` on `main` `bcfe4ff` (after PR #122). Local host:
Linux, GCC 13.3.0, CMake 3.28.3, Ninja 1.11.1, nlohmann/json 3.11.3, Qt 6.4.2 (offscreen
platform). Windows UI jobs run in the PR's native CI. Contract:
[DRIVING_BEHAVIOUR §9](../reference/DRIVING_BEHAVIOUR.md#9-implemented-editor-m332c-d128); decision D128.

## Automated UI fixture (`tests/behaviour_library_ui_tests.cpp`, ctest `behaviour-library-ui`)

A project with two Links joined by a Connector, a route and an input, using the
installed catalogs, opened in the real `EditorWindow`:

| Row | Step | Asserts |
|---|---|---|
| BA19 | Build a library (duplicate, edit copy, add class, add type with override), then Cancel | Document JSON unchanged; catalogs stay external |
| BA19 | Confirm without an edit | No revision, JSON unchanged |
| BA19 | Set `followingTime` 0, Confirm | Dialog stays open with a reason; no revision |
| BA19 | Build the library, Confirm | One revision: catalogs captured, copy named "Slow", class, type and override present; the editor showed users first; one Undo returns to the original JSON, Redo restores |
| BA19 | Select two Links and the Connector, apply the type | One revision assigns all three; one Undo clears them |
| BA19 | Primary road selected | Effective list shows `heavy-vehicle: …-copy · class override` and a `behaviour type default` row |
| BA19 | Delete the referenced copy | Cancelling the replacement keeps it; choosing the default removes it and rewrites the override in one step; Undo restores |
| BA20 | Switch to Thai | New actions, the inherit option, the effective list and the dialog have Thai text |
| BA20 | Save and reopen | JSON equal |

Stable over six consecutive runs. Seeded UI mutations each fail it: committing a
no-edit Confirm, assigning only the primary road, skipping the Confirm dry-run, and
editing the live document instead of a staged copy.

`behaviourlibrary` adds `effective_road_behaviours_report_value_and_source` (inherited,
type default, class override, unknown type refused, and agreement with
`compileBehaviourAssignments`) and an EN/TH key check for the 21 new editor keys.

## Commands and results

`ctest --preset headless`: 65/65. `QT_QPA_PLATFORM=offscreen ctest --preset desktop`:
96/96, including every existing UI suite. Seed-42 CLI output for the four shipped
projects is byte-identical to the M3.3.2b build (the compiler refactor is behaviour-free).

## Limits

Automated Linux evidence only; Windows results belong to the PR, and the owner's visual
review of layout, wording and Thai text is a separate gate (BA20). No new runtime
behaviour, no W74/W99, no calibration.
