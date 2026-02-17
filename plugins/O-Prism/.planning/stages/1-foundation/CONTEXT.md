# Stage 1: Foundation - Context

## Discussion Summary

**Date:** 2026-02-16
**Participants:** User, Claude

## Requirements Confirmed

- 68 APVTS parameters as specified in ARCHITECTURE.md parameter mapping table
- CMakeLists.txt with 7 JUCE modules: juce_audio_basics, juce_audio_processors, juce_audio_formats, juce_dsp, juce_gui_basics, juce_gui_extra, juce_core
- IS_SYNTH TRUE, NEEDS_MIDI_INPUT TRUE, NEEDS_WEB_BROWSER TRUE, NEEDS_WEBVIEW2 TRUE
- JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 for Windows cross-platform support
- PluginProcessor with juce::Synthesiser member, TuningEngine member, APVTS
- PluginEditor with WebView shell at 1200x800 using WebViewRelayManager
- Stub PrismVoice (extends juce::SynthesiserVoice) and PrismSound (extends juce::SynthesiserSound)
- Tuning engine source files copied from scala-tuning-engine module v2.1.0
- Output-only BusesProperties (synth instrument, no audio input)
- getStateInformation/setStateInformation stubs (APVTS + ValueTree for custom state)

## Constraints Identified

- No MTS-ESP support in v1.0 (factory tunings + Scala/KBM import only)
- No preset manager in Stage 1 (deferred to Stage 4)
- No factory wavetable BinaryData in Stage 1 (basic waveforms generated at runtime)
- 68 parameters is highest count in catalog -- requires careful APVTS organization

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Parameter ID naming | BRIEF naming (no prefix) | Match ARCHITECTURE.md table: oscAPos, filtACutoff, masterTune, tuningPreset, etc. |
| WebView relay management | WebViewRelayManager class | Automatic lifecycle management for 68 relays. Cleanest approach for high parameter count. |
| MTS-ESP support | Not in v1.0 | Keep scope manageable. Factory tunings + Scala/KBM import covers all v1.0 use cases. |
| Preset manager | Deferred to Stage 4 | Foundation focuses on APVTS + state persistence. Presets added with factory presets in validation stage. |
| Wavetable data for Stage 1 | Runtime generation | Generate basic single-cycle waveforms (sine, saw, square, triangle) programmatically at startup. No BinaryData needed yet. |
| Internal precision | Double internal, float I/O | Convert float input to double in processBlock, all DSP classes use double, convert back to float for output. Matches BRIEF spec (NFR-01: 64-bit double precision). |
| Voice architecture | Follow O-Lyrica pattern | PrismVoice receives APVTS pointer and TuningEngine pointer from processor. Synthesiser handles MIDI routing and voice stealing. |
| Module integration | scala-tuning-engine v2.1.0 | Copy C++ source files to Source/. Follow integration checklist. Adapt parameter IDs to BRIEF naming. |

## Tuning Module Adaptation Notes

The scala-tuning-engine integration checklist uses `tuning_` prefixed parameter IDs (e.g., `tuning_masterTune`). Since we're using BRIEF naming (no prefix), the following mappings apply:

| Module Convention | O-Prism Parameter ID |
|-------------------|---------------------|
| tuning_masterTune | masterTune |
| tuning_tuningMode | (not used -- no MTS-ESP, mode implicit from tuningPreset) |
| tuning_octaveStretch | octaveStretch |
| tuning_pitchBendRange | pitchBendRange |
| tuning_temperamentPreset | tuningPreset |

The `tuning_tuningMode` parameter (12-TET/Custom/MTS-ESP choice) is not needed since we're not supporting MTS-ESP. The tuning mode is implicit: tuningPreset selects a factory tuning or the user loads a Scala file for custom tuning.

## Stage 1 Deliverables

1. **CMakeLists.txt** -- Build system with all modules, flags, and source files
2. **PluginProcessor.h/cpp** -- APVTS (68 params), Synthesiser, TuningEngine, state persistence stubs
3. **PluginEditor.h/cpp** -- WebView shell (1200x800), WebViewRelayManager with 68 relays + attachments
4. **PrismVoice.h/cpp** -- Stub voice class (extends SynthesiserVoice, produces silence)
5. **PrismSound.h/cpp** -- Stub sound class (extends SynthesiserSound, responds to all notes)
6. **TuningEngine.h/cpp** -- Copied from scala-tuning-engine module
7. **ScaleGenerator.h/cpp** -- Copied from scala-tuning-engine module
8. **EmbeddedTunings.h/cpp** -- Copied from scala-tuning-engine module
9. **TuningExporter.h/cpp** -- Copied from scala-tuning-engine module (optional, for HTML export)

## Test Criteria (from ROADMAP.md)

- [ ] Project builds with no errors (VST3 + AU + Standalone)
- [ ] Plugin loads in DAW as instrument (appears in synth/instrument category)
- [ ] DAW routes MIDI to plugin (verify with MIDI monitor)
- [ ] All 68 parameters visible in DAW automation list
- [ ] Plugin produces silence (no DSP yet, but no crashes)
- [ ] WebView opens at 1200x800 with placeholder content
- [ ] State save/restore works (APVTS round-trip)

## Open Questions

None -- all foundation decisions resolved.

## Next Phase

Ready for: **research** phase (or skip to **plan** phase, since ARCHITECTURE.md already covers Stage 1 scope in detail)
