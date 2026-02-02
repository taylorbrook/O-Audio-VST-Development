# Stage 2: DSP Implementation - Research

**Phase:** Research
**Date:** 2026-02-01
**Status:** Complete

## JUCE Synthesiser Patterns

### SynthesiserVoice Interface

From Context7 JUCE documentation:

```cpp
struct BellVoice : public juce::SynthesiserVoice
{
    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound*, int pitchWheel) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newValue) override;
    void controllerMoved(int controllerNumber, int newValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override;
};
```

### Synthesiser Setup in Processor

```cpp
// In constructor
for (int i = 0; i < 8; ++i)
    synth.addVoice(new BellVoice());
synth.addSound(new BellSound());

// In prepareToPlay
synth.setCurrentPlaybackSampleRate(sampleRate);

// In processBlock
buffer.clear();
synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
```

## Reference Implementation: O-Marimba

O-Marimba uses a proven modal synthesis architecture:

### Structure
- `MarimbaVoice` extends `juce::SynthesiserVoice`
- 8 modal resonators per voice (NUM_MODES = 8)
- Biquad filters for each partial
- `MalletExciter` struct for strike transient
- Parameter setters for processor communication

### Key Patterns

**Modal Mode Struct:**
```cpp
struct ModalMode {
    float b0, a1, a2;  // Biquad coefficients
    float y1, y2;      // State
    float amplitude;
};
```

**Render Loop:**
```cpp
while (--numSamples >= 0) {
    float excitation = exciter.nextSample();
    float modalSum = 0.0f;
    for (auto& mode : modes) {
        modalSum += mode.processSample(excitation) * mode.amplitude;
    }
    outputBuffer.addSample(channel, startSample, finalSample);
    ++startSample;
}
```

## Bell-Specific Implementation

### Partial Ratio Tables (from ARCHITECTURE.md)

**Harmonic (inharmonicity = 0%):**
```cpp
{0.5, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}
```

**Church Bell (inharmonicity = 50%):**
```cpp
{0.5, 1.0, 2.4, 3.0, 4.0, 5.2, 6.0, 8.0}  // Minor third at 2.4x
```

**Gamelan (inharmonicity = 100%):**
```cpp
{0.5, 1.0, 2.1, 3.5, 4.8, 5.8, 7.2, 9.5}
```

### Strike Position Algorithm

Comb filter effect on partials:
```cpp
float strikePositionGain(int partialIndex, float position) {
    float phase = juce::MathConstants<float>::pi * position * (partialIndex + 1);
    return std::abs(std::sin(phase));
}
```

### Decay Time Calculation

Higher partials decay faster (natural bell behavior):
```cpp
const float DECAY_MULTIPLIERS[] = {1.2, 1.0, 0.85, 0.7, 0.6, 0.5, 0.4, 0.3};

float getDecayTime(int modeIndex, float damping, float material) {
    float baseDecay = 2.0f + damping * 8.0f;  // 2-10 seconds
    float materialMult = getMaterialDecayMultiplier(material);
    return baseDecay * DECAY_MULTIPLIERS[modeIndex] * materialMult;
}
```

### Material Decay Multipliers

```cpp
// Bronze=0.0, Steel=0.33, Glass=0.67, Crystal=1.0
float getMaterialDecayMultiplier(float material) {
    if (material < 0.33f) return 1.0f;           // Bronze: reference
    if (material < 0.67f) return 1.4f;           // Steel: longer
    if (material < 1.0f)  return 2.5f;           // Glass: much longer
    return 5.0f;                                  // Crystal: infinite-like
}
```

## Ensemble Voicing Implementation

### Unison Detune Distribution

Symmetric spread around fundamental:
```cpp
std::array<float, 4> calculateUnisonDetunes(int count, float detuneAmount) {
    std::array<float, 4> detunes = {0, 0, 0, 0};
    if (count == 1) return detunes;

    for (int i = 0; i < count; ++i) {
        float offset = (i - (count - 1) / 2.0f) / ((count - 1) / 2.0f);
        detunes[i] = offset * detuneAmount;  // cents
    }
    return detunes;
}
// Example: count=4, detune=50 → [-50, -16.7, +16.7, +50]
```

### Octave Layer Processing

```cpp
struct OctaveLayer {
    float frequencyMultiplier;  // 0.5 (sub), 1.0 (fund), 2.0 (oct)
    float blendLevel;           // 0.0-1.0
};

// Process layers in parallel, mix at end
float subOutput = renderVoice(freq * 0.5f) * subBlend;
float fundOutput = renderVoice(freq * 1.0f);
float octOutput = renderVoice(freq * 2.0f) * octBlend;
float total = subOutput + fundOutput + octOutput;
```

### Stereo Spread

```cpp
float calculatePan(int voiceIndex, int totalVoices, float spread) {
    if (totalVoices == 1) return 0.0f;  // Center
    float position = (voiceIndex - (totalVoices - 1) / 2.0f) / ((totalVoices - 1) / 2.0f);
    return position * spread;  // -1.0 to +1.0
}
```

## Sympathetic Resonance

### Algorithm (from ARCHITECTURE.md)

```cpp
void applySympathetic(int newNote, float intensity) {
    float newFreq = noteToFreq(newNote);

    for (auto& voice : activeVoices) {
        float ratio = newFreq / voice.frequency;

        // Check harmonic relationships: octave, fifth, unison
        if (isNearRatio(ratio, 0.5f, 0.05f) ||   // Octave below
            isNearRatio(ratio, 1.0f, 0.05f) ||   // Unison
            isNearRatio(ratio, 1.5f, 0.05f) ||   // Fifth
            isNearRatio(ratio, 2.0f, 0.05f)) {   // Octave above

            voice.addExcitation(intensity * 0.15f);  // 15% coupling max
        }
    }
}
```

## CPU Optimization Strategies

### SIMD with juce::dsp::Oscillator

```cpp
juce::dsp::Oscillator<float> osc;
osc.initialise([](float x) { return std::sin(x); });
osc.setFrequency(freq);
// Process in blocks for SIMD optimization
```

### Denormal Protection

```cpp
void renderNextBlock(...) {
    juce::ScopedNoDenormals noDenormals;
    // ...
}
```

### Early Exit for Silent Voices

```cpp
if (amplitude < 1e-8f) {
    clearCurrentNote();
    return;
}
```

## Files to Create

1. **BellSound.h** - Simple SynthesiserSound subclass
2. **BellVoice.h** - Voice class with modal synthesis
3. **BellVoice.cpp** - Voice implementation
4. **Modify PluginProcessor.cpp** - Add Synthesiser setup

## Implementation Order

1. **Phase 2.1:** BellVoice with single voice, 8 partials, basic envelope
2. **Phase 2.2:** 8-voice polyphony, strike dynamics, velocity curves
3. **Phase 2.3:** Ensemble voicing (unison, octave), sympathetic resonance

---

*Research completed: 2026-02-01*

## Sources

- JUCE Context7 documentation (/juce-framework/juce)
- O-Marimba implementation (plugins/O-Marimba/Source/)
- ARCHITECTURE.md (bell partial ratios, algorithms)
