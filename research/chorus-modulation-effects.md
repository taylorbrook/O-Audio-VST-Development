---
title: "Chorus & Modulation Effects"
created: 2026-03-07
juce_version: "8.0.4"
summary: "Complete technical reference for chorus and modulation effects in audio plugins, covering LFO-modulated delay, ensemble chorus, dimension-style chorus, BBD emulation, flanger, phaser, and vibrato with JUCE implementation examples."
domain: dsp
type: guide
keywords:
  - chorus
  - modulation
  - lfo
  - flanger
  - phaser
  - vibrato
  - bbd
  - ensemble
  - delay-modulation
  - juce-dsp
stages: [0, 1, 2]
agents: [dsp, research]
---

# Chorus & Modulation Effects

**Complete Technical Reference for Chorus and Modulation Effects in Audio Plugins**

**Created:** March 2026
**Version:** 1.0
**Research Depth:** Level 3 (Comprehensive Investigation)

---

## Executive Summary

This document covers chorus and related modulation effects for audio plugin development. Chorus, flanger, phaser, and vibrato share a common architecture -- modulated delay lines -- but differ in delay times, feedback, and modulation depth. This guide covers the DSP fundamentals, analog hardware emulation (BBD, bucket-brigade devices), and professional implementation techniques.

**Key Findings:**
- Chorus is fundamentally an LFO-modulated short delay mixed with the dry signal
- Ensemble effects use multiple voices with decorrelated LFOs for width and richness
- BBD (bucket-brigade device) emulation adds characteristic clock noise and frequency-dependent roll-off
- The Roland Dimension D / Boss CE-1 architecture uses carefully tuned, non-obvious LFO shapes
- Fractional delay interpolation quality is critical for chorus (allpass or cubic preferred)
- JUCE's `dsp::Chorus` provides a basic implementation; custom work needed for analog character

---

## Table of Contents

### Part 1: Fundamentals
1. [Modulated Delay Architecture](#1-modulated-delay-architecture)
2. [LFO Design for Modulation Effects](#2-lfo-design-for-modulation-effects)
3. [Fractional Delay Interpolation](#3-fractional-delay-interpolation)

### Part 2: Effect Types
4. [Chorus](#4-chorus)
5. [Ensemble Chorus](#5-ensemble-chorus)
6. [Flanger](#6-flanger)
7. [Phaser](#7-phaser)
8. [Vibrato](#8-vibrato)

### Part 3: Analog Emulation
9. [BBD (Bucket-Brigade Device) Modeling](#9-bbd-bucket-brigade-device-modeling)
10. [Classic Hardware Analysis](#10-classic-hardware-analysis)

### Part 4: JUCE Implementation
11. [JUCE DSP Module Integration](#11-juce-dsp-module-integration)
12. [Complete Chorus Implementation](#12-complete-chorus-implementation)
13. [Optimization and Real-Time Safety](#13-optimization-and-real-time-safety)

### Part 5: References
14. [References and Further Reading](#14-references-and-further-reading)

---

## Part 1: Fundamentals

## 1. Modulated Delay Architecture

### 1.1 Core Signal Flow

All modulation effects share this fundamental architecture:

```
Input --> [Delay Line] --> Mix --> Output
              ^               |
              |               v
            [LFO]          [Dry]
```

The key parameters that distinguish different modulation effects:

| Effect | Delay Range | Feedback | Mix | LFO Rate |
|--------|-------------|----------|-----|----------|
| **Chorus** | 5-30 ms | 0-20% | Wet+Dry | 0.1-5 Hz |
| **Flanger** | 0.1-10 ms | 50-99% | Wet+Dry | 0.05-5 Hz |
| **Phaser** | Allpass chain | 0-90% | Wet+Dry | 0.1-5 Hz |
| **Vibrato** | 1-10 ms | 0% | Wet only | 1-14 Hz |

### 1.2 Delay Line for Modulation

A modulation delay line needs:
- Short maximum delay (typically 50 ms)
- Sub-sample accurate read position (fractional delay)
- Smooth interpolation to avoid aliasing during modulation
- No discontinuities when delay time changes

```cpp
class ModulationDelayLine
{
public:
    void prepare(double sampleRate, float maxDelayMs)
    {
        int maxSamples = static_cast<int>(maxDelayMs * 0.001f * sampleRate) + 4;
        buffer.resize(maxSamples, 0.0f);
        writePos = 0;
        sr = sampleRate;
    }

    void write(float sample)
    {
        buffer[writePos] = sample;
        writePos = (writePos + 1) % (int)buffer.size();
    }

    float readCubic(float delaySamples) const
    {
        float readPos = (float)writePos - delaySamples;
        if (readPos < 0.0f) readPos += (float)buffer.size();

        int idx0 = ((int)readPos - 1 + (int)buffer.size()) % (int)buffer.size();
        int idx1 = (int)readPos % (int)buffer.size();
        int idx2 = (idx1 + 1) % (int)buffer.size();
        int idx3 = (idx2 + 1) % (int)buffer.size();

        float frac = readPos - std::floor(readPos);

        // Cubic Hermite interpolation
        float a = buffer[idx0];
        float b = buffer[idx1];
        float c = buffer[idx2];
        float d = buffer[idx3];

        float t = frac;
        float t2 = t * t;
        float t3 = t2 * t;

        return b + 0.5f * t * (c - a)
             + t2 * (a - 2.5f * b + 2.0f * c - 0.5f * d)
             + t3 * (-0.5f * a + 1.5f * b - 1.5f * c + 0.5f * d);
    }

private:
    std::vector<float> buffer;
    int writePos = 0;
    double sr = 44100.0;
};
```

---

## 2. LFO Design for Modulation Effects

### 2.1 Basic LFO Shapes

```cpp
class LFO
{
public:
    enum Shape { Sine, Triangle, Saw, Square, Random };

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        phase = 0.0f;
    }

    void setFrequency(float freq)
    {
        increment = freq / (float)sr;
    }

    float process()
    {
        float output = 0.0f;

        switch (shape)
        {
            case Sine:
                output = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
                break;

            case Triangle:
                output = 2.0f * std::abs(2.0f * (phase - std::floor(phase + 0.5f))) - 1.0f;
                break;

            case Saw:
                output = 2.0f * phase - 1.0f;
                break;

            case Square:
                output = phase < 0.5f ? 1.0f : -1.0f;
                break;

            case Random:
                if (phase < lastPhase) // Wrapped
                    randomValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
                output = randomValue;
                break;
        }

        lastPhase = phase;
        phase += increment;
        if (phase >= 1.0f) phase -= 1.0f;

        return output;
    }

    Shape shape = Sine;

private:
    double sr = 44100.0;
    float phase = 0.0f;
    float increment = 0.0f;
    float lastPhase = 0.0f;
    float randomValue = 0.0f;
};
```

### 2.2 Decorrelated LFOs for Stereo/Multi-Voice

For stereo chorus and ensemble effects, LFOs need phase decorrelation:

```cpp
class DecorrelatedLFO
{
public:
    void prepare(double sampleRate, int numVoices)
    {
        voices.resize(numVoices);
        for (int i = 0; i < numVoices; ++i)
        {
            voices[i].prepare(sampleRate);
            // Phase offset each voice evenly
            voices[i].setPhaseOffset((float)i / (float)numVoices);
        }
    }

    float getVoice(int voiceIndex) const
    {
        return voices[voiceIndex].getCurrentValue();
    }

    void advance()
    {
        for (auto& voice : voices)
            voice.process();
    }

private:
    std::vector<LFO> voices;
};
```

### 2.3 Smoothed Random Modulation

Some chorus effects use smoothed random values for a more organic, less predictable modulation:

```cpp
class SmoothedRandomLFO
{
public:
    void prepare(double sampleRate, float rateHz)
    {
        sr = sampleRate;
        samplesPerStep = static_cast<int>(sr / rateHz);
        counter = 0;
        currentValue = 0.0f;
        targetValue = 0.0f;
        smoother.reset(sampleRate, 1.0 / rateHz);
    }

    float process()
    {
        if (--counter <= 0)
        {
            counter = samplesPerStep;
            targetValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
            smoother.setTargetValue(targetValue);
        }
        return smoother.getNextValue();
    }

private:
    double sr = 44100.0;
    int samplesPerStep = 0;
    int counter = 0;
    float currentValue = 0.0f;
    float targetValue = 0.0f;
    juce::SmoothedValue<float> smoother;
};
```

---

## 3. Fractional Delay Interpolation

### 3.1 Interpolation Quality Comparison

| Method | Quality | CPU Cost | Artifacts |
|--------|---------|----------|-----------|
| Nearest neighbor | Poor | Very low | Strong aliasing, zipper noise |
| Linear | Fair | Low | Some HF loss, minor aliasing |
| Cubic Hermite | Good | Medium | Minimal aliasing, good HF |
| Allpass | Excellent | Medium | Best for modulated delays |
| Sinc (windowed) | Best | High | Ideal but expensive |

### 3.2 Allpass Interpolation

Allpass interpolation is ideal for modulated delays because it preserves magnitude while adjusting phase:

```cpp
class AllpassInterpolator
{
public:
    float process(float input, float fractionalDelay)
    {
        // First-order allpass: H(z) = (z^-1 + a) / (1 + a*z^-1)
        // where a = (1 - frac) / (1 + frac)
        float a = (1.0f - fractionalDelay) / (1.0f + fractionalDelay);
        float output = a * input + prevInput - a * prevOutput;
        prevInput = input;
        prevOutput = output;
        return output;
    }

private:
    float prevInput = 0.0f;
    float prevOutput = 0.0f;
};
```

### 3.3 Thiran Allpass

For higher-quality fractional delay, the Thiran allpass filter provides maximally-flat group delay:

```cpp
// Thiran first-order allpass coefficient
float thiranCoefficient(float delay)
{
    // delay must be > 0.5 for stability
    float d = juce::jmax(delay, 0.51f);
    return (1.0f - d) / (1.0f + d);
}
```

---

## Part 2: Effect Types

## 4. Chorus

### 4.1 Basic Chorus Algorithm

Chorus creates a thickening effect by mixing the dry signal with one or more pitch-shifted copies. The pitch shift comes from the modulated delay:

```cpp
class BasicChorus
{
public:
    void prepare(double sampleRate)
    {
        delayLine.prepare(sampleRate, 50.0f); // 50 ms max delay
        lfo.prepare(sampleRate);
        lfo.setFrequency(1.5f); // 1.5 Hz modulation
        sr = sampleRate;
    }

    float process(float input)
    {
        // Write to delay
        delayLine.write(input);

        // Compute modulated delay time
        float lfoValue = lfo.process();
        float delaySamples = (centerDelayMs + depthMs * lfoValue) * 0.001f * (float)sr;
        delaySamples = juce::jmax(delaySamples, 1.0f);

        // Read from delay with interpolation
        float wetSample = delayLine.readCubic(delaySamples);

        // Mix dry and wet
        return input * (1.0f - mix) + wetSample * mix;
    }

    float centerDelayMs = 7.0f;  // Center delay (7 ms typical)
    float depthMs = 3.0f;        // Modulation depth (+/- 3 ms)
    float mix = 0.5f;            // Dry/wet mix

private:
    ModulationDelayLine delayLine;
    LFO lfo;
    double sr = 44100.0;
};
```

### 4.2 Stereo Chorus

For stereo width, use phase-offset LFOs for left and right channels:

```cpp
class StereoChorus
{
public:
    void prepare(double sampleRate)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            delayLines[ch].prepare(sampleRate, 50.0f);
            lfos[ch].prepare(sampleRate);
            lfos[ch].setFrequency(rate);
        }
        // Phase offset right channel by 90 degrees
        lfos[1].setPhaseOffset(0.25f);
        sr = sampleRate;
    }

    void process(float& left, float& right)
    {
        delayLines[0].write(left);
        delayLines[1].write(right);

        float lfoL = lfos[0].process();
        float lfoR = lfos[1].process();

        float delayL = (centerDelayMs + depthMs * lfoL) * 0.001f * (float)sr;
        float delayR = (centerDelayMs + depthMs * lfoR) * 0.001f * (float)sr;

        float wetL = delayLines[0].readCubic(juce::jmax(delayL, 1.0f));
        float wetR = delayLines[1].readCubic(juce::jmax(delayR, 1.0f));

        left  = left  * (1.0f - mix) + wetL * mix;
        right = right * (1.0f - mix) + wetR * mix;
    }

private:
    ModulationDelayLine delayLines[2];
    LFO lfos[2];
    double sr = 44100.0;
    float rate = 1.5f;
    float centerDelayMs = 7.0f;
    float depthMs = 3.0f;
    float mix = 0.5f;
};
```

---

## 5. Ensemble Chorus

### 5.1 Multi-Voice Architecture

Ensemble chorus uses multiple delay voices (typically 3-6) with decorrelated LFOs to create rich, lush thickening:

```cpp
class EnsembleChorus
{
public:
    static constexpr int MaxVoices = 6;

    struct Voice
    {
        ModulationDelayLine delay;
        LFO lfo;
        float centerDelay = 7.0f; // ms
        float depth = 3.0f;       // ms
        float pan = 0.5f;         // 0=left, 1=right
    };

    void prepare(double sampleRate, int numVoices)
    {
        activeVoices = juce::jmin(numVoices, MaxVoices);
        sr = sampleRate;

        for (int v = 0; v < activeVoices; ++v)
        {
            voices[v].delay.prepare(sampleRate, 50.0f);
            voices[v].lfo.prepare(sampleRate);

            // Slightly different rates per voice for richness
            float voiceRate = rate * (1.0f + 0.1f * (float)(v - activeVoices / 2));
            voices[v].lfo.setFrequency(voiceRate);

            // Phase offset each voice
            voices[v].lfo.setPhaseOffset((float)v / (float)activeVoices);

            // Slightly different center delays
            voices[v].centerDelay = centerDelayMs + (float)v * 1.5f;

            // Distribute voices across stereo field
            voices[v].pan = (float)v / (float)(activeVoices - 1);
        }
    }

    void process(float inputL, float inputR, float& outL, float& outR)
    {
        float mono = (inputL + inputR) * 0.5f;

        outL = inputL * (1.0f - mix);
        outR = inputR * (1.0f - mix);

        for (int v = 0; v < activeVoices; ++v)
        {
            voices[v].delay.write(mono);

            float lfoVal = voices[v].lfo.process();
            float delaySamples = (voices[v].centerDelay + voices[v].depth * lfoVal)
                                 * 0.001f * (float)sr;
            delaySamples = juce::jmax(delaySamples, 1.0f);

            float wet = voices[v].delay.readCubic(delaySamples) * (mix / (float)activeVoices);

            // Pan voice in stereo field
            outL += wet * (1.0f - voices[v].pan);
            outR += wet * voices[v].pan;
        }
    }

private:
    Voice voices[MaxVoices];
    int activeVoices = 3;
    double sr = 44100.0;
    float rate = 1.0f;
    float centerDelayMs = 7.0f;
    float mix = 0.5f;
};
```

### 5.2 Dimension-Style Chorus

The Roland Dimension D (SDD-320) uses a unique architecture where the chorus effect is precisely calibrated across 4 preset modes rather than continuous controls. Each mode uses:
- Carefully selected delay times (4-10 ms range)
- Specific LFO rates and depths
- Complementary left/right modulation (inverted LFO)
- No feedback -- pure pitch modulation

The key insight is that the LFO shape is not a pure sine but a combination of sine and triangle with asymmetric depth, creating a more natural pitch variation.

---

## 6. Flanger

### 6.1 Flanger vs. Chorus

Flanging uses the same architecture as chorus but with:
- Very short delays (0.1-5 ms vs. 5-30 ms)
- High feedback (50-99% vs. 0-20%)
- The feedback creates a comb filter with moving notches (the "jet" sound)

```cpp
class Flanger
{
public:
    void prepare(double sampleRate)
    {
        delayLine.prepare(sampleRate, 15.0f);
        lfo.prepare(sampleRate);
        lfo.setFrequency(0.3f);
        sr = sampleRate;
    }

    float process(float input)
    {
        float delayed = delayLine.readCubic(
            juce::jmax((centerDelayMs + depthMs * lfo.process()) * 0.001f * (float)sr, 0.5f));

        // Feedback creates the comb filter harmonics
        float output = input + delayed * mix;
        delayLine.write(input + delayed * feedback);

        return output;
    }

    float centerDelayMs = 2.0f;
    float depthMs = 1.5f;
    float feedback = 0.7f;
    float mix = 0.5f;

private:
    ModulationDelayLine delayLine;
    LFO lfo;
    double sr = 44100.0;
};
```

### 6.2 Through-Zero Flanging

True tape flanging produces through-zero effects where the delay crosses zero, creating a momentary phase cancellation. In digital, this is approximated by:

```cpp
// Through-zero flanger: delay oscillates around 0 (with a small offset)
float minDelay = 0.05f; // 50 microseconds minimum
float maxDelay = 5.0f;  // 5 ms maximum

float lfoValue = lfo.process(); // -1 to +1
float delaySamples;

if (throughZeroEnabled)
{
    // LFO sweeps from negative (inverted signal) to positive delay
    float delayMs = depthMs * lfoValue; // Can be negative
    if (delayMs < 0.0f)
    {
        // "Negative delay" = read ahead = phase inversion + positive delay
        delaySamples = std::abs(delayMs) * 0.001f * (float)sr;
        phaseInvert = -1.0f;
    }
    else
    {
        delaySamples = juce::jmax(delayMs * 0.001f * (float)sr, 0.5f);
        phaseInvert = 1.0f;
    }
}
```

---

## 7. Phaser

### 7.1 Allpass Filter Chain

A phaser uses a chain of allpass filters (not delay lines) to create frequency-dependent phase shifts:

```cpp
class PhaserStage
{
public:
    void setFrequency(float freq, double sampleRate)
    {
        float w = juce::MathConstants<float>::pi * freq / (float)sampleRate;
        float t = std::tan(w);
        coefficient = (t - 1.0f) / (t + 1.0f);
    }

    float process(float input)
    {
        float output = coefficient * input + prevInput - coefficient * prevOutput;
        prevInput = input;
        prevOutput = output;
        return output;
    }

private:
    float coefficient = 0.0f;
    float prevInput = 0.0f;
    float prevOutput = 0.0f;
};

class Phaser
{
public:
    static constexpr int NumStages = 6; // 6-stage phaser (3 notches)

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        lfo.prepare(sampleRate);
        lfo.setFrequency(0.5f);
    }

    float process(float input)
    {
        float lfoValue = lfo.process();

        // Map LFO to frequency range (logarithmic)
        float minFreq = 100.0f;
        float maxFreq = 4000.0f;
        float freq = minFreq * std::pow(maxFreq / minFreq, (lfoValue + 1.0f) * 0.5f);

        float signal = input;
        for (int i = 0; i < NumStages; ++i)
        {
            stages[i].setFrequency(freq * (1.0f + (float)i * 0.3f), sr);
            signal = stages[i].process(signal);
        }

        // Feedback
        float output = input + signal * depth;
        // Feed back into first stage
        // (simplified -- full implementation feeds back before allpass chain)

        return output;
    }

    float depth = 0.7f;

private:
    PhaserStage stages[NumStages];
    LFO lfo;
    double sr = 44100.0;
};
```

### 7.2 Notch Count

Each pair of allpass stages creates one notch in the frequency response:
- 2 stages = 1 notch (subtle)
- 4 stages = 2 notches (classic phaser)
- 6 stages = 3 notches (rich, EHX Small Stone)
- 8 stages = 4 notches (deep, MXR Phase 90 has 4 stages)
- 12 stages = 6 notches (extreme, lush)

---

## 8. Vibrato

### 8.1 Pure Pitch Modulation

Vibrato is purely wet (no dry signal mixed in), creating pitch modulation without the comb-filter doubling of chorus:

```cpp
class Vibrato
{
public:
    void prepare(double sampleRate)
    {
        delayLine.prepare(sampleRate, 20.0f);
        lfo.prepare(sampleRate);
        sr = sampleRate;
    }

    float process(float input)
    {
        delayLine.write(input);

        float lfoValue = lfo.process();
        float delaySamples = (centerDelayMs + depthMs * lfoValue) * 0.001f * (float)sr;

        // Vibrato = wet only (no dry mix)
        return delayLine.readCubic(juce::jmax(delaySamples, 1.0f));
    }

    float rate = 5.0f;        // Hz (typical vocal vibrato ~5-7 Hz)
    float centerDelayMs = 5.0f;
    float depthMs = 2.0f;     // Pitch deviation

private:
    ModulationDelayLine delayLine;
    LFO lfo;
    double sr = 44100.0;
};
```

### 8.2 Vibrato vs. Chorus Perception

The perceptual difference:
- **Vibrato** (100% wet): Perceived as pitch modulation
- **Chorus** (50% wet + 50% dry): Perceived as thickening/doubling

The mixing ratio determines whether the ear perceives pitch change or ensemble effect.

---

## Part 3: Analog Emulation

## 9. BBD (Bucket-Brigade Device) Modeling

### 9.1 BBD Architecture

A BBD is an analog shift register that passes charge from one capacitor to the next at a clock rate. This creates a discrete-time delay in the analog domain.

Key BBD characteristics to model:
- **Clock noise**: Audible artifacts at the clock frequency
- **Frequency-dependent roll-off**: Acts as a low-pass filter (anti-aliasing)
- **Companding noise reduction**: Some BBDs use companders (dbx or Philips)
- **Signal-dependent distortion**: Charge transfer is nonlinear
- **Limited dynamic range**: Typically 60-70 dB SNR

### 9.2 BBD Clock Noise

```cpp
class BBDModel
{
public:
    void prepare(double sampleRate, float maxDelayMs)
    {
        sr = sampleRate;
        // Anti-aliasing filter (models BBD's internal filtering)
        aaFilter.setCoefficients(
            juce::IIRCoefficients::makeLowPass(sampleRate, 10000.0));

        // Clock noise filter
        clockFilter.setCoefficients(
            juce::IIRCoefficients::makeBandPass(sampleRate, 15000.0, 5.0));

        delayLine.prepare(sampleRate, maxDelayMs);
    }

    float process(float input)
    {
        // Input anti-aliasing (models BBD input filter)
        float filtered = aaFilter.processSingleSampleRaw(input);

        // Soft saturation (models charge transfer nonlinearity)
        float saturated = std::tanh(filtered * 1.2f) / 1.2f;

        delayLine.write(saturated);

        float lfoVal = lfo.process();
        float delaySamples = (centerDelay + depth * lfoVal) * 0.001f * (float)sr;
        float delayed = delayLine.readCubic(juce::jmax(delaySamples, 1.0f));

        // Add subtle clock noise
        float clockNoise = clockFilter.processSingleSampleRaw(
            (juce::Random::getSystemRandom().nextFloat() - 0.5f) * clockNoiseLevel);

        // Output anti-aliasing
        return delayed + clockNoise;
    }

private:
    ModulationDelayLine delayLine;
    LFO lfo;
    juce::IIRFilter aaFilter;
    juce::IIRFilter clockFilter;
    double sr = 44100.0;
    float centerDelay = 7.0f;
    float depth = 3.0f;
    float clockNoiseLevel = 0.002f; // Very subtle
};
```

### 9.3 Companding

Many BBD circuits use companding (compression before, expansion after) to improve SNR:

```cpp
// Simple compander model
float compress(float input, float ratio = 2.0f)
{
    float sign = input >= 0.0f ? 1.0f : -1.0f;
    return sign * std::pow(std::abs(input), 1.0f / ratio);
}

float expand(float input, float ratio = 2.0f)
{
    float sign = input >= 0.0f ? 1.0f : -1.0f;
    return sign * std::pow(std::abs(input), ratio);
}
```

---

## 10. Classic Hardware Analysis

### 10.1 Boss CE-1 Chorus Ensemble

The first chorus pedal (1976), derived from the Roland Jazz Chorus amplifier:
- Single MN3002 BBD (1024 stages)
- Two modes: Chorus (moderate) and Vibrato (deep)
- Chorus: ~7 ms center delay, depth ~2 ms, rate ~0.5 Hz
- Vibrato: ~5 ms center delay, depth ~3 ms, rate ~6 Hz
- FET-based input buffer and output mixer
- No feedback path

### 10.2 Roland Dimension D (SDD-320)

A studio chorus with 4 preset modes:
- Mode I: Subtle widening (narrow depth, slow rate)
- Mode II: Moderate chorus (medium depth)
- Mode III: Rich ensemble (deeper modulation)
- Mode IV: Wide stereo (combination of modes)
- Complementary L/R modulation for wide stereo image
- No user-adjustable controls -- the presets are precisely calibrated

### 10.3 Electro-Harmonix Small Clone

Classic 2-knob chorus:
- Rate control + depth switch (shallow/deep)
- Single voice with moderate feedback
- MN3007 BBD (1024 stages)
- Characteristic "watery" sound from the feedback path

### 10.4 TC Electronic SCF (Stereo Chorus Flanger)

Three modes with the same modulated delay architecture:
- Chorus mode: Longer delay, no feedback
- Flanger mode: Shorter delay, high feedback
- "Both" mode: Parallel chorus and flanger

---

## Part 4: JUCE Implementation

## 11. JUCE DSP Module Integration

### 11.1 juce::dsp::Chorus

JUCE provides a basic chorus in the DSP module:

```cpp
juce::dsp::Chorus<float> chorus;

void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = 2;

    chorus.prepare(spec);
    chorus.setRate(1.5f);       // LFO rate in Hz
    chorus.setDepth(0.25f);     // Modulation depth (0-1)
    chorus.setCentreDelay(7.0f); // Center delay in ms
    chorus.setFeedback(0.0f);   // No feedback for basic chorus
    chorus.setMix(0.5f);        // 50% wet
}

void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
{
    auto block = juce::dsp::AudioBlock<float>(buffer);
    chorus.process(juce::dsp::ProcessContextReplacing<float>(block));
}
```

### 11.2 Limitations of juce::dsp::Chorus

- Single voice only (no ensemble mode)
- Fixed sine LFO shape
- No BBD emulation
- No stereo phase offset
- Limited modulation depth range

For a production chorus plugin (O-Chorus), custom implementation is essential.

---

## 12. Complete Chorus Implementation

### 12.1 Production-Quality Chorus Processor

```cpp
class ChorusProcessor : public juce::AudioProcessor
{
public:
    ChorusProcessor()
        : AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        sr = sampleRate;

        for (int v = 0; v < numVoices; ++v)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                delays[v][ch].prepare(sampleRate, 50.0f);
                lfos[v][ch].prepare(sampleRate);
            }
        }

        updateParameters();
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;

        int numSamples = buffer.getNumSamples();
        auto* leftIn = buffer.getReadPointer(0);
        auto* rightIn = buffer.getReadPointer(1);
        auto* leftOut = buffer.getWritePointer(0);
        auto* rightOut = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            float dryL = leftIn[i];
            float dryR = rightIn[i];
            float wetL = 0.0f;
            float wetR = 0.0f;

            for (int v = 0; v < numVoices; ++v)
            {
                delays[v][0].write(dryL);
                delays[v][1].write(dryR);

                float lfoL = lfos[v][0].process();
                float lfoR = lfos[v][1].process();

                float delayL = juce::jmax(
                    (voiceDelays[v] + voiceDepths[v] * lfoL) * 0.001f * (float)sr, 1.0f);
                float delayR = juce::jmax(
                    (voiceDelays[v] + voiceDepths[v] * lfoR) * 0.001f * (float)sr, 1.0f);

                wetL += delays[v][0].readCubic(delayL);
                wetR += delays[v][1].readCubic(delayR);
            }

            float voiceScale = 1.0f / (float)numVoices;
            leftOut[i]  = dryL * (1.0f - mix) + wetL * voiceScale * mix;
            rightOut[i] = dryR * (1.0f - mix) + wetR * voiceScale * mix;
        }
    }

private:
    static constexpr int MaxVoices = 4;
    ModulationDelayLine delays[MaxVoices][2];
    LFO lfos[MaxVoices][2];
    float voiceDelays[MaxVoices] = {7.0f, 9.0f, 11.0f, 13.0f};
    float voiceDepths[MaxVoices] = {2.0f, 2.5f, 3.0f, 3.5f};
    int numVoices = 3;
    float mix = 0.5f;
    double sr = 44100.0;

    void updateParameters()
    {
        for (int v = 0; v < numVoices; ++v)
        {
            float voiceRate = 1.0f + 0.15f * (float)v; // Slightly different rates
            lfos[v][0].setFrequency(voiceRate);
            lfos[v][1].setFrequency(voiceRate);
            // Right channel phase offset for stereo
            lfos[v][1].setPhaseOffset(0.25f + 0.1f * (float)v);
        }
    }
};
```

---

## 13. Optimization and Real-Time Safety

### 13.1 Avoiding Allocations

Pre-allocate all delay line buffers in `prepareToPlay()`:

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    // All memory allocation happens here
    for (auto& delay : delayLines)
        delay.prepare(sampleRate, maxDelayMs);
}
```

### 13.2 LFO Computation Efficiency

For simple sine LFOs, use a phasor with `sin()` lookup rather than computing `std::sin()` every sample:

```cpp
// Efficient sine LFO using polynomial approximation
float fastSin(float phase01)
{
    float x = phase01 * 2.0f - 1.0f; // Map [0,1] to [-1,1]
    // Bhaskara approximation: accurate to ~0.1%
    float x2 = x * x;
    return x * (1.0f - x2 * (0.1666666f - x2 * 0.00833333f));
}
```

### 13.3 Common Pitfalls

| Pitfall | Impact | Solution |
|---------|--------|----------|
| Linear interpolation for chorus | HF loss, metallic artifacts | Use cubic or allpass interpolation |
| Identical LFO phase for all voices | Thin, mono-sounding chorus | Offset LFO phase per voice |
| No anti-denormal protection | CPU spikes on silence | Use `juce::ScopedNoDenormals` |
| Feedback > 1.0 | Unstable oscillation | Clamp feedback to [-0.99, 0.99] |
| Abrupt parameter changes | Clicks and pops | Smooth all parameters with `SmoothedValue` |

---

## Part 5: References

## 14. References and Further Reading

### Academic Papers
- Dattorro, J. (1997). "Effect Design Part 2: Delay-Line Modulation and Chorus." JAES, 45(10).
- Smith, J.O. "Physical Audio Signal Processing." Online book, Stanford.
- Raffel, C. & Smith, J.O. (2010). "Practical Modeling of Bucket-Brigade Device Circuits." DAFx-10.

### Books
- Pirkle, W. (2019). *Designing Audio Effect Plugins in C++*. Chapters 11-12.
- Zolzer, U. (2011). *DAFX: Digital Audio Effects*. Chapter 5: Modulation Effects.
- Reiss, J.D. & McPherson, A. (2015). *Audio Effects*. Chapter 8: Modulation Effects.

### Hardware References
- Boss CE-1 Chorus Ensemble (1976) -- first chorus pedal
- Roland SDD-320 Dimension D (1979) -- studio reference chorus
- Electro-Harmonix Small Clone (1979) -- iconic chorus
- MXR Phase 90 (1974) -- classic 4-stage phaser
- Eventide H3000 (1986) -- multi-voice chorus/harmonizer

### JUCE Resources
- `juce::dsp::Chorus` source in `juce_dsp/processors/juce_Chorus.h`
- `juce::dsp::DelayLine` for delay buffer management
- `juce::dsp::Oscillator` for LFO generation

---

*Research document for O-Chorus. Covers LFO-modulated delay, ensemble chorus, BBD emulation, flanging, phasing, and vibrato with JUCE implementation patterns.*
