# O-Bells Parameter Specification

**Plugin Type:** Synthesizer (Physical Modeling - Modal Synthesis)
**Total Parameters:** 21
**APVTS Required:** Yes

---

## Main Panel Parameters (7)

### STRIKE_POSITION
- **ID:** `strikePosition`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.5
- **Label:** "Strike"
- **Unit:** "%"
- **DSP Component:** Strike Dynamics Processor
- **Behavior:** Controls partial amplitude distribution via comb filter modeling. 0% = center strike (fundamental dominant), 100% = edge strike (upper partials emphasized, bright metallic)

### MALLET_HARDNESS
- **ID:** `malletHardness`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.5
- **Label:** "Mallet"
- **Unit:** "%"
- **DSP Component:** Strike Dynamics Processor
- **Behavior:** Controls spectral tilt and transient character. 0% = soft felt (dark, long attack 10ms), 100% = hard metal (bright, sharp attack 0.5ms)

### BELL_SIZE
- **ID:** `bellSize`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.5
- **Label:** "Size"
- **Unit:** "%"
- **DSP Component:** Modal Synthesis Engine
- **Behavior:** Scales fundamental frequency character and affects pitch envelope depth. 0% = small hand bell, 100% = large church bell

### DAMPING
- **ID:** `damping`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.7
- **Label:** "Damping"
- **Unit:** "%"
- **DSP Component:** Partial Envelope Generator
- **Behavior:** Controls decay time multiplier. 0% = hand-damped (short decay), 100% = free-ring (very long decay)

### BRIGHTNESS
- **ID:** `brightness`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.5
- **Label:** "Bright"
- **Unit:** "%"
- **DSP Component:** Modal Synthesis Engine
- **Behavior:** Global high-frequency emphasis. Scales upper partial amplitudes. 0% = dark/muted, 100% = brilliant/shimmering

### MATERIAL
- **ID:** `material`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.25
- **Label:** "Material"
- **Unit:** ""
- **DSP Component:** Material Morphing System
- **Behavior:** Continuous morph between material types. 0% = Bronze (warm, traditional), 33% = Steel (bright, modern), 67% = Glass (crystalline), 100% = Crystal (ethereal, pure)

### INHARMONICITY
- **ID:** `inharmonicity`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.5
- **Label:** "Inharm"
- **Unit:** "%"
- **DSP Component:** Modal Synthesis Engine
- **Behavior:** Deviation from harmonic series. 0% = pure harmonic, 50% = church bell ratios (minor third at 2.4x), 100% = gamelan/extreme inharmonicity

---

## Ensemble Section Parameters (4)

### UNISON_COUNT
- **ID:** `unisonCount`
- **Type:** Int
- **Range:** 1 - 4
- **Default:** 1
- **Label:** "Unison"
- **Unit:** ""
- **DSP Component:** Ensemble Voicing System
- **Behavior:** Number of detuned bell copies per voice. Higher values increase CPU proportionally.

### UNISON_DETUNE
- **ID:** `unisonDetune`
- **Type:** Float
- **Range:** 0.0 - 50.0
- **Default:** 10.0
- **Label:** "Detune"
- **Unit:** "cents"
- **DSP Component:** Ensemble Voicing System
- **Behavior:** Spread amount between unison voices. 0 = no detune (identical), 50 = maximum chorus effect

### OCTAVE_BLEND_SUB
- **ID:** `octaveBlendSub`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.0
- **Label:** "Sub"
- **Unit:** "%"
- **DSP Component:** Ensemble Voicing System
- **Behavior:** Sub-octave layer mix (-12 semitones). Adds depth and weight. Disabled when 0%.

### OCTAVE_BLEND_OCT
- **ID:** `octaveBlendOct`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.0
- **Label:** "Oct"
- **Unit:** "%"
- **DSP Component:** Ensemble Voicing System
- **Behavior:** Upper-octave layer mix (+12 semitones). Adds shimmer and brightness. Disabled when 0%.

### STEREO_SPREAD
- **ID:** `stereoSpread`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.5
- **Label:** "Spread"
- **Unit:** "%"
- **DSP Component:** Ensemble Voicing System
- **Behavior:** Stereo width of ensemble panning. 0% = mono (centered), 100% = wide stereo

---

## Advanced Panel Parameters (10)

### PARTIAL_TUNING
- **ID:** `partialTuning`
- **Type:** Float
- **Range:** -100.0 - 100.0
- **Default:** 0.0
- **Label:** "Partial Tune"
- **Unit:** "cents"
- **DSP Component:** Modal Synthesis Engine
- **Behavior:** Fine-tune the minor-third partial (~2.4x fundamental). Allows customization of bell character.

### NONLINEAR_EFFECTS
- **ID:** `nonlinearEffects`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.0
- **Label:** "Nonlinear"
- **Unit:** "%"
- **DSP Component:** Strike Dynamics Processor (WaveShaper)
- **Behavior:** Bell warping/distortion intensity at high velocity. 0% = clean, 100% = aggressive overtones

### SYMPATHETIC_RESONANCE
- **ID:** `sympatheticResonance`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.0
- **Label:** "Sympathetic"
- **Unit:** "%"
- **DSP Component:** Sympathetic Resonance Engine
- **Behavior:** Cross-voice coupling strength. Other voices ring when one strikes (harmonically related notes). Adds realism, increases CPU.

### STRIKE_NOISE_CHARACTER
- **ID:** `strikeNoiseChar`
- **Type:** Choice
- **Choices:** ["Click", "Thud", "Ping"]
- **Default:** 0 (Click)
- **Label:** "Noise"
- **Unit:** ""
- **DSP Component:** Strike Dynamics Processor
- **Behavior:** Transient texture type. Click = impulse (sharp), Thud = lowpass (soft), Ping = bandpass (metallic)

### DECAY_SHAPE
- **ID:** `decayShape`
- **Type:** Choice
- **Choices:** ["Linear", "Exponential", "Multi-stage"]
- **Default:** 1 (Exponential)
- **Label:** "Decay"
- **Unit:** ""
- **DSP Component:** Partial Envelope Generator
- **Behavior:** Envelope curve type. Linear = even fade, Exponential = natural bell decay, Multi-stage = complex realistic decay

### VELOCITY_CURVE
- **ID:** `velocityCurve`
- **Type:** Choice
- **Choices:** ["Linear", "Exponential", "Logarithmic"]
- **Default:** 0 (Linear)
- **Label:** "Velocity"
- **Unit:** ""
- **DSP Component:** Strike Dynamics Processor
- **Behavior:** MIDI velocity response shaping. Linear = 1:1 mapping, Exponential = sensitive at high velocities, Logarithmic = sensitive at low velocities

### PITCH_ENVELOPE
- **ID:** `pitchEnvelope`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.0
- **Label:** "Pitch Env"
- **Unit:** "%"
- **DSP Component:** Pitch Envelope
- **Behavior:** Initial pitch drop amount (characteristic of large bells). 0% = no pitch drop, 100% = up to 50 cents initial deviation

### PITCH_ENV_TIME
- **ID:** `pitchEnvTime`
- **Type:** Float
- **Range:** 5.0 - 200.0
- **Default:** 50.0
- **Label:** "P.Env Time"
- **Unit:** "ms"
- **DSP Component:** Pitch Envelope
- **Behavior:** Return time for pitch envelope. How quickly pitch settles to fundamental.

### OUTPUT_GAIN
- **ID:** `outputGain`
- **Type:** Float
- **Range:** -24.0 - 12.0
- **Default:** 0.0
- **Label:** "Output"
- **Unit:** "dB"
- **DSP Component:** Output Stage
- **Behavior:** Master output level adjustment

### QUALITY
- **ID:** `quality`
- **Type:** Choice
- **Choices:** ["Low", "Medium", "High"]
- **Default:** 2 (High)
- **Label:** "Quality"
- **Unit:** ""
- **DSP Component:** Modal Synthesis Engine
- **Behavior:** CPU vs quality tradeoff. Low = 4 partials, Medium = 6 partials, High = 8 partials

---

## APVTS Parameter Layout

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Main Panel
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"strikePosition", 1}, "Strike", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"malletHardness", 1}, "Mallet", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"bellSize", 1}, "Size", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"damping", 1}, "Damping", 0.0f, 1.0f, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"brightness", 1}, "Bright", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"material", 1}, "Material", 0.0f, 1.0f, 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"inharmonicity", 1}, "Inharm", 0.0f, 1.0f, 0.5f));

    // Ensemble Section
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"unisonCount", 1}, "Unison", 1, 4, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"unisonDetune", 1}, "Detune",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"octaveBlendSub", 1}, "Sub", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"octaveBlendOct", 1}, "Oct", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"stereoSpread", 1}, "Spread", 0.0f, 1.0f, 0.5f));

    // Advanced Panel
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"partialTuning", 1}, "Partial Tune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"nonlinearEffects", 1}, "Nonlinear", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"sympatheticResonance", 1}, "Sympathetic", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"strikeNoiseChar", 1}, "Noise",
        juce::StringArray{"Click", "Thud", "Ping"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"decayShape", 1}, "Decay",
        juce::StringArray{"Linear", "Exponential", "Multi-stage"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"velocityCurve", 1}, "Velocity",
        juce::StringArray{"Linear", "Exponential", "Logarithmic"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"pitchEnvelope", 1}, "Pitch Env", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"pitchEnvTime", 1}, "P.Env Time",
        juce::NormalisableRange<float>(5.0f, 200.0f, 1.0f, 0.5f), 50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"outputGain", 1}, "Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"quality", 1}, "Quality",
        juce::StringArray{"Low", "Medium", "High"}, 2));

    return {params.begin(), params.end()};
}
```

---

## Parameter Groups (for UI organization)

| Group | Parameters |
|-------|------------|
| **Main** | strikePosition, malletHardness, bellSize, damping, brightness, material, inharmonicity |
| **Ensemble** | unisonCount, unisonDetune, octaveBlendSub, octaveBlendOct, stereoSpread |
| **Advanced** | partialTuning, nonlinearEffects, sympatheticResonance, strikeNoiseChar, decayShape, velocityCurve, pitchEnvelope, pitchEnvTime, outputGain, quality |

---

## Contract Validation

- **Total Float Parameters:** 17
- **Total Int Parameters:** 1
- **Total Choice Parameters:** 4
- **Total Parameters:** 22 (21 functional + 1 quality setting)

**All parameters must:**
1. Be registered in APVTS during Stage 1 (Foundation)
2. Have atomic pointers cached in prepareToPlay()
3. Be bound to WebView UI in Stage 3 (GUI)

---

*Generated: 2026-02-01*
*Source: BRIEF.md + ARCHITECTURE.md*
