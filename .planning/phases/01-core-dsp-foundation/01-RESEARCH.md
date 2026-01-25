# Phase 1: Core DSP Foundation - Research

**Researched:** 2026-01-22
**Domain:** JUCE DSP crossover filtering, mono bass summing, latency management
**Confidence:** HIGH

## Summary

This phase establishes the audio signal path architecture for the O-Bass plugin: a dual-mode crossover filter (IIR low-latency and FIR linear-phase), mono bass summing, bypass behavior, and latency reporting to the host. The research validates that JUCE 8.0.9 (the version in use) provides all necessary DSP primitives for this implementation.

The core decision to implement two distinct processing modes (Low-latency IIR vs High-fidelity FIR) is sound and aligns with professional audio plugin design. The `juce::dsp::LinkwitzRileyFilter` class provides 24dB/octave (LR4) crossover filtering out of the box. For the linear-phase FIR mode, JUCE's `Convolution` class with generated FIR coefficients will work well.

**Primary recommendation:** Use JUCE's built-in `dsp::LinkwitzRileyFilter` for IIR mode and `dsp::Convolution` with windowed-sinc FIR coefficients for linear-phase mode. Pre-allocate all buffers in `prepareToPlay()` for both modes regardless of current selection.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.0.9 | Audio plugin framework | Already in use, provides all DSP primitives needed |
| juce_dsp module | 8.0.9 | DSP building blocks | Contains LinkwitzRileyFilter, Convolution, SmoothedValue |
| juce_audio_processors | 8.0.9 | Plugin architecture | APVTS for thread-safe parameters |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `dsp::LinkwitzRileyFilter` | 8.0.9 | IIR crossover (LR4, 24dB/oct) | Low-latency mode |
| `dsp::Convolution` | 8.0.9 | FIR convolution | Linear-phase high-fidelity mode |
| `dsp::ProcessSpec` | 8.0.9 | DSP initialization context | All DSP component setup |
| `SmoothedValue<float>` | 8.0.9 | Parameter interpolation | Crossover frequency changes |
| `AudioBuffer<float>` | 8.0.9 | Sample buffer management | All intermediate buffers |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `dsp::LinkwitzRileyFilter` | Custom cascaded Butterworth | More control but unnecessary complexity |
| `dsp::Convolution` | `dsp::FIR::Filter` | FIRFilter faster for <128 taps but we need longer for crossover |
| Manual coefficient smoothing | `SmoothedValue` | SmoothedValue proven in existing codebase |

**No additional libraries needed.** JUCE 8.0.9 provides everything required.

## Architecture Patterns

### Recommended Project Structure
```
Source/
  PluginProcessor.h/.cpp     # Main processor with mode routing
  PluginEditor.h/.cpp        # UI (minimal for Phase 1)
  DSP/
    CrossoverFilter.h/.cpp   # Encapsulates dual-mode crossover
    MonoSummer.h/.cpp        # L+R summing for bass band
    ProcessingMode.h         # Enum and mode switching logic
```

### Pattern 1: Dual-Mode Crossover Architecture
**What:** Encapsulate both IIR and FIR crossover implementations behind a common interface
**When to use:** When plugin needs switchable low-latency and high-fidelity modes

```cpp
// CrossoverFilter.h
class CrossoverFilter {
public:
    enum class Mode { LowLatency, HighFidelity };

    void setMode(Mode newMode);
    void setCutoffFrequency(float freqHz);
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    // Process and get split bands
    void process(juce::AudioBuffer<float>& input,
                 juce::AudioBuffer<float>& lowBand,
                 juce::AudioBuffer<float>& highBand);

    int getLatencyInSamples() const;

private:
    Mode currentMode = Mode::LowLatency;

    // IIR mode (low-latency)
    juce::dsp::LinkwitzRileyFilter<float> iirLowpass;
    juce::dsp::LinkwitzRileyFilter<float> iirHighpass;

    // FIR mode (linear-phase)
    juce::dsp::Convolution firLowpass;
    juce::dsp::Convolution firHighpass;

    juce::SmoothedValue<float> cutoffFrequency;
};
```

### Pattern 2: Pre-Allocated Buffer Strategy
**What:** Allocate all buffers for worst-case scenario at prepare time
**When to use:** Always for real-time audio processing

```cpp
void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Pre-allocate for BOTH modes, regardless of current setting
    // This ensures mode switches never allocate

    lowBandBuffer.setSize(2, samplesPerBlock);
    highBandBuffer.setSize(2, samplesPerBlock);
    monoBuffer.setSize(1, samplesPerBlock);

    // FIR buffers (largest requirement)
    int maxFirTaps = calculateFirTapsForLowestCrossover(40.0f, sampleRate);
    // Convolution handles internal buffers but we pre-create the IR

    crossover.prepare({sampleRate, (uint32)samplesPerBlock, 2});
}
```

### Pattern 3: Bypass with Explicit Signal Path
**What:** Bypass completely removes processing chain, not just dry mix
**When to use:** User-exposed bypass control

```cpp
void PluginProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer&) {
    ScopedNoDenormals noDenormals;

    if (bypassEnabled) {
        // TRUE BYPASS: signal passes through unmodified
        // No latency compensation applied to dry signal
        return;
    }

    // Normal processing path
    crossover.process(buffer, lowBandBuffer, highBandBuffer);
    monoSummer.process(lowBandBuffer);
    // ... enhancement processing ...
    recombineBands(buffer, lowBandBuffer, highBandBuffer);
}
```

### Pattern 4: Latency Reporting Strategy
**What:** Report accurate latency per mode, update on mode switch
**When to use:** Plugins with variable latency based on settings

```cpp
void PluginProcessor::setProcessingMode(Mode mode) {
    currentMode = mode;

    // Report new latency to host immediately
    int latencySamples = crossover.getLatencyInSamples();
    setLatencySamples(latencySamples);

    // Note: Host support for dynamic latency varies
    // Pro Tools handles it well, Logic/Live may have issues
}
```

### Anti-Patterns to Avoid
- **Allocating in processBlock:** Never use `new`, `std::vector` resizing, or string operations in audio thread
- **Assuming instant mode switch:** Mode changes should crossfade or allow for a small click (per context decision)
- **Using IIR.state directly for coefficient updates:** This can cause clicks; use SmoothedValue or update per-sample
- **Reporting max latency always:** Wastes latency budget in low-latency mode; report accurate values

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Linkwitz-Riley crossover | Cascaded Butterworth filters | `dsp::LinkwitzRileyFilter` | JUCE's implementation is TPT-based, stable, and tested |
| FIR convolution | Direct time-domain convolution | `dsp::Convolution` | Frequency-domain convolution more efficient for >64 taps |
| Parameter smoothing | Manual linear interpolation | `SmoothedValue<float>` | Handles edge cases, thread-safe, proven pattern |
| Thread-safe parameters | Manual atomics | `AudioProcessorValueTreeState` | Already standard in codebase, handles automation |
| Denormal handling | Manual checks | `ScopedNoDenormals` | Single line, cross-platform, no overhead |

**Key insight:** JUCE's DSP module is specifically designed for these exact use cases. Custom implementations risk subtle bugs (filter instability, denormals, thread safety issues) that JUCE's code has already solved.

## Common Pitfalls

### Pitfall 1: Allocating Memory in processBlock
**What goes wrong:** Audio dropouts, clicks, unpredictable glitches
**Why it happens:** Memory allocation can block, trigger garbage collection, or wait for locks
**How to avoid:** Pre-allocate ALL buffers in `prepareToPlay()`, including worst-case FIR mode buffers even when in IIR mode
**Warning signs:** Intermittent glitches that don't reproduce consistently, performance varies with system load

### Pitfall 2: Clicks When Changing Crossover Frequency
**What goes wrong:** Audible zipper noise or clicks when adjusting frequency parameter
**Why it happens:** Abrupt IIR coefficient changes cause transients; SmoothedValue alone may not be sufficient
**How to avoid:** For IIR mode, smooth the cutoff frequency parameter over ~5-10ms using SmoothedValue with multiplicative smoothing (better for frequency). For FIR mode, crossfade between old and new impulse responses.
**Warning signs:** Clicks when automating crossover frequency, worse with fast modulation

### Pitfall 3: Phase Issues When Recombining Bands
**What goes wrong:** Frequency dip or boost at crossover frequency, hollow sound
**Why it happens:** Low and high bands have phase mismatch; Linkwitz-Riley requires correct summing
**How to avoid:** LR4 (24dB/oct) sums flat when low and high outputs are added directly (both at -6dB at crossover). Do NOT invert either band. Use `processSample(channel, input, lowOut, highOut)` method.
**Warning signs:** Comb filtering audible near crossover, bass sounds thin

### Pitfall 4: Incorrect Latency Reporting
**What goes wrong:** Plugin not time-aligned with other tracks in DAW
**Why it happens:** Forgot to call `setLatencySamples()`, or calculated samples incorrectly
**How to avoid:**
- IIR mode: latency is 0 (or nearly 0 for any internal smoothing)
- FIR mode: latency = (numTaps - 1) / 2 for symmetric linear-phase filter
- Call `setLatencySamples()` in `prepareToPlay()` AND when mode changes
**Warning signs:** Transients misaligned when A/B testing with bypass, timing drift in parallel setups

### Pitfall 5: FIR Filter Too Short for Low Crossover Frequency
**What goes wrong:** Poor frequency response, ripple in passband/stopband
**Why it happens:** FIR filter length must increase as cutoff frequency decreases relative to sample rate
**How to avoid:** Calculate required taps: `numTaps = attenuation_dB / (22 * (fStop - fPass) / sampleRate)`. For 40Hz crossover at 44.1kHz with 80dB attenuation, need ~4000+ taps.
**Warning signs:** Audible ripple, crossover doesn't match IIR mode behavior

### Pitfall 6: Mono Sum Phase Cancellation
**What goes wrong:** Bass disappears or sounds weak after mono summing
**Why it happens:** L and R channels have opposite polarity in low frequencies (rare but possible)
**How to avoid:** Simple (L+R)/2 averaging is correct for typical sources. For this plugin, the hard sum is the user's explicit choice. Trust the input is phase-coherent below crossover.
**Warning signs:** Bass level drops dramatically when summed to mono

## Code Examples

Verified patterns from official sources and existing codebase:

### LinkwitzRileyFilter Setup and Processing
```cpp
// Source: JUCE dsp::LinkwitzRileyFilter documentation
// Initialize in prepareToPlay
juce::dsp::LinkwitzRileyFilter<float> lrFilter;
lrFilter.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
lrFilter.setCutoffFrequency(100.0f);  // Hz
lrFilter.prepare(spec);

// Process and get both bands (crossover usage)
void processAsCrossover(int channel, float input, float& low, float& high) {
    lrFilter.processSample(channel, input, low, high);
    // low + high = allpass (flat response)
}
```

### FIR Coefficient Generation for Linear-Phase Lowpass
```cpp
// Source: Standard windowed-sinc FIR design
std::vector<float> generateLinearPhaseLowpass(float cutoffHz, double sampleRate, int numTaps) {
    std::vector<float> coeffs(numTaps);
    float fc = cutoffHz / sampleRate;  // Normalized cutoff
    int M = numTaps - 1;

    for (int n = 0; n <= M; ++n) {
        if (n == M / 2) {
            coeffs[n] = 2.0f * fc;
        } else {
            float x = n - M / 2.0f;
            coeffs[n] = std::sin(2.0f * MathConstants<float>::pi * fc * x) / (MathConstants<float>::pi * x);
        }
        // Apply Blackman window for good stopband attenuation
        float window = 0.42f - 0.5f * std::cos(2.0f * MathConstants<float>::pi * n / M)
                            + 0.08f * std::cos(4.0f * MathConstants<float>::pi * n / M);
        coeffs[n] *= window;
    }

    // Normalize
    float sum = std::accumulate(coeffs.begin(), coeffs.end(), 0.0f);
    for (auto& c : coeffs) c /= sum;

    return coeffs;
}
```

### Convolution Setup with Generated IR
```cpp
// Source: JUCE dsp::Convolution documentation pattern
juce::dsp::Convolution convolver;

void setupFirCrossover(float cutoffHz, double sampleRate) {
    auto coeffs = generateLinearPhaseLowpass(cutoffHz, sampleRate, numTaps);

    // Create AudioBuffer from coefficients
    juce::AudioBuffer<float> irBuffer(1, numTaps);
    irBuffer.copyFrom(0, 0, coeffs.data(), numTaps);

    // Load into convolver (uses zero-latency mode by default)
    convolver.loadImpulseResponse(std::move(irBuffer), sampleRate,
                                   juce::dsp::Convolution::Stereo::no,
                                   juce::dsp::Convolution::Trim::no,
                                   juce::dsp::Convolution::Normalise::no);
}
```

### Mono Summing
```cpp
// Source: Existing codebase pattern
void sumToMono(juce::AudioBuffer<float>& stereoBuffer, juce::AudioBuffer<float>& monoBuffer) {
    const int numSamples = stereoBuffer.getNumSamples();
    auto* left = stereoBuffer.getReadPointer(0);
    auto* right = stereoBuffer.getReadPointer(1);
    auto* mono = monoBuffer.getWritePointer(0);

    for (int i = 0; i < numSamples; ++i) {
        mono[i] = (left[i] + right[i]) * 0.5f;
    }
}
```

### Real-Time Safe Parameter Access
```cpp
// Source: Existing OuariconComp pattern in codebase
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;

    // Atomic parameter reads - real-time safe
    float crossoverFreq = parameters.getRawParameterValue("crossover_freq")->load();
    bool bypassEnabled = parameters.getRawParameterValue("bypass")->load() > 0.5f;

    // No allocations, no locks, no unbounded operations from here
    // ...
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Direct IIR coefficient updates | SmoothedValue for parameters | JUCE 5.x | Eliminates zipper noise |
| Time-domain FIR convolution | FFT-based Convolution | JUCE 5.x | Efficient for >64 tap filters |
| Manual crossover implementations | `dsp::LinkwitzRileyFilter` | JUCE 5.4 | Built-in LR4 with correct summing |
| Fixed latency reporting | Dynamic `setLatencySamples()` | JUCE 5.x | Per-mode latency reporting |

**Deprecated/outdated:**
- `IIRFilter` legacy class: Use `dsp::IIR::Filter` or `dsp::LinkwitzRileyFilter` instead
- Manual DSP chains: Use `dsp::ProcessorChain` for cleaner architecture (optional)

## Open Questions

Things that couldn't be fully resolved:

1. **Dynamic latency host compatibility**
   - What we know: `setLatencySamples()` can be called at runtime; Pro Tools handles it well
   - What's unclear: Logic and Live may not respond to runtime latency changes
   - Recommendation: Test in target DAWs; consider always reporting max latency as fallback option

2. **FIR tap count for 40Hz at high sample rates**
   - What we know: Formula suggests ~4000-8000 taps for 40Hz crossover at 96kHz
   - What's unclear: Exact quality/latency tradeoff acceptable for "high-fidelity" mode
   - Recommendation: Start with 4096 taps (85ms latency at 48kHz), tune based on listening tests

3. **Crossfade strategy on mode switch**
   - What we know: Context decision allows instant toggle with possible click
   - What's unclear: Whether a micro-crossfade (5-10ms) would be imperceptible
   - Recommendation: Implement instant toggle per decision; add crossfade later if clicks are problematic

## Sources

### Primary (HIGH confidence)
- [JUCE dsp::LinkwitzRileyFilter Documentation](https://docs.juce.com/master/classdsp_1_1LinkwitzRileyFilter.html) - API reference, LR4 slope confirmation
- [JUCE dsp::Convolution Documentation](https://docs.juce.com/master/classdsp_1_1Convolution.html) - Latency modes, processing patterns
- [JUCE dsp::Oversampling Documentation](https://docs.juce.com/master/classdsp_1_1Oversampling.html) - Latency calculation method
- Existing codebase (OuariconComp, OuariconAnalogEQ) - Verified APVTS patterns, processBlock structure

### Secondary (MEDIUM confidence)
- [JUCE Forum: Perfect Crossover Filters](https://forum.juce.com/t/perfect-crossover-filters/36125) - All-pass compensation discussion
- [JUCE Forum: Latency Reporting](https://forum.juce.com/t/how-to-report-plugin-latency/55869) - Host compatibility notes
- [Sound on Sound: Making Low Frequencies Mono](https://www.soundonsound.com/sound-advice/q-how-do-you-make-only-low-frequencies-mono) - Mono summing approaches
- [dspGuru FIR Properties](https://dspguru.com/dsp/faqs/fir/properties/) - FIR latency calculation

### Tertiary (LOW confidence)
- [KVR Forum discussions](https://www.kvraudio.com/forum/) - Community experiences with dynamic latency
- [Linea Research LIR White Paper](https://linea-research.co.uk/wp-content/uploads/LR%20Download%20Assets/Tech%20Docs/LIR_LinearPhaseCrossovers.pdf) - Linear phase crossover theory

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - JUCE 8.0.9 in use, all components verified in documentation
- Architecture: HIGH - Patterns match existing codebase and JUCE best practices
- Pitfalls: HIGH - Well-documented in JUCE forums and DSP literature
- FIR design specifics: MEDIUM - Theory is solid, exact tap count needs tuning

**Research date:** 2026-01-22
**Valid until:** 2026-03-22 (JUCE stable, DSP theory doesn't change)
