# Stage 1: Foundation - Research

**Date:** 2026-02-03
**Plugin:** O-SpectralShaper
**Stage:** 1-foundation (research phase)

---

## Research Summary

This document captures implementation research for Stage 1 Foundation of O-SpectralShaper. The Foundation stage establishes the build system and APVTS parameters. DSP implementation occurs in Stage 2.

---

## 1. JUCE Modules Required

### Core Modules (Verified from O-Freeze and O-Bells references)

| Module | Purpose | Required For |
|--------|---------|--------------|
| `juce::juce_audio_basics` | Audio buffer primitives | Core audio types |
| `juce::juce_audio_devices` | Audio device management | Standalone mode |
| `juce::juce_audio_formats` | Audio file I/O | Resource loading |
| `juce::juce_audio_plugin_client` | VST3/AU wrapper | Plugin deployment |
| `juce::juce_audio_processors` | AudioProcessor base | Core plugin class |
| `juce::juce_audio_utils` | Audio utilities | Helper functions |
| `juce::juce_core` | Core JUCE classes | Foundation |
| `juce::juce_data_structures` | ValueTree, APVTS | Parameter system |
| `juce::juce_dsp` | FFT, windowing, DryWetMixer | Stage 2 DSP |
| `juce::juce_events` | Message thread | Async updates |
| `juce::juce_graphics` | Graphics primitives | Rendering |
| `juce::juce_gui_basics` | GUI components | UI framework |
| `juce::juce_gui_extra` | WebBrowserComponent | WebView UI |

**Note:** `juce_dsp` is linked in Stage 1 even though FFT implementation occurs in Stage 2. This avoids CMake reconfiguration later.

---

## 2. Critical JUCE 8 Patterns (From juce8-critical-patterns.md)

### Pattern #1: CMake Header Generation
```cmake
target_link_libraries(O-SpectralShaper ...)

# CRITICAL: MUST come AFTER target_link_libraries
juce_generate_juce_header(O-SpectralShaper)

target_compile_definitions(O-SpectralShaper ...)
```

### Pattern #9: NEEDS_WEB_BROWSER for VST3
```cmake
juce_add_plugin(O-SpectralShaper
    ...
    NEEDS_WEB_BROWSER TRUE  # Required for VST3 WebView support
)
```

### Pattern #11: WebView Member Order (Relays → WebView → Attachments)
```cpp
// 1. Relays first (no dependencies)
std::unique_ptr<juce::WebSliderRelay> mixRelay;

// 2. WebView second (depends on relays via .withOptionsFrom())
std::unique_ptr<juce::WebBrowserComponent> webView;

// 3. Attachments last (depend on both)
std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
```

### Pattern #12: Three Parameters for WebSliderParameterAttachment
```cpp
// JUCE 8 requires undoManager parameter (can be nullptr)
mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *apvts.getParameter("MIX"), *mixRelay, nullptr);  // 3rd param required
```

---

## 3. Parameter Implementation (7 Parameters)

### APVTS Parameter Layout

Based on CONTEXT.md and parameter-spec.md:

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // MIX (0-100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"MIX", 1}, "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // ATTACK_TIME (0.1-50ms, log skew 0.3)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"ATTACK_TIME", 1}, "Attack Time",
        juce::NormalisableRange<float>(0.1f, 50.0f, 0.1f, 0.3f), 10.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // SUSTAIN_TIME (10-500ms, log skew 0.3)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"SUSTAIN_TIME", 1}, "Sustain Time",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.3f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // SENSITIVITY (0-100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"SENSITIVITY", 1}, "Sensitivity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // LOOKAHEAD_ENABLED (bool toggle, default OFF)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"LOOKAHEAD_ENABLED", 1}, "Lookahead Enabled",
        false
    ));

    // LOOKAHEAD_TIME (0.1-10ms)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"LOOKAHEAD_TIME", 1}, "Lookahead Time",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 2.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // OUTPUT_GAIN (-12 to +12 dB)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"OUTPUT_GAIN", 1}, "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    return layout;
}
```

### Boolean Parameter - WebView Binding

For `LOOKAHEAD_ENABLED`, use `WebToggleButtonRelay` (not WebSliderRelay):

```cpp
// PluginEditor.h
std::unique_ptr<juce::WebToggleButtonRelay> lookaheadEnabledRelay;
std::unique_ptr<juce::WebToggleButtonParameterAttachment> lookaheadEnabledAttachment;

// PluginEditor.cpp constructor
lookaheadEnabledRelay = std::make_unique<juce::WebToggleButtonRelay>("lookaheadEnabled");

// WebView creation with .withOptionsFrom(*lookaheadEnabledRelay)

lookaheadEnabledAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
    *apvts.getParameter("LOOKAHEAD_ENABLED"), *lookaheadEnabledRelay, nullptr);
```

**Pattern #19:** JavaScript must use `getToggleState()` not `getSliderState()` for booleans.

---

## 4. Latency Reporting

### Fixed 512-Sample Latency

From CONTEXT.md discussion: Report fixed 512 samples regardless of lookahead toggle state.

```cpp
// PluginProcessor.h
int getLatencySamples() const override { return 512; }
```

**Rationale:**
- FFT processing requires buffer anyway (~11.6ms @ 44.1kHz)
- Dynamic latency causes DAW compensation issues
- Consistent with competitors (Eventide SplitEQ: 10ms, Spiff: ~20ms)

---

## 5. Bus Configuration (Audio Effect)

O-SpectralShaper is an effect (audio in → audio out), NOT a synth:

```cpp
// PluginProcessor.cpp constructor
OSpectralShaperAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}
```

**Do NOT add `IS_SYNTH TRUE`** - this would break DAW MIDI routing expectations for an effect.

---

## 6. CMake Configuration

### juce_add_plugin Settings

```cmake
juce_add_plugin(O-SpectralShaper
    COMPANY_NAME "Ouaricon Development"
    PLUGIN_MANUFACTURER_CODE OuDv
    PLUGIN_CODE OSpS           # 4-char unique code
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-SpectralShaper"
    PLUGIN_VERSION "1.0.0"

    # Effect configuration (NOT a synth)
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    # WebView UI
    NEEDS_WEB_BROWSER TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)
```

### Compile Definitions

```cmake
target_compile_definitions(O-SpectralShaper
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
```

---

## 7. Directory Structure

```
plugins/O-SpectralShaper/
├── CMakeLists.txt
├── Source/
│   ├── PluginProcessor.h
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h
│   └── PluginEditor.cpp
├── Resources/
│   └── ui/
│       ├── index.html               # Placeholder for Stage 3
│       └── js/
│           └── juce/
│               ├── index.js         # Copy from JUCE examples
│               └── check_native_interop.js
└── .planning/
    └── stages/
        └── 1-foundation/
            ├── CONTEXT.md
            └── RESEARCH.md (this file)
```

---

## 8. Reference Implementation Patterns

### From O-Freeze (Effect with DSP)

**PluginProcessor structure:**
- Member initialization order: DSP components → APVTS (ensures APVTS lambda can access members)
- Static `createParameterLayout()` function
- `prepareToPlay()` initializes juce::dsp::ProcessSpec for DSP components
- `getStateInformation()`/`setStateInformation()` for preset save/load

### From O-Bells (WebView with Many Parameters)

**PluginEditor structure:**
- Relays declared before WebView
- Attachments declared after WebView
- WebBrowserComponent::Options with chained `.withOptionsFrom()` calls
- `getResource()` helper with explicit URL mapping
- Timer for meter updates (30Hz)

---

## 9. Curve Data State (Non-APVTS)

Curves are NOT APVTS parameters (not automatable) but must persist:

```cpp
// PluginProcessor.h
private:
    std::array<float, 32> attackCurve{};   // Initialize to 0.0
    std::array<float, 32> sustainCurve{};  // Initialize to 0.0
```

**State save/restore** will be implemented in Stage 3 (GUI) when curve editing is added.

---

## 10. Potential Pitfalls

### Pitfall #1: Missing juce_generate_juce_header()
**Solution:** Add after `target_link_libraries()` in CMakeLists.txt

### Pitfall #2: Wrong WebSliderParameterAttachment signature
**Solution:** Always pass three parameters: `(param, relay, nullptr)`

### Pitfall #3: WebView not loading in VST3
**Solution:** Ensure `NEEDS_WEB_BROWSER TRUE` in juce_add_plugin

### Pitfall #4: Boolean parameter uses wrong JS API
**Solution:** Use `getToggleState()` not `getSliderState()` for LOOKAHEAD_ENABLED

### Pitfall #5: Missing check_native_interop.js
**Solution:** Include in binary data and serve from resource provider

---

## 11. Module Opportunities

### Existing Modules (Not Applicable for Stage 1)

| Module | Relevance |
|--------|-----------|
| webview-relay-manager | Potential - simplifies relay/attachment boilerplate |
| preset-manager | Future - Stage 4 polish |

**Recommendation:** No module dependencies for Stage 1. Keep foundation minimal.

---

## 12. Verification Checklist

Stage 1 is complete when:

- [ ] CMakeLists.txt builds without errors
- [ ] VST3 and AU binaries generated
- [ ] Plugin loads in DAW (Logic Pro, Ableton)
- [ ] All 7 parameters visible in DAW automation
- [ ] Parameters respond to automation changes
- [ ] State save/load works (default APVTS behavior)
- [ ] Latency compensation correct (512 samples)
- [ ] WebView placeholder displays (can be blank page)

---

## Next Phase

Ready for: **plan** phase → Create PLAN.md with task breakdown

Command: `/plugin-plan O-SpectralShaper 1`

---

*Research completed: 2026-02-03*
