# DriveVerb - Parameter Specification

**Status:** 🔒 Locked (immutable contract for implementation)
**Created:** 2025-11-11
**UI Version:** v3

## Total Parameter Count

**Total:** 6 parameters (5 sliders + 1 toggle)

## Parameter Definitions

### SIZE

- **Parameter ID:** `size`
- **Type:** Float
- **Range:** 0.0 to 100.0 %
- **Default:** 40.0 %
- **Skew Factor:** Linear (1.0)
- **Unit:** %
- **UI Control:** Rotary knob (90px diameter), position 1
- **Value Display:** None (knob position indicates value)
- **DSP Usage:** Room dimensions control for algorithmic reverb
  - 0% = very small room (tight booth)
  - 100% = large hall (expansive space)
  - Affects early reflection density and spatial character
- **Automation:** Supported
- **MIDI Learn:** Not implemented

### DECAY

- **Parameter ID:** `decay`
- **Type:** Float
- **Range:** 0.5 to 10.0 seconds
- **Default:** 2.0 seconds
- **Skew Factor:** Logarithmic (0.3) - More resolution in shorter decay times
- **Unit:** s
- **UI Control:** Rotary knob (90px diameter), position 2
- **Value Display:** None (knob position indicates value)
- **DSP Usage:** Reverb tail length
  - 0.5s = short ambience
  - 10.0s = very long cathedral-style decay
  - Independent of SIZE parameter
- **Automation:** Supported
- **MIDI Learn:** Not implemented

### DRY/WET

- **Parameter ID:** `dryWet`
- **Type:** Float
- **Range:** 0.0 to 100.0 %
- **Default:** 30.0 %
- **Skew Factor:** Linear (1.0)
- **Unit:** %
- **UI Control:** Rotary knob (90px diameter), position 3
- **Value Display:** None (knob position indicates value)
- **DSP Usage:** Blend between dry (unprocessed) and wet (reverb) signal
  - 0% = 100% dry, 0% wet (no reverb)
  - 100% = 0% dry, 100% wet (only reverb)
  - Uses equal-power crossfade for smooth transition
- **Automation:** Supported
- **MIDI Learn:** Not implemented

### DRIVE

- **Parameter ID:** `drive`
- **Type:** Float
- **Range:** 0.0 to 24.0 dB
- **Default:** 6.0 dB
- **Skew Factor:** Linear (1.0)
- **Unit:** dB
- **UI Control:** Rotary knob (90px diameter), position 4
- **Value Display:** None (knob position indicates value)
- **DSP Usage:** Tape saturation amount applied ONLY to wet (reverb) signal
  - 0dB = no saturation (clean)
  - 24dB = heavy saturation (aggressive drive)
  - Even harmonic saturation (warm, tube/tape-like character)
  - Dry signal remains completely clean
- **Automation:** Supported
- **MIDI Learn:** Not implemented
- **VU Meter:** Drive level displayed on VU meter (-20dB to +3dB scale)

### FILTER

- **Parameter ID:** `filter`
- **Type:** Float
- **Range:** -100.0 to +100.0 %
- **Default:** 0.0 % (center = bypass)
- **Skew Factor:** Linear (1.0)
- **Unit:** %
- **UI Control:** Rotary knob (90px diameter), position 6 (after toggle)
- **Value Display:** None (knob position indicates value)
- **DSP Usage:** DJ-style filter applied to reverb signal
  - **0% (center):** Filter bypassed, no frequency shaping
  - **Negative values (-1% to -100%):** Low-pass filter
    - -100% = 200Hz cutoff (heavy bass only)
    - Approaching 0% = 20kHz cutoff (transparent)
  - **Positive values (+1% to +100%):** High-pass filter
    - +100% = 10kHz cutoff (heavy treble only)
    - Approaching 0% = 20Hz cutoff (transparent)
  - Uses exponential frequency mapping for musical sweep
  - Q factor: 0.707 (Butterworth response)
  - Filter state resets on bypass to prevent residual energy
- **Automation:** Supported
- **MIDI Learn:** Not implemented
- **Implementation:** Based on GainKnob filter (PluginProcessor.cpp:86-130)

### FILTER POSITION

- **Parameter ID:** `filterPosition`
- **Type:** Boolean (represented as Float for APVTS compatibility)
- **Range:** 0.0 to 1.0
  - 0.0 = PRE (filter before drive)
  - 1.0 = POST (filter after drive)
- **Default:** 1.0 (POST)
- **Skew Factor:** N/A (stepped parameter)
- **Unit:** None
- **UI Control:** Vertical toggle switch (32x90px), position 5 (between Drive and Filter)
- **Value Display:** Labels embedded in toggle (PRE at top, POST at bottom)
- **DSP Usage:** Signal routing switch
  - **PRE (0.0):** Input → Reverb → Filter → Drive → Dry/Wet Mix → Output
  - **POST (1.0):** Input → Reverb → Drive → Filter → Dry/Wet Mix → Output
  - Determines whether filtering happens before or after saturation
  - Affects tonal character (filtered-then-driven vs driven-then-filtered)
- **Automation:** Supported
- **MIDI Learn:** Not implemented

---

## Implementation Notes

### APVTS Declaration (PluginProcessor.cpp)

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // SIZE
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("size", 1),
        "Size",
        juce::NormalisableRange<float>(
            0.0f,      // Min
            100.0f,    // Max
            0.1f,      // Step size
            1.0f       // Skew factor (linear)
        ),
        40.0f      // Default value
    ));

    // DECAY
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("decay", 1),
        "Decay",
        juce::NormalisableRange<float>(
            0.5f,      // Min
            10.0f,     // Max
            0.01f,     // Step size
            0.3f       // Skew factor (logarithmic - more resolution in short times)
        ),
        2.0f       // Default value
    ));

    // DRY/WET
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("dryWet", 1),
        "Dry/Wet",
        juce::NormalisableRange<float>(
            0.0f,      // Min
            100.0f,    // Max
            0.1f,      // Step size
            1.0f       // Skew factor (linear)
        ),
        30.0f      // Default value
    ));

    // DRIVE
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("drive", 1),
        "Drive",
        juce::NormalisableRange<float>(
            0.0f,      // Min
            24.0f,     // Max
            0.1f,      // Step size
            1.0f       // Skew factor (linear)
        ),
        6.0f       // Default value
    ));

    // FILTER
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filter", 1),
        "Filter",
        juce::NormalisableRange<float>(
            -100.0f,   // Min (low-pass)
            100.0f,    // Max (high-pass)
            0.1f,      // Step size
            1.0f       // Skew factor (linear)
        ),
        0.0f       // Default value (center = bypass)
    ));

    // FILTER POSITION
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("filterPosition", 1),
        "Filter Position",
        true       // Default value (true = POST)
    ));

    return { params.begin(), params.end() };
}
```

### DSP Implementation Notes

#### Reverb Engine
```cpp
// Use juce::dsp::Reverb or custom algorithmic reverb
// SIZE parameter controls:
// - Room size (0.0 to 1.0)
// - Early reflection density
// DECAY parameter controls:
// - Damping/decay time independently
```

#### Drive Saturation
```cpp
// Apply ONLY to wet signal (after reverb, position-dependent with filter)
// Even harmonic saturation:
// - Soft clipping or waveshaping
// - Tanh, atan, or polynomial waveshaper
// - 0-24dB gain range before saturation stage
```

#### DJ-Style Filter
```cpp
// Based on GainKnob implementation
// Center (0%) = bypass (no filtering)
// Negative = low-pass (200Hz at -100% to 20kHz at 0%)
// Positive = high-pass (20Hz at 0% to 10kHz at +100%)
// Exponential frequency mapping for musical sweep
// Q = 0.707 (Butterworth)
// Reset filter state on bypass to prevent residual energy
```

#### Signal Flow
```cpp
// PRE mode (filterPosition == false):
float reverbOut = processReverb(input, size, decay);
float filtered = processFilter(reverbOut, filter);
float driven = processDrive(filtered, drive);
float output = mix(input, driven, dryWet);

// POST mode (filterPosition == true):
float reverbOut = processReverb(input, size, decay);
float driven = processDrive(reverbOut, drive);
float filtered = processFilter(driven, filter);
float output = mix(input, filtered, dryWet);
```

### Special Handling

- **Filter bypass:** When `filter == 0.0%`, completely bypass filter processing (no IIR applied)
- **Filter state reset:** Reset filter delay buffers when entering/exiting bypass zone or switching filter type
- **Drive metering:** Track peak drive output level for VU meter display (-20dB to +3dB range)
- **Smoothing:** Apply parameter smoothing to prevent zipper noise (especially on decay, drive, filter)

---

## Contract Rules

This parameter specification is **immutable** during implementation stages:

- ✅ Stage 3 (Shell) reads this file to generate APVTS
- ✅ Stage 4 (DSP) reads this file to implement audio processing
- ✅ Stage 5 (GUI) uses this file to verify parameter binding

If parameter changes are needed after implementation begins:
1. Use `/improve` workflow for versioned modifications
2. Update parameter-spec.md with version history
3. Create migration strategy for existing presets

---

## Version History

- **v1 (2025-11-11):** Initial specification - 6 parameters (size, decay, dryWet, drive, filter, filterPosition)
