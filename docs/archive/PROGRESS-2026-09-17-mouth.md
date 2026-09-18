# PROGRESS archive — 2026-09-17, mouth geometry

Moved whole out of [`../PROGRESS.md`](../PROGRESS.md) on 2026-09-18 under that file's own rule:
append-only, never delete, move old blocks whole into `docs/archive/` when the file gets long.
These two entries are the reasoning behind M1.17's wedge mouth and the M1.12.2 miter
investigation. M1.17 was later reverted and then superseded by M1.18; do not re-derive either.

## 2026-09-17 — The sharp point at a Connector's mouth was the re-miter, not the wedge

**The owner circled a Connector's mouth on our own render: it narrowed to a point where it met the
Link.** Three readings were measured and two of them were wrong, which is the only reason the third
was found.

| reading | what the measurement said |
|---|---|
| the wedge cut runs a tongue across the Link | no — mouth exactly 3.500 m at every arrival 10°–170°, at a Link end and on a Link body alike |
| the 2→1 lane taper closes to a point at the mouth | it does, but the owner confirmed the Connector is **2 lanes → 2 lanes** |
| **the mouth re-miter overshoots** | **yes: 5.59, 9.41, 14.31 and 18.11 m of mouth on a 7.00 m Connector** |

**What the re-miter does and why it ran away.** Where the fixed-distance cut would fold, every
boundary extends its own last leg to meet the Link's cross-section line instead. At a strongly
oblique arrival that line lies near the ribbon's own axis, so the intersection lands many lane
widths out. The bound added in `1f2f014` allowed 1.5 times the mouth's own width of overshoot,
which is nowhere near tight enough. Past about 9 m the two outer boundaries cross each other, and
the surface is filled from a closed ring, so `trimSelfIntersections` closed that fold into a point.
**The spike was the fill trim doing its job on a shape that should never have been handed to it.**

**The bound is now the mouth itself, not a distance to pick.** The fixed-distance cut puts boundary
i exactly on the Link's lane edge, so the mouth's span across the road is the lane widths and
nothing else; a re-miter may only redistribute corners inside that span. Three shapes are tried in
order of how much each is the Link's own cross-section — the fixed-distance cut, then the bounded
re-miter, then the un-cut end square to the Connector — and **the first that does not fold is the
mouth**, taken whole.

**The square end had to come back as the last resort, and the measurement is why.** Bounding the
re-miter alone left the fixed-distance cut folding on its own: the ring trim ate up to **7.67 m** of
mouth. Past roughly 50° off the cross-section a ribbon cannot be cut on a line that near its own
axis without folding, whichever corner placement is used. The 0.12–0.29 m step a square end leaves
against the road is the step Vissim's own screenshot of this joint shows, and the owner's Vissim
reference for it is parallel-sided and stops at the attachment.

**Nothing an ordinary joint draws moved.** Every Link-end attachment and every near-tangential
merge still takes the fixed-distance cut, bit for bit: all 126 existing unit tests passed unchanged
at every step, including the ones pinning the mouth to the Link's lane edges at 1e-9.

**Verification.** 16/16 CTest, 127/127 unit tests, architecture and size guards green. Over 12 Link
headings × 24 arrival headings at a body attachment: every mouth lane exactly 3.500 m to 1e-9, no
ring self-intersection anywhere, and 0.0000 m of mouth lost to the trim — against 18.11 m of mouth
and 7.67 m lost before. The new test fails on the old code with `10.452885 vs 3.500000`.

**Left alone deliberately:** a Connector whose two ends carry different lane counts still closes its
surplus lane at the mouth. The owner was asked whether that taper should move into the middle of the
body and answered to keep it as it is.

**The lesson, again.** The first two readings were built from the render and from the record, and
both were plausible. Only the third survived a measurement. `docs/VISSIM_PARITY.md` already carried
*"ask for a picture of Vissim before reasoning about it"*; the other half of it is **ask for a
number before believing the picture.**

---

## 2026-09-17 — M1.12.2: the miter "bulge" was a measurement, not a defect

**I was about to fix something that was not broken, and measuring first is the only reason I did
not.** The record said a 2→2 Connector through a sharp bend "bulges to 8.698 m of a 7.000 m width,
24% over ... the miter blowing out where the polygon turns hard". I built the case and measured it
three ways, on 90.47° of deflection:

| how the width is measured | reading |
|---|---|
| along the cross-section, at the mitered vertex | **9.9403 m** (+42%) |
| perpendicular, point to the far polyline | 7.0425 m (+0.6%) |
| **projected across the leg the vertex lies on** | **7.000000 m** (exact) |

The first is `width / cos(φ/2)` — the corner-to-corner diagonal of a correctly mitered joint,
which is *what the intersection of two offset legs is*, and what a road painted round a kink
actually measures across its corner. The original 8.698 m is the same identity at a gentler bend
(`7.000 / cos(36.4°)`). The carriageway square to the road never moved.

**The two tests I suspected were both right, and one of them already said so.** I had flagged
`bends_keep_their_full_carriageway_width` for asserting `3.5*sqrt(2)` at a right-angle corner, and
the `8e-2` tolerance in the connector tests. Reading them properly: the first asserts the
carriageway is **10.5 m projected across each leg** *and* `3.5/cos(45°)` between adjacent
boundaries at the vertex — both halves, deliberately. The second's comment states the distinction
outright: "along the cross-section a mitered corner reads wide … square to the road it is the lane
width". A previous session had already worked this out and written it down; the M1.18-era note
calling it "a real defect" was a mis-diagnosis of the same numbers.

**So `offsetGeometry` was not touched.** `network_tests` pins the miter to 1e-9, and removing it
would reinstate the pinch it exists to fix — 18% at 63°, 30% at a right angle. Fixing this
"defect" would have broken every bend in the editor.

**What was genuinely missing is now there.** Width had only ever been bounded from **below**
(`least > .9*3.5`), which is how a claim of 24% over stood unchallenged for a session.
`a_bent_connector_holds_its_width_square_to_the_road_from_both_sides` now asserts it **exactly**,
to 1e-9, on every interior leg of a hard bend, with the along-cross-section reading asserted first
as the forcing so the test cannot pass on a gentle curve. M1.12.1's curved bound went from
`span < 5.7` to the same equality. Both catch a 0.1% width error, checked by inflating the width
in `connectorLaneWidths`.

**Two things measurement corrected mid-flight.** My first forcing assertion required
`minimumRadius < width/2`; the fixture's radius is 17.9 m, so the assertion was simply false — it
is now the deflection angle, which is the property that actually produces the wide reading. And the
exact width was 5.8 mm over until I excluded legs touching either end: those run to a vertex the
**wedge mouth** moved (M1.17), so their direction is the Link's cross-section, not the Connector's.

**`tests/connector_tests.cpp` passed the 500-line guard**, so it split on the seam it already had:
topology (creation, references, retargeting, deletion, history) stays, and shape (width, markings,
bend radius, the mouth) moved to `tests/connector_shape_tests.cpp` with the measuring helpers that
serve it. Test names diffed against `git show HEAD:` — none lost, none duplicated, and no test's
own code changed; the only body differences are the relocated helper block and the new comment.

**The lesson worth keeping:** a distance between two boundaries is a *width* only when it is
measured square to the road. `perpendicular()` carried that warning in its own comment; the 24%
figure was taken with `apart()`, which does not.

**Verification:** 23/23 CTest, 126/126 unit tests, architecture and size guards green. No source
file in `src/` changed at all — this milestone closed on a measurement and a test.

---
