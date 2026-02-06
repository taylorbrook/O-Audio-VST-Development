---
title: "Microtonality Implementation in JUCE VST Plugins"
created: 2026-01-09
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Complete implementation guide for microtonal support in JUCE plugins, covering MTS-ESP client integration, Surge tuning library, custom tuning tables, MPE integration, practical code patterns, and common pitfalls with solutions."
domain: dsp
type: guide
keywords:
  - microtonality
  - juce-dsp
  - mts-esp
  - mpe
  - tuning-tables
  - synthesizer
  - pitch-mapping
stages: [1, 2]
agents: [dsp]
---

# Microtonality Implementation in JUCE VST Plugins

## Complete Implementation Guide

**Research Date:** 2026-01-09
**Research Level:** Comprehensive (Multi-source)
**Confidence:** HIGH

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Synthesizer Implementation](#1-synthesizer-implementation)
3. [Sampler Implementation](#2-sampler-implementation)
4. [Physical Modeling](#3-physical-modeling)
5. [Effects Processing](#4-effects-processing)
6. [JUCE Implementation Patterns](#5-juce-implementation-patterns)
7. [Practical Code Patterns](#6-practical-code-patterns)
8. [Integration Libraries](#7-integration-libraries)
9. [Common Pitfalls and Solutions](#8-common-pitfalls-and-solutions)
10. [References](#9-references)

---

## Executive Summary

### Key Implementation Approaches

| Approach | Complexity | Best For | Integration Time |
|----------|------------|----------|------------------|
| **MTS-ESP Client** | LOW | Universal support, DAW-wide tuning | 15 minutes |
| **Surge Tuning Library** | MEDIUM | Scala/KBM file support | 1-2 hours |
| **Custom Tuning Tables** | MEDIUM | Full control, specialized needs | 2-4 hours |
| **MPE Integration** | MEDIUM-HIGH | Per-note expression, controllers | 4-8 hours |

### Quick Start Recommendation

For new synthesizers, implement **MTS-ESP** as the primary method with **Scala file support** via the Surge tuning library as a fallback. This provides compatibility with both DAW-wide tuning and standalone file-based tuning.

```cpp
// Minimal MTS-ESP Integration (15-minute implementation)
#include "libMTSClient.h"

class MicrotonalSynth {
    MTSClient* mtsClient = nullptr;

    void initialize() {
        mtsClient = MTS_RegisterClient();
    }

    ~MicrotonalSynth() {
        if (mtsClient) MTS_DeregisterClient(mtsClient);
    }

    double getFrequency(int midiNote, int midiChannel = -1) {
        if (mtsClient && MTS_HasMaster(mtsClient)) {
            return MTS_NoteToFrequency(mtsClient, midiNote, midiChannel);
        }
        // Fallback to 12-TET
        return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    }
};
```

---

## 1. Synthesizer Implementation

### 1.1 Oscillator Frequency Calculation

The fundamental challenge: converting MIDI note + tuning offset to precise frequency.

#### Standard 12-TET Formula

```cpp
// JUCE built-in method
double freq = juce::MidiMessage::getMidiNoteInHertz(midiNote, 440.0);

// Equivalent formula
double freq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
```

#### Microtonal Frequency Calculation

```cpp
class MicrotonalFrequency {
public:
    // Method 1: Direct frequency lookup (MTS-ESP style)
    double getFrequency(int midiNote) {
        return tuningTable[midiNote];  // Pre-computed table
    }

    // Method 2: Semitone offset from 12-TET
    double getFrequencyFromOffset(int midiNote, double centsOffset) {
        double baseFreq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
        return baseFreq * std::pow(2.0, centsOffset / 1200.0);
    }

    // Method 3: Ratio-based (for just intonation)
    double getFrequencyFromRatio(double baseFreq, int numerator, int denominator) {
        return baseFreq * (static_cast<double>(numerator) / denominator);
    }

    // Method 4: Floating-point MIDI note
    double getFrequencyFromFloat(double floatMidiNote) {
        return 440.0 * std::pow(2.0, (floatMidiNote - 69.0) / 12.0);
    }
};
```

#### Phase Accumulator for Wavetable

```cpp
class MicrotonalOscillator {
    double phase = 0.0;
    double phaseIncrement = 0.0;
    double sampleRate = 44100.0;

public:
    void setFrequency(double freq) {
        phaseIncrement = freq / sampleRate;
    }

    // For microtonal: accept fractional MIDI note or direct frequency
    void setMicrotonalPitch(double frequencyHz) {
        phaseIncrement = frequencyHz / sampleRate;
    }

    float process(const float* wavetable, int tableSize) {
        int index0 = static_cast<int>(phase * tableSize) % tableSize;
        int index1 = (index0 + 1) % tableSize;
        float frac = static_cast<float>(phase * tableSize - index0);

        float sample = wavetable[index0] * (1.0f - frac) + wavetable[index1] * frac;

        phase += phaseIncrement;
        if (phase >= 1.0) phase -= 1.0;

        return sample;
    }
};
```

### 1.2 Voice Management with Per-Note Tuning

```cpp
class MicrotonalVoice : public juce::SynthesiserVoice {
    double currentFrequency = 440.0;
    double pitchBendSemitones = 0.0;
    double microtonalOffset = 0.0;  // In cents
    MTSClient* mtsClient = nullptr;
    int currentMidiChannel = 1;

public:
    void setMTSClient(MTSClient* client) { mtsClient = client; }

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound*, int currentPitchWheelPosition) override {
        // Get microtonal frequency
        if (mtsClient && MTS_HasMaster(mtsClient)) {
            currentFrequency = MTS_NoteToFrequency(mtsClient, midiNoteNumber, currentMidiChannel);
        } else {
            // Fallback to custom tuning table or 12-TET
            currentFrequency = getTuningTableFrequency(midiNoteNumber);
        }

        updateOscillatorFrequency();
    }

    void pitchWheelMoved(int newPitchWheelValue) override {
        // Convert 14-bit pitch wheel to semitones (typical range: +/- 2 semitones)
        double pitchBendRange = 2.0; // Semitones
        pitchBendSemitones = ((newPitchWheelValue - 8192) / 8192.0) * pitchBendRange;
        updateOscillatorFrequency();
    }

private:
    void updateOscillatorFrequency() {
        double finalFreq = currentFrequency * std::pow(2.0, pitchBendSemitones / 12.0);
        oscillator.setFrequency(finalFreq);
    }
};
```

### 1.3 MPE Voice with Per-Note Pitch Bend

```cpp
class MPEMicrotonalVoice : public juce::MPESynthesiserVoice {
public:
    void noteStarted() override {
        auto note = getCurrentlyPlayingNote();

        // MPE provides per-note pitch bend automatically
        updateFrequency();
    }

    void notePitchbendChanged() override {
        updateFrequency();
    }

private:
    void updateFrequency() {
        auto note = getCurrentlyPlayingNote();

        // JUCE MPE handles per-note + master pitchbend automatically
        double frequency = note.getFrequencyInHertz();

        // For additional microtonal offset (e.g., from MTS-ESP)
        if (mtsClient && MTS_HasMaster(mtsClient)) {
            double mtsRetuning = MTS_RetuningInSemitones(mtsClient,
                note.initialNote, note.midiChannel);
            frequency *= std::pow(2.0, mtsRetuning / 12.0);
        }

        oscillator.setFrequency(frequency);
    }
};
```

### 1.4 FM Synthesis Microtonal Considerations

FM synthesis requires special attention for microtonal support because modulator frequencies affect the harmonic spectrum.

```cpp
class MicrotonalFMOperator {
    double carrierFrequency = 440.0;
    double modulatorRatio = 2.0;  // Harmonic ratio (1.0, 2.0, 3.0, etc.)
    double modulationIndex = 1.0;
    double carrierPhase = 0.0;
    double modulatorPhase = 0.0;
    double sampleRate = 44100.0;

public:
    void setCarrierFrequency(double freq) {
        carrierFrequency = freq;
    }

    // For harmonic FM: keep ratio constant regardless of tuning
    void setModulatorRatio(double ratio) {
        modulatorRatio = ratio;
    }

    // For inharmonic FM: set absolute frequency
    void setModulatorFrequency(double freq) {
        modulatorRatio = freq / carrierFrequency;
    }

    float process() {
        double modulatorFreq = carrierFrequency * modulatorRatio;

        // Calculate modulator contribution
        double modOutput = std::sin(modulatorPhase * juce::MathConstants<double>::twoPi);

        // Apply modulation to carrier phase
        double phaseModulation = modOutput * modulationIndex;
        double carrierOutput = std::sin((carrierPhase + phaseModulation) *
                                        juce::MathConstants<double>::twoPi);

        // Update phases
        carrierPhase += carrierFrequency / sampleRate;
        modulatorPhase += modulatorFreq / sampleRate;

        // Wrap phases
        if (carrierPhase >= 1.0) carrierPhase -= 1.0;
        if (modulatorPhase >= 1.0) modulatorPhase -= 1.0;

        return static_cast<float>(carrierOutput);
    }
};
```

**FM Tuning Design Decision:**
- **Harmonic FM:** Keep modulator as a ratio of carrier - preserves harmonic relationships
- **Inharmonic FM:** Allow absolute modulator frequencies - creates non-harmonic timbres
- **Microtonal FM:** Consider whether ratios should follow the tuning scale

### 1.5 Wavetable Synthesis with Microtonality

```cpp
class MicrotonalWavetableOscillator {
    // Multiple mip-mapped wavetables to prevent aliasing
    std::vector<std::vector<float>> mipMappedTables;
    int baseTableSize = 2048;
    double sampleRate = 44100.0;
    double phase = 0.0;

public:
    void setFrequency(double freq) {
        // Select appropriate mip-map level based on frequency
        int mipLevel = calculateMipLevel(freq);
        currentTable = &mipMappedTables[mipLevel];
        phaseIncrement = freq / sampleRate;
    }

    float process() {
        // Linear or cubic interpolation for sub-sample accuracy
        int tableSize = currentTable->size();
        double scaledPhase = phase * tableSize;

        int i0 = static_cast<int>(scaledPhase) % tableSize;
        int i1 = (i0 + 1) % tableSize;
        float frac = static_cast<float>(scaledPhase - std::floor(scaledPhase));

        float sample = (*currentTable)[i0] * (1.0f - frac) +
                       (*currentTable)[i1] * frac;

        phase += phaseIncrement;
        if (phase >= 1.0) phase -= 1.0;

        return sample;
    }

private:
    int calculateMipLevel(double freq) {
        // Higher frequencies need wavetables with fewer harmonics
        double nyquist = sampleRate / 2.0;
        double fundamentalHarmonics = nyquist / freq;
        return static_cast<int>(std::log2(std::max(1.0, baseTableSize / fundamentalHarmonics)));
    }
};
```

### 1.6 Subtractive Synthesis Filter Tracking

Filter keyboard tracking must account for microtonal pitch offsets:

```cpp
class MicrotonalFilteredVoice {
    juce::dsp::IIR::Filter<float> lowpassFilter;
    double keyboardTrackingAmount = 1.0;  // 0-2, where 1.0 = 100%
    double baseCutoff = 1000.0;           // Hz

public:
    void updateFilterForNote(int midiNote, double microtonalFrequency) {
        // Calculate semitone offset from middle C (MIDI 60)
        double semitoneOffset = 12.0 * std::log2(microtonalFrequency /
            juce::MidiMessage::getMidiNoteInHertz(60));

        // Apply keyboard tracking
        double trackingMultiplier = std::pow(2.0,
            (semitoneOffset * keyboardTrackingAmount) / 12.0);

        double finalCutoff = baseCutoff * trackingMultiplier;
        finalCutoff = juce::jlimit(20.0, 20000.0, finalCutoff);

        *lowpassFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate, static_cast<float>(finalCutoff));
    }
};
```

---

## 2. Sampler Implementation

### 2.1 Pitch Shifting Algorithms Comparison

| Algorithm | Quality | CPU Cost | Artifacts | Best For |
|-----------|---------|----------|-----------|----------|
| **Linear Interpolation** | Low | Very Low | Aliasing at extremes | Simple playback |
| **Cubic/Lagrange Interpolation** | Medium | Low | Slight softening | General use |
| **Sinc Interpolation** | High | Medium | Phase issues | High quality |
| **PSOLA** | High (speech) | Medium | Formant preservation | Vocal samples |
| **Phase Vocoder** | High | High | Transient smearing | Polyphonic content |
| **Granular** | Variable | Medium | Grain artifacts | Creative effects |

### 2.2 Resampling for Arbitrary Tuning

```cpp
class MicrotonalSampler {
    const float* sampleData;
    int sampleLength;
    double sampleRate;
    double originalPitch;  // Pitch of the recorded sample (Hz)
    double readPosition = 0.0;

public:
    void setPitch(double targetPitch) {
        // Calculate playback speed ratio
        playbackRatio = targetPitch / originalPitch;
    }

    // Linear interpolation (simplest)
    float processLinear() {
        int i0 = static_cast<int>(readPosition);
        int i1 = i0 + 1;

        if (i1 >= sampleLength) return 0.0f;

        float frac = static_cast<float>(readPosition - i0);
        float sample = sampleData[i0] * (1.0f - frac) + sampleData[i1] * frac;

        readPosition += playbackRatio;
        return sample;
    }

    // Cubic interpolation (better quality)
    float processCubic() {
        int i0 = static_cast<int>(readPosition);
        int im1 = std::max(0, i0 - 1);
        int i1 = std::min(sampleLength - 1, i0 + 1);
        int i2 = std::min(sampleLength - 1, i0 + 2);

        float frac = static_cast<float>(readPosition - i0);

        // Catmull-Rom spline
        float a = sampleData[im1];
        float b = sampleData[i0];
        float c = sampleData[i1];
        float d = sampleData[i2];

        float t = frac;
        float tt = t * t;
        float ttt = tt * t;

        float sample = 0.5f * ((2*b) +
                               (-a + c) * t +
                               (2*a - 5*b + 4*c - d) * tt +
                               (-a + 3*b - 3*c + d) * ttt);

        readPosition += playbackRatio;
        return sample;
    }

    // Sinc interpolation (highest quality)
    float processSinc(int kernelSize = 16) {
        float sample = 0.0f;
        int center = static_cast<int>(readPosition);
        float frac = static_cast<float>(readPosition - center);

        for (int i = -kernelSize/2; i < kernelSize/2; ++i) {
            int idx = center + i;
            if (idx < 0 || idx >= sampleLength) continue;

            float x = i - frac;
            float sincValue = (x == 0.0f) ? 1.0f :
                std::sin(juce::MathConstants<float>::pi * x) /
                (juce::MathConstants<float>::pi * x);

            // Window function (Blackman-Harris)
            float window = 0.35875f - 0.48829f * std::cos(2 * juce::MathConstants<float>::pi *
                (i + kernelSize/2) / (kernelSize - 1));

            sample += sampleData[idx] * sincValue * window;
        }

        readPosition += playbackRatio;
        return sample;
    }

private:
    double playbackRatio = 1.0;
};
```

### 2.3 Granular Synthesis for Microtonal Playback

Granular synthesis can achieve arbitrary pitch shifts while preserving duration:

```cpp
class GranularPitchShifter {
    struct Grain {
        double position;
        double phase;
        double pitchRatio;
        bool active;
    };

    std::vector<Grain> grains;
    const float* sampleData;
    int sampleLength;
    double grainSizeMs = 30.0;    // 20-50ms typical
    double overlapFactor = 4.0;   // Number of overlapping grains
    double sampleRate;

public:
    float process(double pitchRatio, double readPosition) {
        float output = 0.0f;
        int grainSizeSamples = static_cast<int>(grainSizeMs * sampleRate / 1000.0);

        for (auto& grain : grains) {
            if (!grain.active) continue;

            // Calculate grain position with pitch shift
            double grainPosition = grain.position + grain.phase * pitchRatio;

            if (grainPosition >= sampleLength || grain.phase >= grainSizeSamples) {
                grain.active = false;
                continue;
            }

            // Get sample with interpolation
            float sample = interpolate(grainPosition);

            // Apply window (Hann)
            float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi *
                grain.phase / grainSizeSamples));

            output += sample * window;
            grain.phase += 1.0;
        }

        // Spawn new grains at regular intervals
        maybeSpawnGrain(readPosition);

        // Normalize by overlap factor
        return output / static_cast<float>(overlapFactor);
    }

private:
    float interpolate(double position) {
        int i0 = static_cast<int>(position);
        float frac = static_cast<float>(position - i0);

        if (i0 + 1 >= sampleLength) return 0.0f;

        return sampleData[i0] * (1.0f - frac) + sampleData[i0 + 1] * frac;
    }

    void maybeSpawnGrain(double position) {
        // Spawn grain logic based on overlap factor
    }
};
```

### 2.4 Multi-Sample Optimization for Microtonal Ranges

For samplers using multiple recorded pitches, select the closest sample to minimize pitch shifting:

```cpp
class MultiSampleSelector {
    struct SampleZone {
        int rootNote;              // MIDI note the sample was recorded at
        double rootFrequency;      // Actual frequency (can be microtonal)
        int lowNote, highNote;     // Note range
        const float* data;
        int length;
    };

    std::vector<SampleZone> zones;

public:
    const SampleZone* selectZone(double targetFrequency) {
        const SampleZone* bestZone = nullptr;
        double minRatio = std::numeric_limits<double>::max();

        for (const auto& zone : zones) {
            // Calculate pitch ratio required
            double ratio = targetFrequency / zone.rootFrequency;
            double semitoneShift = 12.0 * std::log2(ratio);

            // Prefer zones requiring less pitch shift
            if (std::abs(semitoneShift) < std::abs(12.0 * std::log2(minRatio))) {
                minRatio = ratio;
                bestZone = &zone;
            }
        }

        return bestZone;
    }

    // For microtonal samplers: zones based on frequency regions, not notes
    const SampleZone* selectZoneByFrequency(double targetFrequency,
                                             double maxCentsShift = 200.0) {
        for (const auto& zone : zones) {
            double ratio = targetFrequency / zone.rootFrequency;
            double centsShift = 1200.0 * std::log2(ratio);

            if (std::abs(centsShift) <= maxCentsShift) {
                return &zone;
            }
        }
        return nullptr;  // No suitable zone
    }
};
```

---

## 3. Physical Modeling

### 3.1 Karplus-Strong with Microtonal Pitch

The fundamental pitch in Karplus-Strong is determined by delay line length:

```cpp
class MicrotonalKarplusStrong {
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>
        delayLine { 88200 };  // Max 2 seconds at 44.1kHz
    juce::dsp::IIR::Filter<float> loopFilter;
    float feedbackSample = 0.0f;
    double sampleRate = 44100.0;

public:
    void prepare(double sr) {
        sampleRate = sr;
        juce::dsp::ProcessSpec spec { sr, 512, 1 };
        delayLine.prepare(spec);
        loopFilter.prepare(spec);
    }

    // Set pitch using microtonal frequency
    void trigger(double frequencyHz, float velocity) {
        // Calculate delay length for desired frequency
        // Fractional samples supported by Lagrange interpolation
        double delaySamples = sampleRate / frequencyHz;

        // Validate range (prevent extreme frequencies)
        delaySamples = juce::jlimit(2.0, 88200.0, delaySamples);

        // Reset and fill with noise burst
        delayLine.reset();
        feedbackSample = 0.0f;

        // Fill delay line with filtered noise (excitation)
        for (int i = 0; i < static_cast<int>(delaySamples); ++i) {
            float noise = (random.nextFloat() * 2.0f - 1.0f) * velocity;
            delayLine.pushSample(0, noise);
        }
    }

    float process(double frequencyHz, float damping) {
        double delaySamples = sampleRate / frequencyHz;

        // Read from delay line
        float output = delayLine.popSample(0, static_cast<float>(delaySamples));

        // Loop filter (string damping)
        float filtered = loopFilter.processSample(output);

        // Feedback with damping
        feedbackSample = filtered * damping;
        delayLine.pushSample(0, feedbackSample);

        return filtered;
    }

private:
    juce::Random random;
};
```

### 3.2 Fractional Delay Filters for Precise Tuning

For sub-sample pitch accuracy, use fractional delay filters:

```cpp
class FractionalDelayAllpass {
    // First-order Thiran allpass for fractional delay
    float y1 = 0.0f;
    float x1 = 0.0f;
    float a1 = 0.0f;

public:
    void setFractionalDelay(double fractionalSamples) {
        // Thiran allpass coefficient
        // Valid for delays in range [0.1, 1.9]
        double d = fractionalSamples;
        a1 = static_cast<float>((1.0 - d) / (1.0 + d));
    }

    float process(float input) {
        float output = a1 * (input - y1) + x1;
        x1 = input;
        y1 = output;
        return output;
    }
};

class PrecisePitchDelayLine {
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> integerDelay { 88200 };
    FractionalDelayAllpass fractionalDelay;

public:
    float popSample(double exactDelay) {
        int integerPart = static_cast<int>(exactDelay);
        double fractionalPart = exactDelay - integerPart;

        // Get integer-delayed sample
        float sample = integerDelay.popSample(0, integerPart);

        // Apply fractional delay
        fractionalDelay.setFractionalDelay(fractionalPart);
        return fractionalDelay.process(sample);
    }
};
```

### 3.3 Waveguide Synthesis with Microtonal Pitch

```cpp
class MicrotonalWaveguide {
    // Bidirectional delay lines
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>
        rightWave { 8820 }, leftWave { 8820 };

    // Termination filters
    juce::dsp::IIR::Filter<float> bridgeFilter, nutFilter;

    double sampleRate = 44100.0;
    float output = 0.0f;

public:
    void setFrequency(double frequencyHz) {
        // Total delay = sample_rate / frequency
        // Split between two directions
        currentDelay = (sampleRate / frequencyHz) / 2.0;
    }

    void process(float excitation) {
        // Read from both directions
        float rightOut = rightWave.popSample(0, static_cast<float>(currentDelay));
        float leftOut = leftWave.popSample(0, static_cast<float>(currentDelay));

        // Reflections with inversion and filtering
        float rightReflect = nutFilter.processSample(-leftOut * damping);
        float leftReflect = bridgeFilter.processSample(-rightOut * damping);

        // Add excitation at pluck position
        rightWave.pushSample(0, rightReflect + excitation);
        leftWave.pushSample(0, leftReflect);

        output = (rightOut + leftOut) * 0.5f;
    }

    float getOutput() const { return output; }

private:
    double currentDelay = 100.0;
    float damping = 0.995f;
};
```

### 3.4 Modal Synthesis Frequency Adjustment

Modal synthesis is particularly well-suited to microtonality because each mode frequency can be adjusted independently:

```cpp
class MicrotonalModalSynth {
    struct Mode {
        double frequency;      // Hz
        double decay;          // Seconds
        double amplitude;      // Relative level

        // State variables
        double y1 = 0.0, y2 = 0.0;
        double a1, a2, b0;

        void updateCoefficients(double sampleRate, double newFrequency) {
            frequency = newFrequency;
            double theta = juce::MathConstants<double>::twoPi * frequency / sampleRate;
            double r = std::exp(-1.0 / (decay * sampleRate));

            a1 = 2.0 * r * std::cos(theta);
            a2 = -r * r;
            b0 = amplitude * (1.0 - r);
        }

        double process(double excitation) {
            double y = b0 * excitation + a1 * y1 + a2 * y2;
            y2 = y1;
            y1 = y;
            return y;
        }
    };

    std::vector<Mode> modes;
    double fundamentalFrequency = 220.0;
    double sampleRate = 44100.0;

public:
    // Set fundamental and adjust all mode frequencies proportionally
    void setFundamental(double newFundamental) {
        double ratio = newFundamental / fundamentalFrequency;
        fundamentalFrequency = newFundamental;

        for (auto& mode : modes) {
            mode.updateCoefficients(sampleRate, mode.frequency * ratio);
        }
    }

    // For non-harmonic/microtonal: set individual mode frequencies
    void setModeFrequency(int modeIndex, double frequency) {
        if (modeIndex < modes.size()) {
            modes[modeIndex].updateCoefficients(sampleRate, frequency);
        }
    }

    double process(double excitation) {
        double output = 0.0;
        for (auto& mode : modes) {
            output += mode.process(excitation);
        }
        return output;
    }
};
```

---

## 4. Effects Processing

### 4.1 Pitch-Aware Delay

Standard delays can create comb filtering that conflicts with microtonal tuning. Solution: tune delay times to the fundamental.

```cpp
class PitchAwareDelay {
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>
        delayLine { 192000 };
    double sampleRate = 44100.0;

public:
    // Set delay time as a multiple of the period
    void setDelayForPitch(double fundamentalHz, int periods) {
        double periodSamples = sampleRate / fundamentalHz;
        double delaySamples = periodSamples * periods;
        delayLine.setDelay(static_cast<float>(delaySamples));
    }

    // Set delay to avoid specific harmonic interference
    void setDelayToAvoidComb(double fundamentalHz) {
        // Set delay to 0.5 periods to create complementary comb
        double periodSamples = sampleRate / fundamentalHz;
        double delaySamples = periodSamples * 0.5;
        delayLine.setDelay(static_cast<float>(delaySamples));
    }

    float process(float input) {
        delayLine.pushSample(0, input);
        return delayLine.popSample(0);
    }
};
```

### 4.2 Harmonic-Aware Distortion

For microtonal content, consider how distortion generates harmonics:

```cpp
class MicrotonalDistortion {
    // Soft clipping preserves more of original pitch content
    float softClip(float input, float drive) {
        return std::tanh(input * drive);
    }

    // Waveshaping with harmonic control
    float waveshape(float input, float oddHarmonics, float evenHarmonics) {
        float output = 0.0f;

        // Odd harmonics (tanh-like)
        output += oddHarmonics * std::tanh(input);

        // Even harmonics (asymmetric)
        float squared = input * input;
        output += evenHarmonics * (squared - 0.5f);

        return output;
    }

    // Low-order waveshaping (fewer added harmonics = less pitch interference)
    float gentleDistortion(float input) {
        // Cubic soft clip: x - x^3/3
        float x = juce::jlimit(-1.0f, 1.0f, input);
        return x - (x * x * x) / 3.0f;
    }
};
```

### 4.3 Pitch Shifting Effects with Microtonal Input

When pitch-shifting already-microtonal audio:

```cpp
class MicrotonalPitchShifter {
    // Phase vocoder approach
    std::vector<float> analysisFrame;
    std::vector<float> synthesisFrame;
    std::vector<float> magnitudes;
    std::vector<float> phases;
    std::vector<float> prevPhases;

    int fftSize = 2048;
    int hopSize = 512;
    double pitchShiftRatio = 1.0;  // 1.0 = no shift

public:
    // Shift by semitones (works with any input tuning)
    void setShiftSemitones(double semitones) {
        pitchShiftRatio = std::pow(2.0, semitones / 12.0);
    }

    // Shift by cents (finer control)
    void setShiftCents(double cents) {
        pitchShiftRatio = std::pow(2.0, cents / 1200.0);
    }

    // Shift to achieve specific interval in current scale
    void setShiftForInterval(double intervalRatio) {
        // e.g., 3/2 for perfect fifth in just intonation
        pitchShiftRatio = intervalRatio;
    }

    void process(float* input, float* output, int numSamples) {
        // Phase vocoder implementation
        // (Full implementation would include FFT analysis/synthesis)
    }
};
```

### 4.4 Vocoder with Formant Preservation

For microtonal vocals, formant preservation is critical:

```cpp
class MicrotonalVocoder {
    // Carrier oscillators (bank of sine waves or synth)
    std::vector<double> carrierFrequencies;
    std::vector<double> carrierPhases;

    // Analysis filter bank
    std::vector<juce::dsp::IIR::Filter<float>> analysisFilters;

    // Envelope followers
    std::vector<juce::LinearSmoothedValue<float>> envelopes;

public:
    // Set carrier frequencies from tuning table
    void setCarrierFrequencies(const std::vector<double>& frequencies) {
        carrierFrequencies = frequencies;
        carrierPhases.resize(frequencies.size(), 0.0);
    }

    // Alternative: derive carriers from analysis pitch
    void setCarriersFromPitch(double fundamentalHz, int numHarmonics) {
        carrierFrequencies.clear();
        for (int i = 1; i <= numHarmonics; ++i) {
            carrierFrequencies.push_back(fundamentalHz * i);
        }
    }

    // Set carriers from microtonal scale
    void setCarriersFromScale(double rootHz, const std::vector<double>& scaleRatios) {
        carrierFrequencies.clear();
        for (double ratio : scaleRatios) {
            carrierFrequencies.push_back(rootHz * ratio);
        }
    }

    float process(float modulatorInput, float carrierInput) {
        // Analyze modulator
        updateEnvelopes(modulatorInput);

        // Synthesize with carrier frequencies modulated by envelopes
        float output = 0.0f;
        for (size_t i = 0; i < carrierFrequencies.size(); ++i) {
            float envelope = envelopes[i].getCurrentValue();
            float carrier = std::sin(carrierPhases[i] * juce::MathConstants<float>::twoPi);
            output += carrier * envelope;

            carrierPhases[i] += carrierFrequencies[i] / sampleRate;
            if (carrierPhases[i] >= 1.0) carrierPhases[i] -= 1.0;
        }

        return output / static_cast<float>(carrierFrequencies.size());
    }

private:
    void updateEnvelopes(float input) {
        for (size_t i = 0; i < analysisFilters.size(); ++i) {
            float filtered = analysisFilters[i].processSample(input);
            float rectified = std::abs(filtered);
            envelopes[i].setTargetValue(rectified);
        }
    }

    double sampleRate = 44100.0;
};
```

---

## 5. JUCE Implementation Patterns

### 5.1 MPE Integration

JUCE provides comprehensive MPE support via `MPESynthesiser` and `MPEInstrument`:

```cpp
class MicrotonalMPEProcessor : public juce::AudioProcessor {
    juce::MPEInstrument mpeInstrument;
    juce::MPESynthesiser mpeSynth;

public:
    MicrotonalMPEProcessor() {
        // Configure MPE zones
        mpeInstrument.enableLegacyMode();  // Or use setZoneLayout for true MPE

        // Configure pitch bend range
        auto layout = juce::MPEZoneLayout();
        layout.setLowerZone(juce::MPEZone(juce::MPEZone::Type::lower,
                                          15,    // 15 member channels
                                          48));  // 48 semitone pitch bend range
        mpeInstrument.setZoneLayout(layout);

        // Add voices
        for (int i = 0; i < 16; ++i) {
            mpeSynth.addVoice(new MicrotonalMPEVoice());
        }
        mpeSynth.addSound(new MicrotonalMPESound());
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override {
        mpeSynth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
    }
};

class MicrotonalMPEVoice : public juce::MPESynthesiserVoice {
public:
    void noteStarted() override {
        auto note = getCurrentlyPlayingNote();
        updateFrequency();
    }

    void notePitchbendChanged() override {
        updateFrequency();
    }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override {
        // Generate audio at current frequency
        for (int sample = startSample; sample < startSample + numSamples; ++sample) {
            float output = oscillator.process();
            for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
                outputBuffer.addSample(channel, sample, output * currentLevel);
            }
        }
    }

private:
    void updateFrequency() {
        auto note = getCurrentlyPlayingNote();

        // getFrequencyInHertz() includes per-note AND master pitchbend
        double frequency = note.getFrequencyInHertz();

        // Optional: apply additional microtonal offset from MTS-ESP
        if (mtsClient && MTS_HasMaster(mtsClient)) {
            double offset = MTS_RetuningInSemitones(mtsClient,
                note.initialNote, note.midiChannel);
            frequency *= std::pow(2.0, offset / 12.0);
        }

        oscillator.setFrequency(frequency);
    }

    MicrotonalOscillator oscillator;
    MTSClient* mtsClient = nullptr;
    float currentLevel = 1.0f;
};
```

### 5.2 Pitch Bend Handling

```cpp
class PitchBendHandler {
    double pitchBendRange = 2.0;  // Semitones (default +/- 2)
    double currentBendSemitones = 0.0;

public:
    // Standard 14-bit pitch bend (0-16383, center at 8192)
    void handlePitchBend(int pitchBendValue) {
        double normalized = (pitchBendValue - 8192) / 8192.0;  // -1 to +1
        currentBendSemitones = normalized * pitchBendRange;
    }

    // MPE high-resolution pitch bend (per-note)
    void handleMPEPitchBend(int pitchBendValue, int midiChannel) {
        // MPE typically uses 48 semitone range
        double normalized = (pitchBendValue - 8192) / 8192.0;
        currentBendSemitones = normalized * 48.0;  // Or get from zone config
    }

    // Convert pitch bend to frequency multiplier
    double getFrequencyMultiplier() const {
        return std::pow(2.0, currentBendSemitones / 12.0);
    }

    // Convert pitch bend to cents
    double getBendInCents() const {
        return currentBendSemitones * 100.0;
    }

    // For microtonal: fine-tune pitch bend to work with scale
    void setMicrotonalBendRange(double maxCents) {
        pitchBendRange = maxCents / 100.0;
    }
};
```

### 5.3 MIDI Note to Frequency with Tuning Tables

```cpp
class TuningTable {
    std::array<double, 128> frequencies;  // All MIDI notes
    double referenceFrequency = 440.0;    // A4
    int referenceNote = 69;               // MIDI note for reference

public:
    TuningTable() {
        // Initialize to 12-TET
        for (int note = 0; note < 128; ++note) {
            frequencies[note] = referenceFrequency *
                std::pow(2.0, (note - referenceNote) / 12.0);
        }
    }

    double getFrequency(int midiNote) const {
        return frequencies[juce::jlimit(0, 127, midiNote)];
    }

    // Load from Scala file (via Surge tuning library)
    void loadScala(const Tunings::Scale& scale,
                   const Tunings::KeyboardMapping& mapping) {
        Tunings::Tuning tuning(scale, mapping);

        for (int note = 0; note < 128; ++note) {
            frequencies[note] = tuning.frequencyForMidiNote(note);
        }
    }

    // Set equal temperament with custom division
    void setEqualTemperament(int divisions, double octaveRatio = 2.0) {
        for (int note = 0; note < 128; ++note) {
            int semitones = note - referenceNote;
            frequencies[note] = referenceFrequency *
                std::pow(octaveRatio, static_cast<double>(semitones) / divisions);
        }
    }

    // Set just intonation ratios
    void setJustIntonation(const std::vector<std::pair<int, int>>& ratios) {
        // ratios = {{1,1}, {16,15}, {9,8}, {6,5}, {5,4}, ...}
        int scaleSize = static_cast<int>(ratios.size());

        for (int note = 0; note < 128; ++note) {
            int octave = (note - referenceNote) / scaleSize;
            int degree = ((note - referenceNote) % scaleSize + scaleSize) % scaleSize;

            double ratio = static_cast<double>(ratios[degree].first) /
                           ratios[degree].second;
            double octaveMultiplier = std::pow(2.0, octave);

            frequencies[note] = referenceFrequency * ratio * octaveMultiplier;
        }
    }

    // Set individual note
    void setNoteFrequency(int midiNote, double frequency) {
        if (midiNote >= 0 && midiNote < 128) {
            frequencies[midiNote] = frequency;
        }
    }
};
```

### 5.4 Voice Class Modifications

```cpp
class MicrotonalSynthVoice : public juce::SynthesiserVoice {
    TuningTable* tuningTable = nullptr;
    MTSClient* mtsClient = nullptr;

    double currentFrequency = 440.0;
    int currentMidiNote = 69;
    int currentMidiChannel = 1;

    // For continuous pitch updates
    bool useContinuousTuning = true;

public:
    void setTuningTable(TuningTable* table) { tuningTable = table; }
    void setMTSClient(MTSClient* client) { mtsClient = client; }

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound*, int) override {
        currentMidiNote = midiNoteNumber;
        updateFrequency();

        // Start envelopes, etc.
        ampEnvelope.noteOn();
    }

    void stopNote(float velocity, bool allowTailOff) override {
        if (allowTailOff) {
            ampEnvelope.noteOff();
        } else {
            clearCurrentNote();
        }
    }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override {
        if (!isVoiceActive()) return;

        // Optionally update tuning continuously (for MTS-ESP)
        if (useContinuousTuning) {
            updateFrequency();
        }

        for (int sample = startSample; sample < startSample + numSamples; ++sample) {
            float envValue = ampEnvelope.getNextSample();
            float output = oscillator.process() * envValue;

            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
                outputBuffer.addSample(ch, sample, output);
            }

            if (!ampEnvelope.isActive()) {
                clearCurrentNote();
                break;
            }
        }
    }

private:
    void updateFrequency() {
        // Priority: MTS-ESP > TuningTable > 12-TET
        if (mtsClient && MTS_HasMaster(mtsClient)) {
            currentFrequency = MTS_NoteToFrequency(mtsClient,
                currentMidiNote, currentMidiChannel);
        } else if (tuningTable) {
            currentFrequency = tuningTable->getFrequency(currentMidiNote);
        } else {
            currentFrequency = juce::MidiMessage::getMidiNoteInHertz(currentMidiNote);
        }

        // Apply pitch bend
        currentFrequency *= pitchBendMultiplier;

        oscillator.setFrequency(currentFrequency);
    }

    MicrotonalOscillator oscillator;
    juce::ADSR ampEnvelope;
    double pitchBendMultiplier = 1.0;
};
```

### 5.5 Real-Time Tuning Table Updates

```cpp
class RealTimeTuningManager {
    std::atomic<bool> tuningChanged { false };
    TuningTable currentTuning;
    TuningTable pendingTuning;
    juce::SpinLock tuningLock;

public:
    // Called from UI thread
    void loadNewTuning(const Tunings::Scale& scale,
                       const Tunings::KeyboardMapping& mapping) {
        // Prepare new tuning (non-realtime)
        TuningTable newTuning;
        newTuning.loadScala(scale, mapping);

        // Swap atomically
        {
            const juce::SpinLock::ScopedLockType lock(tuningLock);
            pendingTuning = newTuning;
        }
        tuningChanged.store(true);
    }

    // Called from audio thread (start of processBlock)
    void updateIfNeeded() {
        if (tuningChanged.load()) {
            const juce::SpinLock::ScopedTryLockType lock(tuningLock);
            if (lock.isLocked()) {
                currentTuning = pendingTuning;
                tuningChanged.store(false);
            }
            // If lock failed, try again next block
        }
    }

    double getFrequency(int midiNote) const {
        return currentTuning.getFrequency(midiNote);
    }
};
```

### 5.6 Parameter Design for Tuning Controls

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createTuningParameters() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Master tuning reference (A4 = 440 Hz default)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TUNING_REF", 1 }, "Reference Pitch",
        juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f),
        440.0f, "Hz"));

    // Pitch bend range
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "BEND_RANGE", 1 }, "Bend Range",
        juce::NormalisableRange<float>(0.0f, 48.0f, 1.0f),
        2.0f, "st"));

    // Tuning mode selection
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "TUNING_MODE", 1 }, "Tuning Mode",
        juce::StringArray { "12-TET", "Scala File", "MTS-ESP", "Just Intonation" },
        0));  // Default to 12-TET

    // Scale degree fine-tuning (optional, for manual adjustments)
    for (int i = 0; i < 12; ++i) {
        juce::String paramId = "TUNE_" + juce::String(i);
        juce::String paramName = "Degree " + juce::String(i) + " Cents";

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { paramId, 1 }, paramName,
            juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
            0.0f, "cents"));
    }

    return layout;
}
```

---

## 6. Practical Code Patterns

### 6.1 Complete Tuning Table Class

```cpp
#pragma once

#include <JuceHeader.h>
#include <array>
#include <optional>

// Optional: Include Surge tuning library
#ifdef USE_SURGE_TUNING
#include "Tunings.h"
#endif

class MicrotonalTuningTable {
public:
    static constexpr int NUM_NOTES = 128;

    MicrotonalTuningTable() {
        initializeToEqual12TET();
    }

    //==========================================================================
    // Initialization Methods
    //==========================================================================

    void initializeToEqual12TET(double referenceHz = 440.0, int referenceNote = 69) {
        for (int note = 0; note < NUM_NOTES; ++note) {
            frequencies[note] = referenceHz *
                std::pow(2.0, (note - referenceNote) / 12.0);
            centsFromEqual[note] = 0.0;
        }
        this->referenceHz = referenceHz;
        this->referenceNote = referenceNote;
    }

    void initializeToEqualTemperament(int divisions, double periodRatio = 2.0,
                                       double referenceHz = 440.0, int referenceNote = 69) {
        for (int note = 0; note < NUM_NOTES; ++note) {
            int steps = note - referenceNote;
            frequencies[note] = referenceHz *
                std::pow(periodRatio, static_cast<double>(steps) / divisions);

            // Calculate cents offset from 12-TET
            double equal12Freq = referenceHz * std::pow(2.0, (note - referenceNote) / 12.0);
            centsFromEqual[note] = 1200.0 * std::log2(frequencies[note] / equal12Freq);
        }
        this->divisions = divisions;
        this->periodRatio = periodRatio;
    }

    void initializeToJustIntonation(const std::vector<std::pair<int, int>>& ratios,
                                     double referenceHz = 440.0, int referenceNote = 69) {
        int scaleSize = static_cast<int>(ratios.size());

        for (int note = 0; note < NUM_NOTES; ++note) {
            int diff = note - referenceNote;
            int octave = diff >= 0 ? diff / scaleSize : (diff - scaleSize + 1) / scaleSize;
            int degree = ((diff % scaleSize) + scaleSize) % scaleSize;

            double ratio = static_cast<double>(ratios[degree].first) / ratios[degree].second;
            frequencies[note] = referenceHz * ratio * std::pow(2.0, octave);

            double equal12Freq = referenceHz * std::pow(2.0, (note - referenceNote) / 12.0);
            centsFromEqual[note] = 1200.0 * std::log2(frequencies[note] / equal12Freq);
        }
    }

#ifdef USE_SURGE_TUNING
    bool loadScalaFile(const juce::File& sclFile,
                       const std::optional<juce::File>& kbmFile = std::nullopt) {
        try {
            auto scale = Tunings::readSCLFile(sclFile.getFullPathName().toStdString());

            Tunings::KeyboardMapping mapping;
            if (kbmFile && kbmFile->existsAsFile()) {
                mapping = Tunings::readKBMFile(kbmFile->getFullPathName().toStdString());
            }

            Tunings::Tuning tuning(scale, mapping);

            for (int note = 0; note < NUM_NOTES; ++note) {
                frequencies[note] = tuning.frequencyForMidiNote(note);
                centsFromEqual[note] = tuning.retuningFromEqualInCentsForMidiNote(note);
            }

            scaleName = scale.name;
            return true;
        }
        catch (const Tunings::TuningError& e) {
            juce::Logger::writeToLog("Tuning error: " + juce::String(e.what()));
            return false;
        }
    }
#endif

    //==========================================================================
    // Accessors
    //==========================================================================

    double getFrequency(int midiNote) const {
        return frequencies[juce::jlimit(0, 127, midiNote)];
    }

    double getCentsOffset(int midiNote) const {
        return centsFromEqual[juce::jlimit(0, 127, midiNote)];
    }

    // For pitch bend: get frequency with semitone offset
    double getFrequencyWithOffset(int midiNote, double semitoneOffset) const {
        return getFrequency(midiNote) * std::pow(2.0, semitoneOffset / 12.0);
    }

    // For glide/portamento: interpolate between two notes
    double getInterpolatedFrequency(int fromNote, int toNote, double position) const {
        double fromFreq = getFrequency(fromNote);
        double toFreq = getFrequency(toNote);

        // Exponential interpolation for perceptually linear pitch glide
        double fromLog = std::log2(fromFreq);
        double toLog = std::log2(toFreq);
        double interpLog = fromLog + (toLog - fromLog) * position;

        return std::pow(2.0, interpLog);
    }

    //==========================================================================
    // Modifiers
    //==========================================================================

    void setNoteFrequency(int midiNote, double frequency) {
        if (midiNote >= 0 && midiNote < NUM_NOTES) {
            frequencies[midiNote] = frequency;

            double equal12Freq = referenceHz *
                std::pow(2.0, (midiNote - referenceNote) / 12.0);
            centsFromEqual[midiNote] = 1200.0 * std::log2(frequency / equal12Freq);
        }
    }

    void setNoteCentsOffset(int midiNote, double cents) {
        if (midiNote >= 0 && midiNote < NUM_NOTES) {
            double equal12Freq = referenceHz *
                std::pow(2.0, (midiNote - referenceNote) / 12.0);
            frequencies[midiNote] = equal12Freq * std::pow(2.0, cents / 1200.0);
            centsFromEqual[midiNote] = cents;
        }
    }

    void setReferenceFrequency(double hz) {
        double ratio = hz / referenceHz;
        referenceHz = hz;

        for (int note = 0; note < NUM_NOTES; ++note) {
            frequencies[note] *= ratio;
        }
    }

private:
    std::array<double, NUM_NOTES> frequencies;
    std::array<double, NUM_NOTES> centsFromEqual;

    double referenceHz = 440.0;
    int referenceNote = 69;
    int divisions = 12;
    double periodRatio = 2.0;
    std::string scaleName = "12-TET";
};
```

### 6.2 Scala File Parser (Standalone)

If not using the Surge library, here's a minimal parser:

```cpp
class ScalaParser {
public:
    struct ParseResult {
        std::string description;
        std::vector<double> cents;  // All intervals in cents
        bool success = false;
        std::string errorMessage;
    };

    static ParseResult parse(const juce::String& sclContent) {
        ParseResult result;
        auto lines = juce::StringArray::fromLines(sclContent);

        int lineIndex = 0;

        // Skip comments (lines starting with !)
        while (lineIndex < lines.size() && lines[lineIndex].trimStart().startsWith("!")) {
            ++lineIndex;
        }

        // Description line
        if (lineIndex >= lines.size()) {
            result.errorMessage = "Missing description line";
            return result;
        }
        result.description = lines[lineIndex++].toStdString();

        // Skip comments
        while (lineIndex < lines.size() && lines[lineIndex].trimStart().startsWith("!")) {
            ++lineIndex;
        }

        // Number of notes
        if (lineIndex >= lines.size()) {
            result.errorMessage = "Missing note count";
            return result;
        }
        int noteCount = lines[lineIndex++].getIntValue();

        // Parse intervals
        result.cents.reserve(noteCount);

        while (result.cents.size() < noteCount && lineIndex < lines.size()) {
            auto line = lines[lineIndex++].trim();

            // Skip comments
            if (line.startsWith("!") || line.isEmpty()) continue;

            // Check for ratio (contains /) or cents (contains .)
            if (line.contains("/")) {
                // Ratio format: numerator/denominator
                auto parts = juce::StringArray::fromTokens(line, "/", "");
                if (parts.size() >= 2) {
                    double num = parts[0].getDoubleValue();
                    double den = parts[1].getDoubleValue();
                    if (den > 0) {
                        double cents = 1200.0 * std::log2(num / den);
                        result.cents.push_back(cents);
                    }
                }
            } else if (line.contains(".")) {
                // Cents format
                result.cents.push_back(line.getDoubleValue());
            } else {
                // Integer ratio (e.g., "2" = 2/1)
                double value = line.getDoubleValue();
                if (value > 0) {
                    double cents = 1200.0 * std::log2(value);
                    result.cents.push_back(cents);
                }
            }
        }

        result.success = (result.cents.size() == noteCount);
        if (!result.success) {
            result.errorMessage = "Expected " + std::to_string(noteCount) +
                " notes, found " + std::to_string(result.cents.size());
        }

        return result;
    }

    static std::vector<double> centsToFrequencies(const std::vector<double>& cents,
                                                   double baseFrequency = 261.63,
                                                   int baseNote = 60) {
        std::vector<double> frequencies(128);
        int scaleSize = static_cast<int>(cents.size());

        for (int note = 0; note < 128; ++note) {
            int diff = note - baseNote;
            int octave = diff >= 0 ? diff / scaleSize : (diff - scaleSize + 1) / scaleSize;
            int degree = ((diff % scaleSize) + scaleSize) % scaleSize;

            double degreeCents = (degree == 0) ? 0.0 : cents[degree - 1];
            double totalCents = degreeCents + (octave * cents.back());

            frequencies[note] = baseFrequency * std::pow(2.0, totalCents / 1200.0);
        }

        return frequencies;
    }
};
```

### 6.3 Frequency Calculation Helpers

```cpp
namespace MicrotonalUtils {

    // Standard conversions
    inline double midiToHz(double midiNote, double a4Hz = 440.0) {
        return a4Hz * std::pow(2.0, (midiNote - 69.0) / 12.0);
    }

    inline double hzToMidi(double frequency, double a4Hz = 440.0) {
        return 69.0 + 12.0 * std::log2(frequency / a4Hz);
    }

    inline double centsToRatio(double cents) {
        return std::pow(2.0, cents / 1200.0);
    }

    inline double ratioToCents(double ratio) {
        return 1200.0 * std::log2(ratio);
    }

    // Pitch bend conversions
    inline double pitchBendToSemitones(int bend14bit, double range = 2.0) {
        return ((bend14bit - 8192) / 8192.0) * range;
    }

    inline double pitchBendToCents(int bend14bit, double rangeSemitones = 2.0) {
        return pitchBendToSemitones(bend14bit, rangeSemitones) * 100.0;
    }

    inline int semitonesToPitchBend(double semitones, double range = 2.0) {
        double normalized = semitones / range;
        return static_cast<int>(normalized * 8192.0 + 8192.0);
    }

    // MPE pitch bend (48 semitone range standard)
    inline double mpePitchBendToSemitones(int bend14bit) {
        return pitchBendToSemitones(bend14bit, 48.0);
    }

    // Interval calculations
    inline double frequencyRatio(double freq1, double freq2) {
        return freq2 / freq1;
    }

    inline double centsInterval(double freq1, double freq2) {
        return 1200.0 * std::log2(freq2 / freq1);
    }

    // Just intonation helpers
    struct JustRatio {
        int numerator;
        int denominator;

        double toRatio() const {
            return static_cast<double>(numerator) / denominator;
        }
        double toCents() const {
            return ratioToCents(toRatio());
        }
    };

    inline const std::vector<JustRatio> JUST_MAJOR_SCALE = {
        {1, 1},   // Unison
        {9, 8},   // Major second
        {5, 4},   // Major third
        {4, 3},   // Perfect fourth
        {3, 2},   // Perfect fifth
        {5, 3},   // Major sixth
        {15, 8},  // Major seventh
        {2, 1}    // Octave
    };

    inline const std::vector<JustRatio> JUST_CHROMATIC = {
        {1, 1},     // C  (unison)
        {16, 15},   // C# (minor second)
        {9, 8},     // D  (major second)
        {6, 5},     // D# (minor third)
        {5, 4},     // E  (major third)
        {4, 3},     // F  (perfect fourth)
        {45, 32},   // F# (augmented fourth)
        {3, 2},     // G  (perfect fifth)
        {8, 5},     // G# (minor sixth)
        {5, 3},     // A  (major sixth)
        {9, 5},     // A# (minor seventh)
        {15, 8},    // B  (major seventh)
        {2, 1}      // C  (octave)
    };

} // namespace MicrotonalUtils
```

### 6.4 Voice Allocation Strategies

```cpp
class MicrotonalVoiceAllocator {
public:
    struct VoiceInfo {
        int voiceIndex;
        int midiNote;
        int midiChannel;
        double frequency;
        bool active;
    };

    // Standard allocation (newest note steals oldest)
    int allocateVoice(int midiNote, int midiChannel) {
        // First, try to find a free voice
        for (size_t i = 0; i < voices.size(); ++i) {
            if (!voices[i].active) {
                voices[i].midiNote = midiNote;
                voices[i].midiChannel = midiChannel;
                voices[i].active = true;
                return static_cast<int>(i);
            }
        }

        // No free voice, steal oldest
        int oldestIndex = 0;
        for (size_t i = 1; i < voices.size(); ++i) {
            // Implementation of age tracking would go here
        }

        return oldestIndex;
    }

    // MPE-aware allocation (respects per-channel voice assignment)
    int allocateMPEVoice(int midiNote, int midiChannel) {
        // In MPE, each channel typically has one note at a time
        for (size_t i = 0; i < voices.size(); ++i) {
            if (voices[i].midiChannel == midiChannel) {
                // Reuse voice on same channel
                voices[i].midiNote = midiNote;
                return static_cast<int>(i);
            }
        }

        // Otherwise, standard allocation
        return allocateVoice(midiNote, midiChannel);
    }

    // Microtonal-aware allocation (group by pitch region)
    int allocateByFrequencyRegion(int midiNote, double frequency) {
        // Prefer stealing voices in same frequency range (for smoother transitions)
        const double proximityThreshold = 200.0;  // Cents

        int closestVoice = -1;
        double closestDistance = std::numeric_limits<double>::max();

        for (size_t i = 0; i < voices.size(); ++i) {
            if (!voices[i].active) {
                return static_cast<int>(i);
            }

            double centsDistance = std::abs(
                MicrotonalUtils::centsInterval(voices[i].frequency, frequency));

            if (centsDistance < closestDistance && centsDistance < proximityThreshold) {
                closestDistance = centsDistance;
                closestVoice = static_cast<int>(i);
            }
        }

        if (closestVoice >= 0) {
            return closestVoice;
        }

        // Fall back to standard allocation
        return allocateVoice(midiNote, 1);
    }

private:
    std::vector<VoiceInfo> voices;
};
```

---

## 7. Integration Libraries

### 7.1 MTS-ESP (ODDSound)

**Repository:** https://github.com/ODDSound/MTS-ESP

**Integration Steps:**

1. Add `libMTSClient.h` and `libMTSClient.cpp` to your project
2. Register client in constructor, deregister in destructor
3. Query frequencies using the client API

```cpp
// In PluginProcessor.h
#include "libMTSClient.h"

class MyProcessor : public juce::AudioProcessor {
    MTSClient* mtsClient = nullptr;

public:
    MyProcessor() {
        mtsClient = MTS_RegisterClient();
    }

    ~MyProcessor() override {
        if (mtsClient) MTS_DeregisterClient(mtsClient);
    }

    // Pass to voices for frequency queries
    MTSClient* getMTSClient() { return mtsClient; }

    // Show connection status in UI
    bool hasMTSMaster() const {
        return mtsClient && MTS_HasMaster(mtsClient);
    }

    juce::String getMTSScaleName() const {
        if (mtsClient && MTS_HasMaster(mtsClient)) {
            return juce::String(MTS_GetScaleName(mtsClient));
        }
        return "Not connected";
    }
};

// In voice processing
double getFrequencyFromMTS(int midiNote, int midiChannel = -1) {
    if (mtsClient && MTS_HasMaster(mtsClient)) {
        // Check if note should be filtered (unmapped in scale)
        if (MTS_ShouldFilterNote(mtsClient, midiNote, midiChannel)) {
            return 0.0;  // Don't play this note
        }
        return MTS_NoteToFrequency(mtsClient, midiNote, midiChannel);
    }
    // Fallback
    return juce::MidiMessage::getMidiNoteInHertz(midiNote);
}
```

**MTS-ESP Complete Client API:**

```cpp
// Registration
MTSClient* MTS_RegisterClient();
void MTS_DeregisterClient(MTSClient* client);

// Connection status
bool MTS_HasMaster(MTSClient* client);

// Note filtering (for unmapped keys)
bool MTS_ShouldFilterNote(MTSClient* client, char midiNote, signed char midiChannel);

// Frequency queries (pick one format)
double MTS_NoteToFrequency(MTSClient* client, char midiNote, signed char midiChannel);
double MTS_RetuningInSemitones(MTSClient* client, char midiNote, signed char midiChannel);
double MTS_RetuningAsRatio(MTSClient* client, char midiNote, signed char midiChannel);

// Reverse lookup
char MTS_FrequencyToNote(MTSClient* client, double freq, signed char midiChannel);

// Scale information
const char* MTS_GetScaleName(MTSClient* client);
double MTS_GetPeriodRatio(MTSClient* client);  // Usually 2.0 (octave)

// MIDI SysEx parsing (for standalone MTS support)
void MTS_ParseMIDIDataU(MTSClient* client, const unsigned char* buffer, int len);
bool MTS_HasReceivedMTSSysEx(MTSClient* client);
```

### 7.2 Surge Tuning Library

**Repository:** https://github.com/surge-synthesizer/tuning-library

**Integration Steps:**

1. Add `Tunings.h` and `TuningsImpl.h` to your project (header-only)
2. Use `Tuning` class for Scala file parsing and frequency lookup

```cpp
#include "Tunings.h"

class SurgeTuningIntegration {
    Tunings::Tuning tuning;
    bool tuningLoaded = false;

public:
    bool loadScalaFiles(const juce::File& sclFile,
                        const juce::File& kbmFile = juce::File()) {
        try {
            auto scale = Tunings::readSCLFile(sclFile.getFullPathName().toStdString());

            if (kbmFile.existsAsFile()) {
                auto mapping = Tunings::readKBMFile(kbmFile.getFullPathName().toStdString());
                tuning = Tunings::Tuning(scale, mapping);
            } else {
                tuning = Tunings::Tuning(scale);
            }

            tuningLoaded = true;
            return true;
        }
        catch (const Tunings::TuningError& e) {
            juce::Logger::writeToLog("Tuning error: " + juce::String(e.what()));
            return false;
        }
    }

    // Create equal temperament
    void setEqualTemperament(int divisions) {
        auto scale = Tunings::evenDivisionOfSpanByM(2, divisions);
        tuning = Tunings::Tuning(scale);
        tuningLoaded = true;
    }

    // Frequency lookup
    double getFrequency(int midiNote) const {
        if (!tuningLoaded) {
            return juce::MidiMessage::getMidiNoteInHertz(midiNote);
        }
        return tuning.frequencyForMidiNote(midiNote);
    }

    // Get cents offset from 12-TET
    double getCentsOffset(int midiNote) const {
        if (!tuningLoaded) return 0.0;
        return tuning.retuningFromEqualInCentsForMidiNote(midiNote);
    }

    // Check if note is mapped
    bool isNoteMapped(int midiNote) const {
        if (!tuningLoaded) return true;
        return tuning.isMidiNoteMapped(midiNote);
    }

    // Get scale name
    juce::String getScaleName() const {
        if (!tuningLoaded) return "12-TET";
        return juce::String(tuning.scale.name);
    }

    // Get scale size
    int getScaleSize() const {
        if (!tuningLoaded) return 12;
        return tuning.scale.count;
    }
};
```

**Surge Tuning Library Key Classes:**

```cpp
namespace Tunings {
    // Scale (from .scl file)
    struct Scale {
        std::string name;
        std::string description;
        int count;                    // Number of notes per period
        std::vector<Tone> tones;      // Individual scale degrees
    };

    // Keyboard Mapping (from .kbm file)
    struct KeyboardMapping {
        int count;
        int firstMidi, lastMidi;      // MIDI note range
        int middleNote;               // Reference note
        int tuningConstantNote;       // Note for reference frequency
        double tuningFrequency;       // Reference frequency (Hz)
        std::vector<int> keys;        // Degree mapping per MIDI note
    };

    // Main class
    class Tuning {
        double frequencyForMidiNote(int midiNote) const;
        double retuningFromEqualInCentsForMidiNote(int midiNote) const;
        int scalePositionForMidiNote(int midiNote) const;
        bool isMidiNoteMapped(int midiNote) const;
    };

    // File I/O
    Scale readSCLFile(const std::string& path);
    Scale parseSCLData(const std::string& content);
    KeyboardMapping readKBMFile(const std::string& path);
    KeyboardMapping parseKBMData(const std::string& content);

    // Generators
    Scale evenDivisionOfSpanByM(double span, int divisions);  // e.g., 19-EDO
    Scale evenTemperament12NoteScale();
}
```

---

## 8. Common Pitfalls and Solutions

### 8.1 Pitch Accuracy Issues

**Problem:** Notes sound out of tune even with correct tuning table.

**Causes & Solutions:**

```cpp
// PITFALL 1: Integer delay line (physical modeling)
// BAD: Using integer delay
int delaySamples = static_cast<int>(sampleRate / frequency);

// GOOD: Use fractional delay with interpolation
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine;
float delaySamples = static_cast<float>(sampleRate / frequency);
output = delayLine.popSample(0, delaySamples);

// PITFALL 2: Sample rate mismatch
// BAD: Hardcoded sample rate
double freq = 440.0 * std::pow(2.0, (note - 69) / 12.0);
phase += freq / 44100.0;  // Wrong if sample rate is different!

// GOOD: Always use actual sample rate
void prepareToPlay(double sampleRate, int samplesPerBlock) {
    this->sampleRate = sampleRate;  // Store it
}
phase += freq / sampleRate;  // Correct

// PITFALL 3: Pitch bend not applied to microtonal base
// BAD: Applying bend to 12-TET and then adding offset
double freq12tet = midiToHz(note) * pitchBendMultiplier;
double freq = freq12tet * microtonalRatio;  // Wrong order!

// GOOD: Apply bend to microtonal frequency directly
double freqMicrotonal = tuningTable.getFrequency(note);
double freq = freqMicrotonal * pitchBendMultiplier;
```

### 8.2 Parameter Smoothing for Real-Time Tuning Changes

**Problem:** Clicks when tuning changes during playback.

**Solution:**

```cpp
class SmoothTuningVoice {
    juce::SmoothedValue<float> smoothedFrequency;

    void prepareToPlay(double sampleRate, int samplesPerBlock) {
        smoothedFrequency.reset(sampleRate, 0.01);  // 10ms smoothing
    }

    void setFrequency(double targetFreq) {
        smoothedFrequency.setTargetValue(static_cast<float>(targetFreq));
    }

    void process(float* buffer, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            float currentFreq = smoothedFrequency.getNextValue();
            oscillator.setFrequency(currentFreq);
            buffer[i] = oscillator.process();
        }
    }
};
```

### 8.3 MPE + Microtonality Conflicts

**Problem:** MPE pitch bend conflicts with microtonal tuning.

**Solution:**

```cpp
class MPEMicrotonalVoice : public juce::MPESynthesiserVoice {
    enum class TuningMode {
        MPE_PITCH_BEND,        // Use MPE pitch bend only
        MICROTONAL_TABLE,      // Use tuning table only
        COMBINED               // MPE bend relative to microtonal base
    };

    TuningMode mode = TuningMode::COMBINED;

    void updateFrequency() {
        auto note = getCurrentlyPlayingNote();

        switch (mode) {
            case TuningMode::MPE_PITCH_BEND:
                // Standard MPE behavior (12-TET + pitch bend)
                frequency = note.getFrequencyInHertz();
                break;

            case TuningMode::MICROTONAL_TABLE:
                // Ignore MPE pitch bend, use table only
                frequency = tuningTable->getFrequency(note.initialNote);
                break;

            case TuningMode::COMBINED:
                // Apply MPE bend relative to microtonal base
                double baseFreq = tuningTable->getFrequency(note.initialNote);
                double bendRatio = note.getFrequencyInHertz() /
                    juce::MidiMessage::getMidiNoteInHertz(note.initialNote);
                frequency = baseFreq * bendRatio;
                break;
        }
    }
};
```

### 8.4 Filter Tracking with Microtonal Pitch

**Problem:** Filter keyboard tracking produces wrong cutoff for microtonal notes.

**Solution:**

```cpp
void updateFilterCutoff(int midiNote, double actualFrequency, double keyboardTracking) {
    // DON'T use MIDI note directly
    // BAD: double offset = midiNote - 60;

    // Calculate offset based on actual frequency
    double middleCFreq = tuningTable->getFrequency(60);  // C4 in current tuning
    double semitoneOffset = 12.0 * std::log2(actualFrequency / middleCFreq);

    double trackingMultiplier = std::pow(2.0, (semitoneOffset * keyboardTracking) / 12.0);
    double cutoff = baseCutoff * trackingMultiplier;

    filter.setCutoff(cutoff);
}
```

### 8.5 Scala File Edge Cases

**Problem:** Some Scala files cause parsing errors or unexpected results.

**Solution:**

```cpp
class RobustScalaLoader {
    bool load(const juce::String& content) {
        try {
            // Handle different line endings
            auto normalized = content.replace("\r\n", "\n").replace("\r", "\n");

            auto result = ScalaParser::parse(normalized);

            if (!result.success) {
                DBG("Parse error: " << result.errorMessage);
                return false;
            }

            // Validate cents values
            for (double cents : result.cents) {
                if (std::isnan(cents) || std::isinf(cents)) {
                    DBG("Invalid cents value detected");
                    return false;
                }

                // Warn about unusual values
                if (cents < -1200 || cents > 4800) {
                    DBG("Warning: Unusual cents value: " << cents);
                }
            }

            // Check octave (last value should be close to 1200 for most scales)
            double octave = result.cents.back();
            if (std::abs(octave - 1200.0) > 1.0 &&
                std::abs(octave - 2400.0) > 1.0) {
                DBG("Warning: Non-standard octave: " << octave << " cents");
            }

            applyTuning(result);
            return true;
        }
        catch (...) {
            DBG("Exception during Scala file parsing");
            return false;
        }
    }
};
```

### 8.6 Performance with Large Tuning Tables

**Problem:** Per-sample tuning lookups are slow.

**Solution:**

```cpp
class OptimizedTuningVoice {
    // Cache frequency at note-on
    double cachedFrequency = 440.0;
    double cachedPitchBendMultiplier = 1.0;

    void startNote(int midiNote, float velocity, ...) {
        // Lookup once at note-on
        cachedFrequency = tuningTable->getFrequency(midiNote);
    }

    void pitchWheelMoved(int newValue) {
        // Calculate multiplier once per pitch bend change
        cachedPitchBendMultiplier = std::pow(2.0,
            ((newValue - 8192) / 8192.0) * bendRange / 12.0);
    }

    void renderNextBlock(...) {
        // Use cached values in audio loop
        double frequency = cachedFrequency * cachedPitchBendMultiplier;
        oscillator.setFrequency(frequency);

        // Only re-check tuning if using continuous MTS-ESP
        if (useContinuousTuning && (frameCount % 64 == 0)) {
            cachedFrequency = tuningTable->getFrequency(currentNote);
        }
    }
};
```

---

## 9. References

### Primary Sources

- **MTS-ESP Library:** https://github.com/ODDSound/MTS-ESP
- **Surge Tuning Library:** https://github.com/surge-synthesizer/tuning-library
- **Scala Scale Format:** https://www.huygens-fokker.org/scala/scl_format.html
- **JUCE MPE Documentation:** https://docs.juce.com/master/classMPEInstrument.html
- **JUCE MPE Tutorial:** https://juce.com/tutorials/tutorial_mpe_introduction/

### Open Source Implementations

- **Surge XT:** https://surge-synthesizer.github.io/
- **Vital:** https://vital.audio/
- **Dexed:** https://asb2m10.github.io/dexed/
- **mda-synths-mts-esp:** https://github.com/eventual-recluse/mda-synths-mts-esp

### Community Resources

- **JUCE Forum - Microtonality:** https://forum.juce.com/t/microtonality/46130
- **Xenharmonic Wiki:** https://en.xen.wiki/w/List_of_Microtonal_Software_Plugins
- **KVR DSP Forum:** https://www.kvraudio.com/forum/viewforum.php?f=33

### Academic References

- **Smith, J.O.** - "Physical Audio Signal Processing" (online book)
  https://ccrma.stanford.edu/~jos/pasp/
- **Valimaki, V.** - "Fractional Delay Filters" publications
  http://users.spa.aalto.fi/vpv/publications/

### Pitch Shifting Algorithms

- **PSOLA:** Time-domain pitch-synchronous overlap-add
- **Phase Vocoder:** Frequency-domain pitch shifting
- **Granular Synthesis:** Grain-based pitch manipulation

---

## Appendix A: Quick Reference Card

### Frequency Conversions

```cpp
// MIDI to Hz (12-TET)
hz = 440 * pow(2, (midi - 69) / 12.0);

// Hz to MIDI
midi = 69 + 12 * log2(hz / 440);

// Cents to ratio
ratio = pow(2, cents / 1200.0);

// Ratio to cents
cents = 1200 * log2(ratio);

// Pitch bend to semitones (14-bit, +/- range)
semitones = ((bend - 8192) / 8192.0) * range;
```

### Common Scale Sizes

| Temperament | Notes/Octave | Example Cents |
|-------------|--------------|---------------|
| 12-TET | 12 | 0, 100, 200... 1200 |
| 19-TET | 19 | 0, 63.16, 126.32... |
| 31-TET | 31 | 0, 38.71, 77.42... |
| 53-TET | 53 | 0, 22.64, 45.28... |
| Just Major | 7+1 | 0, 204, 386, 498, 702, 884, 1088, 1200 |

### Just Intonation Common Ratios

| Interval | Ratio | Cents |
|----------|-------|-------|
| Unison | 1/1 | 0 |
| Minor Second | 16/15 | 112 |
| Major Second | 9/8 | 204 |
| Minor Third | 6/5 | 316 |
| Major Third | 5/4 | 386 |
| Perfect Fourth | 4/3 | 498 |
| Tritone | 45/32 | 590 |
| Perfect Fifth | 3/2 | 702 |
| Minor Sixth | 8/5 | 814 |
| Major Sixth | 5/3 | 884 |
| Minor Seventh | 9/5 | 1018 |
| Major Seventh | 15/8 | 1088 |
| Octave | 2/1 | 1200 |

### JUCE DSP Classes for Microtonality

```cpp
// Fractional delay (use for physical modeling)
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>

// Parameter smoothing (prevent clicks on tuning changes)
juce::SmoothedValue<float>

// MPE support
juce::MPESynthesiser
juce::MPESynthesiserVoice
juce::MPEInstrument

// Standard synthesizer (can be extended for microtonality)
juce::Synthesiser
juce::SynthesiserVoice
```

---

**Document Version:** 1.0
**Last Updated:** 2026-01-09
**Author:** Research Agent
