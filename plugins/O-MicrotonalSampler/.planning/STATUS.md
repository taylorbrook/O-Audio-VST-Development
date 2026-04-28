---
plugin: O-MicrotonalSampler
stage: 2-dsp
phase: plan
status: plan_complete
last_updated: 2026-04-27
---

# Resume Point

## Current State: Stage 2 Plan Complete — Ready for Execute

Stage 2 (DSP) plan phase complete. PLAN.md decomposes Stage 2 into 5 sequential sub-stages (2.1 voice DSP → 2.2 loader → 2.3 vel crossfade → 2.4 voice-steal → 2.5 loop-detect) with 33 numbered tasks, file-level scope per task, dependency graph, per-sub-stage Gate verification (build + pluginval + functional test + atomic commit), and a final acceptance checklist mapping back to all 15 Stage-2 requirements. No new module deps — both shared modules and `juce_audio_formats`/`juce_dsp` already wired in Stage 1.

## Completed So Far

**Ideation:** ✓ Complete
**Stage 1 (Foundation):** ✓ Verified — silent shell builds + AU/VST3/Standalone validate
**Stage 2 Discuss:** ✓ Complete (CONTEXT.md, 2026-04-27)
**Stage 2 Research:** ✓ Complete (RESEARCH.md, 2026-04-27)
**Stage 2 Plan:** ✓ Complete (PLAN.md, 2026-04-27)

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

## Next Steps

1. **Stage 2 Execute (next)** — `/plugin-execute O-MicrotonalSampler 2-dsp`
2. UI mockup (parallelizable any time before Stage 3) — `/ui-mockup O-MicrotonalSampler`
