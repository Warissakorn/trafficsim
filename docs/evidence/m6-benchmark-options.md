# M6 benchmark option sheet (M6.0, 2026-10-10)

Prepared by a session for Q4, open since 2026-09-10: *which published benchmarks define the M6
tolerance?* ([D143](../decisions/RECORD.md#d143), R2 (b); the 2026-10-03 review's
[O2 and S0](../archive/ROADMAP-review-2026-10-03.md)). The owner answers by letter in the
[owner sitting](../plans/OWNER_SITTING.md) B9. Nothing here is decided until then.

## 1. Rules of this sheet

- **No engine numbers.** No TrafficSim figure was produced or looked at for this sheet, and none
  appears in it. Every number below is a published value or an input a comparison would set.
  Choosing a benchmark the engine already happens to match would undo the pre-registration, as
  D34's blind C0 was meant to prevent.
- **No tolerance.** The tolerance is the owner's next decision. It is chosen after the options
  and before the first comparable run, then published in the docs and shown in the app (ROADMAP
  M6).
- **Published sources only.** At least two per benchmark kind. A comparison with another
  simulator is not an option (D38).
- **Not validation.** This sheet validates nothing. Every results screen keeps the
  not-yet-validated marker until M6 passes.

The three kinds come from the M6 done-when: capacity and delay for a signalised approach, and
gap-acceptance capacity for an unsignalised minor movement. Kind U includes calibrated gap
acceptance, which moved to M6 by R7.

| Kind | Quantity |
|---|---|
| **S1** | Signalised approach: saturation flow and capacity |
| **S2** | Signalised approach: delay |
| **U** | Unsignalised minor movement: capacity against conflicting flow, with its critical and follow-up headways |

## 2. What TrafficSim can measure today

These are the commands a comparison would run. This sheet runs none of them. `F` is a project
built for the benchmark's conditions, and the seeds follow D88 (at least 40, 42–81).

| Figure | Command | Definition | Gaps every comparison must state |
|---|---|---|---|
| Queue discharge headway, so saturation flow `3600 / h` | `trafficsim-cli --project F --discharge --discharge-warmup W`, optionally `--discharge-type car` and `--discharge-steady-first/last` ([DISCHARGE](../reference/DISCHARGE.md), D121) | Per lane and cycle: the gaps between front crossings of the stop line by vehicles queued at green, ranks 3–5 by default; startup estimates for ranks 1–2 | veh/h, not pc/h, with no PCU conversion. Crossings are stamped at the end of a tick, so each gap carries up to one time step of quantisation. Initial and partial greens are unavailable. One run per seed: there is no `--seeds` form |
| Section delay | `trafficsim-cli --project F --seeds 42-81 --csv out.csv`, with a travel-time section on the approach ([TRAVEL_TIME_SECTIONS](../reference/TRAVEL_TIME_SECTIONS.md), [BATCH](../reference/BATCH.md)) | Per trip, `max(0, travel time − free-flow time)` between two section lines, with the free-flow time at the vehicle's own desired speed. Crossings are interpolated within a tick. Mean, SD and 95 % CI over seeds | Not HCM control delay. It equals the travel-time definition of control delay only when the section starts upstream of the longest queue and ends past the acceleration zone. The `max(0, ·)` drops the negative values a signed definition would average. Free flow is each driver's desired speed, not a stated free-flow speed, so the figure also counts delay behind slower leaders that no signal causes; against a vertical-queue formula (S2-b, S2-c) it also counts deceleration and acceleration around the line, an offset each comparison declares before it runs |
| Vehicles counted per movement, so throughput per hour | The same batch: each movement's `vehicles` over the evaluation period | Trips that end inside the period (D132). With a cool-down, trips released inside it (D146) | Equals capacity only while the movement's queue never empties. A capacity count uses no cool-down, so it counts trips that end inside the window. A saturated seed is likely to be flagged overloaded (BATCH §3); that is expected, and the flag is kept |
| Whole-route movement delay | The same batch | Source wait plus in-network time, minus the desired-speed reference (D39) | Includes source waiting and entry acceleration: not a benchmark quantity |

Model facts that decide which options fit:

- **Arrivals are generated as Poisson streams** ([SIMULATION](../reference/SIMULATION.md)), then
  inserted at most one per source segment per tick behind a standstill-distance guard (D108) and
  carried by car-following. The stream reaching the stop line is therefore near-random, with a
  minimum headway. A formula that assumes uniform arrivals, such as HCM's uniform delay term
  alone, describes a stream the engine cannot generate. A random-arrival formula is the closer
  match, not an exact one, and the distance from source to stop line must be stated.
- **Gap acceptance is a deterministic threshold.** Each priority rule has one gap time and one
  headway distance, not a distribution of critical gaps (`src/core/types.hpp`). The follow-up
  headway is not a parameter; it emerges from car-following. A capacity formula fed with the
  engine's own effective headways tests the implementation, not the behaviour (see kind U).
- **Signals are fixed time,** with green, amber and red, and amber stop-or-go (M4.2,
  [AMBER](../reference/AMBER.md)). Actuated control is not modelled yet (M4).
- **Vehicle types come from the catalog** (car, heavy vehicle). Base conditions mean a car-only
  composition.

## 3. Kind S1 — saturation flow and capacity of a signalised approach

| Option | Source | Quantity and conditions | Comparable figure and command | Limits |
|---|---|---|---|---|
| **S1-a** Handbook default | [HCM7] Ch. 19 | Base saturation flow `s0` in pc/h/ln: 1,900 by default (HCM 2010 gives 1,750 for areas under 250,000 population; not checked in the 7th edition). Lane-group capacity `c = N·s·g/C`, with `s` the adjusted saturation flow in veh/h/ln. Base conditions: 12 ft lanes, no heavy vehicles, level grade, no parking, no buses stopping, even lane utilisation, through movements only (both from HCM 2010-based agency sources; not checked in the 7th edition) | `--discharge` on a car-only, straight, through-only lane. `--discharge-steady-first 5` with a declared last rank mirrors the 1985 HCM's "headways after the fourth vehicle in the queue" (quoted in [LiPrev02]; the 7th edition's field procedure was not read); `--discharge-startup-last 4` gives the start-up estimate over the first four. Then `3600 / h` per lane and cycle, averaged over cycles and seeds | A national planning default, not a site measurement; local calibrations differ from it. The steady headway is almost set by the following model's parameters, so a match tests the parameter choice unless the model, behaviour set and catalog are frozen first. The 1985 HCM's field method runs to the last vehicle of each standing queue, while `--discharge` uses a declared rank range of vehicles queued at green. Effective green is not measured: amber ends the discharge window, so `c` must use a declared `g` |
| **S1-b** UK regression | [RR67] | 2080 pcu/h for a non-nearside lane 3.2 m wide (the abstract's figure), straight ahead, from 64 UK sites, with terms for lane width, gradient, nearside position and turning radius. It replaced [TP56] | The same `--discharge` command on a non-nearside straight lane of the reference width | Only the intercept can be tested: the engine has no grade or kerb effect. The 64 sites were chosen to avoid heavy parking and pedestrian activity, and TRL Software's notes call the 2080 figure optimistic, better than an average site. UK fleet of the 1980s; pcu, not veh. Left-hand traffic, so the nearside lane maps directly onto a Thai drawing |
| **S1-c** Australian method | [ARR123] | Base saturation flows in through-car units by environment class and lane type, with Akçelik's definitions of lost time and effective green | The same command | The table values were not read for this sheet and must be taken from the report. 1970s Australian data; left-hand traffic |
| **S1-d** Field headway profile | [Bonneson92], [LiPrev02] | Discharge headway by queue position at US signalised junctions. Bonneson finds the minimum headway is reached only from about the eighth position | The crossings `--discharge` emits per cycle give the headway at each rank; ranks are declared to match each paper's positions | Neither paper's per-position headways were read for this sheet; they must be taken from the papers before pre-registration. A profile tests behaviour, not one parameter. Five sites in Bonneson, US fleet. Cycles with fewer queued vehicles than the last declared rank are unavailable |
| **S1-e** Field trajectories | [NGSIM] Lankershim Blvd (2005), Peachtree St (2006) | Vehicle trajectories, free, on signalised arterials; saturation headways can be measured at the stop lines | An adapter that turns the trajectories into crossing records for the discharge estimator, which `estimateDischarge` already accepts in code; no CLI path exists | A dataset, not a published figure: the comparison figure must first be computed by a stated method, and the gate asks for a published result. Prevailing conditions: mixed fleet, platoons, congested periods. Lankershim has two 15-minute periods, so few saturated cycles per lane; Peachtree's periods were not confirmed |

## 4. Kind S2 — delay on a signalised approach

| Option | Source | Quantity and conditions | Comparable figure and command | Limits |
|---|---|---|---|---|
| **S2-a** Handbook control delay | [HCM7] Ch. 19; closed forms in [HCM2000] Ch. 16 | Control delay in s/veh over an analysis period `T`. In HCM 2000, `d = d1·PF + d2 + d3`, with `d1 = 0.5·C·(1 − g/C)² / (1 − min(1, X)·g/C)` and `d2 = 900·T·[(X − 1) + √((X − 1)² + 8·k·I·X/(c·T))]`, with `k = 0.5` for pretimed control and `I = 1` for an isolated approach; `T` is in hours and `c` is the lane-group capacity in veh/h. HCM 2010 adopted incremental queue accumulation for the uniform term; the 7th edition's form, including whether it keeps `PF`, must be read before use | Section delay, `--seeds 42-81 --csv`, on a section from upstream of the longest queue to past the acceleration zone. `X` is computed from a declared `s` and `g`, because TrafficSim reports no v/c. Evaluation window `T` after a warm-up. `d3 = 0`, and `d2` itself, assume no residual queue at the start of `T`, while the warm-up starts the window loaded, so the two diverge most near `X = 1`. The batch selects trips by section end crossing (no cool-down) or by release (with one), not by arrival during `T`; that offset must be declared | `d1` alone assumes uniform arrivals, which the engine cannot generate; only `d1 + d2` describes Poisson arrivals. The `max(0, ·)` truncation and the desired-speed reference differ from control delay. Paid |
| **S2-b** Random-arrival formula | [TP39] | Webster's mean delay at a fixed-time approach with random arrivals; the first two terms are queueing theory, the third an empirical correction fitted to simulation: `d = c(1 − λ)² / [2(1 − λx)] + x² / [2q(1 − x)] − 0.65·(c/q²)^(1/3)·x^(2+5λ)`, where `c` is the cycle in s, `λ = g/c`, `q` the arrival flow in veh/s and `x = q/(λ·s)` the degree of saturation, with `s` the saturation flow in veh/s. Undersaturated only | The same section delay, at stated `x` below 1 | Diverges as `x → 1`. The saturation flow and effective green it needs must be declared before the run, from the S1 choice or a stated lost-time convention, never fitted to engine output |
| **S2-c** Exact queueing results | [Darroch64], [vanLeeuwaarden06]; overview in [TFT9] | The fixed-cycle traffic-light queue with Poisson (and more general) arrivals in a discrete slot model, one departure per green slot: Darroch gives the stationary queue-length distribution with bounds on mean delay; van Leeuwaarden gives the generating functions of queue length and delay, evaluated by numerical procedures | A project as close to the slot model as the engine allows: one lane, a green and red program without amber; the same section delay | A vertical queue with constant departures and no start-up loss, whereas the engine discharges a physical queue by car-following, with start-up loss: that difference must be stated. It checks the queueing arithmetic, not driving behaviour. No published table fits a TrafficSim setting: the reference value is computed from the papers' numerical procedures by a script that does not exist yet, and whether the departure slot itself counts as delay must be confirmed first. Useful before S2-a or S2-b, not instead of them |
| **S2-d** Time-dependent model | [ARR123]; [AkcRou94] | Akçelik's two-term delay (uniform plus overflow), finite at and above saturation; the 1994 paper extends it to platooned arrivals | The same section delay, including oversaturated `x` | Its constants must be read from the report. Overlaps S2-a in form |
| **S2-e** Published field delay | [Kyte08] | Field-measured delay on Lankershim Boulevard ([NGSIM]) against HCM uniform delay and incremental queue accumulation | A Lankershim project with its timing and volumes; the same section delay | Closely spaced signals, reported as actuated by a secondary source, and congested periods: hard to reproduce. No numbers from the paper were seen. Read first whether it tabulates its field delays, whether it compares total control delay or only the uniform-delay component, and its field delay definition. Paid |

## 5. Kind U — capacity of an unsignalised minor movement

**Read first.** The engine has no critical-headway parameter, so no option below is set by
writing a published `t_c` into a rule. A minor vehicle waits while any major vehicle through the
conflict is within `headway` **metres** of the conflict entry, occupies the area, or would reach
it within `gapTime` at its current speed ([M3_CONTRACT](../reference/M3_CONTRACT.md) §4). A
conflict zone reserves the whole area, and Stop control holds each vehicle at the line for at
least one tick (§5). The effective critical headway, front to front between major vehicles, is
therefore `gapTime` plus at least the time the earlier major vehicle takes to clear the area.
The critical headway must be translated into the rule's `gapTime` and `headway` by a method fixed
before the run. The follow-up headway emerges from car-following and the Stop service; no rule
parameter sets it, so it can only be measured.

**One command for all options, not available today.** Each option needs:
- a car-only project with a Stop-controlled minor approach whose demand exceeds its capacity;
- one project per major-flow level, because a batch reports one evaluation period;
- `trafficsim-cli --project F --seeds 42-81 --csv out.csv` with no cool-down;
- the minor movement's `vehicles` per hour of window, against the major flow actually counted.

No such project exists. `trafficsim-t-junction-sweep` varies gap time and headway, not major
flow. Nothing counts vehicles at a priority or Stop line: `--stop-lines` (M3.2.8c) counts
crossings per green at signal heads only, one run per seed, and M5.1's evaluation nodes and stop
lines are open. `--discharge` also works at signal heads only, so no follow-up headway is measured
at a priority line.
US and German sources are right-hand traffic: on a Thai, left-hand drawing, the HCM's rank-2
minor right turn is the minor left turn.

| Option | Source | Quantity and conditions | What else it needs | Limits |
|---|---|---|---|---|
| **U-a** Handbook TWSC | [HCM7] Ch. 20; field basis [Kyte96] | Potential capacity `c_p = v_c·e^(−v_c·t_c/3600) / (1 − e^(−v_c·t_f/3600))` in veh/h against conflicting flow `v_c`, with base `t_c` and `t_f` per movement. For a major street of fewer than four lanes, as `t_c / t_f` (from software documentation: PTV Visum help and HCS7 output for the 6th and 7th editions, with Bentley CUBE help, calibrated to HCM 2000/2010, the only source for the minor-through 4.0 s; the manual's exhibit must be read before use): major left 4.1 / 2.2 s, minor right 6.2 / 3.3 s, minor through 6.5 / 4.0 s, minor left 7.1 / 3.5 s. A rank-2 movement without pedestrians has potential capacity equal to movement capacity | The `gapTime` translation above | The base values are means of US driver distributions from the 1990s. `v_c` is assembled from the counted major movements by the HCM's movement-specific conflicting-flow rules, while the engine blocks on every major vehicle through the conflict, which is not the same set. Rank-3 and rank-4 movements need the HCM's impedance factors, so a rank-2 movement is the clean case. Paid |
| **U-b** German handbook | [HBS15] S5 (urban), L5 (rural) | Capacity reported (FGSV seminar slides) to follow Siegloch's form `c = (3600/t_f)·e^(−q_p·(t_g − t_f/2)/3600)`, in passenger-car units per hour, with tabled `t_g` and `t_f` by manoeuvre, possibly also by sign. The exact HBS 2015 equation, including whether it adds a minimum major headway, was not confirmed | As U-a; Stop or Yield chosen to match the table | The `t_g` and `t_f` tables were not read for this sheet. At equal parameters Siegloch's form is never below Harders', and the two agree at low major flow. Under Yield, a minor vehicle that cannot stop ignores the gap tests (the commitment rule), which the HBS model has no counterpart for. Paid, in German |
| **U-c** Analytic theory | [Harders68], [Siegloch73], [Tanner62], presented in [TFT8] | Closed-form capacity of an idealised queue: exponential major headways, one fixed `t_c` and `t_f`, an unimpeded major stream; Tanner adds a minimum major headway | The engine's own effective `t_c` and emergent `t_f`, measured first | Circular: fed with the engine's own headways, it checks the implementation, not behaviour. The engine is not the idealised queue (distance and occupancy tests, area reservation, car-following between major vehicles), so a difference is not automatically a defect. A verification step before U-a or U-b |
| **U-d** Field data behind the HCM | [Kyte96] | Field measurements at 79 US two-way-stop sites: `t_c` estimated from accepted and rejected gaps, `t_f` measured from queued minor vehicles. The calibrated values went into the 1997 HCM, Chapter 10 | The same `t_c` and `t_f` measured in the engine by the report's method: an observer that does not exist | Whether the report tabulates capacity points as well as headways was not confirmed |
| **U-e** Calibration path (R7) | [BKT99] estimation methods; field trajectories [inD] | Critical-gap estimation from accepted and rejected gaps (the paper recommends maximum likelihood and Hewitt's method), applied to drone trajectories at four unsignalised intersections in Germany, free for non-commercial research | An adapter for the trajectories, then the translation into `gapTime` and `headway` | A calibration method, not a capacity benchmark: it yields the parameters that U-a or U-b then tests. Right-before-left sites cannot run, because undetermined priority blocks Run |

## 6. What a comparison fixes before it runs

Whichever options are chosen, a later session writes these down, with the tolerance, before the
first comparable run:

- the following model and behaviour set, with the catalog hashes `--discharge` already prints;
- the time step, the seeds (42–81), the warm-up and window, and a cool-down of zero for capacity
  counts;
- for S1, the declared ranks, the cycles kept and the lane;
- for S2, the section's start and end, the `x` values and how `s` and `g` are declared;
- for U, the major-flow levels, the translation from published `t_c` to `gapTime` and `headway`,
  and how the engine's emergent `t_f` is measured and set against the published one;
- the unit conversion: pc, pcu, tcu or Pkw-E against veh, with car-only demand where the source
  states base conditions.

## 7. Owner questions (answer by letter, sitting B9)

| # | Question | Options | Session's recommendation |
|---|---|---|---|
| Q4 | What kind of benchmark defines M6? (the 2026-10-03 O2) | (a) handbook and analytic references · (b) a published field dataset · (c) local Thai measurements · (d) (a) now, (c) later | **(d)**, as the 2026-10-03 review recommended. No published Thai study on these quantities could be verified for this sheet. The first pass found leads it could not check before its search budget ran out: a Bangkok saturation-flow paper (Minh & Sano 2003, *Journal of the Eastern Asia Society for Transportation Studies* 5), and a thesis and a journal paper on unsignalised delay at Thai universities. (c) would rest on those after a follow-up check, or on the owner's own measurement |
| S1 | Which saturation-flow benchmark, primary and secondary? | S1-a … S1-e | **S1-a** primary, the handbook the LOS pack already follows (it cites the 6th edition; the option is the 7th). **S1-d** secondary: a headway profile by queue position tests behaviour, which one base value cannot |
| S2 | Which delay benchmark, primary and secondary? | S2-a … S2-e | **S2-b** below saturation, because it assumes random arrivals, which the engine approximates (§2), with **S2-a** (`d1 + d2`) across and above saturation. Run **S2-c** first as a check of the queueing arithmetic |
| U | Which minor-movement benchmark, and is the calibration path booked? | U-a … U-e | **U-a** on a rank-2 movement, after **U-c** as a verification step; **U-e** booked as the R7 calibration path. The `gapTime` translation is decided with the tolerance |

An answer here chooses sources only. It closes nothing, validates nothing and removes no marker.

## 8. Later levels

Meso and Macro (M10–M12, [D145](../decisions/RECORD.md#d145)) get their own benchmark kinds when
their contracts are written, by the same rule: options before any engine number. The design names
the candidates: published equilibrium solutions for Macro, analytic queue and shockwave cases
for Meso ([MULTI_LEVEL_MODELLING](../plans/MULTI_LEVEL_MODELLING.md) §3.7).

## References

How these were checked: three passes with web search on 2026-10-10 (research, an adversarial
check, and a third checking only the details printed here), against publisher, TRID, NAP, FHWA or
DOI records where reachable. The HCM and HBS texts are paid and were not read: their values above
come from agency and software documentation, seminar slides and papers that reproduce them, and
must be read in the manual before use. No
exhibit or equation number is given where it could not be confirmed. A search for a published
Thai study of these quantities found none that could be verified (leads in §7, Q4).

- **[AkcRou94]** Akçelik, R., Rouphail, N.M. (1994). Overflow queues and delays with random and
  platooned arrivals at signalized intersections. *Journal of Advanced Transportation* 28(3),
  227–251.
- **[ARR123]** Akçelik, R. (1981). *Traffic Signals: Capacity and Timing Analysis.* Research
  Report ARR No. 123. Australian Road Research Board. [TRID](https://trid.trb.org/View/173392)
- **[BKT99]** Brilon, W., Koenig, R., Troutbeck, R.J. (1999). Useful estimation procedures for
  critical gaps. *Transportation Research Part A* 33(3–4), 161–186.
- **[Bonneson92]** Bonneson, J.A. (1992). Modeling queued driver behavior at signalized
  junctions. *Transportation Research Record* 1365, 99–107. [TRID](https://trid.trb.org/View/371410)
- **[Darroch64]** Darroch, J.N. (1964). On the traffic-light queue. *Annals of Mathematical
  Statistics* 35(1), 380–388. [doi:10.1214/aoms/1177703761](https://doi.org/10.1214/aoms/1177703761)
- **[Harders68]** Harders, J. (1968). *Die Leistungsfähigkeit nicht signalgeregelter städtischer
  Verkehrsknoten.* Forschung Straßenbau und Straßenverkehrstechnik, Heft 76. Bonn: Bundesminister
  für Verkehr. Not read; cited through [TFT8].
- **[HBS15]** FGSV (2015). *Handbuch für die Bemessung von Straßenverkehrsanlagen (HBS),
  Ausgabe 2015.* FGSV Nr. 299. Köln: FGSV Verlag. Chapters S5 and L5, *Knotenpunkte ohne
  Lichtsignalanlage*. [FGSV](https://www.fgsv.de/regelwerk/dialog-zu-ausgewaehlten-regelwerken/hbs-2015/teil-s-stadtstrassen/kapitel-s5)
- **[HCM2000]** Transportation Research Board (2000). *Highway Capacity Manual 2000.* Chapter 16,
  Signalized Intersections.
- **[HCM7]** Transportation Research Board (2022). *Highway Capacity Manual, 7th Edition: A Guide
  for Multimodal Mobility Analysis.* National Academies Press. Chapter 19, Signalized
  Intersections; Chapter 20, Two-Way Stop-Controlled Intersections.
  [doi:10.17226/26432](https://doi.org/10.17226/26432). The LOS pack (`data/los/hcm.json`) cites
  the 6th edition (2016).
- **[inD]** Bock, J., Krajewski, R., Moers, T., Runde, S., Vater, L., Eckstein, L. (2020). The inD
  dataset: a drone dataset of naturalistic road user trajectories at German intersections.
  *IEEE Intelligent Vehicles Symposium (IV)*, 1929–1934. [arXiv:1911.07602](https://arxiv.org/abs/1911.07602)
- **[Kyte08]** Kyte, M., Dixon, M.P., Nayak, V., Abdel-Rahim, A., Strong, D.W. (2008). Testing
  incremental queue accumulation method using Lankershim Boulevard NGSIM data set: a replacement
  for HCM signalized intersection uniform delay and queue method in Los Angeles, California.
  *Transportation Research Record* 2071, 63–70. [doi:10.3141/2071-08](https://doi.org/10.3141/2071-08)
- **[Kyte96]** Kyte, M., Tian, Z., Mir, Z., Hameedmansoor, Z., Kittelson, W., Vandehey, M.,
  Robinson, B., Brilon, W., Bondzio, L., Wu, N., Troutbeck, R. (1996). *Capacity and Level of
  Service at Unsignalized Intersections. Final Report Volume 1: Two-Way-Stop-Controlled
  Intersections.* NCHRP Web Document 5 (Project 3-46). Transportation Research Board.
  [TRID](https://trid.trb.org/View/476626)
- **[LiPrev02]** Li, H., Prevedouros, P.D. (2002). Detailed observations of saturation headways and
  start-up lost times. *Transportation Research Record* 1802, 44–53.
  [doi:10.3141/1802-06](https://doi.org/10.3141/1802-06)
- **[NGSIM]** U.S. Department of Transportation, FHWA. *Next Generation Simulation (NGSIM) Vehicle
  Trajectories and Supporting Data*, data.transportation.gov dataset 8ect-6jqj: Lankershim
  Boulevard, Los Angeles (16 June 2005; fact sheet FHWA-HRT-07-029) and Peachtree Street, Atlanta
  (November 2006). [catalog](https://catalog.data.gov/dataset/next-generation-simulation-ngsim-vehicle-trajectories-and-supporting-data)
- **[RR67]** Kimber, R.M., McDonald, M., Hounsell, N.B. (1986). *The prediction of saturation
  flows for road junctions controlled by traffic signals.* TRRL Research Report RR67. Transport
  and Road Research Laboratory. [TRID](https://trid.trb.org/View/237965)
- **[Siegloch73]** Siegloch, W. (1973). *Die Leistungsermittlung an Knotenpunkten ohne
  Lichtsignalsteuerung.* Forschung Straßenbau und Straßenverkehrstechnik, Heft 154. Bonn:
  Bundesminister für Verkehr. Not read; cited through [TFT8].
- **[Tanner62]** Tanner, J.C. (1962). A theoretical analysis of delays at an uncontrolled
  intersection. *Biometrika* 49(1–2), 163–170.
- **[TFT8]** Troutbeck, R.J., Brilon, W. Unsignalized intersection theory. Chapter 8 in Gartner,
  N.H., Messer, C.J., Rathi, A.K. (eds.), *Revised Monograph on Traffic Flow Theory.* FHWA (web
  edition, undated; usually cited as 1997).
  [PDF](https://www.fhwa.dot.gov/publications/research/operations/tft/chap8.pdf)
- **[TFT9]** Rouphail, N., Tarko, A., Li, J. Traffic flow at signalized intersections. Chapter 9 of
  the same monograph. [PDF](https://www.fhwa.dot.gov/publications/research/operations/tft/chap9.pdf)
- **[TP39]** Webster, F.V. (1958). *Traffic Signal Settings.* Road Research Technical Paper No. 39.
  Road Research Laboratory. London: HMSO. Its formula is reproduced in [TFT9].
- **[TP56]** Webster, F.V., Cobbe, B.M. (1966). *Traffic Signals.* Road Research Technical Paper
  No. 56. Road Research Laboratory. [TRID](https://trid.trb.org/View/159910)
- **[vanLeeuwaarden06]** van Leeuwaarden, J.S.H. (2006). Delay analysis for the fixed-cycle
  traffic-light queue. *Transportation Science* 40(2), 189–199.
  [doi:10.1287/trsc.1050.0125](https://doi.org/10.1287/trsc.1050.0125)
