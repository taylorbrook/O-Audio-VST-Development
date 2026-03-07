---
title: "FFT Processing Best Practices in JUCE"
created: 2026-02-04
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Comprehensive guide to implementing high-quality FFT-based audio processing in JUCE plugins, covering STFT architecture, buffer management, window functions, overlap-add synthesis, COLA compliance, and performance optimization strategies."
domain: dsp
type: guide
keywords:
  - fft
  - stft
  - spectral-processing
  - windowing
  - overlap-add
  - juce-dsp
  - buffer-management
  - artifact-prevention
stages: [1, 2, 3]
agents: [dsp]
status: stale
stale_reason: "References deprecated API: getLatencySamples() override (non-virtual in JUCE 8)"
---

# FFT Processing Best Practices in JUCE

A comprehensive guide to implementing high-quality FFT-based audio processing in JUCE plugins, with focus on artifact prevention and performance optimization.

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [JUCE FFT Fundamentals](#juce-fft-fundamentals)
3. [STFT Architecture](#stft-architecture)
4. [Buffer Management](#buffer-management)
5. [Window Functions](#window-functions)
6. [Overlap-Add Synthesis](#overlap-add-synthesis)
7. [Performance Optimization](#performance-optimization)
8. [Common Pitfalls](#common-pitfalls)
9. [Implementation Patterns](#implementation-patterns)
10. [References](#references)

---

## Executive Summary

FFT (Fast Fourier Transform) processing enables powerful spectral manipulation but introduces unique challenges around audio artifacts. This document covers:

- **JUCE's `dsp::FFT` class** - proper initialization and usage patterns
- **STFT (Short-Time Fourier Transform)** - the standard architecture for real-time spectral processing
- **Artifact prevention** - window selection, overlap ratios, and COLA compliance
- **Performance** - FFT size trade-offs and optimization strategies

**Key Insight**: Most FFT artifacts stem from three sources:
1. Spectral leakage (inadequate windowing)
2. Synthesis discontinuities (non-COLA-compliant overlap-add)
3. Phase incoherence (improper phase handling between frames)

---

## JUCE FFT Fundamentals

### Core Classes

JUCE provides FFT functionality through the `juce_dsp` module:

```cpp
#include <juce_dsp/juce_dsp.h>

// Core FFT class
juce::dsp::FFT fft;

// Windowing support
juce::dsp::WindowingFunction<float> window;
```

### Initialization

```cpp
// FFT size must be power of 2
static constexpr int fftOrder = 10;  // 2^10 = 1024 samples
static constexpr int fftSize = 1 << fftOrder;

// Create FFT processor
juce::dsp::FFT fft(fftOrder);

// Create window function (Hann is most common)
juce::dsp::WindowingFunction<float> window(
    fftSize + 1,  // +1 for periodic (not symmetric) window
    juce::dsp::WindowingFunction<float>::hann,
    false  // normalized = false
);
```

**Critical**: Use `fftSize + 1` for the window length to create a **periodic** window. A symmetric window (length = fftSize) causes subtle amplitude errors at frame boundaries.

### Forward and Inverse Transforms

```cpp
// Data buffer (must be 2x FFT size for complex output)
std::array<float, fftSize * 2> fftData;

// Copy audio into fftData[0..fftSize-1]
// ...

// Forward transform (real input → complex output)
fft.performRealOnlyForwardTransform(fftData.data());

// Process spectrum here...
// fftData now contains: [real0, imag0, real1, imag1, ... realN/2, imagN/2]

// Inverse transform (complex input → real output)
fft.performRealOnlyInverseTransform(fftData.data());

// Result in fftData[0..fftSize-1]
```

### Accessing Frequency Bins

```cpp
// Cast to complex for easier manipulation
auto* cdata = reinterpret_cast<std::complex<float>*>(fftData.data());

int numBins = fftSize / 2 + 1;  // DC to Nyquist

for (int bin = 0; bin < numBins; ++bin)
{
    float magnitude = std::abs(cdata[bin]);
    float phase = std::arg(cdata[bin]);

    // Frequency of this bin
    float frequency = bin * sampleRate / fftSize;

    // Modify magnitude/phase as needed
    magnitude *= someGain;

    // Reconstruct complex value
    cdata[bin] = std::polar(magnitude, phase);
}
```

### Frequency Resolution

| Sample Rate | FFT Size | Frequency Resolution | Latency |
|-------------|----------|---------------------|---------|
| 44.1 kHz    | 512      | 86 Hz               | 11.6 ms |
| 44.1 kHz    | 1024     | 43 Hz               | 23.2 ms |
| 44.1 kHz    | 2048     | 21.5 Hz             | 46.4 ms |
| 44.1 kHz    | 4096     | 10.8 Hz             | 92.9 ms |
| 96 kHz      | 2048     | 46.9 Hz             | 21.3 ms |

**Trade-off**: Larger FFT = better frequency resolution but higher latency.

**Tip**: Scale FFT size with sample rate to maintain consistent frequency resolution:
```cpp
int getFFTSize(double sampleRate)
{
    if (sampleRate >= 88200.0)
        return 2048;  // High sample rates
    return 1024;      // Standard rates
}
```

---

## STFT Architecture

The Short-Time Fourier Transform is the standard architecture for real-time spectral processing:

```
Input Audio → [Windowing] → [FFT] → [Spectral Processing] → [IFFT] → [Windowing] → [Overlap-Add] → Output
```

### Key Parameters

1. **FFT Size (N)**: Frame length, determines frequency resolution
2. **Hop Size (H)**: Distance between consecutive frames
3. **Overlap Factor**: `N / H` (typically 4 for 75% overlap)
4. **Analysis Window**: Applied before FFT
5. **Synthesis Window**: Applied after IFFT (optional but recommended)

### Overlap Factor Guidelines

| Overlap | Hop Size (N=1024) | Quality | CPU Load | Use Case |
|---------|-------------------|---------|----------|----------|
| 50%     | 512               | Good    | Lower    | Analysis only, simple filtering |
| 75%     | 256               | Better  | Higher   | Most spectral processing |
| 87.5%   | 128               | Best    | Highest  | Time-stretching, pitch-shifting |

**Recommendation**: Use 75% overlap (overlap factor = 4) as the default for spectral modification.

---

## Buffer Management

### FIFO-Based Architecture

The standard approach uses two circular buffers (FIFOs):

```cpp
class STFTProcessor
{
public:
    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int overlap = 4;
    static constexpr int hopSize = fftSize / overlap;

    STFTProcessor()
        : fft(fftOrder),
          window(fftSize + 1,
                 juce::dsp::WindowingFunction<float>::hann,
                 false)
    {
        inputFifo.fill(0.0f);
        outputFifo.fill(0.0f);
    }

    float processSample(float input)
    {
        // Write input to FIFO
        inputFifo[pos] = input;

        // Read output (and clear for next accumulation)
        float output = outputFifo[pos];
        outputFifo[pos] = 0.0f;

        // Advance position
        if (++pos == fftSize)
            pos = 0;

        // Process frame every hopSize samples
        if (++count == hopSize)
        {
            count = 0;
            processFrame();
        }

        return output;
    }

private:
    void processFrame()
    {
        // Copy input FIFO to FFT buffer (handle wraparound)
        auto* fftPtr = fftData.data();
        auto* inputPtr = inputFifo.data();

        std::memcpy(fftPtr, inputPtr + pos,
                    (fftSize - pos) * sizeof(float));
        std::memcpy(fftPtr + fftSize - pos, inputPtr,
                    pos * sizeof(float));

        // Apply analysis window
        window.multiplyWithWindowingTable(fftPtr, fftSize);

        // Forward FFT
        fft.performRealOnlyForwardTransform(fftPtr);

        // === Spectral processing goes here ===
        processSpectrum(fftPtr);

        // Inverse FFT
        fft.performRealOnlyInverseTransform(fftPtr);

        // Apply synthesis window
        window.multiplyWithWindowingTable(fftPtr, fftSize);

        // Gain correction for overlapped windows
        constexpr float windowCorrection = 2.0f / 3.0f;
        for (int i = 0; i < fftSize; ++i)
            fftPtr[i] *= windowCorrection;

        // Overlap-add into output FIFO
        for (int i = 0; i < fftSize; ++i)
        {
            int idx = (pos + i) % fftSize;
            outputFifo[idx] += fftPtr[i];
        }
    }

    virtual void processSpectrum(float* fftData) = 0;

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, fftSize> inputFifo;
    std::array<float, fftSize> outputFifo;
    std::array<float, fftSize * 2> fftData;

    int pos = 0;
    int count = 0;
};
```

### Latency Considerations

- **Minimum latency**: `fftSize` samples (due to look-ahead for windowing)
- **Report to host**: Override `getLatencySamples()` to return `fftSize`
- **PDC (Plugin Delay Compensation)**: DAWs will compensate automatically

```cpp
int getLatencySamples() override
{
    return fftSize;
}
```

---

## Window Functions

### Available Windows in JUCE

```cpp
enum WindowingMethod
{
    rectangular,
    triangular,
    hann,
    hamming,
    blackman,
    blackmanHarris,
    flatTop,
    kaiser
};
```

### Window Comparison

| Window | Main Lobe Width | Side Lobe Level | Best For |
|--------|-----------------|-----------------|----------|
| **Rectangular** | Narrowest | -13 dB (worst) | Analysis only, known frequencies |
| **Hann** | Moderate | -31 dB | General spectral processing |
| **Hamming** | Moderate | -42 dB | Speech processing |
| **Blackman** | Wide | -58 dB | High dynamic range analysis |
| **Blackman-Harris** | Wider | -92 dB | Highest dynamic range |
| **Flat-Top** | Widest | -44 dB | Amplitude-accurate measurement |
| **Kaiser** | Adjustable | Adjustable | Flexible trade-off |

### Recommendations

1. **Default choice**: Hann window with 75% overlap
2. **High dynamic range**: Blackman-Harris (for detecting quiet signals near loud ones)
3. **Frequency accuracy**: Rectangular (only when frequencies align with bins)
4. **Amplitude measurement**: Flat-top

### Window Gain Correction

When using Hann window with 75% overlap (WOLA):

```cpp
// Hann window squared has average value of 0.375
// With 4x overlap: total gain = 4 × 0.375 = 1.5
constexpr float windowCorrection = 2.0f / 3.0f;  // 1/1.5
```

For 50% overlap:
```cpp
// With 2x overlap: total gain = 2 × 0.375 = 0.75
constexpr float windowCorrection = 4.0f / 3.0f;  // 1/0.75
```

---

## Overlap-Add Synthesis

### COLA (Constant Overlap-Add) Compliance

For perfect reconstruction, windows must satisfy the COLA constraint:

```
∑ w(n - m·H) = constant for all n
```

Where:
- `w(n)` = window function
- `H` = hop size
- `m` = frame index

**COLA-compliant combinations**:

| Window | Valid Overlaps |
|--------|---------------|
| Rectangular | 0% |
| Triangular (Bartlett) | 50% |
| Hann | 50%, 75% |
| Hamming | 50%, 75% |
| Blackman | 66.7% |

### WOLA (Weighted Overlap-Add)

When applying spectral modifications, use WOLA (window both before and after FFT):

```cpp
void processFrame()
{
    // Analysis window (before FFT)
    window.multiplyWithWindowingTable(fftPtr, fftSize);

    fft.performRealOnlyForwardTransform(fftPtr);

    // Spectral modification
    modifySpectrum(fftPtr);

    fft.performRealOnlyInverseTransform(fftPtr);

    // Synthesis window (after IFFT) - CRITICAL for WOLA
    window.multiplyWithWindowingTable(fftPtr, fftSize);

    // Apply gain correction
    applyGainCorrection(fftPtr);

    // Overlap-add
    overlapAdd(fftPtr);
}
```

**Why WOLA matters**: The synthesis window "fades out" any errors introduced by spectral modification at frame boundaries, suppressing audible discontinuities.

### Null Test Verification

To verify your STFT implementation:

1. Create two identical audio tracks
2. Insert your plugin on one track
3. Invert polarity of the plugin track
4. Sum both tracks

**Expected result**: Complete silence (null). Any residual signal indicates implementation errors.

---

## Performance Optimization

### FFT Size Selection Strategy

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Scale FFT size with sample rate
    int targetFFTSize;

    if (sampleRate >= 176400.0)
        targetFFTSize = 4096;
    else if (sampleRate >= 88200.0)
        targetFFTSize = 2048;
    else
        targetFFTSize = 1024;

    // Calculate FFT order
    int order = static_cast<int>(std::log2(targetFFTSize));

    // Reinitialize if changed
    if (order != currentOrder)
    {
        currentOrder = order;
        fft = std::make_unique<juce::dsp::FFT>(order);
        // Rebuild FIFOs, windows, etc.
    }
}
```

### SIMD Optimization

JUCE's windowing function uses SIMD internally. For manual operations:

```cpp
// Use FloatVectorOperations for SIMD-accelerated processing
juce::FloatVectorOperations::multiply(
    fftData.data(),
    windowData.data(),
    fftSize
);

// Multiply by scalar
juce::FloatVectorOperations::multiply(
    fftData.data(),
    gainCorrection,
    fftSize
);

// Add (for overlap-add)
juce::FloatVectorOperations::add(
    outputFifo.data() + offset,
    fftData.data(),
    numSamples
);
```

### Memory Allocation

**Rule**: Never allocate in `processBlock()`.

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    // All allocations happen here
    inputFifo.resize(fftSize);
    outputFifo.resize(fftSize);
    fftData.resize(fftSize * 2);
    windowData.resize(fftSize);

    // Pre-calculate window
    window.fillWindowingTables(windowData.data(), fftSize);
}
```

### Processing Efficiency

```cpp
float processSample(float input)
{
    // This runs for EVERY sample - keep it minimal
    inputFifo[pos] = input;
    float output = outputFifo[pos];
    outputFifo[pos] = 0.0f;

    if (++pos == fftSize) pos = 0;

    // Heavy processing only happens every hopSize samples
    if (++count == hopSize)
    {
        count = 0;
        processFrame();  // FFT + processing + IFFT
    }

    return output;
}
```

---

## Common Pitfalls

### 1. Spectral Leakage

**Symptom**: Frequency smearing, artifacts on pure tones

**Cause**: Not applying window before FFT, or signal discontinuities

**Fix**: Always apply window before FFT

```cpp
// WRONG - no windowing
fft.performRealOnlyForwardTransform(data);

// CORRECT - apply window first
window.multiplyWithWindowingTable(data, fftSize);
fft.performRealOnlyForwardTransform(data);
```

### 2. Synthesis Clicks

**Symptom**: Periodic clicks at frame boundaries

**Cause**: Non-COLA-compliant window/overlap combination

**Fix**: Use WOLA with proper gain correction

```cpp
// Apply synthesis window (same as analysis)
window.multiplyWithWindowingTable(data, fftSize);

// Apply gain correction for chosen overlap
data *= windowCorrection;
```

### 3. Phase Discontinuities

**Symptom**: Phasy, flanging-like artifacts

**Cause**: Phase not handled consistently between frames

**Fix**: For heavy modifications, consider phase vocoder techniques (see artifact prevention document)

### 4. DC Offset

**Symptom**: Gradual drift or sub-bass rumble

**Cause**: Processing affects DC bin incorrectly

**Fix**: Explicitly handle DC bin (index 0)

```cpp
// Preserve or zero DC as appropriate
cdata[0] = 0.0f;  // Zero DC
// OR
cdata[0] = cdata[0];  // Preserve DC unchanged
```

### 5. Symmetric vs Periodic Window

**Symptom**: Subtle amplitude modulation

**Cause**: Using symmetric window (N points) instead of periodic (N+1 points)

**Fix**:
```cpp
// WRONG
juce::dsp::WindowingFunction<float> window(fftSize, ...);

// CORRECT
juce::dsp::WindowingFunction<float> window(fftSize + 1, ...);
```

### 6. Forgetting Latency Compensation

**Symptom**: Plugin output is delayed relative to bypass

**Cause**: Not reporting latency to host

**Fix**:
```cpp
int getLatencySamples() override { return fftSize; }
```

---

## Implementation Patterns

### Pattern 1: Spectrum Analyzer (Analysis Only)

```cpp
class SpectrumAnalyzer
{
public:
    void pushSamples(const float* data, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            fifo[fifoIndex++] = data[i];

            if (fifoIndex == fftSize)
            {
                // Copy to FFT buffer
                std::copy(fifo.begin(), fifo.end(), fftData.begin());

                // Window and transform
                window.multiplyWithWindowingTable(fftData.data(), fftSize);
                fft.performRealOnlyForwardTransform(fftData.data());

                // Convert to magnitude (dB)
                calculateMagnitudes();

                fifoIndex = 0;
                newDataAvailable = true;
            }
        }
    }

private:
    void calculateMagnitudes()
    {
        auto* cdata = reinterpret_cast<std::complex<float>*>(fftData.data());

        for (int i = 0; i < numBins; ++i)
        {
            float mag = std::abs(cdata[i]);
            magnitudes[i] = juce::Decibels::gainToDecibels(mag / fftSize);
        }
    }
};
```

### Pattern 2: Simple Spectral Filter

```cpp
class SpectralFilter : public STFTProcessor
{
protected:
    void processSpectrum(float* data) override
    {
        auto* cdata = reinterpret_cast<std::complex<float>*>(data);

        for (int bin = 0; bin < numBins; ++bin)
        {
            float freq = bin * sampleRate / fftSize;

            // Example: Low-pass at 2kHz
            if (freq > 2000.0f)
            {
                cdata[bin] = 0.0f;
            }
        }
    }
};
```

### Pattern 3: Spectral Gate

```cpp
class SpectralGate : public STFTProcessor
{
protected:
    void processSpectrum(float* data) override
    {
        auto* cdata = reinterpret_cast<std::complex<float>*>(data);

        float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDb);

        for (int bin = 0; bin < numBins; ++bin)
        {
            float magnitude = std::abs(cdata[bin]);

            if (magnitude < thresholdLinear)
            {
                // Soft gate with ratio
                float reduction = magnitude / thresholdLinear;
                cdata[bin] *= reduction;
            }
        }
    }

    float thresholdDb = -40.0f;
};
```

### Pattern 4: Convolution via FFT

```cpp
class FFTConvolver
{
public:
    void setImpulseResponse(const float* ir, int irLength)
    {
        // Pad IR to FFT size
        std::fill(irFFT.begin(), irFFT.end(), 0.0f);
        std::copy(ir, ir + std::min(irLength, fftSize), irFFT.begin());

        // Transform IR
        fft.performRealOnlyForwardTransform(irFFT.data());
    }

protected:
    void processSpectrum(float* data) override
    {
        auto* sig = reinterpret_cast<std::complex<float>*>(data);
        auto* ir = reinterpret_cast<std::complex<float>*>(irFFT.data());

        // Complex multiplication in frequency domain = convolution in time
        for (int bin = 0; bin < numBins; ++bin)
        {
            sig[bin] *= ir[bin];
        }
    }

    std::array<float, fftSize * 2> irFFT;
};
```

---

## References

### Official Documentation
- [JUCE FFT Tutorial](https://docs.juce.com/master/tutorial_simple_fft.html)
- [JUCE Spectrum Analyser Tutorial](https://docs.juce.com/master/tutorial_spectrum_analyser.html)

### Implementation Guides
- [FFT Processing in JUCE - audiodev.blog](https://audiodev.blog/fft-processing/)
- [Making Spectrograms in JUCE - Art+Logic](https://artandlogic.com/making-spectrograms-in-juce/)
- [FftBuffer - GitHub (maxsolomonhenry)](https://github.com/maxsolomonhenry/FftBuffer)
- [fft-juce - GitHub (hollance)](https://github.com/hollance/fft-juce)

### DSP Theory
- [Overlap-Add STFT Processing - CCRMA Stanford](https://ccrma.stanford.edu/~jos/sasp/Overlap_Add_OLA_STFT_Processing.html)
- [Spectral Audio Signal Processing - Julius O. Smith](https://ccrma.stanford.edu/~jos/sasp/)
- [WOLA Processing Steps - dsprelated.com](https://www.dsprelated.com/freebooks/sasp/WOLA_Processing_Steps.html)

### Window Functions
- Harris, F.J. "On the Use of Windows for Harmonic Analysis with the Discrete Fourier Transform" (IEEE, 1978)

---

*Document version: 1.0 | Last updated: 2026-02-04*
