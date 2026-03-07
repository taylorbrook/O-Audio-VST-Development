---
title: "Vocal & Formant Synthesis"
created: 2026-03-07
juce_version: "8.0.4"
summary: "Complete technical reference for vocal and formant synthesis in audio plugins, covering formant filter banks, vowel modeling, source-filter decomposition, phoneme-based synthesis, singing synthesis, and JUCE implementation with real-time performance considerations."
domain: dsp
type: research
keywords:
  - formant-synthesis
  - vocal-modeling
  - singing-synthesis
  - source-filter
  - phoneme
  - vowel-synthesis
  - resonance
  - juce-dsp
stages: [0, 1, 2]
agents: [dsp, research]
---

# Vocal & Formant Synthesis

**Complete Technical Reference for Vocal and Formant Synthesis in Audio Plugins**

**Created:** March 2026
**Version:** 1.0
**Research Depth:** Level 3 (Comprehensive Investigation)

---

## Executive Summary

This document covers vocal and formant synthesis for audio plugin development. Vocal sounds are characterized by formant frequencies -- resonances of the vocal tract that define vowel identity and vocal character. This guide covers formant filter banks, source-filter models, phoneme-based synthesis, and singing synthesis techniques.

**Key Findings:**
- The source-filter model separates excitation (glottis) from resonance (vocal tract), enabling independent control
- Formant frequencies are largely independent of pitch, which is why vowels are recognizable at any fundamental frequency
- Cascaded resonant bandpass filters (formant filters) are the most practical approach for real-time formant synthesis
- A minimum of 3 formant filters is needed for recognizable vowels; 5 formants produce natural-sounding results
- KLATT synthesis remains the gold standard for parametric speech synthesis
- Modern ML approaches (WORLD, CREPE, neural vocoders) offer higher quality but at greater computational cost

---

## Table of Contents

### Part 1: Vocal Acoustics
1. [Source-Filter Model](#1-source-filter-model)
2. [Formant Frequencies and Vowel Space](#2-formant-frequencies-and-vowel-space)
3. [Glottal Excitation](#3-glottal-excitation)

### Part 2: Synthesis Techniques
4. [Formant Filter Banks](#4-formant-filter-banks)
5. [KLATT Formant Synthesis](#5-klatt-formant-synthesis)
6. [FOF (Fonction d'Onde Formantique) Synthesis](#6-fof-synthesis)
7. [Phase Vocoder Approaches](#7-phase-vocoder-approaches)
8. [Phoneme-Based Synthesis](#8-phoneme-based-synthesis)

### Part 3: Advanced Topics
9. [Singing Synthesis](#9-singing-synthesis)
10. [Vocal Effects Processing](#10-vocal-effects-processing)
11. [ML-Based Vocal Synthesis](#11-ml-based-vocal-synthesis)

### Part 4: JUCE Implementation
12. [Formant Filter Implementation](#12-formant-filter-implementation)
13. [Complete Vocal Synthesizer](#13-complete-vocal-synthesizer)
14. [Optimization and Real-Time Safety](#14-optimization-and-real-time-safety)

### Part 5: References
15. [References and Further Reading](#15-references-and-further-reading)

---

## Part 1: Vocal Acoustics

## 1. Source-Filter Model

### 1.1 The Acoustic Model

The human voice is modeled as a two-stage system:

```
[Source (Glottis)] --> [Filter (Vocal Tract)] --> [Radiation (Lips)] --> Output
```

- **Source:** Vocal folds produce a quasi-periodic pulse train (voiced sounds) or noise (unvoiced sounds)
- **Filter:** The vocal tract (throat, mouth, nasal cavity) acts as a resonant filter
- **Radiation:** Lip radiation adds a +6 dB/octave spectral tilt

### 1.2 Source Independence

The source-filter model's key property: the source (pitch/F0) and the filter (formants) are largely independent. This means:
- You can change the pitch without changing the vowel
- You can morph between vowels without affecting pitch
- The same formant pattern applies across different pitches

### 1.3 Signal Flow for Synthesis

```cpp
class SourceFilterSynth
{
public:
    float processSample()
    {
        // Source: generate glottal pulse
        float source;
        if (voiced)
            source = glottalPulse.process(fundamentalFreq);
        else
            source = noiseSource.process() * noiseLevel;

        // Filter: apply formant resonances
        float filtered = source;
        for (int f = 0; f < numFormants; ++f)
            filtered = formantFilters[f].process(filtered);

        // Radiation: simple differentiation (+6 dB/oct)
        float radiated = filtered - prevSample;
        prevSample = filtered;

        return radiated * outputGain;
    }

private:
    GlottalPulse glottalPulse;
    NoiseGenerator noiseSource;
    FormantFilter formantFilters[5];
    int numFormants = 5;
    float fundamentalFreq = 220.0f;
    float noiseLevel = 0.01f;
    bool voiced = true;
    float prevSample = 0.0f;
    float outputGain = 0.1f;
};
```

---

## 2. Formant Frequencies and Vowel Space

### 2.1 Standard Formant Frequencies

Formant frequencies for common vowels (adult male, approximate values):

| Vowel (IPA) | Example | F1 (Hz) | F2 (Hz) | F3 (Hz) | F4 (Hz) | F5 (Hz) |
|-------------|---------|---------|---------|---------|---------|---------|
| /a/ | "father" | 730 | 1090 | 2440 | 3400 | 4500 |
| /i/ | "beat" | 270 | 2290 | 3010 | 3400 | 4500 |
| /u/ | "boot" | 300 | 870 | 2240 | 3400 | 4500 |
| /e/ | "bait" | 530 | 1840 | 2480 | 3400 | 4500 |
| /o/ | "boat" | 570 | 840 | 2410 | 3400 | 4500 |
| /ae/ | "bat" | 660 | 1720 | 2410 | 3400 | 4500 |

### 2.2 Formant Bandwidths

Each formant has an associated bandwidth that affects its resonant character:

| Formant | Typical Bandwidth | Character |
|---------|-------------------|-----------|
| F1 | 60-100 Hz | Narrow, strong resonance |
| F2 | 70-120 Hz | Moderate resonance |
| F3 | 100-150 Hz | Broader resonance |
| F4 | 150-250 Hz | Broad, ambient |
| F5 | 200-300 Hz | Very broad, subtle |

### 2.3 The Vowel Triangle

Vowels can be mapped in a 2D space using F1 (tongue height) and F2 (tongue position):

```
        F2 (High)              F2 (Low)
        Front                   Back

F1 Low:  /i/ (beat)             /u/ (boot)
         /e/ (bait)             /o/ (boat)
F1 High: /ae/ (bat)             /a/ (father)
```

This mapping is used for vowel morphing -- interpolating between formant sets creates smooth transitions between vowel sounds.

### 2.4 Gender and Age Scaling

Formant frequencies scale approximately with vocal tract length:

| Speaker | F scaling | F0 range |
|---------|-----------|----------|
| Adult male | 1.0x (reference) | 85-180 Hz |
| Adult female | 1.15-1.20x | 165-255 Hz |
| Child | 1.20-1.30x | 250-400 Hz |

```cpp
void scaleFormantsForGender(float* formantFreqs, int numFormants, float scaleFactor)
{
    for (int i = 0; i < numFormants; ++i)
        formantFreqs[i] *= scaleFactor;
}
```

---

## 3. Glottal Excitation

### 3.1 Glottal Pulse Models

The glottal pulse is the periodic waveform produced by the vocal folds. Several models exist:

**Rosenberg model:** Simple two-segment polynomial approximation:

```cpp
class RosenbergPulse
{
public:
    float process(float frequency)
    {
        float period = sr / frequency;
        float openPhase = 0.4f; // 40% of period is open
        float output = 0.0f;

        if (phase < openPhase)
        {
            // Opening phase: half-cosine rise
            float t = phase / openPhase;
            output = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * t));
        }
        else if (phase < openPhase + 0.16f)
        {
            // Closing phase: rapid fall
            float t = (phase - openPhase) / 0.16f;
            output = std::cos(juce::MathConstants<float>::pi * 0.5f * t);
        }
        // Else: closed phase (output = 0)

        phase += 1.0f / period;
        if (phase >= 1.0f) phase -= 1.0f;

        return output;
    }

private:
    float phase = 0.0f;
    double sr = 44100.0;
};
```

**LF (Liljencrants-Fant) model:** More accurate 4-parameter model used in research:

```cpp
class LFModel
{
public:
    // Parameters: Rd controls voice quality (1.0=pressed, 2.5=normal, 5.0=breathy)
    float process(float frequency, float Rd = 2.5f)
    {
        // Simplified LF derivative glottal flow
        float T0 = sr / frequency; // Period in samples
        float output = 0.0f;

        float tp = 0.4f * Rd; // Time of max flow
        float te = 0.6f;      // Time of closure

        if (normalizedPhase < te)
        {
            float t = normalizedPhase / te;
            output = std::sin(juce::MathConstants<float>::pi * t)
                   * std::exp(-alpha * t);
        }
        else
        {
            // Return phase (exponential decay)
            float t = (normalizedPhase - te) / (1.0f - te);
            output = -epsilon * std::exp(-epsilon * t);
        }

        normalizedPhase += 1.0f / T0;
        if (normalizedPhase >= 1.0f) normalizedPhase -= 1.0f;

        return output;
    }

private:
    float normalizedPhase = 0.0f;
    float alpha = 3.0f;   // Spectral tilt control
    float epsilon = 10.0f; // Return phase steepness
    double sr = 44100.0;
};
```

### 3.2 Voice Quality Parameters

| Quality | F0 Jitter | Spectral Tilt | Noise | Description |
|---------|-----------|---------------|-------|-------------|
| Pressed | Low | Flat (strong harmonics) | Very low | Tense, strained |
| Modal | Low | -12 dB/oct | Low | Normal speaking |
| Breathy | Moderate | Steep (-18+ dB/oct) | High | Airy, whispery |
| Creaky | High (irregular) | Flat | Low | Vocal fry |

### 3.3 Adding Breathiness

```cpp
float addBreathiness(float voicedSignal, float breathAmount, float noiseLevel)
{
    float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * noiseLevel;

    // Aspiration noise is filtered -- concentrated around F1-F3 region
    float aspirationNoise = aspirationFilter.process(noise);

    return voicedSignal * (1.0f - breathAmount) + aspirationNoise * breathAmount;
}
```

---

## Part 2: Synthesis Techniques

## 4. Formant Filter Banks

### 4.1 Resonant Bandpass Filters

The most common approach to formant synthesis uses cascaded or parallel resonant bandpass (peak) filters:

```cpp
class FormantFilter
{
public:
    void setFormant(float frequency, float bandwidth, float gain, double sampleRate)
    {
        // Second-order bandpass (peak) filter
        float w0 = 2.0f * juce::MathConstants<float>::pi * frequency / (float)sampleRate;
        float r = std::exp(-juce::MathConstants<float>::pi * bandwidth / (float)sampleRate);
        float cosW0 = std::cos(w0);

        float a = juce::Decibels::decibelsToGain(gain);

        // Transfer function: H(z) = a * (1 - r^2) / (1 - 2*r*cos(w0)*z^-1 + r^2*z^-2)
        b0 = a * (1.0f - r * r);
        b1 = 0.0f;
        b2 = 0.0f;
        a1 = -2.0f * r * cosW0;
        a2 = r * r;
    }

    float process(float input)
    {
        float output = b0 * input - a1 * state1 - a2 * state2;
        state2 = state1;
        state1 = output;
        return output;
    }

    void reset()
    {
        state1 = 0.0f;
        state2 = 0.0f;
    }

private:
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float state1 = 0.0f, state2 = 0.0f;
};
```

### 4.2 Parallel vs. Cascade Topology

**Cascade (series):** Each formant filter processes the output of the previous one.
- More natural spectral valleys
- Easier to model anti-formants (zeros)
- Used in KLATT synthesis

**Parallel:** All formant filters receive the same input; outputs are summed.
- Independent gain control per formant
- Easier to tune individual resonances
- Better for creative/musical applications

```cpp
// Parallel formant bank
class ParallelFormantBank
{
public:
    float process(float input)
    {
        float output = 0.0f;
        for (int i = 0; i < numFormants; ++i)
            output += formants[i].process(input);
        return output;
    }

private:
    FormantFilter formants[5];
    int numFormants = 5;
};

// Cascade formant bank
class CascadeFormantBank
{
public:
    float process(float input)
    {
        float signal = input;
        for (int i = 0; i < numFormants; ++i)
            signal = formants[i].process(signal);
        return signal;
    }

private:
    FormantFilter formants[5];
    int numFormants = 5;
};
```

### 4.3 Vowel Presets and Morphing

```cpp
struct VowelPreset
{
    float f[5];  // Formant frequencies
    float bw[5]; // Formant bandwidths
    float g[5];  // Formant gains (dB)
};

const VowelPreset vowelA = {{730, 1090, 2440, 3400, 4500}, {80, 90, 120, 200, 250}, {0, -6, -12, -18, -24}};
const VowelPreset vowelI = {{270, 2290, 3010, 3400, 4500}, {60, 90, 100, 200, 250}, {0, -10, -15, -20, -26}};
const VowelPreset vowelU = {{300,  870, 2240, 3400, 4500}, {70, 80, 100, 200, 250}, {0, -8, -14, -20, -26}};
const VowelPreset vowelE = {{530, 1840, 2480, 3400, 4500}, {70, 90, 110, 200, 250}, {0, -8, -12, -18, -24}};
const VowelPreset vowelO = {{570,  840, 2410, 3400, 4500}, {70, 80, 110, 200, 250}, {0, -6, -12, -18, -24}};

VowelPreset morphVowels(const VowelPreset& a, const VowelPreset& b, float t)
{
    VowelPreset result;
    for (int i = 0; i < 5; ++i)
    {
        // Interpolate in log-frequency domain for perceptually linear morphing
        result.f[i] = std::exp(std::log(a.f[i]) * (1.0f - t) + std::log(b.f[i]) * t);
        result.bw[i] = a.bw[i] * (1.0f - t) + b.bw[i] * t;
        result.g[i] = a.g[i] * (1.0f - t) + b.g[i] * t;
    }
    return result;
}
```

---

## 5. KLATT Formant Synthesis

### 5.1 Overview

The KLATT synthesizer (Dennis Klatt, 1980) is a parametric speech synthesizer using cascaded and parallel formant filters with detailed control over voice quality. It remains the reference implementation for formant synthesis.

### 5.2 Architecture

```
Voicing Source --> [Cascade Formants F1-F5] --> Output (vowels)
                                                     |
Noise Source  --> [Parallel Formants]        -------->+ (fricatives)
                                                     |
Aspiration    --> [Noise Filter]            -------->+ (breathiness)
```

### 5.3 Key KLATT Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| F0 | 80-500 Hz | Fundamental frequency |
| F1-F5 | 200-5000 Hz | Formant frequencies |
| B1-B5 | 30-500 Hz | Formant bandwidths |
| A1-A5 | 0-60 dB | Parallel formant amplitudes |
| AV | 0-80 dB | Voicing amplitude |
| AH | 0-80 dB | Aspiration amplitude |
| AF | 0-80 dB | Frication amplitude |
| OQ | 0-100% | Open quotient |
| TL | 0-42 dB | Spectral tilt |

### 5.4 Simplified KLATT Implementation

```cpp
class KLATTSynth
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        glottal.prepare(sampleRate);
        for (auto& f : cascadeFormants)
            f.reset();
    }

    float processSample()
    {
        // Source
        float voiced = glottal.process(f0) * voicingAmplitude;
        float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
        float aspiration = noise * aspirationAmplitude;

        // Apply spectral tilt to voiced source
        float tiltedVoice = tiltFilter.process(voiced);

        // Cascade branch (vowels)
        float cascadeInput = tiltedVoice + aspiration;
        float cascadeOutput = cascadeInput;
        for (int i = 0; i < 5; ++i)
            cascadeOutput = cascadeFormants[i].process(cascadeOutput);

        // Parallel branch (fricatives)
        float parallelOutput = 0.0f;
        float fricNoise = noise * fricationAmplitude;
        for (int i = 0; i < 5; ++i)
            parallelOutput += parallelFormants[i].process(fricNoise);

        return cascadeOutput + parallelOutput;
    }

    float f0 = 120.0f;
    float voicingAmplitude = 0.5f;
    float aspirationAmplitude = 0.01f;
    float fricationAmplitude = 0.0f;

private:
    double sr = 44100.0;
    LFModel glottal;
    FormantFilter cascadeFormants[5];
    FormantFilter parallelFormants[5];
    juce::IIRFilter tiltFilter;
};
```

---

## 6. FOF Synthesis

### 6.1 Fonction d'Onde Formantique

FOF (Formant Wave Function) synthesis, developed at IRCAM, generates each formant as a grain-like impulse response rather than using continuous filters:

- Each FOF grain is a damped sinusoid at the formant frequency
- Grains are emitted at the fundamental frequency rate
- The envelope of each grain defines the formant bandwidth
- Multiple overlapping FOF grains create rich, natural-sounding vowels

### 6.2 FOF Grain Structure

```cpp
struct FOFGrain
{
    float formantFreq;  // Center frequency of formant
    float bandwidth;    // Bandwidth (controls decay rate)
    float amplitude;    // Peak amplitude
    float duration;     // Grain duration in seconds

    float generate(float t, double sampleRate) const
    {
        if (t > duration) return 0.0f;

        float decay = std::exp(-juce::MathConstants<float>::pi * bandwidth * t);
        float sine = std::sin(2.0f * juce::MathConstants<float>::pi * formantFreq * t);

        // Attack envelope (smooth onset)
        float attack = 1.0f;
        float attackTime = 0.003f; // 3 ms attack
        if (t < attackTime)
            attack = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * t / attackTime));

        return amplitude * attack * decay * sine;
    }
};
```

### 6.3 Advantages of FOF

- More natural attack transients than continuous formant filters
- Better control over formant phase relationships
- Easier to create non-steady-state sounds (consonant-vowel transitions)
- Used extensively in CHANT (IRCAM) for singing synthesis

---

## 7. Phase Vocoder Approaches

### 7.1 Formant Preservation During Pitch Shifting

When pitch-shifting vocal material, formants must be preserved to avoid the "chipmunk effect." The phase vocoder can be modified for formant-preserving pitch shift:

1. Analyze the spectrum to identify formant peaks
2. Shift the fundamental and harmonics
3. Apply the original formant envelope to the shifted spectrum

```cpp
void formantPreservingPitchShift(
    float* magnitudes, float* phases, int fftSize,
    float pitchShiftFactor, float sampleRate)
{
    // Step 1: Extract spectral envelope (formants)
    std::vector<float> envelope(fftSize / 2);
    extractSpectralEnvelope(magnitudes, envelope.data(), fftSize / 2);

    // Step 2: Shift harmonics
    std::vector<float> shiftedMags(fftSize / 2, 0.0f);
    for (int bin = 0; bin < fftSize / 2; ++bin)
    {
        int targetBin = static_cast<int>(bin * pitchShiftFactor);
        if (targetBin < fftSize / 2)
            shiftedMags[targetBin] += magnitudes[bin];
    }

    // Step 3: Apply original envelope to shifted spectrum
    for (int bin = 0; bin < fftSize / 2; ++bin)
    {
        float shiftedEnvBin = bin / pitchShiftFactor;
        if (shiftedEnvBin < fftSize / 2)
        {
            float envValue = envelope[(int)shiftedEnvBin];
            float shiftedEnvValue = envelope[bin];
            if (shiftedEnvValue > 0.001f)
                magnitudes[bin] = shiftedMags[bin] * (envValue / shiftedEnvValue);
        }
    }
}
```

### 7.2 Spectral Envelope Estimation

Methods for extracting the formant envelope from a magnitude spectrum:
- **Cepstral method**: Lifter the cepstrum to separate fine structure (harmonics) from envelope (formants)
- **LPC (Linear Predictive Coding)**: Fit an all-pole model to the spectrum
- **True envelope**: Iterative method that converges to the smooth spectral envelope

---

## 8. Phoneme-Based Synthesis

### 8.1 Phoneme Categories

| Category | Examples | Synthesis Approach |
|----------|----------|-------------------|
| Vowels | /a/, /i/, /u/, /e/, /o/ | Formant filter bank |
| Plosives | /p/, /b/, /t/, /d/, /k/, /g/ | Burst noise + formant transition |
| Fricatives | /s/, /f/, /sh/, /v/, /z/ | Filtered noise |
| Nasals | /m/, /n/, /ng/ | Formants + nasal resonance |
| Liquids | /l/, /r/ | Formant transitions |
| Glides | /w/, /y/ | Rapid formant movement |

### 8.2 Coarticulation

In natural speech, phonemes blend into each other. Formant transitions between phonemes typically take 50-100 ms and follow smooth trajectories:

```cpp
class PhonemeTransition
{
public:
    void setTransition(const VowelPreset& from, const VowelPreset& to, float durationMs, double sampleRate)
    {
        startPreset = from;
        endPreset = to;
        transitionSamples = static_cast<int>(durationMs * 0.001f * sampleRate);
        currentSample = 0;
    }

    VowelPreset getCurrentFormants()
    {
        float t = (float)currentSample / (float)transitionSamples;
        t = juce::jlimit(0.0f, 1.0f, t);
        // Smooth ease-in/ease-out
        t = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * t));

        currentSample++;
        return morphVowels(startPreset, endPreset, t);
    }

private:
    VowelPreset startPreset, endPreset;
    int transitionSamples = 0;
    int currentSample = 0;
};
```

---

## Part 3: Advanced Topics

## 9. Singing Synthesis

### 9.1 Singing vs. Speech

Singing differs from speech in several important ways:
- Sustained, precisely controlled pitch
- Vibrato (5-7 Hz, +/- 50-100 cents typical)
- Extended vowel durations
- Greater dynamic range
- Different formant tuning (singers adjust formants to enhance projection)

### 9.2 Singer's Formant

Trained singers develop a "singer's formant" -- a clustering of F3, F4, and F5 around 2500-3000 Hz that creates a resonance peak allowing the voice to cut through an orchestra.

```cpp
void addSingersFormant(FormantFilter* formants)
{
    // Cluster F3-F5 around 2800 Hz
    formants[2].setFormant(2700.0f, 120.0f, -8.0f, sr);
    formants[3].setFormant(2900.0f, 150.0f, -10.0f, sr);
    formants[4].setFormant(3100.0f, 200.0f, -12.0f, sr);
}
```

### 9.3 Vibrato Implementation

```cpp
class SingingVibrato
{
public:
    void prepare(double sampleRate)
    {
        vibratoLFO.prepare(sampleRate);
        vibratoLFO.setFrequency(5.5f); // ~5.5 Hz vibrato rate
    }

    float modulatePitch(float basePitch)
    {
        float vibratoValue = vibratoLFO.process();
        // +/- 70 cents of vibrato depth
        float semitones = vibratoDepthCents / 100.0f * vibratoValue;
        return basePitch * std::pow(2.0f, semitones / 12.0f);
    }

    float vibratoDepthCents = 70.0f;

private:
    LFO vibratoLFO;
};
```

---

## 10. Vocal Effects Processing

### 10.1 Vowel Filter (Wah-Wah Variant)

A vowel filter sweeps between vowel formant presets, usable as a creative effect:

```cpp
class VowelFilter
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        for (auto& f : formants)
            f.reset();
    }

    float process(float input, float vowelPosition)
    {
        // Map position to vowel pair
        // 0.0 = A, 0.25 = E, 0.5 = I, 0.75 = O, 1.0 = U
        const VowelPreset* presets[] = {&vowelA, &vowelE, &vowelI, &vowelO, &vowelU};
        float scaledPos = vowelPosition * 4.0f;
        int idx1 = juce::jlimit(0, 3, (int)scaledPos);
        int idx2 = juce::jmin(idx1 + 1, 4);
        float frac = scaledPos - (float)idx1;

        VowelPreset current = morphVowels(*presets[idx1], *presets[idx2], frac);

        // Update formant filters
        for (int i = 0; i < 5; ++i)
            formants[i].setFormant(current.f[i], current.bw[i], current.g[i], sr);

        // Process through parallel formant bank
        float output = 0.0f;
        for (int i = 0; i < 5; ++i)
            output += formants[i].process(input);

        return output;
    }

private:
    FormantFilter formants[5];
    double sr = 44100.0;
};
```

### 10.2 Formant Shifting

Shifting all formant frequencies up or down changes the perceived vocal character without affecting pitch:

```cpp
void shiftFormants(VowelPreset& preset, float shiftFactor)
{
    for (int i = 0; i < 5; ++i)
        preset.f[i] *= shiftFactor;
}
// shiftFactor > 1.0 = smaller vocal tract (child/female)
// shiftFactor < 1.0 = larger vocal tract (deep male)
```

---

## 11. ML-Based Vocal Synthesis

### 11.1 WORLD Vocoder

WORLD is an open-source vocoder for speech analysis/synthesis:
- **DIO/Harvest**: F0 estimation (pitch detection)
- **CheapTrick**: Spectral envelope estimation
- **D4C**: Aperiodicity estimation (breathiness)
- Enables pitch and formant manipulation with high quality
- C++ implementation available, suitable for real-time with optimization

### 11.2 Neural Vocoders

Modern neural vocoders generate audio from intermediate representations:
- **WaveNet**: Autoregressive, very high quality but slow
- **WaveRNN**: Faster autoregressive model
- **HiFi-GAN**: GAN-based, real-time capable
- **Vocos**: Lightweight vocoder for efficient synthesis

### 11.3 Practical Considerations for Plugins

ML-based vocal synthesis in plugins faces challenges:
- Model size (10-100+ MB)
- Inference latency (must meet real-time buffer deadlines)
- CPU/GPU requirements
- Cross-platform deployment (ONNX Runtime, TensorFlow Lite, or libtorch)

For real-time plugin use, formant filter banks remain the most practical approach. ML techniques are better suited for offline processing or dedicated synthesis applications.

---

## Part 4: JUCE Implementation

## 12. Formant Filter Implementation

### 12.1 JUCE IIR-Based Formant Filter

```cpp
class JUCEFormantFilter
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        for (auto& filter : filters)
            filter.reset();
    }

    void setFormant(int index, float freq, float bandwidth, float gaindB)
    {
        if (index < 0 || index >= NumFormants) return;

        float Q = freq / bandwidth;
        auto coeffs = juce::IIRCoefficients::makePeakFilter(sr, freq, Q,
            juce::Decibels::decibelsToGain(gaindB));
        filters[index].setCoefficients(coeffs);
    }

    float process(float input)
    {
        float output = 0.0f;
        for (int i = 0; i < NumFormants; ++i)
            output += filters[i].processSingleSampleRaw(input);
        return output;
    }

private:
    static constexpr int NumFormants = 5;
    juce::IIRFilter filters[NumFormants];
    double sr = 44100.0;
};
```

---

## 13. Complete Vocal Synthesizer

### 13.1 Plugin Architecture

```cpp
class VocalSynthProcessor : public juce::AudioProcessor
{
public:
    VocalSynthProcessor()
        : AudioProcessor(BusesProperties()
            .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        sr = sampleRate;
        glottalSource.prepare(sampleRate);
        for (auto& f : formantFilters)
            f.reset();
        vibrato.prepare(sampleRate);

        // Initialize with vowel /a/
        setVowel(vowelA);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        juce::ScopedNoDenormals noDenormals;
        buffer.clear();

        // Handle MIDI for pitch control
        for (const auto metadata : midi)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                float freq = (float)juce::MidiMessage::getMidiNoteInHertz(msg.getNoteNumber());
                currentPitch = freq;
                noteOn = true;
            }
            else if (msg.isNoteOff())
            {
                noteOn = false;
            }
        }

        if (!noteOn) return;

        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getWritePointer(1);

        // Morph vowel based on parameter
        float morphPos = vowelMorphParam;
        const VowelPreset* presets[] = {&vowelA, &vowelE, &vowelI, &vowelO, &vowelU};
        float scaledPos = morphPos * 4.0f;
        int idx1 = juce::jlimit(0, 3, (int)scaledPos);
        int idx2 = juce::jmin(idx1 + 1, 4);
        float frac = scaledPos - (float)idx1;
        VowelPreset current = morphVowels(*presets[idx1], *presets[idx2], frac);
        setVowel(current);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float pitch = vibrato.modulatePitch(currentPitch);
            float source = glottalSource.process(pitch);

            // Add breathiness
            float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * 0.02f;
            source += noise * breathAmount;

            // Apply formant filters (parallel)
            float output = 0.0f;
            for (int f = 0; f < 5; ++f)
                output += formantFilters[f].processSingleSampleRaw(source);

            output *= outputGain;
            outL[i] = output;
            outR[i] = output;
        }
    }

private:
    double sr = 44100.0;
    RosenbergPulse glottalSource;
    juce::IIRFilter formantFilters[5];
    SingingVibrato vibrato;
    float currentPitch = 220.0f;
    bool noteOn = false;
    float vowelMorphParam = 0.0f;
    float breathAmount = 0.1f;
    float outputGain = 0.3f;

    void setVowel(const VowelPreset& preset)
    {
        for (int i = 0; i < 5; ++i)
        {
            float Q = preset.f[i] / preset.bw[i];
            formantFilters[i].setCoefficients(
                juce::IIRCoefficients::makePeakFilter(
                    sr, preset.f[i], Q,
                    juce::Decibels::decibelsToGain(preset.g[i])));
        }
    }
};
```

---

## 14. Optimization and Real-Time Safety

### 14.1 Filter State Management

Reset filter states when notes restart to avoid artifacts:

```cpp
void noteOnReset()
{
    for (auto& f : formantFilters)
        f.reset();
    glottalSource.reset();
}
```

### 14.2 Formant Parameter Smoothing

Abrupt formant changes cause clicks. Use smoothed value updates:

```cpp
juce::SmoothedValue<float> smoothedF1, smoothedF2, smoothedF3;

void prepareToPlay(double sampleRate, int)
{
    smoothedF1.reset(sampleRate, 0.02); // 20 ms smoothing
    smoothedF2.reset(sampleRate, 0.02);
    smoothedF3.reset(sampleRate, 0.02);
}
```

### 14.3 Common Pitfalls

| Pitfall | Impact | Solution |
|---------|--------|----------|
| Updating formant coefficients every sample | CPU waste | Update per block or with SmoothedValue |
| Not clamping formant frequencies | Unstable filters (freq > Nyquist/2) | Clamp to 0.45 * sampleRate |
| Missing denormal protection | CPU spikes on silence | Use ScopedNoDenormals |
| Abrupt vowel transitions | Clicks and pops | Smooth formant interpolation over 20+ ms |
| Ignoring spectral tilt | Unnatural, bright voice | Apply -12 dB/oct tilt to glottal source |

---

## Part 5: References

## 15. References and Further Reading

### Academic Papers
- Klatt, D.H. (1980). "Software for a cascade/parallel formant synthesizer." JASA, 67(3).
- Fant, G. (1970). *Acoustic Theory of Speech Production*. Mouton.
- Rodet, X. (1984). "Time-Domain Formant-Wave-Function Synthesis." Computer Music Journal.
- Kawahara, H. et al. (2017). "WORLD: A Vocoder-Based High-Quality Speech Synthesis System." IEICE Trans.

### Books
- Cook, P.R. (2002). *Real Sound Synthesis for Interactive Applications*. Chapter 9: The Voice.
- Smith, J.O. *Physical Audio Signal Processing*. Online book, Stanford. Chapter: Vocal Tract Models.
- Dodge, C. & Jerse, T.A. (1997). *Computer Music*. Chapter 7: Speech Synthesis.

### Software References
- WORLD vocoder: https://github.com/mmorise/World
- eSpeak: Open-source formant synthesizer
- Pink Trombone: Interactive vocal tract simulator (browser-based)
- Praat: Acoustic analysis software with formant tracking

### Vowel Databases
- Peterson & Barney (1952): Classic American English vowel formant measurements
- Hillenbrand et al. (1995): Updated vowel database with more speakers
- IPA (International Phonetic Association): Standard phonemic notation

---

*Research document for O-Lyrica. Covers source-filter vocal model, formant filter banks, KLATT synthesis, singing synthesis, and phoneme-based approaches with JUCE implementation.*
