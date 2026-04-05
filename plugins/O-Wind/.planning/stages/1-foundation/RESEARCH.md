# O-Wind Stage 1: Foundation - Research

**Researched:** 2026-04-04
**Domain:** JUCE 8.0.4 plugin scaffolding (CMake, APVTS, WebView, SynthesiserVoice)
**Confidence:** HIGH

## Summary

Stage 1 is pure scaffolding -- CMakeLists.txt, PluginProcessor with APVTS, PluginEditor with WebView shell, placeholder HTML. No DSP. The established patterns from O-Bowed (most recent physical modeling synth, same architecture) provide a 1:1 template. Every pattern below is extracted directly from working reference plugins in this codebase.

The parameter-spec-draft.md lists 14 parameter IDs but claims "Total Parameters: 13." This discrepancy must be resolved before implementation. The CONTEXT.md locks "13 automatable" as final. Additionally, the tuning module adds 5 more APVTS parameters (tuning_masterTune, tuning_tuningMode, tuning_octaveStretch, tuning_pitchBendRange, tuning_temperamentPreset) following the same pattern as O-Bowed.

**Primary recommendation:** Clone the O-Bowed Stage 1 structure exactly, substituting O-Wind parameter IDs/ranges. Use `camelCase` parameter IDs (matching O-Bowed convention: `breathPressure` not `breath_pressure`).

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
| Decision | Value |
|----------|-------|
| Plugin ID | `ouaricon-wind` |
| Plugin Code | `OWnd` |
| Manufacturer Code | `${OUARICON_MANUFACTURER_CODE}` (shared) |
| Plugin Type | Synth (`IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`) |
| Audio Buses | Output-only stereo |
| WebView UI | `NEEDS_WEB_BROWSER TRUE` |
| Window Size | 900x600 |
| Parameters | 13 automatable -- treat `parameter-spec-draft.md` as final |
| Voice Model | `juce::SynthesiserVoice` (same as O-Lyrica, O-Bowed) |
| Tuning Module | `modules/tuning/scala-tuning-engine` (existing, separate tab in UI) |
| Oversampling | Deferred to Stage 2 |

### Scope
- CMakeLists.txt with all JUCE modules + tuning module link
- PluginProcessor.h/cpp with APVTS (all 13 params), empty processBlock
- PluginEditor.h/cpp with WebView shell (900x600)
- Placeholder index.html
- Builds and loads in DAW as instrument (no audio yet)
</user_constraints>

## Project Constraints (from CLAUDE.md)

- **Build cache clearing:** Must clear AU cache, remove old binaries, install fresh after every build
- **Plugin structure:** Plugins in `plugins/[PluginName]/`, build output in `build/plugins/[PluginName]/[PluginName]_artefacts/Release/`
- **WebView2 static linking:** MUST use `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` when `NEEDS_WEBVIEW2 TRUE` is set
- **Resource provider paths:** Receives bare paths (e.g., `/`, `/index.html`), NOT full URLs
- **Windows WebView2 user data folder:** Always set `withUserDataFolder()` to temp directory
- **JUCE version:** 8.0.4
- **Handoff protocol:** Must present two-step handoff after stage completion

## CMakeLists.txt Pattern (from O-Bowed)

**Confidence: HIGH** -- Directly extracted from `plugins/O-Bowed/CMakeLists.txt`

### Exact Structure

```cmake
cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-Wind - Physical Modeling Flute Synthesizer
juce_add_plugin(O-Wind
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OWnd
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Wind${OUARICON_DEV_SUFFIX}"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)
```

### Critical Flags
- `NEEDS_WEB_BROWSER TRUE` -- enables juce_gui_extra WebView
- `NEEDS_WEBVIEW2 TRUE` -- links `WebView2LoaderStatic.lib` on Windows
- `EDITOR_WANTS_KEYBOARD_FOCUS FALSE` -- WebView handles its own keyboard focus
- `FORMATS VST3 AU Standalone` -- standard format set

### Source Files

```cmake
target_sources(O-Wind
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/FluteSynthSound.h
        # Tuning module files (referenced from shared module)
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)
```

Note: O-Bowed references tuning module cpp files from the shared module path (`${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/`) rather than copying them locally. The headers are found via `target_include_directories`.

### Include Paths

```cmake
target_include_directories(O-Wind
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
)
```

### JUCE Module Linking

```cmake
target_link_libraries(O-Wind
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
```

This is the exact same module set as O-Bowed. `juce_dsp` is included even at Stage 1 (needed for Stage 2 DSP).

### JuceHeader Generation

```cmake
juce_generate_juce_header(O-Wind)
```

**Must come AFTER `target_link_libraries`** -- JUCE 8 requirement.

### Licensing Module (Conditional)

```cmake
if(OUARICON_LICENSING)
    ouaricon_add_module(O-Wind licensing)
    target_compile_definitions(O-Wind PRIVATE OUARICON_LICENSING_ENABLED=1)
    target_link_libraries(O-Wind PRIVATE juce::juce_cryptography)
endif()
```

### WebView UI Resources (BinaryData)

```cmake
juce_add_binary_data(O-Wind_UIResources
    SOURCES
        Resources/ui/index.html
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css
)

target_link_libraries(O-Wind
    PRIVATE
        O-Wind_UIResources
)
```

### Compile Definitions

```cmake
target_compile_definitions(O-Wind
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
```

**Critical:** `JUCE_WEB_BROWSER=1` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` are BOTH required. The static linking flag auto-defines `JUCE_USE_WIN_WEBVIEW2=1`.

## APVTS Parameter Pattern (from O-Bowed)

**Confidence: HIGH** -- Directly extracted from `plugins/O-Bowed/Source/PluginProcessor.cpp`

### Parameter ID Convention

O-Bowed uses **camelCase** for parameter IDs: `bowSpeed`, `bowPressure`, `bodyMaterial`, `outputLevel`, `infiniteSustain`, etc.

The parameter-spec-draft.md uses **snake_case** (`breath_pressure`, `air_column`). **Convert to camelCase** for consistency with the codebase convention.

### Parameter Creation Pattern

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout OWindAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // AudioParameterFloat with ParameterID version 1, NormalisableRange, default, optional label
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "breathPressure", 1 },
        "Breath Pressure",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // For dB parameters with skewed range:
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outputLevel", 1 },
        "Output Level",
        juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    return layout;
}
```

### Key Details
- `juce::ParameterID { "id", 1 }` -- version 1 for all parameters (version streaming)
- `NormalisableRange<float>(min, max, step)` -- step of 0.01f for most 0-1 params
- No parameter groups used in O-Bowed Stage 1 (flat layout)
- All 13 plugin params + 5 tuning module params in same layout

### Tuning Module Parameters

From `modules/tuning/scala-tuning-engine/snippets/parameters.cpp`, these 5 additional parameters are added to the same `createParameterLayout()`:

```cpp
// tuning_masterTune (400-480 Hz, default 440)
// tuning_tuningMode (choice: 12-TET/Custom/MTS-ESP)
// tuning_octaveStretch (0.95-1.25, default 1.0)
// tuning_pitchBendRange (1-48 semitones, default 2)
// tuning_temperamentPreset (choice: 11 presets)
```

However, O-Bowed's Stage 1 only includes 2 tuning parameters (`referencePitch` and `tuningSystem`) -- not the full 5 from the snippet. This suggests the plugin may customize which tuning params to expose. For O-Wind, follow O-Bowed's simpler pattern (2 tuning params) unless full tuning panel integration is needed at Stage 1.

### O-Wind Parameter ID Mapping (spec -> camelCase)

| Spec ID (snake_case) | Implementation ID (camelCase) | Range | Default |
|----------------------|-------------------------------|-------|---------|
| breath_pressure | breathPressure | 0.0-1.0 | 0.5 |
| embouchure | embouchure | 0.0-1.0 | 0.5 |
| breath_noise | breathNoise | 0.0-1.0 | 0.15 |
| tone_color | toneColor | 0.0-1.0 | 0.5 |
| air_column | airColumn | 0.0-1.0 | 0.5 |
| jet_reflection | jetReflection | -1.0-1.0 | 0.5 |
| end_reflection | endReflection | -1.0-1.0 | 0.5 |
| vibrato_rate | vibratoRate | 2.0-8.0 | 5.0 |
| vibrato_depth | vibratoDepth | 0.0-1.0 | 0.3 |
| width | width | 0.0-2.0 | 1.0 |
| output_level | outputLevel | -60.0-12.0 | 0.0 |
| infinite_sustain | infiniteSustain | 0.0-1.0 | 0.0 |
| reversed_jet | reversedJet | 0.0-1.0 | 0.0 |
| sub_harmonics | subHarmonics | 0.0-1.0 | 0.0 |

**That is 14 IDs.** The spec says 13. See Open Questions below.

## PluginProcessor Pattern (from O-Bowed)

**Confidence: HIGH** -- Directly extracted from `plugins/O-Bowed/Source/PluginProcessor.h/cpp`

### Header Structure

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "FluteSynthSound.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"

class OWindAudioProcessor : public juce::AudioProcessor
{
public:
    OWindAudioProcessor();
    ~OWindAudioProcessor() override;

    // Standard overrides...
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Wind"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Standard program stubs
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }
    TuningEngine* getTuningEngine() { return &tuningEngine; }

private:
    juce::AudioProcessorValueTreeState parameters;
    juce::Synthesiser synthesiser;
    TuningEngine tuningEngine;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OWindAudioProcessor)
};
```

### Constructor -- Output-Only Stereo Bus

```cpp
OWindAudioProcessor::OWindAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    synthesiser.addSound(new FluteSynthSound());
}
```

### processBlock -- Stage 1 (empty)

```cpp
void OWindAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);
    buffer.clear();
}
```

### State Save/Restore

```cpp
void OWindAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OWindAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}
```

## SynthesiserSound Pattern (from O-Bowed)

**Confidence: HIGH** -- Directly extracted from `plugins/O-Bowed/Source/BowedStringSound.h`

```cpp
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class FluteSynthSound : public juce::SynthesiserSound
{
public:
    FluteSynthSound() {}
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
```

Simple catch-all. No SynthesiserVoice implementation at Stage 1.

## PluginEditor WebView Pattern (from O-Bowed)

**Confidence: HIGH** -- Directly extracted from `plugins/O-Bowed/Source/PluginEditor.h/cpp`

### CRITICAL: Member Declaration Order

Members are destroyed in **reverse declaration order**. The order MUST be:

1. **Relays FIRST** (no dependencies)
2. **WebView SECOND** (depends on relays via `.withOptionsFrom()`)
3. **Attachments LAST** (depend on both relays and webView)

Attachments call `evaluateJavascript()` during destruction, so WebView must still exist when attachments are destroyed.

### Relay + WebView + Attachment Pattern

For each parameter:
1. Declare `std::unique_ptr<juce::WebSliderRelay>` (member)
2. Create relay in constructor: `relay = std::make_unique<juce::WebSliderRelay>("paramId")`
3. Register with WebView: `.withOptionsFrom(*relay)`
4. Create attachment after WebView: `attachment = std::make_unique<juce::WebSliderParameterAttachment>(*apvts.getParameter("paramId"), *relay, nullptr)`

### WebView Construction

```cpp
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(juce::File::getSpecialLocation(
                    juce::File::SpecialLocationType::tempDirectory)))
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](const juce::String& url) {
            return getResource(url);
        })
        // Register all relay options here...
        .withOptionsFrom(*breathPressureRelay)
        // ... etc
);
```

### Resource Provider Pattern

```cpp
std::optional<juce::WebBrowserComponent::Resource>
OWindAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeResource = [](const char* data, int size, const char* mimeType) {
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte>(
                reinterpret_cast<const std::byte*>(data),
                reinterpret_cast<const std::byte*>(data) + size
            ),
            juce::String(mimeType)
        };
    };

    if (url == "/" || url == "/index.html")
        return makeResource(BinaryData::index_html,
                           BinaryData::index_htmlSize,
                           "text/html");

    if (url == "/js/juce/index.js")
        return makeResource(BinaryData::index_js,
                           BinaryData::index_jsSize,
                           "text/javascript");

    if (url == "/js/juce/check_native_interop.js")
        return makeResource(BinaryData::check_native_interop_js,
                           BinaryData::check_native_interop_jsSize,
                           "text/javascript");

    if (url == "/js/tuning-panel.js")
        return makeResource(BinaryData::tuningpanel_js,
                           BinaryData::tuningpanel_jsSize,
                           "text/javascript");

    if (url == "/css/tuning-panel.css")
        return makeResource(BinaryData::tuningpanel_css,
                           BinaryData::tuningpanel_cssSize,
                           "text/css");

    DBG("Resource not found: " + url);
    return std::nullopt;
}
```

**Key:** URLs are **bare paths** (e.g., `/`, `/index.html`), NOT full URLs. Do NOT strip scheme/host.

### Navigation

```cpp
webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
setSize(900, 600);
```

## Placeholder index.html Pattern (from O-Bowed)

**Confidence: HIGH**

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>O-Wind</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background: #1a1a1a;
            color: #e0e0e0;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            display: flex;
            align-items: center;
            justify-content: center;
            height: 100vh;
            overflow: hidden;
        }
        .container { text-align: center; }
        h1 { font-size: 2.5rem; font-weight: 300; letter-spacing: 0.1em; color: #ffffff; margin-bottom: 0.5rem; }
        .subtitle { font-size: 0.9rem; color: #888; letter-spacing: 0.05em; }
        .stage { margin-top: 1.5rem; font-size: 0.75rem; color: #555; }
    </style>
</head>
<body>
    <div class="container">
        <h1>O-Wind</h1>
        <div class="subtitle">Physical Modeling Flute Synthesizer</div>
        <div class="stage">Stage 1 - Foundation Shell</div>
    </div>
    <script src="/js/juce/index.js"></script>
    <script src="/js/juce/check_native_interop.js"></script>
</body>
</html>
```

## Tuning Module Integration Pattern

**Confidence: HIGH** -- Extracted from O-Bowed CMakeLists.txt + tuning module snippets

### CMake Integration

O-Bowed does NOT use `ouaricon_add_module()` for the tuning engine. Instead it directly references source files and include paths:

```cmake
# In target_sources:
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp

# In target_include_directories:
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp

# In juce_add_binary_data:
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css
```

### C++ Files in Module

Located at `modules/tuning/scala-tuning-engine/cpp/`:
- TuningEngine.h/cpp
- ScaleGenerator.h/cpp
- EmbeddedTunings.h/cpp
- TuningExporter.h/cpp

### JS/CSS Files

- `modules/tuning/scala-tuning-engine/js/tuning-panel.js`
- `modules/tuning/scala-tuning-engine/snippets/tuning-panel.css`

### JUCE Bridge JS Files

Must be copied to `Resources/ui/js/juce/`:
- `index.js` -- JUCE WebView bridge (SliderState, ComboBoxState, ToggleState, getNativeFunction, getBackendResourceAddress)
- `check_native_interop.js` -- Android compatibility + Backend class definition

These are the same files used across all WebView plugins. Copy from O-Bowed.

## File/Directory Structure for O-Wind

```
plugins/O-Wind/
    CMakeLists.txt
    Source/
        PluginProcessor.h
        PluginProcessor.cpp
        PluginEditor.h
        PluginEditor.cpp
        FluteSynthSound.h
    Resources/
        ui/
            index.html
            js/
                juce/
                    index.js
                    check_native_interop.js
```

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| WebView <-> C++ parameter sync | Custom JS bridge | JUCE WebSliderRelay + WebSliderParameterAttachment | Thread-safe, handles normalization, drag gestures, version-streamed |
| Tuning/microtonal support | Custom frequency tables | scala-tuning-engine module | Handles Scala files, MTS-ESP, 11+ temperaments, octave stretch |
| WebView2 on Windows | Dynamic DLL loading | Static linking flag | MUST use `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` |
| Binary resource serving | File-based resource loading | `juce_add_binary_data` + resource provider | Embedded in plugin binary, no external files needed |
| CMake boilerplate | Custom cmake setup | `OuariconModules.cmake` include | Provides shared manufacturer code, licensing, module system |

## Common Pitfalls

### Pitfall 1: Member Declaration Order in Editor
**What goes wrong:** Crash on plugin close (use-after-free)
**Why it happens:** Attachments call `evaluateJavascript()` during destruction, but WebView is already destroyed
**How to avoid:** Declare in order: Relays -> WebView -> Attachments. Destroyed in reverse.
**Warning signs:** Crash only on close, not during operation

### Pitfall 2: WebView2 Static Linking Flag Missing
**What goes wrong:** Blank WebView on Windows
**Why it happens:** Plugin tries to dynamically load `WebView2Loader.dll` which isn't distributed
**How to avoid:** Always set `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` AND `NEEDS_WEBVIEW2 TRUE`
**Warning signs:** Works on macOS, blank on Windows

### Pitfall 3: Resource Provider URL Stripping
**What goes wrong:** All resource lookups fail, "Frame load interrupted" error
**Why it happens:** Assuming resource provider receives full URLs and stripping scheme/host
**How to avoid:** Compare directly against bare paths: `if (url == "/" || url == "/index.html")`
**Warning signs:** WebView loads but shows blank/error

### Pitfall 4: JuceHeader Generation Order
**What goes wrong:** Build error -- JuceHeader.h not found or incomplete
**Why it happens:** `juce_generate_juce_header()` called before `target_link_libraries()`
**How to avoid:** Always call `juce_generate_juce_header()` AFTER `target_link_libraries()`
**Warning signs:** Compilation errors about missing JUCE module headers

### Pitfall 5: Parameter ID Mismatch Between C++ and JS
**What goes wrong:** Parameters don't sync between WebView UI and processor
**Why it happens:** Parameter ID string in relay doesn't match APVTS parameter ID
**How to avoid:** Use identical strings in `juce::ParameterID{"id", 1}`, `WebSliderRelay("id")`, and JS `getSliderState("id")`
**Warning signs:** Console warning "Creating SliderState for 'X', which is unknown to the backend"

### Pitfall 6: Windows WebView2 User Data Folder
**What goes wrong:** WebView2 silently fails to construct, falls back to IE backend -> blank page
**Why it happens:** DAW plugin host may deny WebView2 access to default user data location
**How to avoid:** Always set `.withUserDataFolder(File::getSpecialLocation(File::tempDirectory))`
**Warning signs:** Works standalone, fails in DAW on Windows

### Pitfall 7: snake_case Parameter IDs
**What goes wrong:** Inconsistency with codebase convention
**Why it happens:** Using spec IDs directly without converting
**How to avoid:** Convert all `snake_case` from spec to `camelCase` for APVTS IDs
**Warning signs:** Mixed convention across plugins

## Open Questions

1. **Parameter Count: 13 or 14?**
   - What we know: The parameter-spec-draft.md lists 14 parameter IDs (breath_pressure, embouchure, breath_noise, tone_color, air_column, jet_reflection, end_reflection, vibrato_rate, vibrato_depth, width, output_level, infinite_sustain, reversed_jet, sub_harmonics) but states "Total Parameters: 13"
   - CONTEXT.md locks "13 automatable"
   - What's unclear: Which parameter should be dropped, or is the "13" count a typo?
   - Recommendation: Count is likely a typo in the spec -- implement all 14. The actual table is more authoritative than the summary count. If the user wants 13, they need to specify which one to drop.

2. **Tuning Parameters: 2 or 5?**
   - What we know: O-Bowed Stage 1 only has 2 tuning params (`referencePitch`, `tuningSystem`). The tuning module snippets define 5 (`tuning_masterTune`, `tuning_tuningMode`, `tuning_octaveStretch`, `tuning_pitchBendRange`, `tuning_temperamentPreset`).
   - What's unclear: Should O-Wind follow O-Bowed's simplified 2-param approach or the full snippet?
   - Recommendation: Follow O-Bowed exactly (2 tuning params) for Stage 1 consistency. Full tuning integration happens in UI stage.

## Validation Architecture

### Build Verification
| Property | Value |
|----------|-------|
| Framework | CMake + Ninja (JUCE 8.0.4) |
| Build command | `ninja O-Wind_VST3 O-Wind_AU` |
| Quick verify | `auval -a \| grep -i wind` |
| Full verify | `pluginval --strictness-level 10 --validate O-Wind.vst3` |

### Stage 1 Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command |
|--------|----------|-----------|-------------------|
| COMPAT-01 | Passes pluginval validation | integration | `pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/O-Wind.vst3` |

### Wave 0 Gaps
- No unit test framework configured (JUCE plugins in this project rely on build success + pluginval + DAW testing)
- pluginval binary must be installed locally

## Sources

### Primary (HIGH confidence)
- `plugins/O-Bowed/CMakeLists.txt` -- CMake structure, compile definitions, binary data, tuning integration
- `plugins/O-Bowed/Source/PluginProcessor.h/cpp` -- APVTS pattern, parameter layout, bus config, state save/restore
- `plugins/O-Bowed/Source/PluginEditor.h/cpp` -- WebView shell, relay/attachment pattern, resource provider, member order
- `plugins/O-Bowed/Source/BowedStringSound.h` -- SynthesiserSound stub pattern
- `plugins/O-Bowed/Resources/ui/index.html` -- Placeholder HTML structure
- `modules/tuning/scala-tuning-engine/snippets/` -- All tuning integration snippets (parameters, native functions, persistence, header)
- `modules/cmake/OuariconModules.cmake` -- Module system interface
- `CMakeLists.txt` (root) -- Shared variables (OUARICON_COMPANY_NAME, OUARICON_MANUFACTURER_CODE, OUARICON_DEV_SUFFIX)

### Secondary (MEDIUM confidence)
- `plugins/O-Formant/CMakeLists.txt` -- Confirms same JUCE module set pattern (without WebView)
- `plugins/O-Lyrica/Source/PluginProcessor.h` -- Confirms TuningEngine integration pattern with SynthesiserVoice

## Metadata

**Confidence breakdown:**
- CMake structure: HIGH -- exact copy from O-Bowed with name substitution
- APVTS parameters: HIGH -- pattern identical, only IDs/ranges change
- WebView shell: HIGH -- identical pattern to O-Bowed
- Tuning integration: HIGH -- same module, same CMake approach
- Parameter count: MEDIUM -- spec inconsistency (13 vs 14)

**Research date:** 2026-04-04
**Valid until:** 2026-05-04 (stable patterns, JUCE 8.0.4 is current)
