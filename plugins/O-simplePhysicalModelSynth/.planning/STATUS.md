---
plugin: O-simplePhysicalModelSynth
stage: 1
phase: discuss
status: in_progress
last_updated: 2026-06-26
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: run_research_phase
next_stage: 1
ready_for_implementation: true
parameter_spec_finalized: true
contract_checksums:
  brief: sha256:9c89444693922104f0b3ebb544558857f80ae3c7268937b64f76343ad820190f
  parameter_spec: sha256:4b4dfc92fe74cfabe91c0d9945fe9d97b35f5ae9ae09277db0482c2bf47728dd
  architecture: sha256:dae920bfd82c36699a13d9b17c5be099adf88d905282f87f597ec487e01bf38a
  roadmap: sha256:ba7b82b87727b31a0eb4cb491b55176ece081c214c89dde92299a2937d4e8071
---

# O-simplePhysicalModelSynth Status

## Current Position

Stage: 1 of 4 (Foundation + Shell) — in progress
Phase: discuss ✓ complete → research next
Status: parameter-spec.md finalized; CONTEXT.md written; ready for research phase
Progress: [###.................] 15%

## Phase Progress

### Stage 1: Foundation
| Phase | Status | Date | Notes |
|-------|--------|------|-------|
| discuss | ✓ | 2026-06-26 | parameter-spec.md promoted from draft (17 params locked); CONTEXT.md written |
| research | → | | Foundation/shell patterns (O-simpleFM analog) |
| plan | | | |
| execute | | | foundation-shell-agent |
| verify | | | pluginval + 17-param check |

## Completed So Far

**Ideation:** ✓ Complete (BRIEF.md, REQUIREMENTS.md, parameter-spec-draft.md)

**Stage 0 (Research & Planning):** ✓ Complete
- Plugin type: Synth (Pedagogical Physical Modeling), IS_SYNTH, 16-voice poly, WebView
- Complexity tier: 4 (synth) with Tier-6 visualization → research depth MODERATE/DEEP
- Architecture grounded in in-house production models (O-Lyrica KS, O-Bells modal, O-Bowed waveguide/friction; O-simpleFM structure/viz/harness)
- All 8 "Research Must Confirm" open questions resolved (see CONTEXT.md)
- JUCE classes mapped: `dsp::DelayLine<Thiran>`, custom `OnePoleLPF`, custom biquad bank, `ADSR`, `Synthesiser`, `dsp::FFT`, `VizRing`, APVTS
- DSP feasibility verified; highest risk (Bow friction) has memoryless-first plan + fallback
- Complexity score: 5.0 (capped; raw 13.0)
- Strategy: staged (Stage 2 DSP = 3 must-phases + 1 nice-phase; Stage 3 GUI = 3 phases)
- ARCHITECTURE.md and ROADMAP.md documented

## Next Steps

1. Stage 1: Foundation (CMake + 17-param APVTS + silent 16-voice shell + harness scaffold) — `/implement O-simplePhysicalModelSynth`
2. Review ARCHITECTURE.md and ROADMAP.md
3. Optional: create UI mockup and finalize full parameter-spec.md (replaces draft) before Stage 1 hardens ranges
4. Pause here

## Context to Preserve

**Key Decisions:**
- Modal = resonant biquad bank driven by the exciter (NOT triggered sinusoids) → cross-driving (FUNC-04) for free
- KS = v1.0 String engine; Position via exciter comb; Waveguide = `nice`/deferrable
- Thiran all-pass fractional delay for tuning (O-Bowed validated)
- Single global lead-voice viz tap (O-simpleFM VizRing reuse)
- Bow = memoryless STK friction, single control; enhanced friction out of scope

**Binding constraints:**
- CMake: IS_SYNTH/NEEDS_MIDI_INPUT/NEEDS_WEB_BROWSER/NEEDS_WEBVIEW2 + JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
- Naming: StringVoice/ModalVoice (not SamplerVoice); no bare end/begin param IDs
- Stage-2 gate: offline render-harness with autocorrelation pitch probe; drop PluginEditor.cpp + #if JUCE_WEB_BROWSER guard

**Files Created (Stage 0):**
- plugins/O-simplePhysicalModelSynth/.planning/research/ARCHITECTURE.md
- plugins/O-simplePhysicalModelSynth/.planning/ROADMAP.md
- plugins/O-simplePhysicalModelSynth/.planning/stages/0-ideation/CONTEXT.md

**Complexity:** 5.0 | **Strategy:** Staged
