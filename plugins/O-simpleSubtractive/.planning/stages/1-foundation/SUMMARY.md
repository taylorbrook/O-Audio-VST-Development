# Stage 1 (Foundation) — SUMMARY

**Plugin:** O-simpleSubtractive
**Date:** 2026-06-25
**Phase:** execute (foundation-shell-agent)
**Outcome:** Source complete — silent, host-loadable subtractive-synth shell with full 20-parameter APVTS + XML state persistence. No DSP (`processBlock` outputs silence). Build/validation deferred to verify phase.

---

## Files created
- `plugins/O-simpleSubtractive/CMakeLists.txt`
- `plugins/O-simpleSubtractive/Source/PluginProcessor.h`
- `plugins/O-simpleSubtractive/Source/PluginProcessor.cpp`
- `plugins/O-simpleSubtractive/Source/PluginEditor.h`
- `plugins/O-simpleSubtractive/Source/PluginEditor.cpp`

## CMake (key facts)
`PLUGIN_CODE OSiS`, `PRODUCT_NAME "O-simpleSubtractive${OUARICON_DEV_SUFFIX}"`, `VERSION 1.0.0`, `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT FALSE`, `IS_MIDI_EFFECT FALSE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`, `FORMATS VST3 AU Standalone`, Ouaricon company/manufacturer vars. Module set includes `juce_dsp`/`juce_gui_extra`/`juce_audio_utils`; `juce_generate_juce_header` AFTER `target_link_libraries`; compile defs `JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_WEB_BROWSER=1 JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 JUCE_USE_CURL=0`. No `juce_add_binary_data`/`ouaricon_add_module`/test `add_subdirectory` (deferred to Stages 2–4).

## 20 parameters (all match ARCHITECTURE.md contract; `ParameterID{id,1}` 2-arg form)
| ID | Value as implemented |
|----|----------------------|
| oscWave | choice [Saw,Square,Triangle,Sine] def Saw(0) |
| subLevel | 0..1 def 0 |
| noiseLevel | 0..1 def 0 |
| filterType | choice [Low-pass,High-pass,Band-pass,Notch] def 0 |
| filterSlope | choice [6,12,24 dB/oct] def idx2 (24) |
| cutoff | 20–20000 Hz def 2000, log skew 0.25 |
| resonance | 0..1 def 0.10 |
| filterEnvAmount | −1..+1 def +0.50 (bipolar) |
| keyTrack | 0..1 def 0 |
| filterAttack/Decay/Sustain/Release | 0.005 / 0.30 / 0.40 / 0.20 |
| ampAttack/Decay/Sustain/Release | 0.005 / 0.30 / 0.80 / 0.10 |
| voiceMode | choice [Poly,Mono,Legato] def 0 |
| glide | 0–1 s def 0, skew 0.5 |
| outputLevel | −60..0 dB def 0 (−60 floor = −inf) |

ADSR time ranges `NormalisableRange{0, 5, 0.0001, 0.35}`.

## Editor decision
Option B (PLAN T4): thin `OSimpleSubtractiveAudioProcessorEditor` hosting a `GenericAudioProcessorEditor` child (640×760, resizable). Keeps class identity + CMake `target_sources` stable so Stage 3 swaps only the body for WebView; matches O-simpleFM layout; all 20 params visible/testable in any host.

## Deviations from PLAN
- ADSR time min = 0.0 s (literal "0–5 s" contract; PLAN noted template used 0.001). Defaults unchanged, in range.
- `outputLevel` floor −60 dB represents "−inf" (contract note + O-simpleFM precedent).
- `OuariconModules.cmake` `include()`d per T1 but no `ouaricon_add_module` call (Stage 4); inert at foundation.
- Choice labels human-readable ("Low-pass/High-pass/Band-pass/Notch", "6/12/24 dB/oct"); DSP will map by index in Stage 2 — indices/defaults match contract.

## Risks flagged for verify build
- Warnings-as-errors: unused `samplesPerBlock`/`midiMessages` guarded with `juce::ignoreUnused`; `currentSampleRate` is a written member. Expected clean.
- AU validation should report exactly 20 global-scope params, category Music Device (`aumu OSiS OuDv` dev / `OuAu` release).
- `PLUGIN_CODE OSiS` assumed free (OSiF/OSiA/OsGr taken) — confirm no AU-registry collision.
- Silence path: `buffer.clear()` + MIDI consumed without synthesis — confirm no NaN/denormal/crash under auval MIDI/render passes.

## Verify-phase actions (orchestrator)
Configure CMake → `ninja O-simpleSubtractive_VST3 O-simpleSubtractive_AU` → auval + pluginval → confirm 20 params + state round-trip + silence. Results → VERIFICATION.md.
