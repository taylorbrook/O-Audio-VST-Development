# Stage 1: Foundation - Research Findings

**Plugin:** O-TextureForge
**Stage:** 1 (Foundation)
**Date:** 2026-02-13
**Focus:** CMakeLists.txt, PluginProcessor, PluginEditor, WebView setup

---

## 1. CMakeLists.txt Configuration

### Reference: O-GrainScatter (closest match - WebView + granular)

**Key elements to replicate:**

```cmake
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(OuariconTextureForge
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuTF                          # Unique, verified not in use
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-TextureForge${OUARICON_DEV_SUFFIX}"
    VERSION 1.0.0
    IS_SYNTH TRUE                             # Instrument plugin
    NEEDS_MIDI_INPUT TRUE                     # Three MIDI modes
    NEEDS_WEB_BROWSER TRUE                    # WebView scatter plot
    NEEDS_WEBVIEW2 TRUE                       # Windows WebView2 static lib
)
```

**Differences from O-GrainScatter:**
- `IS_SYNTH TRUE` (O-GrainScatter is an effect, not a synth)
- `NEEDS_MIDI_INPUT TRUE` (required for three MIDI modes)
- No audio input bus (instrument generates audio, doesn't process it)
- Plugin code: `OuTF` (O-GrainScatter uses `OuGS`)

### Verified Plugin Code Uniqueness

Checked all 20+ plugins - `OuTF` is not used by any existing plugin.

### Binary Data Resources

```cmake
juce_add_binary_data(OuariconTextureForge_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

target_link_libraries(OuariconTextureForge
    PRIVATE
        OuariconTextureForge_UIResources
)
```

### Compile Definitions (standard Ouaricon WebView config)

```cmake
target_compile_definitions(OuariconTextureForge
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
```

### Build Variables (from root CMakeLists.txt)

- `OUARICON_COMPANY_NAME` - "Ouaricon Audio" (release) / "Ouaricon Audio Development" (dev)
- `OUARICON_MANUFACTURER_CODE` - `OuAu` (release) / `OuDv` (dev)
- `OUARICON_DEV_SUFFIX` - "" (release) / "-dev" (dev)
- `OUARICON_LICENSING` - OFF (local dev) / ON (GitHub Actions)

### JUCE Modules Required

Standard Ouaricon set:
- `juce_audio_basics`
- `juce_audio_devices`
- `juce_audio_formats` (file loading in Stage 2)
- `juce_audio_plugin_client`
- `juce_audio_processors` (APVTS)
- `juce_audio_utils`
- `juce_core`
- `juce_data_structures`
- `juce_dsp` (FFT in Stage 2)
- `juce_events`
- `juce_graphics`
- `juce_gui_basics`
- `juce_gui_extra` (WebBrowserComponent)

Plus recommended flags:
- `juce::juce_recommended_config_flags`
- `juce::juce_recommended_lto_flags`
- `juce::juce_recommended_warning_flags`

---

## 2. Bus Configuration (Output-Only Instrument)

### Reference: O-Bells, O-Marimba (both IS_SYNTH TRUE)

Both use output-only bus:
```cpp
: AudioProcessor(BusesProperties()
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
```

**No `isBusesLayoutSupported()` override needed** - neither O-Bells nor O-Marimba implement it. JUCE's default behavior for output-only synths is sufficient.

### O-TextureForge Pattern

Same output-only stereo bus. No input bus needed since audio comes from loaded files (corpus), not from DAW audio routing.

---

## 3. APVTS Parameter Layout

### Parameter Types Needed

| Parameter | Type | JUCE Class | Pattern Reference |
|-----------|------|-----------|-------------------|
| ENERGY | Float 0-1 | `AudioParameterFloat` | O-GrainScatter `density` |
| BRIGHTNESS | Float 0-1 | `AudioParameterFloat` | O-GrainScatter `density` |
| TEXTURE | Float 0-1 | `AudioParameterFloat` | O-GrainScatter `density` |
| POSITION | Float 0-1 | `AudioParameterFloat` | O-GrainScatter `density` |
| GRAIN_DENSITY | Int 1-64 | `AudioParameterInt` | O-GrainScatter `repeats` |
| GRAIN_SIZE | Float 10-500 (skewed) | `AudioParameterFloat` | O-GrainScatter `grain_size` (skew 0.5) |
| SCATTER_X | Float 0-1 | `AudioParameterFloat` | O-GrainScatter `density` |
| SCATTER_Y | Float 0-1 | `AudioParameterFloat` | O-GrainScatter `density` |
| VARIATION | Float 0-1 | `AudioParameterFloat` | O-GrainScatter `density` |
| CROSSFADE | Float 0-100 | `AudioParameterFloat` | O-Tremolo `DEPTH_PARAM` |
| OUTPUT_GAIN | Float -60 to +12 | `AudioParameterFloat` | O-Comp `output_gain` |
| MIDI_MODE | Choice 0-2 | `AudioParameterChoice` | O-GrainScatter `scale` |

### APVTS Constructor Pattern

```cpp
juce::AudioProcessorValueTreeState parameters;
// In constructor:
, parameters(*this, nullptr, "Parameters", createParameterLayout())
```

### Skewed Parameter (GRAIN_SIZE)

From O-GrainScatter `grain_size`: 4th argument to `NormalisableRange` is the skew factor.
```cpp
juce::NormalisableRange<float>(10.0f, 500.0f, 0.1f, 0.5f)  // skew = 0.5 (logarithmic)
```

Skew < 1.0 gives more resolution at the low end (10-100ms), which is what we want for grain size.

### Parameter ID Convention

O-GrainScatter uses `snake_case`: `grain_size`, `dry_wet`, `pitch_random`
CONTEXT.md specifies `UPPER_SNAKE_CASE`: `ENERGY`, `GRAIN_SIZE`, `MIDI_MODE`

**Decision:** Use the IDs from CONTEXT.md (`ENERGY`, `GRAIN_SIZE`, etc.) since they're already documented and match the ARCHITECTURE.md spec. Both conventions work with JUCE.

### Cached Parameter Pointers

```cpp
std::atomic<float>* energyParam = nullptr;
// In constructor:
energyParam = parameters.getRawParameterValue("ENERGY");
```

---

## 4. PluginEditor WebView Setup

### CRITICAL: Member Declaration Order

From O-GrainScatter (production-tested pattern):

```
1. RELAYS (no dependencies) - created first, destroyed last
2. WEBVIEW (depends on relays) - created second
3. ATTACHMENTS (depend on relays + webView) - created last, destroyed first
```

C++ destroys members in REVERSE declaration order. Attachments must be destroyed BEFORE webView, which must be destroyed BEFORE relays.

### Stage 1 Simplified Pattern (No Relay Attachments Yet)

For Stage 1, the WebView is a placeholder. No parameter relays needed until Stage 3 (GUI). The editor only needs:

1. WebBrowserComponent with resource provider
2. Timer for future visualization (can start in Stage 1 as no-op)
3. Resource provider mapping index.html and bridge JS files

### WebView Construction

```cpp
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(juce::File::getSpecialLocation(
                    juce::File::SpecialLocationType::tempDirectory)
                        .getChildFile("OTextureForge_WebView")))
        .withNativeIntegrationEnabled()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
        .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
);
```

**Key patterns from MEMORY.md:**
- `withUserDataFolder()` to temp directory (Windows DAW permission issues)
- `withNativeIntegrationEnabled()` for JS↔C++ bridge
- Resource provider with conditional compilation guard

### Resource Provider

```cpp
std::optional<juce::WebBrowserComponent::Resource>
getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size);
    };

    if (url == "/" || url == "/index.html")
        return Resource{ makeVector(BinaryData::index_html, BinaryData::index_htmlSize), "text/html" };

    if (url == "/js/juce/index.js")
        return Resource{ makeVector(BinaryData::index_js, BinaryData::index_jsSize), "application/javascript" };

    if (url == "/js/juce/check_native_interop.js")
        return Resource{ makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize), "application/javascript" };

    return std::nullopt;
}
```

### URL Loading

```cpp
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif
```

This uses the platform-aware URL scheme:
- macOS/iOS/Linux: `juce://juce.backend/`
- Windows/Android: `https://juce.backend/`

---

## 5. WebView Bridge JavaScript Files

### Source Files (identical across all Ouaricon plugins)

Two files from the JUCE framework:

1. **`js/juce/index.js`** (577 lines) - ES6 module providing:
   - `getNativeFunction()` - Call C++ functions from JS
   - `getSliderState()` - Bidirectional slider sync
   - `getToggleState()` - Bidirectional toggle sync
   - `getComboBoxState()` - Bidirectional combo box sync
   - `getBackendResourceAddress()` - Platform-aware URL construction
   - `ControlParameterIndexUpdater` - MIDI learn support

2. **`js/juce/check_native_interop.js`** (146 lines) - Sets up:
   - `window.__JUCE__` global object
   - `window.__JUCE__.backend` for event communication
   - Platform detection
   - Fallback stubs when not running in JUCE

### HTML Loading Pattern

```html
<script type="module">
  import { getSliderState, getToggleState, getComboBoxState, getNativeFunction }
    from "./js/juce/index.js";
</script>
```

Uses ES6 `type="module"` (juce8-critical-patterns.md #21).

---

## 6. State Serialization

### Standard APVTS XML Pattern (all Ouaricon plugins)

```cpp
void getStateInformation(juce::MemoryBlock& destData) override
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void setStateInformation(const void* data, int sizeInBytes) override
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
```

For Stage 1, this handles all 12 parameters. In Stage 2+, additional state (file path, UMAP seed) will be appended to the XML.

---

## 7. Visualization Snapshot (Stage 1 Prep)

### Double-Buffer Pattern (from O-GrainScatter)

```cpp
struct VizSnapshot {
    // Stage 1: empty struct, ready for Stage 2+ data
};

std::array<VizSnapshot, 2> vizSnapshots {};
std::atomic<int> vizWriteIndex { 0 };

// Getter (called from GUI thread, lock-free)
const VizSnapshot& getVizSnapshot() const {
    return vizSnapshots[static_cast<size_t>(1 - vizWriteIndex.load(std::memory_order_acquire))];
}
```

### Timer Callback (30Hz, no-op in Stage 1)

```cpp
void timerCallback() override
{
    // Stage 1: no visualization data to send
    // Stage 3 will populate this with grain activity + cursor position
}
```

Start timer in constructor: `startTimerHz(30);`

---

## 8. processBlock Stub (Stage 1)

### Output Silence (no DSP yet)

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer (silence - DSP comes in Stage 2)
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    buffer.clear();
}
```

Note: `ScopedNoDenormals` is the modern replacement for manual denormal prevention.

---

## 9. Placeholder HTML (Stage 1)

### Minimal WebView Content

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>O-TextureForge</title>
    <script type="module" src="./js/juce/index.js"></script>
</head>
<body>
    <div>O-TextureForge — Loading UI...</div>
</body>
</html>
```

Simple placeholder. Full Ouaricon Naturalist aesthetic comes in Stage 3.

---

## 10. Pitfalls & Critical Patterns

### From juce8-critical-patterns.md

1. **#8 Resource Provider URL Mapping** - Explicit URL-to-BinaryData mapping in `getResource()`. Never rely on automatic resolution.

2. **#21 ES6 Module Loading** - Use `type="module"` for script imports. Required for the JUCE bridge `index.js`.

3. **#22 IS_SYNTH TRUE** - Required for instrument plugins. Without it, plugin appears in effects list.

### From MEMORY.md

4. **WebView2 Static Linking** - `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` MUST be set when `NEEDS_WEBVIEW2 TRUE`. Otherwise WebView silently shows blank on Windows.

5. **WebView2 User Data Folder** - Must use temp directory to avoid DAW permission issues.

6. **Silent IE Fallback** - If WebView2 fails, JUCE silently falls back to IE (no resource provider support). Handled by proper WebView2 configuration.

### From Codebase Analysis

7. **Member Declaration Order** - Relays → WebView → Attachments. Critical for correct destruction order.

8. **No `isBusesLayoutSupported` for synths** - O-Bells and O-Marimba (both IS_SYNTH) don't override it. Output-only bus is sufficient.

9. **`juce_generate_juce_header()`** - Required in CMakeLists for `#include <JuceHeader.h>` to work.

---

## 11. File Structure (Stage 1 Output)

```
plugins/O-TextureForge/
├── CMakeLists.txt
└── Source/
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp
    ├── PluginEditor.h
    ├── PluginEditor.cpp
    └── ui/public/
        ├── index.html
        └── js/juce/
            ├── index.js          (copy from O-GrainScatter)
            └── check_native_interop.js  (copy from O-GrainScatter)
```

---

## 12. Modules Available for Stage 1

Checked `modules/` directory. No modules needed for Stage 1 (pure shell + parameters). Potential modules for later stages:
- `persistence` - preset manager (Stage 4)
- `licensing` - if commercial release

---

## Summary: No Unknowns

Stage 1 is straightforward with clear reference patterns from O-GrainScatter (WebView), O-Bells/O-Marimba (IS_SYNTH bus config), and O-Comp/O-Tremolo (parameter types). All patterns are production-tested in this codebase.

**Ready for plan phase.**
