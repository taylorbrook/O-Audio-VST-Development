# Phase 4: Controls & Refinement - Research

**Researched:** 2026-01-24
**Domain:** Parameter control, intensity tuning, soft limiting, frequency-dependent compensation
**Confidence:** HIGH (JUCE APIs verified, established DSP patterns, builds on existing codebase)

## Summary

Phase 4 addresses parameter control refinement and intensity tuning issues identified in Phase 3 verification. The core challenge is making bass enhancement more prominent at low crossover frequencies (40-80Hz) while maintaining musical behavior across the full parameter range. The phase implements the four user-facing controls (Frequency, Enhance, Output, Mode) with proper response curves, auto-limiting, and visual feedback.

The primary technical work involves: (1) frequency-dependent intensity compensation that boosts enhancement at lower crossover frequencies, (2) Output gain control with soft clipping at extreme values, (3) tuning both Clean and Colored modes to have comparable perceived intensity at 50% Enhance, and (4) implementing visual feedback for limiting activity. The existing SmoothedValue infrastructure handles click-free parameter transitions.

Key insight from Phase 3 verification: Colored mode's drive range (1.0-4.0) is too conservative, and both modes produce insufficient enhancement at low crossover frequencies. The fix is two-fold: increase Colored mode's drive/harmonic output, and implement automatic intensity scaling based on crossover frequency.

**Primary recommendation:** Implement frequency-dependent intensity scaling where lower crossover frequencies automatically apply more drive/harmonics. Use formula: `intensityScale = 1.0 + (1.0 - normalizedCrossover) * scaleFactor` where normalizedCrossover maps 40-200Hz to 0-1, giving ~2x boost at 40Hz vs 200Hz. Add Output gain parameter with tanh soft clipper at +15dB ceiling.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE SmoothedValue | 7.x | Click-free parameter transitions | Already used in codebase, handles linear/multiplicative smoothing |
| JUCE dsp::Gain | 7.x | Output level control with built-in ramping | setRampDurationSeconds() for smooth gain changes |
| JUCE Decibels | 7.x | dB/linear conversion | decibelsToGain, gainToDecibels utilities |
| JUCE NormalisableRange | 7.x | Parameter skew for frequency controls | 0.5 skew already used for crossover parameter |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| juce::AudioParameterFloat | 7.x | Float parameters with range | Frequency, Enhance, Output |
| juce::AudioParameterChoice | 7.x | Choice parameters | Mode toggle |
| std::tanh | standard | Soft clipping function | Output protection, saturation |
| std::atomic<float> | standard | Thread-safe meter values | Limiting indicator state |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| tanh soft clipper | JUCE dsp::WaveShaper | WaveShaper adds complexity; inline tanh is sufficient for output protection |
| Manual intensity scaling | dB-domain curve | Linear scaling simpler to implement and tune |
| SmoothedValue for crossfade | Linear interpolation | SmoothedValue handles edge cases, already established pattern |

**No additional dependencies** - all components use existing JUCE DSP module and standard library functions.

## Architecture Patterns

### Recommended Project Structure
```
plugins/OBass/Source/
  DSP/
    CrossoverFilter.h/cpp       # [exists] Crossover - may need bandpass extension
    CleanModeProcessor.h/cpp    # [MODIFY] Add frequency-dependent intensity scaling
    ColoredModeProcessor.h/cpp  # [MODIFY] Increase drive range, add 4th harmonic
    HarmonicGenerator.h/cpp     # [MODIFY] Extend bandpass lower to 40Hz
  PluginProcessor.h/cpp         # [MODIFY] Add Output param, intensity scaling, limiting indicator
```

### Pattern 1: Frequency-Dependent Intensity Scaling
**What:** Automatically boost enhancement at lower crossover frequencies
**When to use:** Always - this is the core fix for low-frequency intensity issues
**Example:**
```cpp
// Source: CONTEXT.md decision + Phase 3 verification feedback
// Lower crossover = harder to perceive enhancement = needs more boost

float calculateIntensityScale(float crossoverHz) {
    // Normalize crossover: 40Hz -> 0.0, 200Hz -> 1.0
    float normalized = (crossoverHz - 40.0f) / (200.0f - 40.0f);
    normalized = juce::jlimit(0.0f, 1.0f, normalized);

    // Scale factor: 2.0 at 40Hz, 1.0 at 200Hz (linear interpolation)
    // This gives ~2x more enhancement at lowest frequency
    constexpr float maxBoost = 2.0f;
    return 1.0f + (1.0f - normalized) * (maxBoost - 1.0f);
}

// Usage in CleanModeProcessor::process():
float intensityScale = calculateIntensityScale(crossoverFreq);
float scaledEnhance = enhanceAmount * intensityScale;
// Apply scaledEnhance to harmonic generation
```

### Pattern 2: Output Gain with Soft Clipping
**What:** User-adjustable output level with protection against digital clipping
**When to use:** Output stage - after enhancement, before final output
**Example:**
```cpp
// Source: CONTEXT.md decision (soft clipper at +15-18dB)
// tanh soft clipper provides smooth limiting

class OutputStage {
public:
    void setGainDB(float db) {
        // Clamp to parameter range: -18dB to +18dB
        db = juce::jlimit(-18.0f, 18.0f, db);
        targetGainLinear = juce::Decibels::decibelsToGain(db);
        gainSmooth.setTargetValue(targetGainLinear);
    }

    void prepare(double sampleRate) {
        gainSmooth.reset(sampleRate, 0.020);  // 20ms ramp
    }

    float process(float input) {
        float gained = input * gainSmooth.getNextValue();

        // Soft clip at ~0dBFS (-0.1dB for safety margin)
        // tanh(x) approaches +/-1 asymptotically
        // Scale factor of 0.9 gives soft knee starting around -3dB
        constexpr float ceiling = 0.9f;  // ~-0.9dBFS
        if (std::abs(gained) > ceiling) {
            // Soft clip only the portion above ceiling
            float sign = (gained > 0.0f) ? 1.0f : -1.0f;
            float excess = std::abs(gained) - ceiling;
            float softClipped = ceiling + std::tanh(excess * 2.0f) * (1.0f - ceiling);
            return sign * softClipped;
        }
        return gained;
    }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> gainSmooth;
    float targetGainLinear = 1.0f;
};
```

### Pattern 3: Colored Mode Intensity Fix
**What:** Increase Colored mode drive and add 4th harmonic for warmth
**When to use:** Colored mode processing
**Example:**
```cpp
// Source: Phase 3 verification - Colored mode "more subtle than Clean"
// Fix: Increase drive range from 1.0-4.0 to 2.0-6.0
// Add full even spectrum: 2nd + 4th harmonics

void ColoredModeProcessor::setEnhanceAmount(float amount) {
    enhanceAmount = juce::jlimit(0.0f, 1.0f, amount);

    // UPDATED: Drive range 2.0-6.0 (was 1.0-4.0)
    // Higher minimum ensures Colored mode always has character
    drive = 2.0f + enhanceAmount * 4.0f;
}

float ColoredModeProcessor::asymmetricTanhWithEvenHarmonics(float x) {
    // Original asymmetric tanh with DC correction
    float biased = x + bias;  // bias = 0.3f (was 0.2f) for stronger even harmonics
    float saturated = std::tanh(drive * biased);
    float dcCorrection = std::tanh(drive * bias);
    float processed = saturated - dcCorrection;

    // Add 4th harmonic component for richer even spectrum
    // T4(x) = 8x^4 - 8x^2 + 1 produces 4th harmonic
    float x_limited = std::tanh(x);  // limit to [-1, 1]
    float x2 = x_limited * x_limited;
    float h4 = (8.0f * x2 * x2 - 8.0f * x2 + 1.0f) * 0.15f;  // 15% 4th harmonic

    return processed + h4 * enhanceAmount;
}
```

### Pattern 4: Auto-Limiting with Visual Feedback
**What:** Track limiting activity for UI display
**When to use:** When auto-limit ceiling is approached
**Example:**
```cpp
// Source: CONTEXT.md decision (visual indicator when near limit)

class AutoLimiter {
public:
    void prepare(double sampleRate) {
        // Attack: fast to catch peaks, Release: slow for smooth indication
        limitIndicatorSmooth.reset(sampleRate, 0.100);  // 100ms release
        limitIndicatorSmooth.setCurrentAndTargetValue(0.0f);
    }

    float process(float input, float ceiling = 0.8f) {
        // Apply ceiling limit
        float output = input;
        if (std::abs(input) > ceiling) {
            output = (input > 0.0f) ? ceiling : -ceiling;

            // Track how much we limited (0-1 scale)
            float limitAmount = (std::abs(input) - ceiling) / ceiling;
            limitIndicatorSmooth.setTargetValue(juce::jmin(1.0f, limitAmount));
        }
        else {
            // Decay indicator when not limiting
            if (limitIndicatorSmooth.getTargetValue() > 0.0f)
                limitIndicatorSmooth.setTargetValue(0.0f);
        }

        return output;
    }

    float getLimitIndicator() {
        return limitIndicatorSmooth.getNextValue();
    }

    // Atomic for thread-safe UI access
    void updateMeterValue() {
        limitMeterAtomic.store(limitIndicatorSmooth.getCurrentValue());
    }

    float getLimitMeter() const {
        return limitMeterAtomic.load();
    }

private:
    juce::SmoothedValue<float> limitIndicatorSmooth;
    std::atomic<float> limitMeterAtomic { 0.0f };
};
```

### Pattern 5: Bandpass Extension to 40Hz
**What:** Lower the harmonic bandpass from 60Hz to 40Hz
**When to use:** HarmonicGenerator output filtering
**Example:**
```cpp
// Source: CONTEXT.md decision (extend bandpass to 40Hz)
// Allows harmonics closer to fundamental for deep bass enhancement

void HarmonicGenerator::prepare(const juce::dsp::ProcessSpec& spec) {
    // ... existing setup ...

    // UPDATED: Lower cutoff from 60Hz to 40Hz
    // This allows 2nd harmonic of 20Hz fundamental to pass
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
        sampleRate, 40.0f, 0.707f);  // was 60.0f
    outputBandpassLow.coefficients = hpCoeffs;

    // Upper cutoff remains at 300-400Hz
    auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sampleRate, 400.0f, 0.707f);  // can increase if needed
    outputBandpassHigh.coefficients = lpCoeffs;
}
```

### Anti-Patterns to Avoid
- **Applying intensity scaling after soft limit:** Scale intensity BEFORE limiting, not after. Post-limit scaling defeats the purpose.
- **Using linear smoothing for gain changes:** Use multiplicative smoothing (SmoothedValue::Multiplicative) for dB-scale gain to avoid perceptual discontinuities.
- **Forgetting to update meter atomics:** Limit indicator must use atomic for thread-safe UI access. Don't call getNextValue() from both audio and UI threads.
- **Hard-coding intensity scale factors:** Make them tunable constants at class level for easier adjustment during testing.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| dB/linear conversion | Manual math | juce::Decibels | Handles edge cases, standard formula |
| Parameter ramping | Manual interpolation | juce::SmoothedValue with Multiplicative | Correct perceptual curve for gain |
| Gain application | Per-sample multiplication | juce::dsp::Gain | Built-in smoothing, prepared for block processing |
| Thread-safe metering | Mutexes | std::atomic<float> | Lock-free, sufficient for single float values |
| Range normalization | Manual scaling | NormalisableRange::convertTo0to1() | Handles skew correctly |

**Key insight:** Parameter control is a solved problem in JUCE. Use the established classes to avoid subtle bugs with smoothing, thread safety, and perceptual curves.

## Common Pitfalls

### Pitfall 1: Intensity Scaling Applied After Limiting
**What goes wrong:** Enhancement sounds equally weak at all crossover frequencies despite scaling
**Why it happens:** If you scale after the limiter, the limiter undoes the boost
**How to avoid:** Apply intensity scaling to enhance amount BEFORE it enters the processing chain. Scale the "drive" or "harmonic mix" parameters, not the output.
**Warning signs:** A/B testing 40Hz vs 200Hz crossover sounds identical in intensity.

### Pitfall 2: Clicks on Output Gain Changes
**What goes wrong:** Audible clicks when adjusting Output knob
**Why it happens:** Gain changes applied instantaneously without smoothing
**How to avoid:** Use SmoothedValue with 20ms ramp time. For multiplicative gain (dB scale), use ValueSmoothingTypes::Multiplicative.
**Warning signs:** Fast knob automation causes crackling.

### Pitfall 3: Colored Mode Still Subtle After Drive Increase
**What goes wrong:** Colored mode sounds comparable but still lacking warmth
**Why it happens:** Drive alone doesn't guarantee even harmonics; bias must also be tuned
**How to avoid:** Increase both drive range AND bias value. Test with sine wave to verify 2nd harmonic presence. Consider adding explicit 4th harmonic via Chebyshev T4.
**Warning signs:** Spectrum analyzer shows Colored mode missing strong 2nd harmonic at octave frequency.

### Pitfall 4: Limit Indicator Stuck On
**What goes wrong:** Limit indicator shows limiting even when enhance is low
**Why it happens:** Indicator release time too slow, or threshold too conservative
**How to avoid:** Use ~100ms release time for indicator smoothing. Ensure limiting only activates above -2dB ceiling (0.8 linear).
**Warning signs:** Indicator always glowing even at enhance=10%.

### Pitfall 5: Frequency-Dependent Boost Too Aggressive
**What goes wrong:** 40Hz crossover sounds boomy or distorted while 200Hz is fine
**Why it happens:** Linear scaling of 2x may be too much for very low frequencies
**How to avoid:** Start with 1.5x max boost, increase if needed. Consider sqrt() curve for gentler scaling at extremes.
**Warning signs:** Human testing reports "boomy" or "muddy" at low crossover with moderate enhance.

### Pitfall 6: Output Soft Clipper Introduces Aliasing
**What goes wrong:** Harsh artifacts at extreme positive gain with loud input
**Why it happens:** Soft clipping generates harmonics; at high gain, these may alias
**How to avoid:** Apply soft clipper at reasonable threshold (~0.9). For extreme cases, consider 2x oversampling on output stage. In practice, tanh soft clip at output is mild enough to not need oversampling.
**Warning signs:** A/B testing at +18dB gain shows "digital" quality vs clean bypass.

## Code Examples

Verified patterns from official sources:

### Complete Output Parameter Setup
```cpp
// Source: JUCE AudioParameterFloat docs, existing codebase pattern
// Output parameter with 0dB center, +/-18dB range

// In createParameterLayout():
layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "output", 1 },
    "Output",
    juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f),
    0.0f,  // Default 0dB (unity)
    juce::AudioParameterFloatAttributes()
        .withLabel("dB")
));

// In prepareToPlay():
outputGainSmooth.reset(sampleRate, 0.020);  // 20ms ramp
auto* outputParam = parameters.getRawParameterValue("output");
float initialGainLinear = juce::Decibels::decibelsToGain(outputParam->load());
outputGainSmooth.setCurrentAndTargetValue(initialGainLinear);

// In processBlock():
auto* outputParam = parameters.getRawParameterValue("output");
float targetGainLinear = juce::Decibels::decibelsToGain(outputParam->load());
outputGainSmooth.setTargetValue(targetGainLinear);

// Apply gain per-sample (after enhancement, before output)
for (int i = 0; i < numSamples; ++i) {
    float gain = outputGainSmooth.getNextValue();
    for (int ch = 0; ch < numChannels; ++ch) {
        float sample = buffer.getSample(ch, i) * gain;
        // Soft clip at output if needed
        if (std::abs(sample) > 0.95f) {
            float sign = (sample > 0.0f) ? 1.0f : -1.0f;
            sample = sign * (0.95f + std::tanh((std::abs(sample) - 0.95f) * 10.0f) * 0.05f);
        }
        buffer.setSample(ch, i, sample);
    }
}
```

### Frequency-to-Intensity Mapping with Curve
```cpp
// Source: Phase 3 verification feedback, psychoacoustic bass compensation
// Non-linear curve gives gentler boost at very low frequencies

float calculateIntensityScale(float crossoverHz) {
    // Normalize: 40Hz -> 0.0, 200Hz -> 1.0
    float normalized = juce::jlimit(0.0f, 1.0f,
        (crossoverHz - 40.0f) / 160.0f);

    // Sqrt curve: gentler at extremes
    // At 40Hz: sqrt(1-0) = 1.0, scale = 1.0 + 0.7*1.0 = 1.7x
    // At 120Hz: sqrt(1-0.5) = 0.707, scale = 1.0 + 0.7*0.707 = 1.49x
    // At 200Hz: sqrt(1-1) = 0.0, scale = 1.0x
    float boostFactor = std::sqrt(1.0f - normalized);

    constexpr float maxBoostMultiplier = 0.7f;  // 70% max boost
    return 1.0f + boostFactor * maxBoostMultiplier;
}
```

### Atomic Limiting Indicator for UI
```cpp
// Source: JUCE atomic patterns, existing metering code in codebase

// In PluginProcessor.h:
private:
    std::atomic<float> limitIndicator { 0.0f };
    juce::SmoothedValue<float> limitIndicatorSmooth;

public:
    float getLimitIndicator() const { return limitIndicator.load(); }

// In prepareToPlay():
limitIndicatorSmooth.reset(sampleRate, 0.100);  // 100ms decay
limitIndicatorSmooth.setCurrentAndTargetValue(0.0f);

// In processBlock() (after auto-limiting):
// ... limiting code that sets limitIndicatorSmooth.setTargetValue() ...

// Update atomic at end of block for UI
limitIndicator.store(limitIndicatorSmooth.getCurrentValue());

// In UI timer callback:
float indicatorValue = processor.getLimitIndicator();
// Map to color: 0 = green, 0.5 = yellow, 1.0 = red
// Or use as alpha for "glow" effect
```

### Enhanced Colored Mode with Full Even Spectrum
```cpp
// Source: Phase 3 research, Chebyshev polynomials for harmonic generation

void ColoredModeProcessor::process(juce::AudioBuffer<float>& monoBuffer) {
    const int numSamples = monoBuffer.getNumSamples();
    if (numSamples == 0 || enhanceAmount < 0.001f)
        return;

    float* data = monoBuffer.getWritePointer(0);

    // Precompute for efficiency
    float dcCorrection = std::tanh(drive * bias);

    for (int i = 0; i < numSamples; ++i) {
        float input = data[i];

        // Asymmetric saturation (generates 2nd harmonic primarily)
        float biased = input + bias;  // bias = 0.3f for stronger even
        float saturated = std::tanh(drive * biased);
        float processed = saturated - dcCorrection;

        // Add explicit 4th harmonic for richer even spectrum
        // T4(x) = 8x^4 - 8x^2 + 1
        float x = std::tanh(input * 0.5f);  // normalize to ~[-1,1]
        float x2 = x * x;
        float h4 = (8.0f * x2 * x2 - 8.0f * x2 + 1.0f) * 0.15f * enhanceAmount;

        processed += h4;

        // Mix with dry based on enhance
        float output = input * (1.0f - enhanceAmount) + processed * enhanceAmount;

        // Soft limit
        data[i] = std::tanh(output);
    }
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Fixed enhancement intensity | Frequency-adaptive scaling | Current best practice | Consistent perceived enhancement across crossover range |
| Hard limiting at 0dB | Soft limiting at -2dB | Current practice | Prevents harshness, allows headroom |
| Linear parameter curves | Sqrt/log curves for enhance | Current practice | Musical feel, prevents "jumpy" behavior at extremes |
| No limiting feedback | Visual indicator | Current UX trend | User knows when system is protecting their audio |

**Deprecated/outdated:**
- Hard clipping at output: Creates harsh artifacts. Use tanh soft clip.
- Fixed intensity regardless of frequency: Low crossover sounds weak. Always use frequency compensation.
- Separate limiter plugins: Modern enhancers include internal limiting as standard.

## Open Questions

Things that couldn't be fully resolved:

1. **Exact intensity scale factor for frequency compensation**
   - What we know: Low frequencies need more boost (1.5x-2x seems reasonable)
   - What's unclear: Exact multiplier depends on how enhancement is perceived (subjective)
   - Recommendation: Start with 1.7x at 40Hz, tune via listening tests

2. **Enhance=0% behavior: true bypass vs crossover-active (Claude's discretion)**
   - What we know: True bypass = zero CPU, crossover-active = consistent latency
   - What's unclear: User preference varies
   - Recommendation: **True bypass** - when enhance=0%, skip all processing (cleanModeProcessor and coloredModeProcessor), pass audio through unchanged. This maximizes CPU efficiency. Latency change on enhance toggle is acceptable since users don't typically automate enhance=0%.

3. **Output auto-gain approach (Claude's discretion)**
   - What we know: Enhancement adds perceived loudness; strict unity gain may underwhelm
   - What's unclear: Whether auto-gain, manual+trim, or switchable is best UX
   - Recommendation: **Pure manual** - Output knob is explicit control, no hidden auto-gain. Enhancement naturally adds energy; users can compensate with Output knob. Simpler is better for v1.0.

4. **Per-mode ceiling tuning (Claude's discretion)**
   - What we know: Colored mode has saturation; may need lower ceiling for headroom
   - What's unclear: Whether mode-specific ceilings improve sound or add complexity
   - Recommendation: **Same -2dB (0.8) ceiling for both modes**. Colored mode's saturation already soft-clips internally via tanh. Separate ceilings add complexity without clear benefit.

5. **Adaptive limiting based on crossover frequency (Claude's discretion)**
   - What we know: More boost at low frequencies means more limiting activity
   - What's unclear: Whether ceiling should scale with frequency
   - Recommendation: **No adaptive ceiling** - intensity scaling is applied to input, not ceiling. The ceiling stays at -2dB regardless. This keeps limiting behavior predictable.

## Sources

### Primary (HIGH confidence)
- [JUCE SmoothedValue Documentation](https://docs.juce.com/master/classSmoothedValue.html) - reset, setTargetValue, getNextValue, Multiplicative smoothing type
- [JUCE dsp::Gain Documentation](https://docs.juce.com/master/classdsp_1_1Gain.html) - setGainLinear, setGainDecibels, setRampDurationSeconds
- Existing OBass codebase - SmoothedValue pattern at 20ms ramp, atomic metering pattern

### Secondary (MEDIUM confidence)
- [KVR Audio DSP Forum - Soft Clipping](https://www.kvraudio.com/forum/viewtopic.php?t=122309) - tanh-based soft clipping formula
- [MATLAB Bass Enhancement](https://www.mathworks.com/help/audio/ug/psychoacoustic-bass-enhancement-for-band-limited-signals.html) - tunable gain for intensity control
- Phase 3 Verification Report - Specific intensity issues documented (Colored subtle, low crossover weak)
- Phase 3 Research - Chebyshev T4 polynomial for 4th harmonic

### Tertiary (LOW confidence - for validation)
- Frequency-dependent intensity scaling factor - Requires listening tests to confirm 1.7x multiplier
- Optimal bias value for Colored mode (0.3 vs 0.2) - Subjective, needs A/B testing

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - JUCE APIs verified, existing codebase patterns
- Architecture: HIGH - Builds on Phase 2/3 infrastructure, clear signal flow
- Intensity tuning: MEDIUM - Formulas established, specific values need validation
- Claude's discretion items: HIGH - Clear recommendations with rationale

**Research date:** 2026-01-24
**Valid until:** 60 days (stable domain, parameter control patterns well-established)
