# Stage 1 (Foundation) — CONTEXT

**Plugin:** O-simpleBeatmaker
**Stage:** 1 of 4 — Foundation
**Mode:** Express (auto-advance phases; stop at stage boundary)
**Source:** Auto-compiled from BRIEF.md + ARCHITECTURE.md + ROADMAP.md + parameter-spec.md (Stage 0 contracts), plus two user decisions captured at discuss (below).

## Goal

A **silent, loadable shell**: a synth plugin that instantiates in any DAW (VST3 + AU + Standalone), passes pluginval, exposes all **42 APVTS parameters** via the host generic editor, and persists both the APVTS params **and** the custom 6×32 step-grid (`ValueTree "PATTERN"`). **No DSP, no WebView** — first audio is Stage 2, WebView UI is Stage 3.

## Decisions captured at discuss (2026-06-25)

1. **Run mode = Express** — auto-advance discuss→research→plan→execute→verify within the stage; STOP at the Stage 1→2 boundary for `/clear` + review.
2. **Voice Tune = ±12 semitones** (LOCKED) — resolves the ARCHITECTURE "semitones OR Hz offset" ambiguity; consistent across all 6 voices. Foundation-locking (baked into the APVTS layout / saved sessions).

## Requirements covered

- **COMPAT-01** — loads in DAWs / passes pluginval as a silent shell.
- APVTS layout (42 params) + custom PATTERN ValueTree state + persistence.

## Constraints (inherited, immutable)

- ARCHITECTURE.md is the immutable contract; this stage implements its *Parameter Mapping* and *State Persistence* sections exactly.
- Grid is **custom atomic state**, NOT 384 APVTS params (hard rule).
- WebView2 cross-platform flags set NOW so Stage 3 inherits correct config; the WebView itself is added in Stage 3.
- `getLatencySamples()` is non-virtual in JUCE 8 → use `setLatencySamples(0)`, never override.
- Plugin code `OSiB` (verified free; fits the `OSi{X}` family: OSiF/OSiS/OSiA).

## Out of scope (later stages)

- Voice synthesis, SequencerClock, TimingFeelEngine, VizAnalyzer (Stage 2).
- WebView UI, binary-data target, parameter relays/attachments (Stage 3).
- Factory presets, render-harness probes, optimization (Stage 2 harness / Stage 4).

## Primary reference

O-simpleSubtractive Stage 1 (freshest sibling Foundation): CMake flags, `createParameterLayout` idiom, `GenericAudioProcessorEditor` shell, `getStateInformation`/`setStateInformation`. O-simpleBeatmaker adds the **PATTERN child** on top of that pattern.
