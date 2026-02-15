# Stage 1: Foundation - Execution Plan

**Date:** 2026-02-14
**Phase:** plan
**Goal:** Create a compilable JUCE plugin with ANIRA + ONNX Runtime linked, placeholder models embedded as binary data, all 10 APVTS parameters, and Processor/Editor skeletons.

---

## Goal Statement

Build the O-Texture plugin shell: CMake project with ANIRA FetchContent integration, placeholder ONNX models embedded via `juce_add_binary_data`, all 10 parameters in APVTS with cached atomic pointers, a PluginProcessor skeleton (IS_SYNTH TRUE, stereo out + disabled sidechain, empty processBlock), and a PluginEditor skeleton with WebView placeholder. The plugin must compile, link without missing symbols, and load in a DAW.

---

## Tasks

### 1. [ ] Create CMakeLists.txt with ANIRA FetchContent
- **Files:** `plugins/O-Texture/CMakeLists.txt`
- **Depends on:** none
- **Details:**
  - Follow O-TextureForge pattern (IS_SYNTH TRUE, NEEDS_MIDI_INPUT TRUE)
  - Plugin code: `OuTx` (4 chars, unique)
  - Product name: `O-Texture${OUARICON_DEV_SUFFIX}`
  - Formats: VST3 AU Standalone
  - NEEDS_WEB_BROWSER TRUE, NEEDS_WEBVIEW2 TRUE
  - Add ANIRA via FetchContent (v2.0.3):
    ```cmake
    include(FetchContent)
    FetchContent_Declare(anira
        GIT_REPOSITORY https://github.com/anira-project/anira.git
        GIT_TAG v2.0.3
    )
    set(ANIRA_WITH_ONNXRUNTIME ON CACHE BOOL "" FORCE)
    set(ANIRA_WITH_LIBTORCH OFF CACHE BOOL "" FORCE)
    set(ANIRA_WITH_TFLITE OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(anira)
    ```
  - Link `anira::anira` to plugin target
  - Link standard JUCE modules: audio_basics, audio_devices, audio_formats, audio_plugin_client, audio_processors, audio_utils, core, data_structures, dsp, events, graphics, gui_basics, gui_extra
  - Compile definitions: JUCE_VST3_CAN_REPLACE_VST2=0, JUCE_WEB_BROWSER=1, JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1, JUCE_USE_CURL=0
  - Include OuariconModules.cmake for licensing module support
  - `juce_generate_juce_header()`

### 2. [ ] Add binary data targets for models and UI
- **Files:** `plugins/O-Texture/CMakeLists.txt` (append to Task 1)
- **Depends on:** Task 1
- **Details:**
  - Create `OuariconTexture_ModelData` binary data target:
    ```cmake
    juce_add_binary_data(OuariconTexture_ModelData
        NAMESPACE ModelData
        HEADER_NAME ModelData.h
        SOURCES
            Resources/models/placeholder/decoder.onnx
            Resources/models/placeholder/encoder.onnx
            Resources/models/placeholder/prior.onnx
    )
    ```
  - Create `OuariconTexture_UIResources` binary data target:
    ```cmake
    juce_add_binary_data(OuariconTexture_UIResources
        SOURCES
            Source/ui/public/index.html
    )
    ```
  - Link both binary data targets to plugin
  - Note: UI resources will expand in Stage 3 (add js/juce/index.js etc.)

### 3. [ ] Create placeholder index.html for WebView
- **Files:** `plugins/O-Texture/Source/ui/public/index.html`
- **Depends on:** none
- **Details:**
  - Minimal HTML page with "O-Texture - Stage 1" heading
  - Dark background matching Ouaricon style (#1a1a2e)
  - Lists all 10 parameters with current values (placeholder text)
  - Will be replaced in Stage 3 with full XY pad UI

### 4. [ ] Create PluginProcessor header and implementation
- **Files:** `plugins/O-Texture/Source/PluginProcessor.h`, `plugins/O-Texture/Source/PluginProcessor.cpp`
- **Depends on:** none
- **Details:**
  - Class name: `TextureProcessor` (synth naming convention)
  - Inherits `juce::AudioProcessor`
  - **Bus configuration:** Stereo output enabled, sidechain input disabled (future Transform mode):
    ```cpp
    BusesProperties()
        .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    ```
  - **APVTS member:** `juce::AudioProcessorValueTreeState parameters`
  - **Static method:** `createParameterLayout()` with all 10 parameters:
    - SOURCE: `AudioParameterChoice` (6 options: Rain, Metal, Wind, Crowd, Synth, Organic, default 0)
    - MODE: `AudioParameterChoice` (2 options: Generate, Transform, default 0)
    - X: `AudioParameterFloat` (0.0-1.0, default 0.5)
    - Y: `AudioParameterFloat` (0.0-1.0, default 0.5)
    - CHARACTER_A: `AudioParameterFloat` (0.0-1.0, default 0.5)
    - CHARACTER_B: `AudioParameterFloat` (0.0-1.0, default 0.5)
    - EVOLVE: `AudioParameterFloat` (0.0-1.0, default 0.3)
    - FREEZE: `AudioParameterBool` (default false)
    - BRIGHTNESS: `AudioParameterFloat` (-1.0-1.0, default 0.0)
    - MIX: `AudioParameterFloat` (0.0-1.0, default 1.0)
  - **Cached parameter pointers:** `std::atomic<float>*` for all 10 params, initialized in constructor via `getRawParameterValue()`
  - **processBlock:** `ScopedNoDenormals`, clear buffer (Stage 1 produces silence)
  - **prepareToPlay:** Store sample rate, log buffer size (preparation for Stage 2)
  - **getLatencySamples:** Return 6144 (4096 block + 2048 overlap)
  - **State save/restore:** Direct APVTS pattern (copyState/replaceState, same as O-TextureForge)
  - **acceptsMidi:** true (IS_SYNTH)
  - **producesMidi:** false
  - **isMidiEffect:** false
  - **getName:** "O-Texture"

### 5. [ ] Create PluginEditor header and implementation
- **Files:** `plugins/O-Texture/Source/PluginEditor.h`, `plugins/O-Texture/Source/PluginEditor.cpp`
- **Depends on:** Task 4
- **Details:**
  - Class name: `TextureEditor` (synth naming convention)
  - Inherits `juce::AudioProcessorEditor`
  - **WebView member:** `std::unique_ptr<juce::WebBrowserComponent> webView`
  - **WebView setup:**
    - Backend: webview2
    - Native integration enabled
    - Resource provider for binary data
    - Windows WebView2 user data folder: `"OTexture_WebView"` (temp directory)
  - **Resource provider:** Map `/` to `index.html` from BinaryData
  - **Window size:** 800x600
  - **Paint:** Fill dark background (#1a1a2e)
  - **Resized:** WebView fills bounds
  - No relays or attachments yet (Stage 3)
  - No timer yet (Stage 3)

### 6. [ ] Verify build compiles and links
- **Files:** none (build verification)
- **Depends on:** Tasks 1-5
- **Details:**
  - Run CMake configure (may need to skip O-Texture initially if ANIRA FetchContent fails, then troubleshoot)
  - Build with `ninja OuariconTexture_VST3 OuariconTexture_AU`
  - Verify: No missing symbols, no linker errors
  - Verify: ANIRA + ONNX Runtime link successfully
  - Verify: Binary data targets compile (ModelData.h, BinaryData.h accessible)
  - If ANIRA FetchContent fails: troubleshoot CMake integration, check network access, verify git tag

### 7. [ ] Install and verify DAW loading
- **Files:** none (DAW verification)
- **Depends on:** Task 6
- **Details:**
  - Clear AU cache, install to system folders per CLAUDE.md protocol
  - Verify plugin appears in DAW plugin list (as instrument, IS_SYNTH TRUE)
  - Verify plugin window opens with WebView placeholder
  - Verify all 10 parameters appear in DAW automation list
  - Verify state save/restore (save project, reload, check parameter values)
  - Run `auval -a | grep -i texture` to verify AU registration

---

## Files Summary

**Created:**
- `plugins/O-Texture/CMakeLists.txt` — Full CMake config with ANIRA FetchContent
- `plugins/O-Texture/Source/PluginProcessor.h` — Processor header with APVTS, cached params
- `plugins/O-Texture/Source/PluginProcessor.cpp` — Processor impl with 10 params, empty processBlock
- `plugins/O-Texture/Source/PluginEditor.h` — Editor header with WebView
- `plugins/O-Texture/Source/PluginEditor.cpp` — Editor impl with resource provider
- `plugins/O-Texture/Source/ui/public/index.html` — Placeholder WebView page

**Existing (used as binary data):**
- `plugins/O-Texture/Resources/models/placeholder/decoder.onnx` (129 KB)
- `plugins/O-Texture/Resources/models/placeholder/encoder.onnx` (131 KB)
- `plugins/O-Texture/Resources/models/placeholder/prior.onnx` (6.5 KB)

---

## Success Criteria

- [ ] Plugin compiles without errors (CMake + Ninja)
- [ ] ANIRA and ONNX Runtime link without missing symbols
- [ ] Placeholder ONNX models embedded as binary data (ModelData namespace accessible)
- [ ] Plugin loads in DAW as instrument (IS_SYNTH TRUE classification)
- [ ] All 10 parameters visible in DAW automation list
- [ ] WebView placeholder page displays in plugin window (800x600)
- [ ] State save/restore works (parameter values persist across DAW reload)
- [ ] `auval -a | grep -i texture` shows AU registration
- [ ] No crashes on plugin load/close

---

## Risk Notes

1. **ANIRA FetchContent may fail or be slow** — First time downloading, ~200MB+ with ONNX Runtime. If it fails, check network, try pinning to specific commit instead of tag.
2. **ANIRA shared library distribution** — ANIRA builds as shared lib (.dylib). May need post-build copy to plugin bundle Frameworks/ directory. Handle in CMake with `install()` or `add_custom_command()`.
3. **Binary data namespace collision** — Using separate `ModelData` namespace for ONNX files and default `BinaryData` namespace for UI. Verify both accessible from PluginProcessor/Editor.
4. **ANIRA CMake variables may conflict with JUCE** — FetchContent brings in ANIRA's own CMake targets. If conflicts arise, use `EXCLUDE_FROM_ALL` or isolate with `FetchContent_GetProperties()`.
