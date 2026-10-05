# Supplied target specifications

The owner supplied these three Thai Markdown documents on 2026-09-20 and requested
an audit, implementation, then inclusion in the repository. They are retained as source
material; maintained engineering documentation is in English. Link and Connector remain
verbatim. Network Editor includes the owner's explicit 2026-09-24 removal of two external
simulator references (D38); see [the audit](../SPEC_AUDIT.md) and
[the edit commit](https://github.com/Warissakorn/trafficsim/commit/f2dce3c59e37cd33dcf8b4d823e20b21c4437f45).

These are **requirements/proposals, not implementation or validation claims**. Some statements
conflict with each other or with existing contracts. Read [the audit](../SPEC_AUDIT.md) and
[the delivery roadmap](../ROADMAP.md) before applying them. In particular, a heading that says
“VISSIM-compatible” does not establish scientific or product parity.

Files are split at section boundaries to respect the repository's 500-line limit.
Concatenating each document's parts in order reproduces its retained bytes, including line
endings. [manifest.json](manifest.json) keeps the original names, lengths and SHA-256 digests.
For the authorized Network Editor edit it also records `retained_bytes`, `retained_sha256`
and `modification_commit`; use those retained fields when verifying today's copy.
Do not rewrite supplied text or reset the authorized edit during documentation maintenance.

| Document | Ordered source parts |
|---|---|
| connector | [Part 1](connector/part-01.md), [Part 2](connector/part-02.md) |
| link | [Part 1](link/part-01.md), [Part 2](link/part-02.md), [Part 3](link/part-03.md), [Part 4](link/part-04.md) |
| network-editor | [Part 1](network-editor/part-01.md), [Part 2](network-editor/part-02.md), [Part 3](network-editor/part-03.md), [Part 4](network-editor/part-04.md) |

## Implemented format

TrafficSim schema 7 adds shared Link boundary markings and two additional marking kinds.
It does not implement all the fields in the supplied schema-7 examples. Unsupported network
object fields in schema 7 are rejected before replacing the open document. See
[AUTHORING_EXTENSIONS.md](../AUTHORING_EXTENSIONS.md) for the actual contract.
