# Stage 2 — DSP · Phase 2.3 (Per-grain FX + CPU budget) — Summary

**Plugin:** O-Granulator · **GSD phase:** execute · **Date:** 2026-11-05 · **Branch:** `main`
**Inputs:** PLAN-2.3.md (T1–T14), RESEARCH-2.3.md (R13–R22), CONTEXT-2.3.md (D6–D8)
**Gate:** intra-stage phase (2.2 → 2.3); 2.2 verified at `c10d0f2c`

**Commits:**
- `b342cb60` T1: the N0 neutral-FX out-of-tree null (`tests/n0/`), committed alone, before any FX code (P17)
- `18ee2561` T2–T4: the tracer (plumbing, `GrainFx.h`, the gather → FX → scatter kernel, SVF end to end, U1–U4, V2, N6 SVF form)
- this commit: T5–T14 (drive / crush / ring wired, the remaining hooks, V1/V3/V4, H, Z, K2, N6 all-FX, M3, C3, docs)

## What was built

Every grain now runs a latched per-grain FX chain in ARCHITECTURE §9 order: read → SVF → drive → crush → ring → window·gain·pan.

| File | Role |
|---|---|
| `Source/dsp/GrainFx.h` **(new)** | Header-only free functions. Latch: `randOct`, `svfLatch`, `driveLatch[D]`, `crushLatch`, `ringLatch` — the only `tan` / `pow` / `exp2` / `cos` / `sin`. Stages: `svf` (TPT, LP `v2` / BP `k·v1` / HP), `drive` (`norm·tanh(d·x)`), `crush` (fractional hold + `nearbyint` mid-tread), `ring` (rotator + per-sample Newton renorm). `svfStateFinite` |
| `Source/dsp/EngineConstants.h` | FX constants (`kFxNyquist 0.45`, `kFxRandOct 4`, `kSvfKMax/Span`, `kDriveRef 0.25`, `kDriveGain 16`, `kDriveGuard 1e-4`, crush constants), `kFx*` bits, `kSvf*` modes. `Slot` enum unchanged (R20) |
| `Source/dsp/Grain.h` | `+ driveNorm`; `crushHoldPhase` default 0 → **1** (P24). Still trivially copyable |
| `Source/dsp/ControlGrid.h` | 10 FX pointers per stream; `GridParams::Stream` FX fields (raw Hz for cutoff / ring, never a hand-inverted skew) |
| `Source/dsp/Spawner.h` | `GrainPlan` FX latch + telemetry; `planFx()` (draws `FxCutoff` / `FxCrushProb` / `FxRing` behind their gates, before `acquire`); `assignFxLatch()` (latch only, never state), called by `fill()` |
| `Source/dsp/Engine.h/.cpp` | `renderSegment` = gather → FX → scatter (P18) with the P25 NaN kill. `SpawnRecord` FX fields; `Diag` `nanKills`, `fxStateNonDefaultAtSpawn`, `stealStateChecked/Mismatches`, `nanKillNotRetired`; `TestHooks` `fxSharedLatch`, `fxNoReset`, `fxLiveCoeffs`, `snapshotSteals`, `injectNan`, `forceRetire` (all `OGRAN_TEST_HOOKS` only); retire shadow |
| `Source/PluginProcessor.cpp` | 4 × 10 FX pointers. No new params, no state-codec change |
| `tests/n0/run.sh`, `n0_render.cpp` **(new)** | N0 out-of-tree null vs `c10d0f2c` + N0c + N0n-F/D/C/R, `--strict` |
| `tests/render-harness/engine_gates_23.h`, `engine_gates_23b.h` **(new)** | U / V / H and N6 / Z / K2 / M3 / C3 rows. `main.cpp`: includes, section calls, and an `--only23` iteration flag |

grain-engine 1.1.0 and pitch-collections 1.0.0 are untouched (`git diff --stat c10d0f2c -- modules/` empty).

## Gate results

**Harness: 175 PASS, 0 FAIL (exit 0)**, 0 new warnings (plugin and harness); 2:08 wall. Carried-row diff vs the T1 baseline: **empty** over the 132 deterministic carried rows. S0 steal-free 0 diffs and W2o +0.483 dB are unchanged (see deviation 4 for the B2 rows).

### N0 / R12 (out of tree)

| Row | Result | Measured |
|---|---|---|
| N0 free (4 streams, ΣN 100, cap 128, neutral non-default FX dials) | PASS | max\|Δ\| 0, 0 bitwise diffs; 601 spawns, 0 steals / kills both sides; planCalls 151/150/150/150 |
| N0 capped (ΣN 400, cap 32) | PASS | max\|Δ\| 0; 2371 kills both sides |
| N0c seed + 1 | PASS (differs) | 568,972 diffs |
| N0n-F / D / C / R (`--strict`) | PASS (all DIFFER) | IDENTICAL at T1 (no FX code), each flipped as its stage landed (F at T4, D/C/R at T5–T7) |
| R12 vs `6508c635` | PASS | free and capped max\|Δ\| 0 |

### New harness rows (40)

| Row | Result | Measured |
|---|---|---|
| U1a BP k·v1 at fc, 220 cases (res × fc × fs) | PASS | worst −0.0025 dB (192 k, 20 Hz, res 100 %) |
| U1b LP/BP/HP vs bilinear oracle | PASS | 249 cases, worst +0.0003 dB |
| U1i (INFO) LP/HP peaks res 50/80/90/100 % | — | +0.98 / +6.63 / +10.84 / +20.01 dB (R18 exact) |
| U1c k 0.1 at 0.45·fs, noise 10 s, 48/192 k | PASS | all finite, peak 4.27 |
| U1d `svfStateFinite` | PASS | 12/12 false, finite true |
| U2a y(±r) | PASS | worst 0.50 ulp |
| U2b −12 dBFS peak law, 103 drives | PASS | worst −0.000001 dB |
| U2c guard | PASS | 1.35e-7 rel at d = 1e-4; below the guard: inactive, defaults |
| U2i (INFO) RMS / fundamental | — | RMS +0.001/+0.044/+0.573/+1.578/+2.237 dB (R15 exact) |
| U3a silence → 0, 100 depths | PASS | 600/600 |
| U3b mid-tread lattice | PASS | 40,001 points × 100 depths, 0 bad |
| U3n mid-rise control | PASS (control fails) | \|q(0)\| = ½ LSB at 100/100 depths |
| U3c hold law | PASS | 56,121 runs, mean error 1.8e-5; 998 runs of 48 at 100 % / 48 k; fresh hold OK |
| U3i (INFO) R22 thresholds | — | −12.0 / −17.9 / −20.5 / −33.1 dBFS |
| U4a ring drift, 5 f × 4 fs, 2 s | PASS | worst −141.0 dB (5 kHz @ 96 k) |
| U4b carrier frequency | PASS | worst 0.00023 Hz |
| U4c mix law | PASS | exact |
| U4n per-32 renorm control | PASS (control fails) | −115.6 dB at 5 Hz / 48 k |
| V1a cutoff census | PASS | span 3.99 oct in [−1.99, +2.00]; rand 0 101/101 bitwise; 94 draws exactly at 19 845 Hz, 0 above |
| V1b ring census | PASS | span 3.98 oct; rand 0 101/101; 11 at 19 845 Hz, 0 above (2401 grains) |
| V1c crush Bernoulli | PASS | p0 0/2002, p100 2002/2002, p50 971/2002 (z −1.34) |
| V1d `fxActive` gate table | PASS | 9/9 configs; clear-bit stages keep defaults (3276 live-grain scans) |
| V2 BP centroid ρ | PASS | 0.9974 over 58 grains (span 3.67 oct) |
| V2n shared latch | PASS | ρ −0.34 (span 0.13 oct) |
| V3 ring carrier | PASS | 58/58 within 2 Hz |
| V3n shared latch | PASS | 0/58 |
| V4 crush split | PASS | 78/78 agree (34 crushed / 44 clean); crushed ≥ −8.9 dB vs clean ≤ −53.5 dB out-of-band |
| V4n shared latch | PASS | 34/78 agree |
| H1 slot reuse after resonant grain | PASS | B peak exactly 0; same slot 0; A shadow \|ic\| 4.74 |
| H1n `fxNoReset` | PASS | B peak 6.8e-4 |
| H2 steal copy | PASS | 673 checked, 0 mismatches; fade SVF state advanced 160× |
| H3 fresh state at spawn | PASS | 0 / 1602 (1570 kills), 0 / 1441 (1313 steals) |
| Z1a max everything × 5 | PASS | all finite, 0 subnormal in 432 slot scans per run, nanKills 0; 8003 spawns / 7875 steals @ 48 k LP |
| Z1b hottest path | PASS | finite, 0 subnormal, nanKills 0 |
| Z1b (INFO) wet peak | — | 4.16 (+12.4 dBFS) |
| Z2 NaN/Inf × 5 (4 integrators + fade slot) | PASS | 5/5: finite, 1 kill, retired, max\|Δ\| 0 vs `forceRetire` |
| K2a cutoff sweep | PASS | κ ratio 1.170 |
| K2i (INFO) sweep with live coeffs | — | 1.159 (vacuous as P27 predicted) |
| K2b ring sweep | PASS | 0.800 |
| K2c cutoff step (latched) | PASS | 1.615 |
| K2n live-coefficient step | PASS (control fails) | 105.5; a grain alive across the step |
| N6 all FX, 441/512/4096/var vs 32 | PASS | 0 diffs; bits 0xF, LP/BP/HP 272/422/569, 668/1357 crushed, 1302 kills |
| N6c seed control | PASS | 544,304 diffs |
| M3 liveness / 0 allocations | PASS / PASS | 1 / 0 allocs (2883 spawns, 2754 steals, 1 kill, 1 NaN kill, 14 rebuilds) |

### CPU (P26 phase gate) — 48 k / 128, Release, median of 3 × 10 s

| Config | % of one core | Note |
|---|---|---|
| C3b bypass | 0.12 % | |
| C2 (2.2 row) / C2′ | 9.24 % / 9.11 % | vs 8.92 % at 2.2 verify (+2–4 %). Below the 9.8 % lever threshold |
| **C3w worst** (128 grains, LP res 100 % rand 100 %, drive / crush / ring 100 %) | **17.33 %** | +17.21 over C3b; 1.90× C2′ → **within 30 %** |
| **C3t typical** (4 × 16 grains, LP res 20 % + drive 30 %) | **7.31 %** | **within 10 %** |

**P18 lever decision: not applied.** C2 in the gate run measured 9.24 %, and the standalone C2′ measured 8.99 % and 9.11 %. All are ≤ 9.8 %, so `fxActive == 0` grains use the single kernel. N0 proves it is bitwise 2.2 either way.

### Validation

- `./scripts/build-and-install.sh O-Granulator` → `O-Granulator-dev.{vst3,component}`; dual-variant sweep and AU cache clear OK.
- `auval -v aufx OuGn OuDv`: **AU VALIDATION SUCCEEDED**.
- pluginval strictness 10, in-process: **VST3 SUCCESS, AU SUCCESS** (incl. fuzz parameters).
- Static checks:
  - `StateVariableTPTFilter` / `IIR::` in `Source/`: 0.
  - `std::vector` / `new` / `malloc` / `make_unique` in `GrainFx.h`: 0.
  - `tan` / `pow` / `exp2` / `cos` / `sin` in `GrainFx.h`: only in `randOct`, `svfLatch`, `crushLatch`, `ringLatch`.
  - `modules/` diff vs `c10d0f2c`: empty.
  - Grain static_assert holds.

## Errata (vs ARCHITECTURE / ROADMAP / CONTEXT-2.3; ROADMAP not edited — checksum)

1. **D6 drive law** replaces ARCH §9 `tanh(dx)/d`, `d = 1 + 15·drive²` and ROADMAP 2.3 "tanh(dx)/d". It is now `y = r·tanh(d·x)/tanh(d·r)`, r = 0.25, d = 16·drive², inactive below d = 1e-4. It is peak-unity: RMS rises +2.24 dB at full drive (measured +2.237).
2. **D7 crush** replaces ARCH Algorithm §9 mid-rise / ROADMAP "mid-rise bits". It is now mid-tread `nearbyint(x·2^(b−1))/2^(b−1)`, ties to even; silence stays 0 (U3a).
3. **D8 SVF BP** replaces ARCH "BP = v1". It is now BP = k·v1, measured at 0 dB (worst −0.0025 dB). LP/HP keep their raw peak (+20.01 dB at res 100 %). The output law ignores FX.
4. **R14 ring renormalisation is per sample** (Newton), replacing ARCH / CONTEXT "per pass". The worst corner is not 0.45·fs. Per-sample worst −141.0 dB; the per-32 control reads −115.6 dB at 5 Hz.
5. **R20 draw slots:** the reserved `FxCutoff` / `FxCrushProb` / `FxRing` are used; nothing is appended.
6. **R15 D6 gate metric** = steady-state sample peak.
7. **P25 NaN kill** = the segment's scatter is skipped and the grain retires (replaces CONTEXT "write 0 for the rest of the pass").
8. **P18 kernel:** per-stage ifs hoisted per segment, not specialised loops. C2′ 9.11 %; lever not applied.
9. **P26:** C3 is INFO in the harness exit code; the budget (≤ 30 % / ≤ 10 %) is the phase gate. Met: 17.33 % / 7.31 %.
10. **P27:** the K2 negative control is a step (K2n 105.5×). The sweep control K2i reads 1.159, which confirms it would have been vacuous.
11. **P28:** the crush per-grain audio row is a spectral split (V4), at crush 80 % with a sine at 0.5.
12. **D8 / R19:** QUAL-02 gates on finite only; the wet peak is INFO.
13. **`Grain::crushHoldPhase` default 0 → 1** (P24).
14. **U4b** uses a frequency criterion (≤ 0.01 Hz); measured 0.00023 Hz.
15. Execution deviations: see below.

## Execution deviations

1. **N0 parameter exactness → "landed + identical raw checksum".** On skewed ranges, density 25, size 1000, filter_cutoff 1000 and window_skew −40 cannot be reached exactly through the normalised path (e.g. density lands at 24.9999962). So the N0 non-vacuity condition "every param exact on both sides" is now:
   - every param landed within 1e-5 of its span, and
   - an FNV checksum of the 65 raw values is **identical on both sides**.

   This is the property the null actually needs. Exact count: 52/65 on both sides.
2. **U3b gate form.** `scale = 2^(15 − 14·crush)` is a power of two only at whole-bit depths, so `q·scale` is an integer only to rounding. The row now gates three things: the recovered index (`nearbyint(q·scale) == nearbyint(x·scale)`), |q − x| ≤ ½ LSB + ulps, and exact idempotence. The plan's literal "q·scale is an integer" failed on 377k of 4M points, every one of them a rounding artefact.
3. **U3c gate form.** The float phase accumulator drifts by ~1e-6 per run, so one run in 56k is one sample past floor/ceil(1/inc) (a 49-sample run at 48 k / 100 %). The row now gates runs within floor−1 … ceil+1 and the **mean** run = 1/inc within 0.1 % (measured 1.8e-5), plus 998 runs of exactly 48. This is a rounding effect, not a law change.
4. **Carried-row diff excludes the three B2 "defaults: wet present" rows.** That processor is never seeded (per-instance `Random::getSystemRandom()` seed), so its RMS jitters between runs. Example: −14.2 vs −14.1 dBFS between two runs of the same tree.
5. **V1b clamp sub-render lengthened** to 12 s × 200 g/s (2401 grains). The ring base (5 kHz, the range end) reaches 0.45·fs only for u > 0.997, so the plan's 201-grain render hit the clamp 0 times (vacuous). It now hits it 11 times. **H3 steals render** lengthened to 6 s (1441 spawns, ≥ 1000).
6. **V1a "rand 0 → fcHz == 1000.0f"** compares against the latched `GridParams` cutoff, which here is 1000 exactly, as recorded.
7. **Z1a ran steals, not kills.** At cap 128 with N 1600 there is never a free live slot, so the R3 kill path cannot fire (7875 steals). Kills under full FX are covered by H3 (1570 kills) and N6 (1302).
8. **Z2 "slot retired" is checked inside the engine.** `Diag::nanKillNotRetired` is checked right after `pool.render`, before any spawn can reuse the slot. A block-granular harness check could not tell the target from a grain spawned into the same slot within the firing block.
9. **Z1b wet peak +12.4 dBFS, not R19's ≈ +38 dBFS.** Z1b keeps Z1a's filter_rand 100 %, so most grains' cutoffs miss the 2 kHz tone. R19's +38 dBFS is the rand-0, coherent, tone-at-fc case. INFO only.
10. **Task batching.** T2 + T3 were verified in one cycle, and T5–T7 in one. The carried diff and N0 were run at T4 (tracer) and T8. Also, U2–U4 landed in the tracer commit: they test `GrainFx.h` functions only.
11. **`--only23` harness flag** (test-only iteration aid). The gate run (no arguments) runs every section.

## Carried to the 2.4 discuss

- **R19: resonant LP/HP headroom vs wet safety.**
  - Measured: Z1b +12.4 dBFS with rand 100 %. Up to ≈ +38 dBFS coherent (R19).
  - Option (a): accept ARCH §14 `1.9953·tanh(x/1.9953)` as the safety.
  - Option (b): a latched per-stream `1/peak(k)` make-down, which would amend D8.
- **R22: crush silence threshold.** Measured: −12.0 / −17.9 / −20.5 / −33.1 dBFS at 100 / 93 / 90 / 75 %. Hear it at the 2.4 listening checkpoint.
- **W2o rule:** widen seeds, never the threshold. It is unchanged at +0.483 dB.
- **C2 drift:** C2 rose from 8.92 % to 9.1–9.2 % with the single kernel. That is within the lever threshold. Re-measure in 2.6 with feedback and reverb.
- Still to come: the full PERF-02 worst case with feedback and reverb (2.6); QUAL-02 max feedback (2.4 / 2.6); `getTailLengthSeconds` (2.6).
- **Dev-build caution:** extreme res / level settings can exceed 0 dBFS until 2.4's wet safety exists. Keep monitors down.

## Requirement status (for verify; REQUIREMENTS.md untouched)

- DSP-05: expected complete.
- QUAL-02: partial (max feedback is 2.4 / 2.6).
- QUAL-01: partial (freeze is 2.4).
- PERF-01: partial (freeze, matrix).
- PERF-02: partial (2.6 full worst case).
- PERF-03: partial (feedback / modulation re-gates).

## Next

`/plugin-verify O-Granulator 2-dsp` (Phase 2.3).
