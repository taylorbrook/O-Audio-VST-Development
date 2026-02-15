# Stage 1: Foundation - Execution Plan

**Plugin:** O-TextureForge
**Stage:** 1 (Foundation)
**Created:** 2026-02-14
**Reference Plugins:** O-GrainScatter (WebView pattern), O-Bells (IS_SYNTH bus config)

---

## Goal

Create the plugin shell: CMakeLists.txt build system, PluginProcessor with 12 APVTS parameters (output-only stereo bus, empty processBlock), PluginEditor with WebView placeholder, and JUCE bridge JS files. Plugin must load in DAW, expose all parameters for automation, and display a placeholder WebView UI.

---

## Tasks

### Task 1: Create CMakeLists.txt
**Files:** `plugins/O-TextureForge/CMakeLists.txt`
**Depends on:** None

Configure JUCE 8 plugin build:
- `juce_add_plugin(OuariconTextureForge ...)` with:
  - `PLUGIN_CODE OuTF` (verified unique)
  - `IS_SYNTH TRUE` (instrument, appears in DAW synth browser)
  - `NEEDS_MIDI_INPUT TRUE` (three MIDI modes)
  - `NEEDS_WEB_BROWSER TRUE` + `NEEDS_WEBVIEW2 TRUE` (WebView scatter plot)
  - `FORMATS VST3 AU Standalone`
  - `PRODUCT_NAME "O-TextureForge${OUARICON_DEV_SUFFIX}"`
  - `VERSION 1.0.0`
- `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)`
- `target_sources` for PluginProcessor.cpp and PluginEditor.cpp
- `target_link_libraries` with all standard JUCE modules (audio_basics through gui_extra) + recommended flags
- `juce_generate_juce_header(OuariconTextureForge)`
- `juce_add_binary_data(OuariconTextureForge_UIResources ...)` for index.html + bridge JS
- Compile definitions: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`
- Conditional licensing block (matching O-GrainScatter pattern)

---

### Task 2: Create PluginProcessor header
**Files:** `plugins/O-TextureForge/Source/PluginProcessor.h`
**Depends on:** None

- Class `TextureForgeProcessor : public juce::AudioProcessor`
- Constructor with output-only stereo bus: `BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)`
- APVTS member: `juce::AudioProcessorValueTreeState parameters`
- Static `createParameterLayout()` method
- 12 cached `std::atomic<float>*` parameter pointers (ENERGY, BRIGHTNESS, TEXTURE, POSITION, GRAIN_DENSITY, GRAIN_SIZE, SCATTER_X, SCATTER_Y, VARIATION, CROSSFADE, OUTPUT_GAIN, MIDI_MODE)
- VizSnapshot struct (empty shell for Stage 2+)
- Double-buffer arrays + atomic write index for viz
- `getVizSnapshot()` const getter
- `double currentSampleRate` member (stored in prepareToPlay for Stage 2)
- `getAPVTS()` public accessor
- Standard AudioProcessor overrides (prepareToPlay, releaseResources, processBlock, createEditor, getStateInformation, setStateInformation, etc.)
- `acceptsMidi() → true`, `producesMidi() → false`, `isMidiEffect() → false`

---

### Task 3: Create PluginProcessor implementation
**Files:** `plugins/O-TextureForge/Source/PluginProcessor.cpp`
**Depends on:** Task 2

- Constructor: initialize APVTS with `createParameterLayout()`, cache all 12 parameter pointers via `getRawParameterValue()`
- `createParameterLayout()` with all 12 parameters:
  - `ENERGY`: Float 0.0-1.0, default 0.5
  - `BRIGHTNESS`: Float 0.0-1.0, default 0.5
  - `TEXTURE`: Float 0.0-1.0, default 0.5
  - `POSITION`: Float 0.0-1.0, default 0.0
  - `GRAIN_DENSITY`: Int 1-64, default 8
  - `GRAIN_SIZE`: Float 10-500, default 50, **skew 0.5** (logarithmic)
  - `SCATTER_X`: Float 0.0-1.0, default 0.5
  - `SCATTER_Y`: Float 0.0-1.0, default 0.5
  - `VARIATION`: Float 0.0-1.0, default 0.2
  - `CROSSFADE`: Float 0-100, default 50
  - `OUTPUT_GAIN`: Float -60 to +12, default 0
  - `MIDI_MODE`: Choice ["Pitch-Mapped", "Trigger + Modulate", "Generative Drone"], default index 2
- `prepareToPlay()`: store `currentSampleRate = sampleRate`
- `processBlock()`: `ScopedNoDenormals`, clear output buffer (silence stub)
- `getStateInformation()` / `setStateInformation()`: standard APVTS XML pattern
- `createEditor()`: return new `TextureForgeEditor(*this)`

---

### Task 4: Create PluginEditor header
**Files:** `plugins/O-TextureForge/Source/PluginEditor.h`
**Depends on:** Task 2

- Class `TextureForgeEditor : public juce::AudioProcessorEditor, private juce::Timer`
- Forward declare `TextureForgeProcessor`
- `std::unique_ptr<juce::WebBrowserComponent> webView`
- `TextureForgeProcessor& processorRef`
- `timerCallback()` override (no-op in Stage 1, starts at 30Hz)
- `getResource()` method returning `std::optional<juce::WebBrowserComponent::Resource>`
- `resized()` override
- Standard constructor/destructor
- Comment noting relay/attachment member declaration order for Stage 3

---

### Task 5: Create PluginEditor implementation
**Files:** `plugins/O-TextureForge/Source/PluginEditor.cpp`
**Depends on:** Tasks 3, 4

- Constructor:
  - Set editor size (900 x 600, matching scatter-dominant UI concept)
  - Create WebBrowserComponent with:
    - `Backend::webview2`
    - `withUserDataFolder()` to temp directory (`OTextureForge_WebView`)
    - `withNativeIntegrationEnabled()`
    - `withResourceProvider()` (guarded by `JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE`)
  - Add and make visible webView
  - Navigate to `getResourceProviderRoot()`
  - `startTimerHz(30)`
- `resized()`: set webView bounds to full editor area
- `timerCallback()`: empty (Stage 1 no-op, populated in Stage 3)
- `getResource()`: map URLs to BinaryData:
  - `/` and `/index.html` → `BinaryData::index_html`
  - `/js/juce/index.js` → `BinaryData::index_js`
  - `/js/juce/check_native_interop.js` → `BinaryData::check_native_interop_js`

---

### Task 6: Copy JUCE bridge JavaScript files
**Files:**
- `plugins/O-TextureForge/Source/ui/public/js/juce/index.js` (copy from O-GrainScatter)
- `plugins/O-TextureForge/Source/ui/public/js/juce/check_native_interop.js` (copy from O-GrainScatter)
**Depends on:** None

Copy the two JUCE WebView bridge files from O-GrainScatter. These are standard framework files shared across all Ouaricon plugins.

---

### Task 7: Create placeholder index.html
**Files:** `plugins/O-TextureForge/Source/ui/public/index.html`
**Depends on:** None

Minimal HTML5 placeholder:
- `<script type="module" src="./js/juce/index.js"></script>` (ES6 module loading)
- Body: "O-TextureForge — Loading UI..."
- Charset UTF-8, viewport meta

---

### Task 8: Build and verify
**Files:** None (build verification)
**Depends on:** Tasks 1-7

- Run CMake configure + Ninja build: `OuariconTextureForge_VST3` and `OuariconTextureForge_AU`
- Fix any compilation errors
- Clear AU cache, install VST3 + AU to system folders
- Verify plugin loads (standalone or DAW)

---

## File Manifest

| File | Action | Description |
|------|--------|-------------|
| `plugins/O-TextureForge/CMakeLists.txt` | Create | JUCE 8 build config (IS_SYNTH, WebView, MIDI) |
| `plugins/O-TextureForge/Source/PluginProcessor.h` | Create | Processor header (APVTS, viz snapshot, bus config) |
| `plugins/O-TextureForge/Source/PluginProcessor.cpp` | Create | Processor impl (12 params, empty processBlock) |
| `plugins/O-TextureForge/Source/PluginEditor.h` | Create | Editor header (WebView, Timer) |
| `plugins/O-TextureForge/Source/PluginEditor.cpp` | Create | Editor impl (WebView setup, resource provider) |
| `plugins/O-TextureForge/Source/ui/public/index.html` | Create | Placeholder HTML |
| `plugins/O-TextureForge/Source/ui/public/js/juce/index.js` | Copy | JUCE bridge (from O-GrainScatter) |
| `plugins/O-TextureForge/Source/ui/public/js/juce/check_native_interop.js` | Copy | JUCE interop check (from O-GrainScatter) |

---

## Success Criteria

- [ ] Plugin builds without errors (VST3 + AU + Standalone)
- [ ] Plugin appears in DAW instrument browser (not effects)
- [ ] GUI window opens showing "Loading UI..." placeholder
- [ ] All 12 parameters visible in DAW automation list
- [ ] Parameter changes from DAW reflected in APVTS (verified via debugger or automation readback)
- [ ] macOS AU validates with `auval -v aumu OuTF OuAu` (or dev codes)
- [ ] Standalone app opens without crash

---

## Critical Patterns to Follow

1. **Member declaration order** (Stage 3 prep): when adding relays/attachments later, declare Relays → WebView → Attachments
2. **WebView2 static linking**: `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (from MEMORY.md)
3. **User data folder**: temp directory for WebView2 (from MEMORY.md)
4. **ES6 modules**: `type="module"` for script imports (juce8-critical-patterns.md #21)
5. **Resource provider**: explicit URL-to-BinaryData mapping (juce8-critical-patterns.md #8)
6. **Output-only bus**: no `isBusesLayoutSupported` override needed for IS_SYNTH (from O-Bells pattern)
7. **GRAIN_SIZE skew**: 0.5 skew factor for logarithmic distribution (finer control 10-100ms)

---

**END OF PLAN**
