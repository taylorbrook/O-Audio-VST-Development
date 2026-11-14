---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.3 Summary"
date: 2026-11-13
phase: "2.3"
status: executed
version: "0.3.0"
designer_harness: "88 PASS, 0 FAIL, 14 INFO (default and --long)"
render_harness: "83 PASS, 0 FAIL (default and --long)"
---

# O-ParametricEQ — Stage 2 / Phase 2.3 Summary (continuous modulation)

## Outcome

**Complete. Designer harness 88 PASS / 0 FAIL / 14 INFO (61 s; `--long` 147 s); render harness 83 PASS / 0 FAIL (34 s; `--long` 122 s).** Frequency, gain, Q and gain scale now ramp for 20 ms on a global control grid with per-sample interpolation of the section parameters; the filter state is kept. Version 0.3.0 is built and installed (VST3 + AU dev bundles); pluginval strictness 10 passes (VST3 twice, AU once), `auval` passes, the Standalone stays alive.

Every row passed on its first run as specified. No threshold, motion list or stimulus was changed, no existing check was amended, and nothing was escalated.

No tag.

## Tasks

| Task | Result |
|---|---|
| T1 pre-flight | Checksums equal; `main`; plugin tree clean; baseline 67 / 0 / 12 and 64 / 0; `check_spec.py` OK. |
| T2 / T3 engine | New `Source/dsp/ControlRamp.h`; `BandParams.h`, `SectionEngine.h`, `Band.h`, `Router.h` extended. After this step alone both harnesses still read 67 / 0 / 12 and 64 / 0. |
| T4 designer rows | 21 rows + 2 INFO under "2.3 / modulation (double)", `--long`, `--neg ramp-alloc`. |
| T5 processor | Tick clock, segment loop, non-finite check. |
| T6 version | `OPEQ_VERSION 0.3.0`; `CHANGELOG.md` `## [0.3.0] - 2026-11-13`. |
| T7 render rows | 19 rows + X-cpu2 (INFO), `--long`. |
| T8 gates | Build rc 0, no warning in this plugin's files; both harnesses 0 FAIL, default and `--long`; nine `--neg` controls rc 0; `check_spec.py` OK; `params.tsv` unchanged; static greps clean; the reference renders contain none of the ramp's code. |
| T9 install + validators | Install rc 0, exactly the two `-dev` bundles at 0.3.0. pluginval 10: VST3 `SUCCESS` ×2, AU `SUCCESS`. `auval -v aufx OuPQ OuDv`: `AU VALIDATION SUCCEEDED`, capabilities `[1, 1] [2, 2]`. Standalone `ALIVE`. |
| T10 | This file, STATUS, PLUGINS.md, one path-scoped commit. |

## What was built

- **`ControlRamp.h`**: `controlGridSize` (16 / 32 / 64), `rampLengthTicks` (28, 40, 55, 60, 55, 60, 55, 60 at the eight rates), `eq::ControlRamp` (ln f, effective gain, ln Q on one counter; counted arrival; the arrival design uses the target's own settings).
- **`Band.h`**: `setControlGrid`, `snapTo`, `snapToTarget`, `setTarget`, `controlTick`, `isMoving`, `stepCoefficients`, `processPair`. The position inside a control interval lives in the band, so an interval can be processed in any number of calls.
- **`SectionEngine.h`**: `rampSection` (increments, then `a1` by division from one `g, k`, then the settled recurrences). The settled functions are untouched.
- **`Router.h`**: P4 in the lane-pair form. Stereo calls `processPair`; the Mid and Side loops step the coefficients once per sample. The settled branches and the placement algebra are the 2.2 statements.
- **`BandParams.h`**: canonical gain 0 on Low Cut, High Cut, Notch, Band Pass; `offsetGainDb`; the lowest-Q comment corrected.
- **Processor**: a `uint64` clock advanced as the first statement of `processBlock` and `processBlockBypassed`; structural → `snapTo` + `reset`, continuous → `setTarget`; segments ending at the next tick; end-of-chunk non-finite check. Members added: the clock and K.

## Results

### G3 sweeps (RM-g3, M1): engine / same-motion reference, limit 4.0x

52 readings each. Double engine worst **2.54x**, float plugin worst **2.538x** (both motion 8, 50 Hz, block 32). The float readings carry over from double as research estimated.

| Motion (50 Hz tone) | Double, blocks 32 / 512 | Bench |
|---|---|---|
| 1 Bell f p0 | 1.00 / 0.99 | 1.00 / 0.99 |
| 2 Bell f p1 | 1.00 / 1.00 | 1.00 / 1.00 |
| 3 Bell gain | 0.94 / 0.98 | 0.94 / 0.98 |
| 4 Bell Q | 0.98 / 0.89 | 0.99 / 0.89 |
| 5, 6 Low Cut f p0 / p1 | 1.00 / 1.00 | 1.00 / 1.00 |
| 7 Low Cut Q | **2.17** / 0.96 | 2.17 / 0.96 |
| 8 High Shelf 12 f p0 | **2.54** / 1.05 | 2.54 / 1.06 |
| 9 High Shelf 12 f p1 | 1.01 / 1.02 | 1.01 / 1.02 |
| 10 High Shelf 12 gain | 0.97 / 0.96 | 0.97 / 0.96 |
| 11 High Shelf 12 Q | **1.67** / 0.88 | 1.93 / 0.88 |
| 12, 13 High Shelf 96 f p0 / p1 | 1.00 / 1.00 | 1.00 / 1.00 |

The 1 kHz readings are all 1.00x to 1.03x.

- **Stimulus guard (R3):** engine and reference first differ from their held renders at the same tick in 52 of 52; the render harness's reference first acts at 1040 / 1056 / 1024, as TK expects of the plugin.
- **RM-g3i (interpolation off):** 541x, 142.6x, 350x (bench 541, 143, 415).
- **RM-g3h / M1h (no smoothing, block 512):** 24 of 24 counted readings ≥ 10x, weakest 17.6x (motion 6 at 50 Hz). Void, printed: motion 5 at 50 Hz 9.2x, motion 7 at 1 kHz 8.1x.
- **RM-g3k / M1k:** 96 kHz 2.36x (float 2.33x); 192 kHz 1.25x (float 1.23x).
- **RM-g3r (held references, INFO):** **0 WARN**; largest of the 22 gated-for-WARN readings 2.93x (bench 2.93x); largest of all 52 is 2251x (bench 2251x).

### Kept state, arrival, identity

- **RM-arr:** 140 moving cases (8 types × 5 rates × f / gain / Q / all three) land bit for bit on the static design, `isMoving()` false, 0 designs over the next 100 ticks; every ramp makes exactly its ramp length of designs. 20 gain-only edits on gainless types start nothing. **RM-arrn:** 140 of 140 differ one tick earlier.
- **RM-keep:** 1.74x (bench 1.7x). **RM-keepn:** state cleared 3985x, 2.2 step 1353x. **M6** (plugin): 1.73x. **M6n** (Bell → Low Shelf): 3963x.
- **RM-blk / B1:** bit-identical at every block size and rate (double: 1, 32, 64, 512, 2048; plugin: 32, 64, 512, 2048, 20 of 20 checks). **RM-blkn:** first differing sample 2048 at every rate.
- **TK:** 9 of 9. First differing sample 1040 / 1056 / 1024 for each of bypass, no active band and `processBlockBypassed`.
- **RM-st:** L = R bit for bit on all 8 types during a three-parameter ramp; a lane-after-lane loop gives L ≠ R on 8 of 8 (printed, not gated).
- **RM-zero:** worst 0.066 % (limit 0.5 %; bench 0.075 %), 1.06x the one-sided step (limit 3x; bench 2.1x). Five rates; 88.2 and 192 kHz were not in the bench and read 0.009 % and 0.005 %.
- **RM-norm:** largest one-sample growth of the state norm 0 relative, over 64 million samples on the ramping path.
- **RM-dyn:** 11 checks exact. **SN4:** 26 000 edits field-identical, 4000 Bell controls fire.

### G5 stress

Double engine (RM-g5), `--long`, 600 s per rate: 419 million stereo frames, **0 non-finite**, 54.8 million peak checks, **0 above stack + 12 dB**, 0 structure changes inside a move, all-off output equal to input at 8 of 8 rates.

| fs | Peak re input | Largest stack | Peak − stack, worst block |
|---|---|---|---|
| 22.05 k | +274.9 dB | 535.6 dB | −116.0 dB |
| 32 k | +248.5 | 505.4 | −35.6 |
| 44.1 k | +244.1 | 517.3 | −135.9 |
| 48 k | +249.3 | 596.9 | −136.5 |
| 88.2 k | +282.5 | 605.7 | −89.4 |
| 96 k | +311.0 | 600.8 | −96.0 |
| 176.4 k | +350.2 | 594.4 | **−23.0** |
| 192 k | +291.0 | 563.4 | −94.2 |

The worst margin is 35 dB inside the limit, but far thinner than the bench's −140 dB or lower (see "For verify").

- **RM-g5n:** 150 checks at 8 of 8 rates exceed the stack minus 200 dB; the bound is not vacuous.
- **S5 (plugin, float), `--long`:** finite at 8 rates, no seed change. Peaks +255 to +366 dB re input; largest stack 589.2 dB (guard +600). All bands off: equal to input from the first block.
- **RM-nan:** without the check, 0 of 48 412 samples after the NaN are finite. **S6:** with it, finite from the next block for NaN and for +inf, and from 2 s on equal to a fresh instance within 0 dB on 6 bins.

### G7

- **RM-g7:** residual −235.4 dBFS; silence −79.2 / −110.0 / −280.9 dBFS in the 1st / 10th / 60th second; 3.419 dB/s; monotone. All equal to the bench. **RM-g7n:** fails against 6.8 dB/s.
- **N7:** float residual −159.5 dBFS (limit −140; the float input's own residual is −163.1). Silence costs 0.994x the block time with noise (best of three; the other two 0.998x and 0.999x).

### G6

**RM-alloc:** 0 / 0 over 1e4 ticks with all 24 bands retargeted. **A2:** 0 allocations, 0 frees in 222 834 `processBlock` windows of the stress draw, probe live. **A2n** counts the planted one.

### Gaps from VERIFICATION-2.2

- **M2:** `gain_scale` 100 → 25 gives 6.00024 / 1.50103 / 0.00025 dB, equal to the closed forms; the move reads 1.00x the reference (no smoothing 5.3x). **M2c:** bit-identical with a Low Cut and a Notch.
- **M3** 6 of 6; **M4** equal to a fresh 96 kHz instance; **M5** own component 30.00002 dB, other component below 4e-7 dB.

### CPU (INFO), % of one core, 48 kHz stereo, 24 bands all sweeping

| Case | Double engine | Plugin | Bench |
|---|---|---|---|
| 24 cuts × 96 dB/oct (PERF-02) | 5.50 | 5.45 | 4.35 |
| 24 shelves × 96 dB/oct | 28.0 | 27.5 | 23.0 |
| 24 bells | 4.89 | 4.71 | 4.46 |

All under the 35 % line, and about a fifth above the bench. Settled cost is unchanged from 2.2 (2.73 / 5.23 / 0.82).

### Controls

`--neg` ramp-alloc, rbj, f1, alloc, seed, rbj-b, binary-gate, first-build, engine-alloc: rc 0 each. In-harness: RM-g3i, RM-g3h, RM-arrn, RM-keepn, RM-blkn, RM-g5n, RM-nan, RM-g7n, M1h, M6n, A2n and the added M2c and M4 controls all fire.

### Existing rows (P18)

- Designer: the 67 rows and 12 INFO lines are the baseline's, timing figures apart.
- Render: the 64 rows are the baseline's except S2's detail (`pluginVersion 0.3.0`) and X-cpu timings.
- **No P6 amendment was needed:** no SN1 or EN-dyn check asserted a non-zero gain on a gainless type.

## Deviations

1. **`Router.h` changed** (the plan allowed it for P4's lane-pair form).
2. **API beyond the plan's list:** `setControlGrid`, `snapToTarget` (snap without a fresh target, designs only if moving), `stepCoefficients`, `processPair`, `getStructureMismatchCount`. The designer is a template parameter of `snapTo` / `controlTick` so RM-arr's counting wrapper can plug in.
3. **`setTarget` ignores settings equal to the ramp's target.** Target change detection, not arrival detection.
4. **A continuous edit made during global bypass ramps on un-bypass** (20 ms from the pre-bypass values, from rest). The bypass branch returns before the snapshot compare, as in 2.2. The plan does not say; 2.4's soft bypass replaces this path.
5. **Row IDs B1, S5, S6 already exist as Stage 1 rows.** The plan's IDs were kept under 2.3 headings; a grep for them hits both.
6. **Rows tightened beyond the table:** RM-arr counts designs per ramp; RM-g7 gates the silence cost at 1.2x; M6 / M6n require the exact first-difference index; S6 requires the bad block itself to be non-finite; M2 (i) checks the closed forms against the nominal values.
7. **S5's seed change is automatic** (replay without audio, next seed if the stack reaches +600 dB). It did not trigger.
8. **RM-arr compares sections field by field**, not with `memcmp` (the struct has padding).

## For verify

- **RM-g5's margin.** The worst peak − stack is −23.0 dB at 176.4 kHz and −35.6 dB at 32 kHz, against −140 dB or lower in the bench on its own draw. The row passes, and the same value in the 60 s and 600 s runs says it happens early, while the running stack maximum is still small. Worth confirming that this is the stack's warm-up and not motion adding gain.
- The `--long` runs, once more on both harnesses.
- A hands-on listen: does 20 ms sound right on real material; G3 with several bands moving at once.
- CPU sweeping reads about 20 % above the bench.

## Carry-forward

- **2.4:** G3 step rows (the held-reference threshold there deserves the scrutiny C1 gave the sweeps); structural changes still click; G4 (d), (e), (f); G5 with structural fades; the tick-latched pending change; deviation 4.
- **2.5:** `output_gain` and `auto_gain`.
- **2.6:** G6 with the analyzer on.

## Files

New: `Source/dsp/ControlRamp.h`; this summary.
Changed: `Source/dsp/BandParams.h`, `SectionEngine.h`, `Band.h`, `Router.h`; `Source/PluginProcessor.h/.cpp`; `CMakeLists.txt`; `CHANGELOG.md`; `tests/designer-harness/main.cpp`; `tests/render-harness/main.cpp`; `.planning/STATUS.md`; `PLUGINS.md` (0.3.0).
