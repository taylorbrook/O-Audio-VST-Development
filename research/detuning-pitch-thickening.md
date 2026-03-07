---
title: "Detuning & Pitch Thickening Algorithms"
created: 2026-03-07
juce_version: "8.0.4"
summary: "Complete technical reference for detuning and pitch thickening effects in audio plugins, covering micro-detuning, unison voice management, stereo widening via detuning, pitch shifting techniques, and JUCE implementation for chorus-like thickening without modulation."
domain: dsp
type: algorithm
keywords:
  - detuning
  - pitch-thickening
  - unison
  - micro-pitch
  - stereo-widening
  - pitch-shifting
  - granular-pitch
  - phase-vocoder
  - juce-dsp
stages: [0, 1, 2]
agents: [dsp, research]
---

# Detuning & Pitch Thickening Algorithms

**Complete Technical Reference for Pitch Thickening in Audio Plugins**

**Created:** March 2026
**Version:** 1.0
**Research Depth:** Level 3 (Comprehensive Investigation)

---

## Executive Summary

This document covers detuning and pitch thickening algorithms for audio plugin development. Unlike chorus (which uses modulated delay), pitch thickening creates static pitch-shifted copies of the signal, producing a consistently wide, thick sound without the cyclic modulation artifacts of chorus. This guide covers micro-pitch shifting, unison voice management, and stereo widening techniques.

**Key Findings:**
- Micro-pitch shifting (+/- 5-30 cents) with stereo panning creates convincing width without audible pitch change
- Granular pitch shifting (small overlapping grains) is the most practical real-time approach
- Phase vocoder pitch shifting offers higher quality but with more latency and artifacts
- Unison voice management (supersaw-style) requires careful detuning distribution
- Short pre-delay between voices adds depth perception
- Commercial references: Soundtoys MicroShift, Eventide H3000 MicroPitch

---

## Table of Contents

### Part 1: Fundamentals
1. [Pitch Thickening vs. Chorus](#1-pitch-thickening-vs-chorus)
2. [Pitch Shifting Techniques](#2-pitch-shifting-techniques)
3. [Psychoacoustic Basis for Thickening](#3-psychoacoustic-basis-for-thickening)

### Part 2: Algorithms
4. [Granular Pitch Shifting](#4-granular-pitch-shifting)
5. [Phase Vocoder Pitch Shifting](#5-phase-vocoder-pitch-shifting)
6. [Delay-Based Pitch Shifting](#6-delay-based-pitch-shifting)
7. [Unison Voice Management](#7-unison-voice-management)

### Part 3: Stereo Techniques
8. [Stereo Widening via Detuning](#8-stereo-widening-via-detuning)
9. [Mid-Side Processing](#9-mid-side-processing)
10. [Multi-Voice Panning](#10-multi-voice-panning)

### Part 4: JUCE Implementation
11. [Complete Pitch Thickener Plugin](#11-complete-pitch-thickener-plugin)
12. [Optimization and Real-Time Safety](#12-optimization-and-real-time-safety)

### Part 5: References
13. [References and Further Reading](#13-references-and-further-reading)

---

## Part 1: Fundamentals

## 1. Pitch Thickening vs. Chorus

### 1.1 Key Differences

| Feature | Chorus | Pitch Thickening |
|---------|--------|------------------|
| Pitch variation | Cyclic (LFO) | Static (fixed offset) |
| Modulation | Present | Absent or minimal |
| Sonic character | Swirling, animated | Stable, wide, thick |
| Comb filtering | Yes (varies with LFO) | Fixed (consistent) |
| Latency | Low (~1 ms) | Higher (5-50 ms) |
| Typical use | Guitars, synths | Vocals, mix bus |

### 1.2 The Thickening Effect

When multiple copies of a signal are played at slightly different pitches:
- The brain perceives one source, but wider and thicker
- Below ~10 cents detuning, pitch difference is subliminal
- Above ~30 cents, the copies become audible as separate tones
- The sweet spot (5-20 cents) creates width without obvious pitch change

### 1.3 Commercial References

| Product | Technique | Character |
|---------|-----------|-----------|
| Soundtoys MicroShift | Eventide H3000 algorithm | 3 styles, vintage character |
| Eventide MicroPitch | Granular pitch shift | Clean, precise |
| Waves Doubler | Delay + pitch + modulation | Natural doubling |
| iZotope Vocal Doubler | Formant-aware pitch shift | Vocal-optimized |
| UAD Eventide H910 | Early digital pitch shift | Gritty, character |

---

## 2. Pitch Shifting Techniques

### 2.1 Overview of Approaches

| Method | Quality | Latency | CPU | Best For |
|--------|---------|---------|-----|----------|
| Granular | Good | Low-Medium | Low | Real-time effects |
| Phase Vocoder | Very Good | Medium-High | Medium | Quality pitch shift |
| Delay-Line (Lexicon) | Fair | Very Low | Very Low | Simple detuning |
| PSOLA | Excellent (monophonic) | Medium | Medium | Solo vocals |
| Resample | Perfect pitch | N/A | Low | Offline only |

### 2.2 Cents to Ratio Conversion

```cpp
float centsToRatio(float cents)
{
    return std::pow(2.0f, cents / 1200.0f);
}

float ratioToCents(float ratio)
{
    return 1200.0f * std::log2(ratio);
}

// Examples:
// +10 cents = ratio 1.00578
// -10 cents = ratio 0.99424
// +100 cents = ratio 1.05946 (semitone)
```

---

## 3. Psychoacoustic Basis for Thickening

### 3.1 Precedence Effect (Haas Effect)

When two copies of a sound arrive within ~30 ms, the brain fuses them into one perceived source, but the spatial image widens. This is the foundation of pitch thickening:

- Copy with 0-5 ms delay: perceived as "same source, wider"
- Copy with 5-30 ms delay: perceived as "same source, spacious"
- Copy with >30 ms delay: perceived as echo/doubling

### 3.2 Interaural Decorrelation

Slightly detuning copies creates frequency-dependent phase differences between left and right channels, increasing the perceived stereo width without changing the tonal center.

### 3.3 Masking and Fusion

Micro-pitch differences between copies create spectral masking effects where the ear cannot separate the voices, perceiving instead a single, enriched source.

---

## Part 2: Algorithms

## 4. Granular Pitch Shifting

### 4.1 Overlap-Add Pitch Shifting

The most practical real-time pitch shifting algorithm uses overlapping grains:

```cpp
class GranularPitchShifter
{
public:
    static constexpr int MaxGrainSize = 4096;
    static constexpr int NumGrains = 4; // Overlapping grains

    void prepare(double sampleRate, float grainSizeMs = 40.0f)
    {
        sr = sampleRate;
        grainSize = static_cast<int>(grainSizeMs * 0.001f * sampleRate);
        grainSize = juce::jmin(grainSize, MaxGrainSize);
        hopSize = grainSize / NumGrains;

        buffer.resize(grainSize * 4, 0.0f); // Circular buffer
        writePos = 0;

        for (int g = 0; g < NumGrains; ++g)
        {
            grainPhases[g] = (float)(g * hopSize);
            grainCounters[g] = g * hopSize;
        }
    }

    float process(float input, float pitchRatio)
    {
        // Write input to circular buffer
        buffer[writePos] = input;
        writePos = (writePos + 1) % (int)buffer.size();

        // Sum overlapping grains
        float output = 0.0f;

        for (int g = 0; g < NumGrains; ++g)
        {
            // Read position advances at pitch ratio speed
            float readPos = (float)writePos - grainPhases[g];
            if (readPos < 0.0f) readPos += (float)buffer.size();

            // Interpolated read
            int idx0 = (int)readPos % (int)buffer.size();
            int idx1 = (idx0 + 1) % (int)buffer.size();
            float frac = readPos - std::floor(readPos);
            float sample = buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;

            // Window (Hann)
            float windowPos = (float)grainCounters[g] / (float)grainSize;
            float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi
                                                    * windowPos));

            output += sample * window;

            // Advance grain
            grainPhases[g] += pitchRatio;
            if (grainPhases[g] >= (float)buffer.size())
                grainPhases[g] -= (float)buffer.size();

            grainCounters[g]++;
            if (grainCounters[g] >= grainSize)
            {
                grainCounters[g] = 0;
                // Reset read position relative to write position
                grainPhases[g] = (float)grainSize;
            }
        }

        // Normalize by number of grains (overlapping windows sum to ~1)
        return output * (2.0f / (float)NumGrains);
    }

private:
    std::vector<float> buffer;
    int writePos = 0;
    int grainSize = 0;
    int hopSize = 0;
    double sr = 44100.0;
    float grainPhases[NumGrains] = {};
    int grainCounters[NumGrains] = {};
};
```

### 4.2 Grain Size Selection

| Grain Size | Character | Suitable For |
|------------|-----------|-------------|
| 10-20 ms | Fast, tight, more artifacts | Drums, percussive |
| 30-50 ms | Balanced | General purpose |
| 60-100 ms | Smooth, more latency | Pads, sustained |
| 100-200 ms | Very smooth, high latency | Ambient, offline |

### 4.3 Windowing for Smooth Grains

The window shape affects grain boundary artifacts:

```cpp
float triangleWindow(float position) { return 1.0f - std::abs(2.0f * position - 1.0f); }

float hannWindow(float position)
{
    return 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * position));
}

float tukeyWindow(float position, float taperRatio = 0.5f)
{
    if (position < taperRatio * 0.5f)
        return 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi
               * (2.0f * position / taperRatio - 1.0f)));
    if (position > 1.0f - taperRatio * 0.5f)
        return 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi
               * (2.0f * (position - 1.0f) / taperRatio + 1.0f)));
    return 1.0f;
}
```

---

## 5. Phase Vocoder Pitch Shifting

### 5.1 STFT-Based Approach

For higher quality pitch shifting, use the phase vocoder:

1. STFT analysis with overlap
2. Scale the bin frequencies (shift the spectrum)
3. Re-synthesize with modified phases

```cpp
void phaseVocoderPitchShift(float* magnitudes, float* phases, int numBins,
                             float pitchRatio, float* prevPhases, float* synthPhases,
                             int hopSize, int fftSize, double sampleRate)
{
    float freqPerBin = (float)sampleRate / (float)fftSize;
    float expectedPhaseDiff = 2.0f * juce::MathConstants<float>::pi
                            * (float)hopSize / (float)fftSize;

    for (int bin = 0; bin < numBins; ++bin)
    {
        // Compute true frequency for this bin
        float phaseDiff = phases[bin] - prevPhases[bin];
        prevPhases[bin] = phases[bin];

        // Remove expected phase advance
        phaseDiff -= (float)bin * expectedPhaseDiff;

        // Map to -pi..pi
        phaseDiff = std::fmod(phaseDiff + juce::MathConstants<float>::pi,
                              2.0f * juce::MathConstants<float>::pi)
                  - juce::MathConstants<float>::pi;

        // True frequency
        float trueFreq = (float)bin * freqPerBin + phaseDiff * freqPerBin
                        / expectedPhaseDiff;

        // Scale frequency by pitch ratio
        int targetBin = static_cast<int>((float)bin * pitchRatio);
        if (targetBin < numBins)
        {
            // Accumulate shifted spectrum
            magnitudes[targetBin] = magnitudes[bin];
            synthPhases[targetBin] += trueFreq * pitchRatio * expectedPhaseDiff / freqPerBin;
        }
    }
}
```

### 5.2 Phase Vocoder vs. Granular for Micro-Pitch

For micro-pitch shifting (+/- 30 cents), the difference between phase vocoder and granular is subtle. Granular is generally preferred for real-time effects due to lower latency.

---

## 6. Delay-Based Pitch Shifting

### 6.1 The Lexicon Approach

The simplest pitch shifting uses a continuously varying delay line (the Lexicon "Pitch Change" algorithm):

```cpp
class DelayPitchShifter
{
public:
    void prepare(double sampleRate, float maxDelayMs = 100.0f)
    {
        sr = sampleRate;
        int maxSamples = static_cast<int>(maxDelayMs * 0.001f * sampleRate);
        buffer.resize(maxSamples, 0.0f);
        writePos = 0;
        readPhase = 0.0f;
    }

    float process(float input, float pitchRatio)
    {
        buffer[writePos] = input;

        // Read position moves at different speed than write
        float readSpeed = 1.0f - (pitchRatio - 1.0f);
        readPhase += readSpeed;

        // Wrap read position within buffer
        if (readPhase >= (float)buffer.size()) readPhase -= (float)buffer.size();
        if (readPhase < 0.0f) readPhase += (float)buffer.size();

        // Interpolated read
        int idx0 = (int)readPhase % (int)buffer.size();
        int idx1 = (idx0 + 1) % (int)buffer.size();
        float frac = readPhase - std::floor(readPhase);
        float output = buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;

        writePos = (writePos + 1) % (int)buffer.size();

        return output;
    }

private:
    std::vector<float> buffer;
    int writePos = 0;
    float readPhase = 0.0f;
    double sr = 44100.0;
};
```

### 6.2 Crossfade to Avoid Discontinuities

The delay line eventually wraps, creating a discontinuity. Two crossfading delay lines solve this:

```cpp
class DualDelayPitchShifter
{
public:
    void prepare(double sampleRate)
    {
        delay1.prepare(sampleRate);
        delay2.prepare(sampleRate);
        crossfadePos = 0.0f;
    }

    float process(float input, float pitchRatio)
    {
        float out1 = delay1.process(input, pitchRatio);
        float out2 = delay2.process(input, pitchRatio);

        // Crossfade between the two delay lines
        float fade1 = std::cos(crossfadePos * juce::MathConstants<float>::halfPi);
        float fade2 = std::sin(crossfadePos * juce::MathConstants<float>::halfPi);

        return out1 * fade1 + out2 * fade2;
    }

private:
    DelayPitchShifter delay1, delay2;
    float crossfadePos = 0.0f;
};
```

---

## 7. Unison Voice Management

### 7.1 Supersaw-Style Detuning

For synth-style unison, detune multiple voices symmetrically:

```cpp
struct UnisonVoice
{
    float detuneAmount = 0.0f; // In cents
    float pan = 0.5f;          // Stereo position
    float level = 1.0f;        // Relative level
};

void setupUnisonVoices(UnisonVoice* voices, int numVoices, float totalDetuneCents)
{
    for (int i = 0; i < numVoices; ++i)
    {
        if (numVoices == 1)
        {
            voices[i] = {0.0f, 0.5f, 1.0f};
        }
        else
        {
            // Distribute voices symmetrically around center
            float position = (float)i / (float)(numVoices - 1); // 0 to 1
            voices[i].detuneAmount = (position - 0.5f) * 2.0f * totalDetuneCents;
            voices[i].pan = position; // Spread across stereo field
            voices[i].level = 1.0f / std::sqrt((float)numVoices); // Equal power
        }
    }
}
```

### 7.2 Detuning Distribution Curves

Not all voices should be evenly spaced. Different distributions create different characters:

```cpp
float linearDetune(int voiceIndex, int totalVoices, float maxCents)
{
    float t = (float)voiceIndex / (float)(totalVoices - 1) - 0.5f;
    return t * 2.0f * maxCents; // Linear spread
}

float exponentialDetune(int voiceIndex, int totalVoices, float maxCents)
{
    float t = (float)voiceIndex / (float)(totalVoices - 1) - 0.5f;
    float sign = t >= 0.0f ? 1.0f : -1.0f;
    return sign * std::pow(std::abs(t) * 2.0f, 1.5f) * maxCents; // More voices near center
}
```

### 7.3 Voice Count and Character

| Voices | Character | CPU Cost | Use Case |
|--------|-----------|----------|----------|
| 2 | Subtle widening | Very low | Vocals, acoustic |
| 3 | Classic MicroShift | Low | Most instruments |
| 5 | Rich ensemble | Medium | Synth pads |
| 7 | Full supersaw | Medium-high | EDM leads |
| 16+ | Dense cloud | High | Sound design |

---

## Part 3: Stereo Techniques

## 8. Stereo Widening via Detuning

### 8.1 The MicroShift Algorithm

The classic Eventide MicroShift algorithm:
1. Create two pitch-shifted copies (+X and -X cents)
2. Pan one hard left, the other hard right
3. Add short pre-delay to each (different amounts for L and R)
4. Mix with the dry signal (center)

```cpp
class MicroShift
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        pitchShiftUp.prepare(sampleRate);
        pitchShiftDown.prepare(sampleRate);

        // Pre-delay: 10-25 ms for spatial depth
        int preDelaySamples = static_cast<int>(0.015f * sampleRate); // 15 ms
        preDelayL.resize(preDelaySamples + 5, 0.0f);
        preDelayR.resize(preDelaySamples + 10, 0.0f); // Slightly different
        delayWriteL = 0;
        delayWriteR = 0;
    }

    void process(float input, float& outL, float& outR)
    {
        float ratioUp = centsToRatio(detuneCents);
        float ratioDown = centsToRatio(-detuneCents);

        float shiftedUp = pitchShiftUp.process(input, ratioUp);
        float shiftedDown = pitchShiftDown.process(input, ratioDown);

        // Pre-delay
        preDelayL[delayWriteL] = shiftedDown;
        preDelayR[delayWriteR] = shiftedUp;
        float delayedL = preDelayL[(delayWriteL + 1) % (int)preDelayL.size()];
        float delayedR = preDelayR[(delayWriteR + 1) % (int)preDelayR.size()];
        delayWriteL = (delayWriteL + 1) % (int)preDelayL.size();
        delayWriteR = (delayWriteR + 1) % (int)preDelayR.size();

        // Mix: dry center + shifted sides
        outL = input * (1.0f - mix) + delayedL * mix;
        outR = input * (1.0f - mix) + delayedR * mix;
    }

    float detuneCents = 10.0f;
    float mix = 0.5f;

private:
    GranularPitchShifter pitchShiftUp, pitchShiftDown;
    std::vector<float> preDelayL, preDelayR;
    int delayWriteL = 0, delayWriteR = 0;
    double sr = 44100.0;

    static float centsToRatio(float cents)
    {
        return std::pow(2.0f, cents / 1200.0f);
    }
};
```

### 8.2 Three-Voice MicroShift

The Soundtoys MicroShift Style 1 uses three voices:
- **Center:** Dry signal
- **Left:** Pitch shifted down, delayed 10-15 ms
- **Right:** Pitch shifted up, delayed 15-20 ms

The asymmetric delays add depth without creating obvious echoes.

---

## 9. Mid-Side Processing

### 9.1 Detuning in the Side Channel Only

Apply pitch thickening only to the side component to widen without affecting center:

```cpp
void midSideDetune(float left, float right, float& outL, float& outR)
{
    float mid  = (left + right) * 0.5f;
    float side = (left - right) * 0.5f;

    // Apply detuning to side only
    float processedSide = pitchShifter.process(side, centsToRatio(detuneCents));

    // Widen by increasing side level
    processedSide *= stereoWidth;

    outL = mid + processedSide;
    outR = mid - processedSide;
}
```

---

## 10. Multi-Voice Panning

### 10.1 Voice Panning Strategies

```cpp
enum PanningStrategy { Linear, EqualPower, Clustered };

float calculatePan(int voiceIndex, int totalVoices, PanningStrategy strategy)
{
    float position = (float)voiceIndex / (float)(totalVoices - 1);

    switch (strategy)
    {
        case Linear:
            return position; // Evenly spaced L to R

        case EqualPower:
            // Keep center voice centered, spread others
            if (totalVoices % 2 == 1 && voiceIndex == totalVoices / 2)
                return 0.5f;
            return position;

        case Clustered:
            // Cluster voices toward center with wider extremes
            return 0.5f + (position - 0.5f) * std::abs(position - 0.5f) * 2.0f;
    }

    return 0.5f;
}
```

---

## Part 4: JUCE Implementation

## 11. Complete Pitch Thickener Plugin

```cpp
class PitchThickenerProcessor : public juce::AudioProcessor
{
public:
    PitchThickenerProcessor()
        : AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        sr = sampleRate;

        for (int v = 0; v < MaxVoices; ++v)
        {
            for (int ch = 0; ch < 2; ++ch)
                pitchShifters[v][ch].prepare(sampleRate);
        }

        // Pre-delay lines
        int maxPreDelaySamples = static_cast<int>(0.03f * sampleRate);
        for (int v = 0; v < MaxVoices; ++v)
        {
            preDelays[v].resize(maxPreDelaySamples, 0.0f);
            preDelayPos[v] = 0;
        }

        smoothedDetune.reset(sampleRate, 0.05);
        smoothedMix.reset(sampleRate, 0.02);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;

        float detune = detuneParam;
        float mix = mixParam;
        int voices = numVoices;

        smoothedDetune.setTargetValue(detune);
        smoothedMix.setTargetValue(mix);

        setupVoices(voices, detune);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float currentDetune = smoothedDetune.getNextValue();
            float currentMix = smoothedMix.getNextValue();

            float inputL = buffer.getSample(0, i);
            float inputR = buffer.getNumChannels() > 1 ? buffer.getSample(1, i) : inputL;
            float mono = (inputL + inputR) * 0.5f;

            float wetL = 0.0f, wetR = 0.0f;

            for (int v = 0; v < voices; ++v)
            {
                float ratio = std::pow(2.0f, voiceDetune[v] * currentDetune / 1200.0f);
                float shifted = pitchShifters[v][0].process(mono, ratio);

                // Apply pre-delay
                int preDelaySamples = static_cast<int>(voicePreDelay[v] * 0.001f * (float)sr);
                preDelaySamples = juce::jmin(preDelaySamples, (int)preDelays[v].size() - 1);

                if (preDelaySamples > 0)
                {
                    preDelays[v][preDelayPos[v]] = shifted;
                    int readPos = (preDelayPos[v] - preDelaySamples + (int)preDelays[v].size())
                                  % (int)preDelays[v].size();
                    shifted = preDelays[v][readPos];
                    preDelayPos[v] = (preDelayPos[v] + 1) % (int)preDelays[v].size();
                }

                float voiceGain = 1.0f / std::sqrt((float)voices);
                wetL += shifted * (1.0f - voicePan[v]) * voiceGain;
                wetR += shifted * voicePan[v] * voiceGain;
            }

            buffer.setSample(0, i, inputL * (1.0f - currentMix) + wetL * currentMix);
            if (buffer.getNumChannels() > 1)
                buffer.setSample(1, i, inputR * (1.0f - currentMix) + wetR * currentMix);
        }
    }

private:
    static constexpr int MaxVoices = 8;
    GranularPitchShifter pitchShifters[MaxVoices][2];
    std::vector<float> preDelays[MaxVoices];
    int preDelayPos[MaxVoices] = {};

    float voiceDetune[MaxVoices] = {};
    float voicePan[MaxVoices] = {};
    float voicePreDelay[MaxVoices] = {}; // ms

    juce::SmoothedValue<float> smoothedDetune, smoothedMix;
    double sr = 44100.0;

    float detuneParam = 10.0f; // cents
    float mixParam = 0.5f;
    int numVoices = 2;

    void setupVoices(int count, float maxDetune)
    {
        for (int v = 0; v < count; ++v)
        {
            float position = (float)v / (float)(count - 1); // 0 to 1
            voiceDetune[v] = (position - 0.5f) * 2.0f;      // -1 to +1
            voicePan[v] = position;                           // L to R
            voicePreDelay[v] = 5.0f + position * 15.0f;      // 5-20 ms
        }
    }
};
```

---

## 12. Optimization and Real-Time Safety

### 12.1 Grain Buffer Pre-Allocation

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    // All memory allocation in prepare
    for (auto& shifter : pitchShifters)
        shifter.prepare(sampleRate);
    // Never allocate in processBlock
}
```

### 12.2 Smoothing Detune Changes

Abrupt detune changes cause pitch glitches:

```cpp
juce::SmoothedValue<float> smoothedDetune;
smoothedDetune.reset(sampleRate, 0.05); // 50 ms smoothing time
```

### 12.3 Common Pitfalls

| Pitfall | Impact | Solution |
|---------|--------|----------|
| No crossfade between grains | Click artifacts | Overlap-add with Hann window |
| Too few grains | Audible grain repetition | Use 4+ overlapping grains |
| Identical pre-delay for L and R | Narrow image | Different pre-delays per voice |
| Large detune (>50 cents) | Audible pitch difference | Keep to 5-25 cents for thickening |
| No input level compensation | Output louder than input | Scale by 1/sqrt(numVoices) |

---

## Part 5: References

## 13. References and Further Reading

### Academic/Technical
- Laroche, J. & Dolson, M. (1999). "Improved Phase Vocoder Time-Scale Modification." IEEE Trans.
- de Gotzen, A., Bernardini, N., & Arfib, D. (2000). "Traditional Implementations of a Phase-Vocoder." DAFx.
- Dolson, M. (1986). "The Phase Vocoder: A Tutorial." Computer Music Journal.

### Books
- Zolzer, U. (2011). *DAFX: Digital Audio Effects*. Chapter 7: Pitch Shifting.
- Roads, C. (2001). *Microsound*. MIT Press.
- Pirkle, W. (2019). *Designing Audio Effect Plugins in C++*. Chapter 20: Pitch Shifting.

### Hardware/Software References
- Eventide H3000 Ultra-Harmonizer (1986): MicroPitch algorithm
- Soundtoys MicroShift: Three vintage-inspired micro-pitch modes
- Eventide H910 (1975): First digital pitch shifter
- Lexicon PCM 42: Delay-based pitch change

### JUCE Resources
- `juce::dsp::FFT` for phase vocoder implementations
- `juce::SmoothedValue` for parameter smoothing
- `juce::dsp::Oversampling` for quality improvement

---

*Research document for O-Detune. Covers granular pitch shifting, phase vocoder, unison voice management, MicroShift algorithms, and stereo widening techniques with JUCE implementation.*
