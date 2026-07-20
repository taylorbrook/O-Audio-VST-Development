# DSP Architecture: GainKnob

**CRITICAL CONTRACT:** This specification is immutable during implementation (Stages 3-5). Any changes require versioned modifications through the `/improve` workflow.

**Created:** 2025-11-10
**Plugin Type:** Audio Effect (Utility)
**Processing Type:** Time-domain (direct buffer manipulation)
**I/O Configuration:** Stereo in/out (applies gain uniformly to all channels)

## Core Components

### Gain Conversion Utility
- **JUCE Class:** `juce::Decibels`
- **Purpose:** Convert decibel values to linear gain multipliers
- **Parameters Affected:** GAIN
- **Configuration:** Static utility class, no instantiation needed
- **Method Used:** `juce::Decibels::decibelsToGain(float dB)`

### Buffer Gain Application
- **JUCE Class:** `juce::AudioBuffer<float>::applyGain(float gain)`
- **Purpose:** Apply linear gain multiplier to all audio samples
- **Parameters Affected:** GAIN
- **Configuration:** Called on audio buffer in processBlock()

## Processing Chain

```
Audio Input
     ↓
[Read GAIN parameter (dB)]
     ↓
[Convert dB → Linear]
juce::Decibels::decibelsToGain()
     ↓
[Special case: ≤ -59.9 dB → 0.0]
     ↓
[Apply gain to buffer]
buffer.applyGain(gainLinear)
     ↓
Audio Output
```

## Parameter Mapping

| Parameter ID | DSP Component | Usage | Range | Conversion |
|--------------|---------------|-------|-------|------------|
| GAIN | juce::Decibels | Volume attenuation | -60.0 to 0.0 dB | dB → linear (0.001 to 1.0) |

## Algorithm Details

### Gain Conversion Algorithm

**Input:** GAIN parameter value in decibels (-60.0 to 0.0 dB)
**Output:** Linear gain multiplier (0.0 to 1.0)

**Implementation:**
```cpp
float gainDb = *apvts.getRawParameterValue("GAIN");
float gainLinear;

if (gainDb <= -59.9f) {
    // Special case: treat near-minimum as complete silence
    gainLinear = 0.0f;
} else {
    // Standard dB to linear conversion: gain = 10^(dB/20)
    gainLinear = juce::Decibels::decibelsToGain(gainDb);
}

// Apply to all channels uniformly
buffer.applyGain(gainLinear);
```

**Mathematical Formula:**
- Linear gain = 10^(dB / 20)
- -60 dB ≈ 0.001 (near silence)
- 0 dB = 1.0 (unity gain, no change)

**Special Case Handling:**
- Values ≤ -59.9 dB → Force to 0.0 (complete silence)
- This avoids floating-point denormals and ensures true silence at minimum

## Special Considerations

### Thread Safety
- **Read-only operations:** Parameter read and dB conversion are safe in audio thread
- **No allocations:** All operations are stack-based, no heap allocations in processBlock()
- **No mutex needed:** Single parameter read, no shared state

### Performance
- **Computational cost:** Negligible (single math operation + buffer multiplication)
- **SIMD optimization:** buffer.applyGain() uses JUCE's optimized SIMD path automatically
- **Expected CPU:** < 0.1% (utility plugin)

### Denormal Protection
- **Not critical for this plugin** (gain multiplication doesn't generate denormals)
- Optional: Use `juce::ScopedNoDenormals` in processBlock() as best practice
- Special case at minimum (-59.9 dB → 0.0) prevents near-zero denormal values

### Sample Rate Handling
- **Sample rate independent:** Gain multiplication is instantaneous, no sample rate dependency
- **prepareToPlay:** No buffer allocation or sample rate configuration needed
- **releaseResources:** No cleanup needed

### Edge Cases
- **Zero-length buffers:** Safe (buffer.applyGain handles gracefully)
- **Mono/stereo/multichannel:** Works for any channel count (applies to all channels)
- **Extreme parameter values:** Clamped by APVTS range (-60.0 to 0.0 dB)

## Research References

### JUCE Documentation
- **juce::Decibels:** Static utility class for dB conversions
  - Source: JUCE core module (juce_audio_basics)
  - Documentation: juce::Decibels::decibelsToGain() and gainToDecibels()
- **juce::AudioBuffer:** Audio buffer manipulation
  - Source: JUCE core module (juce_audio_basics)
  - Method: applyGain() applies gain to all samples efficiently

### Professional Examples
- **Logic Pro Gain Plugin:** Simple volume utility (-96 dB to +12 dB range)
- **Pro Tools Trim:** Attenuation-only gain control (-∞ to 0 dB)
- **Waves VolumeShaper:** Advanced gain utility with envelope following

**Industry Standard Ranges:**
- Attenuation-only: -60 dB to 0 dB (typical for trim/utility)
- Makeup gain: 0 dB to +12 dB or +24 dB
- GainKnob uses attenuation-only for simplicity

### Technical Resources
- **Decibel Formula:** gain (linear) = 10^(dB / 20)
- **dB SPL reference:** 0 dB = unity gain (no change to signal level)
- **-∞ dB representation:** Practical minimum of -60 dB or lower (mathematically -∞)

## Version History

- **v1 (2025-11-10):** Initial DSP architecture - Single gain parameter with dB to linear conversion
