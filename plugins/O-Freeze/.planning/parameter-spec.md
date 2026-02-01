# O-Freeze Parameter Specification

**Generated:** 2026-02-01
**Source:** BRIEF.md + ARCHITECTURE.md

---

## Parameters

| ID | Name | Type | Range | Default | Skew | Description |
|----|------|------|-------|---------|------|-------------|
| FREEZE | Freeze | Bool | On/Off | Off | - | Activates freeze capture and granular playback (Manual mode only) |
| THRESHOLD | Threshold | Float | -60 to 0 dB | -40 dB | 1.0 | Input level below which auto-freeze engages (Threshold mode only) |
| MODE | Mode | Choice | Manual, Threshold | Manual | - | Freeze trigger mode selection |
| DRIFT | Drift | Float | 0 to 100% | 0% | 1.0 | Amount of random variation in grain playback positions |
| MIX | Mix | Float | 0 to 100% | 100% | 1.0 | Dry/Wet blend |

---

## Parameter Details

### FREEZE (Button)
- **APVTS Type:** `juce::AudioParameterBool`
- **Purpose:** Manual freeze trigger (activates granular playback)
- **Behavior:** When pressed, locks freeze buffer and starts granular engine
- **UI Element:** Large toggle button with visual state indicator
- **Persistence:** NOT saved with preset (always starts Off)
- **Mode Dependency:** Only active when MODE = Manual

### THRESHOLD (Slider)
- **APVTS Type:** `juce::AudioParameterFloat`
- **Range:** -60.0f to 0.0f dB
- **Default:** -40.0f dB
- **Skew:** 1.0 (linear in dB)
- **Purpose:** Auto-freeze trigger level (Threshold mode only)
- **Behavior:** Freeze engages when input RMS drops below threshold, releases at threshold + 3dB
- **UI Element:** Vertical slider or rotary knob with dB label
- **Persistence:** Saved with preset
- **Mode Dependency:** Only active when MODE = Threshold

### MODE (Choice)
- **APVTS Type:** `juce::AudioParameterChoice`
- **Choices:** ["Manual", "Threshold"]
- **Default:** "Manual" (index 0)
- **Purpose:** Selects freeze trigger mechanism
- **Behavior:**
  - Manual: FREEZE button controls freeze state
  - Threshold: Gate detector controls freeze state
- **UI Element:** Toggle button or segmented control
- **Persistence:** Saved with preset

### DRIFT (Slider)
- **APVTS Type:** `juce::AudioParameterFloat`
- **Range:** 0.0f to 100.0f (%)
- **Default:** 0.0f (static freeze)
- **Skew:** 1.0 (linear)
- **Purpose:** Adds subtle movement to frozen texture
- **Behavior:**
  - 0% = All grains read same buffer position (perfectly static)
  - 100% = Grains read across entire buffer (maximum drift)
- **UI Element:** Rotary knob with % label
- **Persistence:** Saved with preset

### MIX (Slider)
- **APVTS Type:** `juce::AudioParameterFloat`
- **Range:** 0.0f to 100.0f (%)
- **Default:** 100.0f (fully wet)
- **Skew:** 1.0 (linear)
- **Purpose:** Dry/Wet blend
- **Behavior:**
  - 0% = Dry signal only (frozen texture silent)
  - 100% = Wet signal only (frozen texture replaces input)
- **UI Element:** Rotary knob with % label
- **Persistence:** Saved with preset

---

## APVTS Layout

```cpp
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // FREEZE - Manual trigger button
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"FREEZE", 1},
        "Freeze",
        false));  // Default: Off

    // THRESHOLD - Auto-freeze level
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"THRESHOLD", 1},
        "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
        -40.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // MODE - Trigger mode selection
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"MODE", 1},
        "Mode",
        juce::StringArray{"Manual", "Threshold"},
        0));  // Default: Manual

    // DRIFT - Grain position randomization
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"DRIFT", 1},
        "Drift",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // MIX - Dry/Wet blend
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"MIX", 1},
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return { params.begin(), params.end() };
}
```

---

## UI Binding Summary

| Parameter | Relay Type | JS Control | Notes |
|-----------|------------|------------|-------|
| FREEZE | Custom button relay | Toggle button | Visual state indicator, disabled in Threshold mode |
| THRESHOLD | WebSliderRelay | Rotary knob | -60 to 0 dB, disabled in Manual mode |
| MODE | WebToggleButtonRelay | Segmented control | Manual/Threshold toggle |
| DRIFT | WebSliderRelay | Rotary knob | 0-100% |
| MIX | WebSliderRelay | Rotary knob | 0-100% |

---

## Thread Safety Notes

- FREEZE button state: Use `std::atomic<bool>` for safe audio thread access
- THRESHOLD, DRIFT, MIX: Access via `apvts.getRawParameterValue()->load()` (atomic)
- MODE: Access via `apvts.getRawParameterValue()->load()` cast to int for mode index
