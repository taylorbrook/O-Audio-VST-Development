<!-- NOTE: This is a historical Stage 0 planning document and may not reflect the current implementation. -->
# O-IntonationPad Parameter Specification

**Generated from:** ARCHITECTURE.md
**Date:** 2026-01-29
**Total Parameters:** 15

---

## Parameters

### VOICE_COUNT
- **Type:** Int (AudioParameterInt)
- **Range:** 2-12
- **Default:** 5
- **DSP Component:** Chord Generator
- **Description:** Number of chord voices (2=dyad, 3=triad, 12=full voicing)
- **UI Control:** Slider with integer steps

### COMPLEXITY
- **Type:** Float (AudioParameterFloat)
- **Range:** 0.0-1.0 (displayed as 0-100%)
- **Default:** 0.5
- **DSP Component:** Chord Generator
- **Description:** Chord extensions (0=triad, 50%=7th, 100%=13th)
- **UI Control:** Rotary knob

### KEY_ROOT
- **Type:** Choice (AudioParameterChoice)
- **Choices:** C, C#, D, D#, E, F, F#, G, G#, A, A#, B
- **Default:** C (index 0)
- **DSP Component:** Chord Generator
- **Description:** Root note of key for scale-degree analysis
- **UI Control:** Dropdown or 12-segment selector

### KEY_SCALE
- **Type:** Choice (AudioParameterChoice)
- **Choices:** Major, Minor, Dorian, Phrygian, Lydian, Mixolydian, Aeolian, Locrian, Harmonic Minor, Melodic Minor
- **Default:** Major (index 0)
- **DSP Component:** Chord Generator
- **Description:** Scale type for chord type determination
- **UI Control:** Dropdown

### TUNING_SYSTEM
- **Type:** Choice (AudioParameterChoice)
- **Choices:** 12-TET, Just Intonation, Pythagorean, Historical, Scala
- **Default:** Just Intonation (index 1)
- **DSP Component:** Tuning Engine
- **Description:** Active tuning mode
- **UI Control:** Dropdown or tabbed selector

### INVERSION_RANDOM
- **Type:** Float (AudioParameterFloat)
- **Range:** 0.0-1.0 (displayed as 0-100%)
- **Default:** 0.3
- **DSP Component:** Randomization
- **Description:** Chord inversion randomization probability
- **UI Control:** Slider

### TIMING_RANDOM
- **Type:** Float (AudioParameterFloat)
- **Range:** 0.0-100.0 (ms)
- **Default:** 10.0
- **DSP Component:** Randomization
- **Description:** Voice stagger timing window in milliseconds
- **UI Control:** Slider

### DETUNE_RANDOM
- **Type:** Float (AudioParameterFloat)
- **Range:** 0.0-50.0 (cents)
- **Default:** 5.0
- **DSP Component:** Randomization
- **Description:** Micro-detuning range per voice in cents
- **UI Control:** Slider

### WAVETABLE_POS
- **Type:** Float (AudioParameterFloat)
- **Range:** 0.0-1.0 (displayed as 0-100%)
- **Default:** 0.5
- **DSP Component:** Wavetable Oscillator
- **Description:** Base wavetable frame position
- **UI Control:** Rotary knob (large, prominent)

### LFO_RATE
- **Type:** Float (AudioParameterFloat)
- **Range:** 0.01-20.0 (Hz, exponential/skewed)
- **Default:** 0.5
- **Skew:** setSkewFactorFromMidPoint(1.0)
- **DSP Component:** LFO Modulation
- **Description:** LFO frequency
- **UI Control:** Rotary knob

### LFO_DEPTH
- **Type:** Float (AudioParameterFloat)
- **Range:** 0.0-1.0 (displayed as 0-100%)
- **Default:** 0.25
- **DSP Component:** LFO Modulation
- **Description:** LFO modulation depth to wavetable position
- **UI Control:** Rotary knob

### ATTACK_TIME
- **Type:** Float (AudioParameterFloat)
- **Range:** 0.001-5.0 (seconds, exponential/skewed)
- **Default:** 0.5
- **Skew:** setSkewFactorFromMidPoint(0.2)
- **DSP Component:** ADSR Envelope
- **Description:** Amplitude envelope attack time
- **UI Control:** Rotary knob or vertical slider

### RELEASE_TIME
- **Type:** Float (AudioParameterFloat)
- **Range:** 0.01-10.0 (seconds, exponential/skewed)
- **Default:** 2.0
- **Skew:** setSkewFactorFromMidPoint(1.0)
- **DSP Component:** ADSR Envelope
- **Description:** Amplitude envelope release time
- **UI Control:** Rotary knob or vertical slider

### FILTER_CUTOFF
- **Type:** Float (AudioParameterFloat)
- **Range:** 20.0-20000.0 (Hz, logarithmic)
- **Default:** 8000.0
- **Skew:** setSkewFactorFromMidPoint(1000.0)
- **DSP Component:** Low-Pass Filter
- **Description:** Filter cutoff frequency
- **UI Control:** Rotary knob

### MASTER_VOLUME
- **Type:** Float (AudioParameterFloat)
- **Range:** 0.0-1.26 (linear gain, 0 = -inf dB, 1.0 = 0dB, 1.26 = +6dB)
- **Default:** 1.0
- **DSP Component:** Output Gain
- **Description:** Final output level
- **UI Control:** Vertical fader
- **Note:** Use juce::Decibels::decibelsToGain for display

---

## Parameter IDs (C++ constants)

```cpp
namespace ParamIDs
{
    static const juce::String VOICE_COUNT     = "voiceCount";
    static const juce::String COMPLEXITY      = "complexity";
    static const juce::String KEY_ROOT        = "keyRoot";
    static const juce::String KEY_SCALE       = "keyScale";
    static const juce::String TUNING_SYSTEM   = "tuningSystem";
    static const juce::String INVERSION_RANDOM = "inversionRandom";
    static const juce::String TIMING_RANDOM   = "timingRandom";
    static const juce::String DETUNE_RANDOM   = "detuneRandom";
    static const juce::String WAVETABLE_POS   = "wavetablePos";
    static const juce::String LFO_RATE        = "lfoRate";
    static const juce::String LFO_DEPTH       = "lfoDepth";
    static const juce::String ATTACK_TIME     = "attackTime";
    static const juce::String RELEASE_TIME    = "releaseTime";
    static const juce::String FILTER_CUTOFF   = "filterCutoff";
    static const juce::String MASTER_VOLUME   = "masterVolume";
}
```

---

## APVTS Layout Creation

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Chord Generation
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParamIDs::VOICE_COUNT, "Voice Count", 2, 12, 5));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::COMPLEXITY, "Complexity",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::KEY_ROOT, "Key Root",
        juce::StringArray{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::KEY_SCALE, "Key Scale",
        juce::StringArray{"Major", "Minor", "Dorian", "Phrygian", "Lydian",
                          "Mixolydian", "Aeolian", "Locrian", "Harmonic Minor", "Melodic Minor"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::TUNING_SYSTEM, "Tuning System",
        juce::StringArray{"12-TET", "Just Intonation", "Pythagorean", "Historical", "Scala"}, 1));

    // Randomization
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::INVERSION_RANDOM, "Inversion Random",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::TIMING_RANDOM, "Timing Random",
        juce::NormalisableRange<float>(0.0f, 100.0f), 10.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::DETUNE_RANDOM, "Detune Random",
        juce::NormalisableRange<float>(0.0f, 50.0f), 5.0f));

    // Wavetable & Modulation
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::WAVETABLE_POS, "Wavetable Position",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    auto lfoRateRange = juce::NormalisableRange<float>(0.01f, 20.0f);
    lfoRateRange.setSkewFactorFromMidPoint(1.0f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::LFO_RATE, "LFO Rate", lfoRateRange, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::LFO_DEPTH, "LFO Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.25f));

    // Envelope
    auto attackRange = juce::NormalisableRange<float>(0.001f, 5.0f);
    attackRange.setSkewFactorFromMidPoint(0.2f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::ATTACK_TIME, "Attack", attackRange, 0.5f));

    auto releaseRange = juce::NormalisableRange<float>(0.01f, 10.0f);
    releaseRange.setSkewFactorFromMidPoint(1.0f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::RELEASE_TIME, "Release", releaseRange, 2.0f));

    // Filter
    auto filterRange = juce::NormalisableRange<float>(20.0f, 20000.0f);
    filterRange.setSkewFactorFromMidPoint(1000.0f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::FILTER_CUTOFF, "Filter Cutoff", filterRange, 8000.0f));

    // Master
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::MASTER_VOLUME, "Master Volume",
        juce::NormalisableRange<float>(0.0f, 1.26f), 1.0f));

    return { params.begin(), params.end() };
}
```

---

## Parameter Groupings (for UI organization)

### Chord Section
- VOICE_COUNT
- COMPLEXITY
- KEY_ROOT
- KEY_SCALE
- TUNING_SYSTEM

### Randomization Section
- INVERSION_RANDOM
- TIMING_RANDOM
- DETUNE_RANDOM

### Oscillator Section
- WAVETABLE_POS
- LFO_RATE
- LFO_DEPTH

### Envelope Section
- ATTACK_TIME
- RELEASE_TIME

### Filter Section
- FILTER_CUTOFF

### Output Section
- MASTER_VOLUME
