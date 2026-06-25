# Stage 1 (Foundation + Shell) — SUMMARY

**Plugin:** O-simpleGrain · **Stage:** 1 Foundation · **Date:** 2026-06-24
**Execute agent:** foundation-shell-agent · **Result:** silent 8-voice synth shell, full 18-param APVTS + state persistence.

## Files created
- `plugins/O-simpleGrain/CMakeLists.txt`
- `plugins/O-simpleGrain/Source/PluginProcessor.h`
- `plugins/O-simpleGrain/Source/PluginProcessor.cpp`
- `plugins/O-simpleGrain/Source/PluginEditor.h`
- `plugins/O-simpleGrain/Source/PluginEditor.cpp`

## What was built

**CMake (T1):** Mirrors O-simpleFM. `PLUGIN_CODE OsGr` (verified unique), `PRODUCT_NAME "O-simpleGrain${OUARICON_DEV_SUFFIX}"`, `VERSION 0.1.0`, FORMATS `VST3 AU Standalone`, `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE`, both WebView2 flags (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`), `JUCE_WEB_BROWSER=1`, `JUCE_USE_CURL=0`. `OuariconModules.cmake` included; `juce_generate_juce_header` after link. Standard JUCE module set (incl. `juce_dsp`, `juce_gui_extra`). **Omitted** `juce_add_binary_data` + `ouaricon_add_module` with marker comments (Stage 3 UI; `# TODO(Stage 2.3): embed built-in .wav sources`).

**Processor (T2/T3):** `OSimpleGrainAudioProcessor`. APVTS constructed in ctor init list; all 18 cached `std::atomic<float>*` pointers assigned. Engine constants as `static constexpr` (kMaxVoices=8, kMaxGrainsPerVoice=24, kGlobalGrainCap=192, kRootNote=60, kMaxSourceSeconds=10, kWindowLutSize=2048). `currentSourceIdentity{"embedded:fire"}`. `getLatencySamples()` NOT overridden; `setLatencySamples(0)` in `prepareToPlay`. Synth-only bus in `isBusesLayoutSupported` (output mono/stereo, input disabled). Silent, allocation-free `processBlock` (`ScopedNoDenormals`, `buffer.clear()`, MIDI drained).

**18 params (T3):** exact IDs/ranges/defaults/skews per parameter-spec.md. `density` log skew via `setSkewForCentre(20.0f)`; `grainSize` skew 0.4; ADSR times skew 0.35; `freeze` bool; `sourceSample`/`windowShape` choices (Hann default idx 4); `ampSustain` stored 0–1 (default 0.8); `outputLevel` −60 dB floor (perceptual "−inf"). Versioned `ParameterID{"id",1}` throughout. Per-param `withLabel(...)` unit suffixes for host display.

**State (T3):** APVTS XML round-trip PLUS a custom `"SOURCE"` child ValueTree (property `"identity"`) carrying `currentSourceIdentity`. Restored before `replaceState`; default preserved when child absent; root-type guard on load.

**Editor (T4):** minimal 720×480 `juce::AudioProcessorEditor`, paints "O-simpleGrain — Stage 1 shell". No WebView/relays/attachments/Timer.

## Deferred (by design)
- **DSP / audio** → Stage 2 (grain engine, voices, ADSR audio, window LUTs, read head). `processBlock` silent.
- **WebView UI / bindings / visualizations / drag-drop** → Stage 3.
- **Sample decode / embedding / hot-swap** → Stage 2.3 (`# TODO` left in CMake; `sourceSample` is a plain choice for now).
- **Presets / preset-manager** → Stage 4.

## Deviations
None functional. `outputLevel` uses a −60 dB floor as the "−inf" endpoint (standard). Added `withLabel` unit suffixes (within spec). O-simpleFM's later-stage code (preset manager, `juce::Synthesiser`, oversampling, viz ring, WebView, on-screen keyboard) used as structural reference only, intentionally not carried over.

## Build/verification
- Code authored to JUCE 8.0.9. Build + AU validation performed by the orchestrator in the verify phase (see VERIFICATION.md).
- Pre-build IDE diagnostics (`JuceHeader.h not found`, `juce::` undeclared) are the expected un-generated-header cascade; resolved once CMake configures (`juce_generate_juce_header`).
