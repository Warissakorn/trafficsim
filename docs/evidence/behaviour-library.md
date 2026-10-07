# M3.3.2a behaviour library and codec evidence — 2026-10-07

Branch `claude/review-all-prs-kyycls` on `main` `c79e5b6` (after PR #120). Local host:
Linux, GCC 13.3.0, CMake 3.28.3, Ninja 1.11.1, nlohmann/json 3.11.3, Qt 6.4.2.
Native Linux/Windows CI for the PR is separate. Contract:
[DRIVING_BEHAVIOUR §7](../reference/DRIVING_BEHAVIOUR.md#7-implemented-library-and-codec-m332a-d126); decision D126.

## Fixtures (`tests/behaviour_library_tests.cpp`, group `behaviourlibrary`)

The fixture owns the shipped `data/` catalogs (`putDemandCatalog`), draws two Links
joined by a Connector with one routed input, then adds a `slow` behaviour, a `heavy`
class (`heavy-vehicle`) and an `urban` behaviour type (prototype default, heavy →
slow) assigned to one Link and the Connector.

| Row | Case | Asserts |
|---|---|---|
| BA09 | `schema21_round_trips_and_legacy_files_keep_their_version` | Schema 21, keys present only where set, explicit `model`, exact reopen/re-save; without the library the same document saves schema 18 with no new key |
| BA09 | `older_schemas_and_unknown_keys_are_refused_not_dropped` | Schema 20 with each new key → `EDIT_UNSUPPORTED_FIELD`; unknown keys in schema-21 behaviours, types, classes, behaviour types, overrides rejected; `model` absent/null/`w99` → `UNSUPPORTED_BEHAVIOUR_MODEL`; schema 22 → `EDIT_VERSION` |
| BA07 | `invalid_entries_reject_load_and_edit_atomically_even_when_unused` | 12 load refusals (membership, unknown type/class/behaviour/type, blank and duplicate ids, missing default on an unused type, duplicate override, external catalog); History rejections keep JSON, revision and Undo stack |
| BA06 | `duplicate_is_independent_and_shared_edit_reaches_only_its_users` | `slow-copy`, `slow-copy-2`; edit of the original changes value and name only there; users listed for behaviour and road type; class copy has no members; Undo to the start restores JSON exactly |
| BA08 | `referenced_delete_needs_one_validated_reassignment_transaction` | Plain deletes rejected per kind; bad or self replacement rejected; reassignment rewrites both vehicle types and the default in one revision; road/class reassignment in one step; Undo restores |
| D126 | `run_refuses_assigned_roads_and_an_unassigned_library_changes_nothing` | `compileDocument` throws `UNSUPPORTED_BEHAVIOUR_ASSIGNMENT`; two selectable runtime diagnostics; an unassigned library compiles to an equal Scenario |
| — | `every_new_code_is_translated_in_english_and_thai` | 16 new codes present in both locales |

Each of seven targeted mutations fails exactly one case: removing the membership
rule, the pre-21 refusal, the Run refusal, the default requirement, the referenced
delete refusal, the member-free class copy, or the vehicle-type unknown-key check.

## Commands and results

`cmake --preset headless && cmake --build --preset headless && ctest --preset headless`:
all tests pass, including architecture, file-size and documentation guards and the
shipped-project re-save byte tests. Desktop preset with Qt 6.4.2 (offscreen): all suites pass (94 tests).
Seed-42 `trafficsim-cli --project … --stop-lines --discharge` output for
four-leg-signalised, t-junction-priority, lane-change-lab and m2.6-study-template is
byte-identical to `main` `c79e5b6`.

## Limits

No runtime selection, UI or new catalog data. Vehicle-type and behaviour display names
remain authoring metadata. BA06–BA09 have focused evidence only; BA10–BA20 remain open.
