---
plugin: O-GranulatorInstrument
stage: 2
phase: "2.3"
title: "Per-grain FX + CPU budget"
executed: 2026-11-08
version: 0.1.0 (unchanged, PD15)
fork_pin: 82406fce
result: executed — every automated gate green; levers not needed; HANDS-ON-2.1 / 2.2 / 2.3 deferred by the user until the Stage 3 UI
---

# O-GranulatorInstrument — Stage 2 / Phase 2.3 SUMMARY

The engine now runs the per-grain FX that were already forked byte-for-byte from O-Granulator (`forked/GrainFx.h`, unchanged):
- **Latch at spawn.** A new `GiFxLatch.h` holds the parent's `planFx` / `assignFxLatch` with names changed only. It runs before `acquire`, so a refused spawn uses the same draws. It latches the SVF coefficients **including the D10 `svfMake`**, plus drive, crush (with the Bernoulli draw) and the ring carrier.
- **One kernel.** `GiRenderer::renderGrain` is now **gather → FX → scatter**. The 2.2 loop gathers into `L / R / GK`. The FX stages run in the ARCHITECTURE §13 order (SVF → drive → crush → ring) and only when the latched `fxActive` bit is set. The scatter keeps the 2.2 single statements. The SVF NaN kill retires the grain before its segment reaches the bus.
- **Parameters.** `ControlGrid.h` binds the 10 FX rows × 4 streams (ADAPTED region only, re-hashed).
- **Proof.** DSP-03 is proven against the parent at `82406fce`: 48 null rows with a gain band, 6 controls, and 3 statistical rows. FX at their defaults are a true bypass: every renders-2.2 hash is byte-identical.
- **CPU.** D19 and D20 both PASS with a valid machine-speed witness, so no lever was applied.

## Results

| Gate | Result |
|---|---|
| Build | render-harness, param-dump, VST3, AU and Standalone all rc 0. **0 warnings** in plugin sources. |
| Default harness | **150 PASS / 0 FAIL** (PD4 target 150): Stage 1 55 + 2.1 33 + 2.2 29 + FU 15 + FN 3 + FR 1 + FV 7 + FB 4 + FZ 2 + FA 1. **0 "NOT FIRED"** controls. No new duplicate row IDs: the duplicate set is the same as in the 2.2 log. |
| Parity harness (`--parity`) | **99 PASS / 0 FAIL** (PD4 target 99): 41 carried + PF0 + 48 PF + PFc1–6 + PS5–7. `--rows GV` **2 / 0**. |
| DSP-01 carried | Every PN residual and fitted gain is **identical** to the T1 baseline (`diff` empty; −110.9 … −120.2 dB). |
| FX-off bypass | **FR1 14/14** lines = `renders-2.2.sha256`; RB1 7/7; `--dump-render-hashes` byte-identical. **S3 did not fire.** |
| param-dump | Byte-identical to the 2.2 capture and to `.planning/params.tsv`. |
| Fork drift | **0 / 13**. `GrainFx.h` is VERBATIM and byte-identical (`d92940e5…`). ControlGrid column 5 was refreshed to `ba86d4d3…` and column 7 extended. NC-a (byte appended to `Grain.h` in a parent root built from the pin archive) → rc 1. NC-b (byte appended to `GrainFx.h` in a local-root copy) → rc 1. The unmodified parent root gives rc 0. |
| X1 (compdb exclusion) | 151 O-GranulatorInstrument entries → **0** matches of `O-Granulator/Source\|ogi-parity\|82406fce`. |
| X2 (symbol exclusion) | `nm -C` finds **0** `ogran::` symbols in the installed VST3, the installed AU and the harness. Control: the parity binary has 76. |
| auval | `aumu OuGI OuDv` **AU VALIDATION SUCCEEDED**. `ZZZZ` control rc 2. |
| pluginval 10 | VST3 ×2 + AU **SUCCESS** (`--strictness-level 10 --timeout-ms 600000`, one at a time, Logic closed, `Main bus num input channels: 2`). |
| Standalone | Rebuilt explicitly (`OuariconGranulatorInstrument_Standalone`, 16:30:08, newer than the harness at 16:26:44). Alive after 6 s. |
| Install | `build-and-install.sh` swept both variants. Only `O-GranulatorInstrument-dev.{vst3,component}` are on disk. |

## T2 — machine-speed witness (PD2)

Frozen on the **unmodified 2.2 engine**, before any `Source/` edit, in `tests/golden/cpu-calib-2.3.txt`:
- **`ref_ns` 40 523 708**. The three `--calib` processes gave 40 523 708 / 40 553 000 / 40 095 542; max / min = 1.011 ≤ 1.05.
- **C22@2.2:** 14.37 % (min 14.35, max 14.64), live 243.2, duty 0.964. Calib was 40 095 542 before and 40 320 750 after.
- CPU: Apple M4 Max. `cpu_calib.h` is frozen from here on; 2.4 reads the same file.

## DSP-03 parity (PF / PFc / PS5–7)

Parent: rebuilt from `git archive 82406fce` by `run.sh`, rc 0, 0 self-check failures.
- **Every 2.2 row's `.f32` is byte-identical** to the T1 copy.
- **PFC1–4 `.f32` are byte-identical to `PF0.f32`.** Crush prob 0 draws nothing and stays inactive.
- **Parent peak.** The maximum is 0.305, under the 0.45 limit. PFC8 at 0.4 peak with −6 dB level gave 0.250. No PD5 level step was needed beyond the planned one.
- **`fxActive`.** The bits matched the expected set on every grain of every PF row.
- **Stats rows.** PS5 and PS6 have the stage bit on 100 % of grains; PS7 has p̂ 0.299.

**PF0 precondition (PD6):** residual **−149.3 dB** (threshold −130), with **g₀ = 0.9977000**. S1 did not fire.

**PF null rows:** 48/48 at ≤ −90 dB after LS alignment over n = 47 744.
- Grain counts are equal, the FX raws landed identical, `fxActive` is equal per grain, and every reference is ≥ −58.1 dBFS.
- PFC1–4 on our side are bitwise ≡ our PF0.

| Family | Residual (dB), best … worst | Worst margin |
|---|---|---|
| PFL1–9 (LP) | −143.3 … −126.2 (PFL3) | 36.2 dB |
| PFB1–9 (BP) | −144.3 … −126.1 (PFB3) | 36.1 dB |
| PFH1–9 (HP) | −146.4 … −131.2 | 41.2 dB |
| PFD1–4 (drive) | −150.9 … −148.8 | 58.8 dB |
| PFC1–8 (crush) | −152.1 … −148.8 | 58.8 dB |
| PFR1–6 (ring) | −149.3 … −149.1 | 59.1 dB |
| PFX1–3 (chained) | −151.9 … −151.7 | 61.7 dB |

The worst rows are the res-100 % SVF rows (PFL3 / PFB3 / PFL6 / PFB6 / PFH6 ≈ −126 … −131 dB). Every other row is ≤ −134 dB.

**Gain band (PD6):** |g_row / g₀ − 1| ≤ **1e-7** on all 48 rows (worst PFC8, −1e-7; the band is 1e-4). **S4 did not fire.**

**Controls** (every one fired):

| Row | What | Result |
|---|---|---|
| PFc1 | PFB5, our cutoff × 2^(1/12) | −21.0 dB |
| PFc2 | PFD3, our drive 51 | −46.3 dB |
| PFc3 | PFC6, our crush 41 | −0.2 dB |
| PFc4 | PFR4, our ring_freq × 2^(1/12) | −1.0 dB |
| PFc5 | PFX1 + `fxOrderSwap` | −39.8 dB |
| **PFc6** | PFL6 + `legacySvfMake` | null **passes** (−129.5 dB) and the **band fails** (g/g₀ − 1 = −0.684). Alignment alone cannot see a latched-scalar error (P1); the band catches it. |

**Statistics** (n 4000 per side):

| Row | Measured | Control |
|---|---|---|
| **PS5** filter_rand 70 %, LP 1 kHz | D2 0.017, D1 0.019 / 0.016, Δmean 0.022σ, var ratio 0.990, range ±1.399 | ours rand 35 % → D2 0.264, range ±0.700 |
| **PS6** ring_rand 70 %, 200 Hz | D2 0.028, D1 0.015 / 0.015, Δmean 0.034σ, var 1.031, range −1.399 … 1.400 | ours 35 % → D2 0.249 |
| **PS7** crush_prob 30 % | p̂ ours 0.3035, parent 0.2993; \|Δp\| 0.004; crush bit == `crushed` on every grain on both sides | ours prob 15 % → p̂ 0.148 |

## Engine gates (2.3 rows; every control fired)

| Row | Metric · margin | Control |
|---|---|---|
| FU1a–FU4n (15) | Ported unit rows (PD14). BP at fc worst −0.0025 dB; LP/BP/HP vs the oracle × make worst +0.0003 dB (249 cases); drive 0.5 ulp; crush lattice / hold / silence exact; ring drift −141.0 dB; carrier 0.0002 Hz | FU1b inline: oracle without × make → −10.0 dB. FU3n and FU4n rows. |
| FU1i / FU2i / FU3i | INFO. FU1i prints both the pre-D10 (+20.01 dB at res 100) and the D10 (≈ +10 dB) expectations | — |
| FN1 | 33/33 bitwise: chunks 1 / 3 / 17 vs 32 over 11 extremes, including 0.45·fs | per-call ring renorm → 174 230 differ |
| FN2 | blocks 441 / 512 / 4096 / 8192 / var: 0 differ (every FX, s2 BP→HP) | seed + 1 → 127 998 |
| FN3 | 108 unread params → 0 differ; 153/153 bound | `s1_filter_cutoff`, `s4_ring_freq` → 127 998 each |
| FR1 | 14/14 = renders-2.2 | N1-default + `s1_drive` 1 % → hash differs |
| FV1a | 300 grains, log2 span 3.981 (margin 0.381), 0 out; rand 0 100/100 == base; 44.1 k: 86 at exactly 19 845.0, 0 above | rand 50 % → span 1.991 |
| FV1b | span 3.929 (margin 0.329); rand 0 exact; 44.1 k: 6 at the clamp of 2400, 0 above | rand 50 % → 1.964 |
| FV1c | p0 0/2000, p100 2000/2000, p50 1052/2000, z 2.33; bit == crushed | the same log vs p = 0.3 → \|z\| 22.06 |
| FV1d | 10/10 configs; 400 grains, 3630 live-grain scans, 0 bad fields | wrong table → 1 config mismatches |
| FV2 | Spearman ρ **0.999** over 60 isolated grains (centroid span 3.61 oct) | `fxSharedLatch` → ρ −0.006 |
| FV3 | 58/58 USB carriers within 2 Hz | `fxSharedLatch` → 1/58 |
| FV4 | 78/78 agree (42 crushed / 36 clean; crushed min −8.9 dB, clean max −46.8 dB) | `fxSharedLatch` → 42/78 |
| FB1 | poisoned vs clean: max\|Δ\| **0**. 12 811 spawns, 6616 R3 kills, 6209 native steals | poison + `fxNoReset` → 0.0558 |
| FB2 | B on silence peak **0.0** exactly; same slot 0; A shadow \|ic\| 5.0 | `fxNoReset` → B peak 6.7e-4 |
| FB3 | 2146 checked, **0** mismatches; fade SVF state advanced 557× | `stealSnapPerturb` → 2146/2146 |
| FB4 | `fxStateNonDefaultAtSpawn` **0 / 0** (12 811 spawns each; 12 779 kills / 12 555 steals) | poison + `fxNoReset` → 12 811 |
| FZ1 | 44.1 / 48 / 96 k: 0 non-finite, 0 subnormal FX-state fields in the per-tick scan, 0 NaN kills. Peak INFO 1.53 / 1.71 / 1.73. | `injectNan` + `nanKillOff` → 4703 non-finite; scanner fed `denorm_min` → 1 |
| FZ2 | 13/13 (NaN / ±Inf × 4 fields + 1 fade slot): each case has 1 kill, the slot retired, and output bitwise ≡ the `forceRetire` render | `nanKillOff` → 6400 non-finite |
| FA3 | **0** audio-thread allocations over 116 blocks: every FX on, 3 `filter_type` switches, 19 voice steals, cap 256 → 32, 1 NaN kill | liveness malloc → 1 |
| N2 / N2s | moved to `readSet23()` (PD10): 108 unread → 0 differ; 153/153 bound | their own (fired) |

## CPU (`--cpu23`, run alone, Logic closed)

Each timed config has the calib value before and after it, as a ratio to `ref_ns`. Every witness is valid: before ≤ 0.998 × ref, after / before ≤ 1.009, and duty ≥ 0.992.

| Config | Median (min … max) | Live | Verdict |
|---|---|---|---|
| bypass (streams disabled) | 0.31 % | 0 | INFO |
| **C22r** (C22 recipe, FX off, same session) | 14.02 % (13.97 … 14.11) | 243.2 | reference |
| **C23w** (D19 worst case: 4 streams BP 1 kHz res 100 + drive 100 + crush 100 / prob 100 + ring mix 100) | **26.08 %** (25.92 … 26.34) | 243.2 | **PASS ≤ 36 %**, margin 9.92 points |
| **C23t** (D20 typical proxy, PD1) | **5.41 %** (5.40 … 5.43) | **72.0** (inside [60, 90]) | **PASS ≤ 12 %**, margin 6.59 points |
| C23w96 (C23w at 96 k / 128) | 51.68 % | 243.2 | INFO |

- **Δ = C23w − C22r = 12.06 points**, which is **0.0496 % per grain**. The parent's figure is 0.064 %/grain. The projection was ≈ 30–31 % (R§6.1); the measured 26.1 % is below it.
- **C23s:** C22r scaled to the reference machine speed is 14.08 %; C22@2.2 scaled is 14.52 %. The difference is **−0.44 points**, so the gather / scatter restructure costs nothing measurable. There is no flag and no lever-3 / P18 candidate.
- **T11 lever ladder: not fired.** Lever 1 (Hermite) was not touched (PD9).

## Plan decisions

- **Applied as written:** PD1–PD4, PD6, PD8, PD9, PD11–PD13 and PD15 (version 0.1.0, no tag, `CMakeLists.txt` not edited).
- **PD5:** material peaks 0.1 / 0.25 / 0.4, with PFC8 at −6 dB level. The correction holds: at 0.4 peak PFC8 is live, with ref −22 dBFS and parent peak 0.250.
- **PD7:** 48 k null rows never reach a clamp. FU1a, FU1c, FU4a and FV1a / FV1b at 44.1 k gate the clamp instead.
- **PD10:** `readSet23()` holds 153 IDs; N2 and N2s use the literals 153 / 108.
- **PD14:** the FU helpers at `:74-137` were ported with the rows. FU1i prints both expectations, and FU1b carries the inline make-down control.

`GiRenderer.h` statement shape (T6 done-when, P5), before and after:

```
before:  const float gk = w * g.killGain * fader.next();
         bl[c] += l * gk * g.gainL;
         br[c] += r * gk * g.gainR;
after:   GK[n] = w * g.killGain * fader.next();  L[n] = l;  R[n] = r;  ++n;      (gather, same position)
         … FX (if fxa != 0) …
         bl[c0 + j] += L[j] * GK[j] * g.gainL;  br[c0 + j] += R[j] * GK[j] * g.gainR;   (scatter)
```

The operand order and single statements are unchanged, and FR1 / RB1 prove the result is bitwise.

## Deviations (with reasons)

1. **FB1 stimulus: cap 256 with `thinningOff`, then the cap drops to 32 at 1 s.** The plan specified cap 32. The first run at cap 256 + `thinningOff` gave 0 R3 kills, because a saturated pool has no free slot. A cap-32 render never fills the 256 live slots, so it never gives a native steal. The cap drop puts both paths in one render (6616 kills, 6209 steals). This is a stimulus fix; the 0-difference threshold is unchanged.
2. **FB4 kills render uses `thinningOff` at cap 32.** Under shipping thinning, cap 32 spawned only 224 grains, short of the ≥ 1000 the row needs. FB3 and the FZ2 fade-slot case also use `thinningOff`, because native steals need a full pool (the 2.1 H3 precedent).
3. **FZ1 control injection targets a young slot-0 grain.** The first fixed 0.5 s injection landed on a grain that retired inside the same block, and the Crush-100 % hold (48 samples) kept emitting its last finite value, so the control read 0 non-finite. The control now takes the first slot-0 onset ≥ 0.5 s from an identical first render and injects 64 samples into that grain. The detector itself is unchanged.
4. **FB2:** our engine reads a tape, not live input. The tape holds a 1 kHz sine for 1.5 s, then silence. `s1_position` moves 0 → 75 % between grain A and grain B. Density is 2 and size 200 ms, so B reuses A's slot 0.
5. **FV1d** has 10 configs. A "crush 1 prob 100" row was added so the crush-bit-set case is in the table.
6. **Extra test hooks beyond PD12:**
   - `fxSubnormalScan` / `fxSubnormals` drive FZ1's per-tick pool scan.
   - `fxstate::{copy, stateEq, latchEq, poison, subnormals}` helpers are in `Engine.h`, inside `OGI_TEST_HOOKS`.
   - The retire shadow is written whenever the renderer returns false or the fade reaches silence.
   - `legacySvfMake` also applies to the `fxSharedLatch` re-plan.
   None of these compile into the shipping build: X2 finds 0 symbols, and the shipping kernel adds only `if (fxa != 0)`.
7. **Parity plumbing:**
   - Both sides now set every FX row explicitly through `fxBase()`.
   - `parent_render.cpp` gained parent-side self-checks: peak ≤ 0.45 per PF row, `fxActive` == expected per grain, PS5 / PS6 bit ≥ 99 %, and PS7 bit == `crushed` with 0 < p̂ < 1.
   - The manifest carries the landed FX raws and the peak, appended at the end of each line.
8. **The FU port was written by a subagent** from `git show 82406fce:…` and then reviewed here. The FU1b control reuses the main loop's measurement, so it adds no render.

## Carried

- **Hands-on:** HANDS-ON-2.1 (13 rows), HANDS-ON-2.2 (10 rows) and **HANDS-ON-2.3 (9 rows, new)** are deferred by the user to after Stage 3. None is self-certified.
- **Listening notes due in HANDS-ON-2.3:** the R22 crush silence threshold and the R19 LP / HP resonance vs the wet safety.
- **Phase plan:**
  - 2.4: feedback, Freeze, Reset, reverb, and the final 40 % worst-case verdict. C23w is 26.1 % today, which leaves 13.9 points of headroom.
  - 2.5: modulation of the FX destinations b+17…b+25.
  - Stage 4: the typical verdict on real presets (C23t is a regression row).
  - Stage 3: the FX UI.
