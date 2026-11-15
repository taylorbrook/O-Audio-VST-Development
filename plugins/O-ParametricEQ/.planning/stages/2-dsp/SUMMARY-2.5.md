---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.5 Summary"
date: 2026-11-14
phase: "2.5"
status: executed
version: "0.5.0"
designer_harness: "131 PASS, 0 FAIL, 17 INFO (default and --long)"
render_harness: "122 PASS, 0 FAIL (default and --long)"
---

# O-ParametricEQ — Stage 2 / Phase 2.5 Summary (output stage and auto gain)

## Outcome

**Complete. Designer harness 131 PASS / 0 FAIL / 17 INFO; render harness 122 PASS / 0 FAIL; both also with `--long`.**

- **Output gain** (±36 dB) and **auto gain** are applied as one multiply between the bands and the bypass mix. Each gain uses a four-pole one-pole cascade in linear amplitude (20 ms / 100 ms).
- **Auto gain** is the negative pink-weighted mean power gain of the target curve on 512 cell midpoints (C1). It is refreshed on the tick clock, and the estimate is a pure function of the bands' requests: offline and real-time renders are bit-identical (XR-blk5, XO-agid).
- **Idle stage.** With output gain at 0 dB and nothing for auto gain to do, the stage is idle and G4 (a) holds exactly as before (XR-g4a, `auto_gain` off and on).
- **Seeding.** The stage is seeded at the first block and at both un-bypass restarts, so nothing fades in (XR-seed). VERIFICATION-2.4 Issue 2 now has its row (XR-unb).
- **Meter.** Max-hold output peaks per channel, read and reset by `readAndResetOutputPeak` (XR-meter).
- **Build and validators.** 0.5.0 is built and installed. pluginval 10 passes (VST3 ×2, AU), `auval` passes, and the Standalone stays alive.

Three calls were Taylor's in the session (P1 amendment, S5 guard, XR-gstep κ window). See "Deviations".

No tag.

## Tasks

| Task | Result |
|---|---|
| T1 pre-flight | Contract checksums equal; branch `main`; plugin tree clean. Baselines 114 / 0 / 16 and 100 / 0; `check_spec.py` OK. Gate 1→2 PASSED (`--skip-review`, as in 2.1 – 2.4). It ran after T9, not at pre-flight (deviation 9). |
| T2 / T3 | New `Source/dsp/OutputStage.h` (`GainSmoother`, `OutputStage`, `outputGainTarget`, `sameGain`, constants) and `Source/dsp/AutoGain.h` (`AutoGain`, `autoGainRefreshTicks`, constants). No other `Source/dsp` file was touched (P1). |
| T4 designer rows | 17 rows and XO-cpu (INFO) under "2.5 / output stage and auto gain (double)", plus `--neg ag-alloc`. |
| T5 processor | `outputStage`, `autoGain`, `agWanted` / `agOn`, `refreshTicks`, `outputPeak`. `stageIdleForBlock()` is called at step 5 and step 6. `seedOutputStage()` runs at the step 4 restart and the step 7 restart. The tick hand-over and refresh run in the segment loop; the peak update runs after `processAudio` in both entry points. |
| T6 version | `OPEQ_VERSION 0.5.0`; `CHANGELOG.md` has `## [0.5.0] - 2026-11-14`. |
| T7 render rows | 21 rows and X-cpu4 (INFO). S5 amended (P12, then Taylor's guard call). P1 amended (Taylor). |
| T8 gates | Build rc 0, no warning in this plugin's files. Both harnesses 0 FAIL, default and `--long`. Eleven `--neg` controls rc 0. `check_spec.py` OK; `params.tsv` unchanged. Static greps clean. The `Source/dsp` diff is the two new headers only. In `AutoGain.h`, log / exp / pow appear only in `prepare` and in the final dB conversion. `stageIdleForBlock` has its definition and two calls. `gainRefRender`, `hardGainRender`, `linearGainRender` and `agRef` in both harnesses use none of `GainSmoother`, `OutputStage` or `AutoGain`. |
| T9 install + validators | Install rc 0; exactly the two `-dev` bundles, both 0.5.0. pluginval 10: VST3 `SUCCESS` ×2, AU `SUCCESS`. `auval -v aufx OuPQ OuDv`: `AU VALIDATION SUCCEEDED`, capabilities `[1, 1] [2, 2]`. Standalone rebuilt, `ALIVE`. No NaN or inf report. |
| T10 | This file, STATUS, PLUGINS.md, one path-scoped commit. |

## What was built

- **`OutputStage.h`**
  - `GainSmoother`: `y1 … y4` driven toward T with `a = 1 − exp (−13.0622 / N)`. Snaps when all four stages are within 1e-9; `setTarget` only stores T. No clamp (convex combination).
  - `OutputStage`: `prepare` sets N = round (0.020 fs) / round (0.100 fs). Methods `snap`, `setOutputPending`, `applyOutputPending`, `setAutoTarget`, `isUnity`.
  - `process (L, R, n)`: when settled, one constant, and exactly 1.0 touches nothing. Otherwise per sample `g = ag.next() * og.next()`. Either lane pointer may be null.
  - `outputGainTarget`: non-finite → 0 dB, clamp ±36 dB, `pow (10, dB / 20)`.
- **`AutoGain.h`**
  - Grid: 512 log midpoints over 40 Hz – min (16 kHz, 0.45 fs), `t2` set in `prepare`.
  - Per band: the |H|² product of its target design is cached, with `lastEval`, `contributes` and `evaluated`. A band is re-evaluated only when its desired input differs (`sameBandInput`). The lane products and the mean are recomputed from the cache on every call. Then the guards and the clamp to [−18, +12] dB.
  - `isDirty` is cheap (no design). `evaluateAll` forces every band.
  - Counters: evaluations, refreshes. About 220 KB fixed.
- **Processor** (PLAN-2.5 P5 – P7)
  - Per block: `output_gain` → `setOutputPending (outputGainTarget (raw))`; `auto_gain` → `agWanted`.
  - On a tick: band ticks, then `applyOutputPending`. On a refresh tick (tick index a multiple of R): `agOn = agWanted`, then `setAutoTarget (estimate or 1.0)`.
  - Order per segment: dry copy → bands → stage → bypass mix.
  - Step 5 returns only with an empty list **and** the stage idle. Step 6 converts every layout lane when the stage is not idle. At the step 7 restart the stage is seeded, then a jump is taken only when the list is empty and the stage is at unity.

## Results

### Designer harness (double)

| Row | Reading | Bench / limit |
|---|---|---|
| XO-step | Output gain 50 Hz 1.557x, 1 kHz 1.000x, κ 2.220x. Auto gain 1.010x / 1.000x / κ 1.208x | P4 1.56 / 1.00 / 2.22; limits 2.0 / 2.5 |
| XO-steph | 56 / 56 ≥ 10x. Lowest: output gain 17 336x (50 Hz) / 44.4x (1 kHz); auto gain 11 548x / 29.9x. LIN printed: 25.13x / 1.06x (output gain), 5.21x / 1.01x (auto gain) | 23 175x / 59.4x; LIN 25.1 |
| XO-ret | 0 / 64 above 2.0x. Output gain 1.579x (50 Hz), 1.000x (1 kHz); auto gain 1.010x | 1.58x |
| XO-sweep | 20 ms: 0.03x, lag 0.004 / 0.031 dB (block 32 / 512). 1 s: 0.77x / 0.78x | 0.03 / 0.78; ≤ 4.0x, lag ≤ 0.1 dB |
| XO-stress | 200 s: 0 samples outside the target range, 0 ≤ 0. Output gain −36.00 … +36.00 dB, auto gain −16.59 … +11.50 dB | — |
| XO-snap | Output gain 42.12 – 51.63 ms; auto gain 217.23 – 232.45 ms (224 runs, 8 rates); all exact afterwards | 45 – 52 / 222 – 232 |
| XO-ref / XO-refn | 960 renders bit-exact to `gainRefRender`. X = 12 fires 960 / 960; reversed order in float differs 960 / 960 | ≤ 1e-12 |
| XO-unity | 19 / 19 | exact |
| XO-ag | 0 / 68 outside. Worst 0.0551 dB (Bell +30 Q 10, 192 kHz); G12 shelf −6.1665 dB at all four rates | 0.055; 0.06 |
| XO-agref | 20 000 estimates, worst 0 dB | ≤ 1e-12 dB |
| XO-agrip (512) | Q 40 +12: 0.0280 dB; Q 10 +30: 0.1215 dB; Q 40 +30: 2.9212 dB. The 64-point control on Q 40 +12 reads 0.875 dB (fires) | 0.028 / 0.12 / 2.92 |
| XO-agid | 1000 sequences, 60 008 edits, 15 161 refreshes skipped: 0 mismatches against `evaluateAll` | bit-identical |
| XO-agz / XO-agg / XO-agsch | As stated. Guards reach +12 dB (mean 7.8e-46) and −18 dB (mean 3.3e+143); refresh ticks 29 / 43 / 59 / 64 | — |
| XO-alloc | 0 / 0 across 1250 refreshes and 30 000 band designs; hook live; `--neg ag-alloc` counts 1 / 1 | 0 |
| XO-cpu (INFO) | Bell 1.67 µs, 96 dB/oct low cut 6.38 µs, 96 dB/oct shelf 12.34 µs, refresh 2.15 µs | 1.5 / 6.1 / 12.1 / 1.7 |

### Render harness (float, through `processBlock`)

| Row | Reading |
|---|---|
| XR-g12o | 10 / 10, worst 1.59e-07 dB (limit ±0.05) |
| XR-g12s | ±6 dB → ±12 dB; `memcmp` equal to the raw ±12 dB bell |
| XR-g12a / d / n | output / input −0.006 dB; agRef −6.16653 dB (`eq::AutoGain` the same); two renders bit-identical; off: +6.16 dB (control fires) |
| XR-gstep | (a) 112 / 112, worst 1.565x output gain, 1.009x auto gain. (b) κ 56 / 56, worst 2.114x / 1.191x on the bench window (deviation 3). The 2.4 frame-grid figure, 7.226x, is printed as INFO. The four auto-gain band sets estimate exactly −18 / −6 / +6 / +12 dB |
| XR-gsteph | 56 / 56 ≥ 10x, weakest 42.8x output gain / 27.4x auto gain |
| XR-gret | 64 / 64, worst 1.582x (50 Hz), 1.000x (1 kHz) |
| XR-gsweep | 8 / 8. Output gain 0.014 – 1x. The auto-gain sweep (new, no bench figure) reads 1x on every reading, −140.5 dB from its reference |
| XR-ref | 644 / 644, worst −138.9 dB |
| XR-g4a / XR-g4an | 30 / 30 `memcmp` equal (auto gain off and on, mono / stereo, 7 block patterns, denormal tail); control 21 / 21 |
| XR-idle | 14 / 14; equal to the input from 38.75 ms after the second edit |
| XR-seed / XR-seedn | (i) −131.9 dB, constant +5.833 dB (+12 dB output gain, −6.167 dB auto gain); (ii) 2 / 2 `memcmp` equal; control 2 / 2 |
| XR-gbyp | 4 / 4; mix worst −150.3 dB; N = 960 |
| XR-unb / XR-unbn | (i) 4 / 4, (ii) 4 / 4, worst −146.7 dB; controls 4 / 4 + 4 / 4 (a fade one sample late reads −64.9 dB) |
| XR-blk5 | 24 / 24 bit-identical; 49 output-gain and 16 auto-gain events. Evaluations / refreshes are identical at every block size: 129 / 151 (44.1 kHz), 129 / 137 (48 kHz), 129 / 69 (96 kHz), 66 / 37 (192 kHz) |
| XR-meter | 6 / 6; every path reads 2.0 exactly; the read resets to 0 |
| A4 / A4n | 0 allocations / 0 frees in 222 834 windows (57 029 bypassed; 10 028 band evaluations in 793 refreshes); control 1 / 1 |
| X-cpu4 (INFO) | 24 × 96 dB/oct shelves with the gain swept every block: auto gain off 25.5 %, on 26.9 % (+1.4 %; bench +1.4 %). The four X-cpu3 cases with auto gain on are within about 1.3 % of X-cpu3 |

**S5** (amended), 0 non-finite samples everywhere:

| Rate | 600 s (`--long`): guard dB / seed attempt / peak re input / back to input | 60 s guard (default) |
|---|---|---|
| 22.05 kHz | 566.4 / 0 / +243.7 dB / 23.2 ms | 515.0 |
| 32 kHz | 563.9 / 0 / +244.1 / 208.0 ms | 509.5 |
| 44.1 kHz | 578.9 / 0 / +259.8 / 11.6 ms | 527.1 |
| 48 kHz | 559.7 / 0 / +254.4 / 10.7 ms | 509.7 |
| 88.2 kHz | 583.7 / 0 / +228.1 / 17.4 ms | 540.6 |
| 96 kHz | 600.0 (rounded; below the limit) / 0 / +219.4 / 10.7 ms | 575.8 |
| 176.4 kHz | 599.4 / 1 / +252.5 / 43.5 ms | 598.5 |
| 192 kHz | 593.6 / **43** / +249.7 / 194.7 ms | 587.4 |

The 10 s every-block runs pass at all 8 rates. In the default runs the slowest return to the input is 213.3 ms (limit 300 ms).

Run times: designer 85 s default, 4 m 25 s `--long`; render 1 m 24 s default, 15 m 35 s `--long`.

### Existing rows that write the gains

| Row | Reads now |
|---|---|
| P1 (processBlock "all random") | Failed 8 / 12 per layout before the amendment: the random output gain now reaches the output. Amended (deviation 1); 12 / 12. The processBlockBypassed variants keep the full draw and pass unchanged |
| T1a | Random auto gain, output gain forced to 0, no band: passes unchanged (the stage is idle) |
| S1 / S2 / S4 / S6 / S7 | State rows, no audio: unchanged. S2 now prints `pluginVersion 0.5.0` (T6) |
| S5 | Amended (P12 and deviation 2) |

Every other existing row in both harnesses prints the same verdict and figures as the baseline. The only differences are wall-clock INFO timings and S5's reseed INFO lines.

## Deviations

1. **P1 amended (Taylor's call, plan STOP).** The processBlock "all random" variant also sets `output_gain` to exactly 0 dB (raw store) and `auto_gain` off, after the 2.2 amendment's bands-off and bypass-off. The comment at the code names PLAN-2.5. Not in P12's list of amendments.
2. **S5 guard (Taylor's call, plan STOP).**
   - As written (stack + 12 + output gain + 12 < +600), `--long` had no seed in 32 attempts at 176.4 and 192 kHz (rejected guards 742 / 711 dB).
   - The +12 dB peak margin was never part of 2.4's guard (stack < +600); it came from RESEARCH Q5's wording. Guard now: stack + drawn output gain + 12 dB auto-gain allowance < +600.
   - Seed attempts 32 → 256 for S5's gain mode only; 192 kHz needed attempt 43. A3 is unchanged (32 attempts, 2.4 guard). A4 shares the guard function with 32 attempts.
3. **XR-gstep κ window (Taylor's call).** On 2.4's fixed 0.3 s frame grid, block-512 steps land mid-frame and read up to 7.2x. The gate now uses the bench's window: frames from 50 ms before the stage moves to N + 50 ms after. The grid figure is printed as INFO.
4. **`AutoGain::isDirty` / `estimate`:** an edit that keeps a band non-contributing (for example, to a disabled band) is not dirty; its `lastEval` is updated without a design. P4 says "any band whose desired input differs". The invariant holds (XO-agid: 4252 refreshes reported clean, none changed the value). Without this, every edit to a disabled band with auto gain on would convert every lane for up to one refresh.
5. **`contributes` excludes Side bands on a stereo layout** (they enter neither lane). Needed for XO-agz "Side-only → 0 designs". On mono every band contributes. `SlotBand::gainIsZero` is private, so the test is restated and the header says so.
6. **Exact compares in `OutputStage.h`** go through `sameGain (a, b)` = `!(a < b) && !(a > b)` (the `BandParams.h` `sameValue` form): the designer harness builds with `-Wfloat-equal`. `process` accepts a null L as well as a null R.
7. **Test accessors** `getAutoGainEvaluationCount()` / `getAutoGainRefreshCount()` were added to the processor (read-only) for XR-blk5's counts. P8 forbids test switches, not readers; `getBandDynOffsetDb` is the precedent.
8. **Step 7 jump** is `numListed == 0 && outputStage.isUnity()`, not a third `stageIdleForBlock` call. Just after seeding the two are equal (settled, nothing pending, `agOn == agWanted`, a fresh `evaluateAll`).
9. **The 1→2 gate ran after T9** instead of at pre-flight (omitted at T1). PASSED.
10. **Row interpretations** (designer):
    - XO-unity takes ±inf raw → 1.0 (P3: non-finite → 0 dB).
    - XO-ret runs the same four dB cases for auto gain on its 100 ms timescale.
    - XO-agrip adds the other five bells of the Q5 table; their limits are bench figure + half a digit, × 1.2.
    - XO-stress also redraws the auto-gain target at each refresh.
    - XO-alloc drives the stage and estimator without bands (XF-alloc covers the bands).
11. **Row interpretations** (render):
    - XR-gret and XR-unb cut host blocks at the edit samples.
    - Auto-gain steps are 400 ms apart.
    - The XR-gsweep output-gain reference is the per-sample automation × the gain-neutral output (the bench's reference).
    - Band values are set by raw stores where exact values matter (a normalised write rounded 6 → 6.0000005).

The engine headers and the processor were not changed after the first build, apart from the `sameGain` swap (6) and the accessors (7), both made before any 2.5 row ran.

## Carry-forward

- **2.6:** the listen path goes between the bands and the output stage. G6 is re-run with the analyzer on.
- **Stage 3:** the meter reader: `readAndResetOutputPeak` on the editor timer, with `VUMeterBridge` fed from the message thread. The vu-meter module is still only compile-proved.
- **Verify:**
  - Hands-on: auto gain on while enabling a +12 dB shelf (loud for about the smoother's time, then pulled back); sweeping a narrow +12 dB bell with auto gain on; output-gain automation.
  - Both `--long` runs once more.
  - A mutation run over `OutputStage.h`, `AutoGain.h` and the step-5 / step-6 predicate.
  - Re-check deviations 3 – 5.
- **VERIFICATION-2.4 Issue 1** (stop-band burst on a steep cut's slope change) is still open, waiting on the listen.
