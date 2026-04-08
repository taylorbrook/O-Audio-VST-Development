# O-Wind Stage 1: Foundation — Context

**Phase:** discuss
**Date:** 2026-04-04

## Decisions Confirmed

| Decision | Value |
|----------|-------|
| Plugin ID | `ouaricon-wind` |
| Plugin Code | `OWnd` |
| Manufacturer Code | `${OUARICON_MANUFACTURER_CODE}` (shared) |
| Plugin Type | Synth (`IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`) |
| Audio Buses | Output-only stereo |
| WebView UI | `NEEDS_WEB_BROWSER TRUE` |
| Window Size | 900x600 |
| Parameters | 13 automatable — treat `parameter-spec-draft.md` as final |
| Voice Model | `juce::SynthesiserVoice` (same as O-Lyrica, O-Bowed) |
| Tuning Module | `modules/tuning/scala-tuning-engine` (existing, separate tab in UI) |
| Oversampling | Deferred to Stage 2 |

## Tuning Integration

- Link existing `modules/tuning/scala-tuning-engine` via CMake
- C++ files: TuningEngine.h/cpp, ScaleGenerator.h/cpp, EmbeddedTunings.h/cpp, TuningExporter.h/cpp
- JS/CSS: tuning-panel.js, tuning-panel.css (separate tab)
- Integration snippets available in `modules/tuning/scala-tuning-engine/snippets/`
- Follow same pattern as O-Lyrica, O-Bowed

## Parameter Spec

13 parameters across 5 groups (Breath/Excitation, Resonator, Expression, Output, Impossible Physics).
Ranges and defaults from `parameter-spec-draft.md` are final — no changes.

## Scope for Stage 1

- CMakeLists.txt with all JUCE modules + tuning module link
- PluginProcessor.h/cpp with APVTS (all 13 params), empty processBlock
- PluginEditor.h/cpp with WebView shell (900x600)
- Placeholder index.html
- Builds and loads in DAW as instrument (no audio yet)
