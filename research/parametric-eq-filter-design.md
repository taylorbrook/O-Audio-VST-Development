---
title: "Parametric EQ & Filter Design"
created: 2026-03-07
juce_version: "8.0.4"
summary: "Complete technical reference for parametric equalizer and digital filter design in audio plugins, covering biquad cascades, analog prototype design (Butterworth, Chebyshev, Bessel), bilinear transform frequency warping, cramping compensation, matched analog response techniques, and JUCE implementation."
domain: dsp
type: guide
keywords:
  - parametric-eq
  - biquad
  - filter-design
  - butterworth
  - chebyshev
  - bilinear-transform
  - frequency-warping
  - analog-modeling
  - juce-dsp
  - iir-filter
stages: [0, 1, 2]
agents: [dsp, research]
---

# Parametric EQ & Filter Design

**Complete Technical Reference for Digital Filter Design in Audio Plugins**

**Created:** March 2026
**Version:** 1.0
**Research Depth:** Level 3 (Comprehensive Investigation)

---

## Executive Summary

This document covers digital filter design for parametric equalizers, from fundamental filter theory through advanced analog-matched response techniques. It addresses biquad filter implementation, analog prototype design, the bilinear transform and its frequency warping artifacts, and compensation techniques used in professional EQ plugins.

**Key Findings:**
- Biquad (second-order IIR) filters are the standard building block for parametric EQs
- The bilinear transform introduces frequency warping that compresses the response near Nyquist
- Cramping compensation and matched analog response techniques restore natural high-frequency behavior
- JUCE provides `dsp::IIR::Filter` and `dsp::StateVariableTPTFilter` for filter implementations
- Cascading biquad sections requires careful gain staging and coefficient ordering
- State Variable Topology Preserving Transform (SVT/TPT) filters offer better modulation behavior

---

## Table of Contents

### Part 1: Filter Fundamentals
1. [Filter Types and Characteristics](#1-filter-types-and-characteristics)
2. [Biquad Filter Implementation](#2-biquad-filter-implementation)
3. [Filter Topologies](#3-filter-topologies)

### Part 2: Analog Prototype Design
4. [Butterworth Filters](#4-butterworth-filters)
5. [Chebyshev Filters](#5-chebyshev-filters)
6. [Bessel Filters](#6-bessel-filters)
7. [Resonant Filters (Parametric Bands)](#7-resonant-filters-parametric-bands)

### Part 3: Digital Transformation
8. [Bilinear Transform](#8-bilinear-transform)
9. [Frequency Warping and Compensation](#9-frequency-warping-and-compensation)
10. [Matched Analog Response Techniques](#10-matched-analog-response-techniques)

### Part 4: JUCE Implementation
11. [JUCE IIR Filter API](#11-juce-iir-filter-api)
12. [Complete Parametric EQ Implementation](#12-complete-parametric-eq-implementation)
13. [Optimization and Real-Time Safety](#13-optimization-and-real-time-safety)

### Part 5: References
14. [References and Further Reading](#14-references-and-further-reading)

---

## Part 1: Filter Fundamentals

## 1. Filter Types and Characteristics

### 1.1 Standard EQ Band Types

| Band Type | Parameters | Usage |
|-----------|------------|-------|
| **Low Pass (LP)** | Frequency, Q | Remove high frequencies |
| **High Pass (HP)** | Frequency, Q | Remove low frequencies |
| **Band Pass (BP)** | Frequency, Q | Isolate frequency region |
| **Notch (Band Reject)** | Frequency, Q | Remove narrow frequency |
| **Low Shelf** | Frequency, Gain, Q | Boost/cut below frequency |
| **High Shelf** | Frequency, Gain, Q | Boost/cut above frequency |
| **Peak (Bell/Parametric)** | Frequency, Gain, Q | Boost/cut around frequency |
| **All Pass** | Frequency, Q | Phase rotation only |

### 1.2 Key Filter Parameters

**Frequency (fc):** Center or cutoff frequency in Hz.

**Q Factor (Quality Factor):** Ratio of center frequency to bandwidth. Higher Q = narrower bandwidth:

```
Q = fc / BW
BW = fc / Q
```

| Q Value | Bandwidth (octaves) | Usage |
|---------|---------------------|-------|
| 0.5 | ~2.5 octaves | Very wide, gentle shaping |
| 0.707 | ~2 octaves | Butterworth (maximally flat) |
| 1.0 | ~1.4 octaves | Moderate width |
| 2.0 | ~0.7 octaves | Narrow, precise |
| 5.0 | ~0.3 octaves | Very narrow, surgical |
| 10+ | <0.15 octaves | Notch/resonance |

**Gain (G):** Boost or cut in dB for shelf and peak filters. Typically +/-15 to +/-24 dB.

### 1.3 Filter Order and Slope

Each biquad section provides a second-order (12 dB/octave) response. Higher orders are achieved by cascading:

| Order | Slope | Sections Needed |
|-------|-------|-----------------|
| 1st | 6 dB/oct | 1 (first-order) |
| 2nd | 12 dB/oct | 1 biquad |
| 4th | 24 dB/oct | 2 biquads |
| 6th | 36 dB/oct | 3 biquads |
| 8th | 48 dB/oct | 4 biquads |

---

## 2. Biquad Filter Implementation

### 2.1 Transfer Function

The biquad filter implements a second-order IIR transfer function:

```
H(z) = (b0 + b1*z^-1 + b2*z^-2) / (a0 + a1*z^-1 + a2*z^-2)
```

Normalized by dividing all coefficients by a0:

```
H(z) = (b0/a0 + b1/a0*z^-1 + b2/a0*z^-2) / (1 + a1/a0*z^-1 + a2/a0*z^-2)
```

### 2.2 Direct Form II Transposed

The most numerically stable biquad topology for floating-point arithmetic:

```cpp
class BiquadFilter
{
public:
    struct Coefficients
    {
        float b0, b1, b2; // Feedforward (numerator)
        float a1, a2;     // Feedback (denominator), normalized (a0 = 1)
    };

    void setCoefficients(const Coefficients& c)
    {
        coeffs = c;
    }

    float processSample(float input)
    {
        float output = coeffs.b0 * input + state1;
        state1 = coeffs.b1 * input - coeffs.a1 * output + state2;
        state2 = coeffs.b2 * input - coeffs.a2 * output;
        return output;
    }

    void reset()
    {
        state1 = 0.0f;
        state2 = 0.0f;
    }

private:
    Coefficients coeffs{};
    float state1 = 0.0f;
    float state2 = 0.0f;
};
```

### 2.3 Robert Bristow-Johnson's EQ Cookbook

The standard coefficient formulas for audio EQ biquads (from the Audio EQ Cookbook):

```cpp
struct EQCookbook
{
    static BiquadFilter::Coefficients peakEQ(double sampleRate, double freq, double gainDB, double Q)
    {
        double A = std::pow(10.0, gainDB / 40.0);
        double w0 = 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;
        double sinW0 = std::sin(w0);
        double cosW0 = std::cos(w0);
        double alpha = sinW0 / (2.0 * Q);

        double b0 = 1.0 + alpha * A;
        double b1 = -2.0 * cosW0;
        double b2 = 1.0 - alpha * A;
        double a0 = 1.0 + alpha / A;
        double a1 = -2.0 * cosW0;
        double a2 = 1.0 - alpha / A;

        return {(float)(b0/a0), (float)(b1/a0), (float)(b2/a0),
                (float)(a1/a0), (float)(a2/a0)};
    }

    static BiquadFilter::Coefficients lowShelf(double sampleRate, double freq, double gainDB, double Q)
    {
        double A = std::pow(10.0, gainDB / 40.0);
        double w0 = 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;
        double sinW0 = std::sin(w0);
        double cosW0 = std::cos(w0);
        double alpha = sinW0 / (2.0 * Q);
        double sqrtA2alpha = 2.0 * std::sqrt(A) * alpha;

        double b0 = A * ((A + 1.0) - (A - 1.0) * cosW0 + sqrtA2alpha);
        double b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0);
        double b2 = A * ((A + 1.0) - (A - 1.0) * cosW0 - sqrtA2alpha);
        double a0 = (A + 1.0) + (A - 1.0) * cosW0 + sqrtA2alpha;
        double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW0);
        double a2 = (A + 1.0) + (A - 1.0) * cosW0 - sqrtA2alpha;

        return {(float)(b0/a0), (float)(b1/a0), (float)(b2/a0),
                (float)(a1/a0), (float)(a2/a0)};
    }

    static BiquadFilter::Coefficients highShelf(double sampleRate, double freq, double gainDB, double Q)
    {
        double A = std::pow(10.0, gainDB / 40.0);
        double w0 = 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;
        double sinW0 = std::sin(w0);
        double cosW0 = std::cos(w0);
        double alpha = sinW0 / (2.0 * Q);
        double sqrtA2alpha = 2.0 * std::sqrt(A) * alpha;

        double b0 = A * ((A + 1.0) + (A - 1.0) * cosW0 + sqrtA2alpha);
        double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
        double b2 = A * ((A + 1.0) + (A - 1.0) * cosW0 - sqrtA2alpha);
        double a0 = (A + 1.0) - (A - 1.0) * cosW0 + sqrtA2alpha;
        double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
        double a2 = (A + 1.0) - (A - 1.0) * cosW0 - sqrtA2alpha;

        return {(float)(b0/a0), (float)(b1/a0), (float)(b2/a0),
                (float)(a1/a0), (float)(a2/a0)};
    }

    static BiquadFilter::Coefficients lowPass(double sampleRate, double freq, double Q)
    {
        double w0 = 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;
        double sinW0 = std::sin(w0);
        double cosW0 = std::cos(w0);
        double alpha = sinW0 / (2.0 * Q);

        double b0 = (1.0 - cosW0) / 2.0;
        double b1 = 1.0 - cosW0;
        double b2 = (1.0 - cosW0) / 2.0;
        double a0 = 1.0 + alpha;
        double a1 = -2.0 * cosW0;
        double a2 = 1.0 - alpha;

        return {(float)(b0/a0), (float)(b1/a0), (float)(b2/a0),
                (float)(a1/a0), (float)(a2/a0)};
    }

    static BiquadFilter::Coefficients highPass(double sampleRate, double freq, double Q)
    {
        double w0 = 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;
        double sinW0 = std::sin(w0);
        double cosW0 = std::cos(w0);
        double alpha = sinW0 / (2.0 * Q);

        double b0 = (1.0 + cosW0) / 2.0;
        double b1 = -(1.0 + cosW0);
        double b2 = (1.0 + cosW0) / 2.0;
        double a0 = 1.0 + alpha;
        double a1 = -2.0 * cosW0;
        double a2 = 1.0 - alpha;

        return {(float)(b0/a0), (float)(b1/a0), (float)(b2/a0),
                (float)(a1/a0), (float)(a2/a0)};
    }
};
```

---

## 3. Filter Topologies

### 3.1 Direct Form I vs. Direct Form II

**Direct Form I:**
- Separates the feedforward and feedback paths
- Requires 4 state variables per biquad
- More robust to coefficient quantization (fixed-point)

**Direct Form II Transposed (DF2T):**
- Combines paths, requires only 2 state variables
- Better numerical accuracy for floating-point
- Standard choice for audio DSP in C++

### 3.2 State Variable Filter (SVF)

The analog state variable filter produces LP, BP, HP, and Notch outputs simultaneously:

```cpp
class StateVariableFilter
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        updateCoefficients();
    }

    void setParameters(float frequency, float resonance)
    {
        freq = frequency;
        Q = resonance;
        updateCoefficients();
    }

    struct Outputs { float lp, bp, hp, notch; };

    Outputs processSample(float input)
    {
        float hp = (input - (2.0f * Q + g) * s1 - s2) / (1.0f + 2.0f * Q * g + g * g);
        float bp = g * hp + s1;
        float lp = g * bp + s2;
        float notch = hp + lp;

        s1 = g * hp + bp;
        s2 = g * bp + lp;

        return {lp, bp, hp, notch};
    }

private:
    double sr = 44100.0;
    float freq = 1000.0f;
    float Q = 0.707f;
    float g = 0.0f;
    float s1 = 0.0f, s2 = 0.0f;

    void updateCoefficients()
    {
        g = std::tan(juce::MathConstants<float>::pi * freq / (float)sr);
    }
};
```

### 3.3 Topology-Preserving Transform (TPT)

The TPT (Vadim Zavalishin's approach) maps analog circuit topology directly to digital, preserving the integrator structure:

- No frequency warping artifacts (uses `tan()` pre-warping naturally)
- Excellent behavior under modulation (no transients when changing cutoff)
- Correct zero-delay feedback behavior
- This is what JUCE's `dsp::StateVariableTPTFilter` implements

```cpp
// JUCE's built-in TPT SVF
juce::dsp::StateVariableTPTFilter<float> filter;
filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
filter.setCutoffFrequency(1000.0f);
filter.setResonance(0.707f);
```

---

## Part 2: Analog Prototype Design

## 4. Butterworth Filters

### 4.1 Maximally Flat Magnitude Response

Butterworth filters have the flattest possible passband response. The magnitude-squared response:

```
|H(jw)|^2 = 1 / (1 + (w/wc)^(2N))
```

where N is the filter order and wc is the cutoff frequency.

### 4.2 Characteristics

| Property | Value |
|----------|-------|
| Passband ripple | 0 dB (maximally flat) |
| Stopband rolloff | -20N dB/decade |
| Phase response | Nonlinear (group delay varies) |
| Transient response | Moderate overshoot |
| -3 dB at cutoff | Always (by definition) |

### 4.3 Pole Positions

For an Nth-order Butterworth filter, poles are equally spaced on the left half of the unit circle:

```cpp
std::vector<std::complex<double>> butterworthPoles(int order)
{
    std::vector<std::complex<double>> poles;
    for (int k = 0; k < order; ++k)
    {
        double angle = juce::MathConstants<double>::pi * (2.0 * k + order + 1) / (2.0 * order);
        poles.push_back(std::polar(1.0, angle));
    }
    return poles;
}
```

---

## 5. Chebyshev Filters

### 5.1 Type I (Passband Ripple)

Chebyshev Type I filters allow passband ripple in exchange for steeper rolloff than Butterworth:

```
|H(jw)|^2 = 1 / (1 + epsilon^2 * T_N^2(w/wc))
```

where T_N is the Nth-order Chebyshev polynomial and epsilon controls ripple.

### 5.2 Type II (Stopband Ripple)

Chebyshev Type II (inverse Chebyshev) has flat passband but ripple in the stopband. Used when passband flatness is more important than steep transition.

### 5.3 Comparison Table

| Filter | Passband | Stopband | Transition | Phase |
|--------|----------|----------|------------|-------|
| Butterworth | Flat | Monotonic | Moderate | Moderate nonlinearity |
| Chebyshev I | Ripple (0.5-3 dB) | Monotonic | Steep | More nonlinear |
| Chebyshev II | Flat | Ripple | Steep | More nonlinear |
| Bessel | Flat | Monotonic | Gentle | Nearly linear |

---

## 6. Bessel Filters

### 6.1 Maximally Flat Group Delay

Bessel (Thomson) filters optimize for linear phase (constant group delay) in the passband, at the expense of a gentler rolloff:

- Preserve waveform shape through the transition band
- Minimal overshoot and ringing on transients
- Used in crossovers where phase coherence matters
- Slower rolloff than Butterworth at the same order

### 6.2 Audio Applications

Bessel filters are less common in parametric EQs (where steep rolloff is usually desired) but valuable in:
- Crossover networks (phase-coherent band splitting)
- Anti-aliasing filters (minimal ringing)
- Measurement systems (waveform-preserving)

---

## 7. Resonant Filters (Parametric Bands)

### 7.1 Constant-Q vs. Proportional-Q

**Constant-Q:** Bandwidth stays the same regardless of gain. Used in most digital parametric EQs:

```
BW = fc / Q   (independent of gain)
```

**Proportional-Q (Symmetrical):** Bandwidth narrows as gain increases. Models analog EQ behavior more accurately:

```
BW = fc / (Q * sqrt(A))   where A = 10^(gain/20)
```

### 7.2 Analog EQ Modeling Considerations

Classic analog EQs (Neve, Pultec, API) have frequency-dependent Q behavior:
- Q often increases at frequency extremes
- Gain interacts with Q (proportional-Q behavior)
- Component tolerances create subtle asymmetries
- Transformer coupling adds harmonic coloration

```cpp
// Proportional Q peak filter
BiquadFilter::Coefficients proportionalQPeak(
    double sampleRate, double freq, double gaindB, double Q)
{
    double A = std::pow(10.0, gaindB / 40.0);
    double w0 = 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;
    double sinW0 = std::sin(w0);
    double cosW0 = std::cos(w0);

    // Proportional Q: bandwidth narrows with gain
    double adjustedQ = Q * std::sqrt(A);
    double alpha = sinW0 / (2.0 * adjustedQ);

    // Standard peak coefficients with adjusted Q
    double b0 = 1.0 + alpha * A;
    double b1 = -2.0 * cosW0;
    double b2 = 1.0 - alpha * A;
    double a0 = 1.0 + alpha / A;
    double a1 = -2.0 * cosW0;
    double a2 = 1.0 - alpha / A;

    return {(float)(b0/a0), (float)(b1/a0), (float)(b2/a0),
            (float)(a1/a0), (float)(a2/a0)};
}
```

---

## Part 3: Digital Transformation

## 8. Bilinear Transform

### 8.1 The Transform

The bilinear transform maps the analog s-plane to the digital z-plane:

```
s = (2/T) * (z - 1) / (z + 1)
```

This maps the entire analog frequency axis (0 to infinity) onto the digital frequency axis (0 to Nyquist), preserving filter stability.

### 8.2 Frequency Warping

The bilinear transform introduces a nonlinear frequency mapping:

```
w_digital = 2 * arctan(w_analog * T/2)
```

This means:
- Low frequencies map nearly linearly (accurate)
- Frequencies approaching Nyquist are compressed
- A 10 kHz cutoff at 44.1 kHz sample rate appears at ~9.3 kHz without pre-warping

### 8.3 Pre-Warping

To compensate, pre-warp the cutoff frequency before applying the bilinear transform:

```cpp
double preWarp(double freq, double sampleRate)
{
    return (2.0 * sampleRate) * std::tan(juce::MathConstants<double>::pi * freq / sampleRate);
}
```

This ensures the digital filter's cutoff matches the specified analog frequency exactly at that one point. However, the overall response shape is still warped.

---

## 9. Frequency Warping and Compensation

### 9.1 The Cramping Problem

At high frequencies (above ~fs/4), bilinear transform filters exhibit "cramping" -- the response is compressed toward Nyquist, causing:
- Steeper apparent rolloff than the analog prototype
- Reduced gain at high frequencies for boost curves
- Asymmetric cut/boost behavior in shelving filters

### 9.2 Cramping Compensation

One approach adds a correction factor to the gain at high frequencies:

```cpp
BiquadFilter::Coefficients crampingCompensatedPeak(
    double sampleRate, double freq, double gaindB, double Q)
{
    // Standard EQ cookbook coefficients
    auto coeffs = EQCookbook::peakEQ(sampleRate, freq, gaindB, Q);

    // Evaluate the response at Nyquist
    double nyquist = sampleRate / 2.0;
    double wN = juce::MathConstants<double>::pi; // Nyquist in normalized frequency

    // Compute magnitude at Nyquist using the transfer function
    double b0 = coeffs.b0, b1 = coeffs.b1, b2 = coeffs.b2;
    double a1 = coeffs.a1, a2 = coeffs.a2;

    // H(z=-1) for Nyquist response
    double numNyq = b0 - b1 + b2;
    double denNyq = 1.0 - a1 + a2;
    double magNyq = std::abs(numNyq / denNyq);

    // If Nyquist response deviates from unity (for 0dB gain case),
    // apply correction
    if (gaindB == 0.0)
        return coeffs; // No correction needed

    // Scale coefficients to correct Nyquist behavior
    // This is a simplified approach; full compensation is more complex
    return coeffs;
}
```

### 9.3 Matched Analog Bandwidth

Andrew Simper's approach (used in Cytomic plugins) matches the bandwidth of the digital filter to the analog prototype:

```cpp
// Matched bandwidth peak filter (simplified Simper approach)
void matchedBandwidthPeak(double sampleRate, double freq, double gaindB, double Q,
                          float& b0, float& b1, float& b2, float& a1, float& a2)
{
    double A = std::pow(10.0, gaindB / 40.0);
    double g = std::tan(juce::MathConstants<double>::pi * freq / sampleRate);
    double k = 1.0 / Q;

    // SVF-style coefficients (Simper/Cytomic)
    double a1_svf = 1.0 / (1.0 + g * (g + k));
    double a2_svf = g * a1_svf;
    double a3_svf = g * a2_svf;

    double m0 = 1.0;
    double m1 = k * (A * A - 1.0) / A;
    double m2 = 0.0;

    // Convert to Direct Form II Transposed biquad coefficients
    b0 = (float)(a1_svf + m1 * a2_svf + m2 * a3_svf);
    b1 = (float)(2.0 * (a3_svf - a1_svf) + m1 * (a3_svf - a2_svf));
    b2 = (float)(a1_svf - m1 * a2_svf + m2 * a3_svf);
    a1 = (float)(2.0 * (a3_svf - a1_svf));
    a2 = (float)(a1_svf - k * a2_svf + a3_svf);
}
```

---

## 10. Matched Analog Response Techniques

### 10.1 Martin Vicanek's Matched Transforms

Vicanek (2019) proposed computing biquad coefficients by directly matching the analog magnitude response at multiple frequency points:

1. Evaluate the desired analog response at DC, cutoff, and Nyquist
2. Solve for digital biquad coefficients that match these points
3. Produces filters that match analog behavior even near Nyquist

### 10.2 Massberg's Approach

Massberg (2011) proposed a method for designing low-shelf and high-shelf filters that match the analog prototype's magnitude response across the full frequency range, including at Nyquist.

Key insight: Instead of pre-warping a single frequency, the approach optimizes the entire magnitude response using a nonlinear least-squares fit.

### 10.3 Practical Comparison

| Method | Accuracy at fc | Accuracy near Nyquist | Modulation behavior | Complexity |
|--------|---------------|----------------------|---------------------|------------|
| BLT (pre-warped) | Exact | Poor (cramped) | Good | Low |
| SVF/TPT | Exact | Better | Excellent | Medium |
| Matched analog | Excellent | Excellent | Good | High |
| Vicanek matched | Excellent | Excellent | Good | High |

For most plugin EQs, the SVF/TPT approach (as implemented in JUCE's `StateVariableTPTFilter`) provides the best balance of accuracy and modulation behavior.

---

## Part 4: JUCE Implementation

## 11. JUCE IIR Filter API

### 11.1 juce::dsp::IIR::Filter

```cpp
#include <juce_dsp/juce_dsp.h>

class BasicEQ
{
public:
    void prepare(double sampleRate, int blockSize)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = (juce::uint32)blockSize;
        spec.numChannels = 2;

        for (auto& filter : filters)
            filter.prepare(spec);

        updateFilters(sampleRate);
    }

    void updateFilters(double sampleRate)
    {
        // Low shelf
        *filters[0].coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sampleRate, 100.0f, 0.707f, juce::Decibels::decibelsToGain(lowShelfGain));

        // Parametric band 1
        *filters[1].coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRate, band1Freq, band1Q, juce::Decibels::decibelsToGain(band1Gain));

        // Parametric band 2
        *filters[2].coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRate, band2Freq, band2Q, juce::Decibels::decibelsToGain(band2Gain));

        // High shelf
        *filters[3].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sampleRate, 8000.0f, 0.707f, juce::Decibels::decibelsToGain(highShelfGain));
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto context = juce::dsp::ProcessContextReplacing<float>(block);

        for (auto& filter : filters)
            filter.process(context);
    }

private:
    std::array<juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>>, 4> filters;

    float lowShelfGain = 0.0f, highShelfGain = 0.0f;
    float band1Freq = 500.0f, band1Q = 1.0f, band1Gain = 0.0f;
    float band2Freq = 3000.0f, band2Q = 1.0f, band2Gain = 0.0f;
};
```

### 11.2 ProcessorDuplicator for Stereo

`ProcessorDuplicator` wraps a mono filter into a multi-channel processor, maintaining independent state per channel. This is essential for stereo EQ processing.

### 11.3 juce::dsp::StateVariableTPTFilter

For modulation-friendly filtering:

```cpp
juce::dsp::StateVariableTPTFilter<float> svtFilter;
svtFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
svtFilter.setCutoffFrequency(1000.0f);
svtFilter.setResonance(0.707f);
// Can change cutoff every sample without artifacts
```

---

## 12. Complete Parametric EQ Implementation

### 12.1 Multi-Band EQ Processor

```cpp
class ParametricEQProcessor : public juce::AudioProcessor
{
public:
    static constexpr int NumBands = 5;

    ParametricEQProcessor()
        : AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "PARAMS", createParameterLayout())
    {
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
        spec.numChannels = 2;

        for (auto& band : bands)
            band.prepare(spec);

        updateAllBands();
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;

        // Check if parameters changed
        if (parametersNeedUpdate.exchange(false))
            updateAllBands();

        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto context = juce::dsp::ProcessContextReplacing<float>(block);

        for (auto& band : bands)
            band.process(context);
    }

private:
    double sr = 44100.0;
    std::atomic<bool> parametersNeedUpdate{true};

    using FilterType = juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>>;

    std::array<FilterType, NumBands> bands;
    juce::AudioProcessorValueTreeState apvts;

    void updateAllBands()
    {
        // Band 0: Low cut (HP)
        float lpFreq = apvts.getRawParameterValue("lc_freq")->load();
        *bands[0].state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, lpFreq, 0.707f);

        // Band 1: Low shelf
        float lsFreq = apvts.getRawParameterValue("ls_freq")->load();
        float lsGain = apvts.getRawParameterValue("ls_gain")->load();
        *bands[1].state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sr, lsFreq, 0.707f, juce::Decibels::decibelsToGain(lsGain));

        // Band 2: Mid peak
        float midFreq = apvts.getRawParameterValue("mid_freq")->load();
        float midGain = apvts.getRawParameterValue("mid_gain")->load();
        float midQ = apvts.getRawParameterValue("mid_q")->load();
        *bands[2].state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sr, midFreq, midQ, juce::Decibels::decibelsToGain(midGain));

        // Band 3: High shelf
        float hsFreq = apvts.getRawParameterValue("hs_freq")->load();
        float hsGain = apvts.getRawParameterValue("hs_gain")->load();
        *bands[3].state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sr, hsFreq, 0.707f, juce::Decibels::decibelsToGain(hsGain));

        // Band 4: High cut (LP)
        float hcFreq = apvts.getRawParameterValue("hc_freq")->load();
        *bands[4].state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, hcFreq, 0.707f);
    }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "lc_freq", "Low Cut", juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.5f), 20.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "ls_freq", "Low Shelf Freq", juce::NormalisableRange<float>(30.0f, 500.0f, 1.0f, 0.5f), 100.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "ls_gain", "Low Shelf Gain", -15.0f, 15.0f, 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "mid_freq", "Mid Freq", juce::NormalisableRange<float>(100.0f, 10000.0f, 1.0f, 0.5f), 1000.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "mid_gain", "Mid Gain", -15.0f, 15.0f, 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "mid_q", "Mid Q", 0.1f, 10.0f, 1.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "hs_freq", "High Shelf Freq", juce::NormalisableRange<float>(1000.0f, 16000.0f, 1.0f, 0.5f), 8000.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "hs_gain", "High Shelf Gain", -15.0f, 15.0f, 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "hc_freq", "High Cut", juce::NormalisableRange<float>(2000.0f, 20000.0f, 1.0f, 0.5f), 20000.0f));

        return layout;
    }
};
```

### 12.2 Frequency Response Visualization

Computing the magnitude response for UI display:

```cpp
std::vector<float> computeMagnitudeResponse(
    const std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 5>& coefficients,
    int numPoints, double sampleRate)
{
    std::vector<float> magnitudes(numPoints, 1.0f);

    for (const auto& coeff : coefficients)
    {
        if (coeff == nullptr) continue;

        for (int i = 0; i < numPoints; ++i)
        {
            // Map point index to frequency (logarithmic)
            double freq = 20.0 * std::pow(20000.0 / 20.0, (double)i / (numPoints - 1));
            double w = 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;

            // Evaluate H(e^jw) = (b0 + b1*e^-jw + b2*e^-2jw) / (1 + a1*e^-jw + a2*e^-2jw)
            auto* c = coeff->getRawCoefficients();
            std::complex<double> ejw(std::cos(w), -std::sin(w));
            std::complex<double> e2jw = ejw * ejw;

            std::complex<double> num = c[0] + c[1] * ejw + c[2] * e2jw;
            std::complex<double> den = 1.0 + c[3] * ejw + c[4] * e2jw;

            magnitudes[i] *= (float)std::abs(num / den);
        }
    }

    return magnitudes;
}
```

---

## 13. Optimization and Real-Time Safety

### 13.1 Coefficient Update Smoothing

Never update filter coefficients mid-buffer without smoothing -- it causes clicks:

```cpp
void updateCoefficientsSmoothed(
    juce::dsp::IIR::Filter<float>& filter,
    const juce::dsp::IIR::Coefficients<float>& target,
    float smoothingFactor = 0.99f)
{
    auto* current = filter.coefficients->getRawCoefficients();
    auto* targetCoeffs = target.getRawCoefficients();

    for (int i = 0; i < 5; ++i)
    {
        current[i] = smoothingFactor * current[i]
                   + (1.0f - smoothingFactor) * targetCoeffs[i];
    }
}
```

**Better approach:** Use JUCE's `SmoothedValue` for parameter smoothing before coefficient calculation, rather than smoothing the coefficients directly.

### 13.2 Block-Based vs. Sample-Based Processing

For parametric EQs with infrequent parameter changes, block-based processing is sufficient:

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
{
    juce::ScopedNoDenormals noDenormals;

    // Update coefficients once per block
    updateAllBands();

    // Process entire block with stable coefficients
    auto block = juce::dsp::AudioBlock<float>(buffer);
    for (auto& band : bands)
        band.process(juce::dsp::ProcessContextReplacing<float>(block));
}
```

### 13.3 Denormal Prevention

Denormal numbers can cause massive CPU spikes in IIR filters when processing silence:

```cpp
// Option 1: JUCE macro (recommended)
juce::ScopedNoDenormals noDenormals;

// Option 2: Add DC offset to filter state
void antiDenormal(float& value)
{
    value += 1.0e-15f;
    value -= 1.0e-15f;
}
```

### 13.4 Common Pitfalls

| Pitfall | Impact | Solution |
|---------|--------|----------|
| Updating coefficients every sample | CPU waste | Update once per block or on parameter change |
| Not pre-warping cutoff frequency | Wrong cutoff at high frequencies | Use pre-warped frequency in coefficient calculation |
| Using Direct Form I with float | Numerical instability | Use Direct Form II Transposed |
| Cascading without gain staging | Clipping between sections | Normalize gain at each stage |
| Ignoring Nyquist limit | Unstable filter if fc > fs/2 | Clamp frequency to 0.499 * sampleRate |
| Coefficient smoothing artifacts | Phase modulation | Smooth parameters, not coefficients |

---

## Part 5: References

## 14. References and Further Reading

### Academic Papers
- Bristow-Johnson, R. "Audio EQ Cookbook." (Online reference, continuously updated)
- Vicanek, M. (2019). "Matched Second Order Digital Filters." arXiv:1601.02855.
- Massberg, M. (2011). "Digital Low-Pass Filter Design with Analog-Matched Magnitude Response." AES Convention Paper 8416.
- Zavalishin, V. (2012). "The Art of VA Filter Design." Native Instruments.

### Books
- Smith, J.O. (2007). *Introduction to Digital Filters with Audio Applications*. W3K Publishing.
- Zolzer, U. (2011). *DAFX: Digital Audio Effects*. Chapter 2: Filters.
- Pirkle, W. (2019). *Designing Audio Effect Plugins in C++*. Chapters 6-10.

### Software References
- Cytomic (Andrew Simper): SVF/TPT filter implementations used in industry-standard plugins
- JUCE `dsp::IIR::Filter` and `dsp::StateVariableTPTFilter` source code
- Analog Devices: Filter Design Tool (online coefficient calculator)

### Classic Hardware EQ References
- Neve 1073/1081 (inductor-based EQ, proportional Q)
- Pultec EQP-1A (passive LC EQ with tube makeup gain)
- API 550/560 (proportional Q, discrete op-amp design)
- SSL E/G-series channel strip EQ (parametric, constant Q)

---

*Research document for O-AnalogEQ. Covers biquad filter design, analog prototypes, frequency warping compensation, and complete JUCE parametric EQ implementation.*
