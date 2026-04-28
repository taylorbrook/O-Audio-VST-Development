---
plugin: O-MicrotonalSampler
stage: 2-dsp
phase: execute
status: phase_2_2_gate_2_pass
last_updated: 2026-04-27
---

# Resume Point

## Current State: Phase 2.2 Gate 2 PASS — Ready for Phase 2.3

Background sample loader + filename parser + SR conversion landed and validated. `auval` clean, `pluginval --strictness 5` clean. Voice DSP (Phase 2.1) untouched. Commit: `cacffda`.

Stage 2 progress: **2 of 5 sub-stages complete** (2.1 ✓, 2.2 ✓, 2.3 next, 2.4, 2.5).

## Completed So Far

**Ideation:** ✓ Complete
**Stage 1 (Foundation):** ✓ Verified — silent shell builds + AU/VST3/Standalone validate
**Stage 2 Discuss:** ✓ Complete (CONTEXT.md, 2026-04-27)
**Stage 2 Research:** ✓ Complete (RESEARCH.md, 2026-04-27)
**Stage 2 Plan:** ✓ Complete (PLAN.md, 2026-04-27)
**Stage 2 Phase 2.1:** ✓ Gate 1 PASS — cubic-Hermite varispeed voice + ADSR + NE (commit `bb0e7f7`)
**Stage 2 Phase 2.2:** ✓ Gate 2 PASS — background loader + filename parser + SR conversion (commit `cacffda`)

## Stage 2 Locked Decisions (D2-1..D2-12)

- **D2-1 Interpolator:** Cubic-Hermite (4-pt), with conditional 1st-order tilt LPF only if Stage 2.1 sine-sweep test shows aliasing
- **D2-2 Voice-steal:** Override `juce::Synthesiser::findVoiceToSteal` — oldest-released → oldest-held
- **D2-3 Steal ramp:** 5 ms linear (240 samples @ 48 kHz)
- **D2-4 Loop auto-detect:** RMS scan (1024 window, latter 60%) + zero-crossing snap (±64) + 8-sample equal-power xfade; fallback one-shot
- **D2-5 ADSR:** `juce::ADSR` (linear segments)
- **D2-6 Sub-stage order:** 2.1 voice DSP → 2.2 loader → 2.3 vel crossfade → 2.4 voice-steal → 2.5 loop-detect
- **D2-7 Filename parser:** Tolerant per BRIEF.md (multi-convention, case-insensitive, silent-skip + log unparseable)
- **D2-8 Out-of-range notes:** Silence
- **D2-9 SR conversion:** `juce::LagrangeInterpolator` at load time (one-time, background thread)
- **D2-10 Mono → stereo:** Duplicate L/R at unity gain
- **D2-11 Smoothing:** `output_gain` + `velocity_crossfade` only
- **D2-12 NE granularity:** Once at `startNote()`

## Stage 2 Requirements in Scope (15)

FUNC-01..04, FUNC-07, DSP-01..05, DSP-07, DSP-08, PERF-01..04, COMPAT-02, QUAL-01

## Files Created (Stage 2)

- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/CONTEXT.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/RESEARCH.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PLAN.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.1-SUMMARY.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.2-SUMMARY.md`

## Source Files Touched (Phase 2.2)

Created: `Source/FilenameParser.{h,cpp}`
Modified: `Source/SampleLoader.{h,cpp}`, `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.{h,cpp}`, `CMakeLists.txt`

## Known Issue Flagged for Phase 2.3 Pickup

`MicrotonalSamplerVoice.cpp:138` uses plain `shared_ptr` deref+copy rather than `std::atomic_load(sampleMapSource)`. Practical risk near-zero on x86-64/ARM64 but TSan would flag it. Phase 2.3 modifies voice members anyway → fix can ride along.

## Next Steps

1. **Phase 2.3 Execute (next)** — `/plugin-execute O-MicrotonalSampler 2-dsp` (continues to Phase 2.3: velocity-layer crossfade, Tasks 16–20, Gate 3)
2. Remaining sub-stages: 2.3 → 2.4 → 2.5 → full Stage 2 verify
3. UI mockup (parallelizable any time before Stage 3) — `/ui-mockup O-MicrotonalSampler`
