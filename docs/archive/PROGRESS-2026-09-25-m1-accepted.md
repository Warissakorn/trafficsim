# PROGRESS archive — 2026-09-25, M1 accepted (D49, D50)

Moved out of `docs/PROGRESS.md` on 2026-09-25 as the oldest live entry. Unchanged.

## 2026-09-25 — M1 accepted by owner ruling (D49); the M2.6 template; a merge deadlock fixed (D50)

**M1.** The owner ruled M1 usability accepted on the evidence of the one attempt: an engineer who
has used another simulator models an ordinary intersection here in well under 10 minutes (D49).
ROADMAP and `M1_ACCEPTANCE.md` say "accepted by owner ruling" and keep the unshown items listed.
M0 plausibility stays open. D11's naming trigger ("end of M1") is now live.

**The M2.6 template** (owner request). `tools/m26_study_network.hpp` builds
`data/projects/m2.6-study-template.traffic.json` from the four-leg fixture: right-turn pockets, a
new `FourLegOptions::leftBypass` (6 m) so the left turn leaves the kerb lane before its head, the
owner's timing windows (cycle 120 s), one routeless `urban-mixed` input per approach over four
15-minute intervals, and a placed decision per entry Link with per-interval turning counts. **The
volumes are placeholders** (the fixture's), and there is no aerial image. It cannot be C1/C2
evidence — C1 is the owner building from blank. `m26study` pins file = builder. The default
`leftBypass = 0` leaves the four-leg fixture byte-identical.

**The deadlock (D50).** With the free left turn, the East and South kerb lanes locked within a
minute: a left turn held exactly at the exit's start had its front on the exit lane, the through
vehicle behind it stopped 2 m from the join, inside the 7 m headway, and each waited on the other.
M2.0.1's note that the rule "rarely binds" under split phasing is true only while turns wait for
their own green. Fix: the derived stop line is 1 m short of the join. `m26study` forces the old
placement and asserts the deadlock returns, so the test can fail.

**Found, not fixed:** the left turn still waits behind the through queue in the shared kerb lane —
without lane changing (M3.2.8) the bypass only helps when the queue is shorter than 6 m. The
template's left-turn delays (34–60 s) show it.
