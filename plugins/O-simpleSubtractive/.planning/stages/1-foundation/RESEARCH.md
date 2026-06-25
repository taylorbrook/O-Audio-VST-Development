# Stage 1 (Foundation) — RESEARCH

**Plugin:** O-simpleSubtractive
**Date:** 2026-06-25
**Phase:** research
**Depth:** Light — foundation is a sibling-port of a SOLVED pattern (O-simpleFM). All JUCE 8 APIs were already verified against local source (8.0.9) in Stage 0. This phase confirms the exact CMake/APVTS shapes to copy and the gotchas to honor.

---

## Primary template: O-simpleFM (`plugins/O-simpleFM/`)

O-simpleFM is the same archetype (pedagogical synth, MIDI-in, WebView, 16-voice, FFT viz) one generation older and fully shipped. **Copy its foundation skeleton near-verbatim**, swapping the parameter layout (17 → our 20) and identifiers.

### CMake (`O-simpleFM/CMakeLists.txt`) — what to mirror
- `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` at top.
- `juce_add_plugin(O-simpleSubtractive ...)` with:
  - `COMPANY_NAME "${OUARICON_COMPANY_NAME}"`, `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}` (Ouaricon vars resolve to OuAu/OuDv + `-dev` suffix automatically).
  - `PLUGIN_CODE OSiS` — **verified free** (existing codes scanned; OSiF/OSiA/OsGr taken, OSiS not used).
  - `PRODUCT_NAME "O-simpleSubtractive${OUARICON_DEV_SUFFIX}"`, `VERSION "1.0.0"`.
  - `FORMATS VST3 AU Standalone`, `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT FALSE`, `IS_MIDI_EFFECT FALSE`.
  - `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`.
- `target_sources`: `PluginProcessor.cpp/.h`, `PluginEditor.cpp/.h` only (DSP headers added Stage 2; viz Stage 2/3).
- `target_include_directories(... PRIVATE Source)`.
- JUCE module links incl. `juce_dsp`, `juce_gui_extra`, `juce_audio_utils`; `juce::juce_recommended_*` PUBLIC.
- `juce_generate_juce_header(O-simpleSubtractive)` **after** `target_link_libraries` (JUCE 8 requirement).
- `target_compile_definitions(... PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_WEB_BROWSER=1 JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 JUCE_USE_CURL=0)` — **set WebView2 flags now** so Stage 3 inherits a correct cross-platform config.

### What O-simpleFM foundation did NOT include (defer)
- `juce_add_binary_data` UI-resources target → **Stage 3** (no UI files exist yet; adding the target now would fail the build referencing missing index.html).
- `ouaricon_add_module(... preset-manager)` → **Stage 4**.
- `OUARICON_BUILD_TESTS` render-harness `add_subdirectory` → **Stage 2** (the offline DSP correctness gate).

### Plugin auto-registration
Top-level `CMakeLists.txt` (lines 48–55) does `file(GLOB plugins/*)` + `add_subdirectory`. **No manual edit to the root CMake needed** — the new `plugins/O-simpleSubtractive/CMakeLists.txt` is picked up automatically.

## APVTS pattern (`O-simpleFM/Source/PluginProcessor.h/.cpp`)
- Parameter IDs as a "single source of truth" block of `static constexpr` string-view IDs (top of header).
- `juce::AudioProcessorValueTreeState parameters;` member; public `getAPVTS()` accessor for the editor.
- `static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();` builds all params.
- `getStateInformation`/`setStateInformation` override → XML round-trip of `apvts.state` (`copyXmlToBinary`/`getXmlFromBinary`).

### JUCE 8 parameter-construction notes (verified, 8.0.9)
- `juce::AudioParameterChoice` for `oscWave` (4), `filterType` (4), `filterSlope` (3), `voiceMode` (3) — `StringArray` of labels.
- `juce::AudioParameterFloat` with `juce::NormalisableRange<float>` for all floats.
  - **Log skew on `cutoff`**: `NormalisableRange<float>(20.f, 20000.f, 0.f, 0.25f)` (skew ≈0.25 ≈ `getSkewForCentre` near ~640 Hz) — or use `setSkewForCentre`. Match ARCHITECTURE skew ≈0.25.
  - **Bipolar `filterEnvAmount`**: range −1..+1 (display −100..+100%), default +0.5.
  - Time params (`*Attack/Decay/Release`): `0..5 s`, skew 0.35; `glide` 0..1 s skew 0.5.
  - Percent params (`subLevel/noiseLevel/resonance/*Sustain/keyTrack`): 0..1 normalized, display 0–100%.
  - `outputLevel`: −60..0 dB (represent "−inf" as −60 dB floor) default 0.
- Prefer the JUCE 8 `ParameterID{ id, versionHint }` 2-arg constructors (version hint = 1) to silence deprecation and keep host automation stable.

## Editor (foundation = minimal)
- Use **`juce::GenericAudioProcessorEditor`** as the foundation editor: exposes all 20 params in every host immediately → directly verifies the APVTS without any UI work. Replaced wholesale by the WebView editor in Stage 3.
- Set a sensible default size; no Timer, no WebView, no binary data at foundation.

## processBlock (silent stub)
- `juce::ScopedNoDenormals noDenormals;` then `buffer.clear();` (clear any output channels beyond inputs too). Consume MIDI without acting on it.
- `setLatencySamples(0)` in `prepareToPlay` (`getLatencySamples()` is **non-virtual** in JUCE 8 — memory).
- `acceptsMidi()` true, `producesMidi()` false, `isMidiEffect()` false (synth).

## Gotchas honored (from memory / siblings)
- **WebView2 dual flag**: `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` together, set at foundation. (No WebView constructed yet, but config locked.)
- **BinaryData namespace collision** (O-simpleGrain): N/A at foundation (single/zero binary-data target); flagged for Stage 3 when UI resources land.
- **Dev/release variant shadowing**: handled by build/install tooling (`build-and-install.sh` Phase 4), not foundation source.
- **`juce_generate_juce_header` ordering**: must follow `target_link_libraries`.

## Sibling cross-checks
- O-simpleAdditive (`OSiA`) — alternate WebView CMake template; confirms same flag set.
- O-Bassoon — JUCE 8 `SynthesiserVoice` patterns (relevant Stage 2, noted now).

## Open questions
**None.** Foundation is fully specified by contracts + this template. Proceed to plan.
