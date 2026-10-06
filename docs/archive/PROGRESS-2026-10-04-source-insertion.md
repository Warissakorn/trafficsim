## 2026-10-04 — Source entries wait until their first step fits (D108)

NEXT's M3.2.8a.1 source slice: ordinary following/integration from rest must fit the
snapshot leader clearance beyond the unchanged standstill buffer before entry. Otherwise
keep the same sampled vehicle pending; no departure/clamp or RNG draw. Equality admits,
including a zero step at the standstill boundary. Moving merge anticipation stays open.
Contract/tests/input hashes were committed in 6375ad2 before implementation and seed
outputs. Two source regressions fail before and pass after; three tests cover held queue,
release, copied replay/accounting and exact bounds. Linux Release check passes 54/54,
with frozen baselines unchanged. D107 baseline: 120 stress runs pass replay/body/swept/
braking checks; source clamps 3 → 0, moving minor 21 → 21, all clamps 226 → 223, all drain.
Four projects × 40 seeds preserve counts/delay; two reports remain identical. Lab/M2.6
clamps fall 19 → 1 / 858 → 837; max travel-time difference 0.002857143 s. Detailed
[evidence](../evidence/source-first-step.md) records limits. M3/M6 owner gates remain open.

---

