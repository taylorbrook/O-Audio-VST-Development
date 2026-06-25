# Stage 1 (Foundation) — PLAN

**Plugin:** O-simpleSampler · **Stage:** 1 Foundation + Shell · **Date:** 2026-06-25
**Execute agent:** foundation-shell-agent (implemented inline by the orchestrator, mirroring O-simpleGrain commit `2ae282e`)
**Goal:** A silent, valid **16-voice** synth shell (VST3 + AU + Standalone) on macOS with the full **21-param APVTS**, state persistence (incl. loaded-source identity), and a correct cross-platform WebView/CMake config — no audio (Stage 2), no WebView UI (Stage 3).

## Tasks

### T1 — CMakeLists.txt
- Create `plugins/O-simpleSampler/CMakeLists.txt` mirroring O-simpleGrain's foundation CMake.
- `juce_add_plugin(O-simpleSampler ...)`: `PLUGIN_CODE OsSm`, `PRODUCT_NAME "O-simpleSampler${OUARICON_DEV_SUFFIX}"`, `VERSION "0.1.0"`, FORMATS `VST3 AU Standalone`, `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT FALSE`, `IS_MIDI_EFFECT FALSE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`.
- `include(...OuariconModules.cmake)` first.
- `target_sources`: PluginProcessor.{cpp,h}, PluginEditor.{cpp,h} only.
- Standard JUCE module link set + recommended config/lto/warning flags.
- `juce_generate_juce_header(O-simpleSampler)` after link.
- `target_compile_definitions PUBLIC`: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`.
- **OMIT** the `juce_add_binary_data` UI block + `ouaricon_add_module` (Stage 3) AND the samples binary-data target (Stage 2.3). Add TODO comments documenting the **dual-NAMESPACE split** (`UIBinaryData` UI vs `BinaryData` samples).
- **Files:** `plugins/O-simpleSampler/CMakeLists.txt` (new).

### T2 — PluginProcessor.h
- `class OSimpleSamplerAudioProcessor : public juce::AudioProcessor`.
- `namespace OSimpleSampler::ParamIDs` — all 21 IDs as `inline constexpr auto`.
- Members: `juce::AudioProcessorValueTreeState apvts;` + `static ParameterLayout createParameterLayout();`.
- Cached `std::atomic<float>*` for all 21 params.
- Engine constants `static constexpr`: `kMaxVoices = 16`, `kMaxGrainsPerVoice = 4`, `kRootNote = 60`, `kMaxSourceSeconds = 30`, `kStretchGrainMs = 60`, `kNumBuiltIns = 4`.
- `juce::String currentSourceIdentity { "embedded:piano" };` + getter/setter; `kBuiltInNames[kNumBuiltIns] = { "piano","vocal","flute","vinyl" }`; custom-state tags `kSourceStateTag="SOURCE"`, `kSourceIdProp="identity"`.
- Standard AudioProcessor overrides declared. **Do NOT override `getLatencySamples()`.** `getAPVTS()` accessor for the Stage-3 editor.
- **Files:** `plugins/O-simpleSampler/Source/PluginProcessor.h` (new).

### T3 — PluginProcessor.cpp
- `createParameterLayout()` builds all **21 params** exactly per `parameter-spec.md` (versioned IDs; `loopCrossfade`/ADSR skews; `filterCutoff` `setSkewForCentre(1000)`; `ampSustain` unit 0–1; `rootKey`/`tune` as `AudioParameterInt`; `sourceSample`/`loopMode`/`pitchMode` choices; `reverse` bool).
- Ctor: `AudioProcessor(BusesProperties().withOutput("Output", stereo, true))`, `apvts(*this, nullptr, "PARAMETERS", createParameterLayout())`; cache the 21 atomics.
- `prepareToPlay`: store sample rate; `setLatencySamples(0)`; (no DSP).
- `processBlock`: `ScopedNoDenormals`; `buffer.clear()`; consume MIDI; **silent, allocation-free**.
- `isBusesLayoutSupported`: synth — output mono or stereo, input bus disabled.
- `getStateInformation`/`setStateInformation`: APVTS tree **+** `SOURCE/identity` child; round-trip restores both.
- `createEditor` → Stage-1 placeholder; `createPluginFilter` factory.
- **Files:** `plugins/O-simpleSampler/Source/PluginProcessor.cpp` (new).

### T4 — PluginEditor.{h,cpp}
- Minimal `juce::AudioProcessorEditor` placeholder (no WebView). `setSize(720, 480)`; paints centered "O-simpleSampler" + "Stage 1 shell — 21 parameters, silent (audio: Stage 2, UI: Stage 3)". Compiles + closes cleanly.
- **Files:** `plugins/O-simpleSampler/Source/PluginEditor.{h,cpp}` (new).

### T5 — SUMMARY.md
- Write `stages/1-foundation/SUMMARY.md`: files created, the 21 params as implemented (IDs + ranges), state-persistence approach, deferred items (DSP→S2, UI→S3, sample embedding→S2.3, dual-NAMESPACE TODO), deviations.

## Dependencies
T1 → (T2, T3, T4 compile together) → build verify → T5.

## Success Criteria (verify phase)
1. `cmake --build build --target O-simpleSampler_VST3 O-simpleSampler_AU O-simpleSampler_Standalone` builds clean.
2. Plugin auto-discovered (CMakeLists present; `file(GLOB)`).
3. All 21 params present with correct IDs/ranges/defaults (pluginval / generic editor dump).
4. AU validates structurally (`auval`) / VST3 loads.
5. State round-trips (params + `currentSourceIdentity`).
6. Loads as instrument, accepts MIDI, **silent**, no crash.
7. `processBlock` allocation-free.

## Non-Goals (do not do in Stage 1)
- No sampler DSP, voices, read head, ADSR audio, loop/Stretch/Vintage/filter (Stage 2).
- No WebView UI, relays/attachments, waveform editor, drag-drop (Stage 3).
- No `.wav` embedding / sample decode (Stage 2.3) — leave the documented TODO.
- No presets / preset tour (Stage 3.3 / Stage 4).
