# O-Bowed Stage 1: Foundation - Research

**Researched:** 2026-04-04
**Domain:** JUCE 8 plugin project scaffolding (CMake, APVTS, WebView, Tuning module)
**Confidence:** HIGH

## Summary

Stage 1 creates the empty-but-buildable plugin shell: CMakeLists.txt, processor with all 22 APVTS parameters, editor with WebView, placeholder HTML, and tuning module link. Every pattern needed already exists in sibling plugins (O-Lyrica, O-Bells, O-Prism). This research documents the exact patterns to replicate.

The codebase is consistent across all synth plugins. The three-phase editor pattern (Relays -> WebView -> Attachments), the `createParameterLayout()` static function, and the `BusesProperties().withOutput()` constructor are identical in every reference plugin. O-Bowed should follow these verbatim.

**Primary recommendation:** Clone the O-Lyrica/O-Bells patterns exactly, substituting O-Bowed's 22 parameters from `parameter-spec-draft.md`.

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

| Decision | Value |
|----------|-------|
| Plugin ID | `ouaricon-bowed` |
| Plugin Code | `OBwd` |
| Manufacturer Code | `${OUARICON_MANUFACTURER_CODE}` (shared) |
| Plugin Type | Synth (`IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`) |
| Audio Buses | Output-only stereo |
| WebView UI | `NEEDS_WEB_BROWSER TRUE` |
| Window Size | 900x600 |
| Parameters | 22 automatable -- treat `parameter-spec-draft.md` as final |
| Voice Model | `juce::SynthesiserVoice` (same as O-Lyrica) |
| Tuning Module | `modules/tuning/scala-tuning-engine` (existing, separate tab in UI) |
| Oversampling | Deferred to Stage 2 |

### Tuning Integration

- Link existing `modules/tuning/scala-tuning-engine` via CMake
- C++ files: TuningEngine.h/cpp, ScaleGenerator.h/cpp, EmbeddedTunings.h/cpp, TuningExporter.h/cpp
- JS/CSS: tuning-panel.js, tuning-panel.css (separate tab)
- Integration snippets available in `modules/tuning/scala-tuning-engine/snippets/`
- Follow same pattern as O-Lyrica, O-Bells

### Scope for Stage 1

- CMakeLists.txt with all JUCE modules + tuning module link
- PluginProcessor.h/cpp with APVTS (all 22 params), empty processBlock
- PluginEditor.h/cpp with WebView shell (900x600)
- Placeholder index.html
- Builds and loads in DAW as instrument (no audio yet)
</user_constraints>

---

## 1. CMakeLists.txt Pattern

**Confidence:** HIGH -- verified from O-Lyrica, O-Bells, O-Prism CMakeLists.txt

### Template

Source: `plugins/O-Lyrica/CMakeLists.txt`, `plugins/O-Bells/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(O-Bowed
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OBwd
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Bowed${OUARICON_DEV_SUFFIX}"
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

- **`include(OuariconModules.cmake)` BEFORE `juce_add_plugin`** -- O-Lyrica and O-Prism both do this. O-Bells includes it after, but before is the standard.
- **PLUGIN_CODE = `OBwd`** -- locked in CONTEXT.md. 4 chars, unique across the codebase.
- **NEEDS_WEBVIEW2 TRUE** -- CRITICAL for Windows. Without this, WebView2 static lib is not linked and the plugin falls back to IE with a blank page.
- **EDITOR_WANTS_KEYBOARD_FOCUS FALSE** -- O-Bells sets this. Prevents the plugin from stealing keyboard focus from the DAW.

### Source files (Stage 1)

```cmake
target_sources(O-Bowed
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/BowedStringSound.h
        Source/TuningEngine.cpp
        Source/ScaleGenerator.cpp
        Source/EmbeddedTunings.cpp
        Source/TuningExporter.cpp
)
```

**Note:** Tuning module files are copied from `modules/tuning/scala-tuning-engine/cpp/` into `Source/` per the integration checklist. This is the established pattern (O-Lyrica, O-Bells, O-Prism all do this).

### Include paths

```cmake
target_include_directories(O-Bowed
    PRIVATE
        Source
)
```

### JUCE modules (full set matching O-Lyrica)

```cmake
target_link_libraries(O-Bowed
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
juce_generate_juce_header(O-Bowed)
```

This is noted in O-Bells CMakeLists as a JUCE 8 requirement. Without it, `#include <JuceHeader.h>` fails.

### Licensing module (conditional)

```cmake
if(OUARICON_LICENSING)
    ouaricon_add_module(O-Bowed licensing)
    target_compile_definitions(O-Bowed PRIVATE OUARICON_LICENSING_ENABLED=1)
    target_link_libraries(O-Bowed PRIVATE juce::juce_cryptography)
endif()
```

### BinaryData for WebView resources

```cmake
juce_add_binary_data(O-Bowed_UIResources
    SOURCES
        Resources/ui/index.html
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        Resources/ui/js/tuning-panel.js
        Resources/ui/css/tuning-panel.css
)

target_link_libraries(O-Bowed
    PRIVATE
        O-Bowed_UIResources
)
```

**Note:** The JUCE bridge files (`index.js`, `check_native_interop.js`) are copied from another plugin's Resources (they are JUCE framework files, identical across all plugins). The tuning-panel.js and tuning-panel.css come from `modules/tuning/scala-tuning-engine/js/` and `snippets/`.

### Compile definitions

```cmake
target_compile_definitions(O-Bowed
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
```

**CRITICAL:** `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` is mandatory per MEMORY.md. Without it, Windows builds silently fail to show WebView content.

---

## 2. APVTS Parameter Setup

**Confidence:** HIGH -- verified from O-Lyrica and O-Bells PluginProcessor.cpp

### Pattern: Static createParameterLayout()

All plugins use a static member function that returns the layout, called in the constructor initializer list:

```cpp
// In PluginProcessor.h:
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// In PluginProcessor.cpp constructor:
OBowedAudioProcessor::OBowedAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // ... voice setup, etc.
}
```

### Parameter types used in O-Bowed

Based on `parameter-spec-draft.md`, O-Bowed needs 3 parameter types:

**AudioParameterFloat** (18 parameters):

```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "bowSpeed", 1 },
    "Bow Speed",
    juce::NormalisableRange<float>(0.02f, 2.0f, 0.01f),
    0.2f,
    "m/s"
));
```

The 4th argument to NormalisableRange is the skew factor. Use 0.5f for parameters that need finer control at low values (e.g., BRIGHTNESS 20-20000 Hz should be heavily skewed).

**AudioParameterInt** (2 parameters: STRING_COUNT, SYMPATHETIC_COUNT):

```cpp
// Source: plugins/O-Bells/Source/PluginProcessor.cpp line 218
layout.add(std::make_unique<juce::AudioParameterInt>(
    juce::ParameterID { "stringCount", 1 },
    "String Count",
    1,       // min
    4,       // max
    1        // default
));
```

**AudioParameterChoice** (1 parameter: TUNING_SYSTEM):

```cpp
layout.add(std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { "tuningSystem", 1 },
    "Tuning System",
    juce::StringArray { "Scala/TUN", "MTS-ESP", "12-TET" },
    2  // Default: 12-TET (index 2)
));
```

### Complete parameter list (22 params from spec)

| ID | Name | Type | Range | Default | Notes |
|----|------|------|-------|---------|-------|
| `bowSpeed` | Bow Speed | Float | 0.02-2.0 | 0.2 | m/s |
| `bowPressure` | Bow Pressure | Float | 0.01-5.0 | 0.5 | N |
| `bowPosition` | Bow Position | Float | 0.02-0.30 | 0.12 | unitless |
| `rosin` | Rosin | Float | 0.0-1.0 | 0.5 | |
| `bodyMaterial` | Material | Float | 0.0-1.0 | 0.4 | |
| `bodySize` | Size | Float | 0.0-1.0 | 0.5 | |
| `brightness` | Brightness | Float | 20.0-20000.0 | 8000.0 | Hz, use skew ~0.25 |
| `stringCount` | String Count | Int | 1-4 | 1 | |
| `stringTuning1` | String 1 Tuning | Float | -2400.0-2400.0 | 0.0 | cents |
| `stringTuning2` | String 2 Tuning | Float | -2400.0-2400.0 | 0.0 | cents |
| `stringTuning3` | String 3 Tuning | Float | -2400.0-2400.0 | 0.0 | cents |
| `stringTuning4` | String 4 Tuning | Float | -2400.0-2400.0 | 0.0 | cents |
| `sympatheticAmount` | Sympathetic Amount | Float | 0.0-1.0 | 0.0 | |
| `sympatheticCount` | Sympathetic Strings | Int | 0-12 | 0 | |
| `width` | Stereo Width | Float | 0.0-2.0 | 1.0 | |
| `outputLevel` | Output Level | Float | -60.0-12.0 | 0.0 | dB |
| `infiniteSustain` | Infinite Sustain | Float | 0.0-1.0 | 0.0 | |
| `reversedFriction` | Reversed Friction | Float | 0.0-1.0 | 0.0 | |
| `subHarmonics` | Sub-Harmonics | Float | 0.0-1.0 | 0.0 | |
| `referencePitch` | Reference Pitch | Float | 220.0-880.0 | 440.0 | Hz |
| `tuningSystem` | Tuning System | Choice | 0-2 | 2 | Scala/MTS/12TET |

### Skew recommendations

- `brightness` (20-20000 Hz): skew ~0.25 (log-like, finer at low end)
- `bowSpeed` (0.02-2.0): skew ~0.5 (finer at low values)
- `bowPressure` (0.01-5.0): skew ~0.5
- `outputLevel` (-60 to +12 dB): no skew needed (linear is fine for dB)
- All 0-1 range params: no skew (linear)

---

## 3. WebView Editor Shell

**Confidence:** HIGH -- verified from O-Lyrica and O-Bells PluginEditor.cpp/.h

### CRITICAL: Three-Phase Lifetime Pattern

Every plugin in the codebase uses this exact pattern. Destruction order is reverse of declaration order in the header file.

```
Header declaration order (top to bottom):
  1. Relays (WebSliderRelay, WebComboBoxRelay)
  2. WebBrowserComponent
  3. Attachments (WebSliderParameterAttachment, WebComboBoxParameterAttachment)

Destruction order (bottom to top):
  3. Attachments destroyed first (calls evaluateJavascript -- webView must exist)
  2. WebView destroyed second (relays still exist)
  1. Relays destroyed last
```

### Editor Header Pattern

Source: `plugins/O-Lyrica/Source/PluginEditor.h`

```cpp
class OBowedAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OBowedAudioProcessorEditor(OBowedAudioProcessor&);
    ~OBowedAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OBowedAudioProcessor& processorRef;

    // === CRITICAL ORDER: Relays -> WebView -> Attachments ===

    // 1. RELAYS FIRST
    std::unique_ptr<juce::WebSliderRelay> bowSpeedRelay;
    // ... (one per float/int parameter)
    std::unique_ptr<juce::WebComboBoxRelay> tuningSystemRelay;
    // ... (one per choice parameter)

    // 2. WEBVIEW SECOND
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST
    std::unique_ptr<juce::WebSliderParameterAttachment> bowSpeedAttachment;
    // ... (one per float/int parameter)
    std::unique_ptr<juce::WebComboBoxParameterAttachment> tuningSystemAttachment;
    // ... (one per choice parameter)

    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBowedAudioProcessorEditor)
};
```

### Editor Constructor Pattern

Source: `plugins/O-Lyrica/Source/PluginEditor.cpp`, `plugins/O-Bells/Source/PluginEditor.cpp`

```cpp
OBowedAudioProcessorEditor::OBowedAudioProcessorEditor(OBowedAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. CREATE RELAYS
    bowSpeedRelay = std::make_unique<juce::WebSliderRelay>("bowSpeed");
    // ... all relays

    // 2. CREATE WEBVIEW
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
            .withOptionsFrom(*bowSpeedRelay)
            // ... all relay options
            .withOptionsFrom(*tuningSystemRelay)
    );

    addAndMakeVisible(webView.get());

    // 3. CREATE ATTACHMENTS
    auto& apvts = processorRef.getAPVTS();

    bowSpeedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bowSpeed"), *bowSpeedRelay, nullptr);
    // ... all float/int attachments use WebSliderParameterAttachment

    tuningSystemAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("tuningSystem"), *tuningSystemRelay, nullptr);
    // ... all choice attachments use WebComboBoxParameterAttachment

    // NAVIGATE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // SET SIZE (900x600 per CONTEXT.md)
    setSize(900, 600);
}
```

### WebView Options -- Key Points

- **`.withBackend(Backend::webview2)`** -- Forces WebView2 on Windows (not IE).
- **`.withWinWebView2Options(...withUserDataFolder(tempDirectory))`** -- CRITICAL per MEMORY.md. Without this, WebView2 may be denied access to default user data folder in plugin hosts.
- **`.withNativeIntegrationEnabled()`** -- Required for native function calls from JS.
- **`.withResourceProvider(...)`** -- Serves files from BinaryData. Resource provider receives bare paths (e.g., `/`, `/index.html`), NOT full URLs.
- **`.withOptionsFrom(*relay)`** -- Registers each relay with the WebView. MUST be called for every relay.

### Relay type mapping

| Parameter Type | Relay | Attachment |
|---------------|-------|------------|
| AudioParameterFloat | `WebSliderRelay` | `WebSliderParameterAttachment` |
| AudioParameterInt | `WebSliderRelay` | `WebSliderParameterAttachment` |
| AudioParameterChoice | `WebComboBoxRelay` | `WebComboBoxParameterAttachment` |
| AudioParameterBool | `WebToggleButtonRelay` | `WebToggleButtonParameterAttachment` |

For O-Bowed Stage 1: 20 WebSliderRelay + 1 WebComboBoxRelay = 21 relays and 21 attachments. (20 float/int params use WebSliderRelay; 1 choice param uses WebComboBoxRelay.)

### Resource Provider Pattern

Source: `plugins/O-Bells/Source/PluginEditor.cpp` line 857

```cpp
std::optional<juce::WebBrowserComponent::Resource>
OBowedAudioProcessorEditor::getResource(const juce::String& url)
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

    // HTML
    if (url == "/" || url == "/index.html")
        return makeResource(BinaryData::index_html,
                           BinaryData::index_htmlSize,
                           "text/html");

    // JUCE Bridge
    if (url == "/js/juce/index.js")
        return makeResource(BinaryData::index_js,
                           BinaryData::index_jsSize,
                           "text/javascript");

    if (url == "/js/juce/check_native_interop.js")
        return makeResource(BinaryData::check_native_interop_js,
                           BinaryData::check_native_interop_jsSize,
                           "text/javascript");

    // Tuning panel
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

**CRITICAL per MEMORY.md:** The resource provider receives PATHS, not full URLs. Compare directly against `"/"`, `"/index.html"`, etc. Do NOT strip scheme/host.

### paint() and resized()

```cpp
void OBowedAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);  // WebView handles all painting
}

void OBowedAudioProcessorEditor::resized()
{
    if (webView)
        webView->setBounds(getLocalBounds());
}
```

---

## 4. Tuning Module Integration

**Confidence:** HIGH -- verified from integration checklist and O-Bells/O-Lyrica implementations

### Integration Steps (from `modules/tuning/scala-tuning-engine/snippets/INTEGRATION-CHECKLIST.md`)

1. **Copy C++ files** from `modules/tuning/scala-tuning-engine/cpp/` to `Source/`:
   - `TuningEngine.h`, `TuningEngine.cpp`
   - `ScaleGenerator.h`, `ScaleGenerator.cpp`
   - `EmbeddedTunings.h`, `EmbeddedTunings.cpp`
   - `TuningExporter.h`, `TuningExporter.cpp`

2. **Copy JS/CSS** from module:
   - `modules/tuning/scala-tuning-engine/js/tuning-panel.js` -> `Resources/ui/js/tuning-panel.js`
   - `modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` -> `Resources/ui/css/tuning-panel.css`

3. **Copy JUCE bridge files** from another plugin (they are framework files):
   - `Resources/ui/js/juce/index.js`
   - `Resources/ui/js/juce/check_native_interop.js`

4. **Add to CMakeLists.txt** -- already covered in section 1.

### Processor header additions

```cpp
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"

// Members:
TuningEngine tuningEngine;
```

### Usage in processBlock (Stage 2+)

```cpp
double freq = tuningEngine.getFrequency(midiNoteNumber);
```

### Note on Stage 1 scope

For Stage 1, the tuning module C++ files are included and compiled, but:
- No native functions registered in the editor yet (that is Stage 3 GUI work)
- No tuning-panel initialization in index.html yet
- The `TuningEngine` member exists but is not called from processBlock
- The tuning-panel.js and tuning-panel.css are included in BinaryData for when the UI needs them

---

## 5. Processor Shell

**Confidence:** HIGH -- verified from O-Lyrica PluginProcessor.cpp

### Constructor

Source: `plugins/O-Lyrica/Source/PluginProcessor.cpp` line 484

```cpp
OBowedAudioProcessor::OBowedAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize synthesiser with voices (Stage 1: empty voices)
    for (int i = 0; i < 16; ++i)
    {
        synthesiser.addVoice(new BowedStringVoice());
    }
    synthesiser.addSound(new BowedStringSound());
}
```

**Key:** Output-only bus (no `.withInput()`) because this is a synth instrument, not an effect.

### Sound class (trivial)

Source: `plugins/O-Lyrica/Source/HarpSynthSound.h`

```cpp
#pragma once
#include <JuceHeader.h>

class BowedStringSound : public juce::SynthesiserSound
{
public:
    BowedStringSound() {}
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
```

This is a header-only file (no .cpp needed). It accepts all MIDI notes on all channels.

### Voice class (Stage 1 stub)

For Stage 1, create a minimal voice that compiles but produces no audio:

```cpp
#pragma once
#include <JuceHeader.h>

class BowedStringVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<BowedStringSound*>(sound) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound*, int currentPitchWheelPosition) override
    {
        // Stage 2: implement bowed string excitation
    }

    void stopNote(float velocity, bool allowTailOff) override
    {
        clearCurrentNote();
    }

    void pitchWheelMoved(int newPitchWheelValue) override {}
    void controllerMoved(int controllerNumber, int newControllerValue) override {}

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override
    {
        // Stage 2: implement waveguide rendering
    }
};
```

### Standard processor methods

```cpp
void OBowedAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
}

void OBowedAudioProcessor::releaseResources() {}

void OBowedAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}
```

### Other required methods

```cpp
const juce::String getName() const override { return "O-Bowed"; }
bool acceptsMidi() const override { return true; }
bool producesMidi() const override { return false; }
bool isMidiEffect() const override { return false; }
double getTailLengthSeconds() const override { return 10.0; }  // Physical modeling decay

int getNumPrograms() override { return 1; }
int getCurrentProgram() override { return 0; }
void setCurrentProgram(int) override {}
const juce::String getProgramName(int) override { return {}; }
void changeProgramName(int, const juce::String&) override {}
```

### State persistence (minimal for Stage 1)

```cpp
void OBowedAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OBowedAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}
```

### APVTS accessor

```cpp
juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }
```

### Plugin instance creator

```cpp
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBowedAudioProcessor();
}
```

---

## 6. Project Structure

**Confidence:** HIGH -- verified from O-Lyrica, O-Bells directory layouts

### Directory layout for O-Bowed Stage 1

```
plugins/O-Bowed/
  CMakeLists.txt
  Source/
    PluginProcessor.h
    PluginProcessor.cpp
    PluginEditor.h
    PluginEditor.cpp
    BowedStringSound.h          # Header-only SynthesiserSound
    BowedStringVoice.h          # Stage 1 stub (header-only OK for stub)
    TuningEngine.h              # Copied from module
    TuningEngine.cpp
    ScaleGenerator.h
    ScaleGenerator.cpp
    EmbeddedTunings.h
    EmbeddedTunings.cpp
    TuningExporter.h
    TuningExporter.cpp
  Resources/
    ui/
      index.html                # Placeholder HTML
      js/
        juce/
          index.js              # JUCE bridge (framework file)
          check_native_interop.js
        tuning-panel.js         # From tuning module
      css/
        tuning-panel.css        # From tuning module
```

### Placeholder index.html

Source pattern: `plugins/O-Lyrica/Resources/ui/index.html`

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=900, height=600">
    <title>O-Bowed</title>
    <style>
        html, body {
            margin: 0;
            padding: 0;
            height: 100%;
            overflow: hidden;
            box-sizing: border-box;
        }
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            background: #1a1a2e;
            color: #e0e0e0;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', system-ui, sans-serif;
            display: flex;
            align-items: center;
            justify-content: center;
            height: 100%;
            user-select: none;
            -webkit-user-select: none;
        }
        h1 { font-size: 24px; opacity: 0.5; }
    </style>
</head>
<body>
    <h1>O-Bowed</h1>
    <script src="/js/juce/index.js" type="module"></script>
    <script src="/js/juce/check_native_interop.js"></script>
</body>
</html>
```

**Key CSS rules from O-Lyrica:**
- Use `height: 100%`, NOT `100vh` -- JUCE WebView requires percentage-based sizing.
- `overflow: hidden` -- prevents scroll bars in the plugin window.
- `user-select: none; -webkit-user-select: none;` -- native application feel.

---

## 7. Common Pitfalls

### Pitfall 1: Missing NEEDS_WEBVIEW2 or static linking define
**What goes wrong:** Plugin shows blank white page on Windows.
**Why:** Without `NEEDS_WEBVIEW2 TRUE` in juce_add_plugin AND `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` in compile definitions, JUCE falls back to IE backend which does not support resource providers.
**How to avoid:** Both must be present. Verified in MEMORY.md and all reference plugins.

### Pitfall 2: Wrong relay/attachment declaration order in header
**What goes wrong:** Crash on editor destruction. Attachments try to call evaluateJavascript() on destroyed WebView.
**Why:** C++ destroys members in reverse declaration order. If attachments are declared before webView, they are destroyed after it.
**How to avoid:** Follow the EXACT order: Relays -> WebView -> Attachments in the header.

### Pitfall 3: Resource provider comparing full URLs instead of paths
**What goes wrong:** All resource lookups return nullopt. Blank page.
**Why:** The resource provider callback receives bare paths (`/`, `/index.html`), not full URLs (`juce://juce.backend/index.html`). Stripping scheme from a bare path produces an empty string.
**How to avoid:** Direct equality: `if (url == "/" || url == "/index.html")`. Per MEMORY.md.

### Pitfall 4: Missing juce_generate_juce_header()
**What goes wrong:** Compilation error -- `JuceHeader.h` not found.
**Why:** JUCE 8 requires explicit header generation after target_link_libraries.
**How to avoid:** Add `juce_generate_juce_header(O-Bowed)` after target_link_libraries in CMakeLists.txt.

### Pitfall 5: BinaryData identifier naming
**What goes wrong:** Compilation error referencing wrong BinaryData symbols.
**Why:** JUCE's BinaryData system converts file paths to C++ identifiers by flattening paths and replacing special characters. `js/juce/index.js` becomes `index_js`, `tuning-panel.js` becomes `tuningpanel_js` (hyphen removed).
**How to avoid:** Check generated BinaryData.h after first build, or follow the identifier rules: path separators removed, hyphens removed, dots become underscores.

### Pitfall 6: WebView2 user data folder not set
**What goes wrong:** WebView2 silently fails in some DAW hosts on Windows, falls back to IE.
**Why:** Default user data folder may be access-denied in plugin host sandboxes.
**How to avoid:** Always set `.withWinWebView2Options(...withUserDataFolder(File::tempDirectory))`.

---

## 8. Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parameter management | Custom parameter system | `juce::AudioProcessorValueTreeState` | Thread-safe atomic reads, DAW automation, state persistence |
| WebView parameter sync | Manual JS message passing | `WebSliderRelay` + `WebSliderParameterAttachment` | JUCE 8 built-in two-way binding, handles all edge cases |
| Microtonal tuning | Custom tuning math | `modules/tuning/scala-tuning-engine` | Existing module with Scala/TUN/MTS-ESP support |
| Plugin state save/load | Custom serialization | APVTS `copyState()`/`replaceState()` | Handles all ValueTree serialization correctly |
| WebView resource serving | File system loading | BinaryData + resource provider | Embedded in binary, cross-platform, no file path issues |

---

## Project Constraints (from CLAUDE.md)

- **Plugin cache clearing** required after every build (AU cache, remove old binaries, install fresh)
- **Build targets:** `ninja O-Bowed_VST3 O-Bowed_AU` on macOS
- **Verify AU:** `auval -a | grep -i bowed` after install
- **Research docs** go in `research/`, not `docs/`
- **Licensing:** `/add-licensing O-Bowed {product-id}` when ready (not Stage 1)

---

## Sources

### Primary (HIGH confidence)
- `plugins/O-Lyrica/CMakeLists.txt` -- CMake pattern with tuning, WebView, synth flags
- `plugins/O-Lyrica/Source/PluginProcessor.h` -- APVTS pattern, synthesiser member, accessor methods
- `plugins/O-Lyrica/Source/PluginProcessor.cpp` -- createParameterLayout(), constructor, processBlock, state persistence
- `plugins/O-Lyrica/Source/PluginEditor.h` -- Three-phase lifetime pattern (Relays -> WebView -> Attachments)
- `plugins/O-Lyrica/Source/PluginEditor.cpp` -- Relay creation, WebView options, attachment creation, getResource()
- `plugins/O-Lyrica/Source/HarpSynthSound.h` -- Minimal SynthesiserSound pattern
- `plugins/O-Bells/CMakeLists.txt` -- Alternate CMake pattern with EDITOR_WANTS_KEYBOARD_FOCUS
- `plugins/O-Bells/Source/PluginEditor.cpp` -- getResource() pattern, attachment pattern
- `plugins/O-Prism/CMakeLists.txt` -- Tuning module + WebView pattern
- `modules/tuning/scala-tuning-engine/snippets/INTEGRATION-CHECKLIST.md` -- Tuning module integration steps
- `modules/tuning/scala-tuning-engine/snippets/parameters.cpp` -- Tuning APVTS parameters
- `modules/tuning/scala-tuning-engine/snippets/persistence.cpp` -- Tuning state save/restore
- `modules/tuning/scala-tuning-engine/snippets/processor-header.h` -- Tuning engine header additions
- `modules/cmake/OuariconModules.cmake` -- Module system CMake integration

### Memory (HIGH confidence)
- `MEMORY.md` -- WebView2 static linking, resource provider paths, user data folder

---

## Metadata

**Confidence breakdown:**
- CMakeLists.txt: HIGH -- 3 reference plugins with identical patterns
- APVTS parameters: HIGH -- parameter types and constructor pattern verified across codebase
- WebView editor: HIGH -- O-Lyrica and O-Bells use identical three-phase pattern
- Tuning integration: HIGH -- integration checklist exists with step-by-step instructions
- Pitfalls: HIGH -- documented in MEMORY.md from actual bugs encountered

**Research date:** 2026-04-04
**Valid until:** Indefinite (patterns are stable, JUCE 8.0.4 is current)
