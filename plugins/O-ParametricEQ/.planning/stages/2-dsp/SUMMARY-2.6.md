---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.6 Summary (analyzer taps, display model, listen)"
date: 2026-11-15
phase: "2.6"
status: executed
version: "0.6.0"
designer_harness: "150 PASS, 0 FAIL, 18 INFO (default and --long)"
render_harness: "155 PASS, 0 FAIL (default and --long)"
---

# O-ParametricEQ — Stage 2 / Phase 2.6 Summary (analyzer taps, display model, listen)

## Outcome

**Complete. Designer harness 150 PASS / 0 FAIL / 18 INFO; render harness 155 PASS / 0 FAIL; both also with `--long`.**

- **Display model.** `DisplayModel` designs every band with the processor's designer, rate and layout, and emits section packets.
  - G9 (a) on the damped circle passes at all five rates. Worst stereo reading is 2.03e-3 dB (96 kHz), against the 0.05 dB limit.
  - The packets equal the engine's realised designs bit for bit (XR-g9id).
  - G9 scoring was amended twice in session (Taylor's calls, deviation 1).
- **Analyzer.** A −12 dBFS 1 kHz sine reads −12.04 to −12.08 dB at every rate, and a 20 Hz – 20 kHz sweep reads −12.09 to −11.96 dB.
  - Ring overflow never blocks. It keeps the oldest run that fits, or a large block's tail (Taylor's call, deviation 2).
- **Listen.** It engages over a 30 ms input ramp plus the 20 ms fade.
  - Worst burst readings: 1.21x on, 1.35x on band change. κ ≤ 2.38x wherever the destination is within 40 dB of the source (Taylor's call, deviation 3).
  - It matches the per-sample reference exactly in double and at −139.7 dB in float.
- **Listen off is unchanged.** The 16 sessions dumped from the 0.5.0 processor `memcmp` equal, plain and with the analyzer drained on a second thread.
- **G6.** 0 allocations with the rings saturated and listen switching (A5, XT-alloc).
- **The five rows owed by VERIFICATION-2.5** exist and pass.
- **0.6.0** is built and installed. pluginval 10 passes (VST3 ×2, AU) and `auval` passes; the Standalone stays alive.

No tag.

## Tasks

| Task | Result |
|---|---|
| T1 pre-flight | Contract checksums equal STATUS; branch `main`; plugin tree clean (`$S/git-status-T1.txt`). Baselines 131 / 0 / 17 and 122 / 0; `check_spec.py` OK. `--dump-sessions` + `lidSessions()` / `renderLidSession()` added to the render harness before any Source change. 16 sessions (44.1 / 96 kHz × mono / stereo × 4, 4 s, random block sizes 1 – 4096, S5 band draw, output / auto gain, bypass by both routes, −12 dBFS noise + denormal tail) dumped from 0.5.0. Two dumps byte-identical; harness still 122 / 0. |
| T2 | `Source/dsp/SpectrumTap.h` (P2, plus deviation 4) |
| T3 | `Source/dsp/Listen.h` (P4 – P7; decisions in deviation 5) |
| T4 | Additive only: `OutputStage.h` +43 (`nextGains`), `BandParams.h` +16 (`effectiveBandInput`), `ResponseEval.h` +49 (`sectionResponseAtZ`, `bandResponseAtZ`), `Band.h` +31 (`getRealisedDesign`). `git diff … \| grep "^-[^-]"` prints nothing. `SlotBand.h` untouched: it already had `getSlot` / `getCurrentSlot` / `isSlotOff` / `isAsleep`. |
| T5 designer rows | 20 rows under "2.6 / taps, listen, display truth (double)" and `--neg tap-alloc` |
| T6 / T7 | `Source/analysis/SpectrumAnalyzer.h/.cpp`, `Source/analysis/DisplayModel.h/.cpp` |
| T8 processor | P10 / P11; `OPEQ_VERSION 0.6.0`; the two TUs in `target_sources`; `CHANGELOG.md` `## [0.6.0] - 2026-11-15` |
| T9 render rows | 33 rows plus the S5 amendment; the stopped rows were resolved by Taylor's calls (deviations 1 – 3) |
| T10 gates | Build rc 0, no warning in this plugin's files. Both harnesses 0 FAIL, default and `--long`. All 12 designer `--neg` controls rc 0. `check_spec.py` OK; `--dump-params` equals `params.tsv`. All static greps clean. `pathIdleForBlock`: one definition, two calls; `stageIdleForBlock()` is used only inside it. |
| T11 | Install rc 0; exactly the two `-dev` bundles, both 0.6.0. pluginval 10: VST3 `SUCCESS` ×2, AU `SUCCESS`. `auval -v aufx OuPQ OuDv`: `AU VALIDATION SUCCEEDED`, `[1, 1] [2, 2]`. Standalone rebuilt, `ALIVE`. No NaN or inf report. |
| T12 | This file, STATUS, PLUGINS.md, one path-scoped commit |

## What was built

- **`SpectrumTap.h`**
  - Fixed `float ring[32768]`, a free-running atomic `writeIndex`, and the `drops` and `generation` counters.
  - `push` keeps the newest samples that fit **within one push** and counts the rest as drops. It never blocks.
  - `drain` returns the generation and drop count so the reader can discard.
  - `reset()` (prepare only) zeroes the indices and bumps the generation last.
- **`Listen.h`**
  - `listenInputFor` (P4 table) and the solo router (P5).
  - Two `eq::Band` voices and a 30 ms raised-cosine ramp table (`FadeTable::weightOf` formula).
  - Moves: engage, change, release, plus the ramp on the listened band's f / Q. Latch, latest wins.
  - Requests are read at the block start and taken at the control tick (`setPending` / `applyPending`).
  - Also `startSettled`, `goIdle`, `clearState`, `isIdle`, `isEngaged`, `samplesLeftInMove`.
  - About 90 KB, mostly the ramp table.
- **`SpectrumAnalyzer`** (message thread, P3)
  - Registers as consumer in its constructor.
  - FFT size 4096 / 8192 / 16384 by rate, zero-pad 4x, periodic Hann.
  - Drains everything each call. A moved generation or drop count discards stale data.
  - Ballistics: max (new, held − 20 dB/s).
  - Output: 256 log points from 10 Hz to 30 kHz.
- **`DisplayModel`** (message thread, P8)
  - Owns a designer and polls the raw parameter atomics, comparing them by bit pattern.
  - Packets carry `{ band, type, placement, active, sections, onePoles }`; there are also `headerVar` and `allPackets`.
  - `active` mirrors the engine, including sleep. `sleeps()` restates SlotBand's private `gainIsZero`.
  - A missing half is sent as literal 1s (the identity).
  - It is a **friend** of the processor (one raw read path: `readBandSnapshot`, `readGainScaleRaw`). No listener, no `AsyncUpdater`.
- **Processor** (P10 / P11)
  - Taps: mono sum in 256-sample stack pieces, gated by consumer and mode, with the mode read once per block. Each block offers only its last min (n, 32768) samples (deviation 2).
  - Step 4 reads `listenBand` and calls `listen.setPending`. `makeInput` now calls `effectiveBandInput`.
  - Seeding: `seedListen()` runs at both restarts. The step-7 jump also requires `listen.isIdle()`.
  - `pathIdleForBlock()` = `stageIdleForBlock() && listen.isIdle()`.
  - Per segment:
    - on a tick: band ticks, `applyOutputPending`, the auto-gain refresh, `listen.applyPending`, `controlTick`;
    - the segment is capped at `kDryPieceSize` and at move ends while listen is not idle;
    - the dry copy is taken if a bypass fade is in flight or listen is not idle;
    - not engaged → the 2.5 fused `outputStage.process`; engaged → `nextGains`, × ag, `listen.process`, × og.
  - Step 7: `listen.goIdle()`. Step 8: `listen.clearState()`.
  - Accessors: `getBandRealisedDesign`, `getListenState`, `getPreTap` / `getPostTap`, `getPreparedRate` / `getPreparedStereo`, `setAnalyzerConsumer` / `hasAnalyzerConsumer`.

## Results

### Designer harness (double)

| Row | Reading | Bench / limit |
|---|---|---|
| XL-map | 0 / 640 mappings off the P4 table; 0 / 3200 edits not identical; 10 / 10 classifications | exact |
| XL-route | worst 0 of peak, 40 configurations; the Left / Right other lane exactly 0 | ≤ 1e-12 |
| XL-on | (a) 0 / 240 over 2.0x: on 1.213x, off 1.144x. κ 0 / 90 scored over 2.5x: worst 2.31x (48 kHz), 2.27x (96 kHz), 2.38x (192 kHz). 30 κ-void (5 bands × on/off × 3 rates, 40.0 – 71.2 dB apart) | 1.21x / 1.12x; 2.36x |
| XL-onh | 81 / 81 stop-band readings ≥ 10x (lowest 7100x at 50 Hz, 28.6x at 1 kHz); pass-band voids listed; fade from rest 33 635x | 29 672x |
| XL-chg | (a) 0 / 1016 over 2.0x, worst 1.350x; per phase 0 / 3040 at 48 kHz, 0 / 512 at 96 and 192 kHz. κ 0 / 387 scored over 2.5x: 2.37x / 2.29x / 2.24x; 121 κ-void | 1.35x, 0 / 3040; 2.41x |
| XL-sweep | worst 1.110x | ≤ 4.0x |
| XL-ref | 0 relative difference, 4544 renders | ≤ 1e-12 |
| XL-refn | weight one sample early: 4544 / 4544 differ (lowest 4.1e-9); R = 20 ms: 4544 / 4544 (lowest 1.3e-6) | > 1e-9 |
| XL-latch | 13 / 13; 7 moves, 3 latched | — |
| XL-seed | 8 / 8, bit-exact from sample 0, worst 4.7e-15 of peak | ≤ 1e-12 |
| XL-ag | 0 dB difference; estimate −6.1665 dB | 1e-9 dB |
| XO-split | 118 M samples, 0 differ; 3112 snaps | bit-equal |
| XO-os18 | 0 differ; the control fires on 14 / 400 random histories (deviation 6) | bit-exact |
| XT-ring | 43 M samples, 0 mismatches; 1578 overflowing pushes | exact |
| XT-thr | 0 torn runs; gaps equal drops exactly (46 094 808) | 0 |
| XT-gen | 400 cycles, 0 errors | exact |
| XG-exact | 4000 renders, 1 266 635 points; worst 2.06e-5 dB (192 kHz stereo). Per rate stereo / mono: 2.45e-6 / 1.70e-7, 3.80e-6 / 1.94e-7, 5.23e-6 / 2.60e-6, 1.03e-5 / 2.95e-6, 2.06e-5 / 1.39e-5. **646 void** (deviation 1) | 1.1e-6; ≤ 1e-4 |
| XG-ctl | k × 1.01: 2500 / 3750; one-pole dropped: 3222 / 3920; L / M order: 1378 / 1480; Mid for Side: 1625 / 1675 | each fires |
| XT-alloc | 0 / 0; 233 listen moves, 63 856 split-gain segments, drops on both taps; `--neg tap-alloc` 1 / 1 | 0 |
| XL-cpu (INFO) | one voice: Stereo 6.23 ns, Mid 4.13 ns; two voices: 11.89 / 7.29 ns per sample | — |

### Render harness (float, through `processBlock`)

| Row | Reading |
|---|---|
| XR-g9 | 253 / 253 scored. Worst stereo dB, with voids: 44.1 kHz 7.1e-4 (0), 48 kHz 1.74e-3 (1), 88.2 kHz 1.27e-3 (1), 96 kHz 2.03e-3 (2), 192 kHz 7.0e-4 (3); mono worst 1.5e-4. 7 void configurations (5 mono). Bench ≤ 1.2e-3; limit 0.05 |
| XR-g9 (INFO, by each response's own maximum) | ≥ −20 dB: 2.03e-3; −20 … −80 dB: 1.74e-3; below −80 dB: 4.3e-6 (0 of 1010 responses above 0.05 dB) |
| XR-g9u | 129 / 129 scored, 1 void; worst 9.5e-4 / 9.2e-4 / 1.05e-3 / 1.15e-3 / 1.61e-3 dB (25 draws kept per rate from 37 – 40) |
| XR-g9n | k × 1.01: 21.6 dB (246 / 249); one-pole dropped: 150 dB; L / M order: 324 dB; Mid for Side: 336 dB |
| XR-g9id | 262 / 262; 6240 bands compared field by field, 3465 active; disabled, bypassed, 0 dB Bell and 0 % gain-scale send `active: false` |
| XR-g9j | worst 1.37e-8 dB (bench 1.5e-8) |
| XR-dm | 8 / 8 |
| XR-an | 44.1 −12.081, 48 −12.039, 88.2 −12.081, 96 −12.039, 192 −12.039 dB (bench −12.08 / −12.04 / −12.04) |
| XR-answ | −12.087 … −11.956 dB (bench −12.11 … −11.99) |
| XR-ann | zero-pad 1x reads −12.627 dB (fires; bench −12.63) |
| XR-anpp / XR-anrel | +12.00 dB; release within 8e-6 dB of 20 dB/s over 2 s; instant attack |
| XR-gate | 11 / 11 |
| XR-ovf (reworded) | 6 / 6. Blocks 32 and 4096: 96 000 offered, 32 768 kept, 63 232 dropped. One 65 536 block: 63 232 offered, 32 768 kept (its tail, bit-exact). −12.039 dB on the 3rd `process` call |
| XR-gen | 2 / 2; old-rate bins ≤ −163 dB after the prepare (one thread and a second-thread reader) |
| XR-lon | (a) 0 / 320 over 2.0x. κ 0 / 120 scored over 2.5x: 48 kHz 2.345x / 2.279x, 96 kHz 2.294x / 2.269x. 40 κ-void (the 2.566x reading is void: 68 dB gap) |
| XR-lonh | 54 / 54 ≥ 10x; fade from rest worst 20 102x |
| XR-lchg | (a) worst 1.333x; κ 2.347x; 0 / 1024 single phases over 2.0x; 22 κ-void |
| XR-lref | worst −139.7 dB |
| XR-lid / XR-lidn | 16 / 16 `memcmp` equal to the 0.5.0 dump, plain and with the analyzer drained by a second thread; the control fires |
| XR-loff | equal to the input from the release end (sample 25 536); with the denormal tail, from 25 600 |
| XR-lroute | 12 / 12, worst −145.9 dB (includes the R2 case: Left / Right engaged mid-block at block 2048) |
| XR-lseed / XR-lseedn | 6 / 6, worst −148.3 dB; the control fires (closest +1.1 dB) |
| XR-lag | listen with auto gain on vs off `memcmp` equal; output +6 dB reads +6.000 dB |
| XR-blk6 | 24 / 24 bit-identical |
| XR-lbyp | 24 / 24; (a) worst 1.125x; mix −143.7 dB |
| XR-px17 | 4 / 4 at −150.5 dB; the control fires (−23 dB) |
| XR-px11 | 12 / 12; 160 edits land at different samples across renders |
| XR-px4 | 12 / 12 at −150.5 dB; one refresh late reads −43 dB |
| XR-ag3 | worst 3.7e-5 dB; a 0.5 fs top moves the estimate 0.06 dB (fires) |
| A5 / A5n | 0 allocations / 0 frees in 222 834 windows (57 029 bypassed; drops 927 232 per tap; 200 listen switches); the control fires 1 / 1 |
| X-cpu5 (INFO) | taps on + listen: +0.1 – 0.4 % over X-cpu4 (worst: 24 shelves 96 dB/oct always fading, 12.2 % vs 11.8 %) |

**S5** (amended; `--long`, 600 s per rate plus a 10 s every-block run): 16 / 16. Every sample finite. Largest peak +252.5 dB re input; largest guard 600 dB; slowest return to the input 220.6 ms (limit 300 ms); seed changed at 6 of 16 runs.

Existing rows: all 131 designer rows and all 122 render rows print as in the baselines. The exceptions are S2 (`pluginVersion 0.6.0`), the N7 (ii) timing ratio, S5 (planned amendment) and wall-clock INFO timings.

## Deviations

1. **G9 scoring floor (Taylor's calls, plan STOP ×2).**
   - As written ("within 80 dB of each response's own maximum"), the rule scored points below the engine's rounding floor on responses that cancel far below unity. XR-g9 read up to 5.65e3 dB, XR-g9u 46 dB, XR-g9j 5.62e3 dB, and XG-exact 589 dB.
   - Responses peaking at −20 dB or above passed throughout.
   - First call: score within 80 dB of the **largest** maximum among LL / RR / MM / SS. That cleared stereo, but not chains wholly below unity.
   - Second call: add an absolute floor re the unit impulse, **−120 dB** in float (render rows) and **−150 dB** in double (XG-exact, XG-ctl). A configuration with no point above it is listed as void.
   - Voids: XG-exact **646 / 4000** (16 %, mostly mono; every voided chain peaks at −158 dB or lower). XR-g9 7 / 260; XR-g9u 1.
   - Thresholds are unchanged and every control still fires. **For verify:** whether 16 % void in XG-exact is acceptable, or whether the draw should exclude chains wholly below −150 dB.
2. **XR-ovf reworded to P2 (Taylor's call).**
   - The plan's "ring holds the newest 32 768" contradicted P2: the ring never overwrites, and 256-sample pieces keep the oldest run.
   - Now: the processor offers only a block's last min (n, 32 768) samples, so a large offline block still delivers its tail into an empty ring. The row asserts exact drop counts, the oldest-fitting run (or the big block's tail), and the reading within 3 `process` calls.
3. **Listen κ void into silence (Taylor's call, R5).**
   - Low Shelf 20 Hz listens through a 20 Hz low-pass, so the 1 kHz tone falls about 68 dB. κ read 2.61x / 2.83x (XL-on 96 / 192 kHz), 2.67x (XL-chg 96 kHz) and 2.566x (XR-lon 96 kHz).
   - New rule: κ is scored only where the destination's held level at the tone is within 40 dB of the source's. Others are listed by name as κ-void; (a) still binds them. R stays 30 ms.
4. **`SpectrumTap` read state.**
   - The plan's design let a drain store its old `readIndex` just after `reset()` zeroed the indices.
   - Generation and `readIndex` are now packed into one 64-bit atomic, published by compare-exchange. A reset during a drain makes the drain return 0 samples and the new generation.
   - `reset()` does not zero `drops`.
   - A reader must drain everything each time: on a partial drain, the drop count can move one drain before the gap arrives.
5. **Listen decisions the plan did not pin down.**
   - The gain form is chosen by `isEngaged()` (target ≥ 0 or a move in flight), not `isIdle()`. A pending wish waiting for its tick would otherwise switch the form at a block boundary (R1).
   - "Pending" is derived (wish ≠ request).
   - f / Q edits during a move go to the incoming voice. A band-index change is always the two-step move.
   - The ramp multiplies the filter input.
   - Mixes are `a + w (b − a)`.
   - `goIdle()` keeps the target, so `startSettled` must run before the next `process` (asserted).
6. **XO-os18 stimulus added.** On the XO-step / XO-ret pairs the "last stage only" control never fires: the last stage is always the last to settle. 400 random-automation histories (VERIFICATION-2.5 probe T3) were added. The control now fires 14 / 400.
7. **Processor details.**
   - `prepareToPlay` stores `preparedStereo` before `preparedRate` (both release, after the tap resets). A reader that sees the new rate sees the new layout.
   - The analyzer discards the drain where the drop count moved **and** the next one (stricter than P3 step 1).
   - The step-7 jump also requires `listen.isIdle()`.
   - Extra read-only accessors were added (`hasAnalyzerConsumer`, analyzer and model counters).
8. **XR-lid input path.** XR-lid reads the 0.5.0 dump from `--lid-dir <dir>` or `OPEQ_LID_DIR`, and fails with a message if neither is set. No user path is in the public repo. The dump lives in this session's scratchpad only (P13), so verify must regenerate it from a 0.5.0 build (`git worktree` or `git stash` of `Source/` at e9d6b560), or treat XR-lid as session-bound.
9. **Row interpretations** (render):
   - XR-lonh takes the largest of four phases, as XL-onh does.
   - XR-blk6's "offline" is one block per span between events.
   - XR-px4 compares the right lane against a restated gain trajectory.
   - XR-g9 forces an L / M pair into the first draw per rate, so the order control can fire.
   - `stressLoopCore` takes a listen-draw argument; A3 and A4 pass `nullptr`.
10. **XT-thr accounting fix** (designer). Whole pushes dropped after the last read leave no visible gap, so drops are counted from the producer's total.
11. **The 1→2 gate was not re-run**; it passed at 2.5 and no Stage 1 artefact changed.

## Carry-forward

- **For verify 2.6** (closes Stage 2):
  - Hands-on:
    - listen on a narrow band-pass under bass (no burst; 50 ms engage feel);
    - listen on a Mid band of a stereo mix;
    - listen while auto gain is on;
    - the analyzer pre / post can wait for 3.3.
  - A mutation run over `Listen.h`, `SpectrumTap.h`, the processor's listen / tap glue and `pathIdleForBlock`.
  - Both `--long` runs once more.
  - Regenerate the 0.5.0 session dump for XR-lid (deviation 8).
  - Judge deviation 1 (the 16 % XG-exact void) and deviation 3 (κ-void lists).
  - Decide VERIFICATION-2.4 Issue 1 after the listen hands-on.
- **For Stage 3:**
  - The editor registers the analyzer consumer and drives `SpectrumAnalyzer::process (measured dt)` and `DisplayModel::poll()` from its timer.
  - `eqSections` / `analyzerPre` / `analyzerPost` emits and the `init` pull.
  - `listenBand` written by the drag gesture; whether the UI ends listen on a band disable.
  - The meter reader (2.5).
  - G9 (b) under node on packets dumped by the render harness.
