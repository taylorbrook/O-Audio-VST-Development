# O-SpectralShaper Parameter Specification

---
**Contract Status:** VALID (Updated after Stage 1 discussion)
**Generated:** 2026-02-03
**Updated:** 2026-02-03 (Added LOOKAHEAD_ENABLED toggle per user request)
**Plugin Type:** Audio Effect (Spectral Transient Shaper)
---

## APVTS Parameters (7 Total)

| ID | Name | Type | Range | Default | Unit | Skew | Automation |
|----|------|------|-------|---------|------|------|-----------|
| MIX | Mix | Float | 0.0-1.0 | 1.0 | % | Linear | Yes |
| ATTACK_TIME | Attack Time | Float | 0.1-50.0 | 10.0 | ms | 0.3 (Log) | Yes |
| SUSTAIN_TIME | Sustain Time | Float | 10.0-500.0 | 100.0 | ms | 0.3 (Log) | Yes |
| SENSITIVITY | Sensitivity | Float | 0.0-1.0 | 0.5 | % | Linear | Yes |
| LOOKAHEAD_ENABLED | Lookahead Enabled | Bool | true/false | false | - | N/A | Yes |
| LOOKAHEAD_TIME | Lookahead Time | Float | 0.1-10.0 | 2.0 | ms | Linear | Yes |
| OUTPUT_GAIN | Output Gain | Float | -12.0-12.0 | 0.0 | dB | Linear | Yes |

**Note:** Lookahead is split into toggle (default OFF) and time parameter per Stage 1 discussion.

## Non-APVTS State (Curve Data)

| ID | Name | Type | Range | Default | Persistence |
|----|------|------|-------|---------|-------------|
| ATTACK_CURVE | Attack Curve | Float[32] | -1.0 to +1.0 | 0.0 each | State save/restore |
| SUSTAIN_CURVE | Sustain Curve | Float[32] | -1.0 to +1.0 | 0.0 each | State save/restore |

## Implementation Notes

### APVTS Parameter Creation
```cpp
AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    AudioProcessorValueTreeState::ParameterLayout layout;

    // Mix (0-100%)
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID{"MIX", 1}, "Mix",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f,
        AudioParameterFloatAttributes().withLabel("%")
    ));

    // Attack Time (0.1-50ms, log skew)
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID{"ATTACK_TIME", 1}, "Attack Time",
        NormalisableRange<float>(0.1f, 50.0f, 0.1f, 0.3f), 10.0f,
        AudioParameterFloatAttributes().withLabel("ms")
    ));

    // Sustain Time (10-500ms, log skew)
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID{"SUSTAIN_TIME", 1}, "Sustain Time",
        NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.3f), 100.0f,
        AudioParameterFloatAttributes().withLabel("ms")
    ));

    // Sensitivity (0-100%)
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID{"SENSITIVITY", 1}, "Sensitivity",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f,
        AudioParameterFloatAttributes().withLabel("%")
    ));

    // Lookahead Enabled (toggle, default OFF)
    layout.add(std::make_unique<AudioParameterBool>(
        ParameterID{"LOOKAHEAD_ENABLED", 1}, "Lookahead Enabled",
        false  // Default OFF
    ));

    // Lookahead Time (0.1-10ms, only active when enabled)
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID{"LOOKAHEAD_TIME", 1}, "Lookahead Time",
        NormalisableRange<float>(0.1f, 10.0f, 0.1f), 2.0f,
        AudioParameterFloatAttributes().withLabel("ms")
    ));

    // Output Gain (-12 to +12 dB)
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID{"OUTPUT_GAIN", 1}, "Output Gain",
        NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withLabel("dB")
    ));

    return layout;
}
```

### Curve Data State Management

Curves are NOT in APVTS (not automatable) but must be saved/restored with plugin state.

```cpp
// Member variables
std::array<float, 32> attackCurve;   // -1.0 to +1.0
std::array<float, 32> sustainCurve;  // -1.0 to +1.0

// State save
void getStateInformation(MemoryBlock& destData) override {
    auto state = parameters.copyState();

    // Add curve data as binary blob
    auto curvesXml = state.getOrCreateChildWithName("Curves", nullptr);
    curvesXml.setProperty("attackCurve",
        String::toHexString(attackCurve.data(), 32 * sizeof(float)), nullptr);
    curvesXml.setProperty("sustainCurve",
        String::toHexString(sustainCurve.data(), 32 * sizeof(float)), nullptr);

    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

// State restore
void setStateInformation(const void* data, int sizeInBytes) override {
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml && xml->hasTagName(parameters.state.getType())) {
        parameters.replaceState(ValueTree::fromXml(*xml));

        // Restore curves
        auto state = parameters.state;
        auto curves = state.getChildWithName("Curves");
        if (curves.isValid()) {
            // Decode hex string back to float array
            // ... implementation
        }
    }
}
```

## WebView UI Bindings

### APVTS Parameters → WebSliderRelay

Each automatable parameter needs a relay for WebView communication:

```cpp
// In PluginEditor.h
std::unique_ptr<juce::WebSliderRelay> mixRelay;
std::unique_ptr<juce::WebSliderRelay> attackTimeRelay;
std::unique_ptr<juce::WebSliderRelay> sustainTimeRelay;
std::unique_ptr<juce::WebSliderRelay> sensitivityRelay;
std::unique_ptr<juce::WebToggleButtonRelay> lookaheadEnabledRelay;
std::unique_ptr<juce::WebSliderRelay> lookaheadTimeRelay;
std::unique_ptr<juce::WebSliderRelay> outputGainRelay;

// Corresponding attachments
std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
// ... etc.
```

### Curve Data → Native Functions

Curves use direct native function calls (not relays):

```javascript
// JavaScript → C++
Juce.getNativeFunction("setAttackCurve")(curveArray);
Juce.getNativeFunction("setSustainCurve")(curveArray);

// C++ → JavaScript (via evaluateJavascript)
webView->evaluateJavascript("updateAttackCurve([...])");
```

---

*Generated as foundation-shell-agent input contract*
