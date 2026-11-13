---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.1b Summary"
date: 2026-11-13
phase: "2.1b"
status: blocked_c1_continuity
harness: "47 PASS, 1 FAIL (C1), 11 INFO"
---

# O-ParametricEQ — Stage 2 / Phase 2.1b Summary (Designer polish)

**Superseded by `SUMMARY-2.1b-rework.md` (2026-11-13). Kept as the record of the blocked run.**

## Outcome

**Not complete. Stopped at T7 on the plan's escalation rule: D2 is green and C1 (continuity) is red.** The polish is implemented, the C++ equals the Python model (worst 2.95e-10 relative), and every accuracy row passes with margin. But the polished map is not continuous for resonant shelves: C1 finds a 15 dB response jump between neighbouring settings. PLAN-2.1b T7 and risk R1 say this is a research finding, not a port bug, and that no constant or threshold may be changed here. Nothing was changed. **Nothing is committed** (the 17 MB polished golden would have to be regenerated if the algorithm changes).

No processor code changed; the plugin binary is still 0.1.0. No install, no tag.

## The finding (C1)

| | |
|---|---|
| Harness | 341 lines (320 random + 21 fixed sweeps), 2 violations |
| Violation 1, a real jump | 48 dB/oct High Shelf, 48 kHz, line fc 29410 → 1942 Hz, gain −28.06 → +12.87 dB, Q 15.37 → 1.48. Polished / seed jump 637x; jump (32000) / jump (8000) 0.988. The jump stays at 15.1 / 14.9 / 15.6 / 15.2 dB at 8000 / 32 000 / 128 000 / 512 000 steps. |
| Where | fc 27 549 Hz (above Nyquist), gain −27.08 dB, Q 14.53, section 0 (wz 2.968, wp 4.382 rad/sample, Q 37.2). |
| Mechanism | Not a non-finite guard and not the extension gate (C1n shows the same 637x with the binary gate). On one side the four steps land far from the seed (g 0.116 against the seed's 34.0, mB 274 against 1.38) with accept weight 1; one step along the line later the steps land where the objective is not reduced and the accept weight is 0. The fit switches basin and the accept factor follows it. |
| Violation 2, steep but continuous | 48 dB/oct Low Shelf, 44.1 kHz, line starting at fc 23 612 Hz, gain +37.7 dB, Q 28.5: polished 2.30x the seed's step (limit 2x); shrinks 4x per 4x steps down to 512 000 steps. Fails the plan's 2x criterion; it is not a discontinuity. |
| The 21 research sweeps | All pass. |
| Bells | 0 violations in the harness (80 random lines) and in 120 more scratch lines. |

Scratch evidence beyond the harness (execute session, not repo code):

- 600 further random lines (60 per family and rate): non-shrinking jumps on 2 of 60 High Shelf 12 dB/oct lines at 44.1 kHz, 3 of 60 High Shelf 48 dB/oct at 48 kHz, 2 and 1 of 60 Low Shelf 96 dB/oct at 44.1 / 48 kHz. Bells and Low Shelf 12 dB/oct: none.
- 300 000 random settings per row, sections accepted with g or k more than 10x from the seed: bells 0 at every rate; 12 dB/oct shelves 106 at 44.1 kHz (37 with fc inside 0.45 fs), 6 at 48 kHz (0 inside), 0 at 96 kHz; all slopes 566 of 1.07 M sections at 44.1 kHz (96 inside), 284 at 48 kHz (0 inside), 0 at 96 kHz. Failures of the Cholesky / finite guards: 0 everywhere.
- One bounded probe: an extra accept factor fading the polish out with its log-parameter distance from the seed (1.5 → 3 and 0.7 → 1.4). Neither removes the jumps; the tighter one also loses the weak-spot gains. So this is not a one-constant fix.

Reading: the gated grids (bells; shelves with Q ≤ 1.41; steep shelves at Q 1) are unaffected. The jumps live in the region the plan reports but does not gate: resonant shelf sections (high section Q) with the corner near or above Nyquist, the same region as research's "poor basin" weak spot. Research tested continuity on 21 one-dimensional sweeps at 44.1 kHz only and listed that as a limit.

## Decision needed

How to make the polish continuous for resonant shelves. Options, recommended first:

1. **Back to research (recommended).** Scope: the resonant-shelf region only. Candidates: a continuous fade of the polish toward the seed keyed on the analog section itself (section Q and corner against Nyquist), a weight and not a switch; or a better seed there, which research already named as the likely fix for the weak spot. The harness and model are ready, so each candidate is a small change plus one 20 s run.
2. Restrict the polish to sections where it is proven continuous (a smooth fade above some section Q) and give up the weak-spot gains in v1. D7b says "including the weak spots", so this departs from D7.
3. Accept as is. Not recommended: a 15 dB step while sweeping a resonant steep shelf is an audible click.

## Tasks

| Task | Result |
|---|---|
| Gate 1 → 2 | PASSED again (build, pluginval), `--skip-review` as in 2.1. |
| T1 pre-flight | Green: four checksums, `main`, plugin tree clean, designer harness 28 / 0 / 6, render harness 36 / 0, `check_spec.py` OK. |
| T2 model | `tests/model/polish_model.py`, `dump_polish.py`. Written by the execute session, not the dsp-agent. |
| T3 golden | `tests/golden/designs-polished.tsv`: 88 935 rows, 17 035 355 bytes, no CR / nan / inf, two runs identical. Self-check: bells 0.043 / 0.081 / 0.193 (research the same), shelves 12 dB/oct 10–20 k 0.044 / 0.120 / 0.187 (research 0.04 / 0.12 / 0.19). |
| T4 / T5 polish | `Source/dsp/PolishConstants.h`, `Polish.h`; `Designer.h` (`design` polished, `designSeed`, `designWith<Extension>`), `EqTypes.h` (`polishWeight`). Written by the execute session. |
| T6 harness | `tests/designer-harness/main.cpp` extended by the dsp-agent: 16 Phase 2.1b rows, 4 hardening rows, 5 INFO rows. Compiled first time with no warning. |
| T7 | Build rc 0, no warnings. Harness **47 PASS, 1 FAIL, 11 INFO** in 19 s. Everything except C1 is done (below). |
| T8 | This file and STATUS only. No commit. |

## What passes

- **D2** C++ == model: 662 970 cells, worst 2.95e-10 relative, weights 2.96e-08, band response 1.57e-09 dB.
- **G1b** at five rates, all cells inside the 2.1b tolerance. 44.1 kHz at ±6 / ±12 / ±30: bells 0.043 / 0.081 / 0.193; shelves 12 dB/oct Q ≤ 1.41 0.044 / 0.120 / 0.187 (±18: 0.200 against 0.266, the thinnest cell); 24–96 dB/oct at most 0.059 / 0.117 / 0.232 against 0.122 / 0.194 / 0.410. The unmodelled 36 and 72 dB/oct shelves pass (risk R2 did not materialise). Seed and RBJ both fail it (G1bn-seed, G1bn-rbj).
- **NW1** 0 of 178 000 sections worse than the seed. **R2** 4.6e-13 dB. **F2** 8 008 192 designs, 0 bad. **S1** 10 000 of 10 000 bit-identical. **TL2** 5.8e-15 dB. **A2** 0 allocations. **B2** 10 000 / 10 000.
- Every Phase 2.1 row still passes with `designSeed` (D1 still bit-exact on 1 176 175 cells).
- Hardening from VERIFICATION-2.1: G2s literal slopes; N1 at 1e-9 (2.95e-13); G2s-short (worst 0.641 dB/oct, 9 of the 11 skipped cells measured); CS1 / CS2 cut-skirt ceilings (CS2 set from this run + 15 %); P1 two-sided clamp check (32 / 32). Not re-run against the verify mutants.

## Negative controls

| Control | Result |
|---|---|
| `--neg seed`, `rbj-b`, `binary-gate`, `rbj`, `f1`, `alloc` | rc 0 each |
| One cell of a golden copy changed in the 4th significant figure | D2 FAIL (and the standing C1 FAIL), nothing else |
| Constants grep (`1e-3`, `0.006`, `0.002`, `0.03`) in `Source/dsp` | `PolishConstants.h` only |
| Static gates (std-only, no FP flags, no processor wiring, processor and CMake diff) | all empty |
| Render harness, `check_spec.py` | 36 PASS / 0 FAIL; OK 196 params |

**C1n caveat:** the binary-gate control fires (14 violations against 2), but its worst line is the same 637x jump the shipped design has, so on that line it proves nothing. Its own evidence is the 12 dB/oct lines (for example 0.146 → 0.117 dB, not shrinking). C1n should score only lines the smooth design passes.

## Other findings

- **I-odd:** 18 dB/oct, 10–20 k: 0.071 / 0.151 / 0.296 against the INFO ceiling 0.10 / 0.15 / 0.30: 0.001 dB above at ±12 (WARN, not gated).
- **I-weak differs from research Q8 in several cells** on the harness grid: 44.1 kHz, 12 dB/oct Q 20–40, 10–20 k: 0.94 / 3.80 / 0.35 against research 0.22 / 0.93 / 0.35; Q 2–10: 0.20 / 0.59 / 4.40 against 0.13 / 0.25 / 4.25. A scratch scorer on research's settings reproduces research exactly (0.13 / 0.25 / 4.25 and 0.22 / 0.93 / 0.35) from the same headers, so the difference is in how the harness row scores or picks its settings, not in the polish. Not resolved; INFO only.
- **I-new:** Tilt equals the high shelf per slope; 96 dB/oct with Q 4–40: 0.30 / 0.31 / 0.69 at 44.1 kHz (seed 0.94 / 1.94 / 6.30); bells 20–30 kHz 2.70 → 0.43 and 2.69 → 0.19 (model 0.40, 0.17), none worse than the seed; 22.05 / 32 kHz as research said (not reliably better).
- **I-cost** (whole `Designer` call per section, one thread): seed 88.5 ns, polished 643 ns on random bells (39 % with weight > 0), 307 ns where the weight is 0, 405 ns per section on random 96 dB/oct shelves. Under the 2 µs WARN line. "Weight 0" includes fitted-then-rejected sections, so 307 ns is not the pure skip cost.
- **H1** bells 44.1 kHz whole grid: 25/7/68 → 44/7/49 % polished / blended / seed from ±6 to ±30, equal to research Q4.

## Constants (`Source/dsp/PolishConstants.h`)

fixedPoints 16, tiedScales 0.5 / 1.5, tiedPoints 10, numPoints 26, steps 2, extraSteps 2, numFree 6, lambda 1e-3, mu 0.1, seedKeepRms 0.002 dB, seedFullRms 0.006 dB, extensionRms 0.03 dB, acceptRamp 0.1, band 20 Hz … min (20 kHz, 0.499 fs).

## Deviations

1. T2, T4, T5 were written by the execute session; the dsp-agent wrote T6 only.
2. C1 scores on a fixed 160-point log grid (research's t10 grid), not "64 log points plus tied points" (R5): tied points move with the settings, so neighbouring steps would be compared at different frequencies.
3. A step that fails inside the extension leaves φ2 as a whole (ARCHITECTURE wording); research's `cand.py` replaced non-finite elements one by one. No such case occurs in any sweep.
4. Hardening rows from VERIFICATION-2.1 added here (STATUS carried them); they change the tally from the plan's "N + new rows".
5. `BandDesign::polishWeight` is compared by the bit-identity rows (SL1, Q1, P1, B1, B2, S1).
6. Harness additions beyond the plan: `--dump-polished`; F2 also requires weight 0 on unpolished classes.
7. The gate re-run rewrote `stages/1-foundation/gate-report.json`.

## Uncommitted files

Modified: `Source/dsp/Designer.h`, `Source/dsp/EqTypes.h`, `tests/designer-harness/main.cpp`, `.planning/STATUS.md`, `.planning/stages/1-foundation/gate-report.json`.
New: `Source/dsp/Polish.h`, `Source/dsp/PolishConstants.h`, `tests/model/polish_model.py`, `tests/model/dump_polish.py`, `tests/golden/designs-polished.tsv`, this file.

## Harness output: changed 2.1 rows, then the Phase 2.1b block (verbatim, repo-relative paths)

```
  [PASS] N1 10 Hz, Q 40, +30 dB bell at 192 kHz vs prototype, 2-50 Hz       2.95e-13 dB (limit 1e-9)
  [PASS] G2s-short skipped High Cut cells: slope -20 dB point -> 20 kHz within 1.0 dB/oct of the prototype's worst 0.641 dB/oct; 9 measured, 2 unmeasurable (span under one octave) of 11 skipped
           6 dB/oct: worst 0.436 on 7 cells (verify measured 0.43);  12 dB/oct: worst 0.641 on 2 cells (verify measured 0.64 over all nine 12 dB/oct cells; only the skipped ones are scored here)
  [PASS] P1 D10 two-sided: the notch clamp is AT 0.95 pi and the band-pass clamp AT 0.9 pi (harness literals) 32/32 checks: design changes below the clamp, is fixed above it, null / peak at the clamp
  [PASS] CS1 Low Cut 12 / 24 / 96 dB/oct, fc 2-20 kHz, Q 1, 44.1 kHz vs prototype above -60 dB 12 dB/oct 1.3464 dB (ceiling 1.55) | 24 dB/oct 0.8223 dB (ceiling 0.95) | 96 dB/oct 0.0090 dB (ceiling 0.05); verify measured 1.35 / 0.82 / 0.01 with another scorer
  [PASS] CS2 High Cut 12 / 48 / 96 dB/oct, fc <= 10 kHz, Q 1, 44.1 kHz vs prototype above -60 dB 12 dB/oct 1.0966 dB (ceiling 1.27) | 48 dB/oct 3.9805 dB (ceiling 4.58) | 96 dB/oct 4.2161 dB (ceiling 4.85); ceilings = first measured run + 15 %
  [PASS] A1 1e5 designSeed() calls: 0 allocations, 0 frees (malloc_logger, this thread) 0 allocations, 0 frees; hook live (planted: 1 / 1); 248633 sections designed
  [PASS] B1 10 000 mixed settings: main thread == second thread with its own Designer (memcmp) 10000/10000 (used parameters, weights, true prototypes)

-- 2.1b / D2. the polish: constants and golden
           polish constants, read from Source/dsp/PolishConstants.h (PLAN-2.1b P10):
             fixedPoints 16, numTiedScales 2, tiedScales 0.5 / 1.5, tiedPoints 10, numPoints 26
             steps 2, extraSteps 2, numFree 6, lambda 0.001, mu 0.1
             seedKeepRms 0.002 dB, seedFullRms 0.006 dB, extensionRms 0.03 dB, acceptRamp 0.1
             match band 20 Hz ... min (20000 Hz, 0.499 fs)
  [PASS] D2 C++ design == golden designs-polished.tsv (1e-6 rel; mL cL 1e-9; weight 1e-6; response 1e-6 dB) 88935 data lines (C++ list: 88935), 0 unparsed, 0 shape; 662970 cells, 0 out of tolerance, worst rel 2.95e-10; weights: 0 off, worst 2.96e-08; 30723 bands, 0 off, worst 1.57e-09 dB
           worst cell: line 19938 (highShelf fs 44100 slope-index 7 fc 30000 q 40 gain 60 section 0) column mB
           worst band response ends at: line 1618 (lowShelf fs 44100 slope-index 1 fc 30000 q 40 gain 18 section 0)

-- 2.1b / G1b. tightened magnitude match, and its negative controls
  [PASS] G1b-bell bells, Q 0.025-40: 0.05 + 0.012 |g|; >= 88.2 k 0.03 + 0.002 |g| polished; five rates, fc 20 Hz-20 kHz x 40, +-3/6/12/18/30 dB
           fs  44100  worst/tol:  +-3 dB 0.021/0.086  +-6 dB 0.043/0.122  +-12 dB 0.081/0.194  +-18 dB 0.114/0.266  +-30 dB 0.193/0.410
           fs  48000  worst/tol:  +-3 dB 0.013/0.086  +-6 dB 0.016/0.122  +-12 dB 0.026/0.194  +-18 dB 0.032/0.266  +-30 dB 0.073/0.410
           fs  88200  worst/tol:  +-3 dB 0.008/0.036  +-6 dB 0.009/0.042  +-12 dB 0.011/0.054  +-18 dB 0.012/0.066  +-30 dB 0.013/0.090
           fs  96000  worst/tol:  +-3 dB 0.008/0.036  +-6 dB 0.009/0.042  +-12 dB 0.010/0.054  +-18 dB 0.010/0.066  +-30 dB 0.010/0.090
           fs 192000  worst/tol:  +-3 dB 0.007/0.036  +-6 dB 0.007/0.042  +-12 dB 0.007/0.054  +-18 dB 0.008/0.066  +-30 dB 0.008/0.090
           model, polished at +-6 / +-12 / +-30 dB: 44.1 k 0.043 / 0.081 / 0.193, 48 k 0.016 / 0.026 / 0.073, 88.2 k up <= 0.013
  [PASS] G1b-sh12 shelves 12 dB/oct, Q <= 1.41, low and high: same tolerance polished; five rates, fc 20 Hz-20 kHz x 40, +-3/6/12/18/30 dB
           fs  44100  worst/tol:  +-3 dB 0.026/0.086  +-6 dB 0.044/0.122  +-12 dB 0.120/0.194  +-18 dB 0.200/0.266  +-30 dB 0.187/0.410
           fs  48000  worst/tol:  +-3 dB 0.013/0.086  +-6 dB 0.013/0.122  +-12 dB 0.031/0.194  +-18 dB 0.057/0.266  +-30 dB 0.080/0.410
           fs  88200  worst/tol:  +-3 dB 0.007/0.036  +-6 dB 0.008/0.042  +-12 dB 0.009/0.054  +-18 dB 0.009/0.066  +-30 dB 0.012/0.090
           fs  96000  worst/tol:  +-3 dB 0.007/0.036  +-6 dB 0.008/0.042  +-12 dB 0.009/0.054  +-18 dB 0.009/0.066  +-30 dB 0.010/0.090
           fs 192000  worst/tol:  +-3 dB 0.004/0.036  +-6 dB 0.007/0.042  +-12 dB 0.007/0.054  +-18 dB 0.007/0.066  +-30 dB 0.008/0.090
           model at 44.1 k, +-6 / +-12 / +-30 dB: 0.04 / 0.12 / 0.19
  [PASS] G1b-shN shelves 24 / 36 / 48 / 72 / 96 dB/oct, Q 1.0, low and high: same tolerance polished; five rates, fc 20 Hz-20 kHz x 40, +-3/6/12/18/30 dB; every slope passes
           24 dB/oct  fs  44100  worst/tol:  +-3 dB 0.027/0.086  +-6 dB 0.038/0.122  +-12 dB 0.091/0.194  +-18 dB 0.103/0.266  +-30 dB 0.178/0.410
           24 dB/oct  fs  48000  worst/tol:  +-3 dB 0.006/0.086  +-6 dB 0.008/0.122  +-12 dB 0.020/0.194  +-18 dB 0.027/0.266  +-30 dB 0.064/0.410
           24 dB/oct  fs  88200  worst/tol:  +-3 dB 0.003/0.036  +-6 dB 0.004/0.042  +-12 dB 0.006/0.054  +-18 dB 0.004/0.066  +-30 dB 0.005/0.090
           24 dB/oct  fs  96000  worst/tol:  +-3 dB 0.003/0.036  +-6 dB 0.004/0.042  +-12 dB 0.006/0.054  +-18 dB 0.006/0.066  +-30 dB 0.004/0.090
           24 dB/oct  fs 192000  worst/tol:  +-3 dB 0.000/0.036  +-6 dB 0.000/0.042  +-12 dB 0.000/0.054  +-18 dB 0.000/0.066  +-30 dB 0.001/0.090
           36 dB/oct  fs  44100  worst/tol:  +-3 dB 0.025/0.086  +-6 dB 0.036/0.122  +-12 dB 0.068/0.194  +-18 dB 0.092/0.266  +-30 dB 0.174/0.410
           36 dB/oct  fs  48000  worst/tol:  +-3 dB 0.008/0.086  +-6 dB 0.010/0.122  +-12 dB 0.015/0.194  +-18 dB 0.023/0.266  +-30 dB 0.036/0.410
           36 dB/oct  fs  88200  worst/tol:  +-3 dB 0.004/0.036  +-6 dB 0.003/0.042  +-12 dB 0.005/0.054  +-18 dB 0.005/0.066  +-30 dB 0.005/0.090
           36 dB/oct  fs  96000  worst/tol:  +-3 dB 0.003/0.036  +-6 dB 0.003/0.042  +-12 dB 0.005/0.054  +-18 dB 0.004/0.066  +-30 dB 0.006/0.090
           36 dB/oct  fs 192000  worst/tol:  +-3 dB 0.000/0.036  +-6 dB 0.000/0.042  +-12 dB 0.000/0.054  +-18 dB 0.000/0.066  +-30 dB 0.001/0.090
           48 dB/oct  fs  44100  worst/tol:  +-3 dB 0.026/0.086  +-6 dB 0.043/0.122  +-12 dB 0.074/0.194  +-18 dB 0.089/0.266  +-30 dB 0.143/0.410
           48 dB/oct  fs  48000  worst/tol:  +-3 dB 0.016/0.086  +-6 dB 0.017/0.122  +-12 dB 0.027/0.194  +-18 dB 0.035/0.266  +-30 dB 0.057/0.410
           48 dB/oct  fs  88200  worst/tol:  +-3 dB 0.005/0.036  +-6 dB 0.004/0.042  +-12 dB 0.005/0.054  +-18 dB 0.007/0.066  +-30 dB 0.008/0.090
           48 dB/oct  fs  96000  worst/tol:  +-3 dB 0.003/0.036  +-6 dB 0.003/0.042  +-12 dB 0.003/0.054  +-18 dB 0.005/0.066  +-30 dB 0.006/0.090
           48 dB/oct  fs 192000  worst/tol:  +-3 dB 0.000/0.036  +-6 dB 0.000/0.042  +-12 dB 0.000/0.054  +-18 dB 0.000/0.066  +-30 dB 0.000/0.090
           72 dB/oct  fs  44100  worst/tol:  +-3 dB 0.028/0.086  +-6 dB 0.052/0.122  +-12 dB 0.103/0.194  +-18 dB 0.122/0.266  +-30 dB 0.161/0.410
           72 dB/oct  fs  48000  worst/tol:  +-3 dB 0.026/0.086  +-6 dB 0.028/0.122  +-12 dB 0.047/0.194  +-18 dB 0.073/0.266  +-30 dB 0.113/0.410
           72 dB/oct  fs  88200  worst/tol:  +-3 dB 0.005/0.036  +-6 dB 0.007/0.042  +-12 dB 0.005/0.054  +-18 dB 0.008/0.066  +-30 dB 0.009/0.090
           72 dB/oct  fs  96000  worst/tol:  +-3 dB 0.003/0.036  +-6 dB 0.006/0.042  +-12 dB 0.005/0.054  +-18 dB 0.007/0.066  +-30 dB 0.010/0.090
           72 dB/oct  fs 192000  worst/tol:  +-3 dB 0.000/0.036  +-6 dB 0.000/0.042  +-12 dB 0.000/0.054  +-18 dB 0.000/0.066  +-30 dB 0.000/0.090
           96 dB/oct  fs  44100  worst/tol:  +-3 dB 0.043/0.086  +-6 dB 0.059/0.122  +-12 dB 0.117/0.194  +-18 dB 0.158/0.266  +-30 dB 0.232/0.410
           96 dB/oct  fs  48000  worst/tol:  +-3 dB 0.038/0.086  +-6 dB 0.034/0.122  +-12 dB 0.059/0.194  +-18 dB 0.087/0.266  +-30 dB 0.107/0.410
           96 dB/oct  fs  88200  worst/tol:  +-3 dB 0.005/0.036  +-6 dB 0.010/0.042  +-12 dB 0.009/0.054  +-18 dB 0.007/0.066  +-30 dB 0.012/0.090
           96 dB/oct  fs  96000  worst/tol:  +-3 dB 0.003/0.036  +-6 dB 0.006/0.042  +-12 dB 0.007/0.054  +-18 dB 0.007/0.066  +-30 dB 0.011/0.090
           96 dB/oct  fs 192000  worst/tol:  +-3 dB 0.000/0.036  +-6 dB 0.000/0.042  +-12 dB 0.000/0.054  +-18 dB 0.000/0.066  +-30 dB 0.000/0.090
           model at 44.1 k, 24-96 dB/oct, +-6 / +-12 / +-30 dB: 0.06 / 0.12 / 0.23 (36 and 72 dB/oct were not modelled)
  [PASS] G1b-sh6 shelves 6 dB/oct with design(): unchanged 0.10 + 0.02 |g|; >= 88.2 k 0.10 + 0.015 |g| not polished (first-order section); five rates
           fs  44100  worst/tol:  +-3 dB 0.089/0.160  +-6 dB 0.173/0.220  +-12 dB 0.312/0.340  +-18 dB 0.405/0.460  +-30 dB 0.490/0.700
           fs  48000  worst/tol:  +-3 dB 0.089/0.160  +-6 dB 0.173/0.220  +-12 dB 0.312/0.340  +-18 dB 0.405/0.460  +-30 dB 0.490/0.700
           fs  88200  worst/tol:  +-3 dB 0.061/0.145  +-6 dB 0.118/0.190  +-12 dB 0.212/0.280  +-18 dB 0.275/0.370  +-30 dB 0.333/0.550
           fs  96000  worst/tol:  +-3 dB 0.053/0.145  +-6 dB 0.103/0.190  +-12 dB 0.185/0.280  +-18 dB 0.241/0.370  +-30 dB 0.291/0.550
           fs 192000  worst/tol:  +-3 dB 0.015/0.145  +-6 dB 0.029/0.190  +-12 dB 0.052/0.280  +-18 dB 0.067/0.370  +-30 dB 0.081/0.550
  [PASS] G1bn-seed the closed-form seed FAILS the G1b-bell tolerance at 44.1 kHz worst at +-30 dB 0.975 dB (model 0.34 / 0.61 / 0.98 against 0.12 / 0.19 / 0.41)
           fs  44100  worst/tol:  +-3 dB 0.174/0.086  +-6 dB 0.338/0.122  +-12 dB 0.612/0.194  +-18 dB 0.798/0.266  +-30 dB 0.975/0.410
  [PASS] G1bn-rbj the RBJ bilinear bell FAILS the G1b-bell tolerance at 44.1 kHz worst at +-30 dB 17.39 dB
           fs  44100  worst/tol:  +-3 dB 2.427/0.086  +-6 dB 4.796/0.122  +-12 dB 9.183/0.194  +-18 dB 12.867/0.266  +-30 dB 17.392/0.410

-- 2.1b / NW1, R2. never worse per section; reciprocity
  [PASS] NW1 per gain2 section: polished max error <= seed max error + 1e-3 dB (G1b grids, five rates) 0 violations, 0 non-finite, 178000 sections compared; worst (polished - seed) +0 dB (model: 0 violations)
           worst (polished - seed) at: bell slope-index 1 fc 20 q 0.025 gain 3 fs 44100 section 0: seed 0.00035, polished 0.00035 dB
  [PASS] R2 polished: |H(+g) H(-g)| = 1 within 1e-9 dB (bells; shelves and tilt, 8 slopes, Q 0.3 / 1 / 4) bells 4.62e-13 dB, shelves / tilt 1.25e-13 dB, five rates (model 1.2e-12)

-- 2.1b / C1. continuity (about 26 million band designs, threaded)
  [FAIL] C1 continuity: 320 random lines (4 families x 2 rates x 40) + 21 fixed sweeps, 8000 and 32000 steps 341 lines, 2 violations (0 non-finite); worst polished / seed jump 637.706 (limit 2), worst jump (32000) / jump (8000) 0.988 (limit 0.5); model: within 2x, shrinking 4x
           worst polished / seed: random shelf 48 dB/oct #27 (highShelf, slope-index 5, fs 48000): fc 29410.5446 -> 1942.12035 Hz, gain -28.0638385 -> 12.8669676 dB, Q 15.3728095 -> 1.48341725
           worst 32000 / 8000:    random shelf 48 dB/oct #27 (highShelf, slope-index 5, fs 48000): fc 29410.5446 -> 1942.12035 Hz, gain -28.0638385 -> 12.8669676 dB, Q 15.3728095 -> 1.48341725
           first violation:       random shelf 48 dB/oct #2 (lowShelf, slope-index 5, fs 44100): fc 23611.57 -> 11.5678476 Hz, gain 37.666369 -> -18.3506847 dB, Q 28.4559365 -> 0.279256067: seed 0.594050 / 0.148651 dB, polished 1.365787 / 0.352010 dB at 8000 / 32000 steps
  [PASS] C1n the C1 test FAILS a binary extension gate (designWith<BinaryExtension>) on at least one line 181 lines, 14 violations (0 non-finite); worst polished / seed jump 637.706 (limit 2), worst jump (32000) / jump (8000) 1.127 (limit 0.5); fixed sweeps + random shelf lines; model: 0.06-0.13 dB steps that do not shrink
           worst polished / seed: random shelf 48 dB/oct #27 (highShelf, slope-index 5, fs 48000): fc 29410.5446 -> 1942.12035 Hz, gain -28.0638385 -> 12.8669676 dB, Q 15.3728095 -> 1.48341725
           worst 32000 / 8000:    random shelf 12 dB/oct #28 (lowShelf, slope-index 1, fs 48000): fc 15048.468 -> 21.4726288 Hz, gain -57.9055633 -> -43.9619607 dB, Q 15.5999774 -> 0.728975885
           first violation:       random shelf 12 dB/oct #12 (lowShelf, slope-index 1, fs 44100): fc 9852.31699 -> 250.606273 Hz, gain 28.3214203 -> 12.7005966 dB, Q 14.9695065 -> 1.12201954: seed 0.043953 / 0.010989 dB, polished 0.146125 / 0.116868 dB at 8000 / 32000 steps

-- 2.1b / F2, S1, TL2. robustness, skip exactness, tilt identity
  [PASS] F2 polished, 1e6 random settings x 8 rates + range ends: finite, g k G > 0, gain2 m c > 0, weight in [0, 1], count 8008192 designs, 0 bad, fewest draws per type 124225
  [PASS] S1 skip exactness: every gain2 weight 0 -> design == designSeed, used parameters bit for bit 10000 found / 30408 drawn, 0 differ
  [PASS] TL2 tilt error == high-shelf error within 1e-9 dB (12 and 48 dB/oct, G1 shelf grid, 44.1 / 48 kHz) worst 5.79e-15 dB over 20800 bands at fs 44100 slope-index 1 fc 167.535528 q 1.41 gain -6

-- 2.1b / A2, B2. audio-thread fitness with the polish on
  [PASS] A2 1e5 design() calls, polished: 0 allocations, 0 frees (malloc_logger, this thread) 0 allocations, 0 frees; hook live (planted: 1 / 1); 248633 sections designed
  [PASS] B2 polished, 10 000 mixed settings: main thread == second thread with its own Designer (memcmp) 10000/10000 (used parameters, weights, true prototypes)

-- 2.1b / INFO (never failing)
  [INFO] H1 share of gain2 sections, polished (w >= 0.999) / blended / seed (w = 0) in %, at +6 | +12 | +30 dB 
           bells           44.1 k  fc <= 10 k  21/7/73 | 26/7/67 | 39/7/54
           bells           44.1 k  10-20 k     57/13/29 | 69/12/19 | 93/6/1
           bells           44.1 k  whole grid  25/7/68 | 31/7/62 | 44/7/49   (research Q4: polished 25 -> 44, blended 7, seed 68 -> 49 from +-6 to +-30 dB)
           bells           48.0 k  fc <= 10 k  19/6/75 | 24/6/70 | 36/7/57
           bells           48.0 k  10-20 k     53/13/34 | 65/13/22 | 90/6/4
           bells           88.2 k  fc <= 10 k  10/5/86 | 14/5/81 | 25/6/69
           bells           88.2 k  10-20 k     31/9/60 | 40/10/50 | 65/13/22
           bells           96.0 k  fc <= 10 k  9/4/86 | 13/5/82 | 23/6/71
           bells           96.0 k  10-20 k     29/10/60 | 38/12/50 | 66/9/25
           bells          192.0 k  fc <= 10 k  3/4/93 | 6/4/90 | 12/5/83
           bells          192.0 k  10-20 k     12/16/72 | 25/15/60 | 51/7/41
           shelves 12 dB   44.1 k  fc <= 10 k  23/10/66 | 31/10/59 | 43/9/48
           shelves 12 dB   44.1 k  10-20 k     65/12/23 | 71/10/19 | 85/8/8
           shelves 12 dB   48.0 k  fc <= 10 k  22/10/68 | 29/9/62 | 41/9/51
           shelves 12 dB   48.0 k  10-20 k     69/8/23 | 75/10/15 | 87/8/6
           shelves 12 dB   88.2 k  fc <= 10 k  9/7/84 | 14/7/79 | 23/9/68
           shelves 12 dB   88.2 k  10-20 k     25/25/50 | 46/12/42 | 65/15/19
           shelves 12 dB   96.0 k  fc <= 10 k  8/7/85 | 13/7/80 | 22/7/70
           shelves 12 dB   96.0 k  10-20 k     21/23/56 | 42/13/44 | 69/6/25
           shelves 12 dB  192.0 k  fc <= 10 k  0/7/93 | 4/7/89 | 11/6/83
           shelves 12 dB  192.0 k  10-20 k     0/23/77 | 15/19/65 | 37/25/38
  [INFO] I-odd 18 dB/oct shelves, Q 1, 44.1 kHz, polished band error at +-6 / +-12 / +-30 dB; ceiling 0.10 / 0.15 / 0.30 
           fc <= 10 k:  0.052  0.102  0.241   (research Q8, seed/polished: 0.07/0.05, 0.14/0.10, 0.43/0.24)
           10-20 k:     0.071  0.151  0.296   (research Q8, seed/polished: 0.48/0.06, 0.92/0.15, 1.64/0.28)  WARN: above the ceiling
  [INFO] I-weak resonant shelves, seed/polished band error at +-6, +-12, +-30 dB: fc <= 10 k | 10-20 k 
           44.1 k  12 dB/oct, Q 2-10         0.14/0.04, 0.31/0.07, 4.43/0.40 | 4.96/0.20, 7.19/0.59, 7.69/4.40
                   Q8 44.1 k: 0.13/0.04, 0.30/0.07, 4.09/0.19 | 4.96/0.13, 7.19/0.25, 7.63/4.25
           44.1 k  12 dB/oct, Q 20-40        0.14/0.04, 0.32/0.10, 6.75/0.14 | 7.42/0.94, 11.69/3.80, 14.91/0.35
                   Q8 44.1 k: 0.14/0.04, 0.31/0.10, 6.08/0.23 | 7.42/0.22, 11.69/0.93, 14.91/0.35
           44.1 k  48 dB/oct, Q .25 .5 2 4   0.06/0.02, 0.11/0.03, 0.30/0.04 | 1.23/0.19, 2.91/0.14, 8.23/0.36
                   Q8 44.1 k (Q 0.25-4): 0.05/0.02, 0.11/0.03, 0.29/0.05 | 1.23/0.19, 2.90/0.14, 8.23/0.27
           48.0 k  12 dB/oct, Q 2-10         0.07/0.04, 0.13/0.04, 2.09/0.25 | 2.31/0.15, 5.69/0.24, 7.46/0.26
           48.0 k  12 dB/oct, Q 20-40        0.07/0.05, 0.13/0.05, 2.30/0.45 | 2.56/0.46, 7.06/0.54, 11.05/0.45
           48.0 k  48 dB/oct, Q .25 .5 2 4   0.04/0.02, 0.08/0.03, 0.22/0.05 | 0.56/0.10, 1.20/0.12, 4.90/0.30
           the stubborn cell (PLAN-2.1b P4, reported, not gated): 12 dB/oct, Q 2-10, +-30 dB, 10-20 k at 44.1 kHz: research 7.63 seed -> 4.25 dB polished (the last figure of the first 44.1 k line above)
           research at 48 kHz: weak rows at or below 0.40 / 0.46 / 0.43 dB polished
  [INFO] I-new cases research did not run (new evidence; not gated)         
           Tilt as a type, Q 1, fc 20 Hz-20 kHz x 40, polished band error at +-6 / +-12 / +-30 dB:
             44.1 k   6 dB/oct:  0.173  0.312  0.490
             44.1 k  12 dB/oct:  0.043  0.117  0.130
             44.1 k  18 dB/oct:  0.057  0.144  0.281
             44.1 k  24 dB/oct:  0.036  0.087  0.178
             44.1 k  36 dB/oct:  0.036  0.068  0.174
             44.1 k  48 dB/oct:  0.043  0.074  0.143
             44.1 k  72 dB/oct:  0.052  0.103  0.153
             44.1 k  96 dB/oct:  0.059  0.117  0.227
             48.0 k   6 dB/oct:  0.173  0.312  0.490
             48.0 k  12 dB/oct:  0.008  0.022  0.032
             48.0 k  18 dB/oct:  0.059  0.117  0.280
             48.0 k  24 dB/oct:  0.007  0.015  0.051
             48.0 k  36 dB/oct:  0.010  0.015  0.033
             48.0 k  48 dB/oct:  0.017  0.027  0.057
             48.0 k  72 dB/oct:  0.028  0.047  0.102
             48.0 k  96 dB/oct:  0.034  0.059  0.107
           96 dB/oct shelves, Q 4 / 10 / 40, fc 20 Hz-20 kHz x 40, seed/polished at +-6, +-12, +-30 dB:
             44.1 k  0.94/0.30, 1.94/0.31, 6.30/0.69
             48.0 k  0.50/0.09, 1.00/0.17, 2.75/0.10
           bells with fc 20-30 kHz (re-centred seed), fc x 20, Q 0.025-40 x 17, gains +-3 ... +-30 dB, seed -> polished:
             44.1 k  2.70 -> 0.43 dB, 0 of 3400 settings worse than the seed by more than 1e-3 dB   (model, random sweep: 2.66 -> 0.40)
             48.0 k  2.69 -> 0.19 dB, 0 of 3400 settings worse than the seed by more than 1e-3 dB   (model, random sweep: 2.51 -> 0.17)
           bells at 22.05 and 32 kHz, G1 grid, scored 20 Hz - 0.499 fs, seed -> polished (research Q7: not improved reliably):
             22.05 k  fc below 0.499 fs: 1.390 -> 0.867 dB, 2 of 6120 worse than the seed;  every fc to 20 kHz: 5.504 -> 1.998 dB, 24 of 6800 worse
             32.00 k  fc below 0.499 fs: 1.328 -> 0.849 dB, 6 of 6460 worse than the seed;  every fc to 20 kHz: 7.001 -> 2.648 dB, 8 of 6800 worse
  [INFO] I-cost ns per section, whole Designer call (prototype + design), one thread, 44.1 kHz [checksum 6.36888e+06]
           (i)   seed, 400 000 random bells:                     88.8 ns   (research port, section only: 30-36 ns)
           (ii)  polished, the same bells:                      645.1 ns   39.2 % of sections with weight > 0   (model 0.8 us per polished section)
           (iii) polished, the 243265 bells whose weight is 0:    308.8 ns   (seed kept: skipped, or fitted and not accepted)
           (iv)  100 000 random 96 dB/oct shelves (8 sections): seed 30.8 ns, polished 405.0 ns per section; 16.4 % of sections with weight > 0

47 PASS, 1 FAIL, 11 INFO
```
