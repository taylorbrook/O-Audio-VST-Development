# O-Reed Stage 1: Foundation - Research

**Researched:** 2026-04-04
**Domain:** JUCE 8 plugin project scaffolding (CMake, APVTS, WebView, MPESynthesiser, Tuning module)
**Confidence:** HIGH

## Summary

Stage 1 creates the buildable plugin shell: CMakeLists.txt, processor with all 35 APVTS parameters, editor with WebView, placeholder HTML, MPESynthesiser with placeholder voice, and tuning module link. Every pattern needed exists in sibling plugins (O-Bowed for WebView + tuning, O-Formant for MPESynthesiser + MPESynthesiserVoice).

**Primary recommendation:** Clone O-Bowed CMake/WebView patterns verbatim. Clone O-Formant MPESynthesiser/voice patterns. Substitute O-Reed's 35 parameters from `parameter-spec-draft.md`.

---

## User Constraints (from CONTEXT.md)

| Decision | Value |
|----------|-------|
| Plugin ID | `ouaricon-reed` |
| Plugin Code | `ORed` |
| Manufacturer Code | `${OUARICON_MANUFACTURER_CODE}` (shared) |
| Plugin Type | Synth (`IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`) |
| Audio Buses | Output-only stereo |
| WebView UI | `NEEDS_WEB_BROWSER TRUE` |
| Window Size | 900x600 |
| Parameters | 35 automatable — treat `parameter-spec-draft.md` as final |
| Voice Model | `juce::MPESynthesiserVoice` with `enableLegacyMode()` |
| Tuning Module | `modules/tuning/scala-tuning-engine` (existing, separate tab in UI) |
| Oversampling | Deferred to Stage 2 (Phase 3.5) |

---

## 1. CMakeLists.txt Pattern

**Confidence:** HIGH — verified from O-Bowed CMakeLists.txt (most recent synth plugin with WebView + tuning)

### Template

Source: `plugins/O-Bowed/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(O-Reed
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE ORed
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Reed${OUARICON_DEV_SUFFIX}"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)
```

### Key observations

- **`include(OuariconModules.cmake)` BEFORE `juce_add_plugin`** — standard for all plugins.
- **PLUGIN_CODE = `ORed`** — locked in CONTEXT.md. 4 chars, unique across codebase.
- **NEEDS_WEBVIEW2 TRUE** — CRITICAL for Windows static linking.
- **EDITOR_WANTS_KEYBOARD_FOCUS FALSE** — prevents DAW keyboard focus stealing.

### Source files (Stage 1)

```cmake
target_sources(O-Reed
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/ReedWindSound.h
        # Tuning module (referenced from shared module)
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)
```

**Note:** ReedWindSound.h is a placeholder `juce::SynthesiserSound` — required for `juce::Synthesiser::addSound()`. O-Reed uses MPESynthesiser which doesn't need this, but following the same defensive pattern as O-Bowed for consistency. Actually, MPESynthesiser does NOT use SynthesiserSound — it manages voices directly. Skip ReedWindSound.h entirely. See Section 3 for voice model.

### Include paths

```cmake
target_include_directories(O-Reed
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
)
```

### JUCE modules (matching O-Bowed)

```cmake
target_link_libraries(O-Reed
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

### CRITICAL: juce_generate_juce_header

```cmake
# MUST come after target_link_libraries
juce_generate_juce_header(O-Reed)
```

### Licensing module (conditional)

```cmake
if(OUARICON_LICENSING)
    ouaricon_add_module(O-Reed licensing)
    target_compile_definitions(O-Reed PRIVATE OUARICON_LICENSING_ENABLED=1)
    target_link_libraries(O-Reed PRIVATE juce::juce_cryptography)
endif()
```

### BinaryData for WebView resources

```cmake
juce_add_binary_data(O-Reed_UIResources
    SOURCES
        Resources/ui/index.html
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css
)

target_link_libraries(O-Reed
    PRIVATE
        O-Reed_UIResources
)
```

### Compile definitions

```cmake
target_compile_definitions(O-Reed
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
```

**CRITICAL:** `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` is mandatory per MEMORY.md.

---

## 2. APVTS Parameter Setup

**Confidence:** HIGH — verified from O-Bowed and O-Formant PluginProcessor.cpp

### Pattern: Static createParameterLayout()

```cpp
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Constructor:
OReedAudioProcessor::OReedAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Create voices, enableLegacyMode, etc.
}
```

### Parameter types needed (35 total)

**AudioParameterFloat** (27 parameters):
- BREATH_PRESSURE, EMBOUCHURE, REED_HARDNESS, BORE_CHARACTER
- REED_OPENING, BELL_SIZE, AIR_NOISE, DOUBLE_REED, BORE_DIAMETER
- REED_MASS, REED_DAMPING, MOUTHPIECE_VOL, TONE_HOLE_CUTOFF, REGISTER_HOLE, BORE_LENGTH
- VIBRATO_DEPTH, VIBRATO_RATE
- GROWL_AMOUNT, FLUTTER_TONGUE, SUBTONE, ATTACK_CHIFF
- INFINITE_SUSTAIN, REVERSE_BORE, DRONE_PITCH, FEEDBACK_PATH
- REFERENCE_PITCH
- OUTPUT_GAIN

**AudioParameterChoice** (6 parameters):
- INSTRUMENT_PRESET (21 choices: 0-20)
- BORE_PROFILE (2 choices: Simple, Multi-segment)
- VIBRATO_SOURCE (3 choices: Lip, Breath, Throat)
- TUNING_SYSTEM (3 choices: Scala/TUN, MTS-ESP, 12-TET)
- POLY_MODE (2 choices: Monophonic, Polyphonic)
- OVERSAMPLING (2 choices: 2x, 4x)

**AudioParameterBool** (1 parameter):
- DUAL_BORE (default: false)

**AudioParameterInt** (1 parameter):
- MAX_VOICES (1-16, default 8) — non-automatable settings parameter

### Parameter ID conventions

Use camelCase IDs matching the codebase convention:
- `breathPressure`, `embouchure`, `reedHardness`, `boreCharacter`
- `instrumentPreset`, `boreProfile`, `vibratoSource`, etc.
- `dualBore`, `dronePitch`, `feedbackPath`
- `referencePitch`, `tuningSystem`, `polyMode`, `oversampling`
- `maxVoices`, `outputGain`

### INSTRUMENT_PRESET choice values

```cpp
juce::StringArray {
    "Bb Clarinet", "Bass Clarinet",
    "Alto Saxophone", "Tenor Saxophone", "Soprano Saxophone", "Baritone Saxophone",
    "Oboe", "English Horn", "Bassoon",
    "Duduk", "Shehnai", "Suona", "Hichiriki", "Zurna", "Piri",
    "Arghul", "Launeddas", "Mijwiz",
    "Glass Reed", "Metal Wind", "Impossible Bore"
}
```

21 presets (index 0-20), default 0 (Bb Clarinet).

### Skew factors

- TONE_HOLE_CUTOFF (200-8000 Hz): use skew ~0.3f for finer control at low frequencies
- VIBRATO_RATE (1-10 Hz): linear is fine
- REFERENCE_PITCH (220-880 Hz): linear is fine
- OUTPUT_GAIN (-60 to 12 dB): linear is fine
- All 0-1 params: no skew needed (linear)
- DRONE_PITCH (-24 to 24 semitones): linear is fine

---

## 3. Voice Model: MPESynthesiser

**Confidence:** HIGH — verified from O-Formant PluginProcessor.cpp and FormantVoice.h

### Key difference from O-Bowed

O-Bowed uses `juce::Synthesiser` + `SynthesiserVoice` + `SynthesiserSound`. O-Reed uses `juce::MPESynthesiser` + `MPESynthesiserVoice` (no SynthesiserSound needed).

### MPESynthesiser setup pattern (from O-Formant)

```cpp
// Header:
juce::MPESynthesiser synthesiser;

// Constructor:
OReedAudioProcessor::OReedAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Create 16 voice instances (maximum polyphony)
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new ReedWindVoice(i);
        voice->setAPVTS(&parameters);
        synthesiser.addVoice(voice);
    }

    // Enable legacy mode for standard MIDI (channel range 1-16, pitchbend +/-2 semitones)
    synthesiser.enableLegacyMode(2, juce::Range<int>(1, 17));
}
```

**IMPORTANT:** `enableLegacyMode()` must be called AFTER `addVoice()`.

### MPESynthesiserVoice pure virtuals (6 methods)

Stage 1 creates a skeleton ReedWindVoice with silent stubs:

```cpp
class ReedWindVoice : public juce::MPESynthesiserVoice
{
public:
    ReedWindVoice(int voiceIndex = 0);
    void setAPVTS(juce::AudioProcessorValueTreeState* apvts);

    // MPESynthesiserVoice pure virtual overrides
    void noteStarted() override;
    void noteStopped(bool allowTailOff) override;
    void notePressureChanged() override;
    void notePitchbendChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         int startSample, int numSamples) override;

private:
    juce::AudioProcessorValueTreeState* parameters = nullptr;
    // Cached APVTS parameter pointers (set once in setAPVTS)
    // ... all 35 atomic<float>* pointers ...
};
```

Stage 1 stubs: all methods are no-ops except `renderNextBlock` which outputs silence. Parameter pointers are cached in `setAPVTS()` for audio-thread-safe access.

### No SynthesiserSound needed

`juce::MPESynthesiser` manages voice allocation directly via MPE zones / legacy mode. No `addSound()` call needed (unlike `juce::Synthesiser`).

### processBlock pattern

```cpp
void OReedAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synthesiser.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
}
```

---

## 4. Editor Pattern: WebView with Relay/Attachment

**Confidence:** HIGH — verified from O-Bowed PluginEditor.h/cpp

### CRITICAL: Member declaration order

```
1. RELAYS FIRST (no dependencies)
2. WEBVIEW SECOND (depends on relays via .withOptionsFrom())
3. ATTACHMENTS LAST (depend on both relays and webView)
```

Members are destroyed in REVERSE declaration order. Attachments call `evaluateJavascript()` during destruction, so WebView must still exist.

### Relay types for O-Reed

- **27 Float params** → `WebSliderRelay` + `WebSliderParameterAttachment`
- **6 Choice params** → `WebComboBoxRelay` + `WebComboBoxParameterAttachment`
- **1 Bool param (DUAL_BORE)** → `WebToggleButtonRelay` + `WebToggleButtonParameterAttachment`
- **1 Int param (MAX_VOICES)** → `WebSliderRelay` + `WebSliderParameterAttachment` (same as O-Bowed's int params)

**Total:** 27 slider relays + 6 combobox relays + 1 toggle relay + 1 slider relay (int) = 35 relays

### WebToggleButtonRelay pattern

Verified in JUCE 8.0.4 source (`juce_gui_extra/misc/juce_WebControlRelays.h`) and codebase template (`.claude/templates/code-snippets/parameter-binding/toggle-relay.yaml`):

```cpp
// Header:
std::unique_ptr<juce::WebToggleButtonRelay> dualBoreRelay;
std::unique_ptr<juce::WebToggleButtonParameterAttachment> dualBoreAttachment;

// Relay creation:
dualBoreRelay = std::make_unique<juce::WebToggleButtonRelay>("dualBore");

// WebView options:
.withOptionsFrom(*dualBoreRelay)

// Attachment creation:
dualBoreAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
    *apvts.getParameter("dualBore"), *dualBoreRelay, nullptr);
```

### WebView setup (from O-Bowed)

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
        .withOptionsFrom(*relay1)
        .withOptionsFrom(*relay2)
        // ... all 35 relays ...
);
```

### Resource provider (from O-Bowed)

```cpp
std::optional<juce::WebBrowserComponent::Resource>
OReedAudioProcessorEditor::getResource(const juce::String& url)
{
    // URL is bare path (e.g., "/", "/index.html")
    // Do NOT strip scheme — it's already a bare path
    if (url == "/" || url == "/index.html")
        return makeResource(BinaryData::index_html, BinaryData::index_htmlSize, "text/html");

    if (url == "/js/juce/index.js")
        return makeResource(BinaryData::index_js, BinaryData::index_jsSize, "text/javascript");

    if (url == "/js/juce/check_native_interop.js")
        return makeResource(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize, "text/javascript");

    // Tuning panel
    if (url == "/js/tuning-panel.js")
        return makeResource(BinaryData::tuningpanel_js, BinaryData::tuningpanel_jsSize, "text/javascript");

    if (url == "/css/tuning-panel.css")
        return makeResource(BinaryData::tuningpanel_css, BinaryData::tuningpanel_cssSize, "text/css");

    return std::nullopt;
}
```

---

## 5. Tuning Module Integration

**Confidence:** HIGH — verified from O-Bowed CMakeLists.txt and integration checklist

### CMake pattern (reference from shared module)

```cmake
# Source files
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp

# Include path
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp

# BinaryData
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css
```

### Processor integration

```cpp
// Header:
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"

// Member:
TuningEngine tuningEngine;

// Public accessor:
TuningEngine* getTuningEngine() { return &tuningEngine; }
```

### Note on O-Bowed tuning pattern

O-Bowed's CMakeLists.txt references .cpp files from the shared module path and includes the shared module cpp directory. The .h files in O-Bowed's Source/ are from the shared module included via the target_include_directories path. Follow this same pattern — no need to copy headers into Source/.

---

## 6. Placeholder HTML

**Confidence:** HIGH — identical pattern across all WebView plugins

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>O-Reed</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background: #1a1a1a; color: #e0e0e0;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            display: flex; align-items: center; justify-content: center;
            height: 100vh; overflow: hidden;
        }
        .container { text-align: center; }
        h1 { font-size: 2.5rem; font-weight: 300; letter-spacing: 0.1em; color: #ffffff; margin-bottom: 0.5rem; }
        .subtitle { font-size: 0.9rem; color: #888; letter-spacing: 0.05em; }
        .stage { margin-top: 1.5rem; font-size: 0.75rem; color: #555; }
    </style>
</head>
<body>
    <div class="container">
        <h1>O-Reed</h1>
        <div class="subtitle">Physical Modeling Reed Wind Synthesizer</div>
        <div class="stage">Stage 1 - Foundation Shell</div>
    </div>
    <script src="/js/juce/index.js"></script>
    <script src="/js/juce/check_native_interop.js"></script>
</body>
</html>
```

### JUCE bridge files

Copy `index.js` and `check_native_interop.js` from `plugins/O-Bowed/Resources/ui/js/juce/` to `plugins/O-Reed/Resources/ui/js/juce/`. These are JUCE framework files, identical across all plugins.

---

## 7. State Persistence

**Confidence:** HIGH — identical pattern in O-Bowed and O-Formant

```cpp
void OReedAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OReedAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}
```

---

## 8. Files to Create

| File | Purpose |
|------|---------|
| `plugins/O-Reed/CMakeLists.txt` | Build config with all dependencies |
| `plugins/O-Reed/Source/PluginProcessor.h` | OReedAudioProcessor with MPESynthesiser + APVTS + TuningEngine |
| `plugins/O-Reed/Source/PluginProcessor.cpp` | 35-param layout, 16 voices, enableLegacyMode, state serialization |
| `plugins/O-Reed/Source/PluginEditor.h` | Editor with 35 relays + WebView + 35 attachments |
| `plugins/O-Reed/Source/PluginEditor.cpp` | Relay creation, WebView setup, attachment creation, resource provider |
| `plugins/O-Reed/Source/ReedWindVoice.h` | MPESynthesiserVoice skeleton with cached parameter pointers |
| `plugins/O-Reed/Source/ReedWindVoice.cpp` | Silent stubs for all 7 pure virtual methods |
| `plugins/O-Reed/Resources/ui/index.html` | Placeholder WebView HTML |
| `plugins/O-Reed/Resources/ui/js/juce/index.js` | JUCE WebView bridge (copy from O-Bowed) |
| `plugins/O-Reed/Resources/ui/js/juce/check_native_interop.js` | Native interop check (copy from O-Bowed) |

---

## 9. Potential Pitfalls

### MPESynthesiser vs Synthesiser
- MPESynthesiser does NOT use `SynthesiserSound`. Don't create a `ReedWindSound.h`.
- MPESynthesiser voice allocation is zone-based. `enableLegacyMode()` maps all MIDI channels to one zone.
- `enableLegacyMode()` must be called AFTER `addVoice()` calls.

### WebToggleButtonRelay for DUAL_BORE
- Verified in JUCE 8.0.4 source. Uses `WebToggleButtonParameterAttachment` (3 args: parameter, relay, undoManager).
- Must use `AudioParameterBool` (not AudioParameterFloat) for the APVTS parameter.

### 35 relays is a lot of boilerplate
- Each relay needs: declaration (header), creation (constructor step 1), .withOptionsFrom (step 2), attachment (step 3).
- Group by section in comments for readability (matching O-Bowed pattern).

### Resource provider receives bare paths
- `withResourceProvider` callback receives paths like `/`, `/index.html`, `/js/juce/index.js`.
- Do NOT strip scheme — it's already a bare path. Direct equality checks only.

### isBusesLayoutSupported
- O-Formant implements this to enforce stereo output + no input. Recommended for O-Reed too.

### MAX_VOICES parameter
- Int parameter, range 1-16, default 8.
- Not automatable per CONTEXT.md, but JUCE AudioParameterInt is automatable by default.
- Can mark non-automatable with `juce::AudioParameterIntAttributes().withAutomatable(false)` if needed, but this is a Stage 2+ concern.

---

## 10. Reference Plugins

| Plugin | Pattern to Reuse |
|--------|-----------------|
| **O-Bowed** | CMakeLists.txt, WebView editor (relay/attachment pattern), resource provider, tuning module CMake link, BinaryData, placeholder HTML |
| **O-Formant** | MPESynthesiser constructor, enableLegacyMode(2, Range<int>(1,17)), FormantVoice as MPESynthesiserVoice template, processBlock with renderNextBlock, isBusesLayoutSupported |
