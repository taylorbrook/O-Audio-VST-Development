# Technology Stack: O-Bass Psychoacoustic Bass Enhancer

**Project:** O-Bass
**Researched:** 2026-01-22
**Confidence:** HIGH (verified via official JUCE documentation and peer-reviewed psychoacoustic research)

## Executive Summary

O-Bass requires psychoacoustic harmonic generation to create perceived bass enhancement through the "missing fundamental" phenomenon. The implementation uses crossover filtering, nonlinear harmonic generation via waveshaping, harmonic amplitude weighting, and optional oversampling for clean mode. The JUCE DSP module provides all required building blocks.

---

## Recommended Stack

### Core JUCE DSP Classes

| Class | Purpose | Why This |
|-------|---------|----------|
| `dsp::LinkwitzRileyFilter` | Crossover splitting (~60-80Hz) | 4th-order (-24dB/oct) with flat sum response. Purpose-built for crossovers. TPT structure for stability. |
| `dsp::WaveShaper<float>` | Harmonic generation | Memoryless nonlinearity. Supports custom transfer functions. Low latency. |
| `dsp::IIR::Filter<float>` | Bandpass filtering, output filtering | Flexible coefficient design. Low CPU. Familiar Butterworth/Chebyshev options. |
| `dsp::Oversampling` | Anti-aliasing for clean mode | 2x-4x oversampling prevents waveshaper aliasing. FIR or IIR filter options. |
| `dsp::Gain<float>` | Level control, harmonic weighting | Block-optimized gain processing. |
| `dsp::ProcessSpec` | DSP initialization | Standard JUCE pattern for sample rate, block size, channel count. |

### Supporting Classes (Already in Codebase Pattern)

| Class | Purpose | Reference |
|-------|---------|-----------|
| `juce::AudioBuffer<float>` | Audio data container | Standard JUCE buffer |
| `juce::dsp::AudioBlock<float>` | Block-based processing view | Zero-copy wrapper around AudioBuffer |
| `juce::dsp::ProcessContextReplacing<float>` | In-place processing context | Standard JUCE DSP pattern |
| `juce::ScopedNoDenormals` | Prevent denormal slowdown | Critical for filters/waveshapers |

---

## DSP Techniques

### Primary Technique: Psychoacoustic Harmonic Generation

**The Science:** The human auditory system perceives pitch from harmonic relationships, not just fundamental frequency. When harmonics 2f, 3f, 4f, 5f are present, the brain perceives the fundamental f even if absent. This is the "missing fundamental" or "residue pitch" phenomenon.

**Signal Flow:**

```
Input ──┬─────────────────────────────────────────────────┬──> Mix ──> Output
        │                                                 │
        └─> LPF (60-80Hz) ──> Harmonics Gen ──> BPF ──> Gain ──┘
                   │                  │
               Crossover         Waveshaper + Weighting
```

1. **Crossover Split:** Separate bass content (<60-80Hz) from rest of signal
2. **Harmonic Generation:** Apply nonlinear function to generate harmonics
3. **Harmonic Shaping:** Bandpass filter to control harmonic range
4. **Amplitude Weighting:** Apply decreasing gains to higher harmonics
5. **Recombine:** Mix processed harmonics back with original signal

### Harmonic Generation Methods

#### Method 1: Waveshaping (Recommended for Colored Mode)

**Transfer Functions:**

```cpp
// Soft saturation - musical harmonics (all orders)
// Produces primarily odd harmonics with some even
auto tanhShaper = [](float x) { return std::tanh(x); };

// Polynomial for controlled harmonic content
// x^2 produces 2f (octave), x^3 produces 3f, etc.
auto polyShaper = [](float x) {
    float x2 = x * x;
    float x3 = x2 * x;
    return 0.5f * x2 + 0.3f * x3;  // Weight 2nd more than 3rd
};

// Full-wave rectifier - even harmonics only (2f, 4f, 6f)
auto rectifier = [](float x) { return std::abs(x); };

// Asymmetric soft clip - both even and odd harmonics
auto asymClip = [](float x) {
    return std::tanh(x + 0.1f * x * x);  // DC offset creates asymmetry
};
```

**Harmonic Content by Waveshaper Type:**

| Waveshaper | Even Harmonics | Odd Harmonics | Character |
|------------|----------------|---------------|-----------|
| tanh (symmetric) | No | Yes (3, 5, 7...) | Warm, tube-like |
| abs (rectifier) | Yes (2, 4, 6...) | No | Octave doubling |
| Polynomial | Controllable | Controllable | Precise |
| Asymmetric tanh | Yes | Yes | Full harmonic series |

**Recommendation:** Use asymmetric tanh or combined rectifier + clipper to generate both even and odd harmonics. The missing fundamental effect requires consecutive harmonics (2, 3, 4, 5).

#### Method 2: Full-Wave Integration (Recommended for Clean Mode)

Based on MATLAB psychoacoustic bass enhancement reference:

```cpp
// Full-wave integration generates harmonics differently
// Recursive: y[n] = (u[n] > 0 && u[n-1] <= 0) ? 0 : y[n-1] + u[n-1]
class FullWaveIntegrator {
    float lastInput = 0.0f;
    float accumulator = 0.0f;

    float processSample(float input) {
        if (input > 0.0f && lastInput <= 0.0f) {
            accumulator = 0.0f;  // Reset on positive zero-crossing
        } else {
            accumulator += lastInput;
        }
        lastInput = input;
        return accumulator;
    }
};
```

This produces a richer harmonic series with controllable decay.

### Harmonic Amplitude Weighting

**Research-Verified Ratios** (from Larsen et al., IEEE):

| Harmonic | Frequency | Attenuation | Linear Gain |
|----------|-----------|-------------|-------------|
| 2nd | 2f | -6dB | 0.5 |
| 3rd | 3f | -12dB | 0.25 |
| 4th | 4f | -18dB | 0.125 |
| 5th | 5f | -24dB | 0.0625 |

**Alternative Pattern** (0.707 decay per harmonic):

| Harmonic | Gain Multiplier |
|----------|-----------------|
| 2nd | 1.0 |
| 3rd | 0.707 |
| 4th | 0.5 |
| 5th | 0.354 |

The 3rd and 5th harmonics should be slightly emphasized over natural decay for optimal bass perception.

### Clean vs. Colored Modes

#### Clean Mode (Transparent)

- **Goal:** Add perceived bass weight without audible distortion
- **Approach:** Higher oversampling (4x), gentler waveshaping, tighter bandpass
- **Oversampling:** Required. 4x with FIR filters for linear phase
- **Waveshaper:** Mild polynomial or low-drive tanh
- **Post-filter:** Steep bandpass (60-300Hz) to remove upper harmonics

```cpp
// Clean mode setup
juce::dsp::Oversampling<float> oversampler(2, 2,
    juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true);

auto mildShaper = [](float x) {
    return std::tanh(x * 0.5f);  // Low drive
};
```

#### Colored Mode (Saturated)

- **Goal:** Audible warmth and thickness, musical distortion character
- **Approach:** More aggressive waveshaping, wider harmonic range
- **Oversampling:** Optional. 2x for efficiency, or none for aliased "grit"
- **Waveshaper:** Higher-drive tanh or asymmetric function
- **Post-filter:** Gentler bandpass or none (let harmonics extend higher)

```cpp
// Colored mode setup
auto aggressiveShaper = [](float x) {
    return std::tanh(x * 3.0f);  // High drive
};
```

---

## JUCE Implementation Patterns

### Crossover with LinkwitzRileyFilter

```cpp
class BassEnhancer {
    juce::dsp::LinkwitzRileyFilter<float> crossover;
    float crossoverFreq = 80.0f;

    void prepare(const juce::dsp::ProcessSpec& spec) {
        crossover.prepare(spec);
        crossover.setCutoffFrequency(crossoverFreq);
    }

    void processBlock(juce::AudioBuffer<float>& buffer) {
        juce::dsp::AudioBlock<float> block(buffer);

        // Get both outputs from crossover
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float input = buffer.getSample(ch, i);
                float lowOut, highOut;
                crossover.processSample(ch, input, lowOut, highOut);

                // lowOut goes to harmonic generator
                // highOut passes through
            }
        }
    }
};
```

### Waveshaper with Pre/Post Gain

```cpp
// Pattern from existing TapeDegrader.cpp
juce::dsp::WaveShaper<float> harmonicShaper;

// Initialize
harmonicShaper.functionToUse = [](float x) {
    return std::tanh(x);
};

// Process with drive
float drive = 2.0f;  // Adjustable intensity
for (int ch = 0; ch < numChannels; ++ch) {
    auto* data = buffer.getWritePointer(ch);
    juce::FloatVectorOperations::multiply(data, drive, numSamples);
}

juce::dsp::AudioBlock<float> block(buffer);
juce::dsp::ProcessContextReplacing<float> context(block);
harmonicShaper.process(context);

// Compensate output
float compensation = 1.0f / drive;
for (int ch = 0; ch < numChannels; ++ch) {
    auto* data = buffer.getWritePointer(ch);
    juce::FloatVectorOperations::multiply(data, compensation, numSamples);
}
```

### Oversampling for Anti-Aliasing

```cpp
class CleanBassEnhancer {
    juce::dsp::Oversampling<float> oversampler;
    juce::dsp::WaveShaper<float> shaper;

    CleanBassEnhancer()
        : oversampler(2, 2,  // 2 channels, 2^2 = 4x oversampling
            juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
            true)  // max quality
    {}

    void prepare(const juce::dsp::ProcessSpec& spec) {
        oversampler.initProcessing(spec.maximumBlockSize);

        // Prepare shaper at oversampled rate
        juce::dsp::ProcessSpec oversampledSpec = spec;
        oversampledSpec.sampleRate *= oversampler.getOversamplingFactor();
        shaper.prepare(oversampledSpec);
    }

    void processBlock(juce::AudioBuffer<float>& buffer) {
        juce::dsp::AudioBlock<float> block(buffer);

        // Upsample
        auto oversampledBlock = oversampler.processSamplesUp(block);

        // Apply waveshaping at higher sample rate
        juce::dsp::ProcessContextReplacing<float> context(oversampledBlock);
        shaper.process(context);

        // Downsample
        oversampler.processSamplesDown(block);
    }

    float getLatencyInSamples() const {
        return oversampler.getLatencyInSamples();
    }
};
```

### Bandpass for Harmonic Control

```cpp
// Bandpass to select harmonic range (e.g., 80-400Hz)
juce::dsp::IIR::Filter<float> harmonicBandpass;

void updateBandpass(double sampleRate) {
    // Center frequency and Q determine bandpass shape
    float centerFreq = 200.0f;  // Between bass and low-mids
    float Q = 0.707f;  // Moderate bandwidth

    auto coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(
        sampleRate, centerFreq, Q);
    *harmonicBandpass.coefficients = *coefficients;
}
```

---

## What NOT to Use and Why

### Avoid: StateVariableFilter for Crossover

**Why Not:** StateVariableFilter is designed for fast modulation of cutoff frequency with artifact prevention. For a fixed crossover, LinkwitzRileyFilter is purpose-built and more efficient.

### Avoid: ProcessorChain for This Use Case

**Why Not:** The bass enhancement signal flow requires parallel processing (original + processed bass), not sequential. ProcessorChain is for serial chains. Implement parallel routing manually.

### Avoid: High-Order Polynomial Waveshaping Without Oversampling

**Why Not:** "Each order of a polynomial for the waveshaping curve doubles the aliasing." A 5th-order polynomial needs 4x oversampling minimum. Use tanh (which approximates infinite-order polynomial) with proper oversampling, or stay with low-order.

### Avoid: Full-Wave Rectifier Alone

**Why Not:** Rectification produces only even harmonics (2f, 4f, 6f). The missing fundamental phenomenon requires consecutive harmonics. Combine with a half-wave rectifier or clipper for odd harmonics.

### Avoid: Pre-Waveshaper Highpass for Bass Enhancement

**Why Not:** JUCE tutorial recommends highpass before waveshaper to "prevent muddy low frequencies." This is backwards for bass enhancement - we specifically want to process low frequencies. The original signal path preserves clarity.

---

## Algorithm Summary

### Minimal Viable Implementation

```
1. Crossover filter at 60-80Hz (LinkwitzRileyFilter)
2. Low band: Apply waveshaper (tanh with drive control)
3. Optional: Bandpass filter harmonics (80-400Hz)
4. Apply output gain (intensity control)
5. Mix: Original + processed = output
```

### Full Implementation

```
1. Input gain stage
2. Crossover split (LinkwitzRileyFilter, 60-80Hz adjustable)
   - High path: Pass through
   - Low path: Continue to step 3
3. Convert low path to mono (sum L+R)
4. Oversampling up (optional, for clean mode)
5. Waveshaper (asymmetric tanh or polynomial)
6. Oversampling down
7. Bandpass filter (60-400Hz)
8. Harmonic weighting (apply decay curve)
9. Envelope follower (for dynamic response)
10. Intensity control (0-100%)
11. Mix with high path
12. Output gain compensation
```

---

## Parameter Recommendations

| Parameter | Range | Default | Purpose |
|-----------|-------|---------|---------|
| Crossover Freq | 40-120Hz | 80Hz | Where to split bass |
| Intensity/Amount | 0-100% | 50% | Wet/dry mix of harmonics |
| Drive | 1.0-5.0 | 2.0 | Waveshaper input gain |
| Mode | Clean/Colored | Clean | Oversampling + gentle vs. aggressive |
| Original Bass | -inf to 0dB | -6dB | How much fundamental to keep |
| Harmonic Range | 80-600Hz | 80-400Hz | Bandpass upper limit |

---

## Performance Considerations

| Operation | CPU Impact | Notes |
|-----------|------------|-------|
| LinkwitzRileyFilter | LOW | Single 4th-order filter |
| WaveShaper (tanh) | LOW | std::tanh or fast approximation |
| Oversampling 4x FIR | MEDIUM | ~4ms latency, high quality |
| Oversampling 4x IIR | LOW | <1ms latency, phase compromise |
| IIR Bandpass | LOW | Single biquad |

**Latency Budget:**
- No oversampling: <1 sample
- 2x IIR oversampling: ~0.5ms
- 4x FIR oversampling: ~4ms (report to DAW via getLatencyInSamples)

---

## Sources

### Official Documentation
- [JUCE dsp::LinkwitzRileyFilter](https://docs.juce.com/master/classdsp_1_1LinkwitzRileyFilter.html)
- [JUCE dsp::WaveShaper](https://docs.juce.com/master/structdsp_1_1WaveShaper.html)
- [JUCE dsp::Oversampling](https://docs.juce.com/master/classdsp_1_1Oversampling.html)
- [JUCE Tutorial: Introduction to DSP](https://docs.juce.com/master/tutorial_dsp_introduction.html)
- [JUCE Tutorial: Distortion through Waveshaping](https://juce.com/tutorials/tutorial_dsp_convolution/)

### Psychoacoustic Research
- [MathWorks: Psychoacoustic Bass Enhancement for Band-Limited Signals](https://www.mathworks.com/help/audio/ug/psychoacoustic-bass-enhancement-for-band-limited-signals.html) - Full algorithm reference
- [IEEE: A psychoacoustic bass enhancement system with improved transient and steady-state performance](https://ieeexplore.ieee.org/document/6287837/) - Harmonic weighting research
- [Wikipedia: Missing fundamental](https://en.wikipedia.org/wiki/Missing_fundamental) - Foundational psychoacoustics
- [HeadWize: The Psychoacoustic Bass Enhancer](https://headwizememorial.wordpress.com/2018/03/17/the-psychoacoustic-bass-enhancer/) - Implementation specifics

### Industry References
- [Waves: Bass Plugins and Sub Enhancers Compared](https://www.waves.com/bass-plugins-and-sub-enhancers-compared)
- [KVR: The secret of bass enhancement explained](https://www.kvraudio.com/forum/viewtopic.php?t=515885)
- [Sound on Sound: Waves MaxxBass Review](https://www.soundonsound.com/reviews/waves-maxxbass)
- [Medium: Subharmonic Synthesis for Bass Enhancement](https://medium.com/@franz.bender/subharmonic-synthesis-for-bass-enhancement-812b58eca930)

### DSP Theory
- [KVR: Waveshaping and Harmonic Content](https://www.kvraudio.com/forum/viewtopic.php?t=587580)
- [KVR: Tanh approximations](https://www.kvraudio.com/forum/viewtopic.php?t=262823)
- [MusicDSP: Variable-hardness clipping function](https://www.musicdsp.org/en/latest/Effects/104-variable-hardness-clipping-function.html)
