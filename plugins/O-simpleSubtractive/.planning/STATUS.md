---
plugin: O-simpleSubtractive
stage: 2
status: complete
phase: verify
workflow_mode: express
last_updated: 2026-06-25
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: stage_3_gui
next_stage: 3
ready_for_implementation: true
contract_checksums:
  brief: sha256:f9e5eaaafc94bf1f3289806766fe24f609ef2ec9db1ad43cef7295b27694a58c
  parameter_spec: sha256:0ba50c5956d8db980db071f6f9982de71151dd2f84526407e255e91b0f5f8603
  architecture: sha256:90af107beec7bbd0eb0e6bc9db5533d6f0d97951a45dc422ae24254a0c22fea3
  roadmap: sha256:a713331a2b77ca7cf8b2dd274a895d6fa22684966604c145297f6c298f97859e
---

# O-simpleSubtractive Status

## Current Position

Stage: 2 of 4 (DSP) — ✅ complete (all 5 phases, express mode)
Status: Audible polyphonic subtractive synth. Render-harness 18/18, AU VALIDATION SUCCEEDED, pluginval s10 SUCCESS. Ready for Stage 3 (GUI).
Progress: [##########..........] 50%

## Stage 2 (DSP) — ✅ Complete

| Phase | Status | Artifact |
|-------|--------|----------|
| discuss | ✓ | stages/2-dsp/CONTEXT.md |
| research | ✓ | stages/2-dsp/RESEARCH.md |
| plan | ✓ | stages/2-dsp/PLAN.md |
| execute | ✓ | stages/2-dsp/SUMMARY.md (3 phases: 2.1 source+linear filter+ADSR+VCA, 2.2 self-osc+curve, 2.3 voice modes+glide+viz) |
| verify | ✓ | stages/2-dsp/VERIFICATION.md (PASS — 8/8 criteria) |

- New: `Source/OscillatorBank.h`, `SvfZDF.h`, `SubVoice.h`, `SubVizAnalyzer.h`, `tests/render-harness/`
- Modified: `Source/PluginProcessor.{h,cpp}` (Synthesiser + MonoStack + no-oversampling processBlock + viz tap), `CMakeLists.txt`
- **No oversampling** (PolyBLEP) → zero latency. Closed-form filter curve matches the audio filter at **0.00 dB** (QUAL-02 by construction).
- Self-osc: soft-knee limiter + localized negative-k bias → clean bounded sine, in tune at keyTrack=100%.

## Stage 1 (Foundation) — ✅ Complete

20-param silent shell; pluginval SUCCESS + AU VALIDATION SUCCEEDED; state round-trips. (See git + stages/1-foundation/.)

## Next Steps

1. **Stage 3: GUI** — WebView UI + parameter binding (relays/attachments) + draw the headline filter-curve-over-spectrum, oscilloscope, and dual-ADSR display from the already-built `SubVizAnalyzer` + display atomics + `VizRing`. Phased per ROADMAP (3 phases). Run `/clear` then `/implement O-simpleSubtractive`.
2. The audio→UI contract is in place and validated: lead-voice `displayCutoffHz`/`displayK`/`displayType`/`displaySlope` atomics, `filterEnvValue`/`ampEnvValue` atomics, `VizRing`, `getCurrentSampleRate()`.
3. Stage 3 re-introduces a `juce_add_binary_data` (UI resources) — give it a distinct NAMESPACE if a 2nd binary-data target ever appears (O-simpleGrain lesson).

## Context to Preserve

**Key Stage-2 decisions (see SUMMARY.md):**
- Self-osc needs a soft-knee limiter (NOT plain per-sample tanh, which damps) + slightly-negative k at the knob top; nonlinearity gated by `k < 0.6` so the magnitude curve stays exact in the normal range.
- Mono retrigger resets envelopes to 0 (JUCE `ADSR::noteOn()` doesn't zero); Legato suppresses retrigger; Mono/Legato drive voice 0 directly (sample-accurate slices), bypassing Synthesiser note-allocation.
- ENV_OCT=7 for the bipolar filter env; resonanceToK = `2·(1−res)^0.6 − 0.18·res^8`.

**Sibling references:** O-simpleFM (primary template — Synthesiser/Voice/VizRing/render-harness), O-Prism (SVF), O-Bassoon, O-simpleAdditive.

**Files created (Stage 2):**
- plugins/O-simpleSubtractive/Source/{OscillatorBank,SvfZDF,SubVoice,SubVizAnalyzer}.h
- plugins/O-simpleSubtractive/tests/render-harness/{main.cpp,CMakeLists.txt}
- plugins/O-simpleSubtractive/.planning/stages/2-dsp/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}.md
