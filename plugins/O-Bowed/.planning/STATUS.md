---
plugin: O-Bowed
version: 1.3.0
stage: 4
phase: verified
status: complete
last_updated: 2026-04-26
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: dorico_microtonal_smoke_test
next_phase: complete
contract_checksums:
  brief: sha256:0ed36f0b45d58bffffc4595fc1cb1bac080262c68fb3ff0280c0e702aab4b38c
  parameter_spec: sha256:31daa7a53cd0c954ed391d5d0960b5671b78703f063870334d5b9fb533f4930e
  architecture: sha256:954007a23cb50e52c071dd381434ff15c05fcf010274437a6642d6840100fa9e
  roadmap: sha256:e35e47ed476558a088b909f6a88936f8a138d423dddbeb1c35a4f56f180dd2a5
---

# O-Bowed Status

## Current Position

Stage: 4 of 4 (Polish) -- VERIFIED / COMPLETE
Status: All stages verified. 11 factory presets, pluginval level 10 (VST3 + AU), auval pass, CHANGELOG.md v1.0.0. Ready for install.
Progress: [####################] 100%

## v1.3.0 -- Phase 24 propagation (2026-04-26)

- **VST3 Note Expression microtonal support for Dorico** added via shared `note-expression` module (`modules/tuning/note-expression` v1.0.0).
- **Helper-based MPE composition** — `applyPendingTuning` inserted INSIDE `getBaseFrequencyFromTuning(int midiNote)` at `BowedStringVoice.cpp:291-307`. Single source of truth covers both call sites: `noteStarted()` (line 32) and `notePitchbendChanged()` (line 71). `exchange(0.0)` consume semantics correct for one-NE-per-noteOn delivery — second call returns base unchanged because slot is already consumed (correct: NE applies once per noteStarted; MPE pitch-bend updates compose multiplicatively per-block on top via `currentFrequency *= pow(2, bendSemitones/12.0f)`).
- **Composition order:** tuning engine → NE delta → MPE pitch-bend → `waveguideString.trigger(currentFrequency)` (waveguide string period sized to the final tuned frequency on sample 0).
- **CMakeLists.txt:** added `PLUGIN_VERSION "1.3.0"` (was missing from `juce_add_plugin(O-Bowed ...)` — same explicit-add pattern as O-Wind v1.16.0 / O-Reed v1.1.0) + `ouaricon_add_module(O-Bowed note-expression)`.
- Tri-format build clean; bundles freshly installed; AU validates via `scripts/verify-au-link.sh O-Bowed`.
- Dorico 3-point smoke gate DEFERRED to Phase 24 batch validation (per orchestrator direction).

## Completed So Far

**Stage 3 (GUI):** Complete
- WebView UI with Ouaricon Naturalist aesthetic (900x600)
- All 23 parameters bound: 21 WebSliderRelay + 2 WebComboBoxRelay
- SVG arc knobs with relative drag, shift-fine mode, double-click reset
- 3 canvas visualizations: bow-string animation, body resonance spectrum, Schelleng diagram
- Preset browser with save/load/navigate
- Tuning panel integration (Scala/TUN file loading)
- Conditional visibility for stringTuning1-4 and sympatheticAmount
- Botanical illustration overlay
- OuariconPresetManager integrated into processor

**Ideation:** Complete
- Core concept defined (PM bowed string synthesizer)
- Parameters specified (22 automatable parameters)
- Signal flow documented
- Tiered friction model architecture specified
- Morphable body resonator designed
- Microtonal tuning (Scala/TUN, MTS-ESP) included
- MPE support specified
- Requirements extracted with acceptance criteria (27 total)
- Competitive positioning established

**Stage 0:** Complete
- Plugin type defined: Synth (Physical Modeling Bowed String)
- Professional examples researched: 5 (SWAM, Soliste, VS-3, Preparation 2, STK)
- JUCE modules identified: juce_dsp, juce_audio_processors, juce_audio_basics, juce_gui_extra, juce_gui_basics
- DSP feasibility verified (all components implementable with JUCE 8 + custom DSP)
- Parameter ranges researched from acoustic literature and professional plugin analysis
- Complexity score: 5.0 (maximum, raw score 18.0)
- Strategy: Phase-based implementation (5 DSP phases, 3 GUI phases)
- ARCHITECTURE.md documented (11 core components, complete signal flow)
- ROADMAP.md documented (complexity breakdown, phase test criteria)
- CONTEXT.md documented (key decisions, constraints, open questions)

## Next Steps

1. Stage 1: Foundation (create build system and parameters) - Run `/implement O-Bowed`
2. Review ARCHITECTURE.md and ROADMAP.md
3. Pause here

## Context to Preserve

**Key Decisions:**
- Plugin type: Synth (IS_SYNTH TRUE, NEEDS_MIDI_INPUT TRUE)
- Friction model: Tiered (core hyperbolic / enhanced elasto-plastic / quality thermal)
- Body resonator: 8 parallel biquads with coefficient morphing
- Voice pattern: juce::SynthesiserVoice (same as O-Lyrica)
- Oversampling: 2x for friction junction (juce::dsp::Oversampling)
- Tuning: Shared module at modules/tuning/scala-tuning-engine
- Multi-string: 1-4 active + 0-12 sympathetic waveguide strings

**Architecture Files:**
- plugins/O-Bowed/.planning/research/ARCHITECTURE.md
- plugins/O-Bowed/.planning/ROADMAP.md
- plugins/O-Bowed/.planning/stages/0-ideation/CONTEXT.md

**Research Files:**
- research/O-Bowed-research-synthesis.md
- research/bow-string-friction-models.md
- research/O-Bowed-market-research.md
- research/O-Bowed-acoustic-instrument-research.md

## Files Created
- plugins/O-Bowed/.planning/BRIEF.md (ideation)
- plugins/O-Bowed/.planning/REQUIREMENTS.md (ideation)
- plugins/O-Bowed/.planning/parameter-spec-draft.md (ideation)
- plugins/O-Bowed/.planning/research/ARCHITECTURE.md (Stage 0)
- plugins/O-Bowed/.planning/ROADMAP.md (Stage 0)
- plugins/O-Bowed/.planning/stages/0-ideation/CONTEXT.md (Stage 0)
