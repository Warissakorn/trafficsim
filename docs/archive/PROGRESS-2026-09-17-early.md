# PROGRESS archive — 2026-09-17, earlier entries

The 2026-09-17 archive passed the 500-line budget (hard rule 6), so its three oldest entries
moved here whole: the wedge mouth, the `NETWORK_EDITOR.md` section split and the `ROADMAP.md`
reordering. Nothing is edited or summarised — only relocated, the same rule
[`PROGRESS.md`](../PROGRESS.md) sets for itself. Newest first, continuing
[`PROGRESS-2026-09-17.md`](PROGRESS-2026-09-17.md), which holds the later 2026-09-17 entries.

### 2026-09-17 — The mouth is a wedge cut on the Link

The owner circled the joint on a Vissim screenshot — a Connector arriving on a Link **body** at
an angle — and confirmed what to match: *ปากทางเป็นลิ่มตาม Link*. Vissim cuts a Connector's mouth
on the cross-section of the Link it attaches to. Ours was square to the Connector.

**The commit that made it square was justified with the wrong numbers.** `e6dd394` cited 1.06 m
of a 3.50 m lane on a reverse curve, 1.96 m at 60 degrees and 0.46 m at 90. Those belong to a
different defect — **interpolating** the cross-section through the body — fixed one commit
earlier in `e81a591`, whose own message says of the end cut: *"only the joint ... is shorter
through the corner, as it is in Vissim."* So the wedge had already been judged correct, and
`e6dd394` discarded it along with the interpolation, trading it for an overlap of 0.12-0.29 m.

The restoration is the pre-`e6dd394` projection, six lines: each boundary's end is placed at
`spine.front() + from · offset`, where `from` is the Link's own cross-section direction, which
`endCross` was still computing and throwing all of away but its sign.

**A design I proposed first was wrong, and measurement said so.** I planned to *shear* each
boundary's end along its own direction until it met the cross-section line. That lands on the
line but not on the Link's lane edges: the mouth comes out `offset / sin θ` wide. Measured, it
drew a 7.00 m mouth as 7.24 m at 30 degrees and 7.60 m at 120. The projection is both simpler and
correct — measured 7.0000 m at every angle.

**The guard that matters.** The cut must not creep into the body, or it rebuilds the very thing
`e6dd394` removed. Dumped every boundary vertex across six fixtures, before and after: 90
vertices, **36 changed, and all 36 are the two end samples — zero interior vertices moved**. The
body test was tightened from `> 3.4 m` to `= 3.5 m at 1e-9` to hold that line, measured against
the opposite edge's *body*, since a wedge segment is not a lane edge and measuring across one
reads 1.3 cm short without the lane being short.

**The cap I was advised to add is a no-op, so it is not there.** The concern was a wedge deeper
than its own opening leg folding over and feeding `trimSelfIntersections`. Built it: a 14 m range
whose first leg is 2.07 m against a 7.00 m half-width — a wedge three times deeper than its
opening. Zero self-crossings, and the trim never touched the mouth. Recorded rather than coded.

Mouth-to-lane-edge went from 4.7-17.2 cm, 0.12-0.29 m and 0.88 m to **0, to 1e-9**, and the three
tests that asserted squareness now assert exact landing instead — tolerances replaced by
equalities, not relaxed. Three negative checks each broke named tests: no cut at all, cutting
every sample rather than the two ends, and dropping the sign so lane order mirrors at the mouth.

One behaviour outside drawing: `canvas_spatial.cpp` builds the selection and hit-test outline from
these boundaries, so clicking a Connector at its mouth now matches what is drawn.

Booked as M1.17.

---

### 2026-09-17 — NETWORK_EDITOR.md: a section whose title covered a third of the file

`## Connector lane ranges` ran **159 lines, 37% of the manual**, and most of it was not about
connector lane ranges. Under that one heading sat Link lane-tab dragging, lane-edge mitering,
road surfaces and markings, geometry handles, end-handle re-attachment, the connector polyline
and its intermediate points, the wedge mouth, the cubic reach, attachment stations, Link splits,
the runtime limit, and deletion cascades. A reader looking for how markings are drawn had no
reason to open a section named after lane ranges, and would not have found it from the contents.

Split into five sections named for what each one holds:

| Section | Lines |
|---|---|
| Connector lane ranges | 43 |
| Lane edges, road surfaces and markings | 18 |
| Geometry and end handles | 29 |
| Connector shape: intermediate points and the mouth | 50 |
| Attachment stations, Link edits and deletion | 27 |

**Not a rewrite.** Only four heading lines and their blank lines were inserted; the prose was
checked byte-for-byte against `git show HEAD:` with those eight lines stripped back out, and it
is **identical**. The split points fall on existing paragraph breaks, so reading order is
unchanged — what changed is that the contents now tells the truth about where things are.
433 to 441 lines, the whole cost being the headings. No anchor link anywhere in the repo
pointed into this file, so no link broke; the sweep confirms none dangling.

**Still worth a later pass, deliberately not done here:** the paragraph now opening *Geometry
and end handles* is 22 lines and mixes the re-attachment gesture with how a Connector's
cross-section is built — two subjects in one block. Splitting it means rewriting sentences, not
moving lines, which is a content change and belongs in its own session with the behaviour in
front of it.

**Verification:** prose identical to HEAD, no dangling links, size guard green, 23/23 CTest.
No source file touched.

---

### 2026-09-17 — ROADMAP.md put back in sequence, and two stale status lines fixed

`ROADMAP.md` opens by calling itself "**a sequence**, so that any session can see where it sits".
It was not one. The M1 sub-milestones ran in the order they were *written*, so the carve-outs
made under rule 2 had piled up wherever the session that carved them happened to stop:

```
before: ... M1.10 M1.11 M1.12 M1.11.1 M1.13 M1.14 M1.15 M1.16 M1.17 M1.12.1
after:  ... M1.10 M1.11 M1.11.1 M1.12 M1.12.1 M1.13 M1.14 M1.15 M1.16 M1.17
```

Both misplaced entries are the two that are **still open**, which is the worst possible thing to
bury: M1.11.1 sat behind a done M1.12, and M1.12.1 sat last in the file behind five done
milestones, reading like the newest work rather than unstarted work. A carve-out is now filed at
its number, and the M1 preamble says so, so the next one lands in the right place.

**Two status lines were also simply wrong.** `ROADMAP.md` said "M1.1–M1.10 implementation is
available" and `CLAUDE.md` said M1 "covers M1.1–M1.10 ... M1.11 adds body attachments; M1.12
fixes both lane edges" — both written before M1.13–M1.17 shipped and never updated. A session
starting from either would have believed five milestones of work did not exist. Both now name
M1.1–M1.17 and, separately, the two open carve-outs. Note these are *not* a duplicated status
table: each milestone's own state still lives only in its own section, and the preamble names
which are open without restating why.

**Verification:** all 21 M1 sections diffed body-for-body against `git show HEAD:` —
`lost: set()`, `gained: set()`, `bodies differing: none`; everything outside the M1 block
byte-identical; line count unchanged at 434 by the reorder, 440 after the corrected preamble;
separator count unchanged at 14. 23/23 CTest, guards green. No source file touched.

A first attempt scored two false differences here, because my checker split the file only on
`### M1` and so let M2-M7 attach to whichever section came last. The file was fine; the check
was wrong. Bound a section comparison at both ends, not just the start.

---
