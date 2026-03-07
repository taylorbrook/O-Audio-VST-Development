---
title: "FFT Audio Artifact Prevention Guide"
created: 2026-02-04
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Comprehensive reference for understanding, diagnosing, and preventing artifacts in FFT-based audio processing, covering spectral leakage, discontinuity artifacts, phase issues, transient smearing, modulation artifacts, and diagnostic techniques."
domain: dsp
type: research
keywords:
  - fft
  - spectral-leakage
  - audio-artifacts
  - phase-coherence
  - windowing
  - overlap-add
  - transient-smearing
stages: [2, 3]
agents: [dsp]
status: stale
stale_reason: "References deprecated API: getLatencySamples() const override (non-virtual in JUCE 8)"
---

# FFT Audio Artifact Prevention Guide

A comprehensive reference for understanding, diagnosing, and preventing artifacts in FFT-based audio processing.

---

## Table of Contents

1. [Introduction](#introduction)
2. [Spectral Leakage](#spectral-leakage)
3. [Discontinuity Artifacts](#discontinuity-artifacts)
4. [Phase Artifacts](#phase-artifacts)
5. [Transient Smearing](#transient-smearing)
6. [Modulation Artifacts](#modulation-artifacts)
7. [Latency Artifacts](#latency-artifacts)
8. [Diagnostic Techniques](#diagnostic-techniques)
9. [Prevention Checklist](#prevention-checklist)
10. [References](#references)

---

## Introduction

FFT-based audio processing is prone to several categories of artifacts. Understanding the root cause of each allows for targeted prevention strategies.

### Artifact Taxonomy

| Artifact Type | Audible Symptom | Root Cause |
|---------------|-----------------|------------|
| Spectral Leakage | Frequency smearing, "dirty" sound | Signal discontinuities at frame boundaries |
| Clicks/Pops | Periodic clicking | Non-COLA overlap-add, boundary discontinuities |
| Phase Artifacts | Phasiness, flanging | Loss of phase coherence between frames |
| Transient Smearing | Soft attacks, "echo" | Windowing blurs temporal precision |
| Musical Noise | Bird chirping, warbling | Noise floor modulation |
| Latency Issues | Timing problems | Unreported or variable latency |

---

## Spectral Leakage

### What Is Spectral Leakage?

When performing an FFT on a finite-length signal, the implicit assumption is that the signal is periodic—the end connects back to the beginning. If these endpoints don't match smoothly, a discontinuity exists, which the FFT interprets as high-frequency content that wasn't in the original signal.

**Mathematical explanation**: The DFT implicitly multiplies the signal by a rectangular window. The frequency response of a rectangular window is a sinc function with large side lobes. When convolved with the signal's spectrum, sharp frequency peaks spread into adjacent bins.

### Visual Representation

```
Without windowing:
Signal: ~~~~|~~~~|~~~~  (discontinuity at frame boundary)
        ↓ FFT
Spectrum: Sharp peak with "skirts" spreading into adjacent bins

With windowing:
Signal: ~~∩~~|~~∩~~  (smooth taper to zero)
        ↓ FFT
Spectrum: Cleaner peak, minimal spreading
```

### Prevention: Window Functions

Apply a window function before the FFT to taper the signal smoothly to zero at the boundaries:

```cpp
// Apply window before FFT
window.multiplyWithWindowingTable(fftData.data(), fftSize);
fft.performRealOnlyForwardTransform(fftData.data());
```

### Window Function Comparison

| Window | Main Lobe Width | Highest Side Lobe | Best For |
|--------|-----------------|-------------------|----------|
| Rectangular | 2 bins | -13 dB | Never (for modification) |
| Hann | 4 bins | -31 dB | General purpose |
| Hamming | 4 bins | -42 dB | Speech, narrow-band |
| Blackman | 6 bins | -58 dB | High dynamic range |
| Blackman-Harris | 8 bins | -92 dB | Maximum dynamic range |
| Kaiser (β=5) | ~4 bins | ~-40 dB | Adjustable trade-off |
| Kaiser (β=10) | ~6 bins | ~-80 dB | High dynamic range |

### Trade-off: Resolution vs. Leakage

- **Narrow main lobe** = better frequency resolution but worse side lobe rejection
- **Wide main lobe** = worse frequency resolution but better side lobe rejection

**Rule of thumb**: Use Hann for most applications. Only use Blackman/Blackman-Harris when detecting quiet signals near loud ones.

### Implementation

```cpp
// Create Hann window (periodic, not symmetric)
juce::dsp::WindowingFunction<float> window(
    fftSize + 1,  // +1 for periodic window
    juce::dsp::WindowingFunction<float>::hann,
    false  // normalized = false
);

// For high dynamic range applications
juce::dsp::WindowingFunction<float> window(
    fftSize + 1,
    juce::dsp::WindowingFunction<float>::blackmanHarris,
    false
);
```

---

## Discontinuity Artifacts

### Symptoms

- Periodic clicking at frame boundaries (every hopSize samples)
- Rhythmic distortion at FFT frame rate
- "Zipper noise" during parameter changes

### Causes

1. **Non-COLA compliance**: Window/overlap combination doesn't sum to constant
2. **Skipping synthesis window**: Not applying window after IFFT
3. **Incorrect overlap-add accumulation**: Replacing instead of adding

### COLA (Constant Overlap-Add) Constraint

For artifact-free reconstruction:

```
∑ w(n - m·H) = C (constant) for all sample indices n
```

**Verified COLA combinations**:

| Window | 50% Overlap | 75% Overlap | Notes |
|--------|-------------|-------------|-------|
| Rectangular | ✓ (C=1) | ✗ | Only for no modification |
| Triangular | ✓ (C=1) | ✗ | Limited applications |
| Hann | ✓ (C=1) | ✓ (C=1.5) | Recommended |
| Hamming | ≈ (C≈1.08) | ≈ | Small ripple |
| Blackman | ✗ | ≈ | Use 66.7% overlap |

### WOLA (Weighted Overlap-Add)

For spectral modification, apply the window twice—before FFT (analysis) and after IFFT (synthesis):

```cpp
void processFrame()
{
    // 1. Analysis window
    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    // 2. Forward FFT
    fft.performRealOnlyForwardTransform(fftData.data());

    // 3. Spectral modification
    processSpectrum(fftData.data());

    // 4. Inverse FFT
    fft.performRealOnlyInverseTransform(fftData.data());

    // 5. Synthesis window - CRITICAL
    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    // 6. Gain correction
    constexpr float correction = 2.0f / 3.0f;  // For Hann @ 75%
    for (int i = 0; i < fftSize; ++i)
        fftData[i] *= correction;

    // 7. Overlap-add (accumulate, don't replace!)
    for (int i = 0; i < fftSize; ++i)
    {
        int idx = (writePos + i) % outputFifoSize;
        outputFifo[idx] += fftData[i];  // += not =
    }
}
```

### Gain Correction Values

| Window | 50% Overlap | 75% Overlap |
|--------|-------------|-------------|
| Hann | 4/3 (1.333) | 2/3 (0.667) |
| Hamming | ≈1.23 | ≈0.62 |
| Blackman | N/A | ≈0.57 |

### Verification: The Null Test

```
1. Create two tracks with identical audio
2. Insert your FFT plugin on track 2
3. Enable plugin (bypass off)
4. Invert polarity of track 2
5. Sum both tracks
6. Result should be SILENCE
```

Any residual signal indicates discontinuity artifacts.

---

## Phase Artifacts

### Symptoms

- Phasiness, hollow sound
- Flanging/phasing effect
- "Underwater" quality
- Loss of stereo image clarity

### Causes

1. **Phase incoherence between frames**: Phases drift independently
2. **Bin-by-bin processing without phase consideration**: Treating magnitude only
3. **Time-stretching without phase vocoder**: Breaks phase relationships

### Understanding Phase in FFT

Each bin contains magnitude AND phase:

```cpp
std::complex<float> bin = fftData[i];
float magnitude = std::abs(bin);    // "How loud"
float phase = std::arg(bin);        // "Where in cycle"
```

For stationary signals, phase relationships between bins are relatively unimportant. For transients and complex signals, phase coherence matters greatly.

### Phase Vocoder Principles

For time-stretching or pitch-shifting, use phase vocoder techniques:

```cpp
// Phase vocoder: track phase evolution between frames
class PhaseVocoder
{
    void processFrame(float* data, float timeStretchRatio)
    {
        auto* cdata = reinterpret_cast<std::complex<float>*>(data);

        for (int bin = 0; bin < numBins; ++bin)
        {
            float magnitude = std::abs(cdata[bin]);
            float phase = std::arg(cdata[bin]);

            // Calculate expected phase advance
            float expectedPhase = lastPhase[bin] + bin * hopSize * twoPi / fftSize;

            // Calculate actual phase deviation
            float phaseDiff = phase - expectedPhase;

            // Wrap to [-π, π]
            phaseDiff = wrapPhase(phaseDiff);

            // Calculate instantaneous frequency
            float instFreq = bin + phaseDiff * fftSize / (twoPi * hopSize);

            // Accumulate phase at new rate
            synthPhase[bin] += instFreq * synthHopSize * twoPi / fftSize;

            // Reconstruct with adjusted phase
            cdata[bin] = std::polar(magnitude, synthPhase[bin]);

            lastPhase[bin] = phase;
        }
    }

    float wrapPhase(float phase)
    {
        while (phase > pi) phase -= twoPi;
        while (phase < -pi) phase += twoPi;
        return phase;
    }

    std::vector<float> lastPhase;
    std::vector<float> synthPhase;
};
```

### Vertical Phase Coherence

For harmonic sounds, phase relationships between harmonics matter:

```cpp
// Phase locking for harmonic signals
void phaseLockToFundamental(std::complex<float>* data, int fundamentalBin)
{
    float refPhase = std::arg(data[fundamentalBin]);

    for (int harmonic = 2; harmonic * fundamentalBin < numBins; ++harmonic)
    {
        int bin = harmonic * fundamentalBin;
        float mag = std::abs(data[bin]);

        // Lock phase to harmonic relationship
        float lockedPhase = refPhase * harmonic;
        data[bin] = std::polar(mag, lockedPhase);
    }
}
```

### Prevention Strategies

1. **Avoid unnecessary phase modification**: Process magnitude only when possible
2. **Use phase vocoder for time/pitch changes**: Don't just resample frames
3. **Consider phase locking for harmonic content**
4. **Increase overlap for smoother phase transitions**: 75%+ overlap

---

## Transient Smearing

### Symptoms

- Soft, "mushy" attacks
- Pre-echo before transients
- Loss of punch and clarity
- "Phasiness" on drums and percussive sounds

### Root Cause

The windowing function that prevents spectral leakage also blurs temporal precision:

```
Original:    ________|███████████|________
             (silence) (transient) (silence)

Windowed:    ____/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\____
             Energy spreads before and after!
```

A single impulse, when windowed and processed through STFT, becomes a "smeared" version spanning the window length.

### The Fundamental Trade-off

| Parameter | High Value | Low Value |
|-----------|------------|-----------|
| FFT Size | Better frequency resolution | Better time resolution |
| Overlap | Smoother output | Less smearing |
| Window Width | Less leakage | Less smearing |

**For transients**: Smaller FFT size, higher overlap.

### Transient Preservation Techniques

#### 1. Transient Detection

```cpp
class TransientDetector
{
public:
    bool isTransient(float* frame, int size)
    {
        // Calculate spectral flux (change in magnitude)
        float flux = 0.0f;

        for (int bin = 0; bin < size / 2; ++bin)
        {
            float mag = std::abs(frame[bin]);
            float diff = mag - lastMagnitudes[bin];

            // Only count increases (onsets)
            if (diff > 0)
                flux += diff;

            lastMagnitudes[bin] = mag;
        }

        // Compare to threshold
        float threshold = medianFlux * transientSensitivity;
        updateMedian(flux);

        return flux > threshold;
    }

private:
    std::vector<float> lastMagnitudes;
    float medianFlux = 0.0f;
    float transientSensitivity = 2.0f;
};
```

#### 2. Phase Reset at Transients (Röbel Method)

```cpp
void processFrameWithTransientHandling(float* data)
{
    if (transientDetector.isTransient(data, fftSize))
    {
        // Reset synthesis phase to analysis phase
        for (int bin = 0; bin < numBins; ++bin)
        {
            synthPhase[bin] = std::arg(
                reinterpret_cast<std::complex<float>*>(data)[bin]
            );
        }
    }

    // Continue normal processing...
}
```

#### 3. Adaptive Window Sizing

```cpp
void processWithAdaptiveWindow(const float* input)
{
    if (detectTransient(input))
    {
        // Use smaller window for transients
        processWithWindow(input, smallFFT, smallWindow, smallHop);
    }
    else
    {
        // Use larger window for tonal content
        processWithWindow(input, largeFFT, largeWindow, largeHop);
    }
}
```

#### 4. Transient/Steady-State Separation

```cpp
void separateTransientsSteadyState(float* data)
{
    auto* cdata = reinterpret_cast<std::complex<float>*>(data);

    for (int bin = 0; bin < numBins; ++bin)
    {
        float mag = std::abs(cdata[bin]);

        // Smooth magnitude across time (median filter)
        float smoothedMag = medianFilter.process(mag, bin);

        // Transient = original - smoothed
        float transientMag = std::max(0.0f, mag - smoothedMag);

        // Store for separate processing
        transientBins[bin] = std::polar(transientMag, std::arg(cdata[bin]));
        steadyStateBins[bin] = std::polar(smoothedMag, std::arg(cdata[bin]));
    }
}
```

### Recommended Settings for Transient-Heavy Material

- FFT Size: 512-1024 (smaller than default)
- Overlap: 75-87.5%
- Enable transient detection and phase reset
- Consider multi-resolution approach

---

## Modulation Artifacts

### Symptoms

- "Musical noise" or "bird chirping"
- Warbling, gurgling sounds
- Noise floor that rises and falls rhythmically
- Artifacts on quiet passages after loud sections

### Root Cause

When processing reduces signal content (noise reduction, spectral subtraction), the noise floor becomes modulated:

```
Original:  Signal + Noise (steady noise floor)
After:     Reduced Signal + Modulated Noise Residual
           ↓
           Noise rises and falls with signal = "musical noise"
```

### Prevention Strategies

#### 1. Spectral Floor / Noise Floor

```cpp
void applySpectralFloor(std::complex<float>* data, float floorDb)
{
    float floorLinear = juce::Decibels::decibelsToGain(floorDb);

    for (int bin = 0; bin < numBins; ++bin)
    {
        float mag = std::abs(data[bin]);
        float phase = std::arg(data[bin]);

        // Don't let magnitude go below floor
        mag = std::max(mag, floorLinear);

        data[bin] = std::polar(mag, phase);
    }
}
```

#### 2. Soft Masking (vs. Hard Masking)

```cpp
// WRONG: Hard masking creates musical noise
if (magnitude < threshold)
    magnitude = 0.0f;

// BETTER: Soft masking with Wiener filter
float mask = magnitude / (magnitude + noiseEstimate);
magnitude *= mask;

// BEST: Soft masking with floor
float mask = magnitude / (magnitude + noiseEstimate);
mask = std::max(mask, minMask);  // e.g., minMask = 0.1
magnitude *= mask;
```

#### 3. Temporal Smoothing

```cpp
class TemporalSmoother
{
public:
    void smooth(float* magnitudes, int numBins)
    {
        for (int bin = 0; bin < numBins; ++bin)
        {
            // Exponential smoothing
            smoothedMags[bin] = attack * magnitudes[bin]
                              + (1.0f - attack) * smoothedMags[bin];

            // Use smoothed value
            magnitudes[bin] = smoothedMags[bin];
        }
    }

private:
    std::vector<float> smoothedMags;
    float attack = 0.3f;  // Adjust for more/less smoothing
};
```

#### 4. Spectral Smoothing

```cpp
void smoothSpectrally(float* magnitudes, int numBins, int kernelSize)
{
    std::vector<float> temp(magnitudes, magnitudes + numBins);

    for (int bin = kernelSize / 2; bin < numBins - kernelSize / 2; ++bin)
    {
        float sum = 0.0f;
        for (int k = -kernelSize / 2; k <= kernelSize / 2; ++k)
        {
            sum += temp[bin + k];
        }
        magnitudes[bin] = sum / kernelSize;
    }
}
```

---

## Latency Artifacts

### Symptoms

- Plugin output is delayed relative to dry signal
- Timing issues in mix
- Comb filtering when mixed with dry signal
- PDC (Plugin Delay Compensation) not working

### Causes

1. **Unreported latency**: `getLatencySamples()` returns wrong value
2. **Variable latency**: Latency changes during playback
3. **Fractional sample latency**: Sub-sample timing issues

### Minimum Latency Calculation

```cpp
// Latency = FFT size (for look-ahead buffering)
int getLatencySamples() const override
{
    return fftSize;
}

// For some architectures, latency = FFT size + hop size
int getLatencySamples() const override
{
    return fftSize + hopSize;
}
```

### Zero-Latency Approximation

True zero-latency FFT processing is impossible (requires future samples), but you can minimize perceived latency:

```cpp
class LowLatencyFFT
{
    // Use smallest FFT size that meets requirements
    static constexpr int fftSize = 256;  // ~5.8ms at 44.1kHz

    // Maximum overlap
    static constexpr int hopSize = 32;   // 87.5% overlap

    // Compensate in mix
    void processBlock(AudioBuffer<float>& buffer)
    {
        // Process wet signal through FFT
        processFFT(buffer);

        // Delay dry signal to match
        delayLine.process(dryBuffer, fftSize);

        // Mix
        mixWetDry(buffer, dryBuffer);
    }
};
```

### Reporting Latency Correctly

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    // Calculate latency based on FFT configuration
    currentLatency = fftSize;

    // Report to host
    setLatencySamples(currentLatency);
}

// If latency can change (e.g., different FFT size based on sample rate)
void updateLatency(int newLatency)
{
    currentLatency = newLatency;
    setLatencySamples(currentLatency);

    // Notify host of change
    updateHostDisplay();
}
```

---

## Diagnostic Techniques

### 1. Visual Inspection

Use a spectrogram to visualize artifacts:

```cpp
// Generate test signals for diagnosis:

// Pure sine (reveals leakage)
float sine = std::sin(2.0f * pi * 1000.0f * t);

// Impulse train (reveals smearing)
float impulse = (frameCount % 4410 == 0) ? 1.0f : 0.0f;

// Noise (reveals modulation artifacts)
float noise = random.nextFloat() * 2.0f - 1.0f;
```

### 2. Null Test

```cpp
// In plugin, add bypass comparison mode:
void processBlock(AudioBuffer<float>& buffer)
{
    if (nullTestMode)
    {
        // Store original
        AudioBuffer<float> original;
        original.makeCopyOf(buffer);

        // Process
        processFFT(buffer);

        // Subtract original
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            FloatVectorOperations::subtract(
                buffer.getWritePointer(ch),
                original.getReadPointer(ch),
                buffer.getNumSamples()
            );
        }
        // Result should be silence for perfect reconstruction
    }
}
```

### 3. Artifact Measurement

```cpp
class ArtifactMeter
{
public:
    float measureArtifacts(const float* processed, const float* original, int numSamples)
    {
        // Compute difference
        float errorSum = 0.0f;
        float signalSum = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float error = processed[i] - original[i];
            errorSum += error * error;
            signalSum += original[i] * original[i];
        }

        // Signal-to-artifact ratio in dB
        float sar = 10.0f * std::log10(signalSum / (errorSum + 1e-10f));
        return sar;
    }
};
```

### 4. Frequency-Specific Testing

```cpp
void testSpectralLeakage()
{
    // Generate sine at exact bin frequency (should have no leakage)
    float binFreq = sampleRate / fftSize;  // Frequency of bin 1
    float testFreq = binFreq * 10;  // Bin 10

    // Process and check adjacent bins
    // Bins 9 and 11 should be significantly lower than bin 10
}
```

---

## Prevention Checklist

### Before FFT
- [ ] Apply analysis window (Hann recommended)
- [ ] Use periodic window (fftSize + 1 length)
- [ ] Verify buffer is filled correctly

### Spectral Processing
- [ ] Handle DC bin (index 0) appropriately
- [ ] Handle Nyquist bin appropriately
- [ ] Preserve phase when only modifying magnitude
- [ ] Use soft transitions, not hard thresholds

### After IFFT
- [ ] Apply synthesis window (same as analysis for WOLA)
- [ ] Apply correct gain compensation
- [ ] Overlap-add (accumulate, don't replace)

### System Level
- [ ] Report correct latency to host
- [ ] Use COLA-compliant window/overlap combination
- [ ] Verify with null test
- [ ] Test with transient-heavy material
- [ ] Test with quiet passages

### For Time/Pitch Modification
- [ ] Implement phase vocoder (not just frame resampling)
- [ ] Add transient detection
- [ ] Reset phase at transients
- [ ] Consider multi-resolution approach

---

## References

### Academic Papers

- Röbel, A. "[A new approach to transient processing in the phase vocoder](https://hal.sorbonne-universite.fr/hal-01161124v1)" - DAFx-03
- Röbel, A. "[Transient detection and preservation in the phase vocoder](https://hal.science/hal-01161125/document)" - ICMC 2003
- Prüša, Z. & Holighaus, N. "[Phase Vocoder Done Right](https://arxiv.org/pdf/2202.07382)" - arXiv
- Harris, F.J. "On the Use of Windows for Harmonic Analysis with the Discrete Fourier Transform" - IEEE 1978

### Online Resources

- [Understanding Spectral Leakage - Digital Signals Theory](https://brianmcfee.net/dstbook-site/content/ch06-dft-properties/Leakage.html)
- [Spectral Leakage - Wikipedia](https://en.wikipedia.org/wiki/Spectral_leakage)
- [Windowing Functions - Digital Sound & Music](https://digitalsoundandmusic.com/2-3-11-windowing-functions-to-eliminate-spectral-leakage/)
- [WOLA Processing Steps - dsprelated.com](https://www.dsprelated.com/freebooks/sasp/WOLA_Processing_Steps.html)

### JUCE-Specific

- [FFT Processing in JUCE - audiodev.blog](https://audiodev.blog/fft-processing/)
- [JUCE Forum - FFT Spectral Transformation](https://forum.juce.com/t/fft-spectral-transformation-basic-example/16588)

---

*Document version: 1.0 | Last updated: 2026-02-04*
