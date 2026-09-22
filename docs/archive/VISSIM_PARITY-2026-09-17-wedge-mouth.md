# VISSIM PARITY — archived section

Moved out of `docs/VISSIM_PARITY.md` on 2026-09-22 (M1.27.3) to keep that file inside the
500-line limit. It is the oldest of the dated follow-ups and the most superseded: M1.17's wedge
was reverted, M1.18 cut the mouth flush, and M1.19 settled the lane middles. Kept whole because
it records what the owner sent and why the first reading of it was wrong.

---

## 2026-09-17 — The mouth is a wedge, and the square cut was a misreading

The owner circled the joint on a Vissim screenshot: a Connector arriving on a Link **body** at an
angle, its mouth cut on the Link's cross-section. Ours was square to the Connector, from
`e6dd394`. Restored to the cut, which is what `e81a591` — the commit immediately before it — had
already judged Vissim-correct: *"only the joint, where the Connector arrives across the lane and
is cut on that lane's cross-section, is shorter through the corner, as it is in Vissim."*

`e6dd394` re-quoted the **interpolation's** numbers as if they were the end cut's, and traded a
Vissim-correct wedge for a non-Vissim overlap of 0.12-0.29 m. Measured after the restoration:
every mouth lands on its Link's lane edges to 1e-9, the mouth width along the cross-section is
exactly the Link's lane width at 30/60/90/120 degrees, and every interior boundary vertex is
unchanged bit for bit.

This is the second time in two days that a screenshot of the real thing overturned a reading of
Vissim taken from our own geometry — the first was the spline. The lesson is on the record:
**when a shape is meant to match Vissim, ask for a picture of Vissim before reasoning about it.**
