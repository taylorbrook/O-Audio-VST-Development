---
plugin: O-Formant
stage: 4
status: complete
phase: verified
last_updated: 2026-04-05
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: install
next_phase: done
contract_checksums:
  brief: sha256:887e6d791af653926f6ceb139dae19cb6fcc89668d0193023f05aba3da64bd0b
  parameter_spec: sha256:4fb1124cb5c2b37036da9c6b8f0cc92cd29a77ae4f923f6ad83c65b7541c48f8
  architecture: sha256:2e1fae651a274f47508f13825aaf264db18e01f2bd34ab4a280780d47583e563
  roadmap: sha256:3e668584a37cb671c62136f655295c65876cfd79ee8e9adc0bf0ed7beeef010d
---

# O-Formant Status

## Current Position

Stage: 4 of 4 (Polish) -- VERIFIED COMPLETE
Status: All stages verified, pluginval level 10 PASSED (VST3 + AU)
Progress: [####################] 100%

## Completed So Far

- **Ideation:** Complete
  - Core concept defined (physical-model vocal synth, source-filter model)
  - 21 parameters specified with ranges and defaults
  - UI vision captured (2D XY vowel morph pad with formant peaks overlay)
  - Use cases identified (film/game, electronic, ambient, education)
  - Requirements extracted with acceptance criteria (26 requirements)
  - 6 research documents referenced

- **Stage 0:** Complete
  - Plugin type defined: Synth (MIDI Instrument)
  - Professional examples researched: 5 (Pink Trombone, VocalSynth 2, Cantor Digitalis, Plaits, Humanoid)
  - JUCE modules identified: juce_audio_basics, juce_audio_processors, juce_dsp, juce_core, juce_gui_basics, juce_gui_extra
  - DSP feasibility verified (all components implementable with JUCE 8 + custom code)
  - Parameter ranges researched and validated
  - Complexity score: 5.0 (raw 8.2)
  - Strategy: Phase-based implementation (3 DSP phases, 3 GUI phases)
  - ARCHITECTURE.md documented with 11 sections
  - ROADMAP.md documented with phased breakdown
  - CONTEXT.md captured key decisions and constraints

- **Stage 1:** Foundation complete - Build system operational, 21 parameters implemented
  - CMakeLists.txt: IS_SYNTH TRUE, NEEDS_MIDI_INPUT TRUE, juce_dsp linked
  - PluginProcessor: MPESynthesiser with 16 FormantVoice instances, enableLegacyMode()
  - FormantVoice: MPESynthesiserVoice skeleton with 21 cached parameter pointers, silent output
  - APVTS: All 21 parameters (20 Float + 1 Bool) with correct ranges, defaults, skew factors
  - State management: getStateInformation/setStateInformation via APVTS XML
  - GenericAudioProcessorEditor for Stage 1 UI
  - Output-only stereo bus (instrument, no input bus)

- **Stage 2 Phase 2.1:** Core Vocal Engine -- 9 DSP files created, 5 files modified
  - GlottalWavetable: Flat vector storage (128 Rd x 2048 samples x 10 mipmap levels, ~10MB shared)
  - GlottalTableGenerator: Fant 1995 regression, Newton-Raphson solvers, FFT mipmap generation
  - LFGlottalSource: Per-voice wavetable oscillator with bilinear interpolation (Rd + mipmap level)
  - AspirationNoise: Single-pole IIR LP at 4kHz, SmoothedValue breathiness, per-voice Random
  - VowelData: Csound bass voice formant tables (5 vowels, F1-F5, BW, gains as linear)
  - FormantBiquad: 32-byte DF2T biquad struct with inline processSample
  - FormantFilterBank: 5 parallel BPFs with shift/spread, makeBandPass coefficients
  - VowelMorpher: Shepard IDW interpolation, log-domain frequency blending
  - FormantVoice: Full per-sample loop with block-rate coefficient updates, ADSR envelope
  - PluginProcessor: Wavetable generation at construction, voice preparation in prepareToPlay
  - Architecture deviation: Mipmapped wavetable (not direct LF + PolyBLEP) per user decision

- **Stage 2 Phase 2.2:** Modulation & Expression -- 3 DSP files created, 2 files modified
  - VibratoLFO: Sine LFO with onset delay ramp, micro-jitter (+/-0.5% per cycle)
  - PitchGlide: One-pole exponential smoother for portamento (from O-Prism pattern)
  - ConsonantEngine: KLATT parallel noise (LP/HP crossfade + sibilance BP + plosive burst)
  - FormantVoice: Per-sample F0 chain (glide->vibrato->jitter), consonant mix, MPE expression
  - MPE integration: pressure->breathiness (additive), timbre->vowelY (offset), velocity->burst
  - 19 of 21 parameters now connected

- **Stage 3 Phase 3.1:** Layout + Controls + Binding -- WebView operational, 21 parameter controls bound
  - CMakeLists.txt: NEEDS_WEB_BROWSER TRUE, NEEDS_WEBVIEW2 TRUE, binary data target
  - PluginEditor: 20 WebSliderRelays + 1 WebToggleButtonRelay + WebBrowserComponent
  - index.html: Ouaricon Naturalist aesthetic, XY vowel morph pad, 18 seed knobs, 1 toggle
  - main.js: XY pad drag/automation, knob drag/automation, toggle binding
  - Resource provider: bare path matching (5 resources)
  - Build: VST3 + AU compiled, installed to system folders

- **Stage 3 Phase 3.2:** Visual Polish -- Formant overlay, cursor glow, ADSR visualization
  - Cursor glow: 28px radial moss-green gradient behind XY pad cursor
  - Formant dots: F1-F5 labeled dots in lower XY pad, Shepard IDW + shift/spread
  - ADSR canvas: DPR-aware, linear segments, reactive to 4 ADSR params
  - 7 new relay listeners (formantShift, formantSpread, vowelFocus, attack, decay, sustain, release)
  - No C++ changes, JS-only additions
  - Build: VST3 + AU compiled, auval PASS, pluginval level 5 PASS

- **Stage 4 Phase 4.1:** DSP Completion + Presets -- outputGain, stereoWidth, 16 factory presets, preset browser UI
  - outputGain: SmoothedValue 50ms, dB-to-linear, post-synth in processBlock
  - stereoWidth: Per-voice equal-power pan by MIDI note in FormantVoice
  - OuariconPresetManager integrated with 16 factory presets (4 categories)
  - Preset browser WebView UI (prev/next, category dropdown, save)
  - 10 native functions for preset communication
  - pluginval level 5 PASSED
  - State persistence: preset name survives DAW save/load

- **Stage 4 Phase 4.2:** Validation + Release -- pluginval level 10, CHANGELOG.md
  - pluginval level 10 PASSED on VST3 (seed: 0x59b5378) -- all tests including parameter thread safety, state restoration, non-releasing SR switch
  - pluginval level 10 PASSED on AU (seed: 0x5a23e58) -- all tests passed
  - auval PASSED (aumu OuFm OuDv)
  - CHANGELOG.md created (v1.0.0, Keep a Changelog format)
  - VST3 + AU installed to system folders
  - No code fixes needed -- level 10 passed clean on first run

## Next Steps

1. Install plugin to system folders (`/install-plugin O-Formant`)

## Context to Preserve

**Architecture files:**
- `plugins/O-Formant/.planning/research/ARCHITECTURE.md` -- DSP specification (immutable contract)
- `plugins/O-Formant/.planning/ROADMAP.md` -- Implementation plan with phases
- `plugins/O-Formant/.planning/stages/2-dsp/PLAN.md` -- Phase 2.1 task breakdown
- `plugins/O-Formant/.planning/stages/2-dsp/PLAN-2.2.md` -- Phase 2.2 task breakdown
- `plugins/O-Formant/.planning/stages/2-dsp/RESEARCH-2.2.md` -- Phase 2.2 research

**Key Decisions:**
- Voice framework: juce::MPESynthesiser + MPESynthesiserVoice with enableLegacyMode()
- Formant topology: Parallel (not cascade) for v1
- Glottal source: Mipmapped wavetable (deviation from ARCHITECTURE.md, user chose this)
- Custom biquad structs for formant filters (not juce::dsp::IIR::Filter)
- IS_SYNTH TRUE + NEEDS_MIDI_INPUT TRUE in CMakeLists.txt
- Output-only bus (no audio input)
- Per-voice pitch glide (deviation from architecture's monophonic last-note glide)
- MPE pressure additive above knob baseline (not multiplicative)

**Files Created (Phase 2.2):**
- plugins/O-Formant/Source/dsp/VibratoLFO.h
- plugins/O-Formant/Source/dsp/PitchGlide.h
- plugins/O-Formant/Source/dsp/ConsonantEngine.h

**Files Modified (Phase 2.2):**
- plugins/O-Formant/Source/FormantVoice.h
- plugins/O-Formant/Source/FormantVoice.cpp
