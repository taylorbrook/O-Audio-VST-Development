# O-GranulatorInstrument — Stage 4 (Polish / Validation) — RESEARCH, Rework R2 (feedback holds the lane level)

**Date:** 2026-11-12 / 13 · **HEAD:** `6c1e1b8a` (plugin sources last changed at `4c89247c`) · **Input:** `stages/4-polish/CONTEXT-R2.md` (D70–D77, open questions 1–10)
**Method:** measurement, not reasoning. The plugin tree was copied to a scratch folder and built there as a single-plugin configure (`OUARICON_BUILD_TESTS=ON`); nothing in the repo was built or edited. A probe (`--r2probe`) was added to the scratch harness and a prototype makeup was patched into the scratch `GiRecordHead.h` / `SlotTape.h` / `SlotLoader.cpp` / `Engine.cpp`, switchable by environment variable so one binary renders both "R1" and "R2". All levels are lane RMS over the head's latched loop, level 0 of the tape, in dB relative to the lane at t = 0 unless stated. Tags: **V** = file:line or a run this session; **A** = assumed. Paths are relative to `plugins/O-GranulatorInstrument/`.
**Evidence kept:** `stages/4-polish/research-r2/` — `prototype.patch` (the whole prototype, applies to HEAD's tree), `r2probe.h` (the probe), `tab.py`, `cpu.txt` and the result logs named below. The patch's built-in defaults are cap +40 dB and a +6 dB short-term ceiling; the "final" logs were run with `OGI_MK_STX=9` (+9 dB), the full-harness and CPU runs with the +6 dB default (CPU with +9).

**Prototype neutrality (V).** With the makeup switched off the scratch harness runs **512 PASS / 0 FAIL** (all render goldens included), and its 64 preset renders are byte-identical to the ones the repo's own R1 harness binary writes. So "off" is R1, and every number labelled R1 below comes from the same binary.

## Corrections to CONTEXT-R2

| CONTEXT-R2 says | Measured | Effect |
|---|---|---|
| D71: makeup cap "about +12 dB" | The return sits **9 to 30 dB** under the lane per lap on the 13 active presets (median 21 dB), 24 dB on the default patch. With a +12 dB cap the prototype still loses more than 9 dB in 60 s on 11 of 13 presets (`research-r2/probe-cap12.txt`). | The cap has to be about **+40 dB**. Sign-off 1. |
| Matching the return to the lane's level holds the level | Not below Amount 100 %. The convex write of two uncorrelated signals at equal level keeps only (1−λ)² + λ² of the power: −3 dB per lap at λ = 0.5. | The makeup law needs a λ term, κ(λ) = √((2−λ)/λ): +0.4 dB at Amount 100 %, +5.1 at 50 %, +8.7 at 25 %, +13.0 at 10 %. D73 is unchanged (Amount is still the write blend). |
| 15 presets have a Feedback Amount above 0 | **14.** One of them (Drones · Frozen Field) has Freeze on, so its head never writes. | 13 presets change; 51 render bitwise as R1 (V, 64-preset dump). |
| D72: only outliers are re-voiced | 9 of the 13 are more than 3 dB louder over the 10 s audition render; FP2's bank loudness spread goes 6.3 → 12.3 dB (limit 9). | The re-voice is mostly a level trim on about 10 presets. Sign-off 4. |
| D71: "filtering to nothing still fades" | The chain filters are 12 dB / octave. Crossed fully (Low Cut 2000 Hz, High Cut 1000 Hz) on broadband material they lose about 19 dB, which the makeup covers (lane −2.1 dB at 60 s). | A fade needs a loss above the cap, for example a 55 Hz drone under a 2 kHz low cut. |
| The fix is a makeup gain on the return | A gain alone leaves two holes: after note-off the dying return still erases the lane (R1: −6 to −42 dB after one 20 s phrase on the presets), and a slow gain release overshoots. | The prototype adds a stop-writing rule and a fast gain attack. Sign-off 2. |
| Only `requirements` re-pins | ARCHITECTURE D7 / §17 describe the write law and the tail rule; both change. | One R2 addendum paragraph in `ARCHITECTURE.md`; its checksum re-pins too. |

## Summary (what the plan must absorb)

1. **Where the level goes (Q1).** The tap law is the main loss: `g_fb = 1 / (S1 + 1)` is −3.5 to −20 dB on the presets, the grains sum incoherently, and the tap is a mean over voices. The chain adds −0.6 to −10 dB. Total per lap −9 to −30 dB; with κ the gain needed to hold is **+15 to +35 dB**.
2. **Recommended design, prototyped (Q2–Q4):** a feed-forward leveller inside `RecordHead::writeCell`, after the chain and before the ±0.7 writer clamp. It solves, once per 32-sample cell, for the gain that makes the written level equal the lane's pristine level over the latched loop region. Four limits sit on that gain: g ≥ 1, the cap, a peak limit at the 0.7 ceiling, and a short-term ceiling.
3. **Result (V):** on the 13 presets the lane ends 60 s within **−0.4 / +0.0 dB** of its start (R1: −12 to −88 dB). Over 600 s, four presets stay within −1.0 / +0.2 dB: no drift. The default patch (row 2.4-1's stimulus) holds within −2.1 dB at Saturate 0 / 20 and −0.6 dB at 100.
4. **Stop-writing rule (new, Sign-off 2).** With no gated voice on the stream, a return the makeup cannot bring within 6 dB of the target for 50 ms stops the head until a gated voice returns. Note-off then costs the lane **≤ 0.7 dB** on the presets (R1: −6 to −42 dB; makeup without the rule: −2 to −12 dB).
5. **Boundedness is untouched (Q5).** The writer clamp and the convex envelope follow the makeup, so `|w| ≤ max(|wc|, 0.7)` holds for any finite gain. FL1, FL1s, FL1p, FL1sp, CL1 and FL2 / FL2p pass on the prototype.
6. **Invariance (Q6).** Everything is formed at grid ticks from cell data, so it is block-size invariant by construction; a makeup render is bitwise equal at blocks 512 / 32 / 1 / variable and on a repeat run, Post and Pre (V). Feedback 0 and the toggle Off never reach the code (parked head).
7. **Existing rows that move: 4 of 512.** CK1, CK4 and CK7n lose their negative control (the subject passes, the control no longer fires); FP2 fails on bank loudness spread. Parity rows need the makeup off under `chainKnobParity`.
8. **Pre is stable (Q8).** Reverb Mix 100 / Size 100 at Amount 100 %: lane within −0.4 / +1.1 dB over 60 s, mean makeup +5 to +10 dB.
9. **Limits that remain, by design of the 0.7 ceiling:** a dense lane hotter than about −16 dBFS RMS settles on a plateau instead of holding (lower at low Amounts), and a lane with peaks above 0.7 cannot be held at all. Sign-off 3.
10. **The sound changes more than the level.** Presets are 0.3 to 7.2 dB louder over the first 10 s, and five move their spectral centroid by more than half an octave, because the loop filters now act lap after lap on a lane that no longer fades.
11. **CPU (Q10):** no measurable cost. C25m 29.1–29.8 % with the makeup against 28.9–29.2 % without (limit 40 %); the bank's typical maximum stays 9.14 % (limit 12 %).

## Sign-offs for the plan (user decisions)

**APPROVED — Taylor, 2026-11-13: all four, as recommended.** (1) cap +40 dB; (2) stop-writing rule yes; (3) clean peak limit; (4) D72 outlier = more than 3 dB louder or more than 0.5 octave centroid shift, level outliers trimmed to within 1.5 dB of their R1 loudness, centroid outliers a listening call.

| # | Decision | Recommended | Alternative | Why it needs a call |
|---|---|---|---|---|
| 1 | Makeup cap | **+40 dB** (on the gain after dividing out κ) | +36 dB: Live · Four Lanes stream 3 is capped 17 % of the time and ends −3.4 dB | D71 said about +12 dB. That value does not fix F3. The cap is not what protects a silent lane (item "Silence" below), so raising it costs little. |
| 2 | Stop writing when the return can no longer be held, while no key is down | **Yes** | No: every note-off leaves a dip (−2 to −12 dB on the presets, −30 dB in Pre with a long tail) that heals over the next lap of playing | It replaces R1's accepted behaviour "the head keeps writing for the tail's length" (`PreTail` 9.7 s → 1.8 s) and changes hands-on rows 2.4-4 and R1-PRE1. |
| 3 | Peaks above the ceiling | **Clean:** the gain backs off so the return stays under 0.7 | Let the writer clamp clip: a −17 dBFS noise lane holds 4 to 6 dB better at Amount 10–50 %, but 2.5 to 28 % of return samples are hard-clipped | Decides whether a hot lane at a low Amount settles lower (clean) or distorts. |
| 4 | D72 threshold and scope | Outlier = more than **3 dB** louder over the 10 s audition render, or centroid moved more than **0.5 octave**. Level outliers get an `sN_level` trim back to within 1.5 dB of their R1 loudness; centroid outliers are a listening call | Trim all 13 | 10 of 13 are outliers by this rule, so "outliers only" and "all" are nearly the same work. The bank hash re-pins either way. |

Defaults taken without a question: the gain never goes below 1 (two-sided was run and changes nothing measurable); the level reference is the mean over the loop region, not a contour (a sparse lane becomes a steady cloud at its original mean level); the makeup is stereo-linked.

## Q1 · Where the level goes

Measured on R1 over 5–10 s of a held chord (`research-r2/probe-r1-baseline.txt`). n = density × size (overlap), `g_fb` from `forked/OutputLaw.h:199-212`. "Need" = gain to hold, including κ.

| Preset (stream) | Amount | n | `g_fb` dB | Lane → tap dB | Chain dB | Total dB | κ dB | Need dB |
|---|---|---|---|---|---|---|---|---|
| Iron Sky (1) | 50 | 8.1 | −14.1 | −16.2 | −4.3 | −20.5 | 5.1 | 25.5 |
| Deep Well (1) | 70 | 7.7 | −13.7 | −17.9 | −10.3 | −28.2 | 3.0 | 31.3 |
| Organ Loft (2, Pre) | 30 | 7.2 | −13.3 | −18.3 | −3.0 | −21.3 | 7.8 | 29.1 |
| Hollow Wind (1) | 45 | 18.0 | −20.0 | −24.9 | −4.8 | −29.7 | 5.7 | 35.3 |
| Low Orbit (1) | 35 | 8.0 | −14.0 | −18.3 | −5.0 | −23.3 | 7.0 | 30.3 |
| Tape Halo (1) | 80 | 8.0 | −14.0 | −20.0 | −1.3 | −21.3 | 2.1 | 23.4 |
| Night Engine (1) | 55 | 8.1 | −14.1 | −18.3 | −5.4 | −23.7 | 4.5 | 28.2 |
| Feedback Loop (2) | 60 | 4.5 | −10.2 | −15.4 | −1.4 | −16.8 | 4.0 | 20.8 |
| Four Lanes (3) | 40 | 1.6 | −5.1 | −19.4 | −0.6 | −20.0 | 6.3 | 26.3 |
| Fifth Bloom (3, Pre) | 25 | 6.3 | −12.4 | −16.7 | −7.1 | −23.8 | 8.7 | 32.5 |
| Slow Tide (2, Pre) | 30 | 5.4 | −11.4 | −12.1 | −1.3 | −13.4 | 7.8 | 21.2 |
| Dotted Echo (1) | 40 | 1.0 | −3.5 | −7.7 | −1.0 | −8.7 | 6.3 | 15.0 |
| Broken Radio (1) | 50 | 7.2 | −13.3 | −18.9 | −0.7 | −19.6 | 5.1 | 24.7 |

- The lane was still falling during the window, so each figure is good to about ±2 dB. The prototype's mean makeup gain confirms them independently (+12.7 to +36.9 dB).
- **Tap law** (`Engine.cpp:536`, `OutputLaw.h:212`): a peak bound for the coherent worst case, so it over-attenuates an incoherent cloud by design. This is the largest term on every preset.
- **Mean over voices** (`Engine.cpp:3849`): three voices at different pitches sum incoherently and are divided by 3, about −4.8 dB. Row 2.4-6 ("chords thin in the lane") is this term.
- **Chain** (`forked/FeedbackChain.h:212`): the diffuser blend is itself an incoherent mix (Deep Well at Diffuse 80 % plus a 4 kHz High Cut and a −12 st pitch: −10.3 dB). The saturator and the AGC cost under 1 dB at these levels.
- **Default patch** (4 s noise tape, three-note chord, n = 1): −24 dB per lap at Amount 100 %. The lane is at −43 dB after 5 s and below −280 dB at 60 s.

## Q2 – Q4 · The design that was prototyped

Everything lives in `RecordHead::writeCell` (`GiRecordHead.h:140`) plus one field on `SlotTape`. `forked/FeedbackChain.h` is not touched.

**Reference.** `SlotLoader::buildTape` (`SlotLoader.cpp:453`, the single place a tape is made: load, Record-in commit, Reset, sample-rate rebuild) adds a cumulative sum of L² + R² per 256 frames to the tape (about 90 KB at the 2.88 M-frame maximum, built on the worker thread). The head reads the mean square of its latched loop `[lo, hi)` from it in O(1). A changed loop region is picked up at the next latch. Checked: a tape whose first half is 20 dB quieter, looped on that half, holds at that half's level (−1.7 dB), not the whole tape's.

**Per cell, at the grid tick, before the write:**

1. Running one-pole means (τ = 1 s) of three products over the cells already written: A = wc², B = wc·r, C = r² (both channels summed, r before the makeup), and the same filter on the constant 1 (u), so the target `T = ref · u` is unbiased from the first cell after a park.
2. Solve `λ²C·g² + 2λ(1−λ)B·g + (1−λ)²A = T` for g. This is the written power of the convex blend, so it accounts for what is left of the old content and for any correlation between lane and return. No feedback controller, no integrator, nothing to wind up.
3. Limits, in order: cap `g ≤ 10^(40/20) · κ(λ)`; peak `g ≤ 0.7 / pk`, pk = held peak of the return (50 ms release); short-term ceiling `g ≤ κ · √(X · ref / Cf)`, Cf = per-cell mean square of the return held with a 50 ms release, X = +9 dB; floor `g ≥ 1`; a non-finite g becomes 1.
4. The gain moves down inside the cell and up at no more than 0.25 dB per cell (375 dB / s at 48 kHz). It is applied as a linear ramp across the cell, the same shape as an `ExactRamp`.
5. `rw = clamp (g · r, ±0.7)`, then the convex write and envelope exactly as today (`GiRecordHead.h:185-198`).

A parked head resets the makeup state (`Engine.cpp:4055`), so Freeze, the toggle Off and Amount 0 start clean.

**What each part is for, with the measurement that justifies it**

| Part | Without it | With it |
|---|---|---|
| Absolute reference (pristine, not "what is being overwritten") | A relative match can only random-walk and, below Amount 100 %, decays geometrically | 600 s: −1.0 / +0.2 dB on four presets (`probe-final-600s.txt`) |
| κ(λ) | Amount 50 % loses 3 dB per lap even with a perfect level match | Presets at Amount 25–80 % hold within 0.4 dB |
| Cap +40 dB | +12: 11 of 13 presets fade. +24: Deep Well −9, Hollow Wind −8, Four Lanes (3) −44 dB | Capped under 1 % of cells on every preset |
| Fast attack on the gain | A first prototype slewed the gain down at 0.25 dB per cell: a quiet lane swung between −11 and **+26 dB** around its reference | Worst overshoot +1.5 dB |
| Short-term ceiling | Overshoot up to +2.3 dB | Up to +1.1 dB held (+1.5 dB once, Pre after note-off). At X = +6 dB it costs 1 to 2 dB of hold on noise; +9 dB is the better setting |
| Peak limit | The writer clamp hard-clips: up to 1.5 % of return samples on the presets, 28 % on a −17 dBFS noise lane at Amount 10 % | Clipped samples ≤ 0.6 % on the presets; the hold gives way instead (Sign-off 3) |
| Detector τ 1 s | 3 s: a sparse lane ends −18 dB, a −55 dBFS lane −99 dB. 0.3 s: holds, but rides the grain texture (Dotted Echo +1.6 dB) | 1 s |

**AGC interplay (Q3).** The makeup is downstream of the chain AGC and never attenuates, so the AGC only sees it through the lap. The peak limit uses the same 0.7 the AGC and the writer clamp use, so the makeup fills the room under the ceiling and stops there. No level oscillation showed in any run (600 s included).

**Silence, noise floor, Record-in (Q4).** The target is the lane's own pristine level, so the makeup can never lift a lane above where it started:

- An all-zero tape stays bitwise zero (g = 1).
- A −110 dBFS tape is held at −110 dBFS; nothing is raised toward audibility.
- A new Record-in take is a new tape from `buildTape`, so it carries its own reference; the head is parked while the take is pending, as today.
- The cap therefore only matters when the return vanishes while the lane is loud, and the stop-writing rule covers the case where no key is down.

**Stop-writing rule (Sign-off 2).** In `Engine::updateHeads` (`Engine.cpp:3939`): a per-stream latch, set when no gated voice drives the stream and the levelled return has been more than 6 dB short of target for 50 ms, cleared by the next gated voice. While set, the run target is false (20 ms run ramp, park, as for Freeze). The precedent is `resetTailHold` in the same function.

| After a 20 s phrase, lane vs start | R1 | Makeup, no rule | Makeup + rule |
|---|---|---|---|
| Iron Sky (release 6 s) | −13.5 | −3.2 | −0.3 |
| Deep Well (8 s) | −24.4 | −7.5 | −0.6 |
| Organ Loft (Pre, 5 s) | −10.7 | −3.5 | −0.6 |
| Slow Tide (Pre, 5 s) | −10.4 | −3.3 | −0.7 |
| Tape Halo (6 s) | −42.1 | −11.6 | −0.4 |
| Fifth Bloom (Pre, 4 s) | −5.8 | −1.7 | −0.4 |
| Default patch, Post | −110 | −3.9 | −0.1 |
| Default patch, Pre, Mix 100 / Size 100 | −68.5 | −30.2 | −5.4 |

The last row is the worst case found: the loop tank's tail falls slowly enough to stay within 6 dB for a whole 4 s lap. A 3 dB threshold would tighten it (not run).

## Results on the prototype (final settings, X = +9 dB)

**Held chord, 60 s** (`probe-final-held.txt`; min / max over marks at 2, 5, 10, 20, 30, 45, 60 s, then the 60 s value):

| Case | R1 at 60 s | R2 min / max | R2 at 60 s |
|---|---|---|---|
| 13 presets, 15 feedback streams | −11.8 … −87.8 | −2.8 / +0.5 | −0.4 … +0.0 |
| Default patch, Amount 100 %, Saturate 0 / 20 / 100 (noise 0.25) | −283 | −2.1 / +0.3 | −2.1 / −2.0 / −0.6 |
| Same, noise 0.1 (gate candidate), Post | −275 | −1.3 / +0.8 | −0.6 / −0.4 / −0.2 |
| Same, Pre (Mix 50) | −218 | −0.4 / +0.6 | −0.4 |
| Pre, Mix 100, Size 100, Saturate 20 / 100 | −93 | −0.4 / +1.1 | +0.1 / +0.0 |
| Noise 0.1, Amount 50 / 25 / 10 % | −84 / −35 / −13 | — | −0.4 / −0.8 / −3.2 |
| Noise 0.25, Amount 50 / 25 / 10 % | −84 / −35 / −13 | — | −4.6 / −7.2 / −9.1 |
| Sparse lane (100 ms hits, 900 ms gaps), Amount 100 / 50 % | −270 / −83 | −3.6 / +0.2 | −2.3 / −0.2 |
| −55 dBFS noise | −245 | −1.3 / +0.6 | −0.7 |
| Noise at 0.9 peak | −294 | — | −11.7 |
| 220 Hz sine at 0.5 peak | −291 | — | −8.0 |

- The two largest dips on the presets are start-up dips on short, sparse lanes (Four Lanes stream 3: −2.8 dB; Dotted Echo: −2.4 dB), gone by 10 s.
- **Plateau, not hold, for hot lanes.** The return may not exceed 0.7 peak, and at Amount below 100 % it has to be κ times the lane's level. A dense noise-like lane at −16.8 dBFS RMS therefore settles 2 dB down at Amount 100 %, 4.6 at 50 %, 7.2 at 25 %, about 9 at 10 % (still falling slowly at 60 s). At −24.8 dBFS it holds to 25 %. The factory sources sit at about −20 dBFS RMS (peaks 0.25, the noise bed 0.6) and hold at every preset Amount.
- A lane with peaks above 0.7 (0.9-peak noise, 0.5-peak sine with κ) cannot keep them; it settles where its peaks fit.

**Presets, the 10 s audition render (Q9)** (`--dump-preset-renders`, R1 against R2; centroid over 2–10 s):

| Preset | Level 0–10 s, R2 − R1 | Level 6–10 s | Centroid Hz, R1 → R2 | Octaves | Outlier |
|---|---|---|---|---|---|
| Iron Sky | +5.4 | +5.2 | 99 → 148 | +0.58 | level, centroid |
| Deep Well | +6.7 | +9.6 | 55 → 44 | −0.30 | level |
| Organ Loft | +2.9 | +3.6 | 298 → 162 | −0.88 | centroid |
| Hollow Wind | +5.5 | +5.7 | 1050 → 589 | −0.83 | level, centroid |
| Low Orbit | +3.0 | +3.0 | 146 → 245 | +0.74 | centroid (level at the line) |
| Tape Halo | +7.2 | +9.5 | 308 → 325 | +0.08 | level |
| Night Engine | +5.8 | +6.5 | 119 → 113 | −0.08 | level |
| Feedback Loop | +5.4 | +6.3 | 873 → 2013 | +1.21 | level, centroid |
| Four Lanes | +0.4 | +0.1 | 502 → 515 | +0.04 | — |
| Fifth Bloom | +2.5 | +3.6 | 257 → 204 | −0.34 | — |
| Slow Tide | +3.1 | +2.8 | 308 → 266 | −0.21 | level (at the line) |
| Dotted Echo | +0.3 | +2.2 | 362 → 501 | +0.47 | — |
| Broken Radio | +4.8 | +6.4 | 2094 → 2831 | +0.44 | level |

- Peak of any R2 render: 0.73 (Tape Halo). FP2's gated loudness top moves from −23.7 (Init) to −17.7 dBFS (Tape Halo).
- **Longer than the audition.** Over 60 s of a held chord the output's RMS frequency (a cruder measure than the centroid above) keeps moving on some: Dotted Echo 1.1 → 6.7 kHz, Feedback Loop 2.5 → 7.0 kHz, Tape Halo 0.45 → 1.8 kHz, Low Orbit 0.45 → 1.7 kHz. The low cut removes lows every lap and the makeup gives the level back, so the lane walks toward the top of the pass band. On R1 this was masked by the fade. It is what D71 chose ("darker / narrower but not quieter"), but it reads as brighter on these four; Taylor should hear it in Pass 2 before the trims are final.

## Q5 · Boundedness

- **Proof, unchanged.** The makeup multiplies r before `rw = clamp (g·r, ±0.7)`. The write is `w = (1−λ)·wc + λ·rw` with λ ∈ [0, 0.95], then `w` is clamped between `wc` and `rw`. So `|w| ≤ max (|wc|, 0.7)` for any finite g, and by induction the tape peak stays ≤ max (pristine peak, 0.7). A non-finite g is replaced by 1 before use; a non-finite `w` is still scrubbed to `wc`.
- **Measured.** FL1 (60 s, 16 voices × 4 streams, Amount 100 %), FL1s, FL1p, FL1sp and CL1 pass on the prototype with 0 bound violations.
  - FL1: written maximum 0.9893 against a bound of 1.0000 on both. On R1 stream 1's tape ends the 60 s at peak 0.000 (dead); on the prototype at 0.70.
  - FL1s: on R1 the written maximum is 0.4946, so the 0.7 clamp is never reached; on the prototype it is 0.7000 with 71 677 clamped samples. The row tests the clamp for real only with the makeup on.
  - CL1: pre-knee peak 5.96 → 7.61, knee output 1.99325 → 1.99504 against 1.99526. It passes (the knee is an asymptote), but the printed margin drops from 0.0020 to 0.0002.
- **Creep at Saturate 100 %.** None: the lane converges on its reference from below and is not pushed above it (default patch, Saturate 100: −0.7 / +0.3 dB over 60 s; Pre Size 100 / Saturate 100: +0.0 dB).
- **A hot loop still grows as on R1.** The makeup never attenuates, so a loop with natural gain above 1 (resonant per-grain filter, as in FL1's stimulus) climbs to the ceiling exactly as before.

## Q6 · Invariance and the rows that move

- **Block size / determinism (V):** one Post and one Pre makeup render, blocks 512 / 32 / 1 / seeded variable and a repeat: output SHA and tape hash all equal.
- **Goldens:** all 42 lines are stream 1, feedback 0; reproduced with the makeup on.
- **PT rows:** PT1 (38 single-stream presets, bitwise) loses its feedback members. 51 presets stay bitwise, 13 move. The R2 baseline has to be re-dumped for those 13 after the trims; PT2's tolerance set shrinks accordingly.
- **Parity (A, not run):** PB rows compare the loop against the parent under `chainKnobParity`. The parent has no makeup, so the makeup must be off whenever that hook is set (the prototype does this). `tests/parity/run.sh` was not executed this session.
- **Full harness, makeup on: 508 PASS / 4 FAIL** (`research-r2/full-harness-prototype-on.txt`):

| Row | What happens | Repair |
|---|---|---|
| CK1 (freeze edge click) | Subject passes (0.77×). Control `freezeHardStop` reads 0.77× and no longer fires | The lane no longer collapses, so the hard stop lands on different material. Re-derive the stimulus / window so the control exceeds 2.0× again |
| CK4 (Reset swap click) | Subject passes (0.30×). Control `resetHardSwap` 1.59×, under its 2.0× line | Same |
| CK7n (control of CK7) | Stream 1 reads 1.82×, stream 3 0.96×: the control does not fire | Same |
| FP2 (bank loudness) | Spread 12.29 dB (limit 9): Ember Field −30.0 … Tape Halo −17.7 | The D72 level trims |

- **INFO rows that change value:** `PreTail` 9.7 s → 1.8 s (stop-writing rule).

## Q7 · Gate design (D75)

New file `tests/render-harness/engine_gates_r2.h`, rows "LH". Stimulus for LH1–LH3: the default patch, stream 1, a 4 s stereo noise tape at 0.1 peak (−24.8 dBFS RMS), chord 48 / 55 / 60 held, 60 s, lane RMS over the latched loop read at t = 0 and at every lap from 8 s.

| Row | Predicate | Prototype | Same row on R1 / control |
|---|---|---|---|
| LH1 | Amount 100 %, Saturate 0 / 20 / 100, Post: every reading within **−3 / +2 dB** of t = 0 | −1.3 … +0.8 | R1: −275 dB. Control `makeupOff` must fail, and must fall more than 20 dB (stimulus validity) |
| LH1p | Same in Pre (Mix 50) | −0.4 … +0.6 | R1: −218 dB |
| LH2 | Amount 50 / 25 %: within −3 / +2 dB | −0.4 / −0.8 | R1: −84 / −35 dB |
| LH3 | Pre, Mix 100, Size 100, Saturate 20 / 100: within −3 / +2 dB | −0.4 … +1.1 | R1: −93 dB |
| LH4 | No overshoot: −55 dBFS noise, max reading ≤ +3 dB | +0.6 | Control: gain attack slewed → +26 dB |
| LH5 | Zero tape stays bitwise zero; a 1e-5 tape never reads above +2 dB | 0 / +0.9 | — |
| LH6 | Reference follows the loop: two-level tape, loop on the quiet half, within −3 / +2 dB of that half | −1.7 | Control: whole-tape reference → about +17 dB (A, arithmetic) |
| LH7 | Cap: a 55 Hz sine tape under Low Cut 2000 Hz still fades by more than 20 dB in 60 s | not run (A) | Control: cap off |
| LH8 | Note-off: default patch and one preset, lane 20 s after release within −3 dB of the lane at release | −0.1 … −0.7 | Control `mkLatchOff` → −3.9 dB (Post), −30 dB (Pre) |
| LH9 | Block 512 / 32 / 1 / variable and a repeat: output and tape hash equal, Post and Pre | equal | — |
| LH10 | Clamp pressure: on the 13 presets, return samples clamped ≤ 1 % | ≤ 0.6 % | Control: peak limit off → up to 1.5 % |
| LH11 | INFO: 600 s drift on four presets | −1.0 / +0.2 | — |

- The −3 dB side of the band is right for these stimuli; the +3 dB side proposed in D75 is looser than needed, so +2 dB.
- **LH10 exists because the writer clamp hides a makeup that overdrives.** The tape peak cannot show it (it is clamped); the clamp counter can. Without this row a gain bug would pass every bound row.
- A noise tape at 0.25 peak is a poor LH1 stimulus: it sits on the peak limit (−2.1 dB, 1 dB of margin).
- FL1 / FL1p / CL1 stay as they are.

## Q8 · Pre mode

The tank sits before the chain, so "matched after tank B" is simply the same measurement point. With a long tail the tank stores the loop's energy and the makeup works less (mean +5 to +10 dB at Mix 100 / Size 100, against +17 dB in Post). No instability at Mix 100 / Size 100, Saturate 20 or 100. The remaining Pre effect is the note-off case in the stop-writing table (−5.4 dB worst).

## Q10 · CPU

Measured on the scratch binary, one process at a time (`research-r2/cpu.txt`). C25w / C25m run Amount 100 % on four chains, so all four heads write and all four makeups run. The prototype is the expensive form (it calls `exp` and `pow` every cell).

| Row | Makeup off | Makeup on | Limit |
|---|---|---|---|
| C25m (worst case, 16 voices × 4 streams) | 28.86 %, 29.19 % | 29.12 %, 29.79 % | 40 % |
| C25mPre (8 tanks) | — (R1 record 31.21 %) | 30.47 %, 31.08 % | 40 % (INFO) |
| C25tm (typical proxy) | 6.66 % | 6.62 %, 6.66 % | 12 % |
| `--cpu4` bank, typical max | 9.14 % (Fifth Bloom) | 9.14 % (Fifth Bloom) | 12 % |
| `--cpu4` bank, median / p90 | 4.29 % / 6.59 % | 4.31 % / 6.55 % | — |

The difference is inside the run-to-run spread (about ±0.5 points on C25m). Four makeups cost nothing measurable. No lever is needed. No allocation: the state is a few scalars per head, and the reference table is built with the tape on the worker thread.

## Files the plan will touch

| File | Change |
|---|---|
| `Source/dsp/GiRecordHead.h` | Makeup state and constants; `writeCell` takes the reference and the coefficients; counters for the gates |
| `Source/dsp/SlotTape.h`, `Source/dsp/SlotLoader.cpp:504` | Cumulative pristine energy per 256 frames, built in `buildTape` |
| `Source/dsp/Engine.cpp` (`writeHeads` :4041, `updateHeads` :3939, `prepare`) | Pass the reference; the stop-writing latch; coefficients formed once in `prepare`; reset on park |
| `Source/dsp/Engine.h` | Latch member; hooks `makeupOff`, `mkAttackSlew`, `mkLatchOff`, `mkPeakOff`, `mkCapOff`, `mkWholeTapeRef` |
| `tests/render-harness/engine_gates_r2.h`, `main.cpp` | LH rows, `--egr2` |
| `tests/render-harness/engine_gates_24b.h`, `engine_gates_r1.h` | CK1, CK4, CK7n stimulus |
| `tests/parity/ParityRows.h` / `parity_gates_24.h` | Makeup off under `chainKnobParity` |
| `Source/FactoryPresets.h` | `sN_level` trims (Sign-off 4); bank hash re-pins |
| `.planning/research/ARCHITECTURE.md` | R2 addendum to D7 / §17; checksum |
| `.planning/REQUIREMENTS.md`, `CHANGELOG.md`, `HANDS-ON-SESSION-4.md` | DSP-04 / FUNC-08 / PERF-02 evidence; `[1.0.0]` amended; rows 2.4-1, 2.4-4, 2.4-6, R1-PRE1 reworded |

Not touched: `forked/` (drift stays 0 / 13), parameters, destinations, UI, strings, `params.tsv`, the destination goldens.

## Pitfalls

1. **The prototype computes `exp` / `pow` per cell.** Fine for a probe; the real code forms every coefficient once in `prepare` and uses only `sqrt` and arithmetic at the tick. Constants are in seconds, not cells, so 44.1 / 96 kHz behave the same.
2. **Do not slew the gain downward.** This is the +26 dB overshoot. The gain has to be able to fall within one cell.
3. **The reference must come from the pristine, not from the working copy.** Anything measured from the lane itself drifts.
4. **The writer clamp masks an overdriving makeup** (LH10).
5. **A gate on a dense 0.25-peak tape measures the peak limit, not the hold.**
6. **`makeupOff` must be set wherever `chainKnobParity` is**, or every PB row moves.
7. **The latch must clear on a gated voice, not on any counted voice.** A releasing voice is still counted (`VoiceAllocator.h:93`); clearing on it would re-open the head during the release it is meant to protect.
8. **Reset in Pre** already drops the tail term (`resetTailHold`); the new latch is a second, independent term on the same run target. RS2p passes on the prototype with both.
9. **Hands-on wording.** Row 2.4-6 expects chords to thin the lane; the makeup removes that. Row 2.4-4 and R1-PRE1 describe the head writing through the tail.
10. **Presets were levelled against a fading lane.** Trims must be done on the R2 build, and PT's baseline re-dumped after them, not before.

## Not measured

- Windows / MSVC behaviour of the makeup (no feedback golden is cross-platform; D54 unchanged).
- Parity run with the makeup forced off (reasoned only).
- LH7's stimulus, and a 3 dB latch threshold.
- 44.1 kHz and 96 kHz renders of the prototype.
- By-ear character: every number here is a level or a centroid. The audition in Pass 2 is the real test of Sign-offs 2–4.

## Next Phase

Ready for: plan phase (`PLAN-R2.md`). Sign-offs 1–4 approved 2026-11-13.
