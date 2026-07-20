# OrganicHats - Parameter Specification

**Version:** v2 (finalized)
**Total Parameters:** 6
**Created:** 2025-11-12

---

## Parameter Definitions

### CLOSED_TONE
- **Type:** Float
- **Range:** 0.0 to 100.0
- **Default:** 50.0
- **Unit:** %
- **Skew Factor:** 1.0 (linear)
- **UI Control:** Rotary knob, Closed Hi-Hat section, order 1
- **DSP Usage:** Brightness control (dark to bright) for closed hi-hat
- **Label:** "Tone"

### CLOSED_DECAY
- **Type:** Float
- **Range:** 20.0 to 200.0
- **Default:** 80.0
- **Unit:** ms
- **Skew Factor:** 1.0 (linear)
- **UI Control:** Rotary knob, Closed Hi-Hat section, order 2
- **DSP Usage:** Envelope decay time for closed hi-hat
- **Label:** "Decay"

### CLOSED_NOISE_COLOR
- **Type:** Float
- **Range:** 0.0 to 100.0
- **Default:** 50.0
- **Unit:** %
- **Skew Factor:** 1.0 (linear)
- **UI Control:** Rotary knob, Closed Hi-Hat section, order 3
- **DSP Usage:** Noise filtering for warmth vs brightness (closed)
- **Label:** "Noise Color"

### OPEN_TONE
- **Type:** Float
- **Range:** 0.0 to 100.0
- **Default:** 50.0
- **Unit:** %
- **Skew Factor:** 1.0 (linear)
- **UI Control:** Rotary knob, Open Hi-Hat section, order 1
- **DSP Usage:** Brightness control (dark to bright) for open hi-hat
- **Label:** "Tone"

### OPEN_RELEASE
- **Type:** Float
- **Range:** 100.0 to 1000.0
- **Default:** 400.0
- **Unit:** ms
- **Skew Factor:** 1.0 (linear)
- **UI Control:** Rotary knob, Open Hi-Hat section, order 2
- **DSP Usage:** Sustained decay time for open hi-hat
- **Label:** "Release"

### OPEN_NOISE_COLOR
- **Type:** Float
- **Range:** 0.0 to 100.0
- **Default:** 50.0
- **Unit:** %
- **Skew Factor:** 1.0 (linear)
- **UI Control:** Rotary knob, Open Hi-Hat section, order 3
- **DSP Usage:** Noise filtering for warmth vs brightness (open)
- **Label:** "Noise Color"

---

## Parameter Groups

### Closed Hi-Hat (3 parameters)
Parameters that control the closed hi-hat sound:
- CLOSED_TONE
- CLOSED_DECAY
- CLOSED_NOISE_COLOR

### Open Hi-Hat (3 parameters)
Parameters that control the open hi-hat sound:
- OPEN_TONE
- OPEN_RELEASE
- OPEN_NOISE_COLOR

---

## UI Layout

**Window Size:** 600x590px
**Layout Type:** Dual-panel horizontal

**Closed Section (Left):**
- Section header: "Closed Hi-Hat"
- 3 vertical controls (Tone, Decay, Noise Color)

**Open Section (Right):**
- Section header: "Open Hi-Hat"
- 3 vertical controls (Tone, Release, Noise Color)

---

## APVTS Parameter Creation

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Closed Hi-Hat
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "CLOSED_TONE",
        "Closed Tone",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        50.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "CLOSED_DECAY",
        "Closed Decay",
        juce::NormalisableRange<float>(20.0f, 200.0f, 0.1f),
        80.0f,
        "ms"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "CLOSED_NOISE_COLOR",
        "Closed Noise Color",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        50.0f,
        "%"
    ));

    // Open Hi-Hat
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OPEN_TONE",
        "Open Tone",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        50.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OPEN_RELEASE",
        "Open Release",
        juce::NormalisableRange<float>(100.0f, 1000.0f, 0.1f),
        400.0f,
        "ms"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OPEN_NOISE_COLOR",
        "Open Noise Color",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        50.0f,
        "%"
    ));

    return layout;
}
```

---

## Notes

- All parameters use linear scaling (skew factor 1.0)
- No global parameters (velocity sensitivity is always enabled)
- Parameter IDs are UPPERCASE with underscores
- Display labels use mixed case with spaces
- Closed and Open sections mirror each other's parameter structure
- Tone and Noise Color parameters share 0-100% range for consistency
