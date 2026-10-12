---
plugin: O-simpleAmbisonics
stage: 0
status: complete
last_updated: 2026-10-11
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: create_mockup_then_invoke_foundation_shell_agent
next_stage: 1
ready_for_implementation: true
parameter_count: 12
mockup_finalized: false
---

# O-simpleAmbisonics Status

## Current Position

Stage: 0 of 4 (Ideation / Research & Planning) — complete
Status: ARCHITECTURE.md and ROADMAP.md documented (complexity 5.0, phased). Ready for the UI mockup (which finalises `parameter-spec.md`) and then Stage 1 (Foundation).
Progress: [##..................] 10%

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

## Next Steps

1. Create the UI mockup: `/start O-simpleAmbisonics` → option 3 (mockup finalisation produces `parameter-spec.md`; it must keep the 12-parameter set from ARCHITECTURE.md)
2. Stage 1 Foundation: `/implement O-simpleAmbisonics` (foundation-shell-agent) — effect shell, 12-param APVTS, mono/stereo-in → stereo-out buses, `setLatencySamples(64)` from day one, harness scaffold
3. Review `research/ARCHITECTURE.md` and `ROADMAP.md`

## Context to Preserve

**Key decisions (contract — see ARCHITECTURE.md Architecture Decisions §1–10):**
- Binaural = MagLS, SADIE II D1 (KU100), 256 taps @ 48 kHz, three per-order sets in a generated constexpr header `Source/dsp/BinauralFilterData.h` (no second BinaryData target); runtime fixed-partition UPOLS P=64 on `juce::dsp::FFT`; **latency 64 samples constant** in both modes at all sample rates
- Rotation = Ivanic–Ruedenberg recursion; `R = Rz(yaw)·Ry′(pitch)·Rx(roll)`; yaw + = scene CCW from above, pitch + = front rises, roll + = left rises
- Distance: `g = 1.25/(d+0.25)`, air LP `f_c = 20 kHz·min(1, 1/√d)` clamped to 0.45·fs, pre-encode
- Stereo decode: ±60° first-order cardioid pair on rotated W/Y/X, delayed 64; decoders always run; 20 ms crossfades everywhere
- Smoothing: wrap-aware one-pole (τ 20 ms) at block rate + per-sample lerp of coefficient vector / rotation matrix
- Meters: post-rotation `shGain = R·c_eff` (signed) + peaks via `TripleBuffer<MeterFrame>`; polar plot from `order` in JS
- Mono sum = 0.5·(L+R); presets never set `outputLevel`

**Open until later stages:**
- Mockup (Stage 3 input) — no mockup exists yet; `parameter-spec.md` to be produced at finalisation
- PLUGIN_CODE uniqueness check at Stage 1 (suggested `OSiA`)
- SADIE II SOFA download + `sofar` availability at Phase 2.3 (fallback: synthetic spherical-head generator)

**Sibling references:** O-simpleSubtractive (editor/CMake/harness template), O-simpleWavetable (latest WebView + gate conventions), O-simpleFM (viz Timer), O-GrainScatter (encoder, TripleBuffer, generator, fallback decoder).

## Files Created
- plugins/O-simpleAmbisonics/.planning/BRIEF.md (ideation)
- plugins/O-simpleAmbisonics/.planning/REQUIREMENTS.md (ideation)
- plugins/O-simpleAmbisonics/.planning/parameter-spec-draft.md (ideation)
- plugins/O-simpleAmbisonics/.planning/research/ARCHITECTURE.md (Stage 0)
- plugins/O-simpleAmbisonics/.planning/ROADMAP.md (Stage 0)
- plugins/O-simpleAmbisonics/.planning/stages/0-ideation/CONTEXT.md (Stage 0)
