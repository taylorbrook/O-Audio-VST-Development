---
title: "Mallet Percussion Physical Modeling"
created: 2026-03-07
juce_version: "8.0.4"
summary: "Complete technical reference for mallet percussion physical modeling in audio plugins, covering bar vibration modes, marimba and xylophone modal characteristics, resonator tube modeling, mallet-bar interaction, and JUCE implementation for real-time synthesis."
domain: dsp
type: research
keywords:
  - physical-modeling
  - mallet-percussion
  - marimba
  - xylophone
  - modal-synthesis
  - bar-vibration
  - resonator
  - impulse-response
  - juce-dsp
stages: [0, 1, 2]
agents: [dsp, research]
---

# Mallet Percussion Physical Modeling

**Complete Technical Reference for Mallet Percussion Physical Modeling in Audio Plugins**

**Created:** March 2026
**Version:** 1.0
**Research Depth:** Level 3 (Comprehensive Investigation)

---

## Executive Summary

This document covers physical modeling of mallet percussion instruments (marimba, xylophone, vibraphone, glockenspiel) for audio plugin development. While related to bell modal synthesis, mallet percussion involves fundamentally different physics: bar vibration rather than shell vibration, resonator tube coupling, and mallet-bar interaction dynamics.

**Key Findings:**
- Bar vibration modes follow inharmonic frequency ratios unlike bells or strings
- Marimba bars are specifically shaped (arched underside) to tune the first few modes to harmonic ratios
- Resonator tubes (closed-end pipes) amplify specific partials and add the characteristic sustain
- Mallet hardness dramatically affects the spectral content of the excitation
- Modal synthesis with 8-20 modes produces convincing mallet percussion in real time
- The key difference from bells: bars have shorter sustain, different modal ratios, and directional radiation

---

## Table of Contents

### Part 1: Acoustic Foundations
1. [Bar Vibration Physics](#1-bar-vibration-physics)
2. [Modal Frequencies of Percussion Bars](#2-modal-frequencies-of-percussion-bars)
3. [Instrument-Specific Characteristics](#3-instrument-specific-characteristics)

### Part 2: Physical Model Components
4. [Excitation: Mallet-Bar Interaction](#4-excitation-mallet-bar-interaction)
5. [Resonator: Bar Vibration Modes](#5-resonator-bar-vibration-modes)
6. [Resonator Tubes](#6-resonator-tubes)
7. [Radiation and Coupling](#7-radiation-and-coupling)

### Part 3: Synthesis Implementation
8. [Modal Synthesis for Bars](#8-modal-synthesis-for-bars)
9. [Excitation Signal Design](#9-excitation-signal-design)
10. [Resonator Tube Simulation](#10-resonator-tube-simulation)

### Part 4: JUCE Implementation
11. [Complete Mallet Percussion Synthesizer](#11-complete-mallet-percussion-synthesizer)
12. [Optimization and Real-Time Safety](#12-optimization-and-real-time-safety)

### Part 5: References
13. [References and Further Reading](#13-references-and-further-reading)

---

## Part 1: Acoustic Foundations

## 1. Bar Vibration Physics

### 1.1 Euler-Bernoulli Beam Theory

Mallet percussion bars vibrate as free-free beams (unsupported at both ends). The vibration frequencies of a uniform rectangular bar are:

```
fn = (pi / 2L^2) * sqrt(E*I / (rho*A)) * (kn * L)^2
```

Where:
- L = bar length
- E = Young's modulus (material stiffness)
- I = moment of inertia of cross-section
- rho = material density
- A = cross-sectional area
- kn = mode-dependent wavenumber

### 1.2 Mode Shape Constants (kn*L)

For a free-free beam, the first several mode shape constants are:

| Mode | kn*L | Frequency Ratio (to mode 1) |
|------|------|---------------------------|
| 1 | 4.730 | 1.000 |
| 2 | 7.853 | 2.756 |
| 3 | 10.996 | 5.404 |
| 4 | 14.137 | 8.933 |
| 5 | 17.279 | 13.344 |

### 1.3 Key Difference from Strings

Strings have harmonic overtones (2f, 3f, 4f...). Bars have **inharmonic** overtones following the ratio (kn/k1)^2. This is why untreated bars sound "clangy" rather than "tonal."

### 1.4 Material Properties

| Material | Young's Modulus (GPa) | Density (kg/m^3) | Character |
|----------|-----------------------|-------------------|-----------|
| Rosewood | 12-16 | 850-1100 | Warm, dark (marimba) |
| Padauk | 10-14 | 700-900 | Rich, warm (marimba) |
| Fiberglass | 40-50 | 1800-2200 | Bright (synthetic marimba) |
| Steel | 200 | 7800 | Very bright (glockenspiel) |
| Aluminum | 69 | 2700 | Bright, sustained (vibraphone) |
| Brass | 100-125 | 8500 | Warm, sustained |

---

## 2. Modal Frequencies of Percussion Bars

### 2.1 Tuned Bar Ratios

Instrument makers arch the underside of bars to tune the first few modes to harmonic relationships:

**Marimba (tuned rosewood bar):**

| Mode | Ratio to F0 | Tuning Target | Tolerance |
|------|-------------|---------------|-----------|
| 1 | 1.000 | Fundamental | Exact |
| 2 | 4.000 | 2 octaves up | +/- 5 cents |
| 3 | ~9.0-10.0 | ~3.17 octaves | Approximate |
| 4 | ~16+ | Inharmonic | Not tuned |

The 4:1 ratio of mode 2 to mode 1 is the signature of a well-tuned marimba bar. Some concert marimbas also tune mode 3 to 10:1 (approximately 3 octaves + major third).

**Xylophone (tuned synthetic or rosewood bar):**

| Mode | Ratio to F0 | Tuning Target |
|------|-------------|---------------|
| 1 | 1.000 | Fundamental |
| 2 | 3.000 | Octave + fifth (12th) |
| 3 | ~6.0 | ~2.5 octaves |

The xylophone's 3:1 ratio gives it a brighter, more piercing character than the marimba's 4:1.

### 2.2 Vibraphone

| Mode | Ratio | Notes |
|------|-------|-------|
| 1 | 1.000 | Fundamental |
| 2 | 4.000 | 2 octaves (tuned) |
| 3 | ~9.5 | Inharmonic |

Vibraphone bars are aluminum and have sustained resonance due to pedal damping and motor-driven vibrato (rotating discs in the resonator tubes).

### 2.3 Glockenspiel

| Mode | Ratio | Notes |
|------|-------|-------|
| 1 | 1.000 | Fundamental |
| 2 | 2.756 | ~17.4 semitones (untuned) |
| 3 | 5.404 | Inharmonic |

Glockenspiel bars are not undercut, so modes retain their natural inharmonic ratios, producing a "bell-like" quality.

---

## 3. Instrument-Specific Characteristics

### 3.1 Comparison Table

| Property | Marimba | Xylophone | Vibraphone | Glockenspiel |
|----------|---------|-----------|------------|-------------|
| Material | Rosewood/Padauk | Rosewood/Fiberglass | Aluminum | Steel |
| Range | A2-C7 | F4-C8 | F3-F6 | G5-C8 |
| Resonators | Yes (wood/metal) | Yes (metal) | Yes + rotating disc | No |
| Sustain | 1-3 sec | 0.3-1 sec | 2-5 sec (pedal) | 5-10 sec |
| Mode 2 ratio | 4:1 | 3:1 | 4:1 | 2.76:1 |
| Character | Warm, round | Bright, sharp | Warm, vibrato | Bright, bell-like |
| Mallet | Yarn-wrapped | Hard rubber/plastic | Cord/yarn | Brass/plastic |

### 3.2 Decay Characteristics

Each mode decays at its own rate (higher modes decay faster):

```cpp
float modeDecayRate(int modeNumber, float materialDamping, float airDamping)
{
    // Higher modes lose energy faster due to:
    // 1. Internal material damping (proportional to frequency^2)
    // 2. Air radiation damping (proportional to frequency^2 for bars)
    float freqRatio = modeFrequencyRatio(modeNumber);
    return materialDamping * freqRatio * freqRatio + airDamping * freqRatio;
}
```

### 3.3 Nodal Points

The striking position on the bar relative to the nodal points determines which modes are excited:

- **Center strike:** Excites modes 1, 3, 5 (odd transverse modes)
- **Edge strike:** Excites modes 1, 2, 3, 4 (all modes)
- **Node strike:** Suppresses modes with nodes at that position

---

## Part 2: Physical Model Components

## 4. Excitation: Mallet-Bar Interaction

### 4.1 Mallet Types and Spectral Content

The mallet determines the excitation spectrum:

| Mallet Type | Spectral Tilt | Character | Contact Duration |
|-------------|---------------|-----------|-----------------|
| Soft yarn | -12 to -18 dB/oct | Dark, warm | 5-10 ms |
| Medium yarn | -6 to -12 dB/oct | Balanced | 3-7 ms |
| Hard rubber | -3 to -6 dB/oct | Bright, defined | 2-4 ms |
| Plastic | 0 to -3 dB/oct | Very bright, click | 1-3 ms |
| Brass | Flat spectrum | Cutting, metallic | 0.5-2 ms |

### 4.2 Mallet Interaction Model

The mallet-bar contact follows a Hertzian force law:

```cpp
class MalletExcitation
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
    }

    float generate(float malletHardness, float velocity)
    {
        if (!active) return 0.0f;

        // Contact force profile (half-sine approximation)
        float contactDuration = (1.0f - malletHardness * 0.8f) * 0.008f; // 2-10 ms
        float contactSamples = contactDuration * (float)sr;

        float output = 0.0f;
        if (sampleCount < contactSamples)
        {
            float t = (float)sampleCount / contactSamples;
            // Half-sine contact force
            output = velocity * std::sin(juce::MathConstants<float>::pi * t);

            // Harder mallets have sharper attack and more HF content
            if (malletHardness > 0.5f)
            {
                float sharpness = (malletHardness - 0.5f) * 2.0f;
                output = std::pow(output, 1.0f - sharpness * 0.5f);
            }
        }
        else
        {
            active = false;
        }

        sampleCount++;
        return output;
    }

    void trigger(float velocity)
    {
        active = true;
        sampleCount = 0;
    }

private:
    double sr = 44100.0;
    bool active = false;
    int sampleCount = 0;
};
```

### 4.3 Noise Component

Real mallet strikes include broadband noise from the impact:

```cpp
float malletNoise(float malletHardness, float velocity)
{
    float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
    // Harder mallets produce more high-frequency noise
    float noiseLevel = velocity * malletHardness * 0.1f;
    return noise * noiseLevel;
}
```

---

## 5. Resonator: Bar Vibration Modes

### 5.1 Modal Resonator Bank

Each mode is a damped sinusoidal oscillator:

```cpp
class ModalResonator
{
public:
    struct Mode
    {
        float frequency;    // Hz
        float amplitude;    // Relative amplitude
        float decayRate;    // Decay rate (1/seconds)
        float phase;        // Current phase
        float envelope;     // Current envelope value
    };

    void prepare(double sampleRate, int numModes)
    {
        sr = sampleRate;
        modes.resize(numModes);
    }

    void setMode(int index, float freq, float amp, float decayTime)
    {
        if (index >= (int)modes.size()) return;
        modes[index].frequency = freq;
        modes[index].amplitude = amp;
        modes[index].decayRate = 1.0f / decayTime; // seconds
    }

    void trigger()
    {
        for (auto& mode : modes)
        {
            mode.phase = 0.0f;
            mode.envelope = mode.amplitude;
        }
    }

    float process(float excitation)
    {
        float output = 0.0f;

        for (auto& mode : modes)
        {
            if (mode.envelope < 0.0001f) continue;

            // Damped sinusoidal oscillation
            float sample = mode.envelope
                         * std::sin(2.0f * juce::MathConstants<float>::pi * mode.phase);

            // Add excitation energy to this mode
            sample += excitation * mode.amplitude;

            output += sample;

            // Advance phase
            mode.phase += mode.frequency / (float)sr;
            if (mode.phase >= 1.0f) mode.phase -= 1.0f;

            // Apply decay
            mode.envelope *= std::exp(-mode.decayRate / (float)sr);
        }

        return output;
    }

private:
    std::vector<Mode> modes;
    double sr = 44100.0;
};
```

### 5.2 Efficient Modal Implementation Using Biquads

For better efficiency, implement each mode as a resonant biquad filter excited by the mallet impulse:

```cpp
class BiquadMode
{
public:
    void setMode(float frequency, float decayTime, float amplitude, double sampleRate)
    {
        float w0 = 2.0f * juce::MathConstants<float>::pi * frequency / (float)sampleRate;
        float r = std::exp(-1.0f / (decayTime * (float)sampleRate));

        // Resonant bandpass driven by impulse
        b0 = amplitude * (1.0f - r * r) * 0.5f;
        a1 = -2.0f * r * std::cos(w0);
        a2 = r * r;
    }

    float process(float excitation)
    {
        float output = b0 * excitation - a1 * y1 - a2 * y2;
        y2 = y1;
        y1 = output;
        return output;
    }

    void reset()
    {
        y1 = 0.0f;
        y2 = 0.0f;
    }

private:
    float b0 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;
};
```

---

## 6. Resonator Tubes

### 6.1 Closed-End Pipe Resonance

Marimba and vibraphone resonator tubes are closed at the bottom and open at the top. They resonate at odd harmonics:

```
f_n = (2n - 1) * c / (4 * L)    where n = 1, 2, 3, ...
```

The fundamental resonance of the tube is tuned to match the bar's fundamental frequency.

### 6.2 Tube Resonance Model

```cpp
class ResonatorTube
{
public:
    void prepare(double sampleRate, float tubeLength, float tubeRadius)
    {
        sr = sampleRate;

        // Fundamental frequency of closed-end pipe
        float speedOfSound = 343.0f; // m/s at 20C
        float endCorrection = 0.6133f * tubeRadius; // End correction
        float effectiveLength = tubeLength + endCorrection;

        fundamentalFreq = speedOfSound / (4.0f * effectiveLength);

        // Set up resonant filter at tube fundamental
        float Q = 20.0f; // Tube resonance Q (narrow peak)
        tubeFilter.setCoefficients(
            juce::IIRCoefficients::makeBandPass(sampleRate, fundamentalFreq, Q));

        // Tube adds warmth: low-pass characteristic
        dampingFilter.setCoefficients(
            juce::IIRCoefficients::makeLowPass(sampleRate, fundamentalFreq * 3.0f));
    }

    float process(float input)
    {
        float resonated = tubeFilter.processSingleSampleRaw(input);
        float damped = dampingFilter.processSingleSampleRaw(resonated);

        return input + damped * tubeGain;
    }

    float tubeGain = 0.5f;

private:
    juce::IIRFilter tubeFilter;
    juce::IIRFilter dampingFilter;
    float fundamentalFreq = 0.0f;
    double sr = 44100.0;
};
```

### 6.3 Vibraphone Motor Effect

The vibraphone's rotating discs periodically open and close the resonator tubes, creating amplitude modulation (tremolo/vibrato):

```cpp
class VibraphoneMotor
{
public:
    void prepare(double sampleRate)
    {
        lfo.prepare(sampleRate);
        lfo.setFrequency(motorSpeed); // Typically 1-10 Hz
    }

    float process(float input)
    {
        if (!motorEnabled) return input;

        float lfoValue = lfo.process();
        // When disc closes tube, resonance is reduced
        float resonanceAmount = 0.5f + 0.5f * lfoValue; // 0 to 1
        return input * (0.5f + 0.5f * resonanceAmount);
    }

    float motorSpeed = 5.0f; // Hz
    bool motorEnabled = true;

private:
    LFO lfo;
};
```

---

## 7. Radiation and Coupling

### 7.1 Directional Radiation

Bar percussion instruments radiate differently from their top and bottom surfaces:
- The bar radiates as a dipole (sound from top and bottom surfaces)
- The resonator tube radiates as a monopole (sound from the open end)
- The combined radiation pattern is complex and position-dependent

### 7.2 Bar-Tube Coupling

The bar and resonator tube exchange energy:
- The bar drives the tube through air coupling
- The tube slightly modifies the bar's decay characteristics
- Strong coupling at the fundamental, weak at higher modes

```cpp
float barTubeCoupling(float barOutput, float tubeResonance, float couplingFactor)
{
    // Bi-directional coupling
    float tubeInput = barOutput * couplingFactor;
    float feedback = tubeResonance * couplingFactor * 0.1f; // Back to bar
    return barOutput + feedback;
}
```

---

## Part 3: Synthesis Implementation

## 8. Modal Synthesis for Bars

### 8.1 Marimba Modal Preset

```cpp
struct InstrumentPreset
{
    struct ModeData
    {
        float freqRatio;   // Ratio to fundamental
        float amplitude;   // Relative amplitude (0-1)
        float decayTime;   // Seconds
    };

    int numModes;
    ModeData modes[16];
};

const InstrumentPreset marimbaPreset = {
    10, // 10 modes
    {
        {1.000f, 1.00f, 2.0f},   // Mode 1: fundamental
        {4.000f, 0.50f, 0.8f},   // Mode 2: 2 octaves (tuned)
        {9.500f, 0.15f, 0.4f},   // Mode 3: ~3 octaves
        {16.00f, 0.05f, 0.2f},   // Mode 4: inharmonic
        {25.00f, 0.02f, 0.15f},  // Mode 5: inharmonic
        {2.200f, 0.10f, 0.5f},   // Torsional mode
        {6.000f, 0.08f, 0.3f},   // Lateral mode
        {1.003f, 0.20f, 1.8f},   // Detuned fundamental (coupling)
        {4.010f, 0.12f, 0.7f},   // Detuned mode 2 (coupling)
        {12.00f, 0.03f, 0.25f},  // Higher inharmonic
    }
};

const InstrumentPreset xylophonePreset = {
    8,
    {
        {1.000f, 1.00f, 0.5f},   // Shorter decay than marimba
        {3.000f, 0.60f, 0.3f},   // 3:1 ratio (brighter)
        {6.000f, 0.20f, 0.15f},
        {10.00f, 0.08f, 0.10f},
        {15.00f, 0.03f, 0.08f},
        {1.800f, 0.12f, 0.3f},   // Torsional
        {4.500f, 0.06f, 0.2f},
        {8.000f, 0.04f, 0.12f},
    }
};

const InstrumentPreset vibraphonePreset = {
    8,
    {
        {1.000f, 1.00f, 4.0f},   // Long sustain (aluminum)
        {4.000f, 0.40f, 2.5f},   // Tuned
        {9.200f, 0.12f, 1.5f},
        {16.50f, 0.05f, 0.8f},
        {25.50f, 0.02f, 0.4f},
        {1.002f, 0.15f, 3.5f},   // Beat frequency pair
        {6.500f, 0.06f, 1.0f},
        {11.00f, 0.03f, 0.6f},
    }
};

const InstrumentPreset glockenspielPreset = {
    8,
    {
        {1.000f, 1.00f, 6.0f},   // Very long sustain (steel)
        {2.756f, 0.70f, 4.0f},   // Natural ratio (not tuned)
        {5.404f, 0.30f, 2.5f},   // Natural ratio
        {8.933f, 0.12f, 1.5f},
        {13.34f, 0.05f, 0.8f},
        {3.500f, 0.15f, 3.0f},   // Torsional mode
        {7.000f, 0.08f, 2.0f},
        {10.50f, 0.04f, 1.2f},
    }
};
```

### 8.2 Strike Position Effect

The strike position determines which modes are excited (based on the mode shape at that point):

```cpp
float strikePositionAmplitude(int modeNumber, float strikePosition)
{
    // strikePosition: 0.0 = center, 1.0 = edge
    // Mode shapes for free-free beam (simplified)
    // Even modes have antinodes at center, odd modes have nodes

    // Approximate mode shape at strike position
    float x = 0.5f + strikePosition * 0.4f; // Position along bar (0.1 to 0.9)
    float modeShape = std::cos((float)modeNumber * juce::MathConstants<float>::pi * x);

    return std::abs(modeShape);
}
```

---

## 9. Excitation Signal Design

### 9.1 Combined Excitation

```cpp
class MalletStrike
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
    }

    float process()
    {
        if (!active) return 0.0f;

        float contactDuration = (1.0f - hardness * 0.8f) * 0.008f * (float)sr;
        float t = (float)sampleCount / contactDuration;

        float output = 0.0f;
        if (t < 1.0f)
        {
            // Contact force
            float force = velocity * std::sin(juce::MathConstants<float>::pi * t);

            // Shape based on hardness
            force = std::pow(force, 1.0f + hardness);

            // Add impact noise for harder mallets
            float noise = 0.0f;
            if (hardness > 0.3f)
            {
                noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f)
                       * velocity * (hardness - 0.3f) * 0.5f;
                // Noise decays faster than force
                noise *= (1.0f - t) * (1.0f - t);
            }

            output = force + noise;
        }
        else
        {
            active = false;
        }

        sampleCount++;
        return output;
    }

    void trigger(float vel, float hard)
    {
        velocity = vel;
        hardness = hard;
        active = true;
        sampleCount = 0;
    }

private:
    double sr = 44100.0;
    float velocity = 0.0f;
    float hardness = 0.5f;
    bool active = false;
    int sampleCount = 0;
};
```

---

## 10. Resonator Tube Simulation

### 10.1 Waveguide Tube Model

For physically accurate resonator tubes, use a simple waveguide:

```cpp
class WaveguideTube
{
public:
    void prepare(double sampleRate, float fundamentalFreq)
    {
        sr = sampleRate;
        // Delay length = half-wavelength for closed-end pipe
        int delaySamples = static_cast<int>(sampleRate / (2.0f * fundamentalFreq));
        delayLine.resize(delaySamples, 0.0f);
        writePos = 0;

        // End reflection filter (models radiation impedance)
        float cutoff = fundamentalFreq * 4.0f;
        endFilter.setCoefficients(
            juce::IIRCoefficients::makeLowPass(sampleRate, cutoff, 0.5f));
    }

    float process(float input)
    {
        // Read from delay
        int readPos = (writePos + 1) % (int)delayLine.size();
        float delayed = delayLine[readPos];

        // End reflection: invert and low-pass filter (closed end)
        float reflected = endFilter.processSingleSampleRaw(-delayed) * reflectionCoeff;

        // Write input + reflected wave
        delayLine[writePos] = input + reflected;
        writePos = (writePos + 1) % (int)delayLine.size();

        // Output is the wave at the open end
        return delayed + input;
    }

    float reflectionCoeff = 0.95f; // Energy retention per round trip

private:
    std::vector<float> delayLine;
    int writePos = 0;
    juce::IIRFilter endFilter;
    double sr = 44100.0;
};
```

---

## Part 4: JUCE Implementation

## 11. Complete Mallet Percussion Synthesizer

```cpp
class MalletPercussionProcessor : public juce::AudioProcessor
{
public:
    MalletPercussionProcessor()
        : AudioProcessor(BusesProperties()
            .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        sr = sampleRate;

        for (int m = 0; m < MaxModes; ++m)
            modeFilters[m].reset();

        malletStrike.prepare(sampleRate);
        resonatorTube.prepare(sampleRate, 261.6f); // Default: middle C
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
                float velocity = msg.getFloatVelocity();
                triggerNote(freq, velocity);
            }
        }

        if (!noteActive) return;

        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getWritePointer(1);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Generate excitation
            float excitation = malletStrike.process();

            // Sum modal resonators
            float barOutput = 0.0f;
            for (int m = 0; m < activePreset->numModes; ++m)
                barOutput += modeFilters[m].process(excitation);

            // Apply resonator tube
            float withTube = barOutput;
            if (useResonator)
                withTube = resonatorTube.process(barOutput);

            // Apply vibrato motor (vibraphone only)
            if (useMotor)
                withTube = vibMotor.process(withTube);

            float output = withTube * outputGain;

            // Check if note has decayed
            if (std::abs(output) < 0.00001f && !malletStrike.isActive())
            {
                decayCounter++;
                if (decayCounter > (int)sr) // 1 second of silence
                    noteActive = false;
            }
            else
            {
                decayCounter = 0;
            }

            outL[i] = output;
            outR[i] = output;
        }
    }

private:
    static constexpr int MaxModes = 16;
    double sr = 44100.0;
    BiquadMode modeFilters[MaxModes];
    MalletStrike malletStrike;
    ResonatorTube resonatorTube;
    VibraphoneMotor vibMotor;

    const InstrumentPreset* activePreset = &marimbaPreset;
    bool noteActive = false;
    bool useResonator = true;
    bool useMotor = false;
    float outputGain = 0.3f;
    float malletHardness = 0.5f;
    int decayCounter = 0;

    void triggerNote(float fundamental, float velocity)
    {
        noteActive = true;
        decayCounter = 0;

        // Set up modes from preset
        for (int m = 0; m < activePreset->numModes; ++m)
        {
            float freq = fundamental * activePreset->modes[m].freqRatio;
            float amp = activePreset->modes[m].amplitude;
            float decay = activePreset->modes[m].decayTime;

            // Clamp frequency to Nyquist
            if (freq < (float)sr * 0.45f)
                modeFilters[m].setMode(freq, decay, amp, sr);
            else
                modeFilters[m].setMode(100.0f, 0.01f, 0.0f, sr); // Mute
        }

        // Set up resonator tube for this note
        resonatorTube.prepare(sr, fundamental);

        // Trigger mallet
        malletStrike.trigger(velocity, malletHardness);
    }
};
```

---

## 12. Optimization and Real-Time Safety

### 12.1 Mode Culling

Skip modes that have decayed below audibility:

```cpp
// In process loop
if (modeFilters[m].envelope < 0.0001f)
    continue; // Skip inaudible modes
```

### 12.2 Frequency Clamping

Always clamp mode frequencies to below Nyquist:

```cpp
float freq = fundamental * modeRatio;
freq = juce::jmin(freq, (float)sampleRate * 0.45f);
```

### 12.3 Common Pitfalls

| Pitfall | Impact | Solution |
|---------|--------|----------|
| Modes above Nyquist | Aliasing, instability | Clamp to 0.45 * sampleRate |
| No denormal protection | CPU spike as modes decay | ScopedNoDenormals |
| Too few modes | Thin, unrealistic | Use 8-16 modes minimum |
| Ignoring strike position | Static timbre | Modulate mode amplitudes by position |
| No velocity sensitivity | Unmusical response | Scale excitation + mode decay by velocity |
| Fixed decay for all notes | Unnatural | Longer decay for lower notes |

---

## Part 5: References

## 13. References and Further Reading

### Academic Papers
- Chaigne, A. & Doutaut, V. (1997). "Numerical simulations of xylophones: Time-domain modeling of the vibrating bars." JASA, 101(1).
- Bork, I. (1995). "Practical tuning of xylophone bars and resonators." Applied Acoustics, 46(1).
- Henrique, L.L. & Antunes, J. (2003). "Optimal design and physical modelling of mallet percussion instruments." Acta Acustica.

### Books
- Fletcher, N.H. & Rossing, T.D. (1998). *The Physics of Musical Instruments*. Chapter 19: Mallet Percussion.
- Rossing, T.D. (2000). *Science of Percussion Instruments*. World Scientific.
- Cook, P.R. (2002). *Real Sound Synthesis for Interactive Applications*. Chapter 10: Bars and Pipes.
- Smith, J.O. "Physical Audio Signal Processing." Stanford online book.

### Software References
- PianoTeq (Modartt): Physical modeling including vibraphone, marimba, xylophone
- Chromaphone (AAS): Physical modeling mallet percussion
- Soniccouture Vibes: Sampled vibraphone with physical modeling resonance
- JUCE `juce::dsp::IIR::Filter` for modal resonators

---

*Research document for O-Marimba. Covers bar vibration physics, mallet-bar interaction, resonator tubes, instrument-specific modal presets, and JUCE physical modeling implementation.*
