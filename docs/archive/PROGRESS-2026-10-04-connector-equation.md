## 2026-10-04 — Direct Connector equation for vehicle motion (D107)

Owner clarified: use the existing Bézier equation without PolyPoint driving segments.
Runtime positions/tangents now evaluate that equation per mapped lane pair; lengths
integrate |B'(t)| and distance inversion is bracketed. D106 midpoint guides remain drawing
only. Point counts/interior drags cannot change the equation; retained controls map to its
arc stations. Input metadata was registered in local 6318fe2/66282d7 before seed studies.
Linux Release: 54/54 groups, including analytic tests and 240 curve/add/drop runs. Four
projects × 40 seeds preserve counts but change timing; 120 stress runs pass safety checks,
with total clamps 225 → 226 (minor 22 → 24). Frozen baselines unchanged. Methods, results,
compatibility limits and pending Qt/owner review: [evidence](../evidence/connector-equation.md).

---

