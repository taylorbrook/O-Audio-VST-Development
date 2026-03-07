---
title: "Bass Synthesis"
created: 2026-03-07
juce_version: "8.0.4"
summary: "Complete technical reference for bass synthesis in audio plugins, covering subtractive synthesis for bass, sub-harmonic generation, waveshaping, psychoacoustic bass enhancement, FM bass, and analog bass synth emulation with JUCE implementation."
domain: dsp
type: research
keywords:
  - bass-synthesis
  - subtractive
  - sub-harmonic
  - waveshaping
  - psychoacoustic-bass
  - oscillator
  - filter-envelope
  - analog-modeling
  - juce-dsp
stages: [0, 1, 2]
agents: [dsp, research]
---

# Bass Synthesis

**Complete Technical Reference for Bass Synthesis in Audio Plugins**

**Created:** March 2026
**Version:** 1.0
**Research Depth:** Level 3 (Comprehensive Investigation)

---

## Executive Summary

This document covers bass synthesis techniques for audio plugin development. Bass sounds require special attention to waveform generation, filtering, envelope design, and low-frequency optimization. This guide covers subtractive synthesis for bass, sub-harmonic generation, FM bass, waveshaping, psychoacoustic bass enhancement, and analog bass synth emulation.

**Key Findings:**
- Anti-aliased oscillators are critical for bass -- aliasing folds back into the fundamental range and is clearly audible
- Subtractive synthesis with resonant low-pass filtering remains the most versatile bass synthesis approach
- Sub-harmonic generation creates frequency content below the input fundamental
- Psychoacoustic bass uses harmonic series to create the perception of bass on small speakers
- Analog bass synth emulation (Minimoog, 303, SH-101) requires modeling filter nonlinearity
- JUCE's `dsp::Oscillator` and `dsp::LadderFilter` provide good starting points

---

## Table of Contents

### Part 1: Fundamentals
1. [Bass Frequency Range and Perception](#1-bass-frequency-range-and-perception)
2. [Anti-Aliased Oscillators for Bass](#2-anti-aliased-oscillators-for-bass)
3. [Filter Design for Bass](#3-filter-design-for-bass)

### Part 2: Synthesis Techniques
4. [Subtractive Bass Synthesis](#4-subtractive-bass-synthesis)
5. [FM Bass Synthesis](#5-fm-bass-synthesis)
6. [Waveshaping and Distortion](#6-waveshaping-and-distortion)
7. [Sub-Harmonic Generation](#7-sub-harmonic-generation)
8. [Psychoacoustic Bass Enhancement](#8-psychoacoustic-bass-enhancement)

### Part 3: Analog Emulation
9. [Classic Bass Synth Architectures](#9-classic-bass-synth-architectures)
10. [Filter Nonlinearity Modeling](#10-filter-nonlinearity-modeling)

### Part 4: JUCE Implementation
11. [Complete Bass Synthesizer](#11-complete-bass-synthesizer)
12. [Optimization and Real-Time Safety](#12-optimization-and-real-time-safety)

### Part 5: References
13. [References and Further Reading](#13-references-and-further-reading)

---

## Part 1: Fundamentals

## 1. Bass Frequency Range and Perception

### 1.1 Frequency Ranges

| Range | Frequency | Musical Notes | Character |
|-------|-----------|---------------|-----------|
| Sub-bass | 20-60 Hz | C0 (16 Hz) - B1 (62 Hz) | Felt more than heard, rumble |
| Bass | 60-250 Hz | C2 (65 Hz) - B3 (247 Hz) | Fundamental bass tones |
| Low-mid | 250-500 Hz | C4 area | Bass warmth, body |
| Mid | 500-2000 Hz | | Bass presence, attack |

### 1.2 Psychoacoustic Considerations

- Human hearing sensitivity drops below 80 Hz (Fletcher-Munson curves)
- The "missing fundamental" effect: harmonics can imply a fundamental that is not present
- Small speakers cannot reproduce sub-bass; psychoacoustic techniques create the illusion
- Phase relationships in bass are less perceptible than in higher frequencies
- Mono bass below ~100 Hz is standard in most mixing contexts

### 1.3 Bass in the MIDI Range

| MIDI Note | Frequency | Typical Use |
|-----------|-----------|-------------|
| C1 (36) | 65.4 Hz | Standard bass range bottom |
| E1 (40) | 82.4 Hz | Open E string (bass guitar) |
| A1 (45) | 110 Hz | Open A string |
| C2 (48) | 130.8 Hz | Middle bass |
| C3 (60) | 261.6 Hz | Upper bass / low mid |

---

## 2. Anti-Aliased Oscillators for Bass

### 2.1 Why Aliasing Matters for Bass

Aliasing from non-bandlimited oscillators is more problematic for bass because:
- Aliased harmonics fold back near the fundamental
- At low frequencies, the ratio of fundamental to Nyquist is high, creating many harmonics
- Aliased components create dissonant, inharmonic tones that are clearly audible

### 2.2 PolyBLEP Oscillators

PolyBLEP (Polynomial Band-Limited Step) is the standard approach for anti-aliased oscillators:

```cpp
class PolyBLEPOscillator
{
public:
    enum Waveform { Saw, Square, Triangle };

    void prepare(double sampleRate)
    {
        sr = sampleRate;
    }

    void setFrequency(float freq)
    {
        increment = freq / (float)sr;
    }

    float process()
    {
        float output = 0.0f;

        switch (waveform)
        {
            case Saw:
                output = 2.0f * phase - 1.0f;
                output -= polyBLEP(phase, increment);
                break;

            case Square:
                output = phase < pulseWidth ? 1.0f : -1.0f;
                output += polyBLEP(phase, increment);
                output -= polyBLEP(std::fmod(phase + (1.0f - pulseWidth), 1.0f), increment);
                break;

            case Triangle:
            {
                // Integrated square wave
                float sq = phase < 0.5f ? 1.0f : -1.0f;
                sq += polyBLEP(phase, increment);
                sq -= polyBLEP(std::fmod(phase + 0.5f, 1.0f), increment);
                // Leaky integrator
                triState = triState * 0.999f + sq * increment * 4.0f;
                output = triState;
                break;
            }
        }

        phase += increment;
        if (phase >= 1.0f) phase -= 1.0f;

        return output;
    }

    Waveform waveform = Saw;
    float pulseWidth = 0.5f;

private:
    double sr = 44100.0;
    float phase = 0.0f;
    float increment = 0.0f;
    float triState = 0.0f;

    static float polyBLEP(float phase, float increment)
    {
        float dt = increment;
        if (phase < dt)
        {
            float t = phase / dt;
            return t + t - t * t - 1.0f;
        }
        else if (phase > 1.0f - dt)
        {
            float t = (phase - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }
};
```

### 2.3 Wavetable Oscillators for Bass

Wavetable synthesis provides clean bass tones with arbitrary waveshapes:

```cpp
class WavetableOscillator
{
public:
    void prepare(double sampleRate, const float* waveform, int tableSize)
    {
        sr = sampleRate;
        table.assign(waveform, waveform + tableSize);
        this->tableSize = tableSize;
    }

    void setFrequency(float freq)
    {
        increment = freq * (float)tableSize / (float)sr;
    }

    float process()
    {
        // Linear interpolation
        int idx0 = (int)phase % tableSize;
        int idx1 = (idx0 + 1) % tableSize;
        float frac = phase - std::floor(phase);

        float output = table[idx0] * (1.0f - frac) + table[idx1] * frac;

        phase += increment;
        if (phase >= (float)tableSize) phase -= (float)tableSize;

        return output;
    }

private:
    std::vector<float> table;
    int tableSize = 0;
    double sr = 44100.0;
    float phase = 0.0f;
    float increment = 0.0f;
};
```

---

## 3. Filter Design for Bass

### 3.1 Low-Pass Filter for Bass Shaping

The filter is the most critical component for bass character. A resonant low-pass filter shapes the harmonic content:

| Filter Type | Character | Classic Use |
|-------------|-----------|-------------|
| 1-pole (6 dB/oct) | Gentle, warm | Vintage warmth |
| 2-pole (12 dB/oct) | Smooth, round | Roland SH-101 |
| 4-pole (24 dB/oct) | Aggressive, tight | Moog Minimoog |
| SVF (multi-mode) | Versatile | Modern bass synths |

### 3.2 Filter Resonance in Bass

Resonance (Q) at the cutoff frequency creates a peak that adds character:
- Low resonance (0-30%): Warm, full bass
- Medium resonance (30-60%): Honky, nasal character
- High resonance (60-90%): Acid, squelchy (303 territory)
- Self-oscillation (100%): Filter becomes a sine oscillator

### 3.3 Filter Envelope

The filter envelope is crucial for bass articulation:

```cpp
class FilterEnvelope
{
public:
    enum Stage { Idle, Attack, Decay, Sustain, Release };

    void prepare(double sampleRate)
    {
        sr = sampleRate;
    }

    void noteOn()
    {
        stage = Attack;
        envelopeValue = 0.0f;
    }

    void noteOff()
    {
        stage = Release;
    }

    float process()
    {
        switch (stage)
        {
            case Attack:
                envelopeValue += 1.0f / (attackMs * 0.001f * (float)sr);
                if (envelopeValue >= 1.0f)
                {
                    envelopeValue = 1.0f;
                    stage = Decay;
                }
                break;

            case Decay:
                envelopeValue -= (1.0f - sustainLevel)
                                / (decayMs * 0.001f * (float)sr);
                if (envelopeValue <= sustainLevel)
                {
                    envelopeValue = sustainLevel;
                    stage = Sustain;
                }
                break;

            case Sustain:
                envelopeValue = sustainLevel;
                break;

            case Release:
                envelopeValue -= envelopeValue / (releaseMs * 0.001f * (float)sr);
                if (envelopeValue < 0.001f)
                {
                    envelopeValue = 0.0f;
                    stage = Idle;
                }
                break;

            case Idle:
                envelopeValue = 0.0f;
                break;
        }

        return envelopeValue;
    }

    float attackMs = 1.0f;
    float decayMs = 200.0f;
    float sustainLevel = 0.3f;
    float releaseMs = 100.0f;

private:
    double sr = 44100.0;
    Stage stage = Idle;
    float envelopeValue = 0.0f;
};
```

---

## Part 2: Synthesis Techniques

## 4. Subtractive Bass Synthesis

### 4.1 Classic Signal Chain

```
[Oscillator 1] -\
                  +-> [Mixer] -> [Filter] -> [Amp Envelope] -> Output
[Oscillator 2] -/       |
                    [Noise] -> [Filter Envelope controls cutoff]
```

### 4.2 Oscillator Mixing for Bass

Common bass oscillator configurations:

| Config | Character | Example |
|--------|-----------|---------|
| Single saw | Clean, present | Moog bass |
| Two saws (detuned) | Thick, wide | Supersaw bass |
| Saw + square (sub) | Full, deep | DnB bass |
| Square + square (5th) | Power bass | Synth-pop |
| Triangle (sub-osc) | Pure sub-bass | Sub layer |

```cpp
float generateBassOscMix(float freq, float detune, float subLevel)
{
    osc1.setFrequency(freq);
    osc2.setFrequency(freq * (1.0f + detune * 0.01f)); // Slight detune

    float osc1Out = osc1.process(); // Saw wave
    float osc2Out = osc2.process(); // Saw wave (detuned)

    // Sub oscillator: one octave below, square wave
    subOsc.setFrequency(freq * 0.5f);
    float subOut = subOsc.process();

    return (osc1Out + osc2Out) * 0.5f + subOut * subLevel;
}
```

### 4.3 Typical Bass Preset Parameters

| Parameter | Pluck Bass | Pad Bass | Acid Bass | Sub Bass |
|-----------|-----------|---------|----------|---------|
| Osc waveform | Saw | Saw x2 | Saw | Triangle/Sine |
| Filter cutoff | 800 Hz | 200 Hz | 1200 Hz | 120 Hz |
| Filter resonance | 20% | 10% | 70% | 0% |
| Filter env amount | 80% | 10% | 90% | 0% |
| Filter attack | 0 ms | 0 ms | 0 ms | 0 ms |
| Filter decay | 150 ms | 2000 ms | 300 ms | - |
| Filter sustain | 10% | 80% | 5% | - |
| Amp attack | 0 ms | 50 ms | 0 ms | 5 ms |
| Amp decay | 300 ms | - | 200 ms | - |
| Amp sustain | 0% | 80% | 50% | 100% |

---

## 5. FM Bass Synthesis

### 5.1 Two-Operator FM Bass

FM synthesis creates complex harmonics from simple operators:

```cpp
class FMBass
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
    }

    void setFrequency(float freq)
    {
        carrierFreq = freq;
        modulatorFreq = freq * fmRatio; // Ratio determines harmonic content
    }

    float process()
    {
        // Modulator
        float modOutput = std::sin(2.0f * juce::MathConstants<float>::pi * modPhase);
        modOutput *= fmIndex * modulatorFreq; // FM index scales modulation depth

        // Carrier (frequency modulated by modulator)
        float carrierInstFreq = carrierFreq + modOutput;
        float output = std::sin(2.0f * juce::MathConstants<float>::pi * carrierPhase);

        // Advance phases
        carrierPhase += carrierInstFreq / (float)sr;
        modPhase += modulatorFreq / (float)sr;

        if (carrierPhase >= 1.0f) carrierPhase -= 1.0f;
        if (modPhase >= 1.0f) modPhase -= 1.0f;

        return output;
    }

    float fmRatio = 1.0f;   // Carrier:modulator ratio
    float fmIndex = 2.0f;   // Modulation depth

private:
    double sr = 44100.0;
    float carrierFreq = 0.0f;
    float modulatorFreq = 0.0f;
    float carrierPhase = 0.0f;
    float modPhase = 0.0f;
};
```

### 5.2 FM Bass Ratio Table

| Ratio (C:M) | Harmonic Content | Character |
|-------------|------------------|-----------|
| 1:1 | All harmonics | Electric bass, growl |
| 1:2 | Even harmonics emphasized | Round, warm |
| 1:3 | Every 3rd harmonic | Hollow, organ-like |
| 2:1 | Sub-harmonics + harmonics | Deep, complex |
| 1:1.5 | Inharmonic partials | Metallic, bell-like |

### 5.3 FM Index Envelope

The key to dynamic FM bass is enveloping the FM index:

```cpp
// FM index controlled by filter envelope
float envValue = filterEnvelope.process();
fmBass.fmIndex = envValue * fmDepth; // Index decays with envelope
```

High index at attack creates a bright, percussive transient that decays to a warm fundamental.

---

## 6. Waveshaping and Distortion

### 6.1 Bass Distortion Types

| Type | Transfer Function | Character |
|------|-------------------|-----------|
| Soft clip | `tanh(x * gain)` | Warm, tube-like |
| Hard clip | `clamp(x, -1, 1)` | Aggressive, digital |
| Asymmetric | Different +/- curves | Even harmonics, tube character |
| Foldback | `sin(x * gain)` | Complex, rich harmonics |
| Bit crush | Quantize amplitude | Lo-fi, gritty |

### 6.2 Waveshaping for Harmonic Enrichment

```cpp
class BassDistortion
{
public:
    float process(float input, float drive)
    {
        // Pre-gain
        float gained = input * drive;

        // Asymmetric soft clipping (tube character)
        float output;
        if (gained >= 0.0f)
            output = std::tanh(gained);
        else
            output = std::tanh(gained * 0.8f) / 0.8f;

        // High-pass to remove DC offset from asymmetric clipping
        float filtered = output - dcBlockState;
        dcBlockState = output - filtered * 0.999f;

        return filtered;
    }

private:
    float dcBlockState = 0.0f;
};
```

### 6.3 Multi-Band Distortion for Bass

Apply distortion only to the mid/high content while keeping the sub-bass clean:

```cpp
void multiBandBassDistortion(float input, float& output, float drive)
{
    float low = lowpassFilter.process(input);   // Below 120 Hz
    float high = input - low;                    // Above 120 Hz

    // Distort only the upper content
    float distortedHigh = std::tanh(high * drive);

    // Recombine: clean sub + distorted harmonics
    output = low + distortedHigh;
}
```

---

## 7. Sub-Harmonic Generation

### 7.1 Octave Divider

Generate a tone one octave below the input by dividing the frequency:

```cpp
class SubHarmonicGenerator
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        envelope.prepare(sampleRate);
    }

    float process(float input)
    {
        // Zero-crossing detection for frequency tracking
        if (prevSample < 0.0f && input >= 0.0f)
        {
            // Positive zero crossing -- one period complete
            float period = (float)samplesSinceLastCrossing;
            subFrequency = (float)sr / (period * 2.0f); // Half frequency = octave down
            samplesSinceLastCrossing = 0;
        }
        samplesSinceLastCrossing++;
        prevSample = input;

        // Generate sub-harmonic sine
        float subSample = std::sin(2.0f * juce::MathConstants<float>::pi * subPhase);
        subPhase += subFrequency / (float)sr;
        if (subPhase >= 1.0f) subPhase -= 1.0f;

        // Follow input envelope for natural dynamics
        float env = envelope.process(std::abs(input));

        return subSample * env * subLevel;
    }

    float subLevel = 0.5f;

private:
    double sr = 44100.0;
    float prevSample = 0.0f;
    float subFrequency = 0.0f;
    float subPhase = 0.0f;
    int samplesSinceLastCrossing = 0;
    EnvelopeFollower envelope;
};
```

### 7.2 Harmonic Synthesis Sub-Bass

Rather than pitch detection, synthesize sub-harmonics from the harmonic series:

```cpp
float harmonicSubBass(float input, float fundamentalFreq, double sampleRate)
{
    // Generate sine at the fundamental
    static float phase = 0.0f;
    float subSine = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
    phase += fundamentalFreq / (float)sampleRate;
    if (phase >= 1.0f) phase -= 1.0f;

    // Follow input level
    float level = std::abs(input);

    return subSine * level;
}
```

---

## 8. Psychoacoustic Bass Enhancement

### 8.1 The Missing Fundamental Effect

The human auditory system perceives the fundamental frequency even when only harmonics are present. This is used to create bass perception on small speakers:

```cpp
class PsychoacousticBass
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        // Isolate bass content (below 100 Hz)
        bassFilter.setCoefficients(
            juce::IIRCoefficients::makeLowPass(sampleRate, 100.0));
        // Output filter for generated harmonics
        harmonicFilter.setCoefficients(
            juce::IIRCoefficients::makeBandPass(sampleRate, 200.0, 1.0));
    }

    float process(float input)
    {
        // Extract bass content
        float bass = bassFilter.processSingleSampleRaw(input);

        // Generate harmonics of the bass content
        float harmonic2 = bass * bass;               // 2nd harmonic (rectification)
        float harmonic3 = bass * bass * bass;         // 3rd harmonic (cubing)

        // Filter harmonics to useful range (100-400 Hz)
        float harmonics = harmonicFilter.processSingleSampleRaw(
            harmonic2 * 0.7f + harmonic3 * 0.3f);

        // Mix: original + generated harmonics
        return input + harmonics * enhanceAmount;
    }

    float enhanceAmount = 0.3f;

private:
    juce::IIRFilter bassFilter;
    juce::IIRFilter harmonicFilter;
    double sr = 44100.0;
};
```

### 8.2 MaxxBass-Style Processing

The Waves MaxxBass algorithm:
1. Split the signal at the crossover frequency (~100 Hz)
2. Apply nonlinear processing to generate harmonics of the bass content
3. Filter the harmonics to remove the original fundamental
4. Mix the harmonics with the original signal
5. Optionally reduce the original sub-bass (for small speaker translation)

---

## Part 3: Analog Emulation

## 9. Classic Bass Synth Architectures

### 9.1 Minimoog Bass

The Moog Minimoog is the definitive bass synthesizer:
- 3 oscillators (saw, triangle, square, pulse)
- Moog 4-pole ladder filter (24 dB/oct)
- Filter resonance with self-oscillation
- ADS envelopes (no release in the original)

Key to the Moog bass sound: the ladder filter's nonlinear response at high resonance creates a characteristic "squelch."

### 9.2 Roland TB-303

The TB-303 acid bass sound comes from:
- Single saw or square oscillator
- 4-pole diode ladder filter (18 dB/oct effective slope)
- High resonance with unique resonance peak shape
- Slide (portamento) between notes
- Accent: increases filter envelope depth and amplitude

```cpp
class TB303Bass
{
public:
    void noteOn(float freq, bool accent, bool slide)
    {
        if (slide)
        {
            // Portamento to new frequency
            targetFreq = freq;
            slideRate = 0.003f; // ~60 ms slide
        }
        else
        {
            currentFreq = freq;
            targetFreq = freq;
            filterEnv.noteOn();
            ampEnv.noteOn();
        }

        isAccent = accent;
    }

    float process()
    {
        // Slide
        currentFreq += (targetFreq - currentFreq) * slideRate;

        // Oscillator
        osc.setFrequency(currentFreq);
        float signal = osc.process();

        // Filter envelope
        float envAmount = isAccent ? accentEnvAmount : normalEnvAmount;
        float filterCutoff = baseCutoff + filterEnv.process() * envAmount;
        filter.setCutoffFrequency(filterCutoff);
        filter.setResonance(resonance);

        float filtered = filter.process(signal);

        // Amp envelope (accent = louder)
        float ampAmount = isAccent ? 1.0f : 0.7f;
        return filtered * ampEnv.process() * ampAmount;
    }

private:
    PolyBLEPOscillator osc;
    juce::dsp::LadderFilter<float> filter;
    FilterEnvelope filterEnv;
    FilterEnvelope ampEnv;
    float currentFreq = 0.0f;
    float targetFreq = 0.0f;
    float slideRate = 0.003f;
    float baseCutoff = 300.0f;
    float resonance = 0.7f;
    float normalEnvAmount = 2000.0f;
    float accentEnvAmount = 5000.0f;
    bool isAccent = false;
};
```

### 9.3 Roland SH-101

The SH-101 bass is characterized by:
- Single oscillator (saw/square/pulse + sub-oscillator)
- 2-pole (12 dB/oct) low-pass filter
- The 12 dB/oct filter gives a warmer, less aggressive sound than the Moog 24 dB/oct
- Sub-oscillator adds weight without muddiness

---

## 10. Filter Nonlinearity Modeling

### 10.1 Moog Ladder Filter

The Moog ladder filter's nonlinearity is central to its sound. JUCE provides a ladder filter:

```cpp
juce::dsp::LadderFilter<float> moogFilter;

void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = 1;

    moogFilter.prepare(spec);
    moogFilter.setMode(juce::dsp::LadderFilterMode::LPF24);
    moogFilter.setCutoffFrequencyHz(800.0f);
    moogFilter.setResonance(0.7f);
    moogFilter.setDrive(1.5f); // Nonlinear drive
}
```

### 10.2 Diode Ladder (303-style)

The 303's diode ladder has a different character from the Moog transistor ladder:
- Effective slope closer to 18 dB/oct (not a true 24 dB/oct)
- Resonance peak has a different shape (broader, less sharp)
- Self-oscillation behavior is different

### 10.3 Drive and Saturation in the Filter

```cpp
// Simple nonlinear filter model
float nonlinearFilter(float input, float cutoff, float resonance, float drive)
{
    // Apply saturation within the filter feedback loop
    float fb = resonance * std::tanh(prevOutput * drive);
    float inputSaturated = std::tanh((input - fb) * drive) / drive;

    // Apply filter
    float output = prevOutput + cutoff * (inputSaturated - prevOutput);
    prevOutput = output;

    return output;
}
```

---

## Part 4: JUCE Implementation

## 11. Complete Bass Synthesizer

### 11.1 Plugin Architecture

```cpp
class BassSynthProcessor : public juce::AudioProcessor
{
public:
    BassSynthProcessor()
        : AudioProcessor(BusesProperties()
            .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        sr = sampleRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
        spec.numChannels = 1;

        ladderFilter.prepare(spec);
        ladderFilter.setMode(juce::dsp::LadderFilterMode::LPF24);

        osc1.prepare(sampleRate);
        osc2.prepare(sampleRate);
        subOsc.prepare(sampleRate);

        filterEnv.prepare(sampleRate);
        ampEnv.prepare(sampleRate);

        glideSmooth.reset(sampleRate, 0.05); // 50 ms glide
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        juce::ScopedNoDenormals noDenormals;
        buffer.clear();

        // Handle MIDI
        for (const auto metadata : midi)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                float freq = (float)juce::MidiMessage::getMidiNoteInHertz(msg.getNoteNumber());
                glideSmooth.setTargetValue(freq);
                filterEnv.noteOn();
                ampEnv.noteOn();
                noteActive = true;
            }
            else if (msg.isNoteOff())
            {
                filterEnv.noteOff();
                ampEnv.noteOff();
            }
        }

        if (!noteActive) return;

        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getWritePointer(1);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float freq = glideSmooth.getNextValue();

            // Oscillators
            osc1.setFrequency(freq);
            osc2.setFrequency(freq * (1.0f + detune * 0.01f));
            subOsc.setFrequency(freq * 0.5f);

            float signal = osc1.process() * 0.4f
                         + osc2.process() * 0.3f
                         + subOsc.process() * subLevel;

            // Filter with envelope
            float envValue = filterEnv.process();
            float cutoff = filterCutoff + envValue * filterEnvAmount;
            cutoff = juce::jlimit(20.0f, 18000.0f, cutoff);
            ladderFilter.setCutoffFrequencyHz(cutoff);
            ladderFilter.setResonance(filterResonance);
            ladderFilter.setDrive(filterDrive);

            float filtered = ladderFilter.processSample(signal, 0);

            // Amp envelope
            float ampValue = ampEnv.process();
            float output = filtered * ampValue * outputGain;

            if (ampValue < 0.001f && ampEnv.stage == FilterEnvelope::Idle)
                noteActive = false;

            outL[i] = output;
            outR[i] = output;
        }
    }

private:
    double sr = 44100.0;
    PolyBLEPOscillator osc1, osc2, subOsc;
    juce::dsp::LadderFilter<float> ladderFilter;
    FilterEnvelope filterEnv, ampEnv;
    juce::SmoothedValue<float> glideSmooth;

    bool noteActive = false;
    float detune = 5.0f;        // cents
    float subLevel = 0.3f;
    float filterCutoff = 400.0f;
    float filterResonance = 0.5f;
    float filterDrive = 1.2f;
    float filterEnvAmount = 3000.0f;
    float outputGain = 0.5f;
};
```

---

## 12. Optimization and Real-Time Safety

### 12.1 Mono Processing

Bass is almost always monophonic. Use a single voice (mono synth) rather than polyphony:

```cpp
// Mono: single voice, last-note priority
void handleNoteOn(int noteNumber, float velocity)
{
    activeNote = noteNumber;
    float freq = juce::MidiMessage::getMidiNoteInHertz(noteNumber);
    glideSmooth.setTargetValue(freq);
}
```

### 12.2 Oversampling for Filter Nonlinearity

Nonlinear filters (ladder, diode) produce harmonics that can alias. Oversample the filter section:

```cpp
juce::dsp::Oversampling<float> oversampler{1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR};

void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    oversampler.initProcessing((size_t)samplesPerBlock);
}

void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
{
    auto block = juce::dsp::AudioBlock<float>(buffer);
    auto oversampledBlock = oversampler.processSamplesUp(block);
    // Process at 2x sample rate...
    oversampler.processSamplesDown(block);
}
```

### 12.3 Common Pitfalls

| Pitfall | Impact | Solution |
|---------|--------|----------|
| Aliased oscillators | Harsh, inharmonic overtones | Use PolyBLEP or wavetable |
| No DC blocking after distortion | Speaker damage, woofer excursion | High-pass at 5-10 Hz |
| Filter cutoff above Nyquist | Instability | Clamp to 0.45 * sampleRate |
| Polyphonic bass | Mud, phase cancellation | Use monophonic voice allocation |
| No glide smoothing | Clicks on note changes | Smooth frequency transitions |

---

## Part 5: References

## 13. References and Further Reading

### Academic/Technical
- Stilson, T. & Smith, J.O. (1996). "Analyzing the Moog VCF with Considerations for Digital Implementation." Computer Music Conference.
- Valimaki, V. & Smith, J.O. (2010). "Alias-Free Virtual Analog Oscillators Using Polynomial BLEPs." IEEE Trans. Audio, Speech, Language.
- Zavalishin, V. (2012). "The Art of VA Filter Design." Native Instruments.

### Books
- Pirkle, W. (2019). *Designing Audio Effect Plugins in C++*. Chapters 6-7: Oscillators and Filters.
- Russ, M. (2012). *Sound Synthesis and Sampling*. Chapter 4: Subtractive Synthesis.
- Smith, J.O. "Physical Audio Signal Processing." Online book, Stanford.

### Hardware References
- Moog Minimoog Model D (1970) -- the definitive bass synthesizer
- Roland TB-303 (1982) -- acid bass sound
- Roland SH-101 (1982) -- versatile mono synth bass
- Moog Sub 37/Subsequent 37 -- modern Moog bass
- Novation Bass Station II -- modern analog bass

### JUCE Resources
- `juce::dsp::LadderFilter` for Moog-style ladder filter
- `juce::dsp::Oscillator` for basic waveform generation
- `juce::dsp::Oversampling` for nonlinear filter processing

---

*Research document for O-Bass. Covers subtractive synthesis, FM bass, waveshaping, sub-harmonic generation, psychoacoustic bass, and analog bass synth emulation with JUCE implementation.*
