# Stage 2: DSP — Phase 2.3 (continuous modulation) - Verification

## Verification Date

2026-11-14 (verify session). What was run, against `07d044d8`:

- An independent re-run: the three harness objects (designer `main.cpp`, render `main.cpp`, the render harness's `PluginProcessor.cpp`) were deleted and recompiled; both harnesses were run in default and `--long` form, and all nine negative controls, and compared with SUMMARY-2.3.
- A full read of `ControlRamp.h`, `Band.h`, `SectionEngine.h`, `Router.h`, `BandParams.h` and `PluginProcessor.cpp`.
- A probe written in this session (`probe.cpp`, 37 rows) that drives the real processor and uses none of `Source/dsp` and none of the harness code.
- A 36-mutant run of the engine headers and the processor through both harnesses and the probe.
- An instrumented copy of the designer harness's G5 run, for the margin question SUMMARY-2.3 left for verify.
- pluginval strictness 10 (VST3, AU) and `auval` on the installed 0.3.0 bundles.

The probe, the mutation driver, the G5 instrumentation (as a diff) and their outputs are in `stages/2-dsp/verify-2.3/`.

Scope: Phase 2.3 only. Stage 2 has three more phases (2.4 – 2.6), so this file does not close the stage.

## Goal-Backward Analysis

### Original Goals (ROADMAP Phase 2.3 as amended; PLAN-2.3)

1. Frequency, gain, Q and gain scale ramp for 20 ms on a global control grid, interpolated per sample; the filter state is kept.
2. G3 sweep rows: ≤ 4.0x the same-motion per-sample reference; the interpolation-off and no-smoothing controls read ≥ 10x (DSP-04).
3. G5: finite at 8 rates, peak within the static stack + 12 dB; a non-finite input does not stick (DSP-04, DSP-06).
4. G7: noise floor, ring-down 3.42 dB/s, ≤ -270 dBFS in the 60th second (DSP-05).
5. G6 under stress: no allocation in `processBlock` (PERF-01).
6. Events on multiples of 2048 render bit-identically at blocks 32, 64, 512, 2048; the grid is global and independent of the host block.
7. A settled band equals the static design of its settings and does no design work.
8. The four harness gaps of VERIFICATION-2.2 (kept state, `gain_scale` / band bypass through the plugin, re-prepare, Mid / Side response) and the canonical gain on gainless types.
9. Stage 1 and 2.2 rows unchanged; 0.3.0 builds, installs and validates.

### Deliverables (from SUMMARY-2.3.md, confirmed in the tree)

1. `Source/dsp/ControlRamp.h` (new); `Band.h`, `SectionEngine.h`, `Router.h`, `BandParams.h` extended. Std-only, `noexcept`, no allocation.
2. `PluginProcessor.h/.cpp`: tick clock advanced first in `processBlock` and `processBlockBypassed`, segment loop, end-of-chunk non-finite check.
3. Designer harness: 21 rows + 2 INFO, `--long`, `--neg ramp-alloc`. Render harness: 19 rows + X-cpu2, `--long`.
4. `CMakeLists.txt` at 0.3.0, `CHANGELOG.md` `## [0.3.0]`, dev bundles installed.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 20 ms ramps, state kept | ✅ Achieved | Probe, through the plugin, against first principles: a 0 → +12 dB gain move takes 16.1 – 16.3 ms from 10 % to 90 % at 44.1 / 48 / 96 / 192 kHz (16 expected) and reads 3.0 / 6.0 / 8.9 dB at +5 / +10 / +15 ms (linear in dB); `gain_scale` takes the same ramp; a notch swept 100 Hz ↔ 10 kHz is deepest on a 1 kHz tone at +10.3 / +10.4 ms (log f; linear f would be 1.8 / 18.2 ms); a Q move 0.1 → 10 reads the Q 1 level at mid-ramp (7.01 against 6.94 dB; linear Q would read 0.82); a new target at mid-ramp takes 20 ms more from where it stood. RM-keep 1.74x, M6 1.73x. |
| 2 G3 sweeps | ✅ Achieved | RM-g3 worst 2.54x, M1 2.538x, RM-g3k 2.36x / 1.25x; RM-g3i, RM-g3h, M1h fire; 0 WARN. Equal to the summary. Probe: no step in the output on any move tried (largest first difference 0.92x – 1.11x a steady tone's), with one band and with 8 bands automated per block at blocks 32 / 64 / 512 / 1024. See Issue 6 for what the second difference shows. |
| 3 G5, non-finite input | ✅ Achieved | `--long`: 419 250 000 stereo frames, 0 non-finite, 0 of 54.8 million checks above stack + 12 dB; S5 finite at 8 rates; RM-nan / S6. The thin margin is explained below. |
| 4 G7 | ✅ Achieved | RM-g7 -235.4 dBFS, 3.419 dB/s, -280.9 dBFS in the 60th second, monotone; N7 -159.5 dBFS, silence 1.00x the block time with noise. |
| 5 G6 under stress | ✅ Achieved | RM-alloc 0 / 0, A2 0 / 0 with a live probe, A2n fires, `--neg ramp-alloc` rc 0. Read: no lock, listener or container growth in the audio path. |
| 6 Block identity, global grid | ✅ Achieved | RM-blk, B1, TK 9 of 9. Probe: a host block of 4096 on prepared sizes 512 / 100 / 7, and host blocks 512 / 64, are bit-identical to prepared 4096. |
| 7 Settled = static | ✅ Achieved | RM-arr 140 cases bit for bit, 0 designs over the next 100 ticks; RM-arrn fires. Probe: steady response after a three-parameter move equals a static instance on 5 types. |
| 8 Gaps from 2.2 | ✅ Achieved | M2, M2c, M3, M4, M5, M6 / M6n, SN4. The 2.2 mutants X7, X8, P3 and R7 are now caught by the render harness (M2, M4, M3, M5). Probe: gain and `gain_scale` edits on the four gainless types are bit-identical to no edit. |
| 9 Existing rows, 0.3.0 validates | ✅ Achieved | `params.tsv` unchanged, `check_spec.py` OK. Installed binaries equal the build artefacts (SHA-1) and are newer than every source file. pluginval 10 `SUCCESS` on VST3 and AU; `auval` succeeded with `[1, 1] [2, 2]`. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.3 of 7)
**Requirements touched by this phase:** 4 (all must)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-04: click-free modulation | must | ⚠️ Partial | Criterion 1 met (G3 sweep rows as amended; probe). Criterion 3 holds on this engine (G5, 600 s × 8 rates, blocks 4096 – 1) and re-runs in 2.4 with structural fades. Criterion 2 (type, slope, placement, enable) is 2.4. |
| DSP-05: stability and noise | must | ✅ Complete | Both criteria met: RM-g7 / N7 at 192 kHz, 10 Hz, Q 40; silence decays monotonically and costs 1.00x the block time with signal. |
| PERF-01: real-time safety | must | ⚠️ Partial | Both criteria met on this engine (A2 under stress; read). Left partial because G6 re-runs with the analyzer on in 2.6. |
| DSP-06: ranges | must | ⚠️ Partial | Unchanged: range ends are in every G5 draw and stay finite; reachability from UI and typed entry is Stage 3. |

FUNC-05 stays partial: `gain_scale` is now gated through the plugin (M2); output gain is 2.5.

**Requirements Summary (all 27):**
- ✅ Complete: 6 (COMPAT-01, FUNC-03, DSP-01, DSP-02, DSP-05, DSP-07)
- ⚠️ Partial: 9
- ⏸️ Pending / later phase: 12
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Contract checksums | ✅ Pass | brief, parameter-spec, ARCHITECTURE and ROADMAP equal STATUS |
| Commit hygiene | ✅ Pass | `07d044d8` lists this plugin's paths and `PLUGINS.md`; plugin tree clean; no tag on the commit |
| Build (three objects deleted, recompiled) | ✅ Pass | rc 0; no warning from this plugin's files (the SDK's libc++ `availability.h` notice only) |
| Designer harness, default and `--long` | ✅ Pass | `88 PASS, 0 FAIL, 14 INFO` both times. Every 2.3 figure read equals SUMMARY-2.3. |
| Render harness, default and `--long` | ✅ Pass | `83 PASS, 0 FAIL` both times |
| Negative controls | ✅ Pass | `ramp-alloc`, `rbj`, `f1`, `alloc`, `seed`, `rbj-b`, `binary-gate`, `first-build`, `engine-alloc` rc 0; an unknown name rc 255 |
| Static gates | ✅ Pass | The three greps print nothing; `dynOffsetDb`: one declaration, reads only; `makeBandInput`: one definition |
| Reference independence | ✅ Pass | `refRender`, `holdRender`, `hardRender` in both harnesses contain no `ControlRamp`, `controlTick`, `setTarget`, `snapTo`, `rampSection`, `stepCoefficients` or `eq::Band` |
| Layout golden, spec | ✅ Pass | `--dump-params` equals `params.tsv`; `OK: 196 params` |
| Installed bundles | ✅ Pass | Exactly the two `-dev` bundles, version 0.3.0, binaries identical to the build artefacts |
| pluginval 10, `auval` | ✅ Pass | VST3 `SUCCESS`, AU `SUCCESS`, `AU VALIDATION SUCCEEDED` |
| Independent probe | ✅ Pass | 37 / 37 (`verify-2.3/probe-output.txt`) |

### The RM-g5 margin (SUMMARY "For verify")

The question was whether the worst peak − stack of −23.0 dB (176.4 kHz) and −35.6 dB (32 kHz) is the running stack's warm-up or motion adding gain. It is the warm-up. The instrumented run (`verify-2.3/g5diag.diff`, same draw, 60 s per rate) reports where each rate's worst check falls:

| fs | Worst margin | At | Running stack there | Peak there (re input) | Worst after 1 s |
|---|---|---|---|---|---|
| 22.05 k | −116.0 dB | first block | 152.6 dB | +36.7 dB | −187.4 dB |
| 32 k | −35.6 | 0.128 s (second block) | 41.3 | +5.7 | −135.6 |
| 44.1 k | −135.9 | 8.24 s | 349.8 | +213.9 | −135.9 |
| 48 k | −136.5 | 0.853 s | 179.2 | +42.7 | −195.3 |
| 88.2 k | −89.4 | first block | 0.5 | −88.9 | −273.7 |
| 96 k | −96.0 | first block | 110.6 | +14.6 | −159.7 |
| 176.4 k | −23.0 | first block | 12.7 | −10.2 | −281.3 |
| 192 k | −94.2 | first block | 90.4 | −3.8 | −241.3 |

At 176.4 kHz the "thin" check is the first block of the run: the stack of that block's own targets is 12.7 dB and the output is 10 dB *below* the input. After the first second no rate comes within 135 dB of the bound. Against the largest stack of the trailing 2 s only (512-sample periods), the worst reading at any rate is −147.8 dB. Motion adds no gain that this draw can show.

The other side of the same figures: once the running maximum has grown, the bound sits 135 dB or more above anything the engine produces, so the peak clause of G5 is a loose gate (Issue 5).

### Mutation run

36 mutated copies of `Source/` were compiled against the unmodified harness sources and the probe in the session scratchpad; nothing in the repo or in `build/` was changed. The unmodified control, run in the same parallel batch, gives 88 / 0 / 14, 83 / 0 and 37 / 0.

| Mutation | Designer rows failing | Render rows failing | Probe rows failing |
|---|---|---|---|
| C1 ramp length 10 ms | RM-g3 | M1 | 9 |
| C4 K = 32 at 48 kHz | RM-g3 | M1, TK | 0 |
| C5 gain not ramped | RM-g3, RM-keep | M1, M6 | 11 |
| C6 Q not ramped | RM-g3 | M1 | 1 |
| C7 frequency not ramped | RM-g3, RM-g3k | M1, M1k | 2 |
| C8 a retarget keeps the old counter | RM-g3, RM-g3k | M1, M1k | 4 |
| B6 `processPair` steps twice per sample | RM-st | M2 (ii) | 6 |
| B14 `processLane` does not step | RM-g3, RM-g3k | M1, M1k, M6 | 4 |
| B17 `setDesign` does not cancel an interpolation | RM-g5 | S5 | 4 |
| SE1 `a1`, `a2` not re-derived while ramping | RM-g3, RM-g3k, RM-norm, RM-g5 | M1, M1k, B1, S5, A2 | 9 |
| SE3 one-pole mix not interpolated | RM-g3, RM-g3k | M1, M1k | 2 |
| SE5 SVF mix not interpolated | RM-g3, RM-g3k, RM-keep | M1, M1k, M6 | 5 |
| R7 Mid uses `L + R` (router and prediction) | EN-ms | M5 | 1 |
| P3 band bypass ignored | SN1 | M3 | 1 |
| X7 `gain_scale` missing from the snapshot | – | M2 (i), M2c | 1 |
| X8 `prepareToPlay` does not invalidate the snapshots | – | M4 | 1 |
| X20 `processBlockBypassed` does not advance the clock | – | TK | 0 |
| X21 the bypass branch does not advance the clock | – | TK | 0 |
| X22 the no-active-band return does not advance the clock | – | TK | 0 |
| X23 non-finite check removed | – | S6 | 0 |
| X25 a continuous change snaps (no ramp) | – | M1, M1k, M2 (ii), M6, TK | 19 |
| X35 `prepareToPlay` does not hand the bands the grid | – | M1k | 1 |
| C3 arrival designed from the ramp values | RM-arr, RM-dyn | **none** | 0 |
| B5 tick step 1 keeps the accumulated sum | RM-arr, RM-dyn | **none** | 0 |
| B10 moving flag latched | RM-arr, RM-dyn | **none** | 0 |
| B15 `snapTo` keeps a pending target | RM-g5 | **none** | 0 |
| P5 a type change classified continuous | RM-g5 | **none** | 0 |
| P10 no canonical gain on gainless types | RM-arr, RM-arrn, SN4 | none (output is identical; only the design work differs) | 0 |
| **B7 Mid loop does not step on the ramping path** | **none** | **none** | 2 |
| **B7s Side loop does not step on the ramping path** | **none** | **none** | 3 |
| **B13 `snapToTarget` does nothing** | **none** | **none** | 2 |
| **SE2 one-pole `Gc` held while ramping** | **none** | **none** | 1 |
| **X28 the tick phase restarts at every chunk** | – | **none** | 1 |
| **X29 `prepareToPlay` does not zero the clock** | – | **none** | 1 |
| **X26 a structural change on an active band keeps the state** | – | **none** | **0** |
| B3 `setDesign` clears the state | none | none | 0 (equivalent in 2.3: `setDesign` is reached only through `snapTo`, whose callers reset or have just reset, and through the structure-mismatch path, which never fires) |

28 of 35 live mutants are caught by the harnesses. Seven survive both (Issues 1 – 4); the probe catches six of them. Five more are caught by the designer harness only (Issue 4).

## Human Verification

Not blocking.

- [ ] Logic (AU) or one VST3 host, generic editor: drag frequency, gain and Q of a few bands on real material, and automate them. No zipper, no click; 20 ms feels immediate enough.
- [ ] The same with several bands moving at once and with `gain_scale`.
- [ ] One extreme throw on a low sustained tone (a bell at the tone, gain from one end to the other): listen for a soft tick at the start or end of the move (Issue 6).

Expected in this build and not defects: type / slope / placement / enable changes click, global bypass is a hard switch, output gain and auto gain do nothing.

The engine headers and the processor were read in full with no defect found. The two harnesses were not read beyond the reference renders and the G5 row; the mutation run, the controls and the probe corroborate their scoring.

## Issues Found

None is a defect in the shipped code. All are gate gaps or observations for the 2.4 plan.

1. **The ramping path of a Mid or Side band has no row.** With the coefficient step removed from the Router's Mid or Side loop (B7, B7s) both harnesses pass: the G3 rows use Stereo, RM-st covers Stereo, M5 reads a steady state. The probe catches both (a Mid band on L = R input equals the Stereo band's output through a three-parameter move; here the difference is 0). Add to RM-st or M1: the same move on Mid with L = R and on Side with L = −R, compared with the Stereo render.
2. **One-pole sections on the ramping path are thinly gated.** The 13 G3 motions use 12 and 96 dB/oct, which have no one-pole half. With `Gc` held during a ramp (SE2) both harnesses pass; the probe's 6 dB/oct High Cut sweep catches it. Held `cL` / `cH` (SE3) is caught, so the path is not blind, only its pole. Add a G3 motion on a 6 or 18 dB/oct cut.
3. **Three processor rules rest on the probe alone.**
   - `snapToTarget` (B13): bypass or `processBlockBypassed` in the middle of a move must leave the band settled on its target. Probe: the blocks after it equal a fresh instance bit for bit.
   - The tick phase across chunks (X28): every harness block is at most the prepared size. Probe: host block 4096 on prepared 512 / 100 / 7.
   - The clock reset in `prepareToPlay` (X29): M4 makes no move after the re-prepare. Probe: re-prepare, then two moves in blocks of 500.
   2.4 rewrites the bypass path; the other two want one render-harness row each.
4. **"A structural change clears the state" is no longer gated on an active band.** With the `reset()` after `snapTo` removed (X26) nothing fails, the probe included: U2 goes through the inactive branch, and M6n reads 2.7e6x with or without the reset because the design step dominates. Likewise a type change classified continuous (P5) is seen only by RM-g5's structure count, and four engine rules (C3, B5, B10, B15) only by the designer harness. 2.4 replaces this path with crossfades, so the row belongs in its plan rather than being added now.
5. **G5's peak clause is loose after warm-up.** The running-maximum stack is 135 dB or more above every peak after the first second (table above). It caught nothing in the mutation run that finiteness did not. If 2.4 keeps the clause, a stack over a trailing window would make it bite; this run reads −148 dB against a 2 s window, so there is room to tighten.
6. **The ramp has corners.** A ramp linear in dB starts and stops with a change of slope. On extreme throws the output's second difference reads 16x – 64x a steady tone's (bell at a 50 Hz tone, ±30 dB in 20 ms) and 25x with block-rate automation at 1024-sample blocks, while its first difference stays at 1.0x: there is no step, and a hard change reads 132x on the first difference. This is the behaviour the same-motion reference was chosen to accept (plan call U1), and it is how any linear smoother behaves; it is recorded because the G3 verdict cannot see it. The listening item above is the check.
7. **Deviation 4 of the SUMMARY stands:** a continuous edit made during global bypass ramps on un-bypass. 2.4's soft bypass replaces the path.
8. **CPU sweeping** reads about a fifth above the bench (5.5 % for the PERF-02 case, 28 % for 24 shelves × 96 dB/oct, against the 35 % line). INFO in this phase; G10 binds in Stage 4.

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 2.3)

**Ready for next phase:** Yes. Phase 2.4 (structural crossfades and bypass) has no research or plan yet; next is research for 2.4.

**Blockers (if any):** none. Carried forward, non-blocking:
- harness rows for the Mid / Side ramping path, a one-pole G3 motion, an oversize host block, a move after a re-prepare (Issues 1 – 3);
- the state rule of a structural change and the bypass-during-a-move rule, in whatever form 2.4 gives them (Issues 3, 4);
- the G5 peak clause (Issue 5);
- the hands-on listen.
