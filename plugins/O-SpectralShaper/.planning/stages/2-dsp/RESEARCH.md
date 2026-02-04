# Stage 2: DSP Implementation - Research

**Date:** 2026-02-03
**Plugin:** O-SpectralShaper
**Stage:** 2-dsp (Spectral Transient Shaping)
**Research Focus:** JUCE FFT APIs, overlap-add STFT, spectral flux detection, per-band processing

---

## 1. JUCE FFT API Analysis

### 1.1 Core Class: `juce::dsp::FFT`

**Header:** `juce_dsp/frequency/juce_FFT.h`
**Module:** `juce_dsp`

```cpp
// Construction
juce::dsp::FFT fft { order };  // FFT size = 2^order
// For 512-point FFT: order = 9 (2^9 = 512)

static constexpr int FFT_ORDER = 9;
static constexpr int FFT_SIZE = 1 << FFT_ORDER;  // 512
juce::dsp::FFT forwardFFT { FFT_ORDER };
juce::dsp::FFT inverseFFT { FFT_ORDER };
```

**Key Methods:**

| Method | Description | Array Size Required |
|--------|-------------|---------------------|
| `performRealOnlyForwardTransform(float* data, bool onlyNonNegative = false)` | Time → Frequency | 2 * FFT_SIZE (1024) |
| `performRealOnlyInverseTransform(float* data)` | Frequency → Time | 2 * FFT_SIZE (1024) |
| `performFrequencyOnlyForwardTransform(float* data, bool onlyNonNegative = false)` | Time → Magnitude only | 2 * FFT_SIZE (1024) |
| `getSize()` | Returns FFT_SIZE | N/A |

**Data Layout After Forward Transform:**
```
fftData[0]      = DC component (real, bin 0)
fftData[1]      = Nyquist component (real, bin N/2)
fftData[2*n]    = Real part of bin n (for n = 1 to N/2-1)
fftData[2*n+1]  = Imaginary part of bin n
```

**Important Notes:**
- `performRealOnlyInverseTransform` automatically divides by FFT_SIZE
- Array must be allocated as `2 * FFT_SIZE` floats even if only using non-negative frequencies
- FFT object caches lookup tables - create once and reuse

### 1.2 WindowingFunction Class

**Header:** `juce_dsp/frequency/juce_Windowing.h`
**Module:** `juce_dsp`

```cpp
// Construction
juce::dsp::WindowingFunction<float> window {
    FFT_SIZE,
    juce::dsp::WindowingFunction<float>::hann,
    false  // normalise (false for STFT synthesis)
};

// Apply window
window.multiplyWithWindowingTable(samples, FFT_SIZE);
```

**Available Window Types:**
- `rectangular` - No windowing (unity gain)
- `triangular` - Linear ramp up/down
- `hann` - **Recommended for STFT** (good COLA properties)
- `hamming` - Similar to Hann, slightly different sidelobes
- `blackman` - Lower sidelobes, wider main lobe
- `blackmanHarris` - Even lower sidelobes
- `flatTop` - For accurate magnitude measurement
- `kaiser` - Adjustable via beta parameter

**Why Hann for O-SpectralShaper:**
- Smooth transitions at window edges
- COLA (Constant Overlap-Add) property with 50% overlap
- Sum of overlapping Hann windows = constant gain (2.0)
- Good balance of time/frequency resolution

### 1.3 DryWetMixer Class

**Header:** `juce_dsp/processors/juce_DryWetMixer.h`
**Module:** `juce_dsp`

```cpp
juce::dsp::DryWetMixer<float> dryWetMixer;

// In prepareToPlay:
juce::dsp::ProcessSpec spec;
spec.sampleRate = sampleRate;
spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
dryWetMixer.prepare(spec);
dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::linear);

// In processBlock:
dryWetMixer.pushDrySamples(juce::dsp::AudioBlock<float>(buffer));  // BEFORE processing
// ... STFT processing ...
dryWetMixer.setWetMixProportion(mixValue);  // 0.0-1.0
dryWetMixer.mixWetSamples(juce::dsp::AudioBlock<float>(buffer));  // AFTER processing
```

**Latency Consideration:**
- DryWetMixer has internal latency compensation
- Our 512-sample STFT latency must be accounted for
- Dry signal must be delayed 512 samples to match wet path

---

## 2. Overlap-Add STFT Implementation Pattern

### 2.1 Core Algorithm

**Configuration for O-SpectralShaper:**
```cpp
static constexpr int FFT_ORDER = 9;
static constexpr int FFT_SIZE = 512;
static constexpr int HOP_SIZE = 256;  // 50% overlap
static constexpr int NUM_OVERLAPS = FFT_SIZE / HOP_SIZE;  // 2
static constexpr int NUM_BINS = FFT_SIZE / 2 + 1;  // 257 usable bins
```

**Latency:** 512 samples = 11.6ms @ 44.1kHz, 10.7ms @ 48kHz, 5.3ms @ 96kHz

### 2.2 Sample-by-Sample STFT Processor

From `research/spectral-transient-shaper-research.md` and O-Freeze reference:

```cpp
class STFTProcessor {
public:
    static constexpr int FFT_ORDER = 9;
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;  // 512
    static constexpr int HOP_SIZE = FFT_SIZE / 2;    // 256

private:
    juce::dsp::FFT forwardFFT { FFT_ORDER };
    juce::dsp::FFT inverseFFT { FFT_ORDER };
    juce::dsp::WindowingFunction<float> window {
        FFT_SIZE,
        juce::dsp::WindowingFunction<float>::hann,
        false  // Don't normalize - we handle COLA scaling manually
    };

    std::array<float, FFT_SIZE> inputFIFO {};
    std::array<float, FFT_SIZE> outputFIFO {};
    std::array<float, FFT_SIZE * 2> fftData {};  // Real + imaginary interleaved
    int fifoIndex = 0;

public:
    void reset() {
        inputFIFO.fill(0.0f);
        outputFIFO.fill(0.0f);
        fftData.fill(0.0f);
        fifoIndex = 0;
    }

    float processSample(float input) {
        // Write input to FIFO
        inputFIFO[fifoIndex] = input;

        // Read delayed output (latency = FFT_SIZE samples)
        float output = outputFIFO[fifoIndex];
        outputFIFO[fifoIndex] = 0.0f;  // Clear for next overlap

        // Advance FIFO index
        if (++fifoIndex >= HOP_SIZE) {
            fifoIndex = 0;
            processFrame();
        }

        return output;
    }

private:
    void processFrame() {
        // 1. Copy input FIFO to FFT buffer
        std::copy(inputFIFO.begin(), inputFIFO.end(), fftData.begin());

        // 2. Apply analysis window
        window.multiplyWithWindowingTable(fftData.data(), FFT_SIZE);

        // 3. Forward FFT
        forwardFFT.performRealOnlyForwardTransform(fftData.data());

        // 4. *** SPECTRAL PROCESSING CALLBACK ***
        // processSpectrum(fftData.data(), NUM_BINS);

        // 5. Inverse FFT
        inverseFFT.performRealOnlyInverseTransform(fftData.data());

        // 6. Apply synthesis window
        window.multiplyWithWindowingTable(fftData.data(), FFT_SIZE);

        // 7. COLA scaling (Hann with 50% overlap sums to 2.0, scale by 0.5)
        constexpr float colaScale = 1.0f / float(NUM_OVERLAPS);  // 0.5

        // 8. Overlap-add to output FIFO
        for (int i = 0; i < FFT_SIZE; ++i) {
            int idx = (fifoIndex + i) % FFT_SIZE;
            outputFIFO[idx] += fftData[i] * colaScale;
        }

        // 9. Shift input FIFO for next frame
        std::rotate(inputFIFO.begin(), inputFIFO.begin() + HOP_SIZE, inputFIFO.end());
    }
};
```

### 2.3 COLA Verification

**Hann Window COLA Property:**
- With 50% overlap (N/2 hop), sum of overlapping Hann windows = 2.0
- Must scale by 0.5 to achieve unity gain
- This is why `colaScale = 1.0f / NUM_OVERLAPS`

**Null Test Verification:**
- In bypass mode (no spectral processing), output should match input delayed by FFT_SIZE samples
- Any deviation indicates COLA or windowing error

---

## 3. Per-Band Transient Detection Algorithm

### 3.1 Band Boundary Calculation (Logarithmic)

```cpp
static constexpr int NUM_BANDS = 32;

struct BandBoundary {
    int startBin;  // First FFT bin in band
    int endBin;    // Last FFT bin + 1 (exclusive)
};

std::array<BandBoundary, NUM_BANDS> bandBoundaries;

void setupBandBoundaries(double sampleRate) {
    const float minFreq = 20.0f;
    const float maxFreq = static_cast<float>(sampleRate / 2.0);
    const float logMin = std::log10(minFreq);
    const float logMax = std::log10(maxFreq);
    const int numBins = FFT_SIZE / 2 + 1;  // 257 bins for 512-point FFT

    int prevBin = 0;
    for (int band = 0; band < NUM_BANDS; ++band) {
        // Calculate start bin (logarithmic spacing)
        float logFreqStart = logMin + (logMax - logMin) * band / NUM_BANDS;
        float freqStart = std::pow(10.0f, logFreqStart);
        int startBin = std::max(prevBin, int(freqStart * FFT_SIZE / sampleRate));

        // Calculate end bin
        float logFreqEnd = logMin + (logMax - logMin) * (band + 1) / NUM_BANDS;
        float freqEnd = std::pow(10.0f, logFreqEnd);
        int endBin = std::clamp(int(freqEnd * FFT_SIZE / sampleRate), startBin + 1, numBins);

        bandBoundaries[band] = { startBin, endBin };
        prevBin = endBin;
    }
}
```

**Band Distribution @ 44.1kHz with 512-point FFT:**
| Bands | Frequency Range | Bins per Band | Musical Context |
|-------|-----------------|---------------|-----------------|
| 1-5   | 20-80 Hz        | ~1-2 bins     | Sub-bass, kick fundamentals |
| 6-12  | 80-400 Hz       | ~2-4 bins     | Bass, low-mids |
| 13-20 | 400-2kHz        | ~4-8 bins     | Mids, snare body |
| 21-27 | 2-8 kHz         | ~8-16 bins    | Presence, snare crack |
| 28-32 | 8-22 kHz        | ~16-30 bins   | Air, cymbals |

### 3.2 Spectral Flux Detection Per Band

**Algorithm from ARCHITECTURE.md and research:**

```cpp
struct Band {
    float prevMagnitude = 0.0f;   // Previous frame band magnitude
    float fastEnvelope = 0.0f;    // Fast attack envelope (1ms attack)
    float slowEnvelope = 0.0f;    // Slow attack envelope (15ms attack)
    float transientActivity = 0.0f;  // 0.0 to 1.0
};

std::array<Band, NUM_BANDS> bands;

void detectTransients(float* fftData, float hopTime, float sensitivity) {
    // Envelope time constants
    const float FAST_ATTACK = 0.001f;   // 1ms
    const float SLOW_ATTACK = 0.015f;   // 15ms
    const float RELEASE = 0.020f;       // 20ms

    // Calculate coefficients based on hop time
    float fastCoeff = std::exp(-hopTime / FAST_ATTACK);
    float slowCoeff = std::exp(-hopTime / SLOW_ATTACK);
    float releaseCoeff = std::exp(-hopTime / RELEASE);

    for (int band = 0; band < NUM_BANDS; ++band) {
        // 1. Calculate band magnitude (sum of bin magnitudes)
        float bandMag = 0.0f;
        int startBin = bandBoundaries[band].startBin;
        int endBin = bandBoundaries[band].endBin;

        for (int bin = startBin; bin < endBin; ++bin) {
            float real = fftData[bin * 2];
            float imag = fftData[bin * 2 + 1];
            bandMag += std::sqrt(real * real + imag * imag);
        }
        // Average over band width
        bandMag /= float(endBin - startBin);

        // 2. Spectral flux (positive-only = half-wave rectified)
        float flux = std::max(0.0f, bandMag - bands[band].prevMagnitude);
        bands[band].prevMagnitude = bandMag;

        // 3. Fast envelope (instant attack, exponential release)
        if (flux > bands[band].fastEnvelope)
            bands[band].fastEnvelope = flux;
        else
            bands[band].fastEnvelope *= releaseCoeff;

        // 4. Slow envelope (smoothed attack, exponential release)
        if (flux > bands[band].slowEnvelope)
            bands[band].slowEnvelope += (1.0f - slowCoeff) * (flux - bands[band].slowEnvelope);
        else
            bands[band].slowEnvelope *= releaseCoeff;

        // 5. Transient = fast - slow, scaled by sensitivity
        float transient = std::max(0.0f, bands[band].fastEnvelope - bands[band].slowEnvelope);
        bands[band].transientActivity = transient * sensitivity;
    }
}
```

### 3.3 Why Positive-Only Spectral Flux

- **Energy increases** = transient onsets (attack)
- **Energy decreases** = releases/decays (sustain)
- Half-wave rectification prevents false triggers on release tails
- Matches human perception of "attack" vs "body/tail"

---

## 4. Envelope Shaping with Curves

### 4.1 Curve Data Structure

```cpp
static constexpr int NUM_BANDS = 32;

// Curve arrays: -1.0 (max cut) to +1.0 (max boost), 0.0 = no change
std::array<float, NUM_BANDS> attackCurve;   // Attack boost/cut per band
std::array<float, NUM_BANDS> sustainCurve;  // Sustain boost/cut per band

// Initialize to neutral
attackCurve.fill(0.0f);
sustainCurve.fill(0.0f);
```

### 4.2 Gain Calculation Per Band

```cpp
void applyEnvelopeShaping(float* fftData, float attackTimeMs, float sustainTimeMs) {
    for (int band = 0; band < NUM_BANDS; ++band) {
        float transient = bands[band].transientActivity;  // 0.0-1.0

        // Attack gain: applied proportional to transient activity
        // Curve value * time parameter scales the effect
        float attackDB = attackCurve[band] * attackTimeMs * 0.1f;
        float attackGain = juce::Decibels::decibelsToGain(attackDB * transient);

        // Sustain gain: applied proportional to (1 - transient)
        float sustainDB = sustainCurve[band] * sustainTimeMs * 0.01f;
        float sustainGain = juce::Decibels::decibelsToGain(sustainDB * (1.0f - transient));

        // Combined gain
        float combinedGain = attackGain * sustainGain;

        // Apply to FFT bins in this band (magnitude scaling, preserve phase)
        int startBin = bandBoundaries[band].startBin;
        int endBin = bandBoundaries[band].endBin;

        for (int bin = startBin; bin < endBin; ++bin) {
            fftData[bin * 2] *= combinedGain;      // Real
            fftData[bin * 2 + 1] *= combinedGain;  // Imaginary
        }
    }
}
```

### 4.3 Why Magnitude-Only Processing

- **Phase preservation** is critical for audio quality
- Scaling magnitude (real and imag equally) preserves phase
- Avoids "phasiness" and stereo image issues
- Standard practice in professional spectral processors

---

## 5. Thread-Safe Curve Synchronization

### 5.1 Double-Buffering Pattern

From CONTEXT.md decision: use double-buffering with atomic swap.

```cpp
// In PluginProcessor.h
std::array<float, NUM_BANDS> attackCurveBuffers[2];
std::array<float, NUM_BANDS> sustainCurveBuffers[2];
std::atomic<int> activeCurveBuffer { 0 };

// GUI thread writes to inactive buffer, then swaps
void setAttackCurve(const std::array<float, NUM_BANDS>& newCurve) {
    int writeBuffer = 1 - activeCurveBuffer.load();
    attackCurveBuffers[writeBuffer] = newCurve;
    activeCurveBuffer.store(writeBuffer);  // Atomic swap
}

// Audio thread reads from active buffer
const std::array<float, NUM_BANDS>& getAttackCurve() const {
    return attackCurveBuffers[activeCurveBuffer.load()];
}
```

### 5.2 Alternative: Per-Band Atomics

For simpler implementation (if double-buffering proves complex):

```cpp
std::array<std::atomic<float>, NUM_BANDS> attackCurve;
std::array<std::atomic<float>, NUM_BANDS> sustainCurve;

// Thread-safe read/write per band
float getAttackValue(int band) { return attackCurve[band].load(); }
void setAttackValue(int band, float value) { attackCurve[band].store(value); }
```

**Trade-off:** More atomic operations, but simpler logic.

---

## 6. Lookahead Buffer Implementation

### 6.1 Purpose

- FFT processing has inherent analysis delay
- Lookahead lets detector "see" transient before it reaches output
- Enables cleaner attack shaping without pre-ringing

### 6.2 Implementation

```cpp
// Lookahead buffer (circular)
juce::AudioBuffer<float> lookaheadBuffer;
int lookaheadWritePos = 0;
int lookaheadSamples = 0;  // 0 when disabled, up to 441 @ 44.1kHz (10ms)

void updateLookahead(bool enabled, float lookaheadTimeMs, double sampleRate) {
    if (enabled)
        lookaheadSamples = static_cast<int>(lookaheadTimeMs * 0.001f * sampleRate);
    else
        lookaheadSamples = 0;
}

// In processBlock: read delayed sample for STFT input
float getLookaheadSample(int channel, float currentInput) {
    auto* bufferData = lookaheadBuffer.getWritePointer(channel);

    // Write current input
    bufferData[lookaheadWritePos] = currentInput;

    // Read delayed sample
    int readPos = (lookaheadWritePos - lookaheadSamples + lookaheadBuffer.getNumSamples())
                  % lookaheadBuffer.getNumSamples();
    float delayedSample = bufferData[readPos];

    return delayedSample;
}
```

### 6.3 Latency Reporting

When lookahead is enabled, total latency increases:

```cpp
void updateLatency() {
    int totalLatency = FFT_SIZE + lookaheadSamples;
    setLatencySamples(totalLatency);
}
```

**Decision from CONTEXT.md:** Keep latency fixed at 512 samples for v1.0 simplicity. Lookahead operates within the existing buffer structure.

---

## 7. Dry Path Latency Matching

### 7.1 Problem

- Wet path has 512-sample latency (STFT processing)
- Dry path must be delayed equally to prevent comb filtering at partial mix

### 7.2 Solution: Delay Buffer for Dry Signal

```cpp
// Dry signal delay buffer (matches STFT latency)
juce::AudioBuffer<float> dryDelayBuffer;
int dryDelayWritePos = 0;

void prepareDryDelay(int numChannels, double sampleRate) {
    dryDelayBuffer.setSize(numChannels, FFT_SIZE);
    dryDelayBuffer.clear();
    dryDelayWritePos = 0;
}

float getDryDelayedSample(int channel, float input) {
    auto* bufferData = dryDelayBuffer.getWritePointer(channel);

    // Read delayed sample (512 samples behind)
    float delayed = bufferData[dryDelayWritePos];

    // Write current sample
    bufferData[dryDelayWritePos] = input;

    return delayed;
}

// Advance write position (call once per sample, after all channels)
void advanceDryDelay() {
    dryDelayWritePos = (dryDelayWritePos + 1) % FFT_SIZE;
}
```

### 7.3 Integration with DryWetMixer

Since `juce::dsp::DryWetMixer` pushes dry samples before processing, we need to delay the dry signal manually:

```cpp
// Option A: Use DryWetMixer with pre-delayed dry signal
// Option B: Implement manual dry/wet mixing with delay

// For v1.0: Use manual mixing (simpler, more control)
void processBlock(...) {
    for (int sample = 0; sample < numSamples; ++sample) {
        for (int ch = 0; ch < numChannels; ++ch) {
            float input = buffer.getSample(ch, sample);

            // Dry path (latency-matched)
            float dry = getDryDelayedSample(ch, input);

            // Wet path (STFT processing)
            float wet = stftProcessor[ch].processSample(input);

            // Mix
            float output = dry * (1.0f - mixValue) + wet * mixValue;
            buffer.setSample(ch, sample, output);
        }
        advanceDryDelay();
    }
}
```

---

## 8. Performance Optimization Opportunities

### 8.1 SIMD for Magnitude Calculation

```cpp
#include <juce_dsp/juce_dsp.h>

using SIMDFloat = juce::dsp::SIMDRegister<float>;

float calculateBandMagnitudeSIMD(float* fftData, int startBin, int endBin) {
    SIMDFloat sum = 0.0f;
    constexpr auto simdSize = SIMDFloat::size();  // 4 (SSE) or 8 (AVX)

    int bin = startBin;

    // Process 4 bins at a time
    for (; bin + simdSize <= endBin; bin += simdSize) {
        // Load interleaved real/imag pairs
        // Note: This requires careful handling of interleaved data
        // May need to deinterleave first
    }

    // Handle remainder
    float tailSum = 0.0f;
    for (; bin < endBin; ++bin) {
        float real = fftData[bin * 2];
        float imag = fftData[bin * 2 + 1];
        tailSum += std::sqrt(real * real + imag * imag);
    }

    return sum.sum() + tailSum;
}
```

### 8.2 FloatVectorOperations for Overlap-Add

```cpp
// Optimized overlap-add
juce::FloatVectorOperations::addWithMultiply(
    outputFIFO.data(),  // destination
    fftData.data(),     // source
    colaScale,          // multiplier
    FFT_SIZE            // count
);
```

### 8.3 Pre-computed Values

```cpp
// In prepareToPlay - compute once
float hopTime;  // HOP_SIZE / sampleRate
float fastCoeff, slowCoeff, releaseCoeff;  // Envelope coefficients

void prepareToPlay(double sampleRate, int samplesPerBlock) {
    hopTime = float(HOP_SIZE) / float(sampleRate);

    const float FAST_ATTACK = 0.001f;
    const float SLOW_ATTACK = 0.015f;
    const float RELEASE = 0.020f;

    fastCoeff = std::exp(-hopTime / FAST_ATTACK);
    slowCoeff = std::exp(-hopTime / SLOW_ATTACK);
    releaseCoeff = std::exp(-hopTime / RELEASE);

    setupBandBoundaries(sampleRate);
}
```

---

## 9. Module Reuse Opportunities

### 9.1 Existing Patterns from O-Freeze

O-Freeze implements granular synthesis with:
- Hann window generation (COLA-scaled)
- Circular buffer management
- Sample-by-sample processing
- Grain state tracking

**Reusable Patterns:**
- Window generation code (adapt for 512-point)
- Circular buffer index management
- State machine pattern (for bypass mode)

### 9.2 Potential Shared Modules

| Module Concept | Purpose | Reusability |
|----------------|---------|-------------|
| `ouaricon-stft` | Generic STFT processor | High - all spectral plugins |
| `ouaricon-envelope` | Envelope followers | High - dynamics, compressors |
| `ouaricon-spectral-bands` | Log band splitting | Medium - spectral effects |

**Recommendation:** Implement inline for v1.0, extract to module after validation.

---

## 10. Known Pitfalls and Mitigations

### 10.1 FFT Data Layout Confusion

**Pitfall:** JUCE FFT data layout is interleaved real/imag, not separate arrays.

**Mitigation:**
```cpp
// Correct access pattern
for (int bin = 0; bin < numBins; ++bin) {
    float real = fftData[bin * 2];
    float imag = fftData[bin * 2 + 1];
    // NOT: fftData[bin] and fftData[numBins + bin]
}
```

### 10.2 Windowing Normalization

**Pitfall:** JUCE WindowingFunction has `normalize` parameter that affects magnitude.

**Mitigation:** Use `normalise = false` for STFT synthesis window:
```cpp
juce::dsp::WindowingFunction<float> window {
    FFT_SIZE,
    juce::dsp::WindowingFunction<float>::hann,
    false  // Don't normalize - we handle COLA scaling
};
```

### 10.3 FFT Inverse Scaling

**Pitfall:** `performRealOnlyInverseTransform` already divides by FFT_SIZE.

**Mitigation:** Don't double-scale. Only apply COLA compensation (0.5 for 50% overlap).

### 10.4 Denormal Prevention

**Pitfall:** Very small FFT bin values can cause CPU spikes.

**Mitigation:**
```cpp
void processBlock(...) {
    juce::ScopedNoDenormals noDenormals;
    // ...
}
```

### 10.5 Thread Safety of Curve Updates

**Pitfall:** Torn reads if GUI writes mid-audio-callback.

**Mitigation:** Double-buffering with atomic swap (see Section 5).

---

## 11. Test Criteria Verification Plan

### Phase 2.1: Core STFT Engine

| Test | Method | Pass Criteria |
|------|--------|---------------|
| Audio pass-through | Sine wave input | No artifacts, clicks, or level changes |
| Null test | Input - Output | Silence (< -120dB with delay compensation) |
| Phase coherence | L+R sum of panned signal | No cancellation |
| Latency | DAW alignment test | 512-sample delay matches reported latency |

### Phase 2.2: Transient Detection

| Test | Method | Pass Criteria |
|------|--------|---------------|
| Impulse response | Single click | All bands detect transient |
| Sine wave | Steady tone | Low transient activity (< 0.1) |
| Drum loop | Mixed percussion | High activity on hits, low between |
| Sensitivity sweep | 0-100% range | Proportional detection response |

### Phase 2.3: Envelope Shaping

| Test | Method | Pass Criteria |
|------|--------|---------------|
| Attack boost | attackCurve[all] = +1.0 | Audible transient enhancement |
| Sustain cut | sustainCurve[all] = -1.0 | Reduced tail, tighter sound |
| Mix blend | 0-100% sweep | Smooth crossfade, no clicks |
| Output gain | -12 to +12 dB | Level changes correctly |

---

## 12. Summary and Recommendations

### 12.1 Implementation Order

1. **STFTProcessor class** - Sample-by-sample interface, perfect reconstruction
2. **Band boundary setup** - Logarithmic 32-band mapping
3. **Transient detection** - Spectral flux + dual envelopes per band
4. **Curve application** - Attack/sustain gain calculation
5. **Dry/wet mixing** - Latency-matched dry path
6. **Lookahead buffer** - Optional pre-triggering

### 12.2 JUCE Classes to Use

| Class | Purpose | Module |
|-------|---------|--------|
| `juce::dsp::FFT` | Forward/inverse transforms | juce_dsp |
| `juce::dsp::WindowingFunction<float>` | Hann window | juce_dsp |
| `juce::SmoothedValue<float>` | Gain smoothing | juce_core |
| `juce::Decibels` | dB ↔ linear conversion | juce_audio_basics |
| `juce::FloatVectorOperations` | SIMD-optimized math | juce_audio_basics |
| `juce::ScopedNoDenormals` | CPU protection | juce_audio_basics |

### 12.3 Files to Create

| File | Contents |
|------|----------|
| `Source/STFTProcessor.h` | Class declaration |
| `Source/STFTProcessor.cpp` | Implementation |

### 12.4 Files to Modify

| File | Changes |
|------|---------|
| `Source/PluginProcessor.h` | Add STFT instances, band structures, curve arrays |
| `Source/PluginProcessor.cpp` | Implement prepareToPlay(), processBlock() |

---

**Research Complete. Ready for Planning Phase.**

*Next: `/plugin-plan O-SpectralShaper 2-dsp`*
