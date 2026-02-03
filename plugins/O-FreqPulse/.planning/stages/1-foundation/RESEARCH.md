# Stage 1: Foundation - Research

**Researched:** 2026-02-03
**Domain:** JUCE Plugin Architecture, Parameter Management, CMake Build System
**Confidence:** HIGH

## Summary

This research covers the foundation stage for O-FreqPulse, focusing on creating a JUCE plugin shell with 165 parameters organized by frequency band for DAW automation visibility. The primary challenge is implementing JUCE `AudioProcessorParameterGroup` to achieve proper parameter grouping in DAW automation lists (especially VST3), while maintaining performance with a large parameter count.

Key findings:
1. JUCE `AudioProcessorParameterGroup` enables hierarchical parameter organization that VST3 hosts display correctly
2. AU/AUv3 has limitations with parameter group display (alphabetical sorting in Logic)
3. Large parameter counts (165) require cached `std::atomic<float>*` pointers for real-time performance
4. The existing O-series plugin CMake pattern provides a proven template

**Primary recommendation:** Use nested `AudioProcessorParameterGroup` with "Global", "Band 1 Sub", "Band 2 Low", "Band 3 Mid", "Band 4 High" groups. Step grid parameters should be nested within their respective band groups.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| juce_audio_processors | JUCE 8.x | AudioProcessor base, APVTS, parameter system | Required for all JUCE plugins |
| juce_dsp | JUCE 8.x | FFT, WindowingFunction, DryWetMixer | Standard DSP toolkit for spectral processing |
| juce_gui_extra | JUCE 8.x | WebBrowserComponent for UI | Enables HTML/CSS/JS-based plugin UI |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| juce_audio_basics | JUCE 8.x | SmoothedValue, buffer utilities | Always needed for audio processing |
| juce_core | JUCE 8.x | String, Array, atomic utilities | Always needed |
| juce_data_structures | JUCE 8.x | ValueTree serialization | For state save/load |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| APVTS groups | Flat parameters with prefixes | Flat works but loses DAW group display |
| WebView UI | Native JUCE components | Native is faster iteration but less flexible design |

**Installation (CMakeLists.txt):**
```cmake
target_link_libraries(O-FreqPulse
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

## Architecture Patterns

### Recommended Project Structure
```
plugins/O-FreqPulse/
├── CMakeLists.txt
├── Source/
│   ├── PluginProcessor.h
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h
│   └── PluginEditor.cpp
└── (Stage 3: Source/ui/public/...)
```

### Pattern 1: AudioParameterGroup for Hierarchical Parameters

**What:** Use `AudioProcessorParameterGroup` to create nested parameter hierarchies that DAWs display as grouped automation.

**When to use:** Any plugin with >10 parameters that benefit from logical grouping (e.g., per-band, per-voice, per-channel).

**Example:**
```cpp
// Source: JUCE documentation + forum patterns
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ===== GLOBAL GROUP =====
    auto globalGroup = std::make_unique<juce::AudioProcessorParameterGroup>(
        "global", "Global", "|");

    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1}, "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    globalGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"steps", 1}, "Steps",
        juce::StringArray{"4", "8", "16", "32"}, 2));

    globalGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"rate", 1}, "Rate",
        juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                          "1/8T", "1/16T", "1/4D", "1/8D"}, 4));

    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"swing", 1}, "Swing",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"smoothing", 1}, "Smoothing",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 5.0f));

    layout.add(std::move(globalGroup));

    // ===== PER-BAND GROUPS (4 bands) =====
    const juce::StringArray bandNames = {"Sub", "Low", "Mid", "High"};
    const float lowDefaults[] = {20.0f, 120.0f, 500.0f, 4000.0f};
    const float highDefaults[] = {120.0f, 500.0f, 4000.0f, 20000.0f};

    for (int n = 0; n < 4; ++n)
    {
        juce::String groupId = "band" + juce::String(n);
        juce::String groupName = "Band " + juce::String(n + 1) + " " + bandNames[n];

        auto bandGroup = std::make_unique<juce::AudioProcessorParameterGroup>(
            groupId, groupName, "|");

        juce::String prefix = "band" + juce::String(n) + "_";

        // Band control parameters
        bandGroup->addChild(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{prefix + "enable", 1},
            bandNames[n] + " Enable", true));

        bandGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{prefix + "low", 1},
            bandNames[n] + " Low",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
            lowDefaults[n]));

        bandGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{prefix + "high", 1},
            bandNames[n] + " High",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
            highDefaults[n]));

        bandGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{prefix + "depth", 1},
            bandNames[n] + " Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

        // Euclidean parameters
        bandGroup->addChild(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{prefix + "euc_on", 1},
            bandNames[n] + " Euclidean", false));

        bandGroup->addChild(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{prefix + "euc_steps", 1},
            bandNames[n] + " Euc Steps", 1, 32, 16));

        bandGroup->addChild(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{prefix + "euc_pulses", 1},
            bandNames[n] + " Euc Pulses", 1, 32, 8));

        bandGroup->addChild(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{prefix + "euc_offset", 1},
            bandNames[n] + " Euc Offset", 0, 31, 0));

        // Step grid parameters (32 steps per band)
        for (int m = 0; m < 32; ++m)
        {
            juce::String stepId = "step_b" + juce::String(n) + "_s" + juce::String(m);
            juce::String stepName = bandNames[n] + " Step " + juce::String(m + 1);

            bandGroup->addChild(std::make_unique<juce::AudioParameterBool>(
                juce::ParameterID{stepId, 1}, stepName, false));
        }

        layout.add(std::move(bandGroup));
    }

    return layout;
}
```

### Pattern 2: Cached Parameter Pointers for Real-Time Access

**What:** Store `std::atomic<float>*` pointers as member variables to avoid string-based lookups in processBlock.

**When to use:** Any plugin with many parameters accessed in the audio thread.

**Example:**
```cpp
// In PluginProcessor.h
class OFreqPulseAudioProcessor : public juce::AudioProcessor
{
private:
    // Cached parameter pointers (initialized once in constructor)
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* stepsParam = nullptr;
    std::atomic<float>* rateParam = nullptr;
    std::atomic<float>* swingParam = nullptr;
    std::atomic<float>* smoothingParam = nullptr;

    // Per-band cached pointers
    struct BandParams {
        std::atomic<float>* enable = nullptr;
        std::atomic<float>* low = nullptr;
        std::atomic<float>* high = nullptr;
        std::atomic<float>* depth = nullptr;
        std::atomic<float>* eucOn = nullptr;
        std::atomic<float>* eucSteps = nullptr;
        std::atomic<float>* eucPulses = nullptr;
        std::atomic<float>* eucOffset = nullptr;
        std::array<std::atomic<float>*, 32> steps;
    };
    std::array<BandParams, 4> bandParams;

    juce::AudioProcessorValueTreeState parameters;
};

// In PluginProcessor.cpp constructor
OFreqPulseAudioProcessor::OFreqPulseAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache global parameters
    mixParam = parameters.getRawParameterValue("mix");
    stepsParam = parameters.getRawParameterValue("steps");
    rateParam = parameters.getRawParameterValue("rate");
    swingParam = parameters.getRawParameterValue("swing");
    smoothingParam = parameters.getRawParameterValue("smoothing");

    // Cache per-band parameters
    for (int n = 0; n < 4; ++n)
    {
        juce::String prefix = "band" + juce::String(n) + "_";
        bandParams[n].enable = parameters.getRawParameterValue(prefix + "enable");
        bandParams[n].low = parameters.getRawParameterValue(prefix + "low");
        bandParams[n].high = parameters.getRawParameterValue(prefix + "high");
        bandParams[n].depth = parameters.getRawParameterValue(prefix + "depth");
        bandParams[n].eucOn = parameters.getRawParameterValue(prefix + "euc_on");
        bandParams[n].eucSteps = parameters.getRawParameterValue(prefix + "euc_steps");
        bandParams[n].eucPulses = parameters.getRawParameterValue(prefix + "euc_pulses");
        bandParams[n].eucOffset = parameters.getRawParameterValue(prefix + "euc_offset");

        for (int m = 0; m < 32; ++m)
        {
            juce::String stepId = "step_b" + juce::String(n) + "_s" + juce::String(m);
            bandParams[n].steps[m] = parameters.getRawParameterValue(stepId);
        }
    }
}
```

### Anti-Patterns to Avoid

- **String lookup in processBlock:** NEVER call `getRawParameterValue("paramId")` inside `processBlock()`. Cache pointers in constructor.
- **Adding parameters after group attachment:** Once `AudioProcessorParameterGroup` is added to the layout, do not mutate it.
- **Mixing parameters and groups at same level:** Keep hierarchy clean - groups should contain only parameters or only subgroups when possible.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parameter smoothing | Custom interpolation | `juce::SmoothedValue<float>` | Handles sample-rate changes, thread-safe |
| Dry/wet mixing | Manual crossfade | `juce::dsp::DryWetMixer<float>` | Handles equal-power, latency compensation |
| State serialization | Custom XML/JSON | `APVTS::copyState()` / `replaceState()` | Handles all parameters automatically |
| FFT transforms | Custom FFT | `juce::dsp::FFT` | SIMD-optimized (vDSP on macOS) |
| Windowing functions | Custom Hann window | `juce::dsp::WindowingFunction<float>` | COLA normalization built-in |

**Key insight:** JUCE's parameter system handles serialization, threading, and host communication automatically. Custom solutions risk breaking DAW compatibility.

## Common Pitfalls

### Pitfall 1: AU Parameter Ordering in Logic
**What goes wrong:** Parameters appear in alphabetical order in Logic's automation list, not in creation order.
**Why it happens:** AU/AUv3 format limitation - Logic sorts parameters alphabetically regardless of group structure.
**How to avoid:** Accept this limitation for AU. VST3 respects creation order. Consider prefixing parameter names with numbers if order is critical (e.g., "01 Mix", "02 Steps").
**Warning signs:** User reports of confusing automation list in Logic.

### Pitfall 2: Parameter Version Hints
**What goes wrong:** Saved automation breaks when parameters are added or reordered.
**Why it happens:** DAWs identify parameters by hash of ID + version hint. Changing hints breaks mapping.
**How to avoid:** Always use version hint `1` for all parameters in initial release. Only increment for new parameters in updates.
**Warning signs:** Preset recall failures, automation not applied correctly.

### Pitfall 3: Large Parameter Count Memory
**What goes wrong:** Plugin uses excessive memory or has slow startup.
**Why it happens:** Each parameter creates ValueTree nodes, listener registrations, and atomic storage.
**How to avoid:** 165 parameters is acceptable. Avoid >500 without careful optimization. Consider composite parameters for related values.
**Warning signs:** Slow plugin scan, high memory usage in Activity Monitor.

### Pitfall 4: Thread-Unsafe Parameter Access
**What goes wrong:** Crashes or data corruption when automating parameters.
**Why it happens:** Accessing `getRawParameterValue()` result without understanding it returns `std::atomic<float>*`.
**How to avoid:** Use `->load()` to read, never dereference directly. Never call non-real-time-safe methods in processBlock.
**Warning signs:** Random crashes during automation, inconsistent parameter values.

## Code Examples

### CMakeLists.txt Template
```cmake
# Source: Existing O-series plugins (O-Freeze, O-Detune)
cmake_minimum_required(VERSION 3.15)

# Plugin configuration
juce_add_plugin(O-FreqPulse
    VERSION 1.0.0
    COMPANY_NAME "Ouaricon Development"
    PLUGIN_MANUFACTURER_CODE OuDv
    PLUGIN_CODE OFPu
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-FreqPulse"
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(O-FreqPulse
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(O-FreqPulse
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-FreqPulse
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

# Generate JuceHeader.h (JUCE 8 requirement)
juce_generate_juce_header(O-FreqPulse)

# Compile definitions
target_compile_definitions(O-FreqPulse
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
```

### Minimal PluginProcessor.h Template
```cpp
// Source: Pattern derived from O-Freeze
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

class OFreqPulseAudioProcessor : public juce::AudioProcessor
{
public:
    OFreqPulseAudioProcessor();
    ~OFreqPulseAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-FreqPulse"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

private:
    // Cached parameter pointers for real-time access
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* stepsParam = nullptr;
    std::atomic<float>* rateParam = nullptr;
    std::atomic<float>* swingParam = nullptr;
    std::atomic<float>* smoothingParam = nullptr;

    struct BandParams {
        std::atomic<float>* enable = nullptr;
        std::atomic<float>* low = nullptr;
        std::atomic<float>* high = nullptr;
        std::atomic<float>* depth = nullptr;
        std::atomic<float>* eucOn = nullptr;
        std::atomic<float>* eucSteps = nullptr;
        std::atomic<float>* eucPulses = nullptr;
        std::atomic<float>* eucOffset = nullptr;
        std::array<std::atomic<float>*, 32> steps;
    };
    std::array<BandParams, 4> bandParams;

    // APVTS (must be after cached pointers for initialization order)
    juce::AudioProcessorValueTreeState parameters;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OFreqPulseAudioProcessor)
};
```

### State Save/Load Pattern
```cpp
// Source: O-Freeze pattern
void OFreqPulseAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OFreqPulseAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `AudioParameter*` classes | `AudioProcessorValueTreeState` | JUCE 5.0+ | Automatic serialization, attachments |
| Manual XML state | `APVTS::copyState()`/`replaceState()` | JUCE 5.0+ | Less boilerplate, consistent handling |
| Flat parameter lists | `AudioProcessorParameterGroup` | JUCE 5.4+ | DAW group display for VST3 |
| `ParameterID(String)` | `ParameterID{String, int}` with version hint | JUCE 7.0+ | Better automation compatibility |

**Deprecated/outdated:**
- Direct `AudioParameterFloat` without APVTS: Still works but misses serialization benefits
- `JUCE_FORCE_USE_LEGACY_PARAM_IDS`: Should be avoided for new plugins

## Open Questions

Things that couldn't be fully resolved:

1. **Step Grid Parameter Strategy**
   - What we know: 128 step parameters can be exposed as individual bools
   - What's unclear: Whether to nest steps as subgroup within band groups, or keep flat
   - Recommendation: Nest within band groups for cleaner DAW display

2. **AU Parameter Ordering Workaround**
   - What we know: Logic sorts AU params alphabetically
   - What's unclear: Whether numeric prefixes in display names help
   - Recommendation: Accept limitation, VST3 works correctly

## Sources

### Primary (HIGH confidence)
- [JUCE AudioProcessorParameterGroup Docs](https://docs.juce.com/master/classAudioProcessorParameterGroup.html) - Complete API reference
- [JUCE AudioProcessorValueTreeState Docs](https://docs.juce.com/master/classAudioProcessorValueTreeState.html) - Parameter management
- Existing O-series plugins (O-Freeze, O-Detune, O-MultiBandCompressor) - Proven CMake and parameter patterns

### Secondary (MEDIUM confidence)
- [JUCE Forum: Plug-in parameter groups](https://forum.juce.com/t/plug-in-parameter-groups/29409) - DAW behavior analysis
- [JUCE Forum: Parameters en masse](https://forum.juce.com/t/how-to-access-audioprocessorvaluetreestate-parameters-en-masse/20455) - Large parameter count optimization

### Tertiary (LOW confidence)
- General JUCE CMake patterns - Verified against existing codebase

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Verified against existing O-series plugins
- Architecture: HIGH - JUCE documentation + existing codebase patterns
- Pitfalls: MEDIUM - Forum discussions + JUCE documentation

**Research date:** 2026-02-03
**Valid until:** 2026-03-03 (30 days - JUCE API stable)
