---
plugin: O-GranulatorInstrument
stage: 2
phase: "2.4"
title: "Working-copy feedback, Freeze, Reset, reverb + the parent D9 knee"
executed: 2026-11-08
version: 0.1.0 (unchanged, PD21)
fork_pin: 82406fce
commits: C1 3d71bebe (knee adoption, attributed) + C2 (this phase)
result: executed — every automated gate green; S6 hit and resolved by the user (local kappa; CK2 / CK3 → INFO + HANDS-ON-2.4); HANDS-ON-2.1 / 2.2 / 2.3 / 2.4 deferred until the Stage 3 UI
---

# O-GranulatorInstrument — Stage 2 / Phase 2.4 SUMMARY

The engine now feeds grain output back into each slot's working copy and finishes the output path.

**Feedback loop**
- **Tap.** Each stream has an amp-weighted mean tap with the per-(k,s) `g_fb` law. Its statements are separate from the mix statements, so true bypass holds.
- **Chains.** 4 forked `FeedbackChain`s (VERBATIM) run at a unity knob with 32-sample returns.
- **Record heads.** There is one per slot, at exactly 1×.
  - Each writes `(1 − λ)·wc + λ·r[n − 32]` for the whole cell at the grid tick.
  - The writer has a ±0.7 clamp and a convex envelope.
  - λ is `0.95 · feedback · run`, both on 20 ms ramps; the head parks only when λ_eff has settled at exactly 0.
  - The mip children are settled at every wrap and park.
- **Spawner clamp.** It works in head-relative modular coordinates with g_a = 40, plus forced-crossing telemetry.

**Freeze, Reset, SR rebuild**
- **Freeze** stops the heads over 20 ms.
- **Reset** is an off-thread rebuild from the pristine source, taken through the 20 ms dual-read crossfade once the head parks.
- **SR rebuild** parks the heads; they resume at region lo.

**Output path:** `ReverbUnit` (atomics written at the tick) → R96 wet guard → the parent's `wetKnee` → `output_gain`.

## Results

| Gate | Result |
|---|---|
| Build | render-harness, param-dump, VST3, AU and Standalone all rc 0. **0 warnings** in plugin sources (shipping VST3 compiled at T10 too). |
| Default harness | **179 PASS / 0 FAIL**, **0 NOT FIRED**. That is 150 carried + TB0 / TB1 / TB2 + 26 new rows. The plan targeted 181; CK2 / CK3 are now INFO by user decision (§ Click gates). The duplicate-ID set equals the 2.3 log's. |
| `--parity` | **133 PASS / 0 FAIL** (target 133): 99 carried + PB0 / PB1–12 / PBd / PBc1–5 + PR1–12 / PRk / PRc1–2. `--rows GV` **2 / 0**. |
| `--cpu25` | **C25w 26.92 % ≤ 40 % PASS** (live 243.2, writes on 4 slots every window, min 480 000 / window). **C25t 5.54 % ≤ 12 % PASS** (live 72.0). The witness is valid throughout (calib 0.93 × ref). |
| True bypass | **TB0 14/14** = renders-2.4. **TB1**: shipping 14/14 = renders-2.4 and legacy 14/14 = renders-2.2, with every 2.4 control moved and freeze toggled. **RB1 7/7** and **FR1 14/14** (under legacy). Each W2 checkpoint (T5–T10) was byte-identical to both. |
| param-dump | Byte-identical to `.planning/params.tsv` and the T1 capture. |
| Fork drift | **0 / 13**. `FeedbackChain.h` `49b29942…` and `OutputLaw.h` `a550ff9d…` are VERBATIM. ControlGrid column 5 was refreshed to `faa04ef3…`, and column 7 gains the 9 global rows. NC-a (byte appended to `Grain.h`, parent root from the pin archive) → rc 1. NC-b (byte appended to `FeedbackChain.h`, local-root copy) → rc 1. The clean parent root → rc 0. |
| X1 (compdb) | 151 entries → **0** matches of `O-Granulator/Source\|ogi-parity\|82406fce`. |
| X2, auval, pluginval, Standalone | See § T19. |

## C1 — knee adoption, attributed (PD1)

- **The swap.** `Engine::mixPiece`'s two `kWetCeil·std::tanh` statements became `wetKnee`. Hook `legacyWetTanh` restores the exact old statements.
- **Attribution.** Under legacy the dump is **byte-identical** to `renders-2.2.sha256` (14/14), and its 7 `-pinned` lines equal `renders-2.1.sha256`.
- **Re-pin.** `renders-2.4.sha256` was pinned once. **12 / 14 lines moved**: CH, E1, H1, N1, P2 and Q4, pinned + default. The two V3 lines did not move because V3 hashes the grain log, not the output; the plan's "14/14 expected" overlooked this.
- **Parity.** The PN residuals fell from −110.9…−120.2 dB to **−143.0…−151.3 dB**, as predicted. The PF / PFc rows are unchanged (they use `wetSafetyOff`). GV0 is now +0.0202 dB (INFO); `kVoiceGainMeasuredDb` was not re-measured.
- **Rows.** The harness was 152/0. RB1 / FR1 and W0a / W0b (which also compare against renders-2.1) run under legacy.
- **TB2 criterion correction.** Above kKneeT the knee guarantees `|out| ≤ |pre|`; the plan's strict `<` is false in float. In 1018 samples just above T, `tanh(u) ≈ u` rounds back to x. The knee is the parent's verbatim code, so the row now checks `≤` and reports the equal count as INFO.

## Parity — PB (FeedbackChain) and PR (reverb + knee)

| Family | Result |
|---|---|
| **PB0** (tap) | **−152.0 dB**, g_PB = **1.0000000** |
| PB1–PB12 (return) | **−116.3 … −141.8 dB**, \|g/g_PB − 1\| ≤ **1.0e-8**. PB9 (sat 100) env max 0.766 (AGC engaged). Landed fb raws are identical. |
| PBd | xcorr(our return, parent return) peaks at lag **0**. Our written layer vs our return peaks at lag **32** (the z^-32). INFO: layer vs tap peaks at 33 (the chain's group delay). |
| PBc1–5 | −41.3 dB; band −0.757 (unity knob); −36.2 dB; −28.4 dB (testAgcLegacy); −1.8 dB (lag 31). All fire. |
| PR1–PR12, PRk | **−96.8 … −112.0 dB**, \|g/g₀ − 1\| ≤ **2.9e-7** (g₀ 0.9977 from PF0). The PRk parent peak is 0.632 (knee engaged, no level step needed). |
| PRc1–2 | −0.0 dB (size 51); −47.3 dB, band +5.0e-3 (legacyWetTanh). Both fire. |

**Deviations (row construction only; nothing tuned):**
- **PB0 first read −57.4 dB, and I diagnosed it rather than tuning.** The whole difference sat in the first 480 samples. The parent enables its stream at T_e, so its tap carries a 10 ms fb-enable ramp; ours was enabled from sample 0. After those 480 samples the taps were bitwise identical. Our PB side now enables the stream at T_e, giving the same i/480 ramp through the audibility ramp. That is S1 option (a), and the precondition then read −152 dB.
- **Our PB / PR side uses a silent pre-roll to T_e** (note-on at T_e). The reverb's LFOs and smoothers, and the chains, then follow the parent's absolute-time history.
- `ParityRows.h` gains the `gl` / `oursGl` global-param fields and `blockSize` (PB rows render in 32-sample blocks). `parent_render.cpp` appends 8 manifest columns and the `tap_*.f32` / `ret_*.f32` dumps. All 87 carried parent `.f32` files are byte-identical to the T1 copy.

## Law arm, ceiling, settle, Reset (eg24)

| Row | Measured | Control |
|---|---|---|
| FL1 (60 s, fb 100, 16 × 4, sat 20, full-scale noise) | **0 bound violations**: written max 0.989 / 0.969 / 0.980 / 0.984 vs bound 1.000 (pristine). 0 non-finite / subnormal in the output, every tape level and every chain. Writes on 4 slots; rClamps 0. | scanners denorm_min → 1, NaN → 1; bound + 1 ulp → 1 |
| FL1s (sat 0, 0.5-peak tapes) | 0 violations against **0.7f exactly**; rClamps 0 (INFO) | rClampOff + injectReturn 0.95 → written 0.90 > 0.7 |
| FL2 runaway | tap RMS 1 voice −80.0 dB, 16 voices −104.3 dB (−24.3) | sumTap → +75.3 dB |
| FL3 freeze | hashes after the ramp + settle == 10 s later; writes frozen (963 712); parks 4 | the edge hash differs |
| FL4 fb 0, 60 s | 4/4 hashes == pristine build; 0 writes | fb 0.1 % → 4/4 differ |
| FL5 block null (20 s, fb 80, freeze × 4, 2 Resets, reverb 50 → 0 → 50, steals, bend) | 441 / 512 / 4096 / var: **0** samples differ, tape hashes equal | seed + 1 → differs |
| FL6 NaN in chain s2 | scrubs 1, every tape finite, s1 / s3 / s4 equal, writeScrubs 0 | writeScrubOff + NaN return → 544 non-finite |
| FL7 unread params, loop active | 0 differ, tape hashes equal (99 randomised, 162 / 162 bound) | fb_diffusion 51 / reverb_size 51 → differ |
| FL8 tap law | worst tap peak **0.4944** ≤ 0.5 (12 configs) | fbArchLaw → 0.762 |
| CL1 (FL1 render) | knee peak **1.96515** ≤ 1.99526 (pre-knee 3.93) | wetSafetyOff (5 s) → 3.93 |
| CL2 (FZ1, 44.1 / 48 / 96 k) | 1.6464 / 1.8154 / 1.8326 (pre 2.01 / 2.55 / 2.63) | injectPreKnee 8 + wetSafetyOff → 8.0; knee on → 1.99513 |
| MS1 (≥ 10 laps, park) | **0** mismatching samples vs an offline build of the final level 0 (settles 44, parks 4) | settleOff → 5135 |
| RS1 / RS2 | Reset (all) while frozen == pristine, still frozen 2 s later; Reset s1 while parked == pristine, s2–s4 unchanged | no Reset → ≠ pristine |
| RS3 host idle | pending max 1, 204 resets published / 204 freed, deferred 0, live 4 | no entry counter → 20 deferred |
| RS4 | B (A + 10 ms) taken at A + 992 smp (after A's 960-sample fade); pending ≤ 1 | B at A + 100 ms → taken 960 smp later, after the park (swapWaits 60) |
| SR1 (44.1 k tape, 96 k session) | 0 writes before the land, hashes unchanged, root rate error 0.000000 cent; head at region lo on the land; 135 040 writes after | runIgnoreRate → 115 072 writes before the land |
| FA4 | **0 allocations** / 333 blocks (taken 10, xfades 6, parks 17, settles 38, steals 20, writes 327 168) | liveness malloc → 1 |

**FL1 / FL2 note.** With no external input the loop gain is below 1 (mean tap, chain filters, g_fb ≤ 1), so at fb 100 % the copy decays: the 60 s FL1 tapes end near silence. `maxWritten` records the true maximum, so the bound check is not vacuous. FL2's last-10 s window sits at −80 / −104 dB, which makes it a loop-gain comparison: the sum tap sustains the copy, +75 dB. HANDS-ON-2.4 row 1 is the listening check.

## Head clamp, click gates, reverb (eg24b)

- **HC1:** 0 forced crossings and **0 scan violations over 4.01 M grain segments** (3992 clamp moves, levels 0–3, reverse 50 %). INFO parkedSpawnCrossings 17. headClampOff → 10 325 violations.
- **HC2:** 5088 forced crossings counted (region 30 ms); the HC1 region gives 0.
- **HC3:** 1.71 M calls, 0 infeasible at W + 1, 0 bad results, 0 not forced at W. g_a 16 → 160 876 results inside the PD9 set. Fix in my test: inside-ness is evaluated in absolute tape coordinates, as the clamp forms it. The relative form rounded the clamp's one-ulp-outside results back onto the boundary.
- **RV1:** −24.8 dBFS during input, RT60 0.224 s. **RV2:** κ 0.955×, gone within 60 ms, then bitwise the mix-0 render; reverbHardCut → 36.8×. **RV3:** κ 1.139×; with a +0.1 step added → 18.0×.

**Click gates: S6 and the user's decisions (2026-11-08).**
- **What failed.**
  - The plan's whole-window κ (max |Δ²| / window RMS) is blind to any event that changes the level. A Reset brings the pristine copy back at about 30× the erased level, so the loud audio afterwards outweighs the click.
  - The default chain makes r ≈ wc, so a head stop leaves almost no splice. CK1 / CK2 / CK4 hard controls read ≤ 1×.
- **Stimulus fix (the one allowed).** The chain passband is empty (fb_low_cut 2000 > fb_high_cut 1000, the PB3 / PB6 setting), so r ≈ 0 and every stop / start / swap leaves the worst-case 0.95·wc splice.
- **User decision 1: local κ.** κ is now the max over 5 ms frames of max |Δ²| / the frame's own RMS (frames below −80 dBFS skipped). The threshold stays 2.0× and the hard controls stay. It applies to CK1–CK4 and RV2 / RV3.
- **Geometry corrections (my construction errors).**
  - Position 50 % for all CK rows: at 0 %, spray 100 % reflects every grain into [0, W/2], away from the events.
  - CK2 parks its head with Freeze before the fb step, because a parked head resumes where it parked.
- **Result:**

| Row | Ratio | Hard control |
|---|---|---|
| **CK1** freeze | 0.609× | freezeHardStop 20.5× |
| **CK4** Reset swap | 0.378× | resetHardSwap 20.4× |

- **User decision 2: CK2 and CK3 → INFO + HANDS-ON-2.4 rows 3–4.** Their hard controls cannot fire by construction.
  - **CK3:** the run condition parks a head only once no voice is held, so no grain reads the park point while it is parked. A new note restarts the head there while r ≈ 0, so the write it lays down is continuous with what was behind it. The 2.46× (subject = hard) is the evolved copy's content, not a click.
  - **CK2:** the hard step reads 0.76×.
  - Nothing points to a DSP defect: the 20 ms ramps demonstrably separate from the hard stops in CK1 and CK4. INFO values: CK2 0.523× (hard 0.757×); CK3 2.458× (hard 2.458×).

**Carried-row adaptation: FZ1's control.** The planned R96 wet guard now zeroes the un-killed NaN on the wet bus, so "non-finite output samples" read 0. The control counts output **or** guard scrubs instead, with the same meaning and no threshold change.

## CPU (D25, `--cpu25`, run alone; ref_ns 40 523 708)

| Config | Median | Notes |
|---|---|---|
| bypass | 0.56 % | |
| C23w@2.4 (fb 0, reverb 0, same session) | 25.35 % | |
| **C25w** (C23w + fb 100 / diffusion 100 on 4 chains + reverb 100 / size 100) | **26.92 %** | Δ vs C23w@2.4 **+1.57 points** (projection +2–4) |
| **C25t** | **5.54 %** | |
| C25r (requestReset every 200 ms) | 27.87 % | +0.95 points; 374 dual-read fades |
| C25w96 (96 k / 128) | 54.13 % | INFO |
| C25s | +1.03 points (scaled 27.22 vs 26.20, T2 C23w@2.3 = 26.64 %) | **FLAG > +0.6**: the fb-0 restructure cost (wet storage, tap accumulation, 4 chains always running, reverb call) |

The levers were not fired (T18 skipped): C25w has 13.1 points of margin.

## T19 — install and validators

| Check | Result |
|---|---|
| Install | `build-and-install.sh O-GranulatorInstrument` rc 0. Both variants swept; only `O-GranulatorInstrument-dev.{vst3,component}` are on disk. |
| Standalone | Rebuilt explicitly (`OuariconGranulatorInstrument_Standalone`). The binary (20:12:10) is newer than the harness (20:09:15). Alive after 6 s. |
| auval | `aumu OuGI OuDv` **AU VALIDATION SUCCEEDED**. The `ZZZZ` control gives rc 2. |
| pluginval 10 | VST3 × 2 + AU **SUCCESS**: `--strictness-level 10 --timeout-ms 600000`, one at a time, Logic closed, `Main bus num input channels: 2`. |
| X2 | `nm -C` finds **0** `ogran::` symbols in the installed VST3, the installed AU and the harness. Control: the parity binary has 76. |
| HANDS-ON-2.4 | Written (11 rows) and deferred until the Stage 3 UI, with 2.1 / 2.2 / 2.3. |

## Plan-phase decisions as executed

- **PD1–PD9 as planned, except:**
  - The knee moved 12 / 14 lines (V3, see C1).
  - TB2 checks `≤` (see C1).
  - The reverb constants were already in the forked `EngineConstants.h` (`kRevPredelayMs`, `kRevMod`, `kRevShimmer`), so `GiConstants.h` gains nothing (a duplicate would not compile).
- **PD10 settle span corrected.** `kSettleFrames = 2048`, not margin(4) = 1074. A level-l child depends on 127 parent frames, so the stale span after the last write is 126·(2^l − 1) level-0 frames: 1890 at l = 4. margin(4) bounds the read reach, not the child dependency. `settleRun` is also clipped to the flushed span F + margin(L): `flushTail` derives exactly that set, so a further settle would derive children no build derives. MS1 proves the result bitwise.
- **PD12** as planned, plus the enable-at-T_e alignment and the pre-roll (see Parity).
- **PD13.** The metric was amended by the user (local κ), with the stimulus and geometry corrections above. CK2 / CK3 are INFO.
- **PD14–PD21** as planned. The default harness has 179 rows, not 181 (CK2 / CK3 INFO).
- **PBd's second half** checks the written layer against our **return** at lag 32 (the z^-32 is between return and write). Layer vs tap peaks at 33 because of the chain's group delay, recorded as INFO.
- **Dev aids:** `--eg24` runs the 2.4 engine gates alone, and the `OGI_ROWS` env filter is honoured by eg24 / eg24b only. The default harness runs everything.

## Deviations from the plan

1. **CK metric and closure (S6):** user decisions, above.
2. **PB stream enable at T_e, plus the PB / PR pre-roll:** row construction, diagnosed above.
3. **TB2 `≤`**, the **HC3 absolute-coordinate check** and the **FZ1 control (wet guard)**: test corrections, explained above.
4. **Settle span 2048 and the clip to the flushed span** (PD10 correction).
5. **Renders moved 12 / 14** (V3 hashes the grain log).
6. **CL1 / FL1s / FL4 controls** render 5–10 s, not 60 s. Each control only has to cross its threshold.

## Files

- **Source.** `dsp/Engine.{h,cpp}` (reverb, wet guard, knee, tap, 4 chains, record heads, swap rule, head clamp, scan hooks).
  - New: `dsp/GiRecordHead.h`, `dsp/GiHeadClamp.h`.
  - Changed: `dsp/MipTape.h` (`settleRun`, `hashForTest`, `levelPeak`, `scanForTest`), `dsp/SlotLoader.{h,cpp}` (`requestReset`, `resetSync`, `resetInFlight`), `PluginProcessor.{h,cpp}` (`requestReset`, `resetSlotSync`), `dsp/GiConstants.h` (comment), `dsp/forked/ControlGrid.h` (9 globals, ADAPTED), `dsp/forked/FORK-MANIFEST.md`.
- **Tests.** New: `engine_gates_24.h`, `engine_gates_24b.h`, `parity_gates_24.h`. Changed: `render_hashes.h` (overrides), `engine_gates_21/22/23.h` (readSet24, legacy RB1 / FR1 / W0, FZ1 control), `main.cpp`, `tests/parity/{ParityRows.h,parent_render.cpp}`, `tests/golden/renders-2.4.sha256` (C1).
- **CMakeLists.txt** is not edited. **No tag.** Version stays 0.1.0.

## Requirements this phase moves (verify sets the final status)

- **DSP-04:** pending → automated criteria covered: the 0.7 / pristine bound (FL1 / FL1s), NaN (FL6), Reset (RS1–RS4), Freeze (FL3), fb 0 (FL4), the leaf nulls (PB).
- **DSP-05:** pending → automated arm: PR nulls and RV1–RV3. The listening check is HANDS-ON-2.4 row 7.
- **PERF-02:** the final 40 % worst case is PASS at 26.92 %.
- **PERF-01 / PERF-03:** the Reset / Freeze / SR-change arms are in (FA4, RS3, RS4, SR1).
- **QUAL-01:** CK1 / CK4 / RV2 / RV3 are automated. CK2 / CK3 are hands-on (HANDS-ON-2.4 rows 3–4).
