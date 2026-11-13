---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.1b Rework Summary"
date: 2026-11-13
phase: "2.1b"
status: executed
harness: "54 PASS, 0 FAIL, 12 INFO"
supersedes: "SUMMARY-2.1b.md (the blocked run)"
---

# O-ParametricEQ — Stage 2 / Phase 2.1b Rework Summary (polish continuity)

## Outcome

**Complete. Designer harness 54 PASS, 0 FAIL, 12 INFO (35 s).** The polished designer is continuous on every line tested and never far worse than its seed off the grid; every accuracy row keeps its tolerance; the first Phase 2.1b build fails all three new protections. The three decided changes are in: shelf re-centre on the seed side, step limit 0.35, no polish below 44.1 kHz.

One thing did not go as planned and was decided by Taylor during the run: Phase 2.1 row **T1 failed** on one line the plan said the rule would not reach (deviation 1). The ARCHITECTURE "Model results" row was amended.

No processor code changed; the plugin binary is still 0.1.0. No install, no tag.

## Tasks

| Task | Result |
|---|---|
| Gate 1 → 2 | PASSED (`--skip-review`, as in 2.1 and 2.1b). |
| T1 pre-flight | Checksums equal; `main`; tree exactly as the plan lists; baseline reproduced: **47 PASS, 1 FAIL (C1), 11 INFO**; render harness 36 / 0; `check_spec.py` OK. |
| T2 model seed | `tests/model/eqmodel.py`: `recentre_pair`, three constants, `sstep`. Written by the execute session. |
| T3 designs golden + audit | 162 855 rows, two runs byte-identical, no CR / nan / inf. **4800 lines changed**, all `lowShelf` / `highShelf` / `tilt` `gain2` (1600 each); 4800 lines have a pair above 0.95 π with Q above 1.5 and all 4800 changed; 0 changed lines for any other type or class. The audit computes the region from the Phase 2.1 formula written out in the script, not from the model's design slot. |
| T3 model self-check | The eleven RESEARCH-2.1 Q1 rows reproduce. One moved, attributed to the rule: Shelf 48 dB/oct, Q 1, 10–20 k at 44.1 kHz: 0.84 / 1.72 / 4.01 → **0.84 / 1.85 / 4.33** (section 0 has Q 2.56). Q2 cells (notch, band pass): 0 changed lines. |
| T4 / T5 C++ | `Prototype.h` (`detail::recentrePair`, constants beside `bellRecentre`), `EqTypes.h` (the one `detail::smoothStep`), `PolishConstants.h` (`stepLimit`, `minSampleRateHz`), `Polish.h` (`ShippedPolicy`, `polishStep<Policy>` with the limit under `if constexpr`), `Designer.h` (`prepare` sets the rate flag, `isPolishEnabled()`, `designWith<Policy>`). Written by the execute session. |
| T6 / T7 polished model + golden | `polish_model.py` (`STEP_LIMIT`, `MIN_FS`). Golden 88 935 rows, byte-identical twice, clean. Self-check: bells 0.044 / 0.083 / 0.193 (expect 0.043 / 0.081 / 0.193 within 0.01); shelves 12 dB/oct Q ≤ 1.41, 10–20 k 0.050 / 0.119 / 0.188 (expect 0.043 / 0.120 / 0.184 within 0.02; the script's fc and Q grid is not research's). Changed against the first build's golden: bell 830 of 1617 lines, lowShelf 11 377, highShelf 11 370, tilt 11 370 of 29 106 each. |
| T8 harness | Written by the dsp-agent (one resume after a network error); compiled first time with one unused-constant warning, fixed with `[[maybe_unused]]`. |
| T9 build, gates, controls | Below. |
| T10 | This file, addenda, STATUS, one commit. |

## Results

| Row | Result |
|---|---|
| **D1** | 1 176 175 cells, all bit-exact against the regenerated `designs.tsv`. |
| **D2** | 662 970 cells, worst 4.49e-13 relative; weights worst 2.96e-08; band response 1.1e-12 dB. |
| **C1** | 1101 lines, **0 violations**. Worst polished / seed 2.167x (limit 4x), **1 line above 2x** (18 dB/oct Low Shelf, 44.1 kHz); worst 32 000 / 8000 ratio 0.295 (limit 0.5). |
| C1, the two lines that failed | `48 dB/oct #27` at 48 kHz: 0.980x, shrink 0.250 (was 637x, 0.988). `48 dB/oct #2` at 44.1 kHz: 1.339x, shrink 0.295 (was 2.30x). |
| **C1-hi** | 216 lines at 88.2 / 96 / 192 kHz, 0 violations, worst 1.010x, shrink 0.252. New evidence (research ran these rates at limit 0.5 only). |
| **C1n** | Fires: 13 violations on 181 lines the shipped design passes (worst 33.9x, shrink 1.067). |
| **NW2** | 1 000 000 bands, 2 598 194 `gain2` sections per rate. Worst excess **+0.660 dB at 44.1 kHz** (research +0.67), **+0.270 dB at 48 kHz**; ceiling +1.0. Above +0.01 dB at 44.1 kHz: 294 of 31 053 in the hard region (Q ≥ 4, upper pair above 0.9 Nyquist), 55 of 2 567 141 in the rest (worst +0.126). 96 kHz (INFO): +0.000. |
| **RA1** | 0 runaway of 1 451 300 sections with weight > 0. |
| **PR1** | 20 000 designs at 22.05 / 32 kHz: 0 differ from `designSeed`, every weight 0; the same settings at 44.1 kHz: 1773 bands with a weight > 0. |
| **P1s** | Continuity at the three boundaries: worst 3.27e-08 relative (limit 1e-6), 8 / 8 and 24 / 24 checks. Region: 499 568 pairs, 0 changed outside it, 0 true sections off the 2.1 formula; 3853 moved pairs, magnitude at `wT` within 6.35e-16. |
| **G1b** | Every cell inside tolerance at five rates (verbatim below). The 72 dB/oct ±30 dB cell at 44.1 kHz is **0.247** against 0.410 (first build 0.161; research predicted 0.247). Thinnest cell unchanged in kind: 12 dB/oct ±18 dB at 44.1 kHz. |
| NW1, R2, F2, S1, TL2, A2, B2 | Pass (verbatim below). |
| Phase 2.1 rows | G1-bell, G1-sh12, G1-sh6, R1, N1, G2s, CS1, CS2, NB1–NB4 print the same figures as the baseline log. T1: see deviation 1. |
| Stage 1 | Render harness 36 PASS / 0 FAIL; `check_spec.py` OK, 196 params. |

G1b steep shelves at 44.1 / 48 kHz, first build:

```
24 dB/oct  fs  44100  worst/tol:  +-3 dB 0.027/0.086  +-6 dB 0.038/0.122  +-12 dB 0.091/0.194  +-18 dB 0.103/0.266  +-30 dB 0.178/0.410
24 dB/oct  fs  48000  worst/tol:  +-3 dB 0.006/0.086  +-6 dB 0.008/0.122  +-12 dB 0.020/0.194  +-18 dB 0.027/0.266  +-30 dB 0.064/0.410
36 dB/oct  fs  44100  worst/tol:  +-3 dB 0.025/0.086  +-6 dB 0.036/0.122  +-12 dB 0.068/0.194  +-18 dB 0.092/0.266  +-30 dB 0.174/0.410
36 dB/oct  fs  48000  worst/tol:  +-3 dB 0.008/0.086  +-6 dB 0.010/0.122  +-12 dB 0.015/0.194  +-18 dB 0.023/0.266  +-30 dB 0.036/0.410
48 dB/oct  fs  44100  worst/tol:  +-3 dB 0.026/0.086  +-6 dB 0.043/0.122  +-12 dB 0.074/0.194  +-18 dB 0.089/0.266  +-30 dB 0.143/0.410
48 dB/oct  fs  48000  worst/tol:  +-3 dB 0.016/0.086  +-6 dB 0.017/0.122  +-12 dB 0.027/0.194  +-18 dB 0.035/0.266  +-30 dB 0.057/0.410
72 dB/oct  fs  44100  worst/tol:  +-3 dB 0.028/0.086  +-6 dB 0.052/0.122  +-12 dB 0.103/0.194  +-18 dB 0.122/0.266  +-30 dB 0.161/0.410
72 dB/oct  fs  48000  worst/tol:  +-3 dB 0.026/0.086  +-6 dB 0.028/0.122  +-12 dB 0.047/0.194  +-18 dB 0.073/0.266  +-30 dB 0.113/0.410
96 dB/oct  fs  44100  worst/tol:  +-3 dB 0.043/0.086  +-6 dB 0.059/0.122  +-12 dB 0.117/0.194  +-18 dB 0.158/0.266  +-30 dB 0.232/0.410
96 dB/oct  fs  48000  worst/tol:  +-3 dB 0.038/0.086  +-6 dB 0.034/0.122  +-12 dB 0.059/0.194  +-18 dB 0.087/0.266  +-30 dB 0.107/0.410
```

Rework:

```
24 dB/oct  fs  44100  worst/tol:  +-3 dB 0.027/0.086  +-6 dB 0.038/0.122  +-12 dB 0.092/0.194  +-18 dB 0.107/0.266  +-30 dB 0.187/0.410
24 dB/oct  fs  48000  worst/tol:  +-3 dB 0.006/0.086  +-6 dB 0.008/0.122  +-12 dB 0.020/0.194  +-18 dB 0.027/0.266  +-30 dB 0.064/0.410
36 dB/oct  fs  44100  worst/tol:  +-3 dB 0.025/0.086  +-6 dB 0.037/0.122  +-12 dB 0.068/0.194  +-18 dB 0.091/0.266  +-30 dB 0.173/0.410
36 dB/oct  fs  48000  worst/tol:  +-3 dB 0.008/0.086  +-6 dB 0.010/0.122  +-12 dB 0.015/0.194  +-18 dB 0.023/0.266  +-30 dB 0.032/0.410
48 dB/oct  fs  44100  worst/tol:  +-3 dB 0.026/0.086  +-6 dB 0.043/0.122  +-12 dB 0.079/0.194  +-18 dB 0.098/0.266  +-30 dB 0.153/0.410
48 dB/oct  fs  48000  worst/tol:  +-3 dB 0.016/0.086  +-6 dB 0.017/0.122  +-12 dB 0.027/0.194  +-18 dB 0.036/0.266  +-30 dB 0.057/0.410
72 dB/oct  fs  44100  worst/tol:  +-3 dB 0.028/0.086  +-6 dB 0.052/0.122  +-12 dB 0.109/0.194  +-18 dB 0.161/0.266  +-30 dB 0.247/0.410
72 dB/oct  fs  48000  worst/tol:  +-3 dB 0.026/0.086  +-6 dB 0.028/0.122  +-12 dB 0.047/0.194  +-18 dB 0.075/0.266  +-30 dB 0.096/0.410
96 dB/oct  fs  44100  worst/tol:  +-3 dB 0.043/0.086  +-6 dB 0.059/0.122  +-12 dB 0.119/0.194  +-18 dB 0.205/0.266  +-30 dB 0.214/0.410
96 dB/oct  fs  48000  worst/tol:  +-3 dB 0.038/0.086  +-6 dB 0.034/0.122  +-12 dB 0.059/0.194  +-18 dB 0.087/0.266  +-30 dB 0.118/0.410
```

Phase 2.1 INFO rows that moved (seed, shelves ≥ 24 dB/oct, Q 1, 10–20 k, 44.1 kHz):

```
before: 24 dB/oct: 0.63 / 1.24 / 2.31   48 dB/oct: 0.84 / 1.72 / 4.01   96 dB/oct: 0.87 / 1.77 / 4.91
after:  24 dB/oct: 0.63 / 1.24 / 2.31   48 dB/oct: 0.84 / 1.85 / 4.33   96 dB/oct: 0.87 / 1.77 / 5.04
```

## Negative controls

| Control | Result |
|---|---|
| `--neg first-build` | rc 0. All three rows fail on the first build: NW2 fails; RA1 1949 runaway sections; C1 block A: the 637x line, shrink 0.988. |
| `--neg binary-gate`, `seed`, `rbj-b`, `rbj`, `f1`, `alloc` | rc 0 each. |
| One `mB` cell of a `designs.tsv` copy changed in the 6th significant figure | D1 FAIL only. |
| One `mB` cell of a `designs-polished.tsv` copy changed in the 4th | D2 FAIL only. |
| `--dump-designs` twice | identical SHA-1, equal to the golden's (`3a733591…`). |
| Constants grep | `0.35`, `44100`: `PolishConstants.h` only. `1.5`, `4.0` in `Prototype.h`: the two definitions, plus `std::pow (a.wp / a.wz, 4.0)` in `analogSecondOrderMag2` (an exponent, there since 2.1). PLAN-2.1b T7.5: `PolishConstants.h` only. |
| Static gates (std-only tokens, FP flags, processor wiring, processor and CMake diff) | all empty. |

## Constants

`Prototype.h`: shelfRecentre 0.95 (x π), shelfRecentreQLo 1.5, shelfRecentreQHi 4.0 (beside bellRecentre 0.95, recentreQMin 0.025, recentreQMax 160).
`PolishConstants.h`: stepLimit 0.35, minSampleRateHz 44100; the rest as in SUMMARY-2.1b.

## Other findings

- **I-odd WARN stands:** 18 dB/oct, 10–20 k: 0.071 / 0.151 / 0.299 against the INFO ceiling 0.10 / 0.15 / 0.30 (0.001 dB above at ±12; not gated; it was there in the first build).
- **I-weak** (44.1 kHz, 10–20 k, polished): 12 dB/oct Q 2–10 0.14 / 0.44 / 0.44 (first build 0.20 / 0.59 / 4.40; the stubborn cell is now 0.44 against the bench's 0.45); Q 20–40 0.94 / 0.98 / 0.42 (first build 0.94 / 3.80 / 0.35). All under the RP11 ceilings 1.0 / 1.5 / 1.0. The ±6 dB cell of Q 20–40 reads 0.94 against the bench's 0.75: the grid difference SUMMARY-2.1b reported is still there and still unresolved. New row, 12 dB/oct Q 2–40, fc 20–30 k: 0.58 / 0.22 / 0.38, equal to the bench.
- **I-cost:** seed 90.1 ns (was 88.5); polished bells 685 ns (was 643); weight-0 bells 315 ns (was 307); 96 dB/oct shelves 426 ns per section (was 385–405). Under the 2 µs WARN line.
- **H1** unchanged: 25/7/68 → 44/7/49 %.

## Deviations

1. **T1 failed and the contract table was amended (Taylor's call, this session).** PLAN T9.4 lists T1 among the rows the rule does not reach. It does reach one of T1's eleven lines: Shelf 48 dB/oct, Q 1, 10–20 k at 44.1 kHz, whose section 0 has Q 2.56. The seed there reads 0.84 / 1.85 / 4.33 against the table's 0.84 / 1.72 / 4.01 (T1 limit ±0.05). Model and C++ agree (D1 bit-exact) and the polished family is inside tolerance. Resolution: that one row of ARCHITECTURE "Model results" and the harness's T1 expectation now read 0.84 / 1.85 / 4.33; the `architecture` checksum is refreshed. The other ten lines are unchanged.
2. **NW2 draws 250 000 bands per type** (1 000 000 per rate), not 250 000 in total: the literal reading gives about 650 000 sections and voids the row against its own 1 000 000 floor. The result (2.6 million sections) matches plan risk R6.
3. **First-build control: bells keep the 2.1 bell re-centre as their seed**, as the first build did; only shelf / tilt sections are seeded from the true section.
4. **P1s (iii) / (iv) run at eight rates** to reach the 1000 moved-pair floor (3853 found). `0.95 π ≤ wT ≤ w0` has 1e-12 relative slack (`exp (log x)` can land one ulp either side).
5. **Seeds the plan does not name:** C1-hi `0xC1C1C1C3`, NW2 `0x4E573200`, PR1 `0x50523100`, P1s `0x50315331`.
6. `SmoothExtension` is renamed `ShippedPolicy` (plan T5 allowed it). `Designer::isPolishEnabled()` added (PR1 reads it).
7. T2, T4, T5, T6 were written by the execute session; the dsp-agent wrote T8.
8. T7's shelf self-check reads 0.050 at ±6 dB against 0.043 (inside the 0.02 limit); the harness's own G1b-sh12 cell is 0.044.

## Carry-forward

- **Verify 2.1b:** re-verify the seed change (the T3 audit, P1s, the amended T1 line) as well as the polish. `VERIFICATION-2.1.md` covers the unchanged lines only. The new rows were not run against mutants.
- **Phase 2.2:** nothing is polished below 44.1 kHz. `Designer::prepare` must run before the first `design` at every rate change.
- Still open, INFO only: the I-weak grid difference; the I-odd 0.001 dB WARN.

## Harness output: the Phase 2.1b block (verbatim, repo-relative paths)

```
  [PASS] P1s D10 shelf re-centre continuous across 0.95 pi and across pair Q 1.5 and 4 (x (1 +- 1e-9), harness literals) worst relative change: seed parameters 3.27e-08, design-side pair 2e-09 (limit 1e-6); 8/8 frequency crossings, 24/24 Q-boundary checks
  [PASS] P1s D10 shelf re-centre region: 10 000 random shelf / tilt settings x 8 rates (buildPrototype, harness literals) 499568 pairs: 0 changed outside the region, 0 true sections off the Phase 2.1 formula, 0 other differences; 3853 moved pairs, 0 of them bad (0.95 pi <= wT <= w0, Q clamp, magnitude at wT: worst 6.35e-16 relative over 3853, limit 1e-12)
  [PASS] T1 eleven 'Model results' rows reproduced within 0.05 dB           0 of 33 cells off; measured (expected) at +-6 / +-12 / +-30 dB below
           shelf48 Q1 10-20k           0.836 (0.84)  1.854 (1.85)  4.335 (4.33)

-- 2.1b / D2. the polish: constants and golden
           polish constants, read from Source/dsp/PolishConstants.h (PLAN-2.1b P10):
             fixedPoints 16, numTiedScales 2, tiedScales 0.5 / 1.5, tiedPoints 10, numPoints 26
             steps 2, extraSteps 2, numFree 6, lambda 0.001, mu 0.1
             seedKeepRms 0.002 dB, seedFullRms 0.006 dB, extensionRms 0.03 dB, acceptRamp 0.1
             match band 20 Hz ... min (20000 Hz, 0.499 fs)
             stepLimit 0.35, minSampleRateHz 44100 Hz
           shelf re-centre constants, read from Source/dsp/Prototype.h: shelfRecentre 0.95 pi, shelfRecentreQLo 1.5, shelfRecentreQHi 4
  [PASS] D2 C++ design == golden designs-polished.tsv (1e-6 rel; mL cL 1e-9; weight 1e-6; response 1e-6 dB) 88935 data lines (C++ list: 88935), 0 unparsed, 0 shape; 662970 cells, 0 out of tolerance, worst rel 4.49e-13; weights: 0 off, worst 2.96e-08; 30723 bands, 0 off, worst 1.1e-12 dB
           worst cell: line 31007 (lowShelf fs 48000 slope-index 1 fc 12000 q 4 gain -30 section 0) column G
           worst band response ends at: line 290 (bell fs 44100 slope-index 1 fc 12000 q 40 gain -60 section 0)

-- 2.1b / G1b. tightened magnitude match, and its negative controls
  [PASS] G1b-bell bells, Q 0.025-40: 0.05 + 0.012 |g|; >= 88.2 k 0.03 + 0.002 |g| polished; five rates, fc 20 Hz-20 kHz x 40, +-3/6/12/18/30 dB
           fs  44100  worst/tol:  +-3 dB 0.021/0.086  +-6 dB 0.043/0.122  +-12 dB 0.081/0.194  +-18 dB 0.114/0.266  +-30 dB 0.193/0.410
           fs  48000  worst/tol:  +-3 dB 0.013/0.086  +-6 dB 0.016/0.122  +-12 dB 0.026/0.194  +-18 dB 0.033/0.266  +-30 dB 0.081/0.410
           fs  88200  worst/tol:  +-3 dB 0.008/0.036  +-6 dB 0.009/0.042  +-12 dB 0.011/0.054  +-18 dB 0.012/0.066  +-30 dB 0.013/0.090
           fs  96000  worst/tol:  +-3 dB 0.008/0.036  +-6 dB 0.009/0.042  +-12 dB 0.010/0.054  +-18 dB 0.010/0.066  +-30 dB 0.010/0.090
           fs 192000  worst/tol:  +-3 dB 0.007/0.036  +-6 dB 0.007/0.042  +-12 dB 0.007/0.054  +-18 dB 0.008/0.066  +-30 dB 0.008/0.090
           model, polished at +-6 / +-12 / +-30 dB: 44.1 k 0.043 / 0.081 / 0.193, 48 k 0.016 / 0.026 / 0.073, 88.2 k up <= 0.013
  [PASS] G1b-sh12 shelves 12 dB/oct, Q <= 1.41, low and high: same tolerance polished; five rates, fc 20 Hz-20 kHz x 40, +-3/6/12/18/30 dB
           fs  44100  worst/tol:  +-3 dB 0.026/0.086  +-6 dB 0.044/0.122  +-12 dB 0.120/0.194  +-18 dB 0.199/0.266  +-30 dB 0.186/0.410
           fs  48000  worst/tol:  +-3 dB 0.013/0.086  +-6 dB 0.013/0.122  +-12 dB 0.031/0.194  +-18 dB 0.056/0.266  +-30 dB 0.079/0.410
           fs  88200  worst/tol:  +-3 dB 0.007/0.036  +-6 dB 0.008/0.042  +-12 dB 0.009/0.054  +-18 dB 0.009/0.066  +-30 dB 0.012/0.090
           fs  96000  worst/tol:  +-3 dB 0.007/0.036  +-6 dB 0.008/0.042  +-12 dB 0.009/0.054  +-18 dB 0.009/0.066  +-30 dB 0.010/0.090
           fs 192000  worst/tol:  +-3 dB 0.004/0.036  +-6 dB 0.007/0.042  +-12 dB 0.007/0.054  +-18 dB 0.007/0.066  +-30 dB 0.008/0.090
           model at 44.1 k, +-6 / +-12 / +-30 dB: 0.04 / 0.12 / 0.19
  [PASS] G1b-shN shelves 24 / 36 / 48 / 72 / 96 dB/oct, Q 1.0, low and high: same tolerance polished; five rates, fc 20 Hz-20 kHz x 40, +-3/6/12/18/30 dB; every slope passes
           24 dB/oct  fs  44100  worst/tol:  +-3 dB 0.027/0.086  +-6 dB 0.038/0.122  +-12 dB 0.092/0.194  +-18 dB 0.107/0.266  +-30 dB 0.187/0.410
           24 dB/oct  fs  48000  worst/tol:  +-3 dB 0.006/0.086  +-6 dB 0.008/0.122  +-12 dB 0.020/0.194  +-18 dB 0.027/0.266  +-30 dB 0.064/0.410
           24 dB/oct  fs  88200  worst/tol:  +-3 dB 0.003/0.036  +-6 dB 0.004/0.042  +-12 dB 0.006/0.054  +-18 dB 0.004/0.066  +-30 dB 0.005/0.090
           24 dB/oct  fs  96000  worst/tol:  +-3 dB 0.003/0.036  +-6 dB 0.004/0.042  +-12 dB 0.006/0.054  +-18 dB 0.006/0.066  +-30 dB 0.004/0.090
           24 dB/oct  fs 192000  worst/tol:  +-3 dB 0.000/0.036  +-6 dB 0.000/0.042  +-12 dB 0.000/0.054  +-18 dB 0.000/0.066  +-30 dB 0.001/0.090
           36 dB/oct  fs  44100  worst/tol:  +-3 dB 0.025/0.086  +-6 dB 0.037/0.122  +-12 dB 0.068/0.194  +-18 dB 0.091/0.266  +-30 dB 0.173/0.410
           36 dB/oct  fs  48000  worst/tol:  +-3 dB 0.008/0.086  +-6 dB 0.010/0.122  +-12 dB 0.015/0.194  +-18 dB 0.023/0.266  +-30 dB 0.032/0.410
           36 dB/oct  fs  88200  worst/tol:  +-3 dB 0.004/0.036  +-6 dB 0.003/0.042  +-12 dB 0.005/0.054  +-18 dB 0.005/0.066  +-30 dB 0.005/0.090
           36 dB/oct  fs  96000  worst/tol:  +-3 dB 0.003/0.036  +-6 dB 0.003/0.042  +-12 dB 0.005/0.054  +-18 dB 0.004/0.066  +-30 dB 0.006/0.090
           36 dB/oct  fs 192000  worst/tol:  +-3 dB 0.000/0.036  +-6 dB 0.000/0.042  +-12 dB 0.000/0.054  +-18 dB 0.000/0.066  +-30 dB 0.001/0.090
           48 dB/oct  fs  44100  worst/tol:  +-3 dB 0.026/0.086  +-6 dB 0.043/0.122  +-12 dB 0.079/0.194  +-18 dB 0.098/0.266  +-30 dB 0.153/0.410
           48 dB/oct  fs  48000  worst/tol:  +-3 dB 0.016/0.086  +-6 dB 0.017/0.122  +-12 dB 0.027/0.194  +-18 dB 0.036/0.266  +-30 dB 0.057/0.410
           48 dB/oct  fs  88200  worst/tol:  +-3 dB 0.005/0.036  +-6 dB 0.004/0.042  +-12 dB 0.005/0.054  +-18 dB 0.007/0.066  +-30 dB 0.008/0.090
           48 dB/oct  fs  96000  worst/tol:  +-3 dB 0.003/0.036  +-6 dB 0.003/0.042  +-12 dB 0.003/0.054  +-18 dB 0.005/0.066  +-30 dB 0.006/0.090
           48 dB/oct  fs 192000  worst/tol:  +-3 dB 0.000/0.036  +-6 dB 0.000/0.042  +-12 dB 0.000/0.054  +-18 dB 0.000/0.066  +-30 dB 0.000/0.090
           72 dB/oct  fs  44100  worst/tol:  +-3 dB 0.028/0.086  +-6 dB 0.052/0.122  +-12 dB 0.109/0.194  +-18 dB 0.161/0.266  +-30 dB 0.247/0.410
           72 dB/oct  fs  48000  worst/tol:  +-3 dB 0.026/0.086  +-6 dB 0.028/0.122  +-12 dB 0.047/0.194  +-18 dB 0.075/0.266  +-30 dB 0.096/0.410
           72 dB/oct  fs  88200  worst/tol:  +-3 dB 0.005/0.036  +-6 dB 0.007/0.042  +-12 dB 0.005/0.054  +-18 dB 0.008/0.066  +-30 dB 0.009/0.090
           72 dB/oct  fs  96000  worst/tol:  +-3 dB 0.003/0.036  +-6 dB 0.006/0.042  +-12 dB 0.005/0.054  +-18 dB 0.007/0.066  +-30 dB 0.010/0.090
           72 dB/oct  fs 192000  worst/tol:  +-3 dB 0.000/0.036  +-6 dB 0.000/0.042  +-12 dB 0.000/0.054  +-18 dB 0.000/0.066  +-30 dB 0.000/0.090
           96 dB/oct  fs  44100  worst/tol:  +-3 dB 0.043/0.086  +-6 dB 0.059/0.122  +-12 dB 0.119/0.194  +-18 dB 0.205/0.266  +-30 dB 0.214/0.410
           96 dB/oct  fs  48000  worst/tol:  +-3 dB 0.038/0.086  +-6 dB 0.034/0.122  +-12 dB 0.059/0.194  +-18 dB 0.087/0.266  +-30 dB 0.118/0.410
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

-- 2.1b / C1. continuity (1101 + 216 lines of 40 000 settings each, threaded)
  [PASS] C1 continuity: block A (320 random lines + 21 fixed sweeps) + block B (760 random lines), 8000 and 32000 steps 1101 lines, 0 violations (0 non-finite, 0 not shrinking); worst polished / seed jump 2.167 (limit 4), 1 lines above 2x; worst jump (32000) / jump (8000) 0.295 (limit 0.5); research: about 1 line in 1000 above 2x, at most 2.4x, shrinking 4x
           worst polished / seed: random shelf 18 dB/oct #16 (lowShelf, slope-index 2, fs 44100): fc 249.84469 -> 12019.7162 Hz, gain -8.3730318 -> -59.4538555 dB, Q 0.448156539 -> 4.13716238
           worst 32000 / 8000:    random shelf 48 dB/oct #2 (lowShelf, slope-index 5, fs 44100): fc 23611.57 -> 11.5678476 Hz, gain 37.666369 -> -18.3506847 dB, Q 28.4559365 -> 0.279256067
           failed in the first build: random shelf 48 dB/oct #27 at 48000 Hz: seed 0.101932 / 0.025481 dB, polished 0.099847 / 0.024961 dB at 8000 / 32000 steps; polished / seed 0.980, 32000 / 8000 0.250: within both limits
           failed in the first build: random shelf 48 dB/oct #2 at 44100 Hz: seed 0.606497 / 0.151763 dB, polished 0.688055 / 0.203264 dB at 8000 / 32000 steps; polished / seed 1.339, 32000 / 8000 0.295: within both limits
  [PASS] C1-hi continuity at 88.2 / 96 / 192 kHz: 216 random lines (9 families x 3 rates x 8), 8000 and 32000 steps 216 lines, 0 violations (0 non-finite, 0 not shrinking); worst polished / seed jump 1.010 (limit 4), 0 lines above 2x; worst jump (32000) / jump (8000) 0.252 (limit 0.5); research (step limit 0.5 only): 0 of 1080
           worst polished / seed: high-rate tilt 48 dB/oct #4 (tilt, slope-index 5, fs 96000): fc 742.849033 -> 26153.2986 Hz, gain 44.4948999 -> 55.066575 dB, Q 0.784172088 -> 4.75789494
           worst 32000 / 8000:    high-rate bell #4 (bell, slope-index 1, fs 192000): fc 1092.64594 -> 8878.95084 Hz, gain -55.9141549 -> 44.1810017 dB, Q 38.8703329 -> 0.0369238188
  [PASS] C1n the C1 test FAILS a binary extension gate (designWith<BinaryExtension>) on a line the shipped design passes 181 lines, 13 violations (0 non-finite, 13 not shrinking); worst polished / seed jump 33.902 (limit 4), 15 lines above 2x; worst jump (32000) / jump (8000) 1.067 (limit 0.5); fixed sweeps + block A random shelf lines; 0 lines the shipped design fails left out
           worst polished / seed: fixed Q sweep #19 (lowShelf, slope-index 1, fs 44100): fc 16000 -> 16000 Hz, gain 18 -> 18 dB, Q 0.025 -> 40
           worst 32000 / 8000:    fixed gain sweep #12 (bell, slope-index 1, fs 44100): fc 19000 -> 19000 Hz, gain 0.001 -> 60 dB, Q 8 -> 8
           first violation:       random shelf 12 dB/oct #12 (lowShelf, slope-index 1, fs 44100): fc 9852.31699 -> 250.606273 Hz, gain 28.3214203 -> 12.7005966 dB, Q 14.9695065 -> 1.12201954: seed 0.044022 / 0.011006 dB, polished 0.167368 / 0.141016 dB at 8000 / 32000 steps

-- 2.1b / NW2, RA1. random full range: never far worse than the seed, no runaway fit (threaded)
  [PASS] NW2 random full range, per gain2 section: polished max error <= seed max error + 1.0 dB (44.1 / 48 kHz) worst (polished - seed) +0.660 dB at 44.1 kHz, +0.270 dB at 48 kHz; 2598194 / 2598194 sections (research: +0.67 dB over 2.8 million sections at 44.1 kHz)
           44.1 k: 1000000 bands, 2598194 gain2 sections, 28.9 % with weight > 0, 0 non-finite; worst (polished - seed) +0.660 dB, 0 above +1 dB; above +0.01 dB: 294 of 31053 with Q >= 4 and the upper pair above 0.9 Nyquist (worst +0.660), 55 of 2567141 in the rest (worst +0.126); runaway 0
             worst (polished - seed) at: lowShelf slope-index 7 fc 16235.6085 q 39.6857621 gain 59.771329 fs 44100 section 0: seed 1.0247, polished 1.6847 dB
           48.0 k: 1000000 bands, 2598194 gain2 sections, 26.9 % with weight > 0, 0 non-finite; worst (polished - seed) +0.270 dB, 0 above +1 dB; above +0.01 dB: 2 of 26841 with Q >= 4 and the upper pair above 0.9 Nyquist (worst +0.254), 38 of 2571353 in the rest (worst +0.270); runaway 0
             worst (polished - seed) at: lowShelf slope-index 7 fc 16235.6085 q 39.6857621 gain 59.771329 fs 48000 section 0: seed 0.5320, polished 0.8018 dB
  [PASS] RA1 on NW2's draw (44.1 / 48 kHz): no section with weight > 0 has g or k more than 10x from the seed's 0 runaway of 1451300 sections with weight > 0 (research: 0; the first build about 1000 per rate)
  [INFO] NW2 / RA1 on the same draw at 96 kHz (not gated)                   
           96.0 k: 1000000 bands, 2598194 gain2 sections, 13.1 % with weight > 0, 0 non-finite; worst (polished - seed) +0.000 dB, 0 above +1 dB; above +0.01 dB: 0 of 3580 with Q >= 4 and the upper pair above 0.9 Nyquist (worst +0.000), 0 of 2594614 in the rest (worst +0.000); runaway 0
             worst (polished - seed) at: lowShelf slope-index 7 fc 493.558774 q 22.6522105 gain 36.5907557 fs 96000 section 0: seed 0.0000, polished 0.0000 dB

-- 2.1b / PR1, F2, S1, TL2. rate gate, robustness, skip exactness, tilt identity
  [PASS] PR1 no polish below 44.1 kHz: 10 000 mixed settings at 22.05 and 32 kHz, design == designSeed (memcmp), every weight 0 20000 designs, 0 differ, 0 with a weight other than 0, isPolishEnabled false at both rates; the same settings at 44.1 kHz: 1773 bands with a weight > 0
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
           10-20 k:     0.071  0.151  0.299   (research Q8, seed/polished: 0.48/0.06, 0.92/0.15, 1.64/0.28)  WARN: above the ceiling
  [INFO] I-weak resonant shelves, seed/polished band error at +-6, +-12, +-30 dB: fc <= 10 k | 10-20 k; polished ceiling at 44.1 kHz, 10-20 k: 1.0 / 1.5 / 1.0 dB 
           44.1 k  12 dB/oct, Q 2-10         0.14/0.04, 0.31/0.07, 3.32/0.25 | 3.44/0.14, 3.44/0.44, 3.36/0.44
                   bench, polished, 44.1 k: 0.04, 0.07-0.10, 0.25-0.39 | 0.18, 0.44, 0.45
           44.1 k  12 dB/oct, Q 20-40        0.14/0.04, 0.32/0.10, 3.56/0.39 | 3.83/0.94, 3.70/0.98, 3.56/0.42
                   bench, polished, 44.1 k: 0.04, 0.07-0.10, 0.25-0.39 | 0.75, 1.03, 0.43
           44.1 k  48 dB/oct, Q .25 .5 2 4   0.06/0.02, 0.11/0.03, 0.30/0.04 | 1.23/0.18, 3.55/0.13, 4.82/0.41
           44.1 k  12 dB/oct, Q 2-40, fc 20-30 k: 3.73/0.58, 3.65/0.22, 3.24/0.38   (bench, polished: 0.58, 0.22, 0.38)
           48.0 k  12 dB/oct, Q 2-10         0.07/0.04, 0.13/0.04, 2.51/0.12 | 2.74/0.11, 3.45/0.19, 3.36/0.17
           48.0 k  12 dB/oct, Q 20-40        0.07/0.05, 0.13/0.05, 2.84/0.16 | 3.08/0.42, 3.69/0.50, 3.56/0.16
           48.0 k  48 dB/oct, Q .25 .5 2 4   0.04/0.02, 0.08/0.03, 0.22/0.05 | 0.56/0.10, 1.20/0.14, 4.62/0.22
           48.0 k  12 dB/oct, Q 2-40, fc 20-30 k: 3.71/0.28, 3.64/0.11, 3.30/0.17
           the stubborn cell (PLAN-2.1b P4, reported, not gated): 12 dB/oct, Q 2-10, +-30 dB, 10-20 k at 44.1 kHz: 0.44 dB polished against the bench's 0.45 dB
           bench at 48 kHz: weak rows at or below 0.34 / 0.52 / 0.17 dB polished
  [INFO] I-new cases research did not run (new evidence; not gated)         
           Tilt as a type, Q 1, fc 20 Hz-20 kHz x 40, polished band error at +-6 / +-12 / +-30 dB:
             44.1 k   6 dB/oct:  0.173  0.312  0.490
             44.1 k  12 dB/oct:  0.043  0.117  0.131
             44.1 k  18 dB/oct:  0.057  0.144  0.282
             44.1 k  24 dB/oct:  0.036  0.088  0.187
             44.1 k  36 dB/oct:  0.036  0.068  0.173
             44.1 k  48 dB/oct:  0.043  0.079  0.153
             44.1 k  72 dB/oct:  0.052  0.109  0.239
             44.1 k  96 dB/oct:  0.059  0.119  0.191
             48.0 k   6 dB/oct:  0.173  0.312  0.490
             48.0 k  12 dB/oct:  0.008  0.022  0.032
             48.0 k  18 dB/oct:  0.059  0.117  0.280
             48.0 k  24 dB/oct:  0.007  0.015  0.051
             48.0 k  36 dB/oct:  0.010  0.015  0.032
             48.0 k  48 dB/oct:  0.017  0.027  0.057
             48.0 k  72 dB/oct:  0.028  0.047  0.084
             48.0 k  96 dB/oct:  0.034  0.059  0.082
           96 dB/oct shelves, Q 4 / 10 / 40, fc 20 Hz-20 kHz x 40, seed/polished at +-6, +-12, +-30 dB:
             44.1 k  0.94/0.30, 1.94/0.33, 6.09/0.31
             48.0 k  0.50/0.09, 1.00/0.17, 2.75/0.12
           bells with fc 20-30 kHz (re-centred seed), fc x 20, Q 0.025-40 x 17, gains +-3 ... +-30 dB, seed -> polished:
             44.1 k  2.70 -> 0.44 dB, 0 of 3400 settings worse than the seed by more than 1e-3 dB   (model, random sweep: 2.66 -> 0.40)
             48.0 k  2.69 -> 0.21 dB, 0 of 3400 settings worse than the seed by more than 1e-3 dB   (model, random sweep: 2.51 -> 0.17)
           22.05 and 32 kHz: nothing is polished below 44.1 kHz; design == designSeed bit for bit, every weight 0 (row PR1)
  [INFO] I-cost ns per section, whole Designer call (prototype + design), one thread, 44.1 kHz [checksum 6.36885e+06]
           (i)   seed, 400 000 random bells:                     90.1 ns   (research port, section only: 30-36 ns)
           (ii)  polished, the same bells:                      685.4 ns   39.2 % of sections with weight > 0   (model 0.8 us per polished section)
           (iii) polished, the 243265 bells whose weight is 0:    314.6 ns   (seed kept: skipped, or fitted and not accepted)
           (iv)  100 000 random 96 dB/oct shelves (8 sections): seed 32.8 ns, polished 426.2 ns per section; 16.4 % of sections with weight > 0

54 PASS, 0 FAIL, 12 INFO
```
