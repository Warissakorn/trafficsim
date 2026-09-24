# PROGRESS archive — 2026-09-23, the editor benchmark

Moved out of `PROGRESS.md` on 2026-09-24 as the oldest live entry. Verbatim.

## 2026-09-23 — The editor benchmark was measuring itself

**Request:** another optimization pass. The booked candidate was `occupiedSpans`, the largest
cost left inside a tick. The pass never got there, because the first measurement was wrong.

**What started it.** The editor harness said a frame with something selected cost 98 ms against
42 for an empty one — a 2.3× penalty nobody had looked at. Three experiments killed three
guesses: it was not `ItemIgnoresTransformations` on the lane-grip labels, not `drawLaneHandles`,
not `drawCopyPreview`. Then the fourth question — *is it the selection at all?* — measured six
identical batches in a row:

    28.0 → 54.3 → 85.0 → 108.6 → 136.3 → 158.8 ms

**Every redraw was making the next one slower**, and the "selection penalty" was only the third
batch of a climbing series. `QGraphicsScene::clear()` hands its items to an index that reclaims
them on a **deferred** update; the timing loop never pumped the event loop, so the index grew
without bound. With `QApplication::processEvents()` between frames — which is what the real
editor does, since Qt delivers mouse moves through that loop — it is flat at 11.5 ms.

**So the product was never the problem, the instrument was**, and the instrument's answer
depended on its repetition count. That makes every number M1.27.1 published wrong, including the
one in `CLAUDE.md`. Hard rule 4 is about fidelity claims; a frame-rate claim is one.

**Re-run against the fixed harness**, same interleaved medians of five, alternating a binary
built at `b2e1244` (the commit with the harness and no cache) with today's:

| intersections | links | redraw | pick |
|---|---|---|---|
| 20 | 40 | 15.73 → **2.30** (−85%) | 8.53 → **1.28** (−85%) |
| 40 | 80 | 36.78 → **4.91** (−87%) | 20.15 → **3.31** (−84%) |
| 80 | 160 | 95.07 → **10.64** (−89%) | 57.18 → **9.98** (−82%) |

**D28 pays more than it claimed, not less.** A mouse move on the 160-link corridor is 152 ms →
21, or 6.6 frames per second → 48; the entry had said 132 → 30 and 8 → 34. The `after` column is
where the artifact did its damage: with the cache a frame is cheap, so the growing index was
most of what the old harness timed. **M1.27.1's gate is met more strongly than recorded**, so
nothing reopens under rule 8 — which is the answer this had to establish before anything else.

**The correction is in the entries themselves**, not only here: the M1.27.1 table, the two
frame-rate sentences, the M1.27.3 summary line, ROADMAP M1.27 and `CLAUDE.md` all carry the
measured numbers now, each marked as corrected on this date.

### Then `occupiedSpans` stopped reading the whole route

The pass's next item. `appendSpans` (`src/core/routes.cpp`) walked **every** `RoutePart` of a
vehicle's route for every vehicle on every tick. Parts are contiguous and ordered by station, so
the ones a vehicle covers are a single run: `lower_bound` to the first part whose end is past the
rear bumper, then forward while the front has reached the next start. Both comparisons are
written exactly as the scan wrote them — same subtraction, same operand order — so the same parts
match in the same order. `occupiedSpans` also now reserves, since most vehicles sit in one
segment and the vector is rebuilt from empty every tick.

**The wall clock could not resolve it.** The box swung ±7% this session, the same size as the
effect: one round said −6.3%, the next spanned 975–1119 ms. So the judgement was made on
callgrind instruction counts, which have no noise at all.

| | 6 intersections | 24 intersections |
|---|---|---|
| `occupiedSpans` itself | 34.98M → **30.96M** (−11.5%) | 180.96M → **130.44M** (−27.9%) |
| whole program | 328.0M → **324.0M** (−1.2%) | 1930.3M → **1879.9M** (−2.6%) |

Isolated at 24 intersections: the binary search is −1.94% and the `reserve` a further −0.67%, so
the one line pays for itself.

**The estimate was 10–13% and the answer is 2.6%, which is the part worth remembering.** The
scan was not 90% waste, it was about 28% at this size: the per-element test is two comparisons,
so scanning fifty of them is not much dearer than six `lower_bound` steps — and **80% of the
fixture's vehicles are on the short entering routes** (12 × 600 veh/h) rather than the long
through route (1800 veh/h). The route length was the wrong thing to reason from; the demand mix
decides how many vehicles ever pay the long scan. The fixture was not changed to flatter the
change. The owner kept it at −2.6% because the saving is a complexity one and grows with the
network (−11.5% → −27.9% of `occupiedSpans` between the two sizes), where M2 and M5 will be.

Replay is byte-identical: the four seed fixtures and `trajectory-digest.json` pass untouched,
`trafficsim-cli 42` prints `29.249359418430977`, and the benchmark completes the same 475 trips
with the same peak of 235 vehicles.

### And a click stopped rebuilding the scene twice

`cancel()` forgets the in-flight gesture **and** repaints. Four callers then repainted again for
their own reasons, so every click on the canvas rebuilt the whole scene twice. `cancel()` is now
`resetGesture(); redraw();`, and the callers that already draw take `resetGesture()`.

The harness only timed `redraw()`, so the click had to be made measurable first — a new row that
alternates between two links, committed with its baseline before the fix. At 160 links, medians
of five: **20.51 → 9.70 ms a click, −53%**, against 10.9 ms for a single redraw. A click now
costs one rebuild, which is the floor.

**One of the four was not a double at all, and `editor-rotation-ui` said so.** `setVisibleLevel`
ends in `visibleLevelChanged` and `selectionChanged`, which tell the shell but do not draw, so
`cancel()`'s repaint was its only one; dropping it left a cancelled rotation preview on the
canvas. That site keeps `cancel()`, with a comment saying why. The test that caught it counts
preview items in the scene after each of six cancellation routes — it was written for M1.22.2 and
it earned its keep here.

### The per-tick fleet sort became a merge

Item 7 of the plan, which had been ranked "not recommended" at an estimated 1.5% and was asked
for anyway. `stepSimulation` re-sorted the whole vehicle list by id every tick. It never needed
to: the survivors arrive in the order last tick's rebuild wrote them, so the list is **two
sorted runs** — the fleet, then this tick's arrivals, appended in scheduled-time order. Sorting
the short tail and merging is linear where sorting the fleet again is `n log n`.

| | before | after | |
|---|---|---|---|
| 6 intersections | 324,044,186 Ir | 317,359,614 Ir | −2.06% |
| 24 intersections | 1,879,880,181 Ir | 1,847,057,272 Ir | −1.75% |

The sort itself was 2.12% of the program at 24 intersections and is now about 0.03%; the rest of
what it cost went into the `is_sorted` check, which is the honest way to do this. **A hand-built
initial state need not be ordered** — `test::withVehicles` places vehicles in the caller's order
— so the prefix is checked rather than assumed, and an unsorted one falls back to the full sort.
Trusting `state.tick != 0` instead would have saved another 0.35% and bought a trap.

Ids are unique, so id order is a total order and the merge produces exactly what `std::sort`
produced. Replay is byte-identical: the four seed fixtures and `trajectory-digest.json` pass
untouched, `trafficsim-cli 42` prints `29.249359418430977`, and the benchmark completes the same
475 trips with the same peak of 235.

**It does not scale up, and that is worth writing down.** The saving is slightly *smaller* at 24
intersections than at 6, because the sort's share of a tick shrinks as the rest of the tick grows
— the opposite of the `occupiedSpans` change, which was kept for exactly that scaling property.
This one is worth its eight lines because it also states the invariant the code already relied
on; it is not worth revisiting for more.

### And item 6 was built, measured and reverted

`redraw()` copies every `Link` by value so that one of them — the primary — can carry a drag or
lane-resize preview. Copying only that one, and drawing the rest straight out of the document,
removes 159 Link copies a frame at 160 links.

| | before | after | |
|---|---|---|---|
| `redraw()` inclusive | 200,879,176 Ir | 198,519,361 Ir | −1.17% |
| whole program | 551,679,517 Ir | 548,798,736 Ir | −0.52% |
| wall, 160 links, median of 7 | 11.28 ms | 10.91 ms | −3.3%, inside a ±10% spread |

**Reverted.** A frame is `QGraphicsItem` construction — thousands of items against 159 struct
copies — so the ratio does not improve at any network size, and unlike item 3 there is no
scaling argument to keep it on. It also read worse: a scratch `Link` and a reference-selecting
ternary in place of one loop variable. The number is recorded so nobody measures it twice; what
is actually left in a frame is M1.23's culling and LOD.

### And the standing orders stopped reading a 14,800-token file to find twenty lines

The pass's last item, and the only one that is not about the program. `CLAUDE.md` said *read the
**Next** section of `docs/PROGRESS.md`* — but a section is not a file, so every session paid for
the whole log to reach it.

**`docs/NEXT.md` is now the one live to-do**: the thread of work in progress, and the owner's
standing items, which were a second `## Next` further down the same file. `PROGRESS.md` keeps
what it is good at — the history, the reasoning, and the decision log every `D`-number points
at — and is read for the entry behind whatever is being changed, not as a matter of course.

| what a session reads at start | before | after |
|---|---|---|
| `CLAUDE.md` | 3,180 | 3,320 |
| `docs/ARCHITECTURE.md` | 3,271 | 3,273 |
| `docs/PROGRESS.md` → `docs/NEXT.md` | 14,553 | **1,914** |
| **total** | **21,004** | **8,507** (−59%) |

Token counts are the audit script's character-based estimate, before and after with the same
script. `CLAUDE.md` grew by 140: the new row and the rule that stops the duplication coming back.

**The rule matters more than the move.** Two places saying what to do next is exactly what hard
rule 3 forbids, and the copy that rots is always the one in the log — so `NEXT.md` is rewritten
each session and a new entry carries no `Next` of its own. Entries written before today keep
theirs, as history.

**The audit's 79 "broken references" were mostly not broken, and saying so is the point.**
`nlohmann/json.hpp`, `id/lane-2` and `chord/3` are not paths; `MIGRATION.md`'s TypeScript paths
name a codebase that is deliberately in Git history only; `docs/specs/` names files its
proposals would create; and `ARCHITECTURE.md`'s `src/render/` is a correct past-tense sentence
about a directory M1.24 removed. **Five were real** and are fixed: three paths missing their
`src/` prefix, and two markdown links inside `docs/archive/` written as if from `docs/`, which
404ed — including one a file used to point at itself. The count reads 79 → 74; reporting it as
79 → 0 would have meant breaking four dozen correct sentences to satisfy a script.
