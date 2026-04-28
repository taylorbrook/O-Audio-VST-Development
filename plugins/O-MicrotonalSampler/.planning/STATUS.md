---
plugin: O-MicrotonalSampler
stage: 2-dsp
phase: execute
status: phase_2_3_code_complete_awaiting_build
last_updated: 2026-04-27
---

# Resume Point

## Current State: Phase 2.3 — Code Complete, Awaiting User Build + Verify + Commit

Equal-power velocity-layer crossfade implemented in voice DSP (Tasks 16–19).
`std::atomic_load` carry-over fix from Phase 2.2 landed alongside.
Test fixture generator (`tests/fixtures/4-layer/generate.py`) committed.

**Gate 3 closure pending user execution** of:
1. Triple-target build (`ninja O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU O-MicrotonalSampler_Standalone`)
2. Cache-clear + fresh install per CLAUDE.md
3. `pluginval --strictness 5` (must be clean — zero allocations in processBlock)
4. (Optional, recommended) Run `tests/fixtures/4-layer/generate.py` and DAW-load to validate subjective audible crossfade
5. Atomic commit with message: `feat(O-MicrotonalSampler): equal-power velocity-layer crossfade - Phase 2.3 Gate 3 PASS`

Recipe lives in `.planning/stages/2-dsp/PHASE-2.3-SUMMARY.md` "Recipe for User to Close Gate 3" section.

Stage 2 progress: **2 of 5 sub-stages fully closed** (2.1 ✓, 2.2 ✓, 2.3 code-complete, 2.4 next, 2.5).

## Completed So Far

**Ideation:** ✓ Complete
**Stage 1 (Foundation):** ✓ Verified — silent shell builds + AU/VST3/Standalone validate
**Stage 2 Discuss:** ✓ Complete (CONTEXT.md, 2026-04-27)
**Stage 2 Research:** ✓ Complete (RESEARCH.md, 2026-04-27)
**Stage 2 Plan:** ✓ Complete (PLAN.md, 2026-04-27)
**Stage 2 Phase 2.1:** ✓ Gate 1 PASS — cubic-Hermite varispeed voice + ADSR + NE (commit `bb0e7f7`)
**Stage 2 Phase 2.2:** ✓ Gate 2 PASS — background loader + filename parser + SR conversion (commit `cacffda`)
**Stage 2 Phase 2.3:** 🟡 Code-complete — equal-power velocity-layer crossfade (no commit yet)

## Stage 2 Locked Decisions (D2-1..D2-12)

- **D2-1 Interpolator:** Cubic-Hermite (4-pt), with conditional 1st-order tilt LPF only if Stage 2.1 sine-sweep test shows aliasing
- **D2-2 Voice-steal:** Override `juce::Synthesiser::findVoiceToSteal` — oldest-released → oldest-held (R1: JUCE default already matches; no override needed)
- **D2-3 Steal ramp:** 5 ms linear (240 samples @ 48 kHz)
- **D2-4 Loop auto-detect:** RMS scan (1024 window, latter 60%) + zero-crossing snap (±64) + 8-sample equal-power xfade; fallback one-shot
- **D2-5 ADSR:** `juce::ADSR` (linear segments)
- **D2-6 Sub-stage order:** 2.1 voice DSP → 2.2 loader → 2.3 vel crossfade → 2.4 voice-steal → 2.5 loop-detect
- **D2-7 Filename parser:** Tolerant per BRIEF.md (multi-convention, case-insensitive, silent-skip + log unparseable)
- **D2-8 Out-of-range notes:** Silence
- **D2-9 SR conversion:** `juce::LagrangeInterpolator` at load time (one-time, background thread)
- **D2-10 Mono → stereo:** Duplicate L/R at unity gain
- **D2-11 Smoothing:** `output_gain` + `velocity_crossfade` only (D2-11; vel-xfade actually consumed once at startNote — no SmoothedValue needed for it; output_gain has the smoother)
- **D2-12 NE granularity:** Once at `startNote()`

## Stage 2 Requirements in Scope (15)

FUNC-01..04, FUNC-07, DSP-01..05, DSP-07, DSP-08, PERF-01..04, COMPAT-02, QUAL-01

## Files Created (Stage 2)

- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/CONTEXT.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/RESEARCH.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PLAN.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.1-SUMMARY.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.2-SUMMARY.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.3-SUMMARY.md` (this phase)
- `plugins/O-MicrotonalSampler/tests/fixtures/4-layer/generate.py` (this phase)

## Source Files Touched (Phase 2.3)

Modified: `Source/MicrotonalSamplerVoice.h`, `Source/MicrotonalSamplerVoice.cpp`
Created: `tests/fixtures/4-layer/generate.py`

## Resolved Carry-Over from Phase 2.2

`MicrotonalSamplerVoice.cpp:138` `std::atomic_load` flag — **resolved**. Voice
now uses `std::atomic_load(sampleMapSource)` under the same
`__cpp_lib_atomic_shared_ptr` guard the producer side already uses. TSan-clean.

## Next Steps

1. **User: Close Phase 2.3 Gate 3** — follow recipe in `PHASE-2.3-SUMMARY.md`. Build + pluginval + commit.
2. **Phase 2.4 Execute** — `/plugin-execute O-MicrotonalSampler 2-dsp` (continues to Phase 2.4: voice-steal 5-ms ramp, Tasks 21–28, Gate 4)
3. Remaining sub-stages: 2.4 → 2.5 → full Stage 2 verify
4. UI mockup (parallelizable any time before Stage 3) — `/ui-mockup O-MicrotonalSampler`
