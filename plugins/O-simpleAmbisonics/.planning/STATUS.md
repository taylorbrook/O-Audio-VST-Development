---
plugin: O-simpleAmbisonics
stage: 1
status: stage_1_complete
last_updated: 2026-10-12
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
workflow_mode: manual
current_phase: verify
next_phase: discuss
next_action: plugin-discuss
next_stage: 2
ready_for_implementation: true
parameter_count: 12
mockup_finalized: false
plugin_code: OSAm
---

# O-simpleAmbisonics Status

## Current Position

Stage: 1 of 4 (Foundation) — ✅ VERIFIED (2026-10-12)
Status: Stage 1 complete. Verify re-ran everything independently on the INSTALLED bundles (byte-identical to the build): harness P0–P5 ALL PASS (digest 6a1df555bc125ae3 reproduced), 0 warnings on a forced TU rebuild, auval SUCCEEDED (caps [1,2] [2,2]), pluginval s10 SUCCESS VST3 + AU, pedalboard V1–V3 (12-param contract, pass-through with reported == measured latency, 12-value state round-trip) ALL PASS. COMPAT-01 + FUNC-07 complete. Next: Stage 2 / discuss (Phase 2.1).
Progress: [#######.............] 35%

## Phase Progress

### Stage 1: Foundation
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-10-11 | |
| research | ✓ | 2026-10-11 | |
| plan | ✓ | 2026-10-12 | |
| execute | ✓ | 2026-10-12 | |
| verify | ✓ | 2026-10-12 | |

## Completed So Far

**Ideation:** ✓ Complete
- Core concept, architecture, parameters, UI vision, use cases, 27 requirements with acceptance criteria.

**Stage 0:** ✓ Complete — Research & Planning (2026-10-11)
- Plugin type: audio effect, stereo/mono in → stereo out, 12 parameters, WebView UI
- Prior art reused: O-GrainScatter `SpatialEncoder.h` (encoder), `TripleBuffer.h`, header-generation pipeline; both in-house ambisonics deep-dives
- JUCE 8.0.15 classes verified in local source: `dsp::FFT`, `Interpolators::WindowedSinc`, `dsp::FirstOrderTPTFilter`, `SmoothedValue`; `dsp::Convolution` read and rejected for the 16→2 topology
- Eight research questions resolved (see `stages/0-ideation/CONTEXT.md`): MagLS from SADIE II KU100 (Apache-2.0), fixed-partition UPOLS P=64 → constant 64-sample latency, Ivanic–Ruedenberg rotation, soft-knee distance + air LP, always-running decoders with 20 ms crossfades, FIFO block-size independence, wrap-aware smoothing, ±60° virtual-cardioid stereo pair, 12-parameter v1.0 set (spread / weighting / solo deferred to v1.1)
- Complexity score: 5.0 (capped; raw 10.0) → staged implementation (DSP 3 phases, GUI 3 phases)
- ROADMAP covers every must-priority requirement by ID with a named phase and gate

**Stage 1 / discuss:** ✓ Complete (2026-10-11)
- `stages/1-foundation/CONTEXT.md` — foundation scope: effect CMake (VST3 + AU + Standalone, WebView2 flags, `juce_dsp`), mono/stereo-in → stereo-out buses, 12-param APVTS, pass-through `processBlock` (input conditioner → 64-sample delay → seeded output gain), `setLatencySamples(64)`, placeholder editor, `Source/dsp/SpatialEngine.h` seam, `tests/render-harness/` scaffold
- Decisions: `PLUGIN_CODE OSAm` (Stage 0's `OSiA` is taken by O-simpleAdditive); proceed without `parameter-spec.md` (sibling precedent — ARCHITECTURE.md §Parameter Mapping is the authoritative 12-param contract; the mockup may not add parameters)

**Stage 1 / research:** ✓ Complete (2026-10-11)
- `stages/1-foundation/RESEARCH.md` — templates pinned: O-simpleSubtractive `CMakeLists.txt` + Stage-1 APVTS skeleton (commit 62d4c0e8), O-Emulator Stage-1 effect shell + `tests/render-harness` scaffold with derived version code (commit 8ec32da9), O-Orbit mono/stereo-in → stereo-out bus check
- Pass-through chain designed from JUCE 8.0.15 source: `setLatencySamples(64)` once in `prepareToPlay`; recommended `Source/dsp/FixedDelay.h` (`FixedDelay<kPartition>`, std::array, zero alloc, survives into Stage 2's `DecoderMixer`) over `dsp::DelayLine`; `LinearSmoothedValue` output gain seeded via `setCurrentAndTargetValue(decibelsToGain(dB, −60))`; slice loop for oversized host blocks; finite scrub
- 12-parameter construction table verified (`setSkewForCentre(2 m)` / `(0.2 Hz)` — skews transmit to the WebView frontend, lambda ranges do not); JUCE headers scanned — no `juce::` name collisions with any of the 12 IDs
- Editor recommendation: O-Emulator shape (inline `GenericAudioProcessorEditor`, harness at `JUCE_WEB_BROWSER=0`); Stage-1 harness probes P0–P5 specified (contract, pass-through bit-equal + no fade-in, Left-only + mono-in, ragged blocks, oversized block, impulse latency = 64, −60 dB silence)
- Verify tooling: pluginval strictness 10 (CI parity), targeted `auval -v aufx OSAm OuDv`

**Stage 1 / plan:** ✓ Complete (2026-10-12)
- `stages/1-foundation/PLAN.md` — T1 CMake (effect, `OSAm`, no BinaryData) · T2 `Source/dsp/{SpatialEngine,FixedDelay}.h` seam with `kPartition = 64` · T3 processor (buses, 12-param table, seeded pass-through with slice loop, XML state + `pluginVersion`) · T4 inline `GenericAudioProcessorEditor` (no editor TU) · T5 `tests/render-harness` scaffold from O-Emulator 8ec32da9 with probes P0–P5 + FNV digest · T6 build / harness / build-and-install / `auval -v aufx OSAm OuDv` / pluginval s10 ×2 · T7 SUMMARY + status
- Success criteria mirror CONTEXT.md; gotchas checklist from RESEARCH §8 embedded for the executor

**Stage 1 / execute:** ✓ Complete (2026-10-12)
- `stages/1-foundation/SUMMARY.md` — files: `CMakeLists.txt` (effect, `OSAm`, VST3+AU+Standalone, WebView2 flags, no BinaryData), `Source/PluginProcessor.{h,cpp}` (12-param APVTS, mono/stereo-in → stereo-out, pass-through conditioner → `FixedDelay<64>` → seeded gain, slice loop, finite scrub, Generic editor inline, XML state + `pluginVersion`), `Source/dsp/{SpatialEngine,FixedDelay}.h` seam (`kPartition = 64`, no-op engine), `tests/render-harness/{CMakeLists.txt,main.cpp}` (derived version, probes P0–P5 + digest)
- Gate 0→1 bypassed with `--force` (no source tree yet — standing pattern; logged)
- Harness: P0 (45 checks) / P1 / P1b / P2 / P3 / P4 (impulse 100 → 164) / P5 ALL PASS; digest `6a1df555bc125ae3`
- Build 0 warnings; `build-and-install.sh` OK; `auval -v aufx OSAm OuDv` SUCCEEDED; pluginval strictness 10 ×2 SUCCESS for VST3 and AU
- **Deviation:** the five degree parameters use `NormalisableRange` interval **0** (continuous) instead of 0.01 — JUCE snaps every host value to `start + interval·floor(…)`, and `−180 + 0.01f·18000 = −4.02e-06`, so the 0° default was never exactly 0 (P0 caught it). Siblings carry the same latent offset unobserved.
- Build-time corrections: namespace alias `osa_dsp` (bare `dsp::` ambiguous with `juce::dsp` under JuceHeader's `using namespace juce`); input legs via `getChannelLayoutOfBus(true, 0)`

**Stage 1 / verify:** ✓ Complete (2026-10-12)
- `stages/1-foundation/VERIFICATION.md` — ✅ VERIFIED, every SUMMARY claim re-run independently on the installed bundles; COMPAT-01 and FUNC-07 → complete in REQUIREMENTS.md
- Independent measurer: pedalboard on the installed VST3 — 12 params with contract ranges/defaults/choices; stereo noise → both outputs == 0.5·(L+R) 8192/8192 exact with residual lag 0 after host latency compensation (reported 64 == actual); impulse amplitude exactly 1.0; 12-value `raw_state` round-trip restores every decoded value
- Test-harness lesson (not a defect): a live VST3 instance's `raw_value` is the wrapper's cached un-snapped host float — round-trip gates compare DECODED values (memory written)
- Human DAW checks listed as non-blocking; COMPAT-02 (Stage 4) repeats them in Logic + Live

## Next Steps

1. Stage 2 / discuss: `/plugin-discuss O-simpleAmbisonics 2-dsp` — Phase 2.1 (source → encoder → stereo decode; ROADMAP §Phase 2.1; SUMMARY §Stage-2 handoff)
2. Non-blocking human checks from `stages/1-foundation/VERIFICATION.md` (Logic stereo + mono track, Generic editor 12 rows, project save/reopen) — repeated formally under COMPAT-02 at Stage 4
3. UI mockup (`/start O-simpleAmbisonics` → option 3) before Stage 3; it finalises `parameter-spec.md` and must keep the 12-parameter set

## Context to Preserve

**Key decisions (contract — see ARCHITECTURE.md Architecture Decisions §1–10):**
- Binaural = MagLS, SADIE II D1 (KU100), 256 taps @ 48 kHz, three per-order sets in a generated constexpr header `Source/dsp/BinauralFilterData.h` (no second BinaryData target); runtime fixed-partition UPOLS P=64 on `juce::dsp::FFT`; **latency 64 samples constant** in both modes at all sample rates
- Rotation = Ivanic–Ruedenberg recursion; `R = Rz(yaw)·Ry′(pitch)·Rx(roll)`; yaw + = scene CCW from above, pitch + = front rises, roll + = left rises
- Distance: `g = 1.25/(d+0.25)`, air LP `f_c = 20 kHz·min(1, 1/√d)` clamped to 0.45·fs, pre-encode
- Stereo decode: ±60° first-order cardioid pair on rotated W/Y/X, delayed 64; decoders always run; 20 ms crossfades everywhere
- Smoothing: wrap-aware one-pole (τ 20 ms) at block rate + per-sample lerp of coefficient vector / rotation matrix
- Meters: post-rotation `shGain = R·c_eff` (signed) + peaks via `TripleBuffer<MeterFrame>`; polar plot from `order` in JS
- Mono sum = 0.5·(L+R); presets never set `outputLevel`
- `PLUGIN_CODE OSAm`

**Open until later stages:**
- Mockup (Stage 3 input) — no mockup exists yet; `parameter-spec.md` to be produced at finalisation
- SADIE II SOFA download + `sofar` availability at Phase 2.3 (fallback: synthetic spherical-head generator)

**Sibling references:** O-simpleSubtractive (editor/CMake/harness template), O-simpleWavetable (latest WebView + gate conventions), O-simpleFM (viz Timer), O-GrainScatter (encoder, TripleBuffer, generator, fallback decoder); a suite stereo effect (O-Freeze / O-Gain) for effect bus boilerplate.

## Files Created
- plugins/O-simpleAmbisonics/.planning/BRIEF.md (ideation)
- plugins/O-simpleAmbisonics/.planning/REQUIREMENTS.md (ideation)
- plugins/O-simpleAmbisonics/.planning/parameter-spec-draft.md (ideation)
- plugins/O-simpleAmbisonics/.planning/research/ARCHITECTURE.md (Stage 0)
- plugins/O-simpleAmbisonics/.planning/ROADMAP.md (Stage 0)
- plugins/O-simpleAmbisonics/.planning/stages/0-ideation/CONTEXT.md (Stage 0)
- plugins/O-simpleAmbisonics/.planning/stages/1-foundation/CONTEXT.md (Stage 1 discuss)
- plugins/O-simpleAmbisonics/.planning/stages/1-foundation/RESEARCH.md (Stage 1 research)
- plugins/O-simpleAmbisonics/.planning/stages/1-foundation/PLAN.md (Stage 1 plan)
- plugins/O-simpleAmbisonics/.planning/stages/1-foundation/SUMMARY.md (Stage 1 execute)
- plugins/O-simpleAmbisonics/.planning/stages/1-foundation/VERIFICATION.md (Stage 1 verify)
- plugins/O-simpleAmbisonics/CMakeLists.txt (Stage 1 execute)
- plugins/O-simpleAmbisonics/Source/PluginProcessor.h, PluginProcessor.cpp (Stage 1 execute)
- plugins/O-simpleAmbisonics/Source/dsp/SpatialEngine.h, FixedDelay.h (Stage 1 execute)
- plugins/O-simpleAmbisonics/Source/ui/.gitkeep (Stage 1 execute)
- plugins/O-simpleAmbisonics/tests/render-harness/CMakeLists.txt, main.cpp (Stage 1 execute)
