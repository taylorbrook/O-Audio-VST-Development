---
title: "Tremolo & Amplitude Modulation"
created: 2026-03-07
juce_version: "8.0.4"
summary: "Complete technical reference for tremolo and amplitude modulation effects in audio plugins, covering bias tremolo, optical tremolo, harmonic tremolo, ring modulation, auto-pan, and analog circuit emulation with JUCE implementation."
domain: dsp
type: guide
keywords:
  - tremolo
  - amplitude-modulation
  - ring-modulation
  - auto-pan
  - lfo
  - bias-tremolo
  - optical-tremolo
  - harmonic-tremolo
  - juce-dsp
stages: [0, 1, 2]
agents: [dsp, research]
---

# Tremolo & Amplitude Modulation

**Complete Technical Reference for Tremolo and AM Effects in Audio Plugins**

**Created:** March 2026
**Version:** 1.0
**Research Depth:** Level 3 (Comprehensive Investigation)

---

## Executive Summary

This document covers tremolo and amplitude modulation effects for audio plugin development. While conceptually simple -- multiplying a signal by an LFO -- the nuances of analog tremolo emulation, different circuit topologies, and AM variants create a rich design space. This guide covers bias tremolo, optical tremolo, harmonic tremolo, ring modulation, and auto-pan effects.

**Key Findings:**
- Tremolo is amplitude modulation where the modulator stays in the sub-audio range (0.5-20 Hz)
- Analog tremolo character comes from circuit topology: bias, optical, and harmonic types each have distinct sounds
- Optical tremolo has asymmetric attack/release due to photoresistor characteristics
- Harmonic tremolo applies amplitude modulation differently to low and high frequency bands
- Ring modulation uses audio-rate modulation frequencies, creating sum and difference tones
- JUCE's `SmoothedValue` and LFO utilities simplify implementation

---

## Table of Contents

### Part 1: Fundamentals
1. [Amplitude Modulation Theory](#1-amplitude-modulation-theory)
2. [LFO Design for Tremolo](#2-lfo-design-for-tremolo)
3. [Modulation Depth and Symmetry](#3-modulation-depth-and-symmetry)

### Part 2: Tremolo Types
4. [Bias Tremolo](#4-bias-tremolo)
5. [Optical Tremolo](#5-optical-tremolo)
6. [Harmonic Tremolo](#6-harmonic-tremolo)

### Part 3: Related Effects
7. [Ring Modulation](#7-ring-modulation)
8. [Auto-Pan](#8-auto-pan)
9. [Amplitude LFO with Tempo Sync](#9-amplitude-lfo-with-tempo-sync)

### Part 4: JUCE Implementation
10. [Complete Tremolo Plugin](#10-complete-tremolo-plugin)
11. [Optimization and Real-Time Safety](#11-optimization-and-real-time-safety)

### Part 5: References
12. [References and Further Reading](#12-references-and-further-reading)

---

## Part 1: Fundamentals

## 1. Amplitude Modulation Theory

### 1.1 Basic AM Formula

Amplitude modulation multiplies the audio signal by a modulating waveform:

```
output(t) = input(t) * (1 + depth * mod(t))
```

Where:
- `depth` controls modulation intensity (0 = none, 1 = full)
- `mod(t)` is the modulating waveform, typically an LFO oscillating between -1 and +1

### 1.2 Unipolar vs. Bipolar Modulation

**Unipolar modulation (tremolo):** The gain oscillates between `(1-depth)` and `1`:
```
output = input * (1 - depth * (1 - mod) / 2)
```

**Bipolar modulation (ring mod):** The gain passes through zero:
```
output = input * mod
```

The key distinction: tremolo never inverts the signal's polarity (unipolar), while ring modulation does (bipolar).

### 1.3 Spectral Effects

| Mod Frequency | Perception | Spectrum |
|---------------|------------|----------|
| 0.5-7 Hz | Tremolo (rhythmic pulsing) | Unchanged (just envelope modulation) |
| 7-20 Hz | Fast tremolo / flutter | Slight spectral smearing |
| 20+ Hz | Ring modulation | Sidebands at fc +/- fm |

When the modulation frequency enters the audio range (>20 Hz), sidebands appear in the spectrum at the sum and difference frequencies, creating the characteristic ring modulation sound.

---

## 2. LFO Design for Tremolo

### 2.1 Common LFO Shapes

```cpp
class TremoloLFO
{
public:
    enum Shape { Sine, Triangle, Square, SoftSquare, RampUp, RampDown };

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        phase = 0.0f;
    }

    void setRate(float rateHz) { increment = rateHz / (float)sr; }

    float process()
    {
        float output = 0.0f;

        switch (currentShape)
        {
            case Sine:
                output = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
                break;

            case Triangle:
                output = 4.0f * std::abs(phase - 0.5f) - 1.0f;
                break;

            case Square:
                output = phase < 0.5f ? 1.0f : -1.0f;
                break;

            case SoftSquare:
            {
                // Square with rounded edges (tanh shaping)
                float sine = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
                output = std::tanh(sine * 4.0f);
                break;
            }

            case RampUp:
                output = 2.0f * phase - 1.0f;
                break;

            case RampDown:
                output = 1.0f - 2.0f * phase;
                break;
        }

        phase += increment;
        if (phase >= 1.0f) phase -= 1.0f;

        return output;
    }

    Shape currentShape = Sine;

private:
    double sr = 44100.0;
    float phase = 0.0f;
    float increment = 0.0f;
};
```

### 2.2 Waveshaping for Analog Character

Real analog tremolo LFOs are never perfect waveforms. Adding subtle waveshaping creates more organic modulation:

```cpp
float analogSine(float phase)
{
    // Sine with slight asymmetry (models tube-based oscillator)
    float sine = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
    // Add small 2nd harmonic for asymmetry
    float second = std::sin(4.0f * juce::MathConstants<float>::pi * phase) * 0.05f;
    return sine + second;
}
```

### 2.3 Rate Ranges

| Speed | Rate (Hz) | Character | Musical Use |
|-------|-----------|-----------|-------------|
| Very slow | 0.5-2 | Gentle swell | Ambient, sustain |
| Slow | 2-5 | Classic tremolo | Ballads, clean guitar |
| Medium | 5-8 | Standard tremolo | Rock, country |
| Fast | 8-12 | Intense pulsing | Surf rock, thriller |
| Very fast | 12-20 | Stuttering / flutter | Experimental |

---

## 3. Modulation Depth and Symmetry

### 3.1 Depth Control

```cpp
float applyTremolo(float input, float lfoValue, float depth)
{
    // Unipolar: gain ranges from (1-depth) to 1
    float gain = 1.0f - depth * (1.0f - lfoValue) * 0.5f;
    return input * gain;
}
```

### 3.2 Asymmetric Modulation

Some vintage tremolos have asymmetric modulation -- the dip is deeper than the rise:

```cpp
float asymmetricDepth(float lfoValue, float depth, float asymmetry)
{
    // asymmetry: 0 = symmetric, 1 = dip only, -1 = rise only
    float positive = juce::jmax(lfoValue, 0.0f);
    float negative = juce::jmin(lfoValue, 0.0f);

    float shaped = positive * (1.0f - asymmetry * 0.5f)
                 + negative * (1.0f + asymmetry * 0.5f);

    return 1.0f - depth * (1.0f - shaped) * 0.5f;
}
```

---

## Part 2: Tremolo Types

## 4. Bias Tremolo

### 4.1 Circuit Description

Bias tremolo modulates the bias point of an amplification stage (typically a tube). By shifting the operating point, the gain of the stage varies with the LFO.

### 4.2 Characteristics

| Property | Bias Tremolo |
|----------|-------------|
| Sound | Hard, choppy at high depth |
| Waveform interaction | Amplitude AND harmonic content change |
| Symmetry | Can be asymmetric depending on tube bias point |
| Low-depth character | Subtle, warm |
| High-depth character | Aggressive, on/off switching |
| Classic examples | Fender Blackface amps, Vox AC30 |

### 4.3 DSP Implementation

The key difference from simple amplitude modulation: bias tremolo also changes the distortion characteristics as the bias point moves:

```cpp
class BiasTremolo
{
public:
    void prepare(double sampleRate)
    {
        lfo.prepare(sampleRate);
    }

    float process(float input)
    {
        float lfoValue = lfo.process();

        // Bias offset modulates the operating point
        float biasOffset = depth * lfoValue;

        // The signal is processed through the tube model at the shifted bias
        float biasedInput = input + biasOffset * biasRange;

        // Tube-like transfer curve (asymmetric soft clipping)
        float output = tubeSaturation(biasedInput);

        // The bias modulation changes both gain and harmonic content
        return output;
    }

    float depth = 0.5f;
    float biasRange = 0.3f; // How much the bias shifts

private:
    TremoloLFO lfo;

    float tubeSaturation(float input)
    {
        // Asymmetric soft clipping (models tube bias variation)
        if (input > 0.0f)
            return std::tanh(input);
        else
            return std::tanh(input * 0.7f) / 0.7f; // Softer negative clipping
    }
};
```

### 4.4 Fender Blackface Tremolo

The Fender Blackface tremolo circuit modulates the bias of a gain stage using an oscillator built from half of a 12AX7 tube:
- LFO shape is between sine and triangle
- Rate range: approximately 2-10 Hz
- At maximum depth, the signal is nearly muted at the LFO trough
- The oscillator frequency affects the LFO shape slightly (faster = more triangular)

---

## 5. Optical Tremolo

### 5.1 Circuit Description

Optical tremolo uses a light source (neon bulb or LED) modulated by the LFO and a photoresistor (LDR) that acts as a variable resistor controlling the signal amplitude.

### 5.2 Characteristics

| Property | Optical Tremolo |
|----------|----------------|
| Sound | Smooth, organic |
| Waveform interaction | Pure amplitude modulation (no harmonic change) |
| Symmetry | Asymmetric (slow rise, faster fall due to LDR characteristics) |
| Low-depth character | Gentle, musical |
| High-depth character | Pulsing, breathing |
| Classic examples | Fender Brownface amps, Demeter Tremulator |

### 5.3 LDR Response Modeling

The LDR (light-dependent resistor) has critical nonlinear properties:
- **Turn-on time:** 5-20 ms (light -> low resistance)
- **Turn-off time:** 50-200 ms (dark -> high resistance)
- **Resistance range:** 100 ohms (bright) to 10 Mohms (dark)

```cpp
class OpticalTremolo
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        lfo.prepare(sampleRate);

        // LDR time constants
        float attackMs = 10.0f;
        float releaseMs = 100.0f;
        attackCoeff = std::exp(-1.0f / (0.001f * attackMs * (float)sr));
        releaseCoeff = std::exp(-1.0f / (0.001f * releaseMs * (float)sr));
    }

    float process(float input)
    {
        float lfoValue = lfo.process();

        // Convert LFO to light level (0 = dark, 1 = bright)
        float lightLevel = (lfoValue + 1.0f) * 0.5f * depth;

        // Model LDR response: fast turn-on, slow turn-off
        if (lightLevel > ldrState)
            ldrState += (1.0f - attackCoeff) * (lightLevel - ldrState);
        else
            ldrState += (1.0f - releaseCoeff) * (lightLevel - ldrState);

        // LDR controls signal attenuation
        // Higher light = lower resistance = less attenuation
        float gain = 1.0f - ldrState;

        return input * gain;
    }

    float depth = 0.7f;

private:
    TremoloLFO lfo;
    double sr = 44100.0;
    float ldrState = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
};
```

### 5.4 The "Breathing" Quality

Optical tremolo's asymmetric response creates a distinctive "breathing" quality:
- The gain dips quickly (LDR responds fast to light)
- The gain recovers slowly (LDR takes longer to go dark)
- This creates a rhythmic "in-out" feel that sounds more organic than mathematical tremolo

---

## 6. Harmonic Tremolo

### 6.1 Circuit Description

Harmonic tremolo (also called "vibratone" or "vibrato" in some Fender amps) splits the signal into low and high frequency bands, then modulates each band with opposite-phase LFOs.

### 6.2 Characteristics

| Property | Harmonic Tremolo |
|----------|-----------------|
| Sound | Phaser-like, complex |
| Waveform interaction | Frequency-dependent amplitude modulation |
| Band interaction | Low and high bands pulse in alternation |
| Character | Rich, evolving, spatial |
| Classic examples | Fender Brownface "vibrato" channel |

### 6.3 DSP Implementation

```cpp
class HarmonicTremolo
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        lfo.prepare(sampleRate);

        // Crossover filter: split around 800 Hz
        float crossoverFreq = 800.0f;
        lowFilter.setCoefficients(
            juce::IIRCoefficients::makeLowPass(sampleRate, crossoverFreq));
        highFilter.setCoefficients(
            juce::IIRCoefficients::makeHighPass(sampleRate, crossoverFreq));
    }

    float process(float input)
    {
        // Split into low and high bands
        float lowBand = lowFilter.processSingleSampleRaw(input);
        float highBand = highFilter.processSingleSampleRaw(input);

        // Modulate each band with opposite-phase LFO
        float lfoValue = lfo.process();
        float lowGain = 1.0f - depth * (1.0f - lfoValue) * 0.5f;
        float highGain = 1.0f - depth * (1.0f + lfoValue) * 0.5f; // Inverted

        return lowBand * lowGain + highBand * highGain;
    }

    float depth = 0.7f;

private:
    TremoloLFO lfo;
    juce::IIRFilter lowFilter;
    juce::IIRFilter highFilter;
    double sr = 44100.0;
};
```

### 6.4 Why It Sounds Like a Phaser

When the low band is loud and the high band is quiet (and vice versa), the spectral balance shifts cyclically. This creates a sweeping tonal change similar to a phaser, but achieved through amplitude modulation rather than allpass phase shifting.

---

## Part 3: Related Effects

## 7. Ring Modulation

### 7.1 Ring Modulation vs. Tremolo

Ring modulation uses audio-rate modulation frequencies (20 Hz - 5000+ Hz) and bipolar multiplication (the signal passes through zero):

```cpp
class RingModulator
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        phase = 0.0f;
    }

    float process(float input)
    {
        // Bipolar multiplication: carrier * modulator
        float modulator = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
        phase += frequency / (float)sr;
        if (phase >= 1.0f) phase -= 1.0f;

        // Ring mod = pure multiplication (no original signal)
        float ringMod = input * modulator;

        // Optional mix with original for partial ring mod
        return input * (1.0f - mix) + ringMod * mix;
    }

    float frequency = 440.0f; // Carrier frequency
    float mix = 1.0f;         // 0 = dry, 1 = full ring mod

private:
    double sr = 44100.0;
    float phase = 0.0f;
};
```

### 7.2 Spectral Content

For an input signal at frequency `fc` and a ring modulator at frequency `fm`:
- Output contains: `fc + fm` and `fc - fm` (sum and difference)
- The original `fc` and `fm` are suppressed
- For non-sinusoidal inputs, every harmonic produces sum/difference pairs

### 7.3 Musical Ring Modulation

To create musical (tuned) ring modulation:

```cpp
float musicalRingMod(float input, float inputPitch, float intervalSemitones)
{
    // Set ring mod frequency to create a musical interval
    float modFreq = inputPitch * std::pow(2.0f, intervalSemitones / 12.0f);
    // This creates sum and difference tones that are harmonically related
    float modulator = std::sin(2.0f * juce::MathConstants<float>::pi * modPhase);
    modPhase += modFreq / (float)sr;
    if (modPhase >= 1.0f) modPhase -= 1.0f;
    return input * modulator;
}
```

---

## 8. Auto-Pan

### 8.1 Stereo Amplitude Modulation

Auto-pan is tremolo applied to the stereo field, panning the signal left and right:

```cpp
class AutoPan
{
public:
    void prepare(double sampleRate)
    {
        lfo.prepare(sampleRate);
    }

    void process(float inputL, float inputR, float& outL, float& outR)
    {
        float lfoValue = lfo.process();

        // Convert LFO to panning position (0 = left, 1 = right)
        float panPos = (lfoValue + 1.0f) * 0.5f;
        panPos = panPos * depth + 0.5f * (1.0f - depth); // Limit range by depth

        // Equal-power panning law
        float gainL = std::cos(panPos * juce::MathConstants<float>::halfPi);
        float gainR = std::sin(panPos * juce::MathConstants<float>::halfPi);

        outL = (inputL + inputR) * 0.5f * gainL;
        outR = (inputL + inputR) * 0.5f * gainR;
    }

    float depth = 1.0f; // 0 = no pan, 1 = full L-R sweep

private:
    TremoloLFO lfo;
};
```

### 8.2 Stereo Tremolo Variants

| Variant | Left | Right | Effect |
|---------|------|-------|--------|
| Mono tremolo | Same LFO | Same LFO | Volume pulses, no movement |
| Stereo tremolo | LFO | LFO + 90 deg | Slight stereo motion |
| Auto-pan | LFO -> L gain | Inverted -> R gain | Full L-R sweep |
| Stereo field pulsing | LFO -> width | LFO -> width | Stereo image breathes |

---

## 9. Amplitude LFO with Tempo Sync

### 9.1 BPM-Synced Tremolo

```cpp
class TempoSyncTremolo
{
public:
    enum Division { Whole, Half, Quarter, Eighth, Sixteenth, DottedQuarter, TripletQuarter };

    void setTempo(double bpm, Division division)
    {
        double beatsPerSecond = bpm / 60.0;
        double divisor = 1.0;

        switch (division)
        {
            case Whole:           divisor = 0.25; break;
            case Half:            divisor = 0.5; break;
            case Quarter:         divisor = 1.0; break;
            case Eighth:          divisor = 2.0; break;
            case Sixteenth:       divisor = 4.0; break;
            case DottedQuarter:   divisor = 2.0 / 3.0; break;
            case TripletQuarter:  divisor = 3.0 / 2.0; break;
        }

        float rateHz = (float)(beatsPerSecond * divisor);
        lfo.setRate(rateHz);
    }

    float process(float input)
    {
        float lfoValue = lfo.process();
        float gain = 1.0f - depth * (1.0f - lfoValue) * 0.5f;
        return input * gain;
    }

    float depth = 0.7f;

private:
    TremoloLFO lfo;
};
```

### 9.2 Host Transport Sync in JUCE

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
{
    if (auto* playHead = getPlayHead())
    {
        auto position = playHead->getPosition();
        if (position.hasValue())
        {
            if (auto bpm = position->getBpm())
            {
                tempoSync.setTempo(*bpm, currentDivision);
            }

            // For tight sync, use the playhead position directly
            if (auto ppq = position->getPpqPosition())
            {
                // Reset LFO phase based on bar position for drift-free sync
                double ppqPos = *ppq;
                float phaseFromPPQ = std::fmod((float)ppqPos * divisorMultiplier, 1.0f);
                lfo.setPhase(phaseFromPPQ);
            }
        }
    }

    // Process audio with synced tremolo...
}
```

---

## Part 4: JUCE Implementation

## 10. Complete Tremolo Plugin

### 10.1 Full-Featured Tremolo Processor

```cpp
class TremoloProcessor : public juce::AudioProcessor
{
public:
    TremoloProcessor()
        : AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "PARAMS", createParameterLayout())
    {
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        sr = sampleRate;
        lfo.prepare(sampleRate);
        smoothedDepth.reset(sampleRate, 0.02);
        smoothedRate.reset(sampleRate, 0.02);

        // LDR filter for optical mode
        ldrAttackCoeff = std::exp(-1.0f / (0.01f * (float)sampleRate));
        ldrReleaseCoeff = std::exp(-1.0f / (0.1f * (float)sampleRate));
        ldrState = 0.0f;

        // Crossover for harmonic mode
        lowFilter[0].setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate, 800.0));
        lowFilter[1].setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate, 800.0));
        highFilter[0].setCoefficients(juce::IIRCoefficients::makeHighPass(sampleRate, 800.0));
        highFilter[1].setCoefficients(juce::IIRCoefficients::makeHighPass(sampleRate, 800.0));
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;

        // Read parameters
        float rate = apvts.getRawParameterValue("rate")->load();
        float depth = apvts.getRawParameterValue("depth")->load();
        int mode = static_cast<int>(apvts.getRawParameterValue("mode")->load());
        int shape = static_cast<int>(apvts.getRawParameterValue("shape")->load());

        smoothedRate.setTargetValue(rate);
        smoothedDepth.setTargetValue(depth);
        lfo.currentShape = static_cast<TremoloLFO::Shape>(shape);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float currentRate = smoothedRate.getNextValue();
            float currentDepth = smoothedDepth.getNextValue();
            lfo.setRate(currentRate);

            float lfoValue = lfo.process();

            switch (mode)
            {
                case 0: // Standard
                    processStandard(buffer, i, lfoValue, currentDepth);
                    break;

                case 1: // Optical
                    processOptical(buffer, i, lfoValue, currentDepth);
                    break;

                case 2: // Harmonic
                    processHarmonic(buffer, i, lfoValue, currentDepth);
                    break;
            }
        }
    }

private:
    double sr = 44100.0;
    TremoloLFO lfo;
    juce::SmoothedValue<float> smoothedDepth, smoothedRate;
    juce::AudioProcessorValueTreeState apvts;

    // Optical mode
    float ldrState = 0.0f;
    float ldrAttackCoeff = 0.0f;
    float ldrReleaseCoeff = 0.0f;

    // Harmonic mode
    juce::IIRFilter lowFilter[2], highFilter[2];

    void processStandard(juce::AudioBuffer<float>& buffer, int sample,
                         float lfoValue, float depth)
    {
        float gain = 1.0f - depth * (1.0f - lfoValue) * 0.5f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.setSample(ch, sample, buffer.getSample(ch, sample) * gain);
    }

    void processOptical(juce::AudioBuffer<float>& buffer, int sample,
                        float lfoValue, float depth)
    {
        float lightLevel = (lfoValue + 1.0f) * 0.5f * depth;

        if (lightLevel > ldrState)
            ldrState += (1.0f - ldrAttackCoeff) * (lightLevel - ldrState);
        else
            ldrState += (1.0f - ldrReleaseCoeff) * (lightLevel - ldrState);

        float gain = 1.0f - ldrState;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.setSample(ch, sample, buffer.getSample(ch, sample) * gain);
    }

    void processHarmonic(juce::AudioBuffer<float>& buffer, int sample,
                         float lfoValue, float depth)
    {
        float lowGain = 1.0f - depth * (1.0f - lfoValue) * 0.5f;
        float highGain = 1.0f - depth * (1.0f + lfoValue) * 0.5f;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float input = buffer.getSample(ch, sample);
            float low = lowFilter[ch].processSingleSampleRaw(input) * lowGain;
            float high = highFilter[ch].processSingleSampleRaw(input) * highGain;
            buffer.setSample(ch, sample, low + high);
        }
    }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "rate", "Rate", juce::NormalisableRange<float>(0.5f, 20.0f, 0.01f, 0.5f), 5.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "depth", "Depth", 0.0f, 1.0f, 0.5f));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "mode", "Mode", juce::StringArray{"Standard", "Optical", "Harmonic"}, 0));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "shape", "Shape", juce::StringArray{"Sine", "Triangle", "Square", "Soft Square"}, 0));

        return layout;
    }
};
```

---

## 11. Optimization and Real-Time Safety

### 11.1 LFO Efficiency

For simple tremolo, the LFO computation is trivial (a single `sin()` per sample). Optimization is rarely needed, but for multi-voice or poly tremolos:

```cpp
// Phasor-based sine approximation (faster than std::sin)
float fastSine(float phase)
{
    // Parabolic sine approximation (accurate to ~0.06%)
    float x = phase * 2.0f - 1.0f; // [-1, 1]
    float y = 4.0f * x * (1.0f - std::abs(x));
    // Refinement for better accuracy
    return y * (0.775f + 0.225f * std::abs(y));
}
```

### 11.2 Parameter Smoothing

Always smooth rate and depth changes to avoid clicks:

```cpp
juce::SmoothedValue<float> smoothedRate;
smoothedRate.reset(sampleRate, 0.05); // 50 ms smoothing
```

### 11.3 Common Pitfalls

| Pitfall | Impact | Solution |
|---------|--------|----------|
| No depth smoothing | Clicks when changing depth | Use SmoothedValue with 20+ ms |
| LFO discontinuity at rate change | Phase jump -> click | Maintain phase, only change increment |
| Ring mod without mix control | Too harsh for most uses | Add dry/wet mix |
| Fixed crossover in harmonic mode | Not suitable for all content | Make crossover frequency adjustable |
| Not accounting for latency from filters | Phase shift in harmonic mode | Use linear-phase crossover for critical use |

---

## Part 5: References

## 12. References and Further Reading

### Academic/Technical
- Smith, J.O. "Physical Audio Signal Processing." Online book, Stanford. (Amplitude modulation)
- Zolzer, U. (2011). *DAFX: Digital Audio Effects*. Chapter 3: Amplitude Modulation.
- Pirkle, W. (2019). *Designing Audio Effect Plugins in C++*. Chapter 8: Modulation Effects.

### Circuit Analysis
- Fender Tremolo Circuits: Bias tremolo (Blackface/Silverface), Opto tremolo (Brownface)
- Vox AC30 Tremolo: Bias modulation of output stage
- Univox Uni-Vibe: Optical phase/tremolo hybrid

### Hardware References
- Fender Blackface Vibrolux/Deluxe Reverb (bias tremolo)
- Fender Brownface Vibroverb (optical tremolo, harmonic tremolo)
- Magnatone amps (pitch-shifting vibrato, distinct from tremolo)
- Demeter Tremulator (optical tremolo pedal)
- EHX Pulsar (multi-waveform tremolo pedal)

### JUCE Resources
- `juce::SmoothedValue` for parameter smoothing
- `juce::dsp::Oscillator` for LFO generation
- `juce::AudioPlayHead` for tempo sync

---

*Research document for O-Tremolo. Covers bias, optical, and harmonic tremolo topologies, ring modulation, auto-pan, and tempo-synced AM with JUCE implementation.*
