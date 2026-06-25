---
plugin: O-simpleSubtractive
stage: 1
status: complete
phase: verify
workflow_mode: express
last_updated: 2026-06-25
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: stage_2_dsp
next_stage: 2
ready_for_implementation: true
contract_checksums:
  brief: sha256:f9e5eaaafc94bf1f3289806766fe24f609ef2ec9db1ad43cef7295b27694a58c
  parameter_spec: sha256:0ba50c5956d8db980db071f6f9982de71151dd2f84526407e255e91b0f5f8603
  architecture: sha256:90af107beec7bbd0eb0e6bc9db5533d6f0d97951a45dc422ae24254a0c22fea3
  roadmap: sha256:a713331a2b77ca7cf8b2dd274a895d6fa22684966604c145297f6c298f97859e
---

# O-simpleSubtractive Status

## Current Position

Stage: 1 of 4 (Foundation) — ✅ complete (all 5 phases, express mode)
Status: Silent 20-param synth shell built + validated (pluginval SUCCESS + AU VALIDATION SUCCEEDED). Ready for Stage 2 (DSP).
Progress: [#####...............] 25%

## Stage 1 (Foundation) — ✅ Complete

| Phase | Status | Artifact |
|-------|--------|----------|
| discuss | ✓ | stages/1-foundation/CONTEXT.md |
| research | ✓ | stages/1-foundation/RESEARCH.md |
| plan | ✓ | stages/1-foundation/PLAN.md |
| execute | ✓ | stages/1-foundation/SUMMARY.md (foundation-shell-agent) |
| verify | ✓ | stages/1-foundation/VERIFICATION.md (PASS) |

- Files: `plugins/O-simpleSubtractive/CMakeLists.txt`, `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.{h,cpp}`
- `PLUGIN_CODE OSiS` (aumu OSiS OuDv), v1.0.0, IS_SYNTH + MIDI + WebView2 flags
- 20-param APVTS confirmed (auval: "20 Global Scope Parameters"); state round-trips; processBlock = silence
- Editor: GenericAudioProcessorEditor placeholder (WebView swap in Stage 3)

## Completed So Far

**Ideation:** ✓ Complete
- Core concept: pedagogical subtractive synth (osc→filter→VCA, two independent ADSRs)
- Architecture decided via ideation; 24 requirements extracted (must 14 / should 8 / nice 2)

**Stage 0:** ✓ Complete
- Plugin type: Synth (pedagogical subtractive), MIDI-in → audio-out, 16-voice poly / mono / legato, WebView UI
- Complexity tier 4 escalated toward 6 by first-class real-time filter-curve + FFT/scope viz → research depth MODERATE→DEEP
- **Filter topology RESOLVED:** custom Cytomic ZDF state-variable filter (all 4 modes + 6/12/24 dB + tanh self-osc + exact closed-form magnitude curve); Moog ladder rejected; linear `StateVariableTPTFilter` kept as Fallback A
- Anti-aliasing: PolyBLEP/polyBLAMP (composes — steady phase increment); `keyTrack` param added; bipolar filter-env in octaves
- Professional plugins researched: Minimoog/Moog ladder, TB-303, Prophet/Oberheim SEM (SVF lineage), u-he/Arturia VA, Cytomic
- JUCE 8 APIs verified against local source (8.0.9): Synthesiser, SynthesiserVoice, ADSR, dsp::StateVariableTPTFilter, dsp::FirstOrderTPTFilter, dsp::LadderFilter, dsp::LookupTableTransform, dsp::FFT, dsp::WindowingFunction, SmoothedValue
- Complexity score: **5.0** (capped; raw 11.0)
- Strategy: **staged** (Stage 2 DSP × 3 phases, Stage 3 GUI × 3 phases)
- ARCHITECTURE.md + ROADMAP.md + Stage-0 CONTEXT.md documented

## Next Steps

1. **Stage 2: DSP** — phased build (3 phases per ROADMAP): 2.1 source + LINEAR filter + dual ADSR + VCA; 2.2 self-oscillation + gain comp + magnitude-curve validation; 2.3 voice modes + glide + offline render-harness gate. Run `/clear` then `/implement O-simpleSubtractive`.
2. Highest risk lives here: self-oscillating multimode SVF — build linear filter + curve match FIRST, add self-osc second.
3. Pause here (handoff at Stage 1→2 boundary).

## Context to Preserve

**Key decisions:**
- Filter: custom Cytomic ZDF SVF; Notch = input − k·BP; cascade ×2 for 24 dB (resonance on stage 1); 1-pole for 6 dB; tanh self-osc + gain compensation
- Headline visual: closed-form filter magnitude (`Ω = tan(π·f/fs)/g`) over live output spectrum — same g/k as audio → QUAL-02 by construction
- Anti-aliasing: PolyBLEP/polyBLAMP; no oversampling; zero latency
- `keyTrack` added (default 0%); bipolar `filterEnvAmount` in octaves; 20 core params
- Voice modes via Synthesiser + processor-side MonoController (Mono/Legato + glide) — the new-vs-sibling risk area
- Highest risk: self-oscillating SVF — build linear filter + curve match FIRST, add self-osc second

**Strategy:** complexity 5.0, staged implementation.

**Files created:**
- plugins/O-simpleSubtractive/.planning/research/ARCHITECTURE.md
- plugins/O-simpleSubtractive/.planning/ROADMAP.md
- plugins/O-simpleSubtractive/.planning/stages/0-ideation/CONTEXT.md
- plugins/O-simpleSubtractive/.planning/STATUS.md (this file)

**Sibling references:** O-simpleFM (primary template), O-simpleAdditive, O-Prism (SVF), O-Bassoon, O-simpleGrain.
