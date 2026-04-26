---
plugin: O-Wind
version: 1.16.0
stage: 4
phase: verify
status: installed
gsd_phase: installed
last_updated: 2026-04-26
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: dorico_microtonal_smoke_test
next_stage: complete
contract_checksums:
  brief: sha256:9627df12cfc8f3dc786fb17e553ea73c225e928a8df21408315b4565d9d908cb
  parameter_spec: sha256:20a6e6b97db3c23fc70be36a8315b13c06b1b0650b650d95b92b11d629ee6145
  architecture: sha256:abb527d15fd83c0f80f68c99c56592a6ee1f39392a48d8387c424195a3a42da1
  roadmap: sha256:8f7e6a5ac390ecb0eb36132c4beeb455578173a96abe5437a9bea805f44ed14c
---

# O-Wind Status

## Current Position

Stage: 4 of 4 (Polish) -- VERIFIED COMPLETE
Status: All stages complete. pluginval level 10 PASSED (VST3+AU, DSP+GUI). All 25 requirements verified. Ready for install.
Progress: [####################] 100%

## Completed So Far

**Ideation:** Complete
- Core concept defined (jet-drive waveguide flute synthesizer)
- 13 parameters specified with ranges, defaults, MIDI mappings
- 4 core instrument presets defined (Concert Flute, Shakuhachi, Bansuri, Native American Flute)
- 3 impossible physics parameters for creative sound design
- Two-tier tone hole architecture specified
- Signal flow documented with equations from research
- 25 requirements extracted with acceptance criteria
- 3 research documents completed pre-ideation

**Stage 0:** Complete
- Plugin type defined: Synth (Physical Modeling Flute)
- Professional examples researched: 4 (SWAM Flutes, SWAM VariFlute, Respiro, Ventus Series)
- JUCE modules identified: juce_dsp, juce_audio_processors, juce_audio_basics, juce_gui_extra, juce_gui_basics
- DSP feasibility verified (all JUCE 8 APIs confirmed)
- Complexity score: 5.0 (raw 11.6, capped)
- Strategy: Phase-based implementation (4 DSP phases, 3 GUI phases)
- ARCHITECTURE.md documented (14 core components, 6 architecture decisions)

**Stage 1 (Foundation):** Complete
- CMakeLists.txt created (IS_SYNTH TRUE, NEEDS_MIDI_INPUT TRUE, WebView2 static linking)
- FluteSynthSound.h stub created
- PluginProcessor.h/cpp with APVTS (16 parameters: 14 plugin + 2 tuning)
- PluginEditor.h/cpp with WebView shell (14 relays, 14 attachments, 900x600)
- Build: ninja O-Wind_VST3 O-Wind_AU -- success (zero errors)
- AU registered and installed

**Stage 2 (DSP):** Complete -- 10 DSP components implemented across 4 phases
- Phase 3.1: JetExciter, JetNonlinearity, DCBlocker, BoreWaveguide, FluteSynthVoice, processor wiring
- Phase 3.2: 2x oversampling (per-voice), 8-voice polyphony, breath noise, vibrato LFO, SmoothedValue crossfade, CC/MPE mapping, latency reporting
- Phase 3.3: InstrumentPresets (8 presets), StereoWidth processor, SubHarmonics, infinite sustain, reversed jet, air column connection
- Phase 3.4: ToneHoleSystem (Tier 2 Keefe scattering), expansion presets, tuning system integration, full MPE pitch bend

**Stage 3 (GUI):** Phase 3.1 complete
- 3-tab Naturalist WebView UI (SOUND | TUNING | EFFECTS)
- 14 slider parameters bound via WebSliderRelay + WebSliderParameterAttachment
- 1 toggle parameter (toneHoleToggle) bound via WebToggleButtonRelay
- 1 int parameter (instrumentPreset) bound via WebSliderRelay (0-7 range)
- Preset browser with 8 factory presets via OuariconPresetManager
- Instrument preset selector (8 instrument types)
- Tuning panel integration (lazy-loaded from shared module)
- Botanical fern overlay
- All native functions registered (preset, instrument, tuning)
- Build: zero errors, VST3 + AU installed

## Next Steps

1. Install plugin (`/install-plugin O-Wind`)
2. Manual DAW testing

## v1.16.0 — Phase 24 propagation (2026-04-26)

Adopted shared `note-expression` module — O-Wind responds to Dorico VST3 Note Expression `kTuningTypeID` events. Composition order in `FluteSynthVoice::startNote`: TuningEngine assignment → `applyPendingTuning` (float→double cast at helper boundary) → pitch-bend → BoreWaveguide period derivation (Pattern 2: physical-model period sees tuned frequency at sample 0). CMake delta (a): added missing `PLUGIN_VERSION "1.16.0"` line inside `juce_add_plugin(O-Wind ...)` block. Tri-format build clean; AU validates via `verify-au-link.sh O-Wind`. Atomic 8-file commit. Dorico 3-point smoke gate deferred to Phase 24 batch validation.

## Context to Preserve

**Key Decisions:**
- Plugin type: Synth (Physical Modeling Flute)
- Core concept: Jet-drive + bore waveguide, multi-instrument via parameter sets
- Simpler than O-Bowed: no iterative solver, no body resonator, one-directional exciter
- 2x per-voice oversampling wraps entire feedback loop
- 8 instrument presets (4 core + 4 expansion)
- Stereo width via allpass decorrelation + mid-side (shared O-Bowed pattern)
- Tuning system integrated via parameter listeners
- OuariconPresetManager replaces manual state save/load
- toneHoleToggle and instrumentPreset are APVTS parameters (not ad-hoc atomics)

## Files Created/Modified

### New Files (Stage 3)
- plugins/O-Wind/Resources/ui/img/fern.png

### Modified Files (Stage 3)
- plugins/O-Wind/Resources/ui/index.html (complete 3-tab UI replacing shell)
- plugins/O-Wind/Source/PluginProcessor.h (added OuariconPresetManager, removed atomic preset index)
- plugins/O-Wind/Source/PluginProcessor.cpp (preset manager, factory presets, toneHoleToggle + instrumentPreset params)
- plugins/O-Wind/Source/PluginEditor.h (added toggle relay, instrument relay, fileChooser)
- plugins/O-Wind/Source/PluginEditor.cpp (all native functions, toggle/instrument bindings, fern resource)
- plugins/O-Wind/Source/FluteSynthVoice.h (removed presetIndex pointer param)
- plugins/O-Wind/Source/FluteSynthVoice.cpp (reads instrumentPreset + toneHoleToggle from APVTS)
- plugins/O-Wind/CMakeLists.txt (preset manager include, fern image binary data)
