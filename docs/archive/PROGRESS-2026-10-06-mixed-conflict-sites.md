## 2026-10-06 — Mixed conflict sites and P3–P4 continuation (D118)

Owner authorized grouping all three kinds for a connected owner-pair site, 0.5 m per-side
rail offsets and directional continuation through P3–P4. Groups now retain their full kind
list; Crossing/Merge share controls by owner ID, Branching stays derived/read-only. The
mixed table/dialog identifies it separately from editable parameters. Direct group commands
stage and validate the candidate and reject newly introduced merge-order cycles atomically;
History still publishes one Undo step. Merge takeover preserves complete topology controls.
Offsets are normal to rails, capped at 20% of normal local width for narrow/tapered lanes.
Shared band outlines feed paint/picking. Valid mouth caps are clipped to their attached Link
lanes; cap pieces support physical grouping and finite Link stations support directional
continuation. Caps can lie on either side of a join, so no source/target station-side clamp
is imposed. Stored entryStation–exitStation, schema, runtime paths and solver remain unchanged.
M3.2.4f is carved separately; physical-mouth admission remains M3.2.4e. Regression coverage
adds both traffic sides, oblique offsets, mixed real sites, atomic rejection, geometry Undo,
and Qt display/picking/one-Undo checks. Oldest roadmap session block moved whole to archive.
Validation: GCC 13.3/C++20 and Qt 6.4.2 on Linux; full local desktop CTest passes
**89/89 groups**, including Qt offscreen UI, frozen references and repository guards.
Native Linux/Windows CI and owner appearance remain separate; no owner/fidelity gate closes.
