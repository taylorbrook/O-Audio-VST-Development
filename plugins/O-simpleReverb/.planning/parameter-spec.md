# Parameter Specification: OuariconSimpleReverb

**Generated from:** Creative Brief (2026-01-13)
**Total Parameters:** 6

---

## Parameters

### TYPE
- **ID:** `TYPE`
- **Type:** Choice (Dropdown)
- **Options:** Booth, Room, Hall, Spring, Plate, Ambient
- **Default:** Room (index 1)
- **Description:** Selects reverb algorithm/character

### CHARACTER
- **ID:** `CHARACTER`
- **Type:** Float (Bipolar Knob)
- **Range:** -100% to +100%
- **Default:** 0% (Neutral/center)
- **Description:** Tonal coloration of the reverb tail (Warm ← → Bright)
- **Display:** Percentage with sign

### WET
- **ID:** `WET`
- **Type:** Float (Knob)
- **Range:** 0% to 100%
- **Default:** 25%
- **Description:** Reverb signal level
- **Display:** Percentage

### DRY
- **ID:** `DRY`
- **Type:** Float (Knob)
- **Range:** 0% to 100%
- **Default:** 100%
- **Description:** Original signal level
- **Display:** Percentage

### DECAY
- **ID:** `DECAY`
- **Type:** Float (Knob)
- **Range:** 0.1s to 10.0s
- **Default:** 1.5s
- **Description:** Reverb tail length
- **Display:** Seconds (1 decimal)
- **Skew:** Logarithmic recommended

### SIZE
- **ID:** `SIZE`
- **Type:** Float (Knob)
- **Range:** 0% to 100%
- **Default:** 50% (Medium/center)
- **Description:** Virtual room dimensions (Small ← → Large)
- **Display:** Percentage

---

## APVTS Parameter Definitions

```cpp
std::vector<std::unique_ptr<juce::RangedAudioParameter>> createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // TYPE - Choice parameter (6 options)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"TYPE", 1},
        "Type",
        juce::StringArray{"Booth", "Room", "Hall", "Spring", "Plate", "Ambient"},
        1  // Default: Room
    ));

    // CHARACTER - Bipolar float (-100% to +100%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"CHARACTER", 1},
        "Character",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f  // Default: Neutral
    ));

    // WET - Float (0% to 100%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"WET", 1},
        "Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f  // Default: 25%
    ));

    // DRY - Float (0% to 100%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"DRY", 1},
        "Dry",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f  // Default: 100%
    ));

    // DECAY - Float (0.1s to 10.0s, logarithmic)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"DECAY", 1},
        "Decay",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f),  // Skew 0.5 for log
        1.5f  // Default: 1.5s
    ));

    // SIZE - Float (0% to 100%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"SIZE", 1},
        "Size",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f  // Default: 50% (Medium)
    ));

    return params;
}
```

---

## UI Mapping

| Parameter | Control Type | Size | Position |
|-----------|-------------|------|----------|
| TYPE | Dropdown | Standard | Top row |
| CHARACTER | Knob (bipolar) | 60px | Main row |
| WET | Knob | 60px | Main row |
| DRY | Knob | 60px | Main row |
| DECAY | Knob | 60px | Main row |
| SIZE | Knob | 60px | Main row |

---

## Notes

- CHARACTER is bipolar: negative = warm (LP filter), positive = bright (HS boost), zero = neutral (bypass)
- DRY and WET are independent (not linked as single MIX control)
- DECAY uses logarithmic skew for musical response
- TYPE changes reverb preset parameters (roomSize, damping, width)
