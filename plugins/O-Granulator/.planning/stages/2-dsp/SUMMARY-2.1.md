# Stage 2 — DSP · Phase 2.1 (Ring + pass engine + one stream) — Summary

**Plugin:** O-Granulator · **GSD phase:** execute · **Date:** 2026-11-04 · **Branch:** `main`
**Inputs:** PLAN-2.1.md (T1–T17), RESEARCH-2.1.md, CONTEXT-2.1.md
**Gate 1→2:** PASSED (build, pluginval; schema SKIPPED, as at every phase-command transition)

## What was built

The Stage 1 pass-through is replaced by the 2.1 engine skeleton. It has:
- a 32-sample absolute pass grid with write-before-read;
- a 41 s `MipRing<2>`;
- one live stream (Free timing, telescoping jitter);
- the spawner, with a P1 reach floor and double read positions;
- `GrainPool<Grain,128,16>` with the R3 `grain_limit` cap;
- a Hann renderer whose reads all go through one R1-quantised `readGrain()`;
- the cap-aware incoherent output law;
- equal-power dry/wet and output gain on a control grid with exact-arrival ramps;
- the P4 mix-0 bypass.

The engine is shaped for 4 streams, but only s1 is honoured. `fbDelay` (z⁻³²) exists and holds only zeros. grain-engine 1.1.0 is unmodified (D1).

| File | Role |
|---|---|
| `Source/dsp/EngineConstants.h` | kCell / kFbDelay / ring derivation + static_asserts, CounterRng slot enum (reserved slots for 2.2/2.3) |
| `Source/dsp/Grain.h` | Grain POD at its final layout, trivially copyable |
| `Source/dsp/ControlGrid.h` | `ExactRamp` (no SmoothedValue), `snapDb`, `mixGains`, `GridParams` |
| `Source/dsp/OutputLaw.h` | Hann closed-form ∫w², single-normalisation `g_out(N, q)` |
| `Source/dsp/StreamScheduler.h` | telescoping jitter, persistent epoch (R4), `[4]` table |
| `Source/dsp/CaptureWriter.h` | I/O adapter, input-gain ramp, per-sample push + fb return |
| `Source/dsp/Spawner.h` | draw-before-obtain, reach floor (P1), R3 cap with lazy kill |
| `Source/dsp/Engine.h/.cpp` | per-pass order, renderer, stream mix, dry/wet/output, Diag + test hooks |
| `PluginProcessor.*`, `CMakeLists.txt` | engine wiring (17 params + s2–s4 enable mask); `OGRAN_TEST_HOOKS=1` on the harness only |
| `tests/render-harness/main.cpp`, `engine_gates.h` | rescoped B rows plus the E/A/L/N/O/M/R3 rows |

## Gate results — render harness: 77 PASS, 0 FAIL (exit 0), 0 warnings

| Plan # | Row | Result | Measured |
|---|---|---|---|
| 10 | G, D, S, P, B1, B3 (Stage 1) | PASS | unchanged |
| 3/10 | B2 mix 0 % bitwise, 3 layouts × 6 blocks, params set through the host normalised path (P8) | PASS | 6/6 per layout |
| 3 | B2 defaults: wet present | PASS | −14.2 / −14.1 / −14.2 dBFS |
| — | E0 output-law units | PASS | ∫₀¹w² error 0; continuity 0; P/μ₂ = 43.020 |
| 1 | E1 audible at position 0 | PASS | −16.1 dBFS; first onset at sample 0; position-100 % control is silent |
| 2 | E2 buffer_length 30 → 1 s | PASS | 61 post-change spawns, 0 over the bound, 0 reach violations; step ratio 1.000 vs the same-motion reference (an injected step reads 30.5×) |
| 3 | E3 mix 100 % | PASS | corr −0.0023 (mix-50 control: 0.877) |
| 3 | E4 R2 snap | PASS | the round-trip residue 7.15e-7 dB is real; snapped gain is exactly 1.0f |
| 4 | A1 alias +24 st, 44.1 k | PASS | tones −118.5 dB, sweep −94.5 dB |
| 4 | A1n `forceLevel0` control | fails the threshold, as required | −46.1 / −33.2 dB |
| 5 | L1 periodic-corpus null (rates 0.5 / ~1.5 / 4, 25.5 s reach) | PASS | max\|Δ\| 0 / 8.9e-8 / 0 |
| 5 | L2 double reference at ≥ 25.1 s | PASS | 0, 5.6e-8, 0 samples (a float accumulator is off by up to 8.8e-3) |
| 6 | N1 block-size null 441 / 512 / 4096 / variable (incl. 1, 8192) vs 32; all randomisation on, cap 32, enable toggle | PASS | max\|Δ\| = 0 ×4; 559 spawns, 495 kills |
| 6 | N2 different-seed control | PASS | 545,360 diffs |
| 6 | N3 R1 on / N3n `disableFracQuant` negative control | PASS / non-vacuous | 0 diffs over 108,340 hazard reads / 10 diffs (max 5.96e-8) |
| 6 | N4 R4 re-enable | PASS | 0/8 start delays repeat; second burst index = epoch 26 |
| 7 | O1 oversize (prepare 512 → 8192 / 4096 at position 0) | PASS | finite, canaries intact, 0 reach violations |
| 8 | M1 allocations + liveness | PASS | 0 allocations across 2104 spawns / 1073 steals / 903 kills / toggles / 119 buffer_length changes; the liveness malloc counts 1 |
| 9 | R3 grain_limit 32 / 64 | PASS | maxAlive 32 / 64; kills 569 / 537 |
| 11 | auval, pluginval 10 VST3 + AU | PASS | `aufx OuGn OuDv`; installed as `O-Granulator-dev.{vst3,component}`, no alternate variant found |
| 12 | T15 CPU baseline (non-blocking) | recorded | 128 grains, Hann, 48 k/128: **8.2–8.7 % of one core** (≈ 80× engine bypass at 0.10 %), harness fill included |
| 13 | D3 human listening checkpoint | **pending — at verify** | |

## Deviations and errata

1. **Erratum (output law):** ARCHITECTURE §10 double-counts μ₂. 2.2/2.3 must use `M_m(x)` in the single-normalisation form.
   - The worked example in PLAN T3 / RESEARCH §4 is also wrong. At 200 g/s × 2 s with q = 128, the correct values are M(0.32) = 0.12604, P/μ₂ = **43.02**, **−4.74 dB** vs naive. The plan gave ≈ 61 / −3.2 dB.
2. **R1 hazard is rarer than RESEARCH §5 predicted.** The plan's 20 s / 128-grain / 441-vs-32 negative control showed 0 diffs, so it was vacuous.
   - The rebuilt stimulus parks start delays just under 2^19 and uses the same 20 s / 128 grains / 441 vs 32. It shows 10 diffs in 108k hazard reads.
   - The quantisation is still unconditional (P2).
3. **R3 kills are applied lazily.** `GrainPool::getGrains()` is const, so a kill is recorded as `pendingKill[slot]`. `killStep` is set at the next `pool.render` before the grain renders another sample, which is equivalent to an immediate kill. A pending kill counts as killing for `nAlive` and for progress′.
4. **Pan law:** constant-power, normalised to unity at centre (√2·cos/sin). A hard pan is +3 dB on the remaining side.
5. **Seed changes** take effect at the next grid point.
6. **Disabling s1** lets sounding grains finish naturally, with no forced release.
7. **Harness:**
   - The alloc rows run on every pass instead of behind `--alloc-check`.
   - The exact-value setter checks the raw parameter value, because `get()` differs on skewed ranges.
   - The E2c tone is 437.3 Hz, because 437 Hz was cycle-aligned to the 3 s delay change.
8. **Missing row:** there is no separate steal-free bit-identity row (`grain_limit` 128, N < 128, vs native pool). PLAN Risks lists it. Steal-free behaviour is exercised inside N1/M1 but not isolated. Add it in 2.2 or at verify.
9. **Test-only additions in `Engine.h/.cpp`:** guard canaries and the R1 hazard counter. They compile into the harness only (`OGRAN_TEST_HOOKS`).

## Notes carried forward

- **P7:** `getTailLengthSeconds()` stays 0. 2.6 sets the real tail (grains ≤ 2 s, feedback, reverb).
- **P8:** the B2 defaults rows were replaced on purpose, so this is a gate change, not a regression.
- **Level at verify:** judge level only at defaults (spray 10 %, jitter 20 %). At spray 0, jitter 0 and pitch 0 the incoherent law runs about +7 dB hot by design until the κ blend lands in 2.2.
- **Memory:** 110 MB per instance at 192 k (D2). 2.6 adds the allocation-size check.

## Next

`/plugin-verify O-Granulator 2-dsp`. It includes the D3 listening checkpoint in Standalone or a DAW.
