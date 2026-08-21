# Stage 2: DSP - Execution Plan

**Date:** 2026-08-20
**Contract:** `research/ARCHITECTURE.md` (immutable) — this plan sequences it, never re-decides it.
**Inputs:** CONTEXT.md (decisions), RESEARCH.md (API verification + house patterns), ROADMAP.md (phase gates)

## ✅ EXECUTION GATE — SATISFIED (2026-08-20)

Stage 1 complete and VERIFIED in worktree `VST-development-emulator` (`feat/o-emulator-impl`).
Mockup was skipped (user decision); parameter-spec.md promoted directly from draft — BINDING.
Gate 1→2 PASSED (build + pluginval strictness 10 VST3/AU).

`⟨STAGE-1⟩` placeholders resolved:
- Plugin target: **`OEmulator`** (PLUGIN_CODE `OEmu`); harness target: **`O-Emulator-render-test`** — already scaffolded in Stage 1 (O-Bitrot template, P0 contract / P1 passthrough / P2 ragged invariance; baseline digest `fnv1a64=28e7675cdbec475c`). Task 8 EXTENDS this harness; its CMakeLists exists.
- APVTS param IDs (BINDING, per parameter-spec.md): `console` (choice, 5 consoles, SNES default), `crush`, `age`, `reverb`, `mix` (each 0–100 %, linear; defaults 50/20/0/100).
- `OUARICON_BUILD_TESTS` option block already in plugin CMakeLists (Stage 1).

## Goal

Implement the complete DSP engine per ARCHITECTURE.md: 5 genuine codec round-trips (BRR, SPU-ADPCM, DPCM, GB 4-bit, Genesis 8-bit+ladder), console-domain resampling with authentic interpolation, SPU register-model reverb at 22.05 kHz, per-console output stages, age model, crush macro, latency-compensated mix, click-safe console crossfade — all block-size invariant by construction and gated per phase by a render harness.

## Plan-Level Decisions (resolving RESEARCH.md §6 open items)

1. **Digest-anchor timing (L110):** Per-phase FNV-1a anchors, recorded at each phase's commit, with the re-anchor discipline (`pattern_reanchor_cross_version_digest_probe`): when a later phase legitimately changes output, retire the old anchor as a named constant and assert `digest != retired` (moved), record the new one. Stochastic stages added later (age bed) consume their RNG streams unconditionally from the moment they are introduced.
2. **Latency constant:** computed once in `prepareToPlay` as `aaGroupDelayBudget(16 host samples, fixed) + kFeederChunk(32) + ceil(28 · hostRate / 22050) + gaussianHistory(3 console-domain samples → host)` → single `int totalLatencySamples`; `jassert(≤ kMaxWetLatencySamples)`; `setLatencySamples(N)` then, **after** `dryWetMixer.prepare()`, `setWetLatency((float) N)`. `static constexpr int kMaxWetLatencySamples = 1024;` (covers 192 kHz worst case ≈ 4× the 48 kHz figure with margin). DryWetMixer **must** be constructed `{ kMaxWetLatencySamples }` (RESEARCH §1.2 ctor gotcha).
3. **Butterworth AA:** named table with derivation comment — `Q_i = 1/(2·cos((2i+1)π/16))` → `{0.50980f, 0.60134f, 0.89998f, 2.56292f}`; 4 cascaded `IIR::Filter` per channel fed by `ArrayCoefficients<float>::makeLowPass(hostRate, cutoff, Q_i)`. Crush AA-open: precompute coefficient sets at fixed cutoff breakpoints in `prepareToPlay`; step between sets at control-chunk boundaries (never a Ptr factory on the audio thread).
4. **Crush integer steps:** continuous knob; internal stepping (NES rate index, GB level count, shift floor) quantized at control-chunk boundaries with 5 ms equal-gain micro-fades on step changes. Zipper probes measure the **control trajectory**, not raw sample deltas (L114).

## File Layout (all net-new unless marked Stage-1)

```
plugins/O-Emulator/
  CMakeLists.txt                      (Stage-1; Phase 2.1 adds OUARICON_BUILD_TESTS option block)
  Source/PluginProcessor.h/.cpp       (Stage-1 shell; every phase touches)
  Source/dsp/
    FixedChunkFeeder.h                fixed-chunk FIFO walk (O-Octagon GainStage + O-Texture OverlapAdd models)
    ConsoleEngine.h/.cpp              shared skeleton + 5 configs (pipeline manager, per-console table)
    ConsoleResampler.h/.cpp           AA cascade + Interpolators::Lagrange down; Gaussian/ZOH up
    GaussianInterpolator.h            512-entry S-DSP table (from spec) + 4-tap kernel
    AdpcmEncoder.h                    shared closed-loop block-search skeleton (template: block len + coeff table)
    BrrCodec.h/.cpp                   SNES: 16-sample blocks, filters 0–3, shift 0–12
    SpuAdpcmCodec.h/.cpp              PS1: 28-sample blocks, 5 filter pairs
    DpcmCodec.h                       NES: 1-bit sigma-delta, 7-bit counter, 16-entry NTSC rate table
    WaveQuantizer.h                   GB: 4-bit mid-tread + per-level DAC error + ZOH
    GenesisDac.h                      8-bit mid-tread + Nuked-OPN2 ladder offset + rate-hold
    SpuReverb.h/.cpp                  psx-spx register model @ 22.05 kHz, Hall preset constants
    OutputStage.h                     FirstOrderTPTFilter LP + per-console clip; DC blocker (10 Hz HP)
    AgeModel.h                        noise bed + hum + drift random-walk; per-purpose RNG streams
    CrushCurve.h                      per-console macro mapping tables
    ConsoleCrossfader.h               equal-power 30 ms fade state machine
  tests/render-harness/
    CMakeLists.txt                    copy O-Tapestop harness CMake; target name ⟨STAGE-1⟩
    main.cpp                          O-Bitrot scaffold + probes (see Task 8)
```

`#if OUARICON_RENDER_HARNESS` accessors on the processor (FIFO fill, chunk phase, computed latency) per O-Tapestop `PluginProcessor.h:160-175`.

---

## Tasks

### Phase 2.1 — Engine Skeleton + SNES End-to-End (commit gate: harness green)

1. [ ] **FixedChunkFeeder** — input/output FIFO, 32-host-sample fixed chunk walk, `std::uint64_t` absolute counter reset only in `prepare()`, write-then-consume ordering inside the chunk (`pattern_grain_read_before_capture_write_blocksize`), FIFO owns its latency contribution (`static constexpr int getLatencySamples()`).
   - Files: `Source/dsp/FixedChunkFeeder.h`
   - Depends on: none
2. [ ] **ConsoleResampler (SNES config)** — 4× cascaded Butterworth AA biquads (Q table, decision #3) at 0.45·32000; `Interpolators::Lagrange` decimation using the **returned consumed count** to advance the FIFO (+1 cleared guard sample, RESEARCH §1.1); running-double upsample phase, never reset per block.
   - Files: `Source/dsp/ConsoleResampler.h/.cpp`
   - Depends on: Task 1
3. [ ] **GaussianInterpolator** — 512-entry S-DSP ROM table entered from published spec (hardware constants; GPL hygiene note in header), 4-tap kernel `g[255−i]·s[n−3] + g[511−i]·s[n−2] + g[256+i]·s[n−1] + g[i]·s[n]`, fractional phase accumulator in double.
   - Files: `Source/dsp/GaussianInterpolator.h`
   - Depends on: none
4. [ ] **BRR codec (closed-loop)** — `AdpcmEncoder` skeleton templated on block length + coefficient table; brute-force (shift 0–12 × filter 0–3) min-squared-error against **decoded** history; int32 math, saturate int16; predictor state across blocks.
   - Files: `Source/dsp/AdpcmEncoder.h`, `Source/dsp/BrrCodec.h/.cpp`
   - Depends on: none
5. [ ] **SNES output stage + DC blocker** — `FirstOrderTPTFilter` LP ~10 kHz at host rate + soft clip; 10 Hz HP DC blocker; `snapToZero()` on per-sample paths; `ScopedNoDenormals` in `processBlock`.
   - Files: `Source/dsp/OutputStage.h`
   - Depends on: none
6. [ ] **ConsoleEngine skeleton + SNES wiring** — strategy-object pipeline (AA → down → drive/clip → codec → upsample → output stage) in the fixed-chunk walk; `crush` wired to drive (+0..12 dB) and shift floor; per-console config table with only SNES active.
   - Files: `Source/dsp/ConsoleEngine.h/.cpp`, `Source/dsp/CrushCurve.h` (SNES row), `Source/PluginProcessor.cpp`
   - Depends on: Tasks 1–5
7. [ ] **Latency + mix** — decision #2 exactly: DryWetMixer constructed with `kMaxWetLatencySamples`, prepare-order per O-Bitrot `PluginProcessor.cpp:1263-1310`, linear rule, `mix` wired; non-finite **input** scrub at the `processBlock` boundary (NaN-poisons-Thiran, RESEARCH §1.2).
   - Files: `Source/PluginProcessor.h/.cpp`
   - Depends on: Task 6
8. [ ] **Render harness (Phase 2.1 probe set)** — CMake from O-Tapestop template (target ⟨STAGE-1⟩, compiles processor TU only, `JUCE_WEB_BROWSER=0`, version via `get_target_property`); scaffold from O-Bitrot `main.cpp:1-460` (+`pathologicalStereo`, `bandEnergy`, `makeProcAtRate`, `renderChecksum`, `toneAmplitude`); `Spectrum` with `flatness()` from O-simpleSampler. Probes: block-size digest sweep {64}/{512}/{4096}/{1,7,64,333,4096} vs fixed reference (O-Octagon `AN` shape); mix-0% bit-transparent minus latency with >0.05 s warm-up skip (Bitrot `B`); latency-reported probes (`A`/`Z`, xcorr budgeting ~12–15 samples IIR group delay, L120); determinism (`E`); pathological timeline incl. denormal 1.0e-40f case (Bitrot `U` + Octagon `AR`); SNES alias/Gaussian-rolloff spectral signature (flatness + centroid, ratios vs control render); crush min≠max liveness. All probes: `setBaseline()` first, `setValueNotifyingHost` only, position-hashed excitation, liveness clauses, no wall-clock in verdicts, fixed settle length. Record Phase 2.1 digest anchors (decision #1).
   - Files: `tests/render-harness/CMakeLists.txt`, `tests/render-harness/main.cpp`, plugin `CMakeLists.txt` option block, `#if OUARICON_RENDER_HARNESS` accessors in `Source/PluginProcessor.h`
   - Depends on: Task 7

**Phase 2.1 gate (ROADMAP):** plugin loads VST3+AU; digests identical 64/512/4096; mix-0% null; 32 kHz alias/rolloff signature; crush liveness; pathological probe bounded finite. → git commit.

### Phase 2.2 — PS1 + SPU Reverb (commit gate: harness green incl. re-runs)

9. [ ] **SPU-ADPCM codec** — instantiate `AdpcmEncoder` with 28-sample blocks + 5 coefficient pairs /64; PS1 engine config (22050 Hz domain, Gaussian upsample sharing the SNES table, mild LP + hard clip output stage).
   - Files: `Source/dsp/SpuAdpcmCodec.h/.cpp`, `ConsoleEngine.cpp` (PS1 row), `CrushCurve.h` (PS1 row)
   - Depends on: Task 8 (Phase 2.1 committed)
10. [ ] **SPU reverb** — psx-spx register model per ARCHITECTURE Algorithm Details: input LP → same/different-side reflections with vWALL → 4-comb early echo → 2 series APFs, circular buffer, fixed 22.05 kHz tick; Hall preset registers as named constants; linear-interpolated send/return resampling; float internals; `isfinite` guard resets **state only, keeps coefficients** (non-sticky).
    - Files: `Source/dsp/SpuReverb.h/.cpp`
    - Depends on: none (parallel with Task 9)
11. [ ] **Send/return wiring + Phase 2.2 probes** — post-codec send tap, `reverb` param wired, return summed pre-output-stage (both dulled by Age later); reverb-return alignment ring so the 22.05 kHz round-trip joins the direct path time-aligned **before** summing (L119). Probes: PS1≠SNES spectral distinctness; impulse-response tail structure (comb/APF spacing, short murky decay); 60 s noise @ reverb 100% + crush 100% stability (no growth/NaN); block-size invariance re-run with reverb engaged; re-anchor digests (moved-assert on retired anchors).
    - Files: `ConsoleEngine.cpp`, `PluginProcessor.cpp`, `tests/render-harness/main.cpp`
    - Depends on: Tasks 9, 10

**Phase 2.2 gate:** all ROADMAP 2.2 criteria. → git commit.

### Phase 2.3 — NES / GB / Genesis + Console Switching (commit gate: harness green)

12. [ ] **Three quantizer codecs** — DPCM sigma-delta state machine (±2 steps, clamp 0–126, NTSC 16-entry rate table walked by crush via hold counter); GB 4-bit mid-tread + fixed ±0.5 LSB per-level error table + ZOH; Genesis 8-bit mid-tread + ladder offset (`q >= 0 ? q+1 : q`) + crush rate-hold.
    - Files: `Source/dsp/DpcmCodec.h`, `Source/dsp/WaveQuantizer.h`, `Source/dsp/GenesisDac.h`, `CrushCurve.h` (3 rows)
    - Depends on: Task 11 (Phase 2.2 committed)
13. [ ] **Engine configs for NES/GB/Genesis** — ZOH upsampling, per-console output-stage corners/clip styles (NES RC ~14 kHz + DC-shifted DAC, GB ~8 kHz + crunchy clip, Genesis ladder + ~12 kHz); NES unipolar mapping with structural DC removal verified at the mixer boundary.
    - Files: `ConsoleEngine.cpp`, `OutputStage.h`
    - Depends on: Task 12
14. [ ] **ConsoleCrossfader** — 30 ms equal-power fade (`pattern_hann_pair_is_equal_gain_not_equal_power`); old engine renders through the fade, new engine starts from reset; only two engines concurrent; latency constant across modes (already worst-case by construction); audio-thread state machine, console choice read once per block.
    - Files: `Source/dsp/ConsoleCrossfader.h`, `PluginProcessor.cpp`
    - Depends on: Task 13
15. [ ] **Phase 2.3 probes** — 5-mode spectral distinctness matrix; per-console output-corner bandEnergy ratios; mid-render console-switch timeline via O-Tapestop `Event`/`renderTimeline` (no processBlock spans an event): no clicks (step-size bound), fade-region RMS dip measurement, no latency renegotiation; NES DC fully blocked (dry/wet sum mean ≈ 0); reverb live in all 5 modes; invariance re-run per mode; re-anchor digests.
    - Files: `tests/render-harness/main.cpp`
    - Depends on: Task 14

**Phase 2.3 gate:** all ROADMAP 2.3 criteria. → git commit.

### Phase 2.4 — Age Model + Crush Polish (commit gate: harness green + full re-run)

16. [ ] **Age bed** — host-rate white → one-pole LP 6 kHz noise (level −78→−48 dB, ramp-in from age>5%, normalize only the white-fed stage — `pattern_noise_bed_level_is_rate_dependent`); 60/120/180 Hz phase-accumulator hum (−80→−54 dB); injected post-output-stage, pre-mixer (wet path only). One RNG stream per purpose, consumed unconditionally every sample from introduction (decision #1, L116/L110).
    - Files: `Source/dsp/AgeModel.h`, `ConsoleEngine.cpp`
    - Depends on: Task 15 (Phase 2.3 committed)
17. [ ] **Dulling + drift** — output LP corner × (1.0→0.45) over age, coefficient re-derive gated on the controlling value with no stale-enabled-flag leak (`pattern_conditional_coeff_update_leaks_enabled_flag`); bounded ±15-cent random-walk on resample ratio, control-rate targets integrated **per-sample** on the audio thread (offline-bounce parity); latency NOT re-reported on drift wobble.
    - Files: `AgeModel.h`, `ConsoleResampler.cpp`, `OutputStage.h`
    - Depends on: Task 16
18. [ ] **Crush curve tuning + micro-fades** — per-console curves finalized (drive, shift floor, NES rate walk, GB 16→8→4 levels, Genesis rate, AA-open ≥80% via precomputed coefficient sets); 5 ms micro-fades on every integer step (decision #4).
    - Files: `CrushCurve.h`, `ConsoleResampler.cpp`, `ConsoleEngine.cpp`
    - Depends on: Task 16
19. [ ] **Phase 2.4 probes + smoothing audit** — age min≠max in all 5 modes, continuous scaling; noise-bed level invariance at 44.1/48/96/192 kHz via `makeProcAtRate`; drift bounded (±15 cents, no resampler instability); offline digest == real-time digest; all 4 macro knobs min≠max liveness in all 5 modes; control-trajectory zipper probes on every knob sweep (L114); full probe-suite re-run; final re-anchor.
    - Files: `tests/render-harness/main.cpp`
    - Depends on: Tasks 17, 18
20. [ ] **PERF-01 audit + stage wrap** — code audit: no allocation/locks/file-I/O/Ptr-factories/`setLatencySamples` on the audio thread; CPU-ratio probe (Bitrot `P1` shape, ratio ≤ 0.15 as harness proxy for <10% target); build-and-install, auval smoke. → git commit; then formal `/plugin-verify O-Emulator 2-dsp` (single goal-backward pass per CONTEXT.md cadence).
    - Files: audit only + `tests/render-harness/main.cpp`
    - Depends on: Task 19

---

## Success Criteria (stage-level, from REQUIREMENTS traceability)

- [ ] FUNC-01: all 5 console modes audibly/measurably distinct on identical material (spectral matrix probe)
- [ ] FUNC-02: mix-0% bit-transparent minus reported latency; every macro passes min≠max liveness in all 5 modes
- [ ] FUNC-03: reverb send functional in all 5 modes
- [ ] FUNC-04: mid-playback console switch — no clicks, no stuck audio, no latency renegotiation
- [ ] DSP-01..05: codec round-trips, authentic resampling signatures, per-console output corners, SPU reverb character, age model — each with a passing named probe
- [ ] PERF-01: RT-safety audit clean; CPU ratio ≤ 0.15
- [ ] PERF-02: render digests identical at block sizes 64/512/4096 (+ ragged set) in every phase's final state
- [ ] QUAL-01: pathological timeline (silence, DC, full-scale noise, denormals, NaN input) → bounded finite output, non-sticky recovery
- [ ] Four git commits (one per phase), each with its harness gate green at commit time
- [ ] `/plugin-verify O-Emulator 2-dsp` passes (goal-backward)

## Risk Register (from ARCHITECTURE, with plan hooks)

| Risk | Trigger | Fallback (documented, pre-authorized) |
|------|---------|--------------------------------------|
| SPU reverb register-model port fails validation | Task 11 impulse/stability probes | Tuned Schroeder (4 comb + 2 APF) @ 22.05 kHz — same file, same interface |
| Closed-loop encoder artifacts wrong | Task 8/9 spectral probes | Open-loop encoders (skeleton flag), close loop in point release |
| Arbitrary-ratio resampler invariance bugs | Task 8 digest sweep | Integer-divisor console rates (e.g. SNES @ 24 kHz on 48 kHz host) |
| Drift destabilizes resampler | Task 19 drift probe | Ship age without drift (noise/hum/dulling only) |
