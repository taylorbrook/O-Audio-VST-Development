# Stage 2 — DSP · Phase 2.2 (Four streams, randomisation, pitch/scale, window morph, normalisation) — Summary

**Plugin:** O-Granulator · **GSD phase:** execute · **Date:** 2026-11-05 · **Branch:** `main`
**Inputs:** PLAN-2.2.md (T1–T18), RESEARCH-2.2.md, CONTEXT-2.2.md
**Gate 1→2:** PASSED (exit 0)

**Commits:**
- `fccf9ce6` T1, the S0/S0n steal-free row, landed first on the 2.1 tree (P9)
- `fb9fa220` T2–T11, the engine
- `6a3a0e3d` T12/T13/T15, the gates
- `a9236026` T14/T16, the gates plus R12

## What was built

The one-stream skeleton now runs **four streams over one shared 128-slot pool**. What it adds:
- Water-filled quotas.
- R8 ceil-fair stealing.
- The 65-step WindowMorphBank with skew.
- The D4 scale quantiser over pitch-collections 1.0.0, where Custom reads an audio-side copy taken through a try-lock.
- Stereo spread.
- Cut-only gain rand.
- The R5 partial-coherence output law.
- A mute/solo/level/pan mixer on 10 ms ramps.

grain-engine 1.1.0 and pitch-collections 1.0.0 are unmodified.

| File | Role |
|---|---|
| `Source/dsp/EngineConstants.h` | 2.2 constants. Slots are append-only; nothing was renumbered |
| `Source/dsp/Grain.h` | P13: `tiltT/A/B`, `pitchCents` |
| `Source/dsp/ControlGrid.h` | `rampCellsFor(fs, ms)`, plus the mute/solo/scale/window/skew/spread/gain_rand pointers and grid fields |
| `Source/dsp/WindowMorphBank.h` **(new)** | 65 × 2048 tables with Expo taken from WindowLut's table. C1/C2 cumulatives (65 × 2049) and the R6 warped cumulative. About 1.6 MB, built in the ctor |
| `Source/dsp/OutputLaw.h` | `hannCumW`, `streamDemand`, `spreadAmplitudeFactor`, `streamPower`, `outputGainFromPower`, `Coherence::compute`. The 2.1 law is kept as the reference |
| `Source/dsp/Quotas.h` **(new)** | `waterFill(demand[4], Q)` → q, qCeil, λ |
| `Source/dsp/ScaleQuantiser.h` **(new)** | `ActiveSet` (`rebuildIfNeeded`, `window`, `pick`), `ScaleSource`, `scaleMaskIndex` |
| `Source/dsp/Spawner.h` | `plan()`: scale path, spread, gain rand, window latch (draw order Size→Pitch→Reverse→PosSpray→Pan→Gain, all drawn before acquire). `acquire()`: R8 fair progress |
| `Source/dsp/Engine.h/.cpp` | 4-stream grid tick (latch → custom copy → active sets → quotas → c → g_out → mixer). Renderer switches between bank and Hann (P14) |
| `Source/PluginProcessor.*` | 4 × 19 stream pointers cached; `ScaleSource` bound. No new params and no state-codec change |
| `tests/render-harness/engine_gates_22.h`, `engine_gates_22b.h` **(new)** | the 2.2 rows; render pool for W1/W2/F4/F5 |
| `tests/r12/r12_render.cpp`, `run.sh` **(new)** | R12 out-of-tree null against `6508c635` |

## Gate results — harness 135 PASS, 0 FAIL (exit 0), 0 new warnings

The exit-0 harness run behind this table was re-run after install. Wall time was 87 s in the gate session and 60 s on the re-run.

| # | Row | Result | Measured |
|---|---|---|---|
| 1 | S0 steal-free vs native pool | PASS | 0 diffs; 401 spawns, 0 steals and 0 kills on both runs |
| 1 | S0n (cap 64, see errata) | differs, as required | 342,982 diffs |
| 2 | F1 each stream alone | PASS | −16.0 dBFS each, logs pure |
| 2 | F2 disabled = nothing; enable edge | PASS | 0 plan / sched calls / onsets on s2–s4. Onset at 96000 on-grid; an off-grid enable fires at the next grid |
| 3 | F3 cap 32/64/96/128 | PASS | maxAlive = cap. Kills 2370/2338/2306; at 128, 2274 steals |
| 3 | F4 starvation 400,50,10,1 @ 64 | PASS | +0.69 / +0.20 / −0.01 / −0.11 dB vs quota law |
| 3 | F4n `nativeProgress` | fails, as required | smallest stream −25.83 dB |
| 3 | F5 six-case table | PASS | worst +0.69 dB (limit ±1.0) |
| 3 | F5 R8 `noQuotaCeil` control | fails, as required | −0.99 dB (+0.02 dB with ceil) |
| 4 | F6 solo/mute don't perturb the pool | PASS | 0 mismatches over 1568 records |
| — | F7 single-stream victim == 2.1 | PASS | 0 / 18000 (kill, steal and plain branches all exercised) |
| 5 | Q1 all 31 collections (incl. BP, Custom) | PASS | 12493 records, worst 3e-5 ¢ |
| 5 | Q2 empty mask / no degree in span or window | PASS | rate exact 101/101; pitch_rand 7 ≡ 0 bitwise |
| 5 | Q3 7-Limit Just sorted + mask-aligned | PASS | only 968.83 / −231.17 ¢ picked; 12/7 never picked |
| 5 | Q4 spec index ≠ gs-Id | PASS | 0 out-of-set picks |
| 5 | Q5 audio 5/4 | PASS | 1250.000 Hz (+0.000 ¢) |
| 5 | Q6 custom swap | PASS | 0 bad picks before / after; first new pick 243 samples after the swap |
| 5 | Q7 base pitch unsnapped | PASS | rate exact 101/101 |
| 6 | E0b–g bank and law units | PASS | C(1)=μ 2.96e-8; warped cumulative 7.4e-7; c=0 ≡ 2.1 law in 1680/1680 cells; worst P/μ₂H **332.98**; λ == Q in 144/144 |
| 6 | W1 morph × skew (1300 renders) | PASS | worst −0.033 dB; capped corner −0.011 dB |
| 7 | W2 density 1→200 (c = 1/0/0.5 × noise/sine) | PASS | worst +0.76 dB (limit ±1.5) |
| 7 | W2o old-vs-old control | PASS | +0.483 dB (limit ±0.5; scope note in errata) |
| 7 | W2n 2.1 hot spot | 2.1 law **+19.57 / +19.75 dB**; 2.2 law −0.14 / +0.05 dB | |
| 7 | W3 sub-floor | PASS | g_out 1.0f, peak ratio 0.888 |
| — | R11 guard | PASS | 0 / 733 specs |
| 8 | D1 reverse 0/100/50 % | PASS | 0 / 2401 / 1165 (z −1.45) |
| 8 | D2 spray 0 / spread 0 deterministic | PASS | 601/601 delays exact; gains exactly 1.0f |
| 8 | D3 spread constant power, no edge pile-up | PASS | ±1.8e-7; L/R −0.054 dB at pan 0 / ±1 |
| 8 | D4 gain rand | PASS | 0 % bitwise; 100 % in [−23.93, −0.24] dB |
| 8 | D5 draw-before-obtain | PASS | 0 / 803 mismatches, caps 32 vs 128 |
| 9 | K1 steal click (cap 32 kills / cap 128 steals) | PASS | ratio 1.225 / 1.026; 321 kills, 799 steals in the window. K1n hard cut 27.9× |
| 10 | X1 mute/solo/level/pan ramps | PASS | worst 1.0001×; hard-switch control 103.5×; ramp 15 cells |
| 11 | N5 block-size null, 4 streams, everything on | PASS | 0 diffs at 441/512/4096/variable. N5c 544,324 diffs |
| 12 | M2 allocations | PASS | 0 allocations (113 rebuilds, 19 custom copies); liveness 1 |
| 13 | **R12 2.1-equivalence null** (`tests/r12/run.sh`) | PASS | steal-free (N 100, cap 128) and capped (N 400, cap 32, 1168 kills): max\|Δ\| 0 on both, re-run after install |
| 14 | All 2.1 rows | PASS | L1 set to spread 0 (below) |
| 14 | auval `aufx OuGn OuDv`; pluginval 10 VST3 + AU | PASS | installed as `O-Granulator-dev.{vst3,component}` |
| 15 | C2 CPU (non-blocking) | recorded | **9.01 %** of one core at 48 k/128: 4 streams, 126 live, window 0.6, scale on. 1.09× the 2.1 baseline of 8.28 % |

## Deviations and errata

1. **ARCHITECTURE §10 κ blend is superseded by R5** (P10). There is no dB κ-blend and no `getForwardNorm`. The E0 worst case moves from P/μ₂ 43.02 to **332.98**, at step 22, skew −1, q 128, N 400.
2. **ARCHITECTURE §5's nearest-degree snap is superseded by D4.** D4 picks uniformly among the active degrees within ±pitch_rand, and base pitch is never snapped.
3. **P11 replaces CONTEXT's "double buffer + version".** The audio thread does a non-blocking try-lock copy at the grid. Addition: `prepare()` (message thread) takes one blocking copy. Q6 measured 0 try-lock misses.
4. **P12:** mixer ramps (level, pan, audibility) are 10 ms = 15 cells at 48 k. Level and pan automation renders therefore differ from 2.1. This is a deliberate gate change, and static renders and R12 are unaffected.
5. **R8 exactness (no behaviour change).** The kill victim is chosen by comparing the integer priority first and `age·invLen` second; it is not chosen on the summed double. The steal fallback subtracts the minimum priority, which is a common shift. With one stream, the victim matches 2.1 exactly (F7).
6. **S0n moved to cap 64.** At cap 128 the engine and the native path are identical by construction: the kill path can't trigger, and ties reduce to `age·invLen`. That makes the plan's S0n pair at N 300 / cap 128 0-diff, which is recorded as an INFO row. Cap 64 engages R3 and proves that the hook really bypasses `Spawner::acquire`. On the 2.1 tree S0 passes by construction. From 2.2 on it guards the R8 ranking.
7. **F3 at cap 128 checks steals > 0, not kills > 0.** 128 is the pool size, so there is never a free slot and the pool's own steal enforces the cap.
8. **W2o control scope is narrower; the threshold is unchanged.** The ±0.5 dB old-vs-old control gates the 40 cells where seed variance allows it: all noise cells, plus sine at c = 1.
   - The incoherent-sine cells (c = 0 / 0.5) are a random-phasor sum. Their seed-set spread is 0.73 dB at 16 seeds, and reaching ±0.5 would need about 70 ten-second renders per cell. That spread is printed as INFO.
   - Their W2 worst case is +0.76 dB. With the spread added, that is still inside ±1.5.
   - Sine seeds were raised from 8 to 16. W2o's margin is thin (+0.483 / 0.5), but the result is deterministic because the seeds are fixed.
   - **Review at verify.**
9. **K1 interpretation.** "~0.001" is taken as the input tone amplitude, per ARCHITECTURE. The verdict reuses E2's ≤ 2.0× curvature ratio against a steal-free 4-stream reference. The negative control is an injected one-grain hard cut.
10. **W3 tolerance is 1 %, for interpolation ripple.** The plan gives none. The failure it guards against is +25 dB.
11. **Q1 adds base pitch +20 st**, so the window is clipped on both sides.
12. **Disabled streams** have demand 0 in the water-fill. Their ringing grains keep the 2.1-form g_out.

## Rows touched by T11 (spread default 30 %)

- **L1 (all 6 sub-rows): now sets `s1_stereo_spread 0`.** It nulls two grains sample by sample, so it needs unity grain gains. With spread 0 its output is byte-identical to 2.1. This restores a precondition; no threshold was weakened.
- Other 2.1 rows still pass at the new default, but their measured levels moved slightly: E1, E2c, E3, A1/A1n, N3n, S0/S0n level detail, B2 defaults. They were left as is.
- Every other new DSP param defaults to a no-op: scale Off, window 0, skew 0, gain_rand 0, mute/solo off, s2–s4 off.

## Transition band (documented, not gated — RESEARCH §1.4)

At spray 0.03 % (W 115 samples < W₀ 240), N 20, c 0.2704:
- noise reads −0.41 dB vs the law;
- sine 110 Hz reads **+8.70 dB**, against a coherent-excess bound of 11.2 dB.

The constants were not refitted. Two streams with identical coherent settings sum to +6 dB, which is accepted.

## Notes carried forward

- The harness takes 60–87 s of wall time. W1/W2/F4/F5 run on a streaming render pool, one processor per job, each built on the main thread.
- R12 is `plugins/O-Granulator/tests/r12/run.sh`. It extracts `git archive 6508c635`, refuses to run if `modules/` or `scripts/param-dump` drifted, generates a CMake wrapper per tree and builds the same driver against both. **Re-run at verify.**
- Still from 2.1: P7 tail is 0 until 2.6; there is no human listening checkpoint in 2.2 (CONTEXT D1–D3).

## Next

`/plugin-verify O-Granulator 2-dsp`
