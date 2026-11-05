# Stage 2 — DSP · Phase 2.4 (Freeze, feedback network, output reverb) — Summary

**Plugin:** O-Granulator · **GSD phase:** execute · **Date:** 2026-11-05 · **Branch:** `main`
**Inputs:**
- PLAN-2.4.md (T1–T14, P31–P45)
- RESEARCH-2.4.md (R23–R38)
- CONTEXT-2.4.md (D9–D12)
- Sign-offs: R29 tap law and R30 decay-gate scope

**Gate:** intra-stage phase (2.3 → 2.4); 2.3 verified at `6aed8017`. The six contract checksums matched at entry, and `Source/`, `tests/` and the modules were byte-identical to `6aed8017`.

**Commits:**
- `efba92b7` T1 — the N1 null (`tests/n1/`), B2 seeded, baseline A. Null drivers at `input_gain` −12 dB. The old side archives its own `modules/`.
- `3608d722` T2 — the R23 ring-domain anchor, alone.
- `85790739` T3–T5 — plumbing, the D10 make-down + shared `tptStep`, the D9 knee + `wet[2][kCell]`. Baseline B.
- `47fb3eb0` T6–T7 — the feedback tracer: `FeedbackChain.h`, the tap law, U7 / U8, FB1–FB5, N7 (feedback form).
- `66cce468` T8–T9 — freeze: the arbiter, edges, forced release, reflect, frozen geometry, FZ1–FZ8, N7 (freeze form).
- `63b49f2f` T10–T12 — reverb-unit 1.2.0 + integration, RV1–RV4, Z3 / Z3b / Z4, N7 final, M4, C4.
- this commit — T13–T14: LISTENING-2.4.md, this summary, STATUS.

## What was built

| File | Role |
|---|---|
| `Source/dsp/FeedbackChain.h` **(new)** | Header-only, fixed arrays (no allocation). `tptCoeffs` (the only `tan`), `fbSat` (D12 `tanh(dx)/d`), `agcCoeffs`, `diffuserLengths` (from `kDiffuserMs`), `allpassStep`, and `FeedbackChain` (HP → LP at `kFbK` → AP4 mix → sat → stereo AGC → NaN scrub → `+0.0f`) |
| `Source/dsp/CaptureWriter.h` | The Write Arbiter. `FreezeState`: no push while frozen; the release crossfade with the slope-matched mirror continuation (deviation 12). The live statement is verbatim |
| `Source/dsp/Engine.h/.cpp` | Changes listed below the table |
| `Source/dsp/Spawner.h` | `FrozenGeom` + `frozenDelay` (R24 / R27 floor origin, min spray ±½ grain, reverse cap, the deferral head-lead clamp), `requestKill`, `GrainPlan::svfMake`, `fill (…, W_on)` ring-domain |
| `Source/dsp/GrainFx.h` | `tptStep` (the 2.3 statements, shared, P36); `svfLatch` → `make = 1/√P(k)`; LP `m·v2`, HP `m·(x − k·v1 − v2)` |
| `Source/dsp/OutputLaw.h` | `wetKnee` (D9 branch form), `fbTapS1`, `fbTapGain` = 1/(S1 + 1), `fbTapGainArch` (test reference) |
| `Source/dsp/EngineConstants.h` / `Grain.h` / `ControlGrid.h` | 2.4 constants; `Grain::svfMake`, `Grain::frozenSpawn` (telemetry); 9 parameter pointers + GridParams fields (raw Hz for the skewed fb cutoffs) |
| `Source/PluginProcessor.cpp` | Caches the 9 new pointers |
| `modules/effects/reverb-unit` 1.2.0 | `bind (std::atomic<float>* × 7)` overload only (+ changelog). `modules/registry.yaml` → 1.2.0. O-Marimba builds unchanged (0 module warnings) |
| `tests/n1/` **(new)** | The N1 neutral null vs `6aed8017` + N1c + N1n-MK / KN / FB / FZ / RV (same variant on both sides) |
| `tests/n0`, `tests/r12` | Old side archives its own `modules/`; `input_gain` −12 dB both sides; `maxwet` printed |
| `tests/render-harness/engine_gates_24.h`, `engine_gates_24b.h` **(new)** | U5–U8, FB1–FB5 / N7, FZ, RV, Z3 / Z4, M4, C4. `main.cpp`: B2 seed, `--legacy-24[=knee\|svf]`, `--only24` |

Engine changes (`Source/dsp/Engine.h/.cpp`):
- R23 anchor: `W_on = W_piece + offset + 1` live, the stopped head when frozen.
- Freeze edges sampled at ticks, with the deferred stop (deviation 11) and the R25 forced-release scan.
- `readGrain`: the R25 reflect, and the reach diag split into releasing / non-releasing grains.
- The tap is formed in the mixer pass; the chain runs on every pass.
- `wet[2][kCell]` → reverb → knee, all ahead of the mix-0 fast path.
- The ReverbUnit and its 7 atomics are written at ticks.
- New Diag counters, TestHooks and canaries.

grain-engine 1.1.0 and pitch-collections 1.0.0 are untouched (`git diff --stat 6aed8017` empty).

## Gate results

**Harness: 225 PASS, 0 FAIL** (175 carried + 50 new), 0 warnings (plugin, harness, install). Carried-row diff vs baseline B: **empty** from T6 on.

### N1 / N0 / R12 (out of tree)

| Row | Result | Measured |
|---|---|---|
| N1 free (4 streams, 2.3 FX on and non-peaking, neutral non-default 2.4 dials) | PASS | max\|Δ\| 0; max\|wet\| 0.211 (< T − 1 dB = 0.447) |
| N1 capped (cap 32) | PASS | max\|Δ\| 0; 2371 kills both sides; max\|wet\| 0.065 |
| N1c seed + 1 | PASS (differs) | 568,972 diffs |
| N1n-MK / KN / FB / FZ / RV | **all DIFFER, `--strict` PASS** | Each flipped at its task: MK T4, KN T5, FB T7, FZ T9, RV T10 |
| N0 `--strict` (vs `c10d0f2c`) | PASS at every task | max\|wet\| 0.356 / 0.197 |
| R12 (vs `6508c635`) | PASS at every task | max\|wet\| 0.207 / 0.049 |

### Attribution (P33) — the D9 / D10 re-baseline list

- `--legacy-24` vs baseline A: **only U1c's detail** moves (peak 4.271 → 1.350). U1c is a direct-Grain unit sharing the edited `svfGrain` helper, which the legacy hooks cannot reach. Every processor render is bitwise, which also proves the T3 plumbing and the `tptStep` refactor exact.
- Normal vs A: 14 rows. This is exactly the union of the two single-hook runs; no row changes only with both hooks on.

| Cause | Rows (detail strings only; all still PASS) |
|---|---|
| **D9** (`--legacy-24=svf` isolates it) | D3, E2c, F5 200/1/1/1@32, F5 60/60/0.1/0.1@96, F5 R8 control, S0n, W1 capped corner, W2 c=1, X1 |
| **D10** (`--legacy-24=knee` isolates it) | K2a (LP res 50 %), K2n, N6c (res 60 %), Z1a (res 100 %) |
| Helper edit | U1c (and U1b's oracle × m — its detail did not move) |

**W2o is not on the list** (+0.483 dB, unchanged). B2 is deterministic: two baseline-A runs diff empty.

### New rows

| Row | Result | Measured |
|---|---|---|
| U5a / b / c (D9) | PASS | 2,113,969,054 floats identical; 176 M monotone, y ≤ x, max == float(A) 1.99526227; slope ≤ 1, T⁺ 1.000000 |
| U6a / b / c (D10) | PASS | m == 1.0f over [0, 0.3083]; first m < 1 at res 0.3085; res 100 % peak +10.0055 dB; P(k) ±0.0000 dB |
| U7a–f (chain) | PASS | Oracle worst +0.00057 dB, peak 0.99999416; diffuser 226/398/667/1042 & 207/366/613/957, \|H\| 3.5e-7 dB; AGC 0.70000, 48 / 19196 smp; R36 384,000/384,000 +0; scrub 9/9 |
| U8 / U8n (tap law) | PASS | Theorem excess ≤ −2.1e-3; worst generation −0.476 dB; worst over N ≥ 1 −0.177 dB (INFO); ARCH +4.758 dB |
| FB1 | PASS | +347 dB over fb 0 (the fb-0 tail is exactly silent) |
| FB2a / b / c / d | PASS | HP 3.33 dB, LP 2.08 dB, sat crest 2.58 dB, diffusion ρ 0.527 → −0.033 |
| FB3a (32 corners) / FB3b / FB3n | PASS | Slowest −27.9 dB/s (Tukey / lock / Off ≈ −2.2 dB/gen); FB3b slowest −36.5 dB/s; ARCH law locked **+41.4 dB/s** |
| FB4 | PASS | Peaks 0.22–1.43 (≤ A), finite, 0 subnormal, env ≤ 0.38 |
| FB5 | PASS | Tap L == R (hard pan); mute / solo / disabled max\|Δ\| 0 |
| FZ1 / FZ1n | PASS | Hash held 10 s / differs |
| FZ2a / FZ2an | PASS | **1.186×** / no-defer (P38 form) **23.85×**; stuck reads 11.78× (INFO) |
| FZ2b / FZ2bn | PASS | **1.147×** / old splice **112.7×** (the plain R26 mirror measured 6.98×) |
| FZ3 / FZ3n / FZ3c | PASS | ρ_H −0.001 / 0.990 / crest 1.10× (non-diagnostic) |
| FZ4 / FZ5 | PASS | −16.0 dBFS at 100 and 500 ms / drift 0.037 dB |
| FZ6 (T8) / FZ6 strict | PASS | 0 violations, 0 reflections, forced 8 == snapshot 8; no-defer at 0 st: 1920 reflections, 0 violations / 0 frozen-spawn reflections, 300 reverse spawns within heldCaptured |
| FZ7 | PASS | 7497/7497 tick pairs evolve, hash held, finite |
| FZ8 | PASS | Released −41.07 vs steady −41.62 dB/s (1.3 %, 4 s fit) |
| RV1 / RV2 / RV3 / RV4 | PASS | −24.9 dBFS, RT60 0.241 s / κ 1.52, residual −400 dBFS / κ 1.58 / bitwise dry, 2596 reverb cells |
| Z3 (48 / 44.1 / 192 k) / Z3b / Z4 | PASS | Peaks 1.58 / 1.53 / 0.93 / 1.86 (≤ A); 0 subnormal, 0 scrubs, 0 NaN kills; Z4 max\|Δ\| 0 vs the reset reference |
| N7 final / N7c | PASS | 441 / 512 / 4096 / var == 32 (value compare); engages 2, releases 2, forced 3, reverb cells 11107 / 272,161 diffs |
| M4 | PASS | 0 allocations, liveness 1 (7 engages, 1 scrub, 1 NaN kill, reverb toggles) |
| C4 (INFO) | — | **18.57 %** of one core (C3w re-run 17.80 %, +0.77); headroom to 30 %: 11.43 % |

### T13 validation

| Check | Result |
|---|---|
| `build-and-install.sh O-Granulator` | 0 warnings; dual-variant sweep; caches cleared |
| `auval -v aufx OuGn OuDv` | **PASS** (AU VALIDATION SUCCEEDED) |
| pluginval strictness 10, VST3 | **PASS** (SUCCESS) |
| pluginval strictness 10, AU | **PASS** (SUCCESS) |
| Static: `StateVariableTPTFilter\|IIR::` in `Source/` | 0 |
| Static: `std::vector\|new \|malloc\|make_unique` in `FeedbackChain.h` | 0 |
| Static: `std::tan` in `FeedbackChain.h` | `tptCoeffs` only |
| grain-engine / pitch-collections vs `6aed8017` | empty |
| reverb-unit `cpp` diff | the bind overload only |
| O-Marimba `OMarimba_VST3` against 1.2.0 | builds, 0 warnings from the module |

LISTENING-2.4.md is written (items 1–7 + verdict sheet). The listening checkpoint runs at verify.

## Errata (ARCHITECTURE / ROADMAP not edited — checksums)

1. **D9 wet safety** vs ARCH §14: a knee with identity ≤ −6 dBFS. The ceiling is **≤ A** (max y == float(A)).
2. **D10 amends D8:** LP/HP × 1/√P(k), with P depending on k only. Re-baseline list above.
3. **R29 tap law** (signed off): 1/(S1 + 1), S1 taken pre-spread, no pan in the tap.
4. **R30 decay-gate scope** (signed off): linear decay at FX gain ≤ 1, boundedness elsewhere. The gate metric is dB/s; dB/gen is INFO.
5. **R23** ring-domain anchor; the assert is rewritten.
6. **R24 / R27** frozen floor + floor origin; PosSpray is drawn at spray 0 while frozen.
7. **R25** reflect at the head is now a **safety net only** (see deviation 11). The `reachViolations` definition changes (non-releasing grains only).
8. **R26** release continuation: not `ring[W − heldSpan]`, and **not the plain mirror either** (deviation 12).
9. **R28** anti-buzz detector ρ_H; the crest row is kept, labelled non-diagnostic.
10. **R35** reverb mix ≤ 0.1 % is silently off; mix → 0 fades the tail in 50 ms. PERF-03 uses value compare.
11. **P33** attribution by legacy hooks; U1b's oracle × m (its detail did not move; U1c's did — helper).
12. **P43** B2 seeded; the three rows rejoin the carried diff.
13. **D11** reverb-unit 1.2.0, registry updated. The registry's `used_by` convention records the PLUGIN version, so the O-Granulator entry (0.1.0) is unchanged.
14. The `Engine.h` banner says 2.4 for the arbiter, tap, chain, reverb and knee.
15. **R29's growth claim needs phase lock.** The ARCH law normalises the *mean* Σw. At position 0 (T = 42 samples ≪ H) the per-generation log gain averages negative (Jensen), so the ARCH law decays too. It grows only when T = D − 1 + 32 is a multiple of H.
16. **PLAN U8 threshold** "peak · g ≤ −0.16 dB" cannot hold for N < 1: as S1 → 0, 1/(S1 + 1) → 0 dB (Expo at N 0.05 gives −0.089 dB with a peak exactly 1). It holds for N ≥ 1 (−0.177 dB).

## Execution deviations

1. **Null drivers at `input_gain` −12 dB on both sides** (the plan-sanctioned fix). At 0 dB the cases sat above the knee: N1 free 0.786, N0 free 1.416 / capped 0.786, R12 free 0.825, against T − 1 dB = 0.447.
2. **The old side archives its own `modules/`** in N1 / N0 / R12. Otherwise reverb-unit 1.2.0 would make N0 / R12 refuse at T10. The refusal is narrowed to grain-engine, pitch-collections and param-dump.
3. **N1n controls render the same variant on both sides.** N1n-KN uses `input_gain` +12 dB, because `s1_level` tops out at +6 dB.
4. **Feedback rows are named FB1–FB5.** 2.2 already owns F1–F5, and the carried-diff filter needs distinct prefixes.
5. **U6a sub-check.** m is continuous, so it rounds to 1.0f just past 0.3083 (m = 1 − 5e-8 at 0.3084). The row checks "first m < 1 by res 31 %" (it is at 0.3085).
6. **H3 excludes `svfMake`.** It is a latch, not state, and including it would false-positive. H2 includes it.
7. **U8 restated** as the unimodal theorem (peak ≤ S1 + max w) plus per-generation decay with the 0.95. The plan's −0.16 dB is printed as INFO (erratum 16).
8. **FB1 / FB2a / FB2b use long loops** (position 1.25 % / 5 %). At position 0 the 42-sample loop decays by thousands of dB/s, so the windows read the floor.
9. **FB3a runs 32 corners:** position 0 plus phase-locked D + 31 = H. Locked corners are fitted per generation (80 ms frames); a dB-domain 10 ms fit averages log levels across phases. FB3n runs locked at −120 dBFS so the fit stays linear.
10. **FZ2b runs at spray 50 %.** At spray 0 the 10 ms splice region is never re-read, so any continuation sounds identical (the control did not discriminate).
11. **Deferred head-stop (P38 deviation; your review at verify).**
    - The rising edge latches W0 as the head ⌈5 ms⌉ cells ahead: 9 cells at 48 k, 8 at 44.1 k, 31 at 192 k. The R25 scan runs against that W0, and the ring keeps writing until the stop tick.
    - Reason: R25's reflect is value-continuous, but it reverses the read slope, a 2·s′ kink. On a sine at rate 1 / position 0, every crosser hits the head one sample after the edge, before its fade acts: **κ 23.85×** (sticking measured 11.78×). With the deferral, crossers fade out before the stop: **κ 1.19×**.
    - Spawns inside the window anchor to the future W0 with a head-lead clamp, so they never read ahead of the moving head.
    - Cost: about 5 ms of freeze latency, tick-aligned, so it stays block-size invariant (N7 PASS).
    - The `freezeNoDefer` hook keeps the planned form as the FZ2an / FZ6 control.
12. **Slope-matched release continuation (R26 deviation; your review at verify).** `held = even + β·(odd − even)`:
    - even = the R26 mirror `x[W0 − 1 − k]` (DC-free);
    - odd = the point reflection `2·x[W0 − 1] − x[W0 − 2 − k]` (slope-continuous);
    - β is a raised cosine 1 → 0 over 2 ms (`kFreezeSlopeS`).
    - Measured: the plain mirror 6.98×, the old splice 112.7×, this form **1.15×**.
13. **FZ6:** with the deferral there are 0 reflections by design. The no-defer run at 0 st proves the reflect safety net (1920 reflections, 0 violations). At +12 st, crossers sit about (r − 1)·len from the head and could not reach it inside 5 ms anyway, contrary to the plan's expectation.
14. **FZ7:** the ring hash is taken harness-side at hold start and end, not at every tick (41 s × 2 channels per tick is infeasible). The chain snapshot is taken per tick.
15. **FZ8 tolerance is max(0.2 dB/s, 2 %),** after the plan's 4 s lengthening still read 0.55–0.69 dB/s apart at about −41 dB/s. The ±0.2 dB/s was sized for slopes of a few dB/s; the difference is −0.03 dB/gen.
16. **fb HP / LP coefficients are held per cell:** `tan` at ticks from 20 ms log-domain ramps, not lerped inside the cell. Block-size invariant (N7).
17. **Disabled streams leave the tap** through a 10 ms `fbEnRamp` (no step from their tails). Muted / un-soloed ones go through the audibility ramp.
18. **Z3b runs at 48 k only** (cost). **Z4** injects at the last sample of a cell; the reference resets after that sample, so the renders are identical.
19. **`Grain::frozenSpawn`** is added as telemetry (the FZ6 diag); DSP never reads it.
20. **RV4 "tank running"** is read by a test-only counter of pieces whose wet the reverb changed. The module exposes no `running` accessor, and only the bind overload may change.
21. **N7's "head reflections > 0" non-vacuity** is replaced by forced releases > 0. With the deferral, reflections are 0 by design.

## Carried to 2.5 / 2.6

- The matrix writes the reverb atomics (2.5); 2.4 writes base values only.
- The full PERF-02 budget gate (2.6, C4 18.57 % today). `getTailLengthSeconds` with reverb + feedback (2.6).
- R29 ripple table: only if listening item 2 says tails are too short.
- D12 `1 + 3·sat` and R22: decided by listening items 2 and 5.
- **Deviations 11 / 12** (deferred stop, slope-matched continuation): confirm at the listening checkpoint, item 1.
- W2o rule unchanged: widen seeds, never the threshold.

## Requirements (verify decides; REQUIREMENTS.md untouched)

- Expected complete: FUNC-02, FUNC-05, DSP-06, DSP-07.
- Advanced: QUAL-01 (freeze), QUAL-02 (max feedback), PERF-01, PERF-03.
- PERF-02: C4 measured.
