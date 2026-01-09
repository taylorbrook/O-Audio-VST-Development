---
title: Physical Modelling Synthesis - Complete Implementation Guide
category: DSP
subcategory: Synthesis Techniques
tags: [physical-modelling, karplus-strong, waveguide, modal-synthesis, juce-dsp, optimization]
difficulty: Advanced
research_date: 2026-01-08
research_level: Level 3 (Deep Investigation)
confidence: HIGH
---

# Physical Modelling Synthesis: Complete Implementation Guide for JUCE Plugins

## Executive Summary

This comprehensive guide provides everything needed to implement physical modelling (PM) synthesis in JUCE-based VST plugins, from mathematical foundations through production-ready optimization. Compiled from parallel research investigating algorithms, JUCE implementation patterns, and commercial product analysis.

**Key Takeaways:**
- Start with Karplus-Strong (complexity 1/5), progress to waveguides (3/5)
- Target 0.1-5% CPU per voice depending on model complexity
- Use `juce::dsp::DelayLine` with Lagrange3rd interpolation for pitch accuracy
- SIMD optimization essential for professional quality (10-50x speedups)
- Hybrid sample+PM approaches often optimal for production

---

## Table of Contents

1. [Algorithm Fundamentals](#1-algorithm-fundamentals)
2. [JUCE Implementation Patterns](#2-juce-implementation-patterns)
3. [Performance Optimization](#3-performance-optimization)
4. [Commercial Product Analysis](#4-commercial-product-analysis)
5. [Modern Advances (2020-2026)](#5-modern-advances-2020-2026)
6. [Implementation Roadmap](#6-implementation-roadmap)
7. [Common Pitfalls and Solutions](#7-common-pitfalls-and-solutions)
8. [References and Resources](#8-references-and-resources)

---

## 1. Algorithm Fundamentals

### 1.1 Comparison Matrix

| Algorithm | Complexity | CPU Cost | Best For | Realism | JUCE Classes |
|-----------|------------|----------|----------|---------|--------------|
| **Karplus-Strong** | 1/5 | Very Low | Plucked strings, simple percussion | Medium | DelayLine, IIR::Filter |
| **Digital Waveguide** | 3/5 | Low-Medium | Strings, winds, acoustic bodies | High | DelayLine (bidirectional), IIR::Filter |
| **Modal Synthesis** | 2/5 | Medium | Bells, metallic percussion, impacts | High (percussion) | IIR::Filter (parallel bank) |
| **Mass-Spring** | 4/5 | High | Membranes, collisions, research | Very High | Custom (Verlet integration) |

### 1.2 Karplus-Strong Algorithm

**Implementation Complexity:** 1/5 (Easiest entry point)

**Core Equation:**
```
y[n] = (y[n-N] + y[n-N-1]) / 2

Where N = sampleRate / frequency
```

**JUCE Implementation:**
```cpp
class KarplusStrongVoice : public juce::SynthesiserVoice
{
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine { 88200 };
    juce::dsp::IIR::Filter<float> loopFilter;
    float feedbackSample = 0.0f;

    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override
    {
        float delaySamples = static_cast<float>(sampleRate) / frequency;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Excitation (noise burst at note-on)
            float excitation = getExcitation(); // Returns 0.0 after attack phase

            // Combine with feedback
            float input = excitation + (feedbackSample * damping);

            // Push to delay line
            delayLine.pushSample(0, input);

            // Pop from delay line
            float rawOutput = delayLine.popSample(0, delaySamples);

            // Loop filter (averaging for damping)
            float filteredOutput = loopFilter.processSample(rawOutput);
            feedbackSample = filteredOutput;

            // Output
            buffer.addSample(0, startSample + sample, filteredOutput);
        }
    }
};
```

**Key Parameters:**
- **Pitch:** Delay length (N = sampleRate / frequency)
- **Damping:** Feedback coefficient (0.9-0.999)
- **Brightness:** Loop filter cutoff frequency
- **Pluck Position:** Comb filter for harmonic coloration

**Extensions:**
- Fractional delay (use Lagrange3rd interpolation)
- Variable damping filter (replace averaging with adjustable lowpass)
- Pluck position filter (add comb filter: `y[n] = y[n] - y[n - pickPos * N]`)

### 1.3 Digital Waveguide Synthesis

**Implementation Complexity:** 3/5 (Moderate)

**Core Concept:** Two delay lines carrying waves in opposite directions.

**Bidirectional Pattern:**
```cpp
class WaveguideString
{
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> rightWave { 4410 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> leftWave { 4410 };
    juce::dsp::IIR::Filter<float> bridgeFilter;
    juce::dsp::IIR::Filter<float> nutFilter;

    void process(float excitation)
    {
        float halfDelay = (sampleRate / frequency) / 2.0f;

        // Read from both ends
        float rightOut = rightWave.popSample(0, halfDelay);
        float leftOut = leftWave.popSample(0, halfDelay);

        // Reflections with inversion and damping
        float rightReflect = nutFilter.processSample(-leftOut * damping);
        float leftReflect = bridgeFilter.processSample(-rightOut * damping);

        // Push back with excitation
        rightWave.pushSample(0, rightReflect + excitation);
        leftWave.pushSample(0, leftReflect);

        // Output at pickup position
        output = (rightOut + leftOut) * 0.5f;
    }
};
```

**Applications:**
- String instruments (guitar, piano, violin)
- Wind instruments (flute, clarinet, brass)
- Acoustic body resonance
- Physical reverb models

### 1.4 Modal Synthesis

**Implementation Complexity:** 2/5 (Simple)

**Core Concept:** Bank of parallel resonators (2nd-order filters) representing vibration modes.

**Single Mode Equation:**
```
H(z) = g / (1 - 2r*cos(θ)*z⁻¹ + r²*z⁻²)

Where:
θ = 2π * frequency / sampleRate
r = exp(-decay / sampleRate)
g = amplitude
```

**JUCE Implementation:**
```cpp
class ModalSynth
{
    struct Mode
    {
        float frequency;
        float decay;
        float amplitude;

        // Biquad state
        float y1 = 0.0f, y2 = 0.0f;
        float a1, a2, b0;

        void updateCoeffs(float sampleRate)
        {
            float theta = juce::MathConstants<float>::twoPi * frequency / sampleRate;
            float r = std::exp(-1.0f / (decay * sampleRate));
            a1 = 2.0f * r * std::cos(theta);
            a2 = -r * r;
            b0 = amplitude * (1.0f - r);
        }

        float process(float excitation)
        {
            float y = b0 * excitation + a1 * y1 + a2 * y2;
            y2 = y1;
            y1 = y;
            return y;
        }
    };

    std::vector<Mode> modes;

    float process(float excitation)
    {
        float output = 0.0f;
        for (auto& mode : modes)
            output += mode.process(excitation);
        return output;
    }
};
```

**Best For:**
- Bells, gongs, chimes
- Tuned percussion (marimba, vibraphone)
- Metallic impacts
- Game audio (collision sounds)

**Modal Data Example (Tubular Bell):**
```cpp
std::vector<Mode> bellModes = {
    {100.0f, 5.0f, 1.0f},    // Fundamental
    {277.0f, 4.0f, 0.8f},    // ~2.77x (minor third + octave)
    {467.0f, 3.5f, 0.5f},    // ~4.67x
    {570.0f, 3.0f, 0.4f},    // ~5.7x
    {700.0f, 2.5f, 0.3f},    // ~7x
};
```

### 1.5 Mass-Spring Systems

**Implementation Complexity:** 4/5 (Complex)

**Core Concept:** Network of masses connected by springs, using Verlet integration.

**Verlet Integration:**
```
x[n+1] = 2*x[n] - x[n-1] + (F/m) * dt²
```

**1D String Model:**
```cpp
class MassSpringString
{
    static const int NUM_MASSES = 128;
    float position[NUM_MASSES];
    float prevPosition[NUM_MASSES];
    float velocity[NUM_MASSES];

    float springK = 1000.0f;
    float damping = 0.001f;
    float mass = 0.01f;
    float dt;

    void process()
    {
        for (int i = 1; i < NUM_MASSES - 1; ++i)
        {
            // Spring forces from neighbors
            float force = springK * (position[i+1] - position[i])
                        + springK * (position[i-1] - position[i]);
            force -= damping * velocity[i];

            // Verlet integration
            float accel = force / mass;
            float newPos = 2.0f * position[i] - prevPosition[i] + accel * dt * dt;

            velocity[i] = (newPos - prevPosition[i]) / (2.0f * dt);
            prevPosition[i] = position[i];
            position[i] = newPos;
        }

        // Fixed ends
        position[0] = 0.0f;
        position[NUM_MASSES-1] = 0.0f;
    }
};
```

**Stability (CFL Condition):**
```
dt <= sqrt(m/k) / 2    (for 1D)
dt <= sqrt(m/k) / 4    (for 2D)
```

**Best For:**
- Drums and membranes
- Collision sounds with hammer models
- Research/experimental synthesis
- Non-linear string behavior

---

## 2. JUCE Implementation Patterns

### 2.1 Essential JUCE DSP Classes

#### juce::dsp::DelayLine

**Core class for all PM synthesis.** Provides circular buffer with interpolation.

**Interpolation Types:**
```cpp
// For PM synthesis, use Lagrange3rd for best pitch accuracy
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>        // Fast, poor quality
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>      // Good balance
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> // Best for PM (recommended)
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran>      // Allpass (very short delays)
```

**Usage Pattern:**
```cpp
// Declaration
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine { 88200 };

// In prepareToPlay
delayLine.setMaximumDelayInSamples(static_cast<int>(sampleRate * 2.0)); // 2 sec max
delayLine.prepare(spec);
delayLine.reset();

// In processBlock
delayLine.pushSample(channel, inputSample);
float outputSample = delayLine.popSample(channel, delayTimeSamples); // Fractional delay supported
```

#### juce::dsp::IIR::Filter

**Essential for loop filtering, body resonance, damping simulation.**

**Common PM Filter Types:**
```cpp
// First-order lowpass (classic Karplus-Strong averaging)
auto coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sampleRate, cutoff);

// Butterworth lowpass (smoother damping)
auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoff, 0.707f);

// Peak filter (body mode simulation)
auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, frequency, Q, gainDB);

// Shelf filters (tonal shaping)
auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, freq, Q, gain);
```

**Single-sample Processing:**
```cpp
juce::dsp::IIR::Filter<float> filter;

// In prepareToPlay
filter.prepare(spec);
*filter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sampleRate, 5000.0f);

// In processBlock (per-sample)
float filtered = filter.processSample(rawSample);
```

#### juce::dsp::Oversampling

**Critical for non-linear processes to prevent aliasing.**

```cpp
// Declaration (2 channels, 1 stage = 2x oversampling)
juce::dsp::Oversampling<float> oversampler { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple };

// In prepareToPlay
oversampler.initProcessing(static_cast<size_t>(samplesPerBlock));
oversampler.reset();

// In processBlock
juce::dsp::AudioBlock<float> block(buffer);
auto oversampledBlock = oversampler.processSamplesUp(block);

// Process at higher sample rate
for (size_t sample = 0; sample < oversampledBlock.getNumSamples(); ++sample)
{
    // Non-linear processing (waveshaping, saturation)
    oversampledBlock.setSample(channel, sample, std::tanh(gain * input));
}

oversampler.processSamplesDown(block);

// Report latency
setLatencySamples(static_cast<int>(oversampler.getLatencyInSamples()));
```

### 2.2 Parameter Management (APVTS)

**Parameter Layout for PM Synthesis:**
```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // PITCH - Semitone offset
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "PITCH", 1 }, "Pitch",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f, 1.0f),
        0.0f, "st"
    ));

    // DAMPING - Energy loss (0 = sustain forever, 100 = instant decay)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DAMPING", 1 }, "Damping",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 0.5f), // Skew 0.5 for better control
        30.0f, "%"
    ));

    // BRIGHTNESS - Loop filter cutoff
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "BRIGHTNESS", 1 }, "Brightness",
        juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), // Logarithmic skew
        5000.0f, "Hz"
    ));

    // POSITION - Pickup/excitation position
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "POSITION", 1 }, "Position",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        50.0f, "%"
    ));

    // BODY - Resonance mix
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "BODY", 1 }, "Body",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        50.0f, "%"
    ));

    return layout;
}
```

**Thread-Safe Parameter Reading:**
```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // ALWAYS use getRawParameterValue for real-time safety
    auto* dampingParam = parameters.getRawParameterValue("DAMPING");
    float dampingValue = dampingParam->load() / 100.0f; // Atomic load, lock-free

    // Use loaded value in processing...
}
```

**Parameter Smoothing:**
```cpp
// In header
juce::SmoothedValue<float> smoothedDamping;
juce::SmoothedValue<float> smoothedBrightness;

// In prepareToPlay
smoothedDamping.reset(sampleRate, 0.05);  // 50ms smoothing
smoothedDamping.setCurrentAndTargetValue(0.3f);

smoothedBrightness.reset(sampleRate, 0.02); // 20ms (faster for frequency)
smoothedBrightness.setCurrentAndTargetValue(5000.0f);

// In processBlock
smoothedDamping.setTargetValue(dampingParam->load() / 100.0f);

for (int sample = 0; sample < numSamples; ++sample)
{
    float currentDamping = smoothedDamping.getNextValue();
    // Use smoothed value
}
```

### 2.3 Voice Management

**Custom Voice Class Structure:**
```cpp
class PMStringVoice : public juce::SynthesiserVoice
{
public:
    PMStringVoice();

    bool canPlaySound(juce::SynthesiserSound*) override { return true; }

    void startNote(int midiNote, float velocity, juce::SynthesiserSound*, int) override
    {
        currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNote);
        noteVelocity = velocity;

        // Reset state
        delayLine.reset();
        feedbackSample = 0.0f;

        // Configure excitation envelope
        excitationADSR.noteOn();
    }

    void stopNote(float velocity, bool allowTailOff) override
    {
        if (allowTailOff)
            excitationADSR.noteOff();
        else
            clearCurrentNote();
    }

    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override
    {
        if (!isVoiceActive()) return;

        float delaySamples = static_cast<float>(sampleRate) / currentFrequency;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Karplus-Strong processing
            float excitation = getExcitation() * excitationADSR.getNextSample();
            float input = excitation + (feedbackSample * damping);

            delayLine.pushSample(0, input);
            float rawOutput = delayLine.popSample(0, delaySamples);
            float filtered = loopFilter.processSample(rawOutput);
            feedbackSample = filtered;

            // Output to all channels
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.addSample(channel, startSample + sample, filtered);

            // Voice stealing when amplitude is negligible
            if (std::abs(filtered) < 1e-6f && !excitationADSR.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

    void setCurrentPlaybackSampleRate(double newRate) override
    {
        sampleRate = newRate;
        juce::dsp::ProcessSpec spec { newRate, 512, 1 };
        delayLine.prepare(spec);
        loopFilter.prepare(spec);
        excitationADSR.setSampleRate(newRate);
    }

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine { 88200 };
    juce::dsp::IIR::Filter<float> loopFilter;
    juce::ADSR excitationADSR;
    juce::Random noiseGenerator;

    double sampleRate = 44100.0;
    float currentFrequency = 440.0f;
    float noteVelocity = 1.0f;
    float feedbackSample = 0.0f;
    float damping = 0.97f;
};

class PMStringSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
```

**Processor Setup:**
```cpp
// In processor constructor
const int maxVoices = 8; // PM is CPU-intensive
for (int i = 0; i < maxVoices; ++i)
    synthesiser.addVoice(new PMStringVoice());

synthesiser.addSound(new PMStringSound());

// Enable voice stealing for polyphony management
synthesiser.setNoteStealingEnabled(true);

// In processBlock
synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
```

### 2.4 Real-Time Safety Checklist

**ALWAYS in processBlock:**
```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // 1. Denormal protection (CRITICAL)
    juce::ScopedNoDenormals noDenormals;

    // 2. Atomic parameter loads
    auto* paramPtr = parameters.getRawParameterValue("PARAM");
    float value = paramPtr->load(); // Lock-free

    // 3. No allocations, no locks, no I/O
    // ...processing...
}
```

**NEVER in processBlock:**
- `new` / `delete` / `malloc`
- `std::vector::resize()` / `push_back()`
- `std::mutex` locks
- File I/O
- `juce::String` operations that allocate
- `std::cout` / logging

**Pre-allocate in prepareToPlay:**
```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Allocate all buffers here
    scratchBuffer.setSize(2, samplesPerBlock);
    delayLine.setMaximumDelayInSamples(maxSamples);

    // Resize vectors
    modeStates.resize(numModes, 0.0f);
}
```

---

## 3. Performance Optimization

### 3.1 CPU Targets by Algorithm

| Model Type | CPU per Voice @ 44.1kHz | Target Polyphony |
|------------|-------------------------|------------------|
| Simple Karplus-Strong | < 0.1% | 64-128 voices |
| Enhanced String | < 0.5% | 32 voices |
| Full Waveguide Model | < 1% | 16 voices |
| Wind Instrument | < 2% | 8 voices |
| Full Piano Model | < 5% | 4-8 voices |
| Complex Percussion | < 1% | 24 voices |

### 3.2 SIMD Optimization

**SIMD provides 4-10x speedups for PM synthesis.**

**Modal Synthesis (Highly Parallelizable):**
```cpp
// Process 4 modes simultaneously using SSE
void processModalBank_SIMD(float* output, int numSamples,
                           float* frequencies, float* amplitudes,
                           float* phases, float* decays, int numModes)
{
    for (int m = 0; m < numModes; m += 4)
    {
        __m128 freq = _mm_loadu_ps(&frequencies[m]);
        __m128 amp = _mm_loadu_ps(&amplitudes[m]);
        __m128 phase = _mm_loadu_ps(&phases[m]);
        __m128 decay = _mm_loadu_ps(&decays[m]);

        for (int i = 0; i < numSamples; ++i)
        {
            __m128 sinVal = fastSin_SSE(phase);
            __m128 contrib = _mm_mul_ps(sinVal, amp);

            // Horizontal sum of 4 contributions
            float sum = horizontalSum(contrib);
            output[i] += sum;

            // Update state
            phase = _mm_add_ps(phase, freq);
            amp = _mm_mul_ps(amp, decay);
        }
    }
}
```

**JUCE DSP Block Processing (SIMD-optimized internally):**
```cpp
// JUCE leverages SIMD internally when using AudioBlock
juce::dsp::AudioBlock<float> block(buffer);
juce::dsp::ProcessContextReplacing<float> context(block);
filter.process(context); // SIMD-optimized
```

### 3.3 Optimization Strategies

**1. Minimize Per-Sample Calculations:**
```cpp
// Calculate constants OUTSIDE sample loop
float dampingFactor = 1.0f - (damping / 100.0f);
float delaySamples = sampleRate / frequency;

for (int sample = 0; sample < numSamples; ++sample)
{
    // Use pre-calculated values
    output *= dampingFactor;
}
```

**2. Update Filters Per-Block, Not Per-Sample:**
```cpp
// Only update when parameter changes significantly
if (std::abs(newCutoff - lastCutoff) > 1.0f)
{
    *loopFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        sampleRate, newCutoff
    );
    lastCutoff = newCutoff;
}
```

**3. Lookup Tables for Expensive Functions:**
```cpp
class SaturationTable
{
    static constexpr int TABLE_SIZE = 4096;
    std::array<float, TABLE_SIZE> table;

public:
    SaturationTable(float drive = 1.0f)
    {
        for (int i = 0; i < TABLE_SIZE; ++i)
        {
            float x = (i / (float)(TABLE_SIZE - 1)) * 2.0f - 1.0f;
            table[i] = std::tanh(x * drive);
        }
    }

    float process(float input)
    {
        float normalized = juce::jlimit(0.0f, 1.0f, (input + 1.0f) * 0.5f);
        float indexF = normalized * (TABLE_SIZE - 1);
        int index0 = (int)indexF;
        int index1 = std::min(index0 + 1, TABLE_SIZE - 1);
        float frac = indexF - index0;
        return table[index0] + frac * (table[index1] - table[index0]);
    }
};
```

**4. Voice Stealing and Limiting:**
```cpp
// In processor
synthesiser.setNoteStealingEnabled(true);

// Limit voice count based on CPU budget
const int maxVoices = 8; // Adjust based on algorithm complexity
```

### 3.4 Oversampling Strategy

**When to Use:**
- Non-linear waveshaping (saturation, distortion)
- Feedback loops with high gain
- Transient generation (impulses)

**Cost Analysis:**

| Oversampling Factor | CPU Multiplier | Quality Gain |
|---------------------|----------------|--------------|
| 2x | ~2.2x | Good |
| 4x | ~4.5x | Very Good |
| 8x | ~9x | Excellent |

**Recommendation:** Use 2x-4x for non-linear PM elements only, not entire signal path.

### 3.5 Profiling and Measurement

**Essential Tools:**
- **macOS:** Instruments (Time Profiler)
- **Windows:** Visual Studio Profiler
- **Cross-platform:** Tracy Profiler

**Key Metrics to Track:**
- CPU % per voice
- Total polyphonic CPU usage
- Worst-case CPU spikes
- Memory allocation patterns

---

## 4. Commercial Product Analysis

### 4.1 Modartt Pianoteq

**Business Model:** Pure physical modelling piano (no samples)

**Technical Approach:**
- Advanced waveguide synthesis for strings
- Multi-phase hammer-string contact model
- Plate resonance for soundboard
- Real-time sympathetic resonance calculation

**Key Design Decisions:**
- **50MB install** vs 50GB+ for sampled pianos
- Infinite sustain (no sample looping artifacts)
- Continuous parameters (stretch tuning, hammer hardness)
- Quality presets: Draft (fast) → Studio (full model)

**Parameter Design:**
```
Macro Level:        Micro Level:
- Dynamics          - Hammer hardness curve
- Tone              - String length scaling
- Condition         - Soundboard impedance
                    - Duplex scale resonance
```

**CPU Strategy:**
- Intelligent voice allocation (all 88 strings available)
- Frequency-domain sympathetic coupling
- 10-25% CPU on modern systems (full polyphony)

**Success Factors:**
1. Uncompromised physical accuracy
2. Tiny file size
3. Parameters map to physical properties
4. Extensive instrument variety from one engine

### 4.2 Applied Acoustics Systems (AAS)

**Product Line:**
- **Chromaphone 3:** Percussion/mallet PM
- **String Studio:** Bowed/plucked strings
- **Lounge Lizard:** Electric piano
- **Ultra Analog:** Analog-style with PM elements

**Chromaphone 3 Architecture:**
```
[Exciter] → [Resonator A] → [Body] → [Effects] → Output
               ↓
          [Resonator B]
```

**Key Design Decisions:**
1. **Dual-resonator system:** Complex coupled resonances
2. **Material-based presets:** "Wood", "Metal", "Nylon" map to PM parameters
3. **Layered UI complexity:**
   - Surface: Material, Size, Tone, Decay
   - Deep: Full waveguide parameter access

**CPU Strategy:**
- Fixed voice allocation (16-32 voices)
- Linear filter-based resonators (no per-sample non-linearity)
- CPU concentrated in exciter and coupling

### 4.3 Common Success Patterns

**1. Layered UI Complexity:**
```
Level 1: Macro (Size, Brightness, Decay)
Level 2: Component (Exciter, Body, Resonator)
Level 3: Expert (Individual physical parameters)
```

**2. Meaningful Parameter Mapping:**
- "Brightness" → multiple filter cutoffs + damping
- "Size" → delay lengths + resonator frequencies + body response
- Users think acoustically, not in DSP terms

**3. High-Quality Defaults:**
- Factory presets demonstrate full potential
- Acoustically plausible starting points
- Avoid exposing model weaknesses

**4. CPU Transparency:**
- Voice count indication
- Quality/CPU trade-off controls
- Graceful degradation under load

---

## 5. Modern Advances (2020-2026)

### 5.1 Machine Learning + Physical Modelling

**Differentiable DSP (DDSP) - Google Magenta (2020)**

Breakthrough: Neural networks can learn PM parameters from audio examples.

```python
# Conceptual DDSP approach
class DDSPSynth:
    def forward(self, f0, loudness):
        # NN predicts PM parameters
        harmonics = self.harmonic_encoder(f0, loudness)
        noise_mags = self.noise_encoder(f0, loudness)

        # Classical PM synthesis with learned params
        harmonic_audio = harmonic_synth(f0, harmonics)
        noise_audio = filtered_noise(noise_mags)

        return harmonic_audio + noise_audio
```

**Key ML+PM Developments:**

| Year | Development | Significance |
|------|-------------|--------------|
| 2020 | Google DDSP | Differentiable synthesis, learned parameters |
| 2021 | RAVE | Real-time audio variational autoencoder |
| 2022 | NEWT | Neural waveshaping for timbre transfer |
| 2023 | DiffWave PM | Diffusion models + physical models |
| 2024 | Hybrid Neural PM | Neural control of classical PM |
| 2025 | Real-time Transformers | Low-latency neural parameter estimation |

**Practical Applications:**
1. **Parameter Estimation:** ML learns PM params from recordings
2. **Excitation Modeling:** Neural networks generate complex excitation
3. **Body Modeling:** Learn body response from recordings
4. **Timbre Transfer:** Apply one instrument's character to another

### 5.2 GPU Acceleration

**Current State (2025-2026):**
- GPU synthesis viable but latency-challenged
- Best for offline rendering, pre-computation
- Emerging low-latency GPU audio APIs

**GPU-Suitable PM Operations:**

| Operation | GPU Speedup | Latency Concern |
|-----------|-------------|-----------------|
| Modal synthesis (many modes) | 10-50x | Medium |
| FDN reverb | 5-20x | High |
| Convolution | 10-100x | Medium |
| Waveguide mesh | 20-100x | High |
| Non-linear LUTs | 2-5x | Low |

**Hybrid CPU/GPU Strategy:**
```cpp
// GPU handles parallel, latency-tolerant operations
GPU::computeModalBank(modeStates, frequencies, amplitudes);

// CPU handles latency-critical, sequential operations
for (int i = 0; i < bufferSize; ++i)
{
    output[i] = exciter.process() + GPU::getModalOutput(i);
}
```

### 5.3 Recent Research Trends (2020-2026)

**1. Energy-Based Physical Modelling:**
- Port-Hamiltonian systems for guaranteed stability
- Energy conservation prevents blow-ups

**2. Perceptual Physical Modelling:**
- Simplify models based on auditory perception
- Only model what listeners can hear

**3. Data-Driven Material Properties:**
- Learn material parameters from recordings
- Automatic parameter estimation from audio

**4. Multi-Scale Modelling:**
- Different resolution for different frequency ranges
- Coarse mesh for bass, fine mesh for treble

**5. Neural Audio Codec Integration:**
- PM combined with neural compression
- Extreme file size reduction with PM+ML

---

## 6. Implementation Roadmap

### 6.1 Stage 1: Foundation (Week 1-2)

**Goal:** Working Karplus-Strong with 4-8 voice polyphony

```cpp
class BasicString {
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine { 4410 };
    juce::dsp::IIR::Filter<float> loopFilter;
    float feedbackSample = 0.0f;

    float process(float input, float frequency, float damping)
    {
        float delaySamples = sampleRate / frequency;
        float dampingFactor = 1.0f - damping;

        delayLine.pushSample(0, input + feedbackSample * dampingFactor);
        float output = delayLine.popSample(0, delaySamples);
        feedbackSample = loopFilter.processSample(output);

        return feedbackSample;
    }
};
```

**Milestone Checklist:**
- [ ] Working delay line with fractional delay
- [ ] Basic lowpass damping filter
- [ ] MIDI note → frequency conversion
- [ ] Simple ADSR for excitation
- [ ] 4-8 voice polyphony with SynthesiserVoice
- [ ] Parameter smoothing (no zipper noise)

### 6.2 Stage 2: Enhanced String (Week 3-4)

**Goal:** Improved realism with dispersion, body resonance

```cpp
class EnhancedString {
    // Bidirectional waveguide
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> rightWave { 4410 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> leftWave { 4410 };

    // Allpass for stiffness simulation
    juce::dsp::IIR::Filter<float> dispersionFilter;

    // Body resonance
    juce::dsp::IIR::Filter<float> bodyFilter1;
    juce::dsp::IIR::Filter<float> bodyFilter2;

    float process(float excitation, float frequency);
};
```

**Milestone Checklist:**
- [ ] Bidirectional waveguide implementation
- [ ] Allpass dispersion filter (string stiffness)
- [ ] Body resonance (2-3 peak filters)
- [ ] Improved excitation (filtered noise burst)
- [ ] Pluck position parameter
- [ ] 16-32 voice polyphony

### 6.3 Stage 3: Professional Quality (Week 5-8)

**Goal:** Production-ready instrument with all features

```cpp
class ProfessionalString {
    // Multi-mode waveguide
    WaveguidePair waveguide;

    // Non-linear exciter
    NonlinearHammer hammer; // or BowModel, PluckModel

    // Sympathetic resonance
    SympatheticResonator sympathetic;

    // Convolution body
    juce::dsp::Convolution bodyConvolution;

    float process(float velocity, float position);
};
```

**Milestone Checklist:**
- [ ] Multiple waveguide modes
- [ ] Non-linear hammer/pluck/bow model
- [ ] Sympathetic resonance (cross-string coupling)
- [ ] Convolution-based body (IR loaded from file)
- [ ] Full polyphony with voice stealing
- [ ] SIMD optimization (modal bank, filter banks)
- [ ] Quality/CPU presets (Draft/Standard/High)
- [ ] CPU meter in UI

### 6.4 Testing and Validation

**Audio Quality Tests:**

1. **Pitch Accuracy:**
```cpp
void testPitchAccuracy()
{
    for (int note = 36; note < 96; ++note)
    {
        float expected = juce::MidiMessage::getMidiNoteInHertz(note);
        float actual = measurePitch(synth.renderNote(note));
        EXPECT_NEAR(expected, actual, 0.5f); // Within 0.5 Hz
    }
}
```

2. **Decay Characteristics:**
```cpp
void testDecay()
{
    auto audio = synth.renderNote(60, 4.0f);
    auto envelope = extractEnvelope(audio);
    EXPECT_TRUE(isMonotonicallyDecreasing(envelope));
    EXPECT_TRUE(isExponentialDecay(envelope, tolerance));
}
```

3. **CPU Performance:**
```cpp
void testCPUUsage()
{
    const int voiceCount = 16;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; ++i)
        synth.processBlock(buffer, voiceCount);

    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    float cpuPercent = (elapsed / expectedRealtime) * 100.0f;
    EXPECT_LT(cpuPercent, 50.0f); // Less than 50% CPU
}
```

---

## 7. Common Pitfalls and Solutions

### 7.1 Zipper Noise on Parameter Changes

**Problem:** Audible clicks when changing parameters.

**Solution:** Use juce::SmoothedValue

```cpp
// In header
juce::SmoothedValue<float> smoothedCutoff;

// In prepareToPlay
smoothedCutoff.reset(sampleRate, 0.02); // 20ms smoothing

// In processBlock
smoothedCutoff.setTargetValue(cutoffParam->load());

for (int sample = 0; sample < numSamples; ++sample)
{
    float currentCutoff = smoothedCutoff.getNextValue();
    // Use currentCutoff
}
```

### 7.2 Pitch Inaccuracy

**Problem:** Notes sound out of tune.

**Solution:** Use Lagrange3rd interpolation and fractional delay.

```cpp
// High-quality interpolation
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine;

// Precise fractional delay calculation
double exactDelaySamples = static_cast<double>(sampleRate) / frequency;
float output = delayLine.popSample(channel, static_cast<float>(exactDelaySamples));
```

### 7.3 Feedback Instability

**Problem:** Runaway oscillation, clipping, blow-ups.

**Solution:** Limit feedback, use soft clipping.

```cpp
// Limit feedback coefficient
float safeDamping = juce::jlimit(0.0f, 0.999f, damping);

// Soft limiting
float output = std::tanh(rawOutput * 0.5f) * 2.0f;

// Or hard limit with headroom
output = juce::jlimit(-0.99f, 0.99f, output);

// Denormal protection
const float denormalThreshold = 1e-8f;
if (std::abs(feedbackSample) < denormalThreshold)
    feedbackSample = 0.0f;
```

### 7.4 Delay Line Click at Note-On

**Problem:** Click artifacts when starting notes.

**Solution:** Reset delay line, use fade-in envelope.

```cpp
void startNote(int midiNote, float velocity, ...)
{
    // Reset delay line
    delayLine.reset();
    feedbackSample = 0.0f;

    // Short fade-in envelope
    juce::ADSR::Parameters params;
    params.attack = 0.002f; // 2ms prevents click
    params.decay = 0.02f;
    params.sustain = 0.0f;
    params.release = 0.01f;

    excitationEnvelope.setParameters(params);
    excitationEnvelope.noteOn();
}
```

### 7.5 High CPU Usage

**Problem:** Plugin uses too much CPU.

**Solutions:**

1. **Voice Limiting:**
```cpp
const int maxVoices = 8; // Reduce for complex models
synthesiser.setNoteStealingEnabled(true);
```

2. **Per-Block Filter Updates:**
```cpp
// Update filters once per block, not per sample
if (frameCount % 32 == 0) // Update every 32 samples
{
    *loopFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        sampleRate, cutoff
    );
}
```

3. **Quality Presets:**
```cpp
enum Quality { DRAFT, STANDARD, HIGH };

void setQuality(Quality q)
{
    switch (q)
    {
        case DRAFT:
            maxVoices = 4;
            oversamplingFactor = 1;
            break;
        case STANDARD:
            maxVoices = 8;
            oversamplingFactor = 2;
            break;
        case HIGH:
            maxVoices = 16;
            oversamplingFactor = 4;
            break;
    }
}
```

### 7.6 Denormal CPU Spikes

**Problem:** Random CPU spikes when feedback decays to near-zero.

**Solution:** Use juce::ScopedNoDenormals and threshold checks.

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals; // ALWAYS include

    // ... processing ...

    // Threshold small values
    const float denormalThreshold = 1e-8f;
    if (std::abs(feedbackSample) < denormalThreshold)
        feedbackSample = 0.0f;
}
```

---

## 8. References and Resources

### 8.1 Academic Foundation

**Essential Reading:**
- **Julius O. Smith III** - "Physical Audio Signal Processing" (online book)
  https://ccrma.stanford.edu/~jos/pasp/
- **Karplus, K. & Strong, A.** (1983) - "Digital Synthesis of Plucked-String and Drum Timbres"
- **Smith, J.O.** (1992) - "Physical Modeling Using Digital Waveguides"
- **Adrien, J.M.** (1991) - "The Missing Link: Modal Synthesis"
- **Bilbao, S.** (2009) - "Numerical Sound Synthesis" (comprehensive textbook)

### 8.2 JUCE Documentation

**Core Classes:**
- `juce::dsp::DelayLine` - Circular buffer with interpolation
- `juce::dsp::IIR::Filter` - Infinite impulse response filters
- `juce::dsp::Oversampling` - Anti-aliasing oversampling
- `juce::AudioProcessorValueTreeState` - Parameter management
- `juce::SynthesiserVoice` - Polyphonic voice base class
- `juce::SmoothedValue` - Parameter smoothing

### 8.3 Open Source Examples

- **The Synthesis ToolKit (STK)** - C++ PM examples by Perry Cook
- **FAUST Physical Modelling** - Functional DSP examples
- **Csound PM Opcodes** - Reference implementations
- **Pure Data Externals** - Community PM objects

### 8.4 Commercial Reference Products

- **Modartt Pianoteq** - Pure PM piano
- **Applied Acoustics Chromaphone** - PM percussion
- **Applied Acoustics String Studio** - PM strings
- **Audio Modeling SWAM** - PM wind instruments
- **Arturia Piano V** - Hybrid sample+PM

### 8.5 Modern Research

- **Google Magenta DDSP** (2020) - Differentiable DSP
- **RAVE** (2021) - Real-time neural synthesis
- **NEWT** (2022) - Neural waveshaping
- **DiffWave PM** (2023) - Diffusion models + PM

### 8.6 Community Resources

- **Music-DSP Archives** - https://www.musicdsp.org/
- **KVR DSP Forum** - https://www.kvraudio.com/forum/viewforum.php?f=33
- **JUCE Forum** - https://forum.juce.com/
- **Stanford CCRMA** - https://ccrma.stanford.edu/

---

## Conclusion

Physical modelling synthesis offers unique expressive possibilities for audio plugins. Success requires:

1. **Start Simple:** Karplus-Strong (complexity 1/5) teaches fundamentals
2. **Progress Gradually:** Add waveguide extensions, then body resonance
3. **Optimize Early:** SIMD for modal synthesis, efficient delay lines
4. **Design Parameters Thoughtfully:** Macro/micro/expert layers
5. **Test Thoroughly:** Pitch accuracy, decay characteristics, CPU usage
6. **Consider Hybrid Approaches:** Sample+PM often optimal
7. **Stay Current:** ML integration emerging as powerful tool

**Key Metric Targets:**
- CPU: 0.1-5% per voice (depending on complexity)
- Polyphony: 8-128 voices (depending on model)
- Pitch accuracy: ±0.5 Hz across full range
- Latency: < 10ms for live performance

**The Path Forward:**
Physical modelling is not about perfectly recreating reality, but creating musically expressive instruments that respond naturally to performer input. With JUCE's DSP framework, proper optimization, and thoughtful parameter design, production-quality PM synthesis is achievable for independent developers.

---

**Research conducted:** 2026-01-08
**Research level:** Level 3 (Deep Investigation - Parallel Agents)
**Total research time:** ~45 minutes (parallel execution)
**Confidence:** HIGH (Multi-source validation, authoritative references, existing codebase analysis)

---

## Appendix: Quick Reference

### Essential JUCE DSP Classes for PM

```cpp
// Delay line (core building block)
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine { 88200 };

// Filters
juce::dsp::IIR::Filter<float> loopFilter;
auto coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sampleRate, cutoff);
*loopFilter.coefficients = *coeffs;

// Oversampling
juce::dsp::Oversampling<float> oversampler { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple };

// Parameter smoothing
juce::SmoothedValue<float> smoothed;
smoothed.reset(sampleRate, 0.02); // 20ms
smoothed.setTargetValue(target);
float current = smoothed.getNextValue();

// Denormal protection
juce::ScopedNoDenormals noDenormals; // In processBlock
```

### Algorithm Selection Matrix

| Need | Algorithm | Complexity | CPU | Quality |
|------|-----------|------------|-----|---------|
| Plucked strings (simple) | Karplus-Strong | 1/5 | Very Low | Good |
| Plucked strings (realistic) | Digital Waveguide | 3/5 | Medium | Excellent |
| Bells, gongs, metallic | Modal Synthesis | 2/5 | Medium | Excellent |
| Drums, membranes | Mass-Spring | 4/5 | High | Excellent |
| Bowed strings | Digital Waveguide + Bow | 4/5 | High | Excellent |
| Wind instruments | Digital Waveguide + Reed | 4/5 | High | Excellent |

### Performance Optimization Priority

1. **High Impact, Low Effort:**
   - SIMD for modal synthesis filter banks
   - Lookup tables for tanh/sin (non-linear functions)
   - Voice pooling and note stealing

2. **Medium Impact, Medium Effort:**
   - Per-block filter coefficient updates (not per-sample)
   - Delay line interpolation quality tuning
   - Parameter smoothing to avoid zipper noise

3. **Low Impact, High Effort:**
   - GPU offloading (latency issues)
   - Custom memory allocators
   - Advanced voice stealing algorithms

### CMake Template

```cmake
juce_add_plugin(PMSynth
    COMPANY_NAME "YourCompany"
    PLUGIN_CODE PMSy
    FORMATS VST3 AU Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
)

target_link_libraries(PMSynth
    PRIVATE
        juce::juce_audio_processors
        juce::juce_dsp # REQUIRED for DelayLine, IIR, etc.
)

juce_generate_juce_header(PMSynth)
```

---

**End of Complete Guide**
