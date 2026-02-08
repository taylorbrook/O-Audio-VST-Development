# O-Chorus Parameter Specification

**Plugin Type:** Effect (Multi-voice BBD-style Chorus)
**Total Parameters:** 7
**APVTS Required:** Yes

---

## Parameters

### RATE
- **ID:** `rate`
- **Type:** Float
- **Range:** 0.05 - 5.0
- **Default:** 1.0
- **Label:** "Rate"
- **Unit:** "Hz"
- **Skew:** Logarithmic
- **Smoothing:** 50ms
- **DSP Component:** LFO Modulation System
- **Behavior:** Controls LFO modulation speed. 0.05 Hz = ultra-slow sweep (20s cycle), 1.0 Hz = classic chorus, 5.0 Hz = fast vibrato-like modulation.

### DEPTH
- **ID:** `depth`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.5
- **Label:** "Depth"
- **Unit:** "%"
- **Skew:** Linear
- **Smoothing:** 50ms
- **DSP Component:** LFO Modulation System
- **Behavior:** Modulation amount (delay time variation). 0% = no modulation, 50% = moderate chorus, 100% = maximum modulation (delay varies ±5ms from 10ms base).

### VOICES
- **ID:** `voices`
- **Type:** Int
- **Range:** 1 - 8
- **Default:** 4
- **Label:** "Voices"
- **Unit:** ""
- **Skew:** Linear
- **Smoothing:** None (discrete steps)
- **DSP Component:** Multi-Voice Delay Line Engine
- **Behavior:** Number of chorus voices. 1 = simple vibrato/chorus, 2 = classic stereo (Juno-style), 3 = tri-chorus (Strymon Ola style), 4-8 = lush ensemble.

### WIDTH
- **ID:** `width`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.7
- **Label:** "Width"
- **Unit:** "%"
- **Skew:** Linear
- **Smoothing:** 50ms
- **DSP Component:** Stereo Imaging
- **Behavior:** Stereo spread of voices. 0% = all voices centered (mono), 100% = full stereo width with voices spread evenly across field.

### TONE
- **ID:** `tone`
- **Type:** Float
- **Range:** -1.0 - 1.0
- **Default:** 0.0
- **Label:** "Tone"
- **Unit:** ""
- **Skew:** Linear
- **Smoothing:** 100ms
- **DSP Component:** Tone Control (One-pole IIR Lowpass)
- **Behavior:** High-frequency rolloff on wet signal. -100% = dark/lo-fi BBD (2kHz cutoff), 0% = neutral BBD character (8kHz), +100% = bright/modern (20kHz).

### MIX
- **ID:** `mix`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.5
- **Label:** "Mix"
- **Unit:** "%"
- **Skew:** Linear
- **Smoothing:** 50ms
- **DSP Component:** Mix Stage
- **Behavior:** Dry/wet blend. 0% = full dry (bypass), 50% = classic chorus mix, 100% = full wet (vibrato mode).

### DRIVE
- **ID:** `drive`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.3
- **Label:** "Drive"
- **Unit:** "%"
- **Skew:** Linear
- **Smoothing:** 50ms
- **DSP Component:** Saturation Stage
- **Behavior:** Analog saturation amount. 0% = clean (no saturation), 30% = subtle warmth (default), 100% = heavy saturation for lo-fi character.

---

## APVTS Parameter Layout

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"rate", 1}, "Rate",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.35f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"depth", 1}, "Depth", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"voices", 1}, "Voices", 1, 8, 4));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"width", 1}, "Width", 0.0f, 1.0f, 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"tone", 1}, "Tone",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1}, "Mix", 0.0f, 1.0f, 0.5f));

    return {params.begin(), params.end()};
}
```

---

## Parameter Groups (for UI organization)

| Group | Parameters |
|-------|------------|
| **Modulation** | rate, depth, voices |
| **Character** | width, tone, mix, drive |

---

## Contract Validation

- **Total Float Parameters:** 6
- **Total Int Parameters:** 1
- **Total Choice Parameters:** 0
- **Total Parameters:** 7

**All parameters must:**
1. Be registered in APVTS during Stage 1 (Foundation)
2. Have atomic pointers cached in prepareToPlay()
3. Be bound to WebView UI in Stage 3 (GUI)

---

*Generated: 2026-02-07*
*Source: BRIEF.md + ARCHITECTURE.md*
