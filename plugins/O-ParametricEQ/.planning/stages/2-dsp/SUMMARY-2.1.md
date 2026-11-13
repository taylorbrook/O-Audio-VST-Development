---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.1 Summary"
date: 2026-11-13
phase: "2.1"
status: executed
harness: "28 PASS, 0 FAIL, 6 INFO"
---

# O-ParametricEQ — Stage 2 / Phase 2.1 Summary (Section designer and offline gate)

## Outcome

The closed-form section designer exists as four std-only C++17 headers in `Source/dsp/` and passes the offline gate. **Decision point: MLR passes G1 in C++** at the Stage 0 tolerance at 44.1, 48, 88.2, 96 and 192 kHz; the RBJ control fails it (17.39 dB at ±30 dB). No fallback was written and P8 was not used.

The C++ designer agrees with the Python reference model on all 1,176,175 golden cells **bit-exactly** (the gate allows 1e-9 relative). No processor code changed; the plugin binary is still 0.1.0. Nothing was installed and no tag was made.

## Tasks

| Task | Result |
|---|---|
| Quality gate 1-foundation → 2-dsp | PASSED (build, pluginval). The human code-review checkpoint was skipped with `--skip-review` and a recorded justification: the session is non-interactive, and the checkpoint's focus items concern `processBlock` code that this phase does not write. |
| T1 pre-flight | Green: three contract checksums match STATUS, branch `main`, `OUARICON_BUILD_TESTS=ON`, render harness 36 PASS / 0 FAIL, `check_spec.py` OK, numpy 2.4.0, target paths absent. |
| T2 model | `tests/model/eqmodel.py`, `dump_designs.py`, `README.md` (dsp-agent). |
| T3 golden | `tests/golden/designs.tsv`: 162,855 data rows + 2 comment lines, 29,884,142 bytes, no CR / nan / inf, second run byte-identical. Model self-check: the eleven RESEARCH Q1 rows within 0.01 dB and the four Q2 below-clamp cells within 0.02 dB (15 / 15). |
| T4–T6 designer | `Source/dsp/EqTypes.h`, `ResponseEval.h`, `Prototype.h`, `Designer.h` (dsp-agent). |
| T7 harness | `tests/designer-harness/main.cpp` (dsp-agent). |
| T8 build and gates | One configure (rc 0), build rc 0 with no warning in this plugin's paths on the first compile, harness 28 PASS / 0 FAIL / 6 INFO in 5.5 s, all controls fire, Stage 1 still 36 / 0, static gates clean. |
| T9 | This file, STATUS, path-scoped commit. |

## Negative controls (T8.5, T8.6)

| Control | Result |
|---|---|
| `--dump-designs` twice | Same SHA-1 both runs, and equal to the SHA-1 of the committed golden: the C++ dump is byte-identical to the model's file. |
| One `mB` cell changed in the 6th significant figure (data line 50,001) | Exit 1, D1 FAIL only (1 cell out of tolerance, relative 4.47e-06), 27 other rows PASS. |
| One line deleted | Exit 1, D1 FAIL only (162,854 lines against 162,855). |
| `--neg rbj` | rc 0: G1n fires. |
| `--neg f1` | rc 0: the checker catches the NaN and the −1. |
| `--neg alloc` | rc 0: the planted allocation is counted (1 allocation, 1 free). |
| Static greps on `Source/dsp`, CMake FP flags, processor wiring, processor diff | All four empty. |

## Notes for later phases

- **Z1 (Phase 2.2 neutral sleep):** at gain exactly 0 dB the parameters are the bit-exact identity in 16,020 of 16,020 sections, and the response is 0 dB everywhere on the grid.
- **Seed design time:** 33.9 ns per section (research port: 30–36 ns).
- **Thinnest G1 cell:** 6 dB/oct shelf at ±12 dB, 0.312 against 0.340 at 44.1 and 48 kHz, as research predicted (P9).
- **`trueProto` for tilt** carries the tilt scalar in the last section's `c`. Phase 2.1b's fit target for that section therefore includes it.
- **First-order `AnalogSection` convention:** `Ha = c · (wp / wz) · (s + wz) / (s + wp)`, `c` = DC gain.

## Deviations and choices the plan did not fix

1. **P1 narrowing** (planned): the designer is std-only. `ResponseEval.h` also includes `<complex>`, which P1's list omits and T4 requires.
2. **P3** (planned): every gain-bearing band is designed on the boost side and inverted.
3. **G2s, High Cut stop-band slope:** 11 of 72 cells are skipped because the −50 dB point lies above 20 kHz (all nine 6 dB/oct cells and two 12 dB/oct cells). All 72 Low Cut cells and 61 High Cut cells are measured; worst 0.545 dB/oct against 1.0. The plan and ARCHITECTURE G2 do not say how to treat unmeasurable cells. **The 6 dB/oct High Cut slope is therefore not gated by this row** (its corner gain is). For verify to judge.
4. **T1 and NB1–NB4 are scored on the research grids** (3000 points from 10 Hz; 4000 points, 25 Q, 40 centres, −40 dB mask) without tied points, so the expected values and ceilings stay valid (risk R5). Other rows use the P6 grid.
5. **P1 clamp reference** is designed at `clamp × (1 + 1e-9)`; the exact-clamp design is also bit-identical (3 / 3 notch, 3 / 3 band pass).
6. **CMake:** the harness target also gets `-O3` and an explicit warning set (`-Wall -Wextra -Wconversion -Wshadow -Wsign-conversion -Wfloat-equal`), `CXX_EXTENSIONS OFF`, and links `Threads::Threads` for B1. No FP option.
7. **E1** adds a complex-response check (2.35e-11 relative) beside the magnitude check.
8. **Golden format:** a second `#` comment line holds the column header; flags are `1` / `0`.
9. **Golden size:** 30 MB. Raised with Taylor before the commit; decision: commit as planned.
10. **Review of the agent's code:** the headers and harness were not read line by line by the execute session. Their correctness rests on the gates: bit-exact agreement with a model that was itself tied back to the research tables, and scoring against analog prototypes.

## Harness output (verbatim, repo-relative paths)

```
O-ParametricEQ designer harness (Stage 2, Phase 2.1)  golden dir: plugins/O-ParametricEQ/tests/golden

-- D. golden and evaluator
  [PASS] D1 C++ designs == golden designs.tsv (1e-9 rel; classes, flags, line count) 162855 data lines (C++ list: 162855), 1176175 cells, 0 out of tolerance, 0 shape, worst rel 0, 1176175 bit-exact
  [PASS] E1 ResponseEval == impulse through the recurrences (<= 1e-6 dB above -100 dB) worst 1.66e-10 dB, complex 2.35e-11 rel (limit 1.2e-7), 2376 points, 40 designs

-- G1. magnitude match against the analog prototypes
  [PASS] G1-bell bells, Q 0.025-40: 0.15 + 0.05 |g|; >= 88.2 k 0.10 + 0.015 |g| five rates, fc 20 Hz-20 kHz x 40, +-3/6/12/18/30 dB
           fs  44100  worst/tol:  +-3 dB 0.174/0.300  +-6 dB 0.338/0.450  +-12 dB 0.612/0.750  +-18 dB 0.798/1.050  +-30 dB 0.975/1.650
           fs  48000  worst/tol:  +-3 dB 0.118/0.300  +-6 dB 0.227/0.450  +-12 dB 0.396/0.750  +-18 dB 0.491/1.050  +-30 dB 0.573/1.650
           fs  88200  worst/tol:  +-3 dB 0.034/0.145  +-6 dB 0.066/0.190  +-12 dB 0.111/0.280  +-18 dB 0.136/0.370  +-30 dB 0.153/0.550
           fs  96000  worst/tol:  +-3 dB 0.031/0.145  +-6 dB 0.059/0.190  +-12 dB 0.101/0.280  +-18 dB 0.122/0.370  +-30 dB 0.137/0.550
           fs 192000  worst/tol:  +-3 dB 0.009/0.145  +-6 dB 0.017/0.190  +-12 dB 0.029/0.280  +-18 dB 0.035/0.370  +-30 dB 0.040/0.550
  [PASS] G1-sh12 shelves 12 dB/oct, Q <= 1: 0.15 + 0.06 |g|; >= 88.2 k 0.10 + 0.015 |g| five rates, fc 20 Hz-20 kHz x 40, +-3/6/12/18/30 dB
           fs  44100  worst/tol:  +-3 dB 0.187/0.330  +-6 dB 0.368/0.510  +-12 dB 0.689/0.870  +-18 dB 0.932/1.230  +-30 dB 1.188/1.950
           fs  48000  worst/tol:  +-3 dB 0.159/0.330  +-6 dB 0.317/0.510  +-12 dB 0.615/0.870  +-18 dB 0.866/1.230  +-30 dB 1.160/1.950
           fs  88200  worst/tol:  +-3 dB 0.012/0.145  +-6 dB 0.024/0.190  +-12 dB 0.046/0.280  +-18 dB 0.065/0.370  +-30 dB 0.089/0.550
           fs  96000  worst/tol:  +-3 dB 0.012/0.145  +-6 dB 0.023/0.190  +-12 dB 0.044/0.280  +-18 dB 0.062/0.370  +-30 dB 0.086/0.550
           fs 192000  worst/tol:  +-3 dB 0.004/0.145  +-6 dB 0.008/0.190  +-12 dB 0.016/0.280  +-18 dB 0.023/0.370  +-30 dB 0.031/0.550
  [PASS] G1-sh6 shelves 6 dB/oct: 0.10 + 0.02 |g|; >= 88.2 k 0.10 + 0.015 |g| five rates, fc 20 Hz-20 kHz x 40, +-3/6/12/18/30 dB
           fs  44100  worst/tol:  +-3 dB 0.089/0.160  +-6 dB 0.173/0.220  +-12 dB 0.312/0.340  +-18 dB 0.405/0.460  +-30 dB 0.490/0.700
           fs  48000  worst/tol:  +-3 dB 0.089/0.160  +-6 dB 0.173/0.220  +-12 dB 0.312/0.340  +-18 dB 0.405/0.460  +-30 dB 0.490/0.700
           fs  88200  worst/tol:  +-3 dB 0.061/0.145  +-6 dB 0.118/0.190  +-12 dB 0.212/0.280  +-18 dB 0.275/0.370  +-30 dB 0.333/0.550
           fs  96000  worst/tol:  +-3 dB 0.053/0.145  +-6 dB 0.103/0.190  +-12 dB 0.185/0.280  +-18 dB 0.241/0.370  +-30 dB 0.291/0.550
           fs 192000  worst/tol:  +-3 dB 0.015/0.145  +-6 dB 0.029/0.190  +-12 dB 0.052/0.280  +-18 dB 0.067/0.370  +-30 dB 0.081/0.550
  [PASS] G1n RBJ bilinear bell FAILS the G1-bell tolerance at 44.1 kHz      worst at +-30 dB 17.39 dB (model 13-17)
           fs  44100  worst/tol:  +-3 dB 2.427/0.300  +-6 dB 4.796/0.450  +-12 dB 9.183/0.750  +-18 dB 12.867/1.050  +-30 dB 17.392/1.650
  [PASS] T1 eleven 'Model results' rows reproduced within 0.05 dB           0 of 33 cells off; measured (expected) at +-6 / +-12 / +-30 dB below
           bell 44.1k fc<=16k          0.323 (0.32)  0.583 (0.58)  0.923 (0.92)
           bell 44.1k 16-20k           0.367 (0.37)  0.663 (0.66)  1.050 (1.05)
           bell 48k                    0.227 (0.23)  0.396 (0.40)  0.574 (0.57)
           bell 96k                    0.059 (0.06)  0.101 (0.10)  0.137 (0.14)
           bell 44.1k 20-30k           0.954 (0.95)  1.722 (1.72)  2.698 (2.70)
           shelf12 Q1 fc<=10k          0.051 (0.05)  0.122 (0.12)  0.600 (0.60)
           shelf12 Q1 10-20k           0.368 (0.37)  0.689 (0.69)  1.189 (1.19)
           shelf12 Q.25-1.41 10-20k    0.657 (0.66)  1.167 (1.17)  1.487 (1.49)
           shelf6 44.1k fc 10-30k      0.173 (0.17)  0.312 (0.31)  0.490 (0.49)
           shelf48 Q1 10-20k           0.836 (0.84)  1.717 (1.72)  4.006 (4.01)
           shelf12 Q1 20-30k           0.431 (0.43)  0.777 (0.78)  1.217 (1.22)

-- R / N / F. reciprocity, precision, robustness
  [PASS] R1 |H(+g) H(-g)| = 1 within 1e-9 dB (bells; shelves and tilt, 8 slopes) bells 4.62e-13 dB, shelves / tilt 1.25e-13 dB, five rates
  [PASS] N1 10 Hz, Q 40, +30 dB bell at 192 kHz vs prototype, 2-50 Hz       2.95e-13 dB (limit 1e-6)
  [INFO] N1 the same bell with a naive 1 + a1 + a2 MatchedPair              4.83e-07 dB
  [PASS] F1 1e6 random settings x 8 rates + range ends: finite, g k G > 0, m c >= 0, count 8008192 designs, 0 bad, fewest draws per type 124563
  [PASS] F1n the F1 checker FAILS a NaN and a -1                            clean passes, NaN caught, -1 caught

-- G2 / decisions D6, D8, D9, D10
  [PASS] G2s stop-band slope between -20 and -50 dB within 1.0 dB/oct of 6.02 N worst 0.545 dB/oct; low cut 72/72 cells, high cut 61 measured, 11 skipped (-50 dB point above 20 kHz)
           high cut cells measured per slope index 0..7: 0 7 9 9 9 9 9 9 (of 9 each)
  [PASS] G2s corner gain at Q 1: -3.01 +- 0.10 dB (8 slopes, both cuts)     worst 0.0194 dB, 144 cells
  [PASS] G2s corner gain at Q 4, slopes >= 12 dB/oct: +9.03 +- 0.25 dB      worst 0.0194 dB, 126 cells
  [PASS] SL1 D6: Bell / Notch / Band Pass bit-identical across the 8 slope indices 140/140 comparisons, 20 settings
  [PASS] Q1 D9: at 6 dB/oct the five slope types are bit-identical across Q 0.025 / 1 / 40 80/80 comparisons
  [PASS] TL1 D8: tilt design DC gain is -g/2 (8 slopes, g +-6 / +-18)       worst 6.13e-14 dB
  [PASS] TL1 D8: true prototype high-frequency limit is +g/2                worst 2.93e-14 dB
  [PASS] TL1 D8: 0 dB at the pivot (12 dB/oct, Q 1, fc <= 10 kHz, 44.1 kHz) worst 0.0257 dB, 0.02 of the G1-sh12 tolerance
  [PASS] Z1 gain exactly 0 dB: response within 1e-12 dB of 0 on the grid    worst 0 dB, 5760 designs; parameters bit-exact identity in 16020 of 16020 sections (ALL)
  [PASS] P1 D10: bell parameters continuous across 0.95 pi (fc x (1 +- 1e-9)) worst relative change 3.08e-08 (limit 1e-6)
  [PASS] P1 D10: notch 21/25/30 k and band pass 20/25/30 k == the design at the clamp (memcmp) 18/18; notch at exactly 20947.5 Hz: 3/3 same bits; band pass at exactly 19845.0 Hz: 3/3 same bits

-- NB. notch and band pass below their clamps
  [PASS] NB1 notch, fc <= 10 k, 44.1 kHz                                    0.628 dB (ceiling 0.75, model 0.63)
  [PASS] NB2 notch, 10 k - clamp (0.95 Nyquist), 44.1 kHz                   1.271 dB (ceiling 1.45, model 1.27)
  [PASS] NB3 band pass, fc <= 10 k, 44.1 kHz                                1.453 dB (ceiling 1.65, model 1.45)
  [PASS] NB4 band pass, 10 k - clamp (0.9 Nyquist), 44.1 kHz                2.918 dB (ceiling 3.30, model 2.92)

-- A / B. audio-thread fitness
  [PASS] A1 1e5 design() calls: 0 allocations, 0 frees (malloc_logger, this thread) 0 allocations, 0 frees; hook live (planted: 1 / 1); 248633 sections designed
  [PASS] B1 10 000 mixed settings: main thread == second thread with its own Designer (memcmp) 10000/10000

-- INFO (never failing)
  [INFO] notch / band pass against the TRUE prototype (RESEARCH-2.1 Q2), error where it is above -40 dB 
           notch     44.1 k: fc<=10k 0.63 | 10k-clamp 1.27 | clamp-25k vs TRUE prototype 13.57 | 25-30k 18.92 dB
           band pass 44.1 k: fc<=10k 1.45 | 10k-clamp 2.92 | clamp-25k vs TRUE prototype 25.43 | 25-30k 30.63 dB
           notch     48.0 k: fc<=10k 0.51 | 10k-clamp 1.27 | clamp-25k vs TRUE prototype 5.52 | 25-30k 10.87 dB
           band pass 48.0 k: fc<=10k 1.02 | 10k-clamp 2.92 | clamp-25k vs TRUE prototype 9.52 | 25-30k 14.86 dB
  [INFO] resonant shelves 12 dB/oct at 44.1 kHz, +-6 / +-12 / +-30 dB (RESEARCH-2.1 Q8 weak rows) 
           Q 2-10   fc <= 10 k: 0.14 / 0.31 / 4.43   10-20 k: 4.96 / 7.19 / 7.69
           Q 20-40  fc <= 10 k: 0.14 / 0.32 / 6.75   10-20 k: 7.42 / 11.69 / 14.91
  [INFO] shelves >= 24 dB/oct, Q 1, fc 10-20 k at 44.1 kHz, +-6 / +-12 / +-30 dB (model 0.63/1.24/2.31, 0.84/1.72/4.01, 0.87/1.77/4.91) 
           24 dB/oct: 0.63 / 1.24 / 2.31   48 dB/oct: 0.84 / 1.72 / 4.01   96 dB/oct: 0.87 / 1.77 / 4.91
  [INFO] cut skirts at 44.1 kHz, Q 1 (ARCHITECTURE weak spots: 1.1; 2.1 / 1.2; 3.6 / 5.7; 0.09 / 0.00 / 0.00) 
           high cut 12 dB/oct, fc <= 10 k, down to -80 dB: 1.10
           high cut 48 / 96 dB/oct, top 30 dB, fc <= 10 k: 2.11 / 1.17   fc 10-20 k: 3.63 / 5.69
           low cut 12 / 48 / 96 dB/oct, fc <= 10 k, down to -80 dB: 0.09 / 0.00 / 0.00
  [INFO] seed design time per section (400 000 random bells, 44.1 kHz)      33.9 ns  (research port 30-36 ns)  [checksum 2.52535e+06]

28 PASS, 0 FAIL, 6 INFO
```

## Addendum 2026-11-13 (Phase 2.1b rework)

The closed-form seed changed for shelf / tilt second-order sections that have a pair (zeros or poles) above 0.95 π with Q above 1.5: the pair is designed lower with an effective Q (`detail::recentrePair`, `SUMMARY-2.1b-rework.md`). Nothing else changed.

- `designs.tsv` regenerated: 4800 of 162 855 lines differ, all `gain2` lines of Low Shelf, High Shelf and Tilt (1600 each), every one inside that region and every line of the region; D1 is bit-exact again.
- T1: the line "Shelf 48 dB/oct, Q 1, 10–20 k" at 44.1 kHz moves from 0.84 / 1.72 / 4.01 to 0.84 / 1.85 / 4.33; the ARCHITECTURE table and the row were amended on Taylor's call. The other ten lines are unchanged.
- INFO, shelves ≥ 24 dB/oct, Q 1, 10–20 k at 44.1 kHz: 48 dB/oct 0.84 / 1.72 / 4.01 → 0.84 / 1.85 / 4.33; 96 dB/oct 0.87 / 1.77 / 4.91 → 0.87 / 1.77 / 5.04; 24 dB/oct unchanged.
- G1-bell, G1-sh12, G1-sh6, R1, N1, G2s, CS1, CS2, NB1–NB4 print the same figures as before.
