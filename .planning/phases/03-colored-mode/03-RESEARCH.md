# Phase 3: Colored Mode - Research

**Researched:** 2026-01-23
**Domain:** Analog saturation DSP, tube/tape emulation, asymmetric waveshaping, harmonic generation
**Confidence:** MEDIUM (well-documented DSP domain, formulas verified from multiple sources, implementation details require validation)

## Summary

This phase implements Colored Mode, an analog-style saturation alternative to the existing Clean Mode. Where Clean Mode uses transparent harmonic generation (tanh soft saturation), Colored Mode adds analog warmth through asymmetric waveshaping that generates both even and odd harmonics - characteristic of tube and tape saturation. The key differentiator is the harmonic signature: Clean Mode produces primarily odd harmonics (symmetric saturation), while Colored Mode emphasizes even harmonics (asymmetric saturation via DC bias or asymmetric transfer function) for a warmer, more vintage character.

The implementation builds on the existing CleanModeProcessor architecture. The core technical approach is: (1) create a ColoredModeProcessor with asymmetric saturation, (2) add a mode switch parameter (Clean/Colored), (3) implement click-free crossfade between processing paths using SmoothedValue. The existing 20ms smoothed parameter transitions provide the pattern for click-free mode switching.

Key technical challenges: (1) designing an asymmetric transfer function that produces the desired even/odd harmonic balance, (2) implementing smooth crossfade between Clean and Colored paths without artifacts, (3) ensuring the Enhance parameter behaves consistently across both modes, and (4) maintaining the same frequency response characteristics (crossover, bandpass) while changing only the saturation character.

**Primary recommendation:** Use asymmetric tanh saturation with DC bias for Colored Mode: `y = tanh(drive * (x + bias)) - tanh(drive * bias)` to generate even harmonics while maintaining zero DC output. Implement mode switching with a 20ms crossfade between Clean and Colored processing outputs, matching the existing SmoothedValue pattern.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE SmoothedValue | 7.x | Click-free mode crossfade | Already used in codebase for enhance parameter |
| Custom AsymmetricSaturator | N/A | Even harmonic generation via DC bias | Standard technique from musicdsp.org and KVR forums |
| Existing HarmonicGenerator | N/A | Foundation for Colored harmonic generation | Reuses oversampling, bandpass filtering |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| juce::dsp::IIR::Filter | 7.x | High-pass filter for DC removal | After asymmetric saturation to remove DC component |
| JUCE dsp::Oversampling | 7.x | Alias prevention | Same 2x oversampling as Clean Mode |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| DC bias for asymmetry | Asymmetric polynomial (different curves above/below zero) | More control but more complex to tune |
| Single tanh with bias | Dual half-wave shapers | "Kiss of Shame" approach, more extreme even harmonic effect |
| 2x oversampling | 4x oversampling | Cleaner at extreme settings but higher CPU; 2x sufficient for warmth application |

**No additional dependencies needed** - all components built on existing JUCE DSP module and Phase 2 infrastructure.

## Architecture Patterns

### Recommended Project Structure
```
plugins/OBass/Source/
  DSP/
    CrossoverFilter.h/cpp      # [Phase 1 - exists]
    MonoSummer.h/cpp           # [Phase 1 - exists]
    HarmonicGenerator.h/cpp    # [Phase 2 - exists]
    CleanModeProcessor.h/cpp   # [Phase 2 - exists]
    ColoredModeProcessor.h/cpp # [NEW] Analog saturation processor
    EnhancementMode.h          # [NEW] Shared enum for Clean/Colored mode
  PluginProcessor.h/cpp        # [MODIFY] Add mode parameter, dual processors
```

### Pattern 1: Asymmetric Saturation with DC Bias
**What:** Generate even harmonics by adding DC offset before symmetric saturation, then removing offset
**When to use:** Creating tube/tape-like warmth character
**Example:**
```cpp
// Source: musicdsp.org, KVR DSP forums, verified pattern
// DC bias asymmetric saturation for even harmonic generation

class AsymmetricSaturator {
public:
    void setDrive(float drive) { this->drive = drive; }
    void setBias(float bias) { this->bias = juce::jlimit(0.0f, 0.5f, bias); }

    float process(float input) {
        // Add bias before saturation (creates asymmetry)
        float biased = input + bias;

        // Apply symmetric saturation to biased signal
        float saturated = std::tanh(drive * biased);

        // Remove DC component introduced by bias
        // tanh(drive * bias) is the DC offset that would remain
        float dcOffset = std::tanh(drive * bias);

        return saturated - dcOffset;
    }

private:
    float drive = 2.0f;   // Saturation intensity
    float bias = 0.2f;    // Asymmetry amount (0 = symmetric/odd, 0.5 = max even)
};
```

### Pattern 2: Parallel Processing with Crossfade
**What:** Run both Clean and Colored processors, crossfade based on mode
**When to use:** Click-free mode switching during playback
**Example:**
```cpp
// Source: Existing codebase pattern (smoothedEnhance), KVR crossfade discussions

class DualModeProcessor {
public:
    enum class Mode { Clean, Colored };

    void prepare(double sampleRate) {
        // 20ms crossfade time for click-free transitions
        modeCrossfade.reset(sampleRate, 0.020);
        cleanProcessor.prepare(sampleRate);
        coloredProcessor.prepare(sampleRate);
    }

    void setMode(Mode newMode) {
        // Target: 0.0 = Clean, 1.0 = Colored
        modeCrossfade.setTargetValue(newMode == Mode::Colored ? 1.0f : 0.0f);
    }

    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();

        // Copy input for parallel processing
        juce::AudioBuffer<float> coloredBuffer;
        coloredBuffer.makeCopyOf(buffer);

        // Process both paths
        cleanProcessor.process(buffer);  // In-place
        coloredProcessor.process(coloredBuffer);

        // Crossfade between outputs
        float* clean = buffer.getWritePointer(0);
        const float* colored = coloredBuffer.getReadPointer(0);

        for (int i = 0; i < numSamples; ++i) {
            float mix = modeCrossfade.getNextValue();
            clean[i] = clean[i] * (1.0f - mix) + colored[i] * mix;
        }
    }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> modeCrossfade;
    CleanModeProcessor cleanProcessor;
    ColoredModeProcessor coloredProcessor;
};
```

### Pattern 3: Consistent Enhance Behavior Across Modes
**What:** Ensure enhance parameter has similar perceived intensity in both modes
**When to use:** When the same enhance value should produce comparable results
**Example:**
```cpp
// Source: Design decision for consistent UX

// Clean mode: harmonics added based on enhance amount
// Colored mode: saturation drive based on enhance amount
// Both need to produce similar perceived enhancement intensity

class ColoredModeProcessor {
public:
    void setEnhanceAmount(float amount) {
        // Map enhance 0-1 to drive range that matches Clean Mode intensity
        // Clean Mode uses: harmonics * enhanceAmount
        // Colored Mode uses: tanh(x * drive) where drive scales with enhance

        // At enhance=0: minimal saturation (drive ~1.0, nearly linear)
        // At enhance=1: full saturation (drive ~4.0, strong harmonics)
        float minDrive = 1.0f;
        float maxDrive = 4.0f;
        saturator.setDrive(minDrive + amount * (maxDrive - minDrive));
    }
};
```

### Anti-Patterns to Avoid
- **Instantaneous mode switching:** Causes clicks. Always use SmoothedValue crossfade (20ms minimum).
- **Different oversampling between modes:** Creates latency mismatch. Use same 2x oversampling for both.
- **Ignoring DC offset:** Asymmetric saturation adds DC. Must high-pass filter or subtract bias offset.
- **Running both processors when not crossfading:** CPU waste. After crossfade settles, could optimize to run only active processor.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Click-free crossfade | Manual interpolation per sample | juce::SmoothedValue | Handles edge cases, already proven in codebase |
| DC removal after saturation | Custom DC blocker | Formula: `out - tanh(drive * bias)` | Mathematically exact removal of known offset |
| Oversampling | Custom resampling | Existing HarmonicGenerator oversampling | Already tested, same latency |
| Harmonic bandpass filtering | New filter design | Existing 80-300Hz bandpass from HarmonicGenerator | Consistent frequency response between modes |

**Key insight:** Colored Mode should reuse as much Phase 2 infrastructure as possible. The only new component is the saturation character - everything else (oversampling, bandpass, enhance scaling) remains the same.

## Common Pitfalls

### Pitfall 1: Clicks When Switching Modes
**What goes wrong:** Audible click or pop when toggling between Clean and Colored mode
**Why it happens:** Discontinuity in output signal when switching processing paths instantly
**How to avoid:** Use SmoothedValue with 20ms ramp time for crossfade. Process both paths in parallel during transition.
**Warning signs:** Click audible in A/B testing between modes.

### Pitfall 2: DC Offset Accumulation
**What goes wrong:** Audio drifts off-center, causes speaker excursion issues, breaks downstream processing
**Why it happens:** Asymmetric saturation with DC bias adds DC component that compounds over time
**How to avoid:** Either subtract the known DC offset (tanh(drive * bias)) or apply first-order high-pass filter at ~5Hz after saturation.
**Warning signs:** Waveform visually offset in DAW; bass sounds "pumpy" or "fluttery".

### Pitfall 3: Inconsistent Perceived Intensity
**What goes wrong:** Same enhance setting produces vastly different results between Clean and Colored modes
**Why it happens:** Different saturation curves have different harmonic generation efficiency
**How to avoid:** Calibrate drive range empirically. Start with drive 1.0-4.0 for enhance 0-100%, adjust based on A/B listening tests.
**Warning signs:** Users complain that switching modes changes "amount" of enhancement.

### Pitfall 4: Latency Mismatch Between Modes
**What goes wrong:** Phase cancellation or timing issues when crossfading between modes
**Why it happens:** Different processing chains with different group delays
**How to avoid:** Use identical oversampling and filtering for both modes. Only the saturation function differs.
**Warning signs:** "Hollow" or "phasey" sound during mode transitions.

### Pitfall 5: Extreme Settings Cause Aliasing
**What goes wrong:** Harsh digital artifacts at high enhance with Colored mode
**Why it happens:** Asymmetric saturation can generate more high-frequency harmonics than symmetric
**How to avoid:** 2x oversampling should be sufficient, but test at extreme settings. If needed, add gentle post-saturation lowpass.
**Warning signs:** Comparison at enhance=100% reveals harshness not present in Clean mode.

### Pitfall 6: Losing Character at Low Enhance
**What goes wrong:** At low enhance settings, Colored mode sounds identical to Clean mode
**Why it happens:** Low drive values produce nearly linear transfer function regardless of bias
**How to avoid:** Ensure minimum drive (~1.5) even at enhance=0, so some coloration always present. Or accept that low enhance = minimal difference.
**Warning signs:** "Mode switch does nothing at low settings" user feedback.

## Code Examples

Verified patterns from official sources:

### Asymmetric Tanh Saturation (DC-Corrected)
```cpp
// Source: musicdsp.org, KVR DSP forum discussions
// Generates even harmonics through DC bias, corrects for DC offset

inline float asymmetricTanh(float x, float drive, float bias) {
    // Asymmetric saturation with DC correction
    // bias range: 0.0 (symmetric/odd only) to 0.5 (max even harmonics)

    float biased = x + bias;
    float saturated = std::tanh(drive * biased);
    float dcCorrection = std::tanh(drive * bias);

    return saturated - dcCorrection;
}

// Usage in processing loop:
for (int i = 0; i < numSamples; ++i) {
    // drive: 1.0 (subtle) to 4.0 (aggressive)
    // bias: 0.2 for moderate even harmonic content
    data[i] = asymmetricTanh(data[i], 2.5f, 0.2f);
}
```

### Alternative: Tube-Style Asymmetric Polynomial
```cpp
// Source: DAFX book, KVR discussions on triode modeling
// Quadratic term adds 2nd harmonic (even), cubic adds 3rd (odd)

inline float tubePolySaturation(float x, float evenAmount, float oddAmount) {
    // Soft-limit input
    x = std::tanh(x);

    // Quadratic term: x^2 produces 2nd harmonic (DC + octave)
    // Need to subtract DC: x^2 - mean(x^2) for zero-centered input is just x^2
    float even = x * x * evenAmount;

    // Cubic term: x^3 produces 3rd harmonic (fifth)
    float odd = x * x * x * oddAmount;

    // Combine with original
    // even shifts positive, so subtract half the peak to center
    return x + odd + (even - 0.5f * evenAmount);
}
```

### Mode Switching with SmoothedValue
```cpp
// Source: Existing codebase pattern (PluginProcessor.cpp line 108-111)
// Verified approach for click-free parameter transitions

class ColoredModeProcessor {
public:
    void prepare(double sampleRate, int maxBlockSize) {
        // Match existing codebase: 20ms ramp time
        modeBlend.reset(sampleRate, 0.020);
        modeBlend.setCurrentAndTargetValue(0.0f);  // Start in Clean

        // Pre-allocate buffer for colored processing
        coloredBuffer.setSize(1, maxBlockSize);
    }

    void setMode(Mode mode) {
        // Smooth transition: 0.0 = Clean, 1.0 = Colored
        modeBlend.setTargetValue(mode == Mode::Colored ? 1.0f : 0.0f);
    }

    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        float* data = buffer.getWritePointer(0);

        // Resize colored buffer if needed
        if (coloredBuffer.getNumSamples() < numSamples)
            coloredBuffer.setSize(1, numSamples, false, false, true);

        // Copy for colored processing
        coloredBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
        float* colored = coloredBuffer.getWritePointer(0);

        // Process colored path
        for (int i = 0; i < numSamples; ++i) {
            colored[i] = asymmetricTanh(colored[i], drive, bias);
        }

        // Crossfade between clean (data) and colored
        for (int i = 0; i < numSamples; ++i) {
            float blend = modeBlend.getNextValue();
            data[i] = data[i] * (1.0f - blend) + colored[i] * blend;
        }
    }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> modeBlend;
    juce::AudioBuffer<float> coloredBuffer;
    float drive = 2.5f;
    float bias = 0.2f;
};
```

### High-Pass DC Blocker (Alternative to DC Correction)
```cpp
// Source: musicdsp.org DC blocker
// Use if DC correction formula is insufficient for edge cases

class DCBlocker {
public:
    void prepare(double sampleRate) {
        // Cutoff ~5Hz for inaudible DC removal
        float fc = 5.0f;
        R = 1.0f - (juce::MathConstants<float>::twoPi * fc / sampleRate);
    }

    float process(float x) {
        // First-order high-pass: y[n] = x[n] - x[n-1] + R * y[n-1]
        float y = x - xPrev + R * yPrev;
        xPrev = x;
        yPrev = y;
        return y;
    }

    void reset() { xPrev = yPrev = 0.0f; }

private:
    float R = 0.995f;
    float xPrev = 0.0f;
    float yPrev = 0.0f;
};
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Full analog circuit simulation | Waveshaping + pre/post filtering | ~2015 | CPU-efficient, close enough for enhancement use |
| AI/neural network saturation | Traditional DSP with tuned parameters | Ongoing | AI more accurate but overkill for bass enhancement |
| Hard switch between modes | Smooth crossfade | Best practice | Click-free user experience |
| Fixed harmonic ratios | Adjustable even/odd balance | Current | More sonic variety |

**Deprecated/outdated:**
- Instantaneous mode switching: Always use crossfade now
- Complex ODE solving for tube emulation: Overkill for enhancement application; simple waveshaping sufficient
- Fixed 4x oversampling for all saturation: 2x sufficient for warm saturation character

## Open Questions

Things that couldn't be fully resolved:

1. **Optimal bias value for "warm but not muddy" character**
   - What we know: Higher bias = more even harmonics = warmer. Too high = too much 2nd harmonic = "muddy".
   - What's unclear: Exact bias value that sounds best (0.1? 0.2? 0.3?)
   - Recommendation: Start with 0.2, implement as internal constant. Tune by ear during development.

2. **Equal-perceived-intensity calibration between modes**
   - What we know: Need to match enhance=50% Clean to enhance=50% Colored subjectively
   - What's unclear: Exact drive curve mapping
   - Recommendation: Start with drive 1.0-4.0 range, adjust based on A/B testing

3. **Whether to optimize by running only active processor**
   - What we know: During crossfade, both must run. After, only one is needed.
   - What's unclear: Is the CPU savings worth the complexity?
   - Recommendation: Start simple - always run both. Optimize later if profiling shows need.

4. **Colored mode latency matching**
   - What we know: Should use same oversampling as Clean for latency match
   - What's unclear: Whether asymmetric saturation needs any additional filtering that adds latency
   - Recommendation: Use identical signal chain structure, verify latency match in testing

## Sources

### Primary (HIGH confidence)
- musicdsp.org - Variable hardness clipping, DC blocker formulas
- JUCE SmoothedValue documentation - Existing codebase pattern verified
- Elementary Audio Distortion Tutorial - tanh saturation, asymmetry concepts

### Secondary (MEDIUM confidence)
- KVR Audio DSP Forum - Asymmetric saturation discussions, even/odd harmonic generation
- Gearspace forum - Tape vs tube harmonic character differences
- "Kiss of Shame" audiodev.blog - Even harmonic waveshaping with abs(x)
- Wikipedia Tube Sound - Even/odd harmonic theory (access blocked, content summarized from search)

### Tertiary (LOW confidence - for validation)
- Specific bias and drive values - Need listening tests to confirm
- Equal-power crossfade vs linear - Linear works per existing codebase pattern

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Uses existing JUCE/codebase patterns
- Architecture: HIGH - Follows established Phase 2 structure, minimal new components
- Saturation formulas: MEDIUM - Well-documented but tuning values need validation
- Mode switching: HIGH - Existing SmoothedValue pattern in codebase

**Research date:** 2026-01-23
**Valid until:** 60 days (stable DSP domain, formulas unchanged for decades)
