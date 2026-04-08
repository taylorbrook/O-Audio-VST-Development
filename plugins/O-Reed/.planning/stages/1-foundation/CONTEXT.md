# O-Reed Stage 1: Foundation — Context

**Phase:** discuss
**Date:** 2026-04-04

## Decisions Confirmed

| Decision | Value |
|----------|-------|
| Plugin ID | `ouaricon-reed` |
| Plugin Code | `ORed` |
| Manufacturer Code | `${OUARICON_MANUFACTURER_CODE}` (shared) |
| Plugin Type | Synth (`IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`) |
| Audio Buses | Output-only stereo |
| WebView UI | `NEEDS_WEB_BROWSER TRUE` |
| Window Size | 900x600 |
| Parameters | 35 automatable — treat `parameter-spec-draft.md` as final |
| Voice Model | `juce::MPESynthesiserVoice` with `enableLegacyMode()` |
| Tuning Module | `modules/tuning/scala-tuning-engine` (existing, separate tab in UI) |
| Oversampling | Deferred to Stage 2 (Phase 3.5) |

## Parameter Notes

- INSTRUMENT_PRESET: Choice (0-20), 21 instrument presets as APVTS parameter. May evolve into a broader preset system later — 21 is the starting point.
- MAX_VOICES: Int (1-16), settings parameter (not automatable).
- All other 33 parameters are automatable.

## Tuning Integration

- Link existing `modules/tuning/scala-tuning-engine` via CMake
- C++ files: TuningEngine.h/cpp, ScaleGenerator.h/cpp, EmbeddedTunings.h/cpp, TuningExporter.h/cpp
- JS/CSS: tuning-panel.js, tuning-panel.css (separate tab)
- Integration snippets available in `modules/tuning/scala-tuning-engine/snippets/`
- Follow same pattern as O-Bowed, O-Lyrica

## Scope for Stage 1

- CMakeLists.txt with all JUCE modules + tuning module link
- PluginProcessor.h/cpp with APVTS (all 35 params), empty processBlock
- PluginEditor.h/cpp with WebView shell (900x600)
- Placeholder index.html
- Builds and loads in DAW as instrument (no audio yet)
- UI uses expandable panels and scrolling for parameter density
