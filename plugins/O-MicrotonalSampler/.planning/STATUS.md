---
plugin: O-MicrotonalSampler
stage: 2-dsp
phase: execute
status: phase_2_4_gate_4_pass
last_updated: 2026-04-27
---

# Resume Point

## Current State: Phase 2.4 Gate 4 PASS — Ready for Phase 2.5

Voice-steal tail-ramp (5 ms linear-down) implemented in voice DSP (Tasks 21–25).
Per-voice scratch buffers sized in `prepareToPlay`; `renderTailRamp` helper added;
`startNote` self-detects active steal and captures OLD-note tail BEFORE state reset;
`renderNextBlock` mixes captured tail additively (BEFORE early-out so EC-1/EC-2
still play out cleanly). JUCE default `findVoiceToSteal` kept (R1, D2-2).
`synthesiser.setNoteStealingEnabled(true)` already in place at processor ctor (verified, Task 26).
`stopNote` graceful-release path verified unchanged (Task 27).

Triple build green (8/8 ninja targets). `pluginval --strictness 5 --validate-in-process
--skip-gui-tests` SUCCESS. `auval -v aumu OMtS OuDv` AU VALIDATION SUCCEEDED.
Awaiting atomic commit (recipe in PHASE-2.4-SUMMARY.md).

Subjective DAW checks (16-voice steal, ADSR release=0 click test, polyphony
reduction EC-9, continuity check) deferred to `/plugin-verify` time.

Stage 2 progress: **4 of 5 sub-stages complete** (2.1 ✓, 2.2 ✓, 2.3 ✓, 2.4 ✓, 2.5 next).

## Completed So Far

**Ideation:** ✓ Complete
**Stage 1 (Foundation):** ✓ Verified — silent shell builds + AU/VST3/Standalone validate
**Stage 2 Discuss:** ✓ Complete (CONTEXT.md, 2026-04-27)
**Stage 2 Research:** ✓ Complete (RESEARCH.md, 2026-04-27)
**Stage 2 Plan:** ✓ Complete (PLAN.md, 2026-04-27)
**Stage 2 Phase 2.1:** ✓ Gate 1 PASS — cubic-Hermite varispeed voice + ADSR + NE (commit `bb0e7f7`)
**Stage 2 Phase 2.2:** ✓ Gate 2 PASS — background loader + filename parser + SR conversion (commit `cacffda`)
**Stage 2 Phase 2.3:** ✓ Gate 3 PASS — equal-power velocity-layer crossfade (commit `11bd39c`)
**Stage 2 Phase 2.4:** ⏳ Code complete — voice-steal 5-ms tail ramp (Tasks 21–28); Gate 4 verification pending orchestrator

## Stage 2 Locked Decisions (D2-1..D2-12)

- **D2-1 Interpolator:** Cubic-Hermite (4-pt), with conditional 1st-order tilt LPF only if Stage 2.1 sine-sweep test shows aliasing
- **D2-2 Voice-steal:** Override `juce::Synthesiser::findVoiceToSteal` — oldest-released → oldest-held (R1: JUCE default already matches; no override needed — VERIFIED Phase 2.4)
- **D2-3 Steal ramp:** 5 ms linear (240 samples @ 48 kHz) — IMPLEMENTED Phase 2.4
- **D2-4 Loop auto-detect:** RMS scan (1024 window, latter 60%) + zero-crossing snap (±64) + 8-sample equal-power xfade; fallback one-shot
- **D2-5 ADSR:** `juce::ADSR` (linear segments)
- **D2-6 Sub-stage order:** 2.1 voice DSP → 2.2 loader → 2.3 vel crossfade → 2.4 voice-steal → 2.5 loop-detect
- **D2-7 Filename parser:** Tolerant per BRIEF.md (multi-convention, case-insensitive, silent-skip + log unparseable)
- **D2-8 Out-of-range notes:** Silence
- **D2-9 SR conversion:** `juce::LagrangeInterpolator` at load time (one-time, background thread)
- **D2-10 Mono → stereo:** Duplicate L/R at unity gain
- **D2-11 Smoothing:** `output_gain` + `velocity_crossfade` only (D2-11; vel-xfade actually consumed once at startNote — no SmoothedValue needed; tail ramp also computed inline, no SmoothedValue)
- **D2-12 NE granularity:** Once at `startNote()`

## Stage 2 Requirements in Scope (15)

FUNC-01..04, FUNC-07, DSP-01..05, DSP-07, DSP-08, PERF-01..04, COMPAT-02, QUAL-01

## Files Created (Stage 2)

- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/CONTEXT.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/RESEARCH.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PLAN.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.1-SUMMARY.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.2-SUMMARY.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.3-SUMMARY.md`
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.4-SUMMARY.md` (this phase)
- `plugins/O-MicrotonalSampler/tests/fixtures/4-layer/generate.py`

## Source Files Touched (Phase 2.4)

Modified: `Source/MicrotonalSamplerVoice.h`, `Source/MicrotonalSamplerVoice.cpp`
Verified-no-change: `Source/PluginProcessor.cpp` (Task 26), `Source/MicrotonalSamplerVoice.cpp:stopNote` (Task 27)

## Resolved Carry-Over from Phase 2.2

`MicrotonalSamplerVoice.cpp:138` `std::atomic_load` flag — **resolved in Phase 2.3**. Voice
uses `std::atomic_load(sampleMapSource)` under the same `__cpp_lib_atomic_shared_ptr`
guard the producer side already uses. TSan-clean. No carry-over to Phase 2.4.

## Next Steps

1. **Orchestrator runs Gate 4** — triple build + cache-clearing install + `pluginval --strictness 5` + `auval -v aumu OMtS OuDv` + atomic commit per the recipe in `PHASE-2.4-SUMMARY.md`.
2. **Phase 2.5 Execute (after Gate 4 closes)** — `/plugin-execute O-MicrotonalSampler 2-dsp` (continues to Phase 2.5: loop auto-detect + 8-sample boundary crossfade, Tasks 29–33, Gate 5).
3. **Full Stage 2 verify** (after Phase 2.5) — `/plugin-verify O-MicrotonalSampler 2-dsp`.
4. **UI mockup** (parallelizable any time before Stage 3) — `/ui-mockup O-MicrotonalSampler`.
