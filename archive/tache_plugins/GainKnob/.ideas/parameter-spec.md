# GainKnob - Parameter Specification

**Status:** 🔒 Locked (immutable contract for implementation)
**Created:** 2025-11-10
**UI Version:** v1

## Total Parameter Count

**Total:** 1 parameter

## Parameter Definitions

### GAIN

- **Parameter ID:** `GAIN`
- **Type:** Float
- **Range:** -60.0 to 0.0 dB
- **Default:** 0.0 dB
- **Skew Factor:** Linear (1.0)
- **Unit:** dB
- **UI Control:** Rotary knob (200x200px canvas), centered
- **Value Display:** Below knob, shows "−∞ dB" at minimum, otherwise "X.X dB"
- **DSP Usage:** Volume attenuation multiplier
  - -60 dB ≈ gain multiplier of 0.001 (near silence, represents -∞)
  - 0 dB = gain multiplier of 1.0 (unity gain)
- **Automation:** Supported
- **MIDI Learn:** Not implemented

## Implementation Notes

### APVTS Declaration (PluginProcessor.cpp)

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("GAIN", 1),    // Parameter ID
        "Gain",                           // Parameter name (for DAW)
        juce::NormalisableRange<float>(
            -60.0f,                       // Min
            0.0f,                         // Max
            0.1f,                         // Step size
            1.0f                          // Skew factor (linear)
        ),
        0.0f                              // Default value
    ));

    return { params.begin(), params.end() };
}
```

### DSP Implementation

```cpp
// In processBlock():
float gainDb = *apvts.getRawParameterValue("GAIN");
float gainLinear;

if (gainDb <= -59.9f) {
    gainLinear = 0.0f;  // Complete silence at minimum
} else {
    gainLinear = juce::Decibels::decibelsToGain(gainDb);
}

// Apply gain to audio buffer
buffer.applyGain(gainLinear);
```

### Special Handling

- **-∞ dB Display:** When parameter value is ≤ -59.9 dB, display as "−∞ dB"
- **Gain Conversion:** Use `juce::Decibels::decibelsToGain()` for dB to linear conversion
- **Silence at Minimum:** Gain multiplier should be exactly 0.0 at minimum to ensure complete silence

## Contract Rules

This parameter specification is **immutable** during implementation stages:

- ✅ Stage 3 (Shell) reads this file to generate APVTS
- ✅ Stage 4 (DSP) reads this file to implement audio processing
- ✅ Stage 5 (GUI) uses this file to verify parameter binding

If parameter changes are needed after implementation begins:
1. Use `/improve` workflow for versioned modifications
2. Update parameter-spec.md with version history
3. Create migration strategy for existing presets

## Version History

- **v1 (2025-11-10):** Initial specification - Single GAIN parameter
