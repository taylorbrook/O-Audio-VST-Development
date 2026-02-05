---
title: "Comprehensive Reverb Research"
summary: "Complete technical reference for reverb algorithm design and JUCE implementation covering Schroeder, Freeverb, FDN, and Dattorro architectures, convolution reverb theory, and spatialization techniques."
domain: dsp
type: guide
keywords:
  - reverb
  - algorithmic-reverb
  - convolution
  - fdn
  - dattorro
  - schroeder
  - freeverb
  - spatialization
  - juce
stages: [0, 1, 2]
agents: [dsp, research]
---

# Comprehensive Reverb Research Document

**Version:** 1.0
**Date:** 2026-01-13
**Author:** Plugin Freedom System Research
**Purpose:** Complete technical reference for reverb algorithm design and implementation in audio plugins

**Scope:**
- Algorithmic reverb architectures (Schroeder, Freeverb, FDN, Dattorro)
- Convolution reverb theory and implementation
- JUCE framework integration
- Professional plugin analysis and design patterns
- Sound design applications (film, games, creative)
- Spatialization (mono, stereo, binaural, surround, immersive 3D)

**Document Statistics:**
- 11 major sections + 3 appendices
- ~1,500 lines of technical content
- Code examples in C++/JUCE
- Reference tables for quick lookup

---

## Table of Contents

1. [Fundamentals of Reverberation](#1-fundamentals-of-reverberation)
2. [Algorithmic Reverb Architectures](#2-algorithmic-reverb-architectures)
3. [Core DSP Building Blocks](#3-core-dsp-building-blocks)
4. [Convolution Reverb](#4-convolution-reverb)
5. [Professional Reverb Analysis](#5-professional-reverb-analysis)
6. [JUCE Implementation](#6-juce-implementation)
7. [Parameter Design](#7-parameter-design)
8. [Optimization Techniques](#8-optimization-techniques)
9. [Reverb in Sound Design](#9-reverb-in-sound-design)
10. [Reverb and Spatialization](#10-reverb-and-spatialization)
11. [References and Resources](#11-references-and-resources)

---

## 1. Fundamentals of Reverberation

### 1.1 Natural Reverberation Components

Natural reverberation consists of three distinct temporal regions:

| Component | Timing | Characteristics | Perceptual Role |
|-----------|--------|-----------------|-----------------|
| **Direct Sound** | 0ms | Original signal | Localization, timbre |
| **Early Reflections** | 0-100ms | Sparse, discrete | Room size, shape, distance |
| **Late Reverberation** | 100ms+ | Dense, diffuse | Room character, decay |

### 1.2 Perceptual Thresholds

- **Echo Threshold:** ~50ms - reflections beyond this become audible as discrete echoes
- **Fusion Threshold:** ~80ms - reflections blend into continuous "wash"
- **Target Echo Density:** ~1000 echoes/second for natural-sounding late reverb
- **RT60:** Time for reverb to decay 60dB - primary decay measurement

### 1.3 Early vs Late Reflections

**Early Reflections (0-100ms):**
- Typically 15-100ms after direct sound
- Provide spatial impression (room shape perception)
- Affect perceived source distance
- Create stereo width and depth
- Implemented via tapped delay lines (TDL)

**Late Reverberation (100ms+):**
- Statistically dense (exponentially decaying noise)
- Frequency-dependent decay (HF decays faster)
- Provides "wash" and sustain character
- Implemented via recursive filter networks

---

## 2. Algorithmic Reverb Architectures

### 2.1 Schroeder Reverberator (1962)

The foundational digital reverb architecture by Manfred Schroeder.

**Architecture:** 4 parallel comb filters → 2 series allpass filters

**Comb Filter Transfer Function:**
```
H(z) = 1 / (1 - g*z^(-N))
```
Where:
- N = delay length (samples)
- g = feedback coefficient (|g| < 1 for stability)

**Allpass Filter Transfer Function:**
```
H(z) = (-g + z^(-N)) / (1 - g*z^(-N))
```

**Design Rules:**
- Delay lengths should be mutually prime (no common factors)
- Open-loop gain should not exceed 0.85 (-1.4 dB)
- Allpass delays follow progression: M_i*T ≈ 100ms / 3^i
- With g=0.708, yields ~810 echoes/second

**Schroeder Allpass Coefficients:**
- For flat magnitude response: g = (√5-1)/2 ≈ 0.618 (golden ratio)

**Mixing Matrix (4x4):**
```
[  1   1   1   1 ]
[ -1  -1  -1  -1 ]
[ -1   1  -1   1 ]
[  1  -1   1  -1 ]
```

**Limitations:**
- Metallic/ringing artifacts
- Echo density doesn't grow over time (unlike real rooms)
- Requires careful tuning to avoid coloration

### 2.2 Freeverb (Jezar's Algorithm)

Public domain Schroeder-Moorer reverb with optimized parameters.

**Architecture:**
- 8 parallel lowpass-feedback comb filters per channel
- 4 series allpass filters per channel
- Stereo spread via delay line offset (+23 samples for right channel)

**Comb Filter Delay Lengths (samples @ 44.1kHz):**
```cpp
1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
```

**Allpass Filter Delay Lengths (samples):**
```cpp
225, 341, 441, 556
```

**Key Constants:**
```cpp
fixedGain = 0.015
scaleWet = 3.0
scaleDry = 2.0
scaleDamp = 0.4
scaleRoom = 0.28
offsetRoom = 0.7
```

**Lowpass-Feedback Comb Filter:**
```cpp
y[n] = x[n-N] + g * (y[n-N] * (1-damp) + prev * damp)
prev = y[n-N]
```

**Why Freeverb Sounds Good:**
- Exhaustive listening tests for delay length optimization
- Lowpass filtering in comb feedback for natural HF damping
- Stereo spreading without obvious delay artifacts

### 2.3 Moorer Reverberator (1979)

Enhanced Schroeder design with explicit early reflections.

**Architecture:**
1. FIR filter (10-20 taps) for early reflections
2. Parallel comb filters with lowpass in feedback
3. Series allpass filters

**Improvements Over Schroeder:**
- Explicit early reflection simulation
- High-frequency damping via lowpass filters
- More efficient 2-multiply allpass design

### 2.4 Feedback Delay Networks (FDN)

Modern architecture introduced by Gerzon (1971) and Stautner/Puckette (1982).

**Core Concept:** Multiple delays cross-coupled via feedback matrix

**Architecture:**
```
Input → Delay Lines → Feedback Matrix → Output
            ↑__________________|
```

**Lossless Prototype Design:**
1. Start with infinite RT (lossless case)
2. Optimize for smooth "noise-like" impulse response
3. Add frequency-dependent damping for desired RT

**Feedback Matrix Options:**

| Matrix Type | Properties | Use Case |
|-------------|------------|----------|
| **Hadamard** | Orthogonal, efficient FFT-like computation | General purpose |
| **Householder** | N-1 identity + reflection | Simple, effective |
| **Random Orthogonal** | Maximum diffusion | Complex spaces |

**Hadamard Matrix (4x4 normalized):**
```
H_4 = 0.5 * [ 1  1  1  1 ]
            [ 1 -1  1 -1 ]
            [ 1  1 -1 -1 ]
            [ 1 -1 -1  1 ]
```

**Delay Line Design:**
- Lengths should be mutually prime (coprime)
- Avoid low-order dependencies (linear combinations)
- Even distribution across desired time range

**Stability Condition:**
- Feedback matrix must have eigenvalues with magnitude ≤ 1
- Orthogonal/unitary matrices guarantee stability

### 2.5 Dattorro Plate Reverb (1997)

Allpass loop reverb with comprehensive specifications - the "Rosetta Stone" of reverb design.

**Architecture:**
1. **Input Section:** Pre-delay → Bandwidth limiting → Input diffusion
2. **Tank Section:** Figure-8 allpass loop with modulation
3. **Output Section:** Multiple taps for stereo decorrelation

**Input Diffusion (4 cascaded allpasses):**
- Smears input signal before entering tank
- Controls density of early reflections
- Coefficients typically 0.75, 0.75, 0.625, 0.625

**Tank Topology (Figure-8):**
```
┌──[AP1]──[Delay1]──[LP1]──[AP2]──[Delay2]──┐
│                                            │
└────────────────←feedback←─────────────────┘
```

**Key Features:**
- Modulated delay lines (excursion parameter)
- Decay diffusion via nested allpasses
- High-frequency damping in feedback loop
- Input bandwidth limiting

**Why Dattorro Works:**
- Complete specification (all delay lengths, coefficients)
- Smallest memory footprint for good quality
- Simple parameter mapping (size, decay, damping, width)
- Modulation prevents metallic coloration

---

## 3. Core DSP Building Blocks

### 3.1 Delay Lines

**Basic Circular Buffer:**
```cpp
class DelayLine {
    std::vector<float> buffer;
    int writeIndex = 0;

    void push(float sample) {
        buffer[writeIndex] = sample;
        writeIndex = (writeIndex + 1) % buffer.size();
    }

    float read(int delaySamples) {
        int readIndex = writeIndex - delaySamples;
        if (readIndex < 0) readIndex += buffer.size();
        return buffer[readIndex];
    }
};
```

**Interpolated Delay (for modulation):**
```cpp
float readInterpolated(float delaySamples) {
    int index1 = (int)delaySamples;
    int index2 = index1 + 1;
    float frac = delaySamples - index1;

    // Linear interpolation
    return read(index1) * (1.0f - frac) + read(index2) * frac;

    // Or use cubic/Lagrange for better quality
}
```

### 3.2 Comb Filters

**Feedback Comb Filter (FIR):**
```cpp
y[n] = x[n] + g * y[n-N]
```

**Feedforward Comb Filter (IIR):**
```cpp
y[n] = x[n] + g * x[n-N]
```

**Lowpass-Feedback Comb Filter (LBCF):**
```cpp
class LBCombFilter {
    float damp = 0.5f;
    float filterStore = 0.0f;

    float process(float input) {
        float output = delayLine.read(delayLength);
        filterStore = output * (1.0f - damp) + filterStore * damp;
        delayLine.push(input + filterStore * feedback);
        return output;
    }
};
```

### 3.3 Allpass Filters

**Schroeder Allpass:**
```cpp
class SchroederAllpass {
    float g;  // coefficient (typically 0.5-0.7)

    float process(float input) {
        float delayed = delayLine.read(delayLength);
        float output = -g * input + delayed;
        delayLine.push(input + g * delayed);
        return output;
    }
};
```

**Properties:**
- Magnitude response is flat (unity gain at all frequencies)
- Delays signal while preserving spectral content
- Phase response varies with frequency
- Acts as "impulse diffuser" in series configuration

### 3.4 Diffusion Networks

**Purpose:** Increase echo density without changing spectral content

**Allpass Cascade:**
```cpp
class DiffusionNetwork {
    std::array<SchroederAllpass, 4> allpasses;

    float process(float input) {
        float signal = input;
        for (auto& ap : allpasses) {
            signal = ap.process(signal);
        }
        return signal;
    }
};
```

**Design Guidelines:**
- 4-5 allpasses typically sufficient
- Delay lengths decrease geometrically (100ms, 33ms, 11ms...)
- Coefficients around 0.5-0.7

### 3.5 Modulation

**Purpose:** Prevent metallic coloration in long reverb tails

**LFO-Modulated Delay:**
```cpp
void process(float& sample) {
    // Slow random modulation (0.5-2 Hz)
    float modulation = lfo.getValue() * excursionSamples;
    float delaySamples = baseDelay + modulation;

    sample = delayLine.readInterpolated(delaySamples);
}
```

**Modulation Types:**
- **Chorus-like:** Subtle pitch variation (±0.1-1%)
- **Soft-focus:** Slow random modulation (0.5-2 Hz)
- **Shimmer:** Pitch-shifting in feedback loop

---

## 4. Convolution Reverb

### 4.1 Theory

**Impulse Response (IR):** Complete acoustic signature of a space

**Convolution Mathematics:**
```
y[n] = (x * h)[n] = Σ x[k] * h[n-k]
```
Where:
- x = input signal
- h = impulse response
- y = output (reverberated signal)

**Time Domain Complexity:** O(N²) - impractical for long IRs

### 4.2 FFT-Based Convolution

**Frequency Domain Equivalence:**
```
Y(f) = X(f) * H(f)
```
Multiplication in frequency domain = convolution in time domain

**Complexity:** O(N log N) via FFT

**Overlap-Add Method:**
1. Divide input into blocks of size L
2. Zero-pad to length L + M - 1 (M = IR length)
3. FFT both blocks
4. Multiply in frequency domain
5. IFFT result
6. Overlap-add successive blocks

**Overlap-Save Method:**
- Similar but discards wrapped portion instead of adding

### 4.3 Partitioned Convolution

**Problem:** Single large FFT causes unacceptable latency

**Solution:** Partition IR into smaller blocks

**Uniform Partitioning:**
- All partitions same size
- Each partition processed independently
- Results summed with appropriate delays

**Non-Uniform Partitioning:**
- Small blocks for early IR (low latency)
- Large blocks for later IR (efficiency)
- Typical: 64 samples direct, then 256, 1024, 4096...

**Zero-Latency Convolution:**
```cpp
// First few samples: direct time-domain convolution
for (int i = 0; i < smallBlockSize; i++) {
    output[i] = directConvolve(input, ir, i);
}

// Remainder: partitioned FFT convolution
output += partitionedConvolve(input, ir);
```

### 4.4 Impulse Response Management

**Capture Methods:**
- Sine sweep + deconvolution
- Balloon pop / starter pistol
- Speaker + microphone measurement

**True Stereo Configurations:**
| Type | Channels | Description |
|------|----------|-------------|
| Mono | 1 | Single IR, mono output |
| Stereo | 2 | L/R IRs, maintains input panning |
| True Stereo | 4 | L→L, L→R, R→L, R→R for full spatial capture |

**Processing Considerations:**
- Trim silence before impulse
- Fade tail to noise floor
- Normalize peak/RMS levels
- Store at session sample rate or resample on load

### 4.5 Hybrid Approaches

**Early Reflections (Algorithmic) + Late Reverb (Convolution):**
- Algorithmic for controllable early reflections
- Convolution for authentic late tail
- Crossfade at 50-100ms mark

**Real-Time IR Modulation:**
- Divide IR into early/late halves
- Modulate each independently
- Creates "living" convolution reverb

---

## 5. Professional Reverb Analysis

### 5.1 Legendary Hardware

| Unit | Type | Character | Notable Features |
|------|------|-----------|------------------|
| **Lexicon 224** | Algorithmic | Lush, musical | Plates, halls, invented "Lexicon sound" |
| **Lexicon 480L** | Algorithmic | Pristine, versatile | Industry standard 1987-2010s |
| **EMT 140** | Physical Plate | Warm, dense | Steel plate, 4-8 sec decay |
| **EMT 250** | Digital | Smooth, dark | First digital reverb (1976) |
| **AMS RMX16** | Digital | Punchy, tight | Nonlin, Ambience presets |
| **TC Electronic M6000** | Algorithmic | Clean, modern | VSS technology |
| **Sony DRE-2000** | Digital | Transparent | Concert hall simulation |
| **Bricasti M7** | Algorithmic | Natural, 3D | Modern reference standard |

### 5.2 Top Software Reverbs

**Valhalla DSP:**
- **VintageVerb:** 1970s-80s character (Lexicon, EMT emulation)
- **Room:** Small to medium spaces, tight
- **Shimmer:** Pitch-shifted feedback, ethereal
- **Supermassive:** Massive delays, drones (free)

**FabFilter Pro-R:**
- Smooth, transparent algorithm
- Innovative decay rate visualization
- Per-band decay control
- Modern, clean sound

**Eventide Blackhole:**
- Otherworldly infinite reverbs
- Reverse, freeze modes
- Modulation capabilities
- Excellent for sound design

**LiquidSonics Seventh Heaven:**
- Bricasti M7 emulation
- True stereo IRs
- Fusion-IR technology (modulated capture)
- Professional reference quality

### 5.3 Design Philosophies

**Natural/Realistic:**
- Accurate room simulation
- Physically plausible parameters
- Example: Altiverb, Bricasti M7

**Musical/Colored:**
- Pleasant artifacts encouraged
- Vintage character
- Example: Lexicon 224, Valhalla VintageVerb

**Creative/Experimental:**
- Extreme parameters
- Non-physical effects
- Example: Eventide Blackhole, Soundtoys Little Plate

### 5.4 What Makes Professional Reverb Sound Good

1. **Smooth Tail Decay:** No audible resonances or "pumping"
2. **Proper Diffusion:** Dense enough to avoid flutter, not so dense it's muddy
3. **Natural HF Damping:** High frequencies decay faster (air absorption)
4. **Subtle Modulation:** Prevents metallic coloration without obvious pitch artifacts
5. **Early Reflection Quality:** Defines perceived room character
6. **Stereo Coherence:** Mono-compatible, no phase issues
7. **Clean Transient Response:** Doesn't smear attack of source

---

## 6. JUCE Implementation

### 6.1 juce::dsp::Reverb

JUCE provides a built-in Freeverb implementation.

**Header Includes:**
```cpp
#include <juce_dsp/juce_dsp.h>
```

**Declaration:**
```cpp
juce::dsp::Reverb reverb;
```

**Preparation (prepareToPlay):**
```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    reverb.prepare(spec);
    reverb.reset();
}
```

**Parameter Configuration:**
```cpp
juce::Reverb::Parameters reverbParams;
reverbParams.roomSize = 0.5f;      // 0.0 - 1.0
reverbParams.damping = 0.5f;       // 0.0 - 1.0 (HF absorption)
reverbParams.wetLevel = 0.33f;     // 0.0 - 1.0
reverbParams.dryLevel = 0.4f;      // 0.0 - 1.0
reverbParams.width = 1.0f;         // 0.0 - 1.0 (stereo spread)
reverbParams.freezeMode = 0.0f;    // 0.0 or 1.0

reverb.setParameters(reverbParams);
```

**Processing (processBlock):**
```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);
}
```

### 6.2 juce::dsp::Convolution

**Declaration:**
```cpp
juce::dsp::Convolution convolution;
```

**Loading IR:**
```cpp
// From file
convolution.loadImpulseResponse(
    irFile,
    juce::dsp::Convolution::Stereo::yes,
    juce::dsp::Convolution::Trim::yes,
    0  // size (0 = use original)
);

// From binary data
convolution.loadImpulseResponse(
    BinaryData::hall_wav,
    BinaryData::hall_wavSize,
    juce::dsp::Convolution::Stereo::yes,
    juce::dsp::Convolution::Trim::yes,
    0
);
```

**Processing:**
```cpp
juce::dsp::AudioBlock<float> block(buffer);
juce::dsp::ProcessContextReplacing<float> context(block);
convolution.process(context);
```

### 6.3 DryWetMixer Integration

```cpp
juce::dsp::DryWetMixer<float> dryWetMixer;

void prepareToPlay(double sampleRate, int samplesPerBlock) {
    // ... prepare spec ...
    dryWetMixer.prepare(spec);
    dryWetMixer.reset();
}

void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::dsp::AudioBlock<float> block(buffer);

    // Push dry samples first
    dryWetMixer.pushDrySamples(block);

    // Process reverb (modifies buffer in-place)
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);

    // Mix dry/wet
    dryWetMixer.setWetMixProportion(mixValue);  // 0.0 - 1.0
    dryWetMixer.mixWetSamples(block);
}
```

### 6.4 Custom Reverb Implementation Pattern

```cpp
class CustomReverb {
public:
    void prepare(const juce::dsp::ProcessSpec& spec) {
        sampleRate = spec.sampleRate;

        // Initialize delay lines
        for (auto& comb : combFilters) {
            comb.prepare(spec);
        }
        for (auto& ap : allpassFilters) {
            ap.prepare(spec);
        }

        // Set delay lengths (scale by sample rate)
        // ...
    }

    void process(juce::dsp::ProcessContextReplacing<float>& context) {
        auto& block = context.getOutputBlock();

        for (size_t sample = 0; sample < block.getNumSamples(); ++sample) {
            // Per-sample processing
            float inputL = block.getSample(0, sample);
            float inputR = block.getSample(1, sample);

            // Diffusion
            float diffusedL = processDiffusion(inputL);
            float diffusedR = processDiffusion(inputR);

            // Comb filters
            float combSum = 0.0f;
            for (auto& comb : combFilters) {
                combSum += comb.process(diffusedL);
            }

            // Allpass chain
            float output = combSum;
            for (auto& ap : allpassFilters) {
                output = ap.process(output);
            }

            block.setSample(0, sample, output);
            block.setSample(1, sample, output);
        }
    }

private:
    double sampleRate;
    std::array<LBCombFilter, 8> combFilters;
    std::array<SchroederAllpass, 4> allpassFilters;
};
```

---

## 7. Parameter Design

### 7.1 Standard Reverb Parameters

| Parameter | Range | Description | Implementation |
|-----------|-------|-------------|----------------|
| **Room Size** | 0-100% | Perceived space size | Delay line lengths |
| **Decay/RT60** | 0.1-20s | Reverberation time | Feedback coefficients |
| **Pre-delay** | 0-200ms | Time before reverb onset | Input delay line |
| **Damping** | 0-100% | HF absorption | Lowpass in feedback |
| **Diffusion** | 0-100% | Echo density | Allpass coefficients |
| **Width** | 0-100% | Stereo spread | Cross-channel mixing |
| **Mix** | 0-100% | Dry/wet balance | Output mixing |
| **Early/Late** | -∞ to +6dB | Balance of ER vs tail | Separate level controls |

### 7.2 Parameter Mapping Formulas

**Decay Time to Feedback Coefficient:**
```cpp
// g = feedback coefficient
// RT60 = desired decay time (seconds)
// N = delay length (samples)
// fs = sample rate

g = pow(10.0, (-3.0 * N) / (RT60 * fs));
// Or simplified: g = 0.001^(N / (RT60 * fs))
```

**Room Size to Delay Scaling:**
```cpp
// Scale all delay lines proportionally
float scale = 0.5f + roomSize * 0.5f;  // 50% - 100% of base lengths
int scaledDelay = (int)(baseDelay * scale);
```

**Damping to Lowpass Coefficient:**
```cpp
// damp = 0.0 (no damping) to 1.0 (heavy damping)
float lpCoeff = damp;  // Direct mapping works well
```

### 7.3 Pre-Delay Relationship

Pre-delay should scale with room size for realism:

| Room Type | Pre-delay | RT60 |
|-----------|-----------|------|
| Small Room | 0-10ms | 0.2-0.5s |
| Medium Room | 10-30ms | 0.5-1.0s |
| Large Hall | 30-100ms | 1.5-3.0s |
| Cathedral | 80-150ms | 3.0-8.0s |

**Formula (speed of sound ≈ 343 m/s):**
```cpp
// Pre-delay for room dimension
float predelayMs = (roomDimensionMeters / 343.0f) * 1000.0f;
```

---

## 8. Optimization Techniques

### 8.1 SIMD Considerations

**Vectorizable Operations:**
- Delay line reads (aligned memory)
- Coefficient multiplication
- Accumulation across parallel filters

**JUCE SIMD Types:**
```cpp
using SIMDType = juce::dsp::SIMDRegister<float>;

// Process 4 samples at once
SIMDType processSIMD(SIMDType input) {
    return input * feedbackCoeff + delayOutput;
}
```

### 8.2 Memory Access Patterns

**Delay Line Optimization:**
- Use power-of-2 buffer sizes for fast modulo (masking)
- Align buffers to cache line boundaries
- Consider delay line interleaving for parallel filters

```cpp
// Fast modulo for power-of-2 sizes
int index = (writePos - delay) & sizeMask;  // Instead of %
```

### 8.3 Denormal Prevention

```cpp
void processBlock(...) {
    juce::ScopedNoDenormals noDenormals;
    // ... processing ...
}
```

### 8.4 CPU Efficiency Tips

1. **Batch Parameter Updates:** Update coefficients once per buffer, not per sample
2. **Early-Out for Silence:** Skip processing when input and tail are silent
3. **Reduce Precision:** float32 sufficient for reverb (no need for double)
4. **Parallel Filter Banks:** Use SIMD for parallel comb filters
5. **Lookup Tables:** Pre-compute expensive operations (sin for LFO)

---

## 9. Reverb in Sound Design

Reverb is not just a tool for realism—it's a powerful creative instrument for sound designers, composers, and audio engineers working across music production, film, games, and multimedia.

### 9.1 Creative Reverb Techniques

#### Shimmer Reverb

Shimmer reverb creates an ethereal, octave-shifted wash that has become a staple of ambient music.

**Core Components:**
- Pitch shifter (typically +12 semitones / octave up)
- Long, washy reverb tail
- Feedback loop between pitch shifter and reverb

**Signal Flow:**
```
Input → Reverb → Pitch Shift (+1 octave) → Feedback → Mix with dry
                        ↑__________________________|
```

**Implementation Tips:**
- Place compressor before pitch shifter to maintain tail density
- Use high diffusion for smooth, pad-like texture
- Modulation helps prevent metallic artifacts
- Notable plugins: Valhalla Shimmer, Eventide Blackhole, Strymon BigSky

#### Frozen/Infinite Reverb

Freezing captures a moment of the reverb tail and loops it indefinitely.

**Applications:**
- Creating drone textures from any sound source
- Building risers and transitions
- Generating pad layers from transient material
- Sound design for sci-fi/fantasy ambiences

**Techniques:**
- Automate freeze on/off for dramatic effects
- Modulate size/pitch while frozen for movement
- Layer multiple frozen textures at different pitches
- Use sidechain compression against frozen layer

#### Reverse Reverb

Reverse reverb creates an anticipatory "swell" leading into a sound.

**Traditional Method:**
1. Reverse the audio clip
2. Apply reverb (100% wet)
3. Render/bounce the reverb tail
4. Reverse the rendered file
5. Align with original audio

**Real-Time Method:**
- Some plugins (e.g., Valhalla Supermassive, Soundtoys Little Plate) have reverse modes
- Pre-delay becomes "post-delay" conceptually

**Use Cases:**
- Vocal entrances (creates anticipation)
- Drum hits for tension building
- Transition effects
- Horror/suspense sound design

#### Gated Reverb

Gated reverb provides explosive impact without cluttering the mix.

**Origin:** Discovered accidentally during Peter Gabriel's "Intruder" session (1979) when a talkback mic fed through a compressor captured drum bleed.

**Signal Chain:**
```
Source → Large Reverb → Noise Gate → Output
              ↓
         Gate Sidechain (from dry source)
```

**Parameters:**
- **Attack:** Fast (1-5ms)
- **Hold:** 100-300ms (defines "splash" length)
- **Release:** Fast (10-50ms) for sharp cutoff
- **Threshold:** Set to cut reverb tail cleanly

**Modern Applications:**
- Snare drums (classic 80s sound)
- Toms and kicks for punch
- Synth stabs for rhythmic emphasis
- Vocal effects

### 9.2 Film and Post-Production

#### Worldizing

Worldizing is the technique of playing back audio in a real space and re-recording it to capture authentic acoustics.

**Process:**
1. Set up playback speakers in desired location
2. Position recording microphones
3. Play back dry source audio
4. Record the result including room reflections

**When to Use:**
- When digital reverb doesn't capture the right character
- For period-accurate acoustic environments
- Matching ADR to production audio environments
- Creating authentic "source" music (radio, TV in scene)

**On-Set Impulse Response Capture:**
- Clap 2-3 times during room tone recording
- Allow full decay between claps
- Use deep claps (not bright/sharp)
- Creates usable IR for post-production matching

#### ADR Matching

Matching studio-recorded dialogue to production audio acoustics:

**Approach 1 - Convolution:**
- Use production audio sample as makeshift IR
- Apply to ADR for similar room character
- Fine-tune with algorithmic reverb for tail

**Approach 2 - Matching by Ear:**
- Analyze production audio for RT60
- Match pre-delay to room size
- Adjust early reflection density
- Roll off highs to match air absorption

**Specialized Tools:**
- Audio Ease Indoor (1000+ room IRs)
- iZotope Dialogue Match
- Exponential Audio reverbs with post-production presets

#### Foley Integration

Raw foley recordings need reverb to sit in the scene:

**Guidelines:**
- Match reverb to visual space
- Foley typically needs less reverb than you think
- Use pre-delay to separate from source
- High-pass reverb return to avoid mud
- Consider perspective (close-up vs wide shot)

**Common Issues:**
- Foley too dry = sounds disconnected
- Foley too wet = sounds like a bathroom recording
- Wrong reverb character = breaks immersion

### 9.3 Game Audio

#### Reverb Zones

Games use spatial volumes to trigger different acoustic environments:

**Implementation:**
- Define 3D trigger volumes in game engine
- Associate each zone with reverb preset
- Crossfade between zones as player moves
- Handle overlapping zones with priority system

**Zone Types:**
| Zone | RT60 | Character | Example |
|------|------|-----------|---------|
| Indoor Small | 0.2-0.5s | Tight, close | Closet, bathroom |
| Indoor Medium | 0.5-1.5s | Natural room | Office, bedroom |
| Indoor Large | 1.5-4.0s | Spacious | Cathedral, warehouse |
| Outdoor Open | 0.0-0.3s | Minimal, distant | Field, desert |
| Outdoor Urban | 0.3-0.8s | Slap-back | Street, alley |
| Cave/Tunnel | 2.0-6.0s | Dense, dark | Cave, subway |

#### Occlusion and Obstruction

Dynamic audio propagation based on game geometry:

**Occlusion:** Sound source and listener separated by geometry
- Applies lowpass filter + reduced reverb send
- Simulates sound traveling through walls

**Obstruction:** Direct path blocked, indirect path open
- Reduces direct sound, increases reverb/diffuse
- Simulates sound traveling around obstacles

**Implementation Approaches:**
- Raycast-based (simple, fast)
- Pathfinding-based (accurate, expensive)
- Portal-based (efficient for indoor environments)

**Rainbow Six Siege Innovation:**
- Propagation nodes for diffraction calculation
- Cost-based pathfinding (length + angle penalties)
- Dynamic destruction affects acoustic paths
- Impulse response reverb for authentic rooms

#### Real-Time Considerations

Game audio reverb must be CPU-efficient:

**Optimization Strategies:**
- Use algorithmic reverb (not convolution) for dynamic sources
- Limit concurrent reverb instances
- LOD (Level of Detail) for distant sounds
- Pre-baked reverb for static ambiences
- Convolution only for critical hero sounds

### 9.4 Creative Sound Design Applications

#### Granular Reverb

Combining granular synthesis with reverb for texture creation:

**Characteristics:**
- Audio broken into tiny "grains" (1-100ms)
- Grains scattered, pitch-shifted, time-stretched
- Creates evolving, organic textures
- Blurs line between reverb and synthesis

**Parameters:**
- Grain size (larger = smoother)
- Spray/scatter (randomizes position)
- Pitch variation
- Density
- Feedback

**Notable Plugins:**
- AudioThing Texture
- Output Portal
- GRM Tools Freeze
- Silo by Cymatics

#### Convolution with Non-IR Sources

Using non-traditional sources as impulse responses:

**Creative IR Sources:**
- Metal objects (springs, plates, cymbals)
- Synthesizer patches
- Vocal recordings
- Noise bursts
- Reversed sounds

**Results:**
- Unique timbral imprinting
- Source takes on character of IR
- Creates hybrid sounds
- Useful for sci-fi/fantasy design

#### Spectral Reverb

Frequency-dependent reverb processing:

**Multiband Reverb:**
- Different decay times per frequency band
- Example: Short decay on lows, long on highs
- Creates unnatural but interesting textures

**Spectral Freeze:**
- FFT-based infinite sustain
- Holds specific frequency content
- Creates drone textures from any source

---

## 10. Reverb and Spatialization

Reverb plays a crucial role in spatial audio across all playback formats, from mono to fully immersive 3D audio systems.

### 10.1 Mono Considerations

Even in mono, reverb creates depth perception:

**Depth Cues:**
- Wet/dry ratio indicates distance (more wet = farther)
- Pre-delay suggests room size
- Early reflection timing implies surface distances
- HF rolloff suggests air absorption over distance

**Mono Reverb Design:**
- Focus on temporal cues (pre-delay, decay)
- Avoid stereo-dependent effects
- Ensure mono compatibility of stereo reverbs
- Consider that many playback systems sum to mono

**Mono-to-Stereo Expansion:**
- Use reverb to create stereo width from mono sources
- Early reflections can be panned for width
- Late reverb naturally diffuses across stereo field

### 10.2 Stereo Reverb

#### True Stereo vs Stereo-to-Stereo

**Stereo Input → Mono Reverb → Stereo Output:**
- Simple, CPU efficient
- Loses input panning information
- Good for general use

**True Stereo (4-channel processing):**
- L→L, L→R, R→L, R→R paths
- Preserves input panning
- More realistic spatial image
- Higher CPU cost

**Configuration Matrix:**
```
           Output L    Output R
Input L    Direct+ER   Crossfeed
Input R    Crossfeed   Direct+ER
```

#### Stereo Width Control

**Width Parameter Implementation:**
```cpp
// M/S processing for width control
float mid = (left + right) * 0.5f;
float side = (left - right) * 0.5f;

// Width: 0 = mono, 1 = normal, 2 = extra wide
side *= width;

left = mid + side;
right = mid - side;
```

**Width Considerations:**
- Low width = mono-compatible, centered
- Normal width = natural stereo spread
- High width = wide but may have phase issues
- Check mono compatibility when widening

#### Decorrelation

Creating stereo difference without panning:

**Techniques:**
- Different delay lengths for L/R channels
- Modulation with different LFO phases
- Allpass filters with different coefficients
- Slight pitch/time differences

**Freeverb Approach:**
- Add fixed offset (+23 samples) to all right channel delays
- Creates subtle decorrelation without obvious artifacts

### 10.3 Binaural Audio

#### HRTF (Head-Related Transfer Function)

HRTFs model how sound reaches the ears from any direction:

**Components:**
- **ITD:** Interaural Time Difference (up to ~0.7ms)
- **ILD:** Interaural Level Difference (frequency-dependent)
- **Spectral Cues:** Pinna filtering (ear shape effects)

**HRTF + Reverb:**
```
Source → HRTF (direct sound) → Mix
   ↓
Reverb → HRTF (per reflection) → Mix
```

**Challenges:**
- HRTFs are individual (generic may not work for everyone)
- Elevation perception requires accurate pinna modeling
- Front/back confusion common with generic HRTFs

#### Binaural Room Impulse Responses (BRIR)

Combining room acoustics with HRTF:

**BRIR = HRTF × RIR (Room Impulse Response)**

**Capture Methods:**
- Dummy head recording in real space
- Simulation from room geometry + HRTF database
- Hybrid (measured early + synthetic late)

**Advantages:**
- Single convolution provides both spatialization and reverb
- Highly realistic for captured spaces
- Efficient playback

**Limitations:**
- Fixed listener position
- Large memory requirement for multiple positions
- Cannot easily modify room characteristics

#### Binaural Reverb Plugins

**Key Features to Look For:**
- 360° panning capability
- HRTF selection/personalization
- Independent early reflection placement
- Smooth head-tracking support (for VR)

**Notable Tools:**
- THX Spatial Creator
- dearVR Pro
- Waves B360
- Facebook/Meta Spatial Audio SDK

### 10.4 Surround Sound (5.1/7.1)

#### Channel Configuration

**5.1 Layout:**
```
        C
   L         R

   👤 (listener)

   Ls        Rs

       LFE
```

**7.1 Layout (adds side channels):**
```
        C
   L         R
   Lss       Rss  (side)
   👤
   Ls        Rs   (rear)
       LFE
```

#### Surround Reverb Strategies

**Approach 1: Multiple Stereo Reverbs**
```
Front L/R → Short reverb → Front speakers
Rear Ls/Rs → Long reverb with pre-delay → Rear speakers
Center → Dedicated reverb → Center speaker
```

**Approach 2: True Multichannel Reverb**
- Single algorithm with 5.1/7.1 output
- Coherent early reflections across all channels
- Enveloping late reverb distribution

**Approach 3: Upmixed Stereo**
- Stereo reverb → Decoder → 5.1/7.1
- Less control but simple workflow

#### LFE Considerations

- Generally avoid sending reverb to LFE
- LFE is for discrete low-frequency effects
- Reverb low end handled by main speakers
- Exception: Specialized "subwoofer rumble" design effects

#### Surround Reverb Plugins

| Plugin | Channels | Type | Notable Features |
|--------|----------|------|------------------|
| Waves H-Reverb Surround | Up to 7.1 | Hybrid | Convolution + algorithmic |
| Exponential Audio Stratus 3D | 7.1.4 | Algorithmic | Atmos-ready |
| Audio Ease Altiverb 7 | Up to 7.1 | Convolution | Massive IR library |
| 2CAudio Precedence | 7.1 | Early reflections | Geometric room modeling |

### 10.5 Immersive/3D Audio (Dolby Atmos, Ambisonics)

#### Dolby Atmos

**Object-Based Audio:**
- Audio "objects" with position metadata
- Renderer adapts to any speaker layout
- Up to 128 tracks, 7.1.2 bed + objects

**Reverb in Atmos:**
- Reverb can be applied to objects or beds
- Spatial reverbs maintain object position
- Must not simply upmix stereo reverb

**Apple Music Guidelines:**
- Atmos tracks must be created from multitracks
- Cannot just add reverb to stereo and call it Atmos
- Reverb should enhance spatial positioning

**Atmos-Compatible Reverbs:**
- Spacelab Interstellar (object-based 3D reverb)
- Dolby Atmos Production Suite (built-in reverb)
- Exponential Audio Stratus 3D
- 7.1.4 capable algorithmic reverbs

#### Ambisonics

**B-Format Encoding:**
Ambisonics represents a complete soundfield using spherical harmonics:

**First Order Ambisonics (FOA) - 4 channels:**
- W: Omnidirectional (pressure)
- X: Front-back figure-8
- Y: Left-right figure-8
- Z: Up-down figure-8

**Higher Order Ambisonics (HOA):**
- 2nd order: 9 channels
- 3rd order: 16 channels
- Higher orders = better spatial resolution

**Ambisonics Reverb Processing:**

**Option 1: Process in A-Format**
- Decode to virtual mic array (A-format)
- Apply different processing per channel
- Re-encode to B-format

**Option 2: Process in B-Format**
- Apply same reverb to all channels
- Maintains spatial relationships
- Simpler but less flexible

**Ambisonics Reverb Plugins:**
- Blue Ripple Sound O3A Reverb
- IEM Plug-in Suite (free)
- Zephyr by Neuman
- SSA Plugins

#### Ambisonics to Binaural Decoding

For headphone playback of Ambisonics content:

**Process:**
1. Decode Ambisonics to virtual speaker array
2. Apply HRTF to each virtual speaker
3. Sum to binaural output

**Considerations:**
- Higher order = more accurate spatialization
- Head tracking can rotate the soundfield
- Generic HRTFs may reduce accuracy

### 10.6 VR/AR/XR Audio

#### Head-Tracked Reverb

In VR, the listener's head movement must be reflected in audio:

**Requirements:**
- Low latency (<20ms motion-to-sound)
- Smooth transitions during rotation
- Stable externalization (sounds stay "outside" head)

**Implementation:**
- Real-time HRTF rotation
- Ambisonics rotation (efficient for soundfields)
- Pre-computed BRIRs for discrete positions

#### Room-Scale Acoustics

VR environments need acoustics that match visual spaces:

**Dynamic Considerations:**
- Listener position affects reverb character
- Room geometry may change (destructible environments)
- Near-field sources need special handling

**Approaches:**
- Geometric acoustic simulation (ray tracing)
- Pre-computed reverb zones with interpolation
- Hybrid (real-time early + pre-computed late)

#### Oculus/Meta Spatial Audio

**Near-Field Rendering:**
Beyond a few feet, HRTFs don't change much with distance. Meta's spatial audio handles near-field sources (within arm's reach) specially:

- Distance-dependent HRTF interpolation
- ILD increases dramatically at close range
- Creates sense of objects close to head

**Reverb Integration:**
- Room estimation from visual/depth sensors
- Real-time acoustic simulation
- Shared acoustic model across audio and visuals

### 10.7 Spatial Reverb Implementation Considerations

#### Mono Compatibility

Always verify spatial reverbs collapse well to mono:

```cpp
// Check for phase issues
float mono = (left + right) * 0.5f;
// Compare against original mono source level
```

#### CPU Scaling

Spatial audio is expensive. Consider:

| Format | Typical CPU Multiplier |
|--------|----------------------|
| Mono | 1x |
| Stereo | 2x |
| 5.1 | 6x |
| 7.1.4 | 12x |
| Ambisonics 3rd order | 16x |

**Optimization Strategies:**
- Use lower-order Ambisonics for less critical sources
- LOD system for distant sources
- Pre-render where possible
- Limit simultaneous spatialized sources

#### Format Conversion

Moving between formats requires care:

**Upmixing (e.g., stereo → 5.1):**
- Simple duplication to rears sounds artificial
- Use decorrelation + delay for rear channels
- Consider specialized upmix algorithms

**Downmixing (e.g., 5.1 → stereo):**
- Standard coefficients: L = L + 0.707×C + 0.707×Ls
- Reverb may become too prominent
- Check for phase cancellation

---

## 11. References and Resources

### 11.1 Essential Academic Papers

1. **Schroeder, M.R. (1962)** - "Natural Sounding Artificial Reverberation"
   - Original comb + allpass architecture
   - AES Journal

2. **Moorer, J.A. (1979)** - "About This Reverberation Business"
   - Lowpass comb filters, early reflections
   - Computer Music Journal

3. **Dattorro, J. (1997)** - "Effect Design Part 1: Reverberator and Other Filters"
   - Complete plate reverb specification
   - [PDF](https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf)

4. **Gerzon, M. (1971/1972)** - Feedback Delay Networks
   - Orthogonal matrix feedback

5. **Jot & Chaigne (1991)** - FDN with frequency-dependent decay

### 11.2 Online Resources

- [Stanford CCRMA - Physical Audio Signal Processing](https://ccrma.stanford.edu/~jos/pasp/)
- [Valhalla DSP Blog](https://valhalladsp.com/blog/) - Sean Costello's reverb design insights
- [DSPRelated.com - Reverb Chapters](https://www.dsprelated.com/freebooks/pasp/)
- [Freeverb3 Tips](https://freeverb3-vst.sourceforge.io/tips/reverb.shtml)
- [JUCE DSP Documentation](https://docs.juce.com/master/group__juce__dsp.html)

### 11.3 Open Source Implementations

| Project | Type | License | Notes |
|---------|------|---------|-------|
| **Freeverb** | Schroeder-Moorer | Public Domain | Jezar's original |
| **MVerb** | Dattorro | GPL | Clean C++ implementation |
| **Dragonfly Reverb** | Freeverb3-based | GPL | Full-featured plugin |
| **CloudReverb** | Granular + Reverb | MIT | Experimental |
| **HiFi-LoFi FFTConvolver** | Convolution | LGPL | Efficient partitioned convolution |

### 11.4 JUCE-Specific Resources

- [JUCE Tutorial: DSP Introduction](https://docs.juce.com/master/tutorial_dsp_introduction.html)
- [juce::dsp::Reverb Class Reference](https://docs.juce.com/master/classdsp_1_1Reverb.html)
- [juce::dsp::Convolution Class Reference](https://docs.juce.com/master/classdsp_1_1Convolution.html)

---

## Appendix A: Freeverb Delay Lengths (44.1kHz)

**Comb Filters:**
```
1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
```

**Allpass Filters:**
```
225, 341, 441, 556
```

**Stereo Spread:** +23 samples for right channel

---

## Appendix B: Common RT60 Values

| Environment | RT60 (seconds) |
|-------------|----------------|
| Anechoic Chamber | 0.0 |
| Recording Studio | 0.2 - 0.4 |
| Living Room | 0.4 - 0.6 |
| Small Concert Hall | 1.0 - 1.5 |
| Large Concert Hall | 1.5 - 2.5 |
| Cathedral | 3.0 - 8.0 |
| Parking Garage | 2.0 - 4.0 |

---

## Appendix C: Quick Reference - JUCE Reverb API

```cpp
// === JUCE dsp::Reverb ===
juce::dsp::Reverb reverb;

// Prepare
juce::dsp::ProcessSpec spec { sampleRate, blockSize, numChannels };
reverb.prepare(spec);
reverb.reset();

// Configure (call in processBlock before processing)
juce::Reverb::Parameters params;
params.roomSize = 0.5f;     // 0.0 - 1.0
params.damping = 0.5f;      // 0.0 - 1.0
params.wetLevel = 1.0f;     // 0.0 - 1.0 (use DryWetMixer for control)
params.dryLevel = 0.0f;     // 0.0 - 1.0
params.width = 1.0f;        // 0.0 - 1.0
params.freezeMode = 0.0f;   // 0.0 or 1.0
reverb.setParameters(params);

// Process
juce::dsp::AudioBlock<float> block(buffer);
juce::dsp::ProcessContextReplacing<float> context(block);
reverb.process(context);
```

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-13 | Initial release - Complete reverb research document |

---

*Document compiled from research across academic papers, professional plugin analysis, JUCE documentation, and industry sources. For implementation, always verify with current JUCE API documentation.*

**Key Sources:**
- Stanford CCRMA (Schroeder, Dattorro papers)
- Valhalla DSP Blog (Sean Costello)
- JUCE Framework Documentation
- KVR Audio Forums
- Game Developer (Rainbow Six Siege audio)
- Dolby Atmos Production Guidelines
- Meta Spatial Audio SDK Documentation
