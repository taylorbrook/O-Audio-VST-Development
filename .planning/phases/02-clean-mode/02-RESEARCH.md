# Phase 2: Clean Mode - Research

**Researched:** 2026-01-23
**Domain:** Psychoacoustic harmonic generation, oversampling, pitch tracking, transient handling
**Confidence:** MEDIUM (well-documented domain, JUCE APIs verified, some implementation details require validation)

## Summary

This phase implements psychoacoustic bass enhancement using the "missing fundamental" phenomenon - generating harmonics in the 100-400Hz range that create perceived bass weight on bandwidth-limited playback systems (phones, laptops). The approach is well-established with expired patents (2006-2008) and documented implementations in MATLAB and commercial products like MaxxBass.

The core signal flow is: crossover-filtered bass -> pitch detection -> harmonic generation -> oversampling for alias-free processing -> envelope-controlled blending -> transient ducking -> output. The existing Phase 1 architecture (crossover, mono summer) provides the foundation; Phase 2 adds the harmonic generation core, 4x oversampling, pitch tracking, and transient-aware envelope control.

Key technical challenges: (1) accurate monophonic pitch tracking at bass frequencies (40-200Hz), (2) alias-free harmonic generation via 4x oversampling, (3) transient detection with minimal latency, and (4) spectral-aware blending to avoid harmonic buildup where high band already contains energy.

**Primary recommendation:** Use a Chebyshev polynomial waveshaper for controlled harmonic generation with 4x JUCE dsp::Oversampling (IIR/polyphase for Low Latency mode, FIR for High Fidelity mode), paired with autocorrelation-based pitch tracking (YIN algorithm adapted for bass range) and a dual-time-constant envelope follower with transient ducking.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE dsp::Oversampling | 7.x | 4x oversampling for alias-free harmonic generation | Built-in, CPU-efficient multi-stage halfband filters |
| JUCE dsp::WaveShaper | 7.x | Chebyshev polynomial application | Direct integration with JUCE audio blocks |
| Custom YIN pitch detector | N/A | Monophonic bass frequency tracking | Standard algorithm, tunable for bass range |
| Custom EnvelopeFollower | N/A | Transient detection and ducking | Simple IIR filter with attack/release coefficients |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| JUCE SmoothedValue | 7.x | Parameter smoothing | Harmonic mix, enhance knob |
| JUCE dsp::IIR::Filter | 7.x | Bandpass for harmonic output | Limit harmonics to 60-400Hz range |
| juce::FloatVectorOperations | 7.x | SIMD-optimized vector math | Bulk sample processing |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Chebyshev waveshaper | Full-wave rectifier | Simpler but only generates even harmonics; need HWR+clipper combo for all harmonics |
| YIN pitch detection | MPM (McLeod Pitch Method) | Similar accuracy, MPM may be slightly faster but YIN better documented |
| Custom envelope follower | JUCE dsp::BallisticsFilter | BallisticsFilter designed for compressors; custom gives more control over transient detection |

**No additional dependencies needed** - all components built on JUCE DSP module or custom implementations.

## Architecture Patterns

### Recommended Project Structure
```
plugins/OBass/Source/
  DSP/
    CrossoverFilter.h/cpp      # [Phase 1 - exists]
    MonoSummer.h/cpp           # [Phase 1 - exists]
    HarmonicGenerator.h/cpp    # [NEW] Chebyshev waveshaper + oversampling
    PitchTracker.h/cpp         # [NEW] YIN-based monophonic pitch detection
    EnvelopeFollower.h/cpp     # [NEW] Attack/release envelope with transient detection
    CleanModeProcessor.h/cpp   # [NEW] Orchestrates harmonic generation pipeline
```

### Pattern 1: Psychoacoustic Bass Enhancement Signal Flow
**What:** The MaxxBass-style algorithm that generates perceived bass
**When to use:** Always - this is the core of Clean Mode
**Example:**
```cpp
// Source: MATLAB audioexample.BassEnhancer, verified with IEEE 6287837
// Signal flow in CleanModeProcessor::process()

// 1. Input: mono bass signal from crossover (already filtered by Phase 1)
// 2. Detect fundamental frequency (pitch tracking)
float fundamentalHz = pitchTracker.detectPitch(monoInput, numSamples);

// 3. Calculate harmonic count based on input frequency
// Lower frequencies need more harmonics to create effect
int harmonicCount = calculateAdaptiveHarmonicCount(fundamentalHz);

// 4. Generate envelope for transient-aware processing
envelopeFollower.process(monoInput, envelope, numSamples);

// 5. Upsample for alias-free harmonic generation
auto oversampledBlock = oversampler.processSamplesUp(inputBlock);

// 6. Apply Chebyshev waveshaper to generate harmonics
harmonicGenerator.process(oversampledBlock, fundamentalHz, harmonicCount);

// 7. Downsample back to original rate
oversampler.processSamplesDown(outputBlock);

// 8. Apply transient ducking (reduce harmonics on attacks)
applyTransientDucking(outputBlock, envelope, numSamples);

// 9. Spectral-aware blend (reduce if high band already has energy)
blendWithSpectralAwareness(outputBlock, highBandEnergy);
```

### Pattern 2: Chebyshev Polynomial Waveshaper
**What:** Controlled harmonic generation where Tn(cos(x)) = cos(nx)
**When to use:** When you need to generate specific harmonics from a sinusoidal input
**Example:**
```cpp
// Source: musicdsp.org polynomial waveshaper, KVR forum discussions
// Chebyshev polynomials T1-T8 for harmonic generation

// First 8 Chebyshev polynomials (input x should be normalized to [-1, 1])
inline float T1(float x) { return x; }                                           // fundamental
inline float T2(float x) { return 2.0f*x*x - 1.0f; }                            // 2nd harmonic
inline float T3(float x) { return 4.0f*x*x*x - 3.0f*x; }                        // 3rd harmonic
inline float T4(float x) { return 8.0f*x*x*x*x - 8.0f*x*x + 1.0f; }             // 4th harmonic
inline float T5(float x) { return 16.0f*x*x*x*x*x - 20.0f*x*x*x + 5.0f*x; }     // 5th harmonic

// Combined waveshaper with weighted harmonics
float generateHarmonics(float input, const float* weights, int numHarmonics) {
    float output = 0.0f;
    // Note: Only works correctly for sinusoidal input normalized to [-1, 1]
    output += weights[0] * T1(input);  // fundamental (may reduce or keep)
    output += weights[1] * T2(input);  // 2nd - warm, octave
    output += weights[2] * T3(input);  // 3rd - adds body, fifth
    output += weights[3] * T4(input);  // 4th - brightness
    if (numHarmonics > 4)
        output += weights[4] * T5(input);  // 5th - edge
    return output;
}
```

### Pattern 3: Envelope Follower with Attack/Release
**What:** Dual-coefficient IIR filter that tracks signal envelope with different rise/fall times
**When to use:** Transient detection, dynamic harmonic control
**Example:**
```cpp
// Source: musicdsp.org envelope follower, verified with multiple KVR threads
class EnvelopeFollower {
public:
    void setAttackMs(float ms, double sampleRate) {
        // Time constant: time to fall from 100% to 1%
        attackCoef = std::exp(std::log(0.01f) / (ms * 0.001f * sampleRate));
    }

    void setReleaseMs(float ms, double sampleRate) {
        releaseCoef = std::exp(std::log(0.01f) / (ms * 0.001f * sampleRate));
    }

    float process(float input) {
        float absInput = std::abs(input);
        if (absInput > envelope)
            envelope = attackCoef * (envelope - absInput) + absInput;
        else
            envelope = releaseCoef * (envelope - absInput) + absInput;
        return envelope;
    }

private:
    float attackCoef = 0.0f;
    float releaseCoef = 0.0f;
    float envelope = 0.0f;
};
```

### Pattern 4: JUCE Oversampling Integration
**What:** 4x oversampling with latency-mode-appropriate filter selection
**When to use:** Before any nonlinear processing (waveshaping, saturation)
**Example:**
```cpp
// Source: JUCE docs, DSPModulePluginDemo.h
// Constructor - 2 channels, factor 2 (2^2 = 4x), filter type varies by mode

// Low Latency mode: IIR polyphase (minimal latency, some phase distortion)
juce::dsp::Oversampling<float> oversamplerIIR {
    2,  // numChannels (will be mono, but allocate for safety)
    2,  // factor (2^2 = 4x oversampling)
    juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
    true,  // isMaxQuality
    true   // useIntegerLatency (for easier DAW compensation)
};

// High Fidelity mode: FIR equiripple (linear phase, more latency)
juce::dsp::Oversampling<float> oversamplerFIR {
    2, 2,
    juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
    true, true
};

// In prepareToPlay:
oversampler.initProcessing(samplesPerBlock);
setLatencySamples(crossover.getLatencyInSamples() +
                  static_cast<int>(oversampler.getLatencyInSamples()));

// In processBlock:
auto oversampledBlock = oversampler.processSamplesUp(inputBlock);
// ... process at 4x sample rate ...
oversampler.processSamplesDown(outputBlock);
```

### Anti-Patterns to Avoid
- **Processing complex waveforms through Chebyshev directly:** Chebyshev polynomials only produce clean harmonics from pure sinusoids. For complex bass signals, the harmonics will include intermodulation products. Accept this as "character" or pre-filter to near-sinusoidal.
- **Switching oversampler filter type at runtime:** This requires re-initialization. Use separate oversampler instances for each mode (like Phase 1 crossover approach).
- **Forgetting to report combined latency:** Oversampling adds latency on top of crossover FIR latency. Total must be reported to DAW.
- **Envelope follower with 0ms attack:** Will cause clicking. Minimum ~0.1-1ms attack for smooth response.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Oversampling filters | Custom halfband filters | JUCE dsp::Oversampling | Multi-stage halfband design is complex; JUCE handles polyphase/FIR selection, latency tracking |
| Parameter smoothing | Manual interpolation | juce::SmoothedValue | Handles edge cases, multiplicative vs linear modes |
| Sample-rate-aware timing | Manual coefficient scaling | Formula: `exp(log(0.01)/(ms * sr * 0.001))` | Time constant math is error-prone; use established formula |
| Vector operations | Manual SIMD | juce::FloatVectorOperations | Platform-optimized, handles alignment |
| Pitch period detection | FFT peak finding | YIN/autocorrelation | FFT resolution insufficient for bass; autocorrelation is standard |

**Key insight:** The harmonic generation domain is well-understood with established formulas. Custom solutions risk aliasing, incorrect time constants, or phase issues. Use proven patterns and formulas.

## Common Pitfalls

### Pitfall 1: Aliasing from Harmonic Generation
**What goes wrong:** Generated harmonics above Nyquist fold back as audible artifacts (harsh, metallic sounds)
**Why it happens:** A 50Hz input with 5 harmonics generates up to 250Hz - but waveshaper nonlinearity creates ALL harmonics, not just intended ones. At 44.1kHz, harmonics above 22kHz alias.
**How to avoid:** Always use 4x oversampling BEFORE waveshaping. Process at 176.4kHz, then filter during downsample.
**Warning signs:** Harsh "digital" artifacts, especially on high enhance settings or higher crossover frequencies.

### Pitfall 2: Intermodulation from Complex Input
**What goes wrong:** Chebyshev polynomials generate sum/difference frequencies when input contains multiple sinusoids
**Why it happens:** Nonlinear functions: (a+b)^n contains cross-terms like a*b, not just a^n + b^n
**How to avoid:** Accept as "character" (adds warmth); or pre-filter bass to emphasize fundamental before waveshaping. Monophonic pitch tracking helps lock onto dominant frequency.
**Warning signs:** "Muddy" or "beating" artifacts when multiple bass notes play simultaneously.

### Pitfall 3: Transient Smearing
**What goes wrong:** Kick drums lose punch; attacks become soft
**Why it happens:** Oversampling filters have group delay; envelope followers add latency; harmonic generation "softens" transients
**How to avoid:** Implement transient ducking - detect attacks via fast envelope follower, reduce harmonic mix during transients. In High Fidelity mode, use lookahead to detect transients before they arrive.
**Warning signs:** Comparison with bypass shows "softer" attacks; kick drums lose definition.

### Pitfall 4: Harmonic Buildup at Crossover
**What goes wrong:** Harsh resonance or "honking" around crossover frequency (e.g., 80Hz)
**Why it happens:** Generated harmonics overlap with high band content that already exists
**How to avoid:** Spectral-aware blending - analyze high band energy in 80-400Hz region; reduce harmonic generation proportionally where overlap exists.
**Warning signs:** A/B testing shows "nasal" or "resonant" quality around crossover frequency.

### Pitfall 5: Wrong Pitch Detection for Bass
**What goes wrong:** Pitch tracker locks onto harmonics instead of fundamental; octave errors
**Why it happens:** Autocorrelation can confuse harmonic periods with fundamental period; bass frequencies (40Hz) need long analysis windows (50ms+)
**How to avoid:** Use YIN algorithm with bass-optimized parameters: minimum frequency 30Hz, analysis window >= 50ms. Apply parabolic interpolation for sub-sample accuracy.
**Warning signs:** Harmonics jump octaves unexpectedly; "warbling" or inconsistent enhancement.

### Pitfall 6: Click on Latency Mode Switch
**What goes wrong:** Audible click when switching between Low Latency and High Fidelity modes
**Why it happens:** Different oversamplers have different latencies; sudden latency change causes discontinuity
**How to avoid:** Report latency change to DAW; implement short crossfade (~10ms) during mode transition. Or: accept that users switch modes between songs, not during playback.
**Warning signs:** Click/pop sound when changing latency mode parameter.

## Code Examples

Verified patterns from official sources:

### YIN Pitch Detection Core (Adapted for Bass)
```cpp
// Source: de Cheveigne & Kawahara 2002, adapted from sevagh/pitch-detection
// Key insight: YIN needs 2 periods minimum; at 40Hz that's 50ms = 2205 samples at 44.1kHz

class PitchTracker {
public:
    void prepare(double sampleRate, int maxBlockSize) {
        this->sampleRate = sampleRate;
        // Analysis window: enough for 2 periods of lowest expected frequency (30Hz)
        windowSize = static_cast<int>(sampleRate / 30.0) * 2;  // ~2940 samples at 44.1kHz
        windowSize = juce::jmin(windowSize, 4096);  // Cap for performance

        buffer.resize(windowSize);
        yinBuffer.resize(windowSize / 2);
        writeIndex = 0;
    }

    float detectPitch(const float* input, int numSamples) {
        // Accumulate samples into ring buffer
        for (int i = 0; i < numSamples; ++i) {
            buffer[writeIndex] = input[i];
            writeIndex = (writeIndex + 1) % windowSize;
        }

        // YIN difference function
        int halfWindow = windowSize / 2;
        float runningSum = 0.0f;
        yinBuffer[0] = 1.0f;

        for (int tau = 1; tau < halfWindow; ++tau) {
            float delta = 0.0f;
            for (int j = 0; j < halfWindow; ++j) {
                float diff = buffer[j] - buffer[j + tau];
                delta += diff * diff;
            }
            runningSum += delta;
            yinBuffer[tau] = delta * tau / runningSum;  // Cumulative mean normalized
        }

        // Find first minimum below threshold
        constexpr float threshold = 0.1f;  // YIN threshold, lower = stricter
        int tauEstimate = -1;
        for (int tau = 2; tau < halfWindow; ++tau) {
            if (yinBuffer[tau] < threshold) {
                while (tau + 1 < halfWindow && yinBuffer[tau + 1] < yinBuffer[tau])
                    ++tau;
                tauEstimate = tau;
                break;
            }
        }

        if (tauEstimate < 0)
            return 0.0f;  // No pitch detected

        // Parabolic interpolation for sub-sample accuracy
        float betterTau = parabolicInterpolation(tauEstimate);
        return static_cast<float>(sampleRate / betterTau);
    }

private:
    float parabolicInterpolation(int tau) {
        if (tau < 1 || tau >= static_cast<int>(yinBuffer.size()) - 1)
            return static_cast<float>(tau);

        float s0 = yinBuffer[tau - 1];
        float s1 = yinBuffer[tau];
        float s2 = yinBuffer[tau + 1];
        float adjustment = (s2 - s0) / (2.0f * (2.0f * s1 - s2 - s0));
        return tau + adjustment;
    }

    double sampleRate = 44100.0;
    int windowSize = 2048;
    int writeIndex = 0;
    std::vector<float> buffer;
    std::vector<float> yinBuffer;
};
```

### Adaptive Harmonic Count
```cpp
// Source: Context decision + psychoacoustic research
// Lower frequencies need more harmonics to create missing fundamental effect

int calculateAdaptiveHarmonicCount(float fundamentalHz) {
    // Sub-bass (<40Hz): maximum harmonics - content nearly inaudible
    if (fundamentalHz < 40.0f)
        return 5;

    // Deep bass (40-80Hz): still needs strong harmonic support
    if (fundamentalHz < 80.0f)
        return 4;

    // Mid-bass (80-120Hz): moderate enhancement
    if (fundamentalHz < 120.0f)
        return 3;

    // Upper bass (120-200Hz): minimal - already somewhat audible
    return 2;
}
```

### Harmonic Weight Recommendations
```cpp
// Source: Psychoacoustic research, even/odd harmonic character
// Even harmonics (2, 4) = warm, smooth, octave-related
// Odd harmonics (3, 5) = presence, clarity, "tubelike"

struct HarmonicWeights {
    float h2 = 0.7f;   // 2nd harmonic: strong for warmth (octave)
    float h3 = 0.5f;   // 3rd harmonic: moderate for body (fifth)
    float h4 = 0.3f;   // 4th harmonic: light for clarity (2 octaves)
    float h5 = 0.15f;  // 5th harmonic: subtle for presence
};

// For "transparent" enhancement, slightly favor even harmonics
// These weights produce 2nd > 3rd > 4th > 5th cascade
// Adjust ratios for different character (more odd = more "tube", more even = more "tape")
```

### Transient Ducking Implementation
```cpp
// Source: IEEE 6287837 (transient/steady-state separation concept)
// Detect transients and reduce harmonic mix to preserve punch

class TransientDucker {
public:
    void prepare(double sampleRate) {
        // Fast envelope for transient detection
        fastEnv.setAttackMs(0.5f, sampleRate);   // Very fast attack
        fastEnv.setReleaseMs(20.0f, sampleRate);

        // Slow envelope for average level
        slowEnv.setAttackMs(5.0f, sampleRate);
        slowEnv.setReleaseMs(100.0f, sampleRate);

        this->sampleRate = sampleRate;
    }

    // Returns ducking gain (1.0 = no ducking, 0.0 = full duck)
    float process(float input) {
        float fast = fastEnv.process(input);
        float slow = slowEnv.process(input);

        // Transient detected when fast >> slow
        float ratio = (slow > 0.0001f) ? (fast / slow) : 1.0f;

        // Duck when ratio exceeds threshold
        constexpr float threshold = 2.0f;    // Transient = 2x average
        constexpr float duckAmount = 0.3f;   // Duck to 30% on transients

        if (ratio > threshold) {
            float duckDepth = juce::jmin(1.0f, (ratio - threshold) / threshold);
            return 1.0f - duckDepth * (1.0f - duckAmount);
        }
        return 1.0f;
    }

private:
    EnvelopeFollower fastEnv, slowEnv;
    double sampleRate = 44100.0;
};
```

### Lookahead for High Fidelity Mode
```cpp
// Source: KVR forum discussions on lookahead limiters
// Lookahead = delay input, let envelope "see" future

class LookaheadBuffer {
public:
    void prepare(int lookaheadSamples, int numChannels) {
        this->lookaheadSamples = lookaheadSamples;
        delayBuffer.setSize(numChannels, lookaheadSamples);
        delayBuffer.clear();
        writePos = 0;
    }

    // Returns delayed sample, stores current for later
    float process(int channel, float input) {
        int readPos = (writePos + 1) % lookaheadSamples;
        float delayed = delayBuffer.getSample(channel, readPos);
        delayBuffer.setSample(channel, writePos, input);
        writePos = (writePos + 1) % lookaheadSamples;
        return delayed;
    }

    int getLatency() const { return lookaheadSamples; }

private:
    int lookaheadSamples = 0;
    int writePos = 0;
    juce::AudioBuffer<float> delayBuffer;
};

// Usage in High Fidelity mode:
// 1. Feed envelope follower the undelayed signal (sees transients early)
// 2. Process the delayed signal with the envelope
// Typical lookahead: 1-5ms (44-220 samples at 44.1kHz)
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Full-wave rectifier only | Chebyshev polynomials | ~2005 | Controlled harmonic ratios, both even and odd |
| Fixed harmonic count | Adaptive based on frequency | Current best practice | Better enhancement for deep sub-bass |
| No transient handling | Transient detection + ducking | ~2012 (IEEE paper) | Preserved attack character |
| 2x oversampling | 4x minimum for harmonics | Current consensus | Sufficient headroom for 5+ harmonics |

**Deprecated/outdated:**
- Simple rectifier-only approaches: Only generate even harmonics, pitch sounds doubled
- Fixed 2x oversampling for heavy distortion: Insufficient for clean harmonic generation
- Block-based pitch detection with FFT: Latency too high, resolution insufficient for bass

## Open Questions

Things that couldn't be fully resolved:

1. **Optimal harmonic weighting ratios for "transparent" enhancement**
   - What we know: Even harmonics = warm, odd = present; 2nd harmonic most important
   - What's unclear: Exact ratios for "most transparent" enhancement vary by source material
   - Recommendation: Start with weights [0.7, 0.5, 0.3, 0.15] for H2-H5; provide "enhance" control that scales all proportionally

2. **Spectral awareness implementation specifics**
   - What we know: Need to reduce harmonics where high band already has energy
   - What's unclear: Whether to use simple energy detection, FFT analysis, or filterbank
   - Recommendation: Start simple - low-passed energy measurement of high band in 80-400Hz region; scale harmonic mix inversely

3. **Crossover boundary transition shape**
   - What we know: User decision marked as "Claude's discretion"
   - What's unclear: Sharp cutoff may cause harmonic discontinuity; soft cutoff may muddy enhancement
   - Recommendation: The Phase 1 crossover is already LR4 24dB/oct - use as-is; harmonics are bandpassed anyway, limiting their reach

4. **Optimal YIN threshold for bass**
   - What we know: Standard YIN uses 0.1-0.2 threshold
   - What's unclear: Bass signals may need different threshold due to lower partials ratio
   - Recommendation: Start with 0.1; if octave errors occur, lower to 0.05; expose as internal tuning parameter if needed

## Sources

### Primary (HIGH confidence)
- JUCE dsp::Oversampling official documentation - https://docs.juce.com/master/classdsp_1_1Oversampling.html
- JUCE DSPModulePluginDemo.h - verified oversampling + waveshaper patterns
- musicdsp.org - envelope follower formula, Chebyshev polynomial coefficients

### Secondary (MEDIUM confidence)
- MATLAB audioexample.BassEnhancer documentation - psychoacoustic bass enhancement signal flow
- IEEE 6287837 - transient/steady-state separation for bass enhancement
- de Cheveigne & Kawahara 2002 - YIN pitch detection algorithm
- KVR Audio DSP Forum - multiple threads on envelope followers, oversampling, transient detection

### Tertiary (LOW confidence - for validation)
- Various blog posts on even/odd harmonic character - generally consistent, but subjective
- Specific harmonic weight recommendations - starting points that may need tuning

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - JUCE APIs verified, formulas well-documented
- Architecture: MEDIUM - signal flow established, but integration with Phase 1 needs validation
- Pitfalls: HIGH - well-documented in DSP forums, aliasing is known issue
- Harmonic weights: LOW - subjective, requires listening tests

**Research date:** 2026-01-23
**Valid until:** 60 days (stable domain, no rapidly changing APIs)
