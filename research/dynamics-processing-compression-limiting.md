---
title: "Dynamics Processing: Compression & Limiting"
created: 2026-03-07
juce_version: "8.0.4"
summary: "Comprehensive technical reference for dynamics processing in audio plugins, covering compressor topologies (VCA, FET, optical, vari-mu), multiband crossover design, sidechain filtering, look-ahead, limiting, and JUCE implementation with real-time safety considerations."
domain: dsp
type: guide
keywords:
  - compression
  - limiting
  - dynamics
  - multiband
  - sidechain
  - vca
  - fet
  - optical
  - envelope-follower
  - juce-dsp
stages: [0, 1, 2]
agents: [dsp, research]
---

# Dynamics Processing: Compression & Limiting

**Complete Technical Reference for Dynamics Processing in Audio Plugins**

**Created:** March 2026
**Version:** 1.0
**Research Depth:** Level 3 (Comprehensive Investigation)

---

## Executive Summary

This document covers dynamics processing for audio plugin development, from fundamental gain reduction theory through advanced multiband and look-ahead implementations. It addresses both single-band and multiband compression, limiting, and the analog topology emulations (VCA, FET, optical, vari-mu) that define the character of commercial compressors.

**Key Findings:**
- Envelope detection (peak vs. RMS vs. true-peak) determines compressor character more than any other design choice
- Analog topology emulation requires modeling nonlinear gain element behavior, not just static transfer curves
- Multiband compression requires phase-coherent crossover filters (Linkwitz-Riley preferred)
- Look-ahead introduces latency but enables transparent limiting without overshoot
- JUCE 8 provides `dsp::Compressor` for basic use cases; custom implementations needed for advanced topologies

---

## Table of Contents

### Part 1: Fundamentals
1. [Dynamic Range and Transfer Curves](#1-dynamic-range-and-transfer-curves)
2. [Envelope Detection](#2-envelope-detection)
3. [Gain Computer](#3-gain-computer)

### Part 2: Compressor Topologies
4. [VCA Compression](#4-vca-compression)
5. [FET Compression](#5-fet-compression)
6. [Optical Compression](#6-optical-compression)
7. [Variable-Mu (Vari-Mu) Compression](#7-variable-mu-vari-mu-compression)

### Part 3: Advanced Techniques
8. [Sidechain Filtering](#8-sidechain-filtering)
9. [Look-Ahead and Latency](#9-look-ahead-and-latency)
10. [Multiband Dynamics Processing](#10-multiband-dynamics-processing)
11. [Limiting and Brickwall Limiting](#11-limiting-and-brickwall-limiting)
12. [Parallel Compression](#12-parallel-compression)

### Part 4: JUCE Implementation
13. [JUCE DSP Module Integration](#13-juce-dsp-module-integration)
14. [Complete Compressor Implementation](#14-complete-compressor-implementation)
15. [Real-Time Safety and Optimization](#15-real-time-safety-and-optimization)

### Part 5: References
16. [References and Further Reading](#16-references-and-further-reading)

---

## Part 1: Fundamentals

## 1. Dynamic Range and Transfer Curves

### 1.1 Static Transfer Characteristics

A compressor's behavior is defined by its **static transfer curve** -- the relationship between input level and output level in dB:

| Parameter | Description | Typical Range |
|-----------|-------------|---------------|
| **Threshold** | Level above which compression begins | -60 to 0 dBFS |
| **Ratio** | Input-to-output ratio above threshold | 1:1 to infinity:1 |
| **Knee** | Transition width around threshold | 0 (hard) to 20 dB (soft) |
| **Makeup Gain** | Post-compression gain to restore level | 0 to 40 dB |

### 1.2 Hard Knee vs. Soft Knee

**Hard knee** applies the ratio abruptly at the threshold:

```
if (inputdB > threshold)
    outputdB = threshold + (inputdB - threshold) / ratio;
else
    outputdB = inputdB;
```

**Soft knee** uses a quadratic interpolation region around the threshold:

```cpp
float computeGain(float inputdB, float threshold, float ratio, float kneeWidth)
{
    float halfKnee = kneeWidth * 0.5f;

    if (inputdB < threshold - halfKnee)
    {
        // Below knee region -- no compression
        return inputdB;
    }
    else if (inputdB > threshold + halfKnee)
    {
        // Above knee region -- full compression
        return threshold + (inputdB - threshold) / ratio;
    }
    else
    {
        // In knee region -- quadratic interpolation
        float x = inputdB - threshold + halfKnee;
        float compressionFactor = (1.0f / ratio - 1.0f) / (2.0f * kneeWidth);
        return inputdB + compressionFactor * x * x;
    }
}
```

The soft knee approach produces more transparent compression by gradually introducing the ratio change, which is why most modern mastering compressors default to soft knee operation.

### 1.3 Compression Ratio Spectrum

| Ratio | Usage | Character |
|-------|-------|-----------|
| 1:1 - 2:1 | Gentle leveling | Transparent, natural |
| 2:1 - 4:1 | General compression | Noticeable control |
| 4:1 - 10:1 | Heavy compression | Aggressive, pumping |
| 10:1 - 20:1 | Limiting | Very controlled peaks |
| infinity:1 | Brickwall limiting | Absolute ceiling |

---

## 2. Envelope Detection

The envelope detector (also called level detector or ballistics) converts the audio signal into a smooth control signal representing instantaneous level. This is the most critical component defining compressor character.

### 2.1 Peak Detection

Peak detection tracks the absolute peak level of the signal:

```cpp
class PeakDetector
{
public:
    void prepare(double sampleRate, float attackMs, float releaseMs)
    {
        attackCoeff  = std::exp(-1.0f / (0.001f * attackMs * (float)sampleRate));
        releaseCoeff = std::exp(-1.0f / (0.001f * releaseMs * (float)sampleRate));
        envelope = 0.0f;
    }

    float process(float input)
    {
        float absInput = std::abs(input);

        if (absInput > envelope)
            envelope = attackCoeff * envelope + (1.0f - attackCoeff) * absInput;
        else
            envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * absInput;

        return envelope;
    }

private:
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float envelope = 0.0f;
};
```

### 2.2 RMS Detection

RMS detection measures the average power of the signal, producing smoother, more musical compression:

```cpp
class RMSDetector
{
public:
    void prepare(double sampleRate, float windowMs)
    {
        int windowSamples = static_cast<int>(0.001f * windowMs * sampleRate);
        buffer.resize(windowSamples, 0.0f);
        writeIndex = 0;
        sum = 0.0f;
    }

    float process(float input)
    {
        float squared = input * input;
        sum -= buffer[writeIndex];
        buffer[writeIndex] = squared;
        sum += squared;
        writeIndex = (writeIndex + 1) % (int)buffer.size();
        return std::sqrt(sum / (float)buffer.size());
    }

private:
    std::vector<float> buffer;
    int writeIndex = 0;
    float sum = 0.0f;
};
```

### 2.3 True Peak Detection (ITU-R BS.1770)

True peak detection uses oversampling to detect inter-sample peaks that exceed 0 dBFS. Essential for mastering limiters:

1. Upsample the input by 4x using a polyphase FIR filter
2. Find the maximum absolute value across all oversampled points
3. Use this as the detection level

The ITU-R BS.1770 standard specifies a 48-tap FIR filter at 4x oversampling for true peak measurement.

### 2.4 Decoupled Attack/Release

The classic "branching" envelope follower uses different time constants for attack and release:

**Attack time** (1-100 ms): How fast the compressor reacts to levels above threshold
- Fast attack (0.1-5 ms): Catches transients, can reduce punch
- Medium attack (5-30 ms): Lets transients through, controls sustain
- Slow attack (30-100 ms): Preserves transients, may miss fast peaks

**Release time** (10-5000 ms): How fast gain recovers after signal drops below threshold
- Fast release (10-100 ms): Can cause "pumping" or "breathing"
- Medium release (100-500 ms): Natural feel
- Slow release (500-5000 ms): Smooth, leveling

### 2.5 Auto-Release (Program-Dependent)

Auto-release adapts the release time based on the signal characteristics:

```cpp
float computeAutoRelease(float gainReduction, float releaseMin, float releaseMax)
{
    // Deeper compression = longer release to avoid pumping
    float normalized = juce::jlimit(0.0f, 1.0f, gainReduction / 20.0f);
    return releaseMin + normalized * (releaseMax - releaseMin);
}
```

More sophisticated implementations use dual time constants (fast + slow) blended based on signal density, similar to the SSL G-series bus compressor's auto release.

---

## 3. Gain Computer

The gain computer combines the envelope detector output with the transfer curve to determine the gain reduction to apply.

### 3.1 Feed-Forward vs. Feed-Back

**Feed-forward** (modern approach):
- Sidechain signal is taken from the input
- Gain reduction computed from input level
- More predictable, precise ratio control
- Used in: SSL, Neve, most digital compressors

**Feed-back** (vintage approach):
- Sidechain signal is taken from the output
- Creates implicit program-dependent behavior
- The compression ratio becomes signal-dependent
- Used in: LA-2A, 1176, Fairchild

```cpp
// Feed-forward
float gainReduction = computeGainReduction(inputLevel, threshold, ratio, knee);
float output = input * juce::Decibels::decibelsToGain(gainReduction);

// Feed-back
float gainReduction = computeGainReduction(outputLevel, threshold, ratio, knee);
float output = input * juce::Decibels::decibelsToGain(gainReduction);
// Note: outputLevel uses previous sample's output (1-sample feedback delay)
```

### 3.2 Gain Smoothing

To avoid zipper noise, gain reduction changes must be smoothed:

```cpp
class GainSmoother
{
public:
    void prepare(double sampleRate, float smoothingMs = 5.0f)
    {
        coeff = std::exp(-1.0f / (0.001f * smoothingMs * (float)sampleRate));
    }

    float process(float targetGain)
    {
        smoothedGain = coeff * smoothedGain + (1.0f - coeff) * targetGain;
        return smoothedGain;
    }

private:
    float coeff = 0.0f;
    float smoothedGain = 1.0f;
};
```

---

## Part 2: Compressor Topologies

## 4. VCA Compression

**Voltage-Controlled Amplifier** compressors use a VCA as the gain element, offering precise, clean dynamics control.

### 4.1 Characteristics

| Property | VCA Character |
|----------|--------------|
| Response | Fast, precise |
| Distortion | Very low (clean) |
| Ratio accuracy | Exact |
| Transient handling | Excellent |
| Program dependency | Low (feed-forward) |
| Classic examples | SSL G-series, API 2500, dbx 160 |

### 4.2 VCA Modeling

VCA behavior is approximately linear in the log domain -- the gain reduction in dB is directly proportional to the control voltage. Digital emulation focuses on:

1. Precise envelope detection with configurable attack/release
2. Accurate transfer curve with soft knee
3. Sidechain high-pass filter (common on bus compressors)
4. Harmonic distortion from the VCA element (subtle, even harmonics)

```cpp
class VCACompressor
{
public:
    float processGainElement(float controlSignal)
    {
        // VCA: nearly linear gain control in dB domain
        // Add subtle even-harmonic distortion
        float gain = juce::Decibels::decibelsToGain(controlSignal);

        // Model subtle VCA nonlinearity (even harmonics)
        float distortion = gain * gain * vcaDistortionAmount;
        return gain + distortion;
    }

private:
    float vcaDistortionAmount = 0.001f; // Very subtle
};
```

---

## 5. FET Compression

**Field-Effect Transistor** compressors use a JFET as a variable resistor in a voltage divider, producing aggressive, colored compression.

### 5.1 Characteristics

| Property | FET Character |
|----------|--------------|
| Response | Very fast (microseconds) |
| Distortion | Moderate (adds harmonic richness) |
| Ratio accuracy | Approximate (program-dependent) |
| Transient handling | Aggressive shaping |
| Program dependency | High |
| Classic examples | UREI 1176, Purple MC77 |

### 5.2 FET Nonlinearity

The JFET operates in its triode region as a voltage-controlled resistor, but its resistance is nonlinear:

```cpp
float fetGainElement(float controlVoltage, float inputSignal)
{
    // JFET drain-source resistance model
    // Rds = Rds_on / (1 - Vgs/Vp)^2 approximation
    float pinchOff = -2.5f; // Vp typical for 2N5457
    float vgs = juce::jlimit(pinchOff, 0.0f, -controlVoltage);
    float normalizedVgs = vgs / pinchOff;
    float rds = rdsOn / ((1.0f - normalizedVgs) * (1.0f - normalizedVgs) + 0.001f);

    // Voltage divider with load resistor
    float attenuation = rLoad / (rds + rLoad);

    // FET adds odd harmonics at high levels
    float saturated = std::tanh(inputSignal * attenuation * 1.5f);
    return saturated;
}
```

### 5.3 The 1176 "All-Buttons" Mode

The UREI 1176's "all-buttons-in" mode simultaneously engages all four ratio settings, creating unpredictable, heavily distorted compression. In DSP terms, this overdrives the sidechain and gain element, producing:
- Very fast attack (20 microseconds)
- Nonlinear ratio (changes with level)
- Significant harmonic distortion
- Dramatic transient reshaping

---

## 6. Optical Compression

**Optical (opto)** compressors use a light source (LED or electroluminescent panel) and a photoresistor (LDR) to control gain.

### 6.1 Characteristics

| Property | Optical Character |
|----------|------------------|
| Response | Slow, program-dependent |
| Distortion | Very low (smooth) |
| Ratio accuracy | Very approximate |
| Transient handling | Gentle, preserves feel |
| Program dependency | Very high (LDR hysteresis) |
| Classic examples | Teletronix LA-2A, Tube-Tech CL 1B |

### 6.2 Photoresistor Modeling

The LDR (light-dependent resistor) has asymmetric response characteristics -- it responds faster to increasing light than decreasing:

```cpp
class OpticalGainElement
{
public:
    void prepare(double sampleRate)
    {
        // LDR time constants are much slower than electronic circuits
        float attackMs = 10.0f;   // LED illumination
        float releaseMs = 150.0f; // LDR decay (can be 500ms+ for deep compression)

        attackCoeff = std::exp(-1.0f / (0.001f * attackMs * (float)sampleRate));
        releaseCoeff = std::exp(-1.0f / (0.001f * releaseMs * (float)sampleRate));
        ldrState = 0.0f;
    }

    float process(float controlSignal)
    {
        // Model LDR hysteresis: fast illumination, slow decay
        if (controlSignal > ldrState)
            ldrState += (1.0f - attackCoeff) * (controlSignal - ldrState);
        else
            ldrState += (1.0f - releaseCoeff) * (controlSignal - ldrState);

        // LDR resistance is inversely proportional to light
        // R_ldr = R_dark * (E / E_threshold)^(-gamma)
        // gamma ~ 0.7-0.9 for typical CdS photoresistors
        float gamma = 0.8f;
        float resistance = std::pow(juce::jmax(ldrState, 0.001f), -gamma);
        float attenuation = 1.0f / (1.0f + resistance * 0.01f);

        return attenuation;
    }

private:
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float ldrState = 0.0f;
};
```

### 6.3 Two-Stage Release

The LA-2A's release has a distinctive two-stage characteristic:
- **Stage 1** (0-0.5s): Fast initial recovery (~60 ms time constant)
- **Stage 2** (0.5s+): Slow final recovery (~1-3s time constant)

This creates the "musical" feel that optical compressors are known for -- the gain recovers quickly enough to avoid pumping but slowly enough for smooth leveling.

---

## 7. Variable-Mu (Vari-Mu) Compression

**Variable-mu** compressors use vacuum tubes as the gain element, where the tube's transconductance (mu) is varied by the control signal.

### 7.1 Characteristics

| Property | Vari-Mu Character |
|----------|------------------|
| Response | Slow to medium |
| Distortion | Moderate (warm, even harmonics) |
| Ratio accuracy | Soft, level-dependent |
| Transient handling | Gentle with natural limiting |
| Program dependency | High |
| Classic examples | Fairchild 670, Manley Vari-Mu |

### 7.2 Tube Gain Modeling

```cpp
float variMuGainElement(float controlSignal, float inputSignal)
{
    // Tube transconductance varies with bias point
    // mu = mu_max * (1 - (Vcontrol / Vmax)^2)
    float mu = muMax * (1.0f - controlSignal * controlSignal / (vMax * vMax));
    mu = juce::jmax(mu, muMin); // Prevent zero gain

    // Tube saturation (soft clipping with even harmonics)
    float output = inputSignal * mu;
    float saturated = output - (output * output * output) / 3.0f; // Cubic soft clip
    return saturated;
}
```

### 7.3 Transformer Saturation

Vari-mu compressors typically include output transformers that add:
- Low-frequency saturation at high levels
- High-frequency roll-off (transformer inductance)
- Even harmonic content
- Subtle stereo image thickening in linked stereo pairs

---

## Part 3: Advanced Techniques

## 8. Sidechain Filtering

### 8.1 External Sidechain

External sidechain routing allows the compressor to be triggered by a different signal:

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    auto totalNumInputChannels = getTotalNumInputChannels();

    // Channels 0-1: audio input
    // Channels 2-3: sidechain input (if available)
    bool hasSidechain = totalNumInputChannels > 2;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float scLeft, scRight;

        if (hasSidechain)
        {
            scLeft  = buffer.getSample(2, sample);
            scRight = buffer.getSample(3, sample);
        }
        else
        {
            scLeft  = buffer.getSample(0, sample);
            scRight = buffer.getSample(1, sample);
        }

        float scLevel = (std::abs(scLeft) + std::abs(scRight)) * 0.5f;
        float gainReduction = computeGainReduction(scLevel);

        // Apply to main audio channels
        buffer.getSample(0, sample) *= gainReduction;
        buffer.getSample(1, sample) *= gainReduction;
    }
}
```

### 8.2 Internal Sidechain Filters

A high-pass filter on the sidechain prevents low-frequency energy from triggering unnecessary compression:

```cpp
class SidechainFilter
{
public:
    void prepare(double sampleRate)
    {
        // High-pass filter to remove sub-bass from sidechain
        hpFilter.setCoefficients(
            juce::IIRCoefficients::makeHighPass(sampleRate, sidechainHPFreq));
    }

    float process(float input)
    {
        return hpFilter.processSingleSampleRaw(input);
    }

    void setFrequency(double sampleRate, float freq)
    {
        sidechainHPFreq = freq;
        hpFilter.setCoefficients(
            juce::IIRCoefficients::makeHighPass(sampleRate, sidechainHPFreq));
    }

private:
    juce::IIRFilter hpFilter;
    float sidechainHPFreq = 100.0f; // Typical bus compressor sidechain HPF
};
```

This is critical for bus compression -- without the HPF, kick drum energy causes the entire mix to "pump."

### 8.3 Frequency-Weighted Sidechain

Some compressors use frequency-weighted detection curves (e.g., K-weighting from ITU-R BS.1770) for loudness-aware compression:

- **A-weighting**: Approximates human hearing sensitivity
- **K-weighting**: Two-stage filter for loudness normalization
- **Custom**: Parametric EQ on sidechain for creative control

---

## 9. Look-Ahead and Latency

### 9.1 Look-Ahead Buffer

Look-ahead delays the audio path relative to the sidechain, allowing the compressor to react before transients arrive:

```cpp
class LookAheadCompressor
{
public:
    void prepare(double sampleRate, float lookAheadMs)
    {
        int delaySamples = static_cast<int>(lookAheadMs * 0.001f * sampleRate);
        delayLine.resize(delaySamples, 0.0f);
        writePos = 0;

        // Report latency to host DAW
        setLatencySamples(delaySamples);
    }

    float process(float input)
    {
        // Compute gain reduction from current (future) input
        float level = envelopeDetector.process(input);
        float gainReduction = gainComputer.compute(level);

        // Read delayed audio
        float delayed = delayLine[writePos];

        // Write current input to delay
        delayLine[writePos] = input;
        writePos = (writePos + 1) % (int)delayLine.size();

        // Apply gain reduction to delayed signal
        return delayed * juce::Decibels::decibelsToGain(gainReduction);
    }

private:
    std::vector<float> delayLine;
    int writePos = 0;
    EnvelopeDetector envelopeDetector;
    GainComputer gainComputer;
};
```

**Note:** In JUCE 8, `getLatencySamples()` is non-virtual. Use `setLatencySamples()` in `prepareToPlay()` to report look-ahead latency to the host.

### 9.2 Look-Ahead Attack Shaping

With look-ahead, the attack envelope can be shaped as a smooth ramp rather than an exponential decay, eliminating overshoot:

```cpp
// Linear attack ramp over look-ahead window
float computeLookAheadAttack(float* gainBuffer, int lookAheadSamples)
{
    // Find the minimum gain (maximum compression) in the look-ahead window
    float minGain = 1.0f;
    for (int i = 0; i < lookAheadSamples; ++i)
        minGain = std::min(minGain, gainBuffer[i]);

    // Create linear ramp from current gain to minimum
    for (int i = 0; i < lookAheadSamples; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(lookAheadSamples);
        gainBuffer[i] = 1.0f + t * (minGain - 1.0f);
    }

    return minGain;
}
```

---

## 10. Multiband Dynamics Processing

### 10.1 Crossover Filter Design

Multiband compressors split the audio into frequency bands, process each independently, then sum. The crossover filters must be **phase-coherent** to avoid comb filtering at the crossover points.

**Linkwitz-Riley crossovers** are preferred because they sum flat:

| Type | Slope | Phase at crossover | Sum behavior |
|------|-------|-------------------|--------------|
| Butterworth | 12/18/24 dB/oct | 90/135/180 deg | +3 dB bump at crossover |
| Linkwitz-Riley 2nd | 12 dB/oct | 180 deg | Flat sum (allpass) |
| Linkwitz-Riley 4th | 24 dB/oct | 360 deg | Flat sum (allpass) |
| Linkwitz-Riley 8th | 48 dB/oct | 720 deg | Flat sum (allpass) |

### 10.2 JUCE Linkwitz-Riley Implementation

```cpp
class MultibandCrossover
{
public:
    void prepare(double sampleRate, int samplesPerBlock)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
        spec.numChannels = 2;

        // 3-band crossover using 4th-order Linkwitz-Riley
        lowCrossover.prepare(spec);
        highCrossover.prepare(spec);

        lowCrossover.setCutoffFrequency(lowFreq);
        highCrossover.setCutoffFrequency(highFreq);
    }

    void process(juce::AudioBuffer<float>& input,
                 juce::AudioBuffer<float>& lowBand,
                 juce::AudioBuffer<float>& midBand,
                 juce::AudioBuffer<float>& highBand)
    {
        // First split: low vs (mid+high)
        lowBand.makeCopyOf(input);
        auto tempMidHigh = input; // copy

        auto lowBlock = juce::dsp::AudioBlock<float>(lowBand);
        auto midHighBlock = juce::dsp::AudioBlock<float>(tempMidHigh);

        // Linkwitz-Riley splits into LP and HP simultaneously
        lowCrossover.process(
            juce::dsp::ProcessContextNonReplacing<float>(
                juce::dsp::AudioBlock<float>(input),
                lowBlock));

        // Second split: mid vs high
        midBand.makeCopyOf(tempMidHigh);
        highBand.makeCopyOf(tempMidHigh);

        auto midBlock = juce::dsp::AudioBlock<float>(midBand);
        auto highBlock = juce::dsp::AudioBlock<float>(highBand);

        highCrossover.process(
            juce::dsp::ProcessContextNonReplacing<float>(
                juce::dsp::AudioBlock<float>(tempMidHigh),
                midBlock));
    }

private:
    juce::dsp::LinkwitzRileyFilter<float> lowCrossover;
    juce::dsp::LinkwitzRileyFilter<float> highCrossover;
    float lowFreq = 200.0f;
    float highFreq = 4000.0f;
};
```

### 10.3 Per-Band Compression

Each band gets its own compressor with independent threshold, ratio, attack, release:

```cpp
struct BandCompressor
{
    float threshold = -20.0f;
    float ratio = 4.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float makeupGain = 0.0f;
    bool solo = false;
    bool bypass = false;

    PeakDetector detector;
    GainSmoother smoother;
};
```

**Common multiband strategy:**
- **Low band** (20-200 Hz): Slow attack (30-50 ms), medium release, moderate ratio (2:1-3:1) -- tighten bass without killing transients
- **Mid band** (200-4000 Hz): Medium attack, medium release, moderate ratio -- control vocals/instruments
- **High band** (4000-20000 Hz): Fast attack (1-5 ms), fast release, gentle ratio (1.5:1-2:1) -- de-ess and control harshness

### 10.4 Band Summation and Gain Compensation

After per-band processing, bands must be summed carefully:

```cpp
void sumBands(juce::AudioBuffer<float>& output,
              const juce::AudioBuffer<float>& low,
              const juce::AudioBuffer<float>& mid,
              const juce::AudioBuffer<float>& high)
{
    output.clear();

    for (int ch = 0; ch < output.getNumChannels(); ++ch)
    {
        output.addFrom(ch, 0, low, ch, 0, output.getNumSamples());
        output.addFrom(ch, 0, mid, ch, 0, output.getNumSamples());
        output.addFrom(ch, 0, high, ch, 0, output.getNumSamples());
    }
}
```

---

## 11. Limiting and Brickwall Limiting

### 11.1 Limiter vs. Compressor

A limiter is a compressor with a very high ratio (typically 10:1 to infinity:1) and fast attack. A **brickwall limiter** guarantees no sample exceeds the ceiling.

### 11.2 ISP (Inter-Sample Peak) Limiting

Standard sample-peak limiting misses peaks that occur between samples during DA conversion. True peak limiting requires oversampling:

```cpp
class TruePeakLimiter
{
public:
    void prepare(double sampleRate, int blockSize)
    {
        // 4x oversampling for true peak detection
        oversampler.initProcessing((size_t)blockSize);
        setLatencySamples((int)oversampler.getLatencyInSamples());
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto oversampledBlock = oversampler.processSamplesUp(block);

        // Process at oversampled rate
        for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
        {
            auto* samples = oversampledBlock.getChannelPointer(ch);
            for (size_t i = 0; i < oversampledBlock.getNumSamples(); ++i)
            {
                float level = std::abs(samples[i]);
                float gainReduction = computeLimiterGain(level);
                samples[i] *= gainReduction;
            }
        }

        oversampler.processSamplesDown(block);
    }

private:
    juce::dsp::Oversampling<float> oversampler{2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR};

    float computeLimiterGain(float level)
    {
        float ceiling = juce::Decibels::decibelsToGain(ceilingdB);
        if (level > ceiling)
            return ceiling / level;
        return 1.0f;
    }

    float ceilingdB = -0.3f; // Standard mastering ceiling
};
```

### 11.3 Release Shaping for Transparent Limiting

The release envelope of a limiter is critical for transparency:
- **Linear release**: Simple but can cause distortion on low-frequency content
- **Logarithmic release**: More natural but can be too slow
- **Adaptive release**: Best transparency -- fast release for transients, slow for sustained signals

```cpp
float adaptiveRelease(float gainReduction, float prevGainReduction)
{
    float gr = std::abs(gainReduction);

    // Short transient: fast release (10 ms)
    // Sustained reduction: slow release (100 ms)
    float releaseFactor = juce::jmap(
        juce::jlimit(0.0f, 12.0f, gr),
        0.0f, 12.0f,
        0.01f, 0.1f); // seconds

    return releaseFactor;
}
```

---

## 12. Parallel Compression

Parallel compression (also called "New York compression") blends uncompressed signal with heavily compressed signal:

```cpp
void processParallelCompression(juce::AudioBuffer<float>& buffer, float mixAmount)
{
    // Copy dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Apply heavy compression to wet signal
    compressor.process(buffer); // heavy ratio, fast attack

    // Blend dry and wet
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wet = buffer.getWritePointer(ch);
        const auto* dry = dryBuffer.getReadPointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            wet[i] = dry[i] * (1.0f - mixAmount) + wet[i] * mixAmount;
        }
    }
}
```

**Benefits of parallel compression:**
- Preserves transient detail from the dry signal
- Adds density and sustain from the compressed signal
- Avoids the "over-compressed" sound of heavy direct compression
- Particularly effective on drums and vocals

---

## Part 4: JUCE Implementation

## 13. JUCE DSP Module Integration

### 13.1 juce::dsp::Compressor

JUCE provides a basic compressor in the DSP module:

```cpp
#include <juce_dsp/juce_dsp.h>

class SimpleCompressorPlugin : public juce::AudioProcessor
{
    juce::dsp::Compressor<float> compressor;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
        spec.numChannels = (juce::uint32)getTotalNumOutputChannels();

        compressor.prepare(spec);
        compressor.setThreshold(-20.0f);
        compressor.setRatio(4.0f);
        compressor.setAttack(10.0f);
        compressor.setRelease(100.0f);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto context = juce::dsp::ProcessContextReplacing<float>(block);
        compressor.process(context);
    }
};
```

### 13.2 Limitations of juce::dsp::Compressor

The built-in compressor is limited:
- No sidechain input support
- No soft knee option
- Fixed peak detection (no RMS)
- No look-ahead
- No feed-back topology option

For production plugins (O-Comp, O-MultiBandCompressor), custom implementations are essential.

---

## 14. Complete Compressor Implementation

### 14.1 Full-Featured Compressor Class

```cpp
class CustomCompressor
{
public:
    struct Parameters
    {
        float thresholddB = -20.0f;
        float ratio = 4.0f;
        float attackMs = 10.0f;
        float releaseMs = 100.0f;
        float kneedB = 6.0f;
        float makeupGaindB = 0.0f;
        bool autoMakeup = false;
        bool rmsDetection = false;
        float lookAheadMs = 0.0f;
    };

    void prepare(double sampleRate, int blockSize)
    {
        sr = sampleRate;
        updateCoefficients();

        if (params.lookAheadMs > 0.0f)
        {
            int delaySamples = static_cast<int>(params.lookAheadMs * 0.001 * sr);
            delayBuffer.resize(delaySamples, 0.0f);
            delayWritePos = 0;
        }

        gainSmoother.prepare(sampleRate);
    }

    float processSample(float input)
    {
        // Level detection
        float level;
        if (params.rmsDetection)
            level = rmsDetector.process(input);
        else
            level = peakDetector.process(input);

        // Convert to dB
        float leveldB = juce::Decibels::gainToDecibels(level, -100.0f);

        // Gain computation with soft knee
        float gainReductiondB = computeSoftKneeGain(leveldB);

        // Smooth gain changes
        float smoothedGain = gainSmoother.process(
            juce::Decibels::decibelsToGain(gainReductiondB));

        // Auto makeup gain
        float makeup = params.autoMakeup
            ? computeAutoMakeup()
            : juce::Decibels::decibelsToGain(params.makeupGaindB);

        // Apply look-ahead delay if enabled
        float audioSample = input;
        if (!delayBuffer.empty())
        {
            audioSample = delayBuffer[delayWritePos];
            delayBuffer[delayWritePos] = input;
            delayWritePos = (delayWritePos + 1) % (int)delayBuffer.size();
        }

        currentGainReduction = gainReductiondB;
        return audioSample * smoothedGain * makeup;
    }

    float getGainReduction() const { return currentGainReduction; }

    Parameters params;

private:
    double sr = 44100.0;
    float currentGainReduction = 0.0f;
    PeakDetector peakDetector;
    RMSDetector rmsDetector;
    GainSmoother gainSmoother;
    std::vector<float> delayBuffer;
    int delayWritePos = 0;

    float computeSoftKneeGain(float inputdB)
    {
        float T = params.thresholddB;
        float R = params.ratio;
        float W = params.kneedB;
        float halfKnee = W * 0.5f;

        float outputdB;

        if (inputdB < T - halfKnee)
            outputdB = inputdB;
        else if (inputdB > T + halfKnee)
            outputdB = T + (inputdB - T) / R;
        else
        {
            float x = inputdB - T + halfKnee;
            outputdB = inputdB + ((1.0f / R) - 1.0f) * x * x / (2.0f * W);
        }

        return outputdB - inputdB; // Return gain reduction in dB
    }

    float computeAutoMakeup()
    {
        // Approximate makeup = -(threshold * (1 - 1/ratio)) / 2
        float makeupdB = -(params.thresholddB * (1.0f - 1.0f / params.ratio)) * 0.5f;
        return juce::Decibels::decibelsToGain(makeupdB);
    }

    void updateCoefficients()
    {
        peakDetector.prepare(sr, params.attackMs, params.releaseMs);
        rmsDetector.prepare(sr, 50.0f); // 50ms RMS window
    }
};
```

### 14.2 Gain Reduction Metering

For UI display, expose gain reduction as a thread-safe atomic:

```cpp
class CompressorProcessor : public juce::AudioProcessor
{
    std::atomic<float> gainReductionForUI{0.0f};

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        // Process...
        float maxGR = 0.0f;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // ... process sample
            maxGR = std::min(maxGR, compressor.getGainReduction());
        }
        gainReductionForUI.store(maxGR, std::memory_order_relaxed);
    }
};
```

---

## 15. Real-Time Safety and Optimization

### 15.1 SIMD Processing

Use JUCE's SIMD support for processing multiple samples simultaneously:

```cpp
void processBlockSIMD(juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();
    constexpr int simdSize = juce::dsp::SIMDRegister<float>::size();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i <= numSamples - simdSize; i += simdSize)
        {
            auto samples = juce::dsp::SIMDRegister<float>::fromRawArray(data + i);
            auto abs = samples.abs();
            // Vectorized level detection...
            abs.copyToRawArray(data + i);
        }
    }
}
```

### 15.2 Coefficient Update Strategy

Avoid recomputing filter coefficients every sample:

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
{
    // Update parameters once per block (not per sample)
    if (parametersChanged.exchange(false))
    {
        compressor.params.thresholddB = thresholdParam->load();
        compressor.params.ratio = ratioParam->load();
        // etc.
    }

    // Process entire block
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        // Per-sample processing with cached coefficients
    }
}
```

### 15.3 Memory Allocation

Never allocate memory in `processBlock()`:

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    // Pre-allocate all buffers
    sidechainBuffer.setSize(2, samplesPerBlock);
    bandBuffers[0].setSize(2, samplesPerBlock);
    bandBuffers[1].setSize(2, samplesPerBlock);
    bandBuffers[2].setSize(2, samplesPerBlock);

    setLatencySamples(lookAheadSamples);
}
```

### 15.4 Common Pitfalls

| Pitfall | Impact | Solution |
|---------|--------|----------|
| Denormal numbers | CPU spikes when signal is near zero | Add DC offset (1e-15f) or use `juce::FloatVectorOperations::disableDenormalisedNumberSupport()` |
| Division by zero | Crash in ratio computation | Guard ratio with `juce::jmax(ratio, 1.001f)` |
| Zipper noise | Audible stepping when changing parameters | Smooth all parameter changes with 1-pole filter |
| Log of zero | NaN in dB conversion | Use `juce::Decibels::gainToDecibels(level, -100.0f)` with minimum floor |
| Phase cancellation in multiband | Hollow sound at crossover frequencies | Use Linkwitz-Riley crossovers, verify flat sum |

---

## Part 5: References

## 16. References and Further Reading

### Academic Papers
- Giannoulis, D., Massberg, M., & Reiss, J.D. (2012). "Digital Dynamic Range Compressor Design -- A Tutorial and Analysis." JAES, 60(6).
- McNally, G.W. (1984). "Dynamic Range Control of Digital Audio Signals." JAES, 32(5).
- Zolzer, U. (2011). *DAFX: Digital Audio Effects*. Chapter 4: Dynamic Range Control.

### Books
- Giannoulis et al., "Parameter Automation in a Dynamic Range Compressor," JAES 2013
- Reiss, J.D. & McPherson, A. (2015). *Audio Effects: Theory, Implementation and Application*. CRC Press.
- Pirkle, W. (2019). *Designing Audio Effect Plugins in C++*. Chapters 15-17.

### Standards
- ITU-R BS.1770-4: Algorithms to measure audio programme loudness and true-peak audio level
- AES17: Standard for measurement of digital audio equipment (peak level definitions)
- EBU R128: Loudness normalisation and permitted maximum level of audio signals

### Hardware References
- UREI 1176 (FET), Teletronix LA-2A (Optical), Fairchild 670 (Vari-Mu), SSL G-series Bus Compressor (VCA)
- API 2500 (VCA), Neve 33609 (Diode Bridge), Tube-Tech CL 1B (Optical)

### JUCE Resources
- `juce::dsp::Compressor<float>` source in `juce_dsp/processors/juce_Compressor.h`
- `juce::dsp::LinkwitzRileyFilter` for multiband crossover design
- `juce::dsp::Oversampling` for true peak detection

---

*Research document for O-Comp and O-MultiBandCompressor. Covers VCA, FET, optical, and vari-mu topologies with JUCE implementation patterns.*
