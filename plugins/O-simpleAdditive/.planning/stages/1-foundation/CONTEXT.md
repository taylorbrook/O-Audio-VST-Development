# Stage 1: Foundation — CONTEXT

**Plugin:** O-simpleAdditive
**Stage:** 1 of 4 (Foundation)
**Mode:** express (auto-compiled from contracts — no interactive session)
**Source:** BRIEF.md, research/ARCHITECTURE.md, ROADMAP.md, parameter-spec.md, STATUS.md
**Date:** 2026-06-22

---

## Goal

A **silent, buildable synth shell**: CMake target (synth + WebView2 flags), the full
**33-parameter APVTS**, and state persistence. No audio rendering (first sound: Stage 2 /
Phase 2.1), no WebView UI (Stage 3 — `GenericAudioProcessorEditor` placeholder for now).

## Requirements satisfied this stage

- **FOUND-01** Plugin loads in DAW as an **instrument** (IS_SYNTH, MIDI input), no crash.
- **FOUND-02** All 33 parameters present in the host's generic editor with correct ranges/defaults.
- **FOUND-03** State round-trips (save/reload preserves every parameter).
- **COMPAT-02 (prep)** Cross-platform WebView2 CMake flags set NOW so Stage 3 inherits a correct config.

## Constraints / Decisions (locked at Stage 0)

| Decision | Value | Rationale |
|----------|-------|-----------|
| Polyphony | 16 voices | Matches O-simpleFM. |
| **Oversampling** | **NONE** | Band-limiting is exact (omit k>Kmax). → `setLatencySamples(0)`. **Key delta vs O-simpleFM.** |
| Latency | **Zero** | No oversampling. `getLatencySamples()` is non-virtual in JUCE 8 — never override; use `setLatencySamples(0)`. |
| Parameter count | **33** | 16 drawbars + frameBSource + scanPosition + scanLfoRate + scanLfoDepth + scanEnvAmount + spectralDecay + bitDepth + velToDecay + ampADSR×4 + modADSR×4 + outputLevel. |
| Param storage | drawbars/percent stored **0–1 normalized** (UI ×100 in Stage 3) | Matches O-simpleFM `unitRange()` convention. |
| `frameBSource` | `AudioParameterChoice {sine, saw, square, odd}`, default **saw** | Preset-only Frame B (no 2nd drawbar set). |
| `bitDepth` | `AudioParameterChoice {off,12,10,8,6,4,2}`, default **off** | "off" = clean passthrough sentinel. |
| Defaults | H1=100%, partials 2–16 = 0% | Pure sine on load (pedagogical start). |
| Editor (this stage) | `GenericAudioProcessorEditor` | WebView is Stage 3. |
| Preset manager / FactoryPresets | **NOT yet** | Stage 3.3 / Stage 4. Foundation persists plain APVTS XML. |
| `processBlock` (this stage) | clear buffer + smoothed output-gain ramp + NaN scrub | Silent shell; synth voices added Stage 2. |

## Template / Reuse

**Primary template: O-simpleFM** (shipped). Foundation is a near-verbatim port of
`plugins/O-simpleFM/{CMakeLists.txt, Source/PluginProcessor.{h,cpp}}` with three subtractions:

1. **Remove oversampling** — no `juce::dsp::Oversampling`, no `scaledMidi` rebase, no `kOsFactorLog2`. `setLatencySamples(0)`.
2. **Remove feedback / carrier-tracking / on-screen-keyboard machinery** — `lastNoteHz`, `carrierSounding`, `midiCollector`, `handleUiMidi`, `VizRing` are Stage 2/3 concerns, not Foundation.
3. **Swap the 17-param FM layout for the 33-param additive layout** (see parameter-spec.md).

Keep verbatim: synth/voice skeleton wiring pattern, `BusesProperties` (stereo out, no input),
`isBusesLayoutSupported`, dB→lin smoothed output gain, `ScopedNoDenormals`, state persistence shape.

## Out of scope (later stages)

- Additive render / band-limit / morph / spectral-decay / bit-depth DSP → Stage 2.
- WebView UI, relays/attachments, drawbar spectrum, oscilloscope, tooltips, presets → Stage 3 / 4.
- `VizRing` / `FmVizAnalyzer` viz tap → Stage 2.3 / 3.2.

## Success criteria (this stage)

1. `ninja O-simpleAdditive_VST3 O-simpleAdditive_AU` builds clean (no warnings-as-errors break).
2. `auval` passes for the AU; plugin appears as an instrument.
3. Generic editor shows **exactly 33** parameters, correct ranges/defaults (H1=100%, rest 0%, bitDepth=off, frameBSource=saw).
4. Save state → change params → reload → all 33 restored.
5. Plays silently (no audio yet) without crashing on MIDI input.
6. `setLatencySamples(0)` — host reports zero latency.
