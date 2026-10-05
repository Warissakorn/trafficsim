# Decision index

This is navigation, not a second decision log. The complete decisions and reasoning are
currently in [PROGRESS — Decisions](../PROGRESS.md#decisions). Read the applicable rows
and their evidence before reopening a choice. IDs remain stable even when records move.

| Topic | Start with these decisions |
|---|---|
| Product scope, stack, naming and owner gates | D1–D17, D34, D38, D49, D51–D53 |
| Formats, validation and diagnostics | D18a–D18d, D19a–D19d, D21, D54–D56 |
| Link/Connector geometry and lifecycle | D20–D23, D26, D55, D72–D80, D86, D106–D110 |
| Demand and routing | D25, D32–D33, D37, D42–D46, D71, D93–D94, D111–D113 |
| Signals, conflicts, Stop/Yield and queues | D35–D36, D40, D47–D48, D50, D57–D69 |
| Following, commitment and lane changes | D69, D71, D87–D95, D98, D101–D102, D105, D108 |
| UI, gestures, display and results | D24, D30, D39–D40, D60–D65, D81, D83–D84, D96–D97, D100, D102–D104 |
| Performance, checks and documentation | D27–D29, D31, D70, D82, D85, D91, D99 |

## Reading a historical choice

Check its date, reasoning and failure condition, then follow any later decision that
supersedes it. For example, D15 supersedes the original D3/D4 stack, D11 supersedes
the D9/D10 names, and D107 replaces D106's runtime path choice. A historical row is
not automatically the current contract. Current interfaces live in the reference docs.

For continuous vehicle-pose work, also read [VEHICLE_POSE](../VEHICLE_POSE.md) and the
2026-10-05 phase entries in PROGRESS; those entries do not invent new D-numbers.
