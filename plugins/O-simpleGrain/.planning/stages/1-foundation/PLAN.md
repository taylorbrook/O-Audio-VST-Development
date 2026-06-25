# Stage 1 (Foundation) — PLAN

**Plugin:** O-simpleGrain · **Stage:** 1 Foundation + Shell · **Date:** 2026-06-24
**Execute agent:** foundation-shell-agent
**Goal:** A silent, valid 8-voice synth shell (VST3 + AU + Standalone) on macOS with the full 18-param APVTS, state persistence (incl. loaded-source identity), and a correct cross-platform WebView/CMake config — no audio (Stage 2), no WebView UI (Stage 3).

## Tasks

### T1 — CMakeLists.txt
- Create `plugins/O-simpleGrain/CMakeLists.txt` mirroring `plugins/O-simpleFM/CMakeLists.txt`.
- `juce_add_plugin(O-simpleGrain ...)`: `PLUGIN_CODE OsGr`, `PRODUCT_NAME "O-simpleGrain${OUARICON_DEV_SUFFIX}"`, `VERSION "0.1.0"`, FORMATS `VST3 AU Standalone`, `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT FALSE`, `IS_MIDI_EFFECT FALSE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`.
- `include(...OuariconModules.cmake)` first.
- `target_sources`: PluginProcessor.{cpp,h}, PluginEditor.{cpp,h} only.
- `target_link_libraries`: the standard JUCE module set from O-simpleFM (audio_basics/devices/formats/plugin_client/processors/utils, core, data_structures, dsp, events, graphics, gui_basics, gui_extra) + recommended config/lto/warning flags.
- `juce_generate_juce_header(O-simpleGrain)` after link.
- `target_compile_definitions`: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`.
- **OMIT** the `juce_add_binary_data` UI block and `ouaricon_add_module` (Stage 3). Add a comment: `# Stage 3 (GUI): juce_add_binary_data UI resources + ouaricon_add_module here.` and `# TODO(Stage 2.3): embed built-in .wav sources via juce_add_binary_data.`
- **Files:** `plugins/O-simpleGrain/CMakeLists.txt` (new).

### T2 — PluginProcessor.h
- `class OSimpleGrainAudioProcessor : public juce::AudioProcessor`.
- Members: `juce::AudioProcessorValueTreeState apvts;` `static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();`
- Cached raw-param atomic pointers for all 18 params (declared; assigned in ctor/prepare — unused while silent but established).
- Engine constants as `static constexpr`: `kMaxVoices = 8`, `kMaxGrainsPerVoice = 24`, `kGlobalGrainCap = 192`, `kRootNote = 60`, `kMaxSourceSeconds = 10`, `kWindowLutSize = 2048`.
- `juce::String currentSourceIdentity { "embedded:fire" };` (custom non-APVTS state).
- Standard AudioProcessor overrides declared. **Do NOT override `getLatencySamples()`** (non-virtual JUCE 8).
- **Files:** `plugins/O-simpleGrain/Source/PluginProcessor.h` (new).

### T3 — PluginProcessor.cpp
- `createParameterLayout()` builds all **18 params** exactly per `parameter-spec.md` (IDs/ranges/defaults/skews; versioned `ParameterID{"id",1}`; `density` log skew via `setSkewForCentre(~20)`; `grainSize`/ADSR skews; `freeze` bool; `sourceSample`/`windowShape` choices, Hann default index 4).
- Ctor: `apvts(*this, nullptr, "PARAMETERS", createParameterLayout())`; assign cached atomics.
- `prepareToPlay`: `setLatencySamples(0)`; (no DSP).
- `processBlock`: `juce::ScopedNoDenormals`; clear output buffer; consume MIDI (no synth yet); **silent, allocation-free**.
- `isBusesLayoutSupported`: synth — output mono or stereo, no input bus.
- `getStateInformation`/`setStateInformation`: serialize APVTS tree **+** `currentSourceIdentity` (custom child/property); round-trip restores both.
- `createEditor` returns the Stage-1 placeholder editor; `hasEditor() = true`.
- `createPluginFilter()` factory returns the processor.
- **Files:** `plugins/O-simpleGrain/Source/PluginProcessor.cpp` (new).

### T4 — PluginEditor.{h,cpp}
- Minimal `juce::AudioProcessorEditor` placeholder (no WebView — Stage 3). Fixed/resizable size (~720×480), paints a simple "O-simpleGrain — Stage 1 shell" label so it's visibly alive. Compiles and closes cleanly.
- **Files:** `plugins/O-simpleGrain/Source/PluginEditor.{h,cpp}` (new).

### T5 — SUMMARY.md
- Write `stages/1-foundation/SUMMARY.md`: files created, the 18 params as implemented (IDs + ranges), state-persistence approach, what's intentionally deferred (DSP→S2, UI→S3, sample embedding→S2.3), and any deviations.

## Dependencies
T1 → (T2, T3, T4 independent of each other but all compile together) → T5. Build verification after T1–T4.

## Success Criteria (verify phase)
1. `cmake --build build --target O-simpleGrain_VST3 O-simpleGrain_AU O-simpleGrain_Standalone` (or `ninja` equivalents) builds clean.
2. Plugin auto-discovered (CMakeLists present).
3. All 18 params present with correct IDs/ranges/defaults (pluginval / generic editor dump).
4. AU validates structurally (`auval` after install) / VST3 loads.
5. State round-trips (params + `currentSourceIdentity`).
6. Loads as instrument, accepts MIDI, **silent**, no crash.
7. `processBlock` allocation-free.

## Non-Goals (do not do in Stage 1)
- No grain DSP, voices, ADSR audio, window LUTs, read head (Stage 2).
- No WebView UI, relays/attachments, visualizations, drag-drop (Stage 3).
- No `.wav` embedding / sample decode (Stage 2.3) — leave the documented TODO.
- No presets / preset-manager (Stage 4).
