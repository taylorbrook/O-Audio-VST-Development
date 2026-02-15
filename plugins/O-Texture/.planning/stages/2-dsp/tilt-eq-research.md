# Tilt EQ Research for O-Texture BRIGHTNESS Parameter

**Researched:** 2026-02-14
**Domain:** Audio DSP -- Tilt EQ / Spectral Balance Filter
**Confidence:** HIGH

## Summary

A tilt EQ (also called "tilt filter" or "niveau filter") provides a single-knob control over spectral balance by simultaneously boosting one end of the spectrum while cutting the other end, pivoting around a center frequency. This is the ideal topology for the O-Texture BRIGHTNESS parameter (-1.0 to +1.0), where positive values should brighten the texture and negative values should darken it.

Two viable implementation approaches were investigated: (1) a lightweight 1-pole lowpass/highpass mixing approach from musicdsp.org (modeled on the Elysia mPressor "Niveau" filter), and (2) a dual-shelf approach using JUCE's built-in `IIR::Coefficients::makeLowShelf` / `makeHighShelf`. The 1-pole approach is recommended as the primary implementation due to its minimal CPU cost, simplicity, and suitability for subtle tonal shaping in a texture synthesizer.

**Primary recommendation:** Use the 1-pole tilt filter from musicdsp.org, implemented as a standalone class with per-channel state and `SmoothedValue` for zipper-free parameter changes.

## Mathematical Basis

### 1-Pole Tilt Filter (Recommended)

The algorithm splits the signal into low-frequency and high-frequency components using a 1-pole lowpass filter, then applies opposing gain to each component.

**Core principle:**
```
lowpass_output = a0 * input + b1 * lowpass_state
highpass_output = input - lowpass_output
output = input + lgain * lowpass_output + hgain * highpass_output
```

Where:
- `a0` and `b1` are 1-pole lowpass coefficients for the center frequency
- `lgain` is the gain applied to the low-frequency component
- `hgain` is the gain applied to the high-frequency component
- When BRIGHTNESS > 0: hgain is positive (boost highs), lgain is negative (cut lows)
- When BRIGHTNESS < 0: lgain is positive (boost lows), hgain is negative (cut highs)

**Coefficient calculation:**
```
omega = 2 * pi * f0 / sampleRate
n = 1 / (sampleRate + omega)        // Note: uses sampleRate, not 3*sampleRate
a0 = 2 * omega * n
b1 = (sampleRate - omega) * n
```

The original musicdsp.org algorithm uses `3 * sampleRate` in the denominator, which shifts the actual -3dB point. Using just `sampleRate` places the turnover closer to f0. Either works -- the factor of 3 in the original gives a slightly wider transition band. For O-Texture, the original formula with `3 * sampleRate` is fine since we want a gentle, broad tilt.

**Gain calculation from the BRIGHTNESS parameter:**
```
amp = 6.0 / log(2.0)    // ~8.656

// Map brightness (-1..+1) to dB gain (-6..+6 dB)
gainDB = brightness * 6.0

if (gainDB > 0):
    g1 = -gfactor * gainDB    // low-frequency gain (negative = cut)
    g2 = gainDB                // high-frequency gain (positive = boost)
else:
    g1 = -gainDB               // low-frequency gain (positive = boost)
    g2 = gfactor * gainDB      // high-frequency gain (negative = cut)

lgain = exp(g1 / amp) - 1.0
hgain = exp(g2 / amp) - 1.0
```

The `gfactor` (default: 5) controls the asymmetry between boost and cut. With gfactor=5, a +6 dB high boost produces a -30 dB low cut (scaled through exp), meaning extreme settings approach a first-order highpass or lowpass. For O-Texture's subtle tonal shaping, a lower gfactor (e.g., 2-3) may be more appropriate, limiting the maximum effect.

**Frequency response characteristics:**
- At BRIGHTNESS = 0: perfectly flat (lgain = 0, hgain = 0)
- Slope: ~6 dB/octave maximum (first-order)
- Pivot frequency: f0 (unity gain at center frequency regardless of setting)
- Phase shift: minimal (single pole)

### Dual-Shelf Approach (Alternative)

Uses JUCE's `IIR::Coefficients::makeLowShelf` and `makeHighShelf` in series, with complementary gains:

```
Low shelf gain  = 1.0 / gainFactor    (reciprocal)
High shelf gain = gainFactor
```

This produces a steeper tilt (~12 dB/octave with two 2nd-order filters) and uses the standard Audio EQ Cookbook biquad formulas. The O-AnalogEQ plugin in this codebase already uses this pattern.

## Standard Stack

### Core (Already in Project)
| Library | Version | Purpose | Notes |
|---------|---------|---------|-------|
| JUCE juce_dsp | 8.0.4 | DSP module | Provides `IIR::Filter`, `ProcessSpec`, `AudioBlock`, `SmoothedValue` |
| JUCE juce_audio_basics | 8.0.4 | SmoothedValue | For zipper-free parameter interpolation |

### No Additional Dependencies Required

The tilt EQ is lightweight enough to implement with raw C++ and JUCE's `SmoothedValue`. No additional libraries are needed.

## Architecture Patterns

### Recommended: Standalone TiltFilter Class

```
plugins/O-Texture/Source/
  DSP/
    TiltFilter.h      # Self-contained tilt EQ class
```

The filter should be a self-contained class with:
- Per-channel state (lowpass filter state)
- `prepare(sampleRate)` method
- `reset()` method
- `setParameters(brightness, centerFreq)` method
- `processSample(channel, sample)` method for sample-by-sample processing
- `processBlock(AudioBuffer&)` method for block processing

### Integration Point

In `TextureProcessor::processBlock()`, after the overlap-add output is ready, apply the tilt filter as the final processing step (before output). This matches the CONTEXT.md description: "Post-processing: tilt EQ (BRIGHTNESS parameter)".

## Concrete C++ Implementation

### TiltFilter.h -- Complete Implementation

```cpp
#pragma once
#include <JuceHeader.h>

/**
 * Tilt EQ filter for spectral balance control.
 *
 * Based on the musicdsp.org "Simple Tilt Equalizer" algorithm
 * (modeled on Elysia mPressor "Niveau" filter).
 *
 * A single parameter tilts the spectrum around a center frequency:
 *   - Positive values boost highs and cut lows
 *   - Negative values boost lows and cut highs
 *   - Zero produces a flat response
 *
 * Uses a 1-pole lowpass to split signal into low/high components,
 * then applies opposing gains. Very CPU-efficient (~2 multiplies
 * + 2 adds per sample per channel).
 */
class TiltFilter
{
public:
    TiltFilter() = default;

    /**
     * Initialize the filter for a given sample rate.
     * Call this from prepareToPlay().
     */
    void prepare (double sampleRate, int numChannels = 2)
    {
        sr = sampleRate;
        lpState.resize (static_cast<size_t> (numChannels), 0.0f);

        // Initialize smoothed values (50ms ramp to avoid zipper noise)
        smoothedLGain.reset (sampleRate, 0.05);
        smoothedHGain.reset (sampleRate, 0.05);

        updateCoefficients();

        // Set initial gain values without ramping
        smoothedLGain.setCurrentAndTargetValue (0.0f);
        smoothedHGain.setCurrentAndTargetValue (0.0f);
    }

    /**
     * Reset filter state (clear delay elements).
     * Call this from prepareToPlay() or when starting fresh.
     */
    void reset()
    {
        std::fill (lpState.begin(), lpState.end(), 0.0f);
        smoothedLGain.setCurrentAndTargetValue (smoothedLGain.getTargetValue());
        smoothedHGain.setCurrentAndTargetValue (smoothedHGain.getTargetValue());
    }

    /**
     * Set the tilt amount.
     * @param brightness  Range: -1.0 to +1.0
     *                    Positive = brighter (boost highs, cut lows)
     *                    Negative = darker (boost lows, cut highs)
     *                    Zero = flat (no effect)
     */
    void setBrightness (float brightness)
    {
        brightness = juce::jlimit (-1.0f, 1.0f, brightness);

        if (juce::approximatelyEqual (brightness, currentBrightness))
            return;

        currentBrightness = brightness;

        // Map brightness to gain in dB, then to filter gains
        const float gainDB = brightness * maxGainDB;

        float g1, g2;

        if (gainDB > 0.0f)
        {
            g1 = -gfactor * gainDB;  // Cut lows
            g2 = gainDB;              // Boost highs
        }
        else
        {
            g1 = -gainDB;             // Boost lows
            g2 = gfactor * gainDB;    // Cut highs
        }

        // Convert from dB-like scale to linear gain offset
        // amp = 6 / ln(2) ~= 8.656
        constexpr float amp = 6.0f / 0.693147180559945f;

        const float newLGain = std::exp (g1 / amp) - 1.0f;
        const float newHGain = std::exp (g2 / amp) - 1.0f;

        smoothedLGain.setTargetValue (newLGain);
        smoothedHGain.setTargetValue (newHGain);
    }

    /**
     * Set the center (pivot) frequency.
     * @param freqHz  Center frequency in Hz (default: 800 Hz)
     */
    void setCenterFrequency (float freqHz)
    {
        centerFreq = juce::jlimit (20.0f, 20000.0f, freqHz);
        updateCoefficients();
    }

    /**
     * Process a single sample on a given channel.
     * Use this for sample-by-sample processing within an existing loop.
     */
    float processSample (int channel, float input) noexcept
    {
        jassert (channel >= 0 && channel < static_cast<int> (lpState.size()));

        const float lg = smoothedLGain.getNextValue();
        const float hg = smoothedHGain.getNextValue();

        // 1-pole lowpass
        float& state = lpState[static_cast<size_t> (channel)];
        const float lpOut = a0 * input + b1 * state;
        state = lpOut;

        // High = input - low
        const float hpOut = input - lpOut;

        // Combine: original + gain-weighted low + gain-weighted high
        return input + lg * lpOut + hg * hpOut;
    }

    /**
     * Process an entire audio buffer in-place.
     * Reads brightness from the SmoothedValue for zipper-free operation.
     */
    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // Skip processing if brightness is at zero and not smoothing
        if (juce::approximatelyEqual (currentBrightness, 0.0f)
            && ! smoothedLGain.isSmoothing()
            && ! smoothedHGain.isSmoothing())
            return;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Get smoothed gains (advance smoother once per sample, shared across channels)
            const float lg = smoothedLGain.getNextValue();
            const float hg = smoothedHGain.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* channelData = buffer.getWritePointer (ch);
                const float input = channelData[sample];

                // 1-pole lowpass
                float& state = lpState[static_cast<size_t> (ch)];
                const float lpOut = a0 * input + b1 * state;
                state = lpOut;

                // High = input - low
                const float hpOut = input - lpOut;

                // Combine
                channelData[sample] = input + lg * lpOut + hg * hpOut;
            }
        }
    }

private:
    void updateCoefficients()
    {
        if (sr <= 0.0)
            return;

        const float omega = juce::MathConstants<float>::twoPi * centerFreq;
        const float n = 1.0f / (3.0f * static_cast<float> (sr) + omega);
        a0 = 2.0f * omega * n;
        b1 = (3.0f * static_cast<float> (sr) - omega) * n;
    }

    // Filter coefficients
    float a0 = 0.0f;
    float b1 = 0.0f;

    // Per-channel lowpass state
    std::vector<float> lpState;

    // Parameter smoothing
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedLGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedHGain;

    // Configuration
    double sr = 48000.0;
    float centerFreq = 800.0f;    // Pivot frequency in Hz
    float currentBrightness = 0.0f;
    float maxGainDB = 6.0f;       // Maximum gain in dB at extreme settings
    float gfactor = 4.0f;         // Asymmetry factor (higher = more extreme)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TiltFilter)
};
```

### Integration in TextureProcessor

**In PluginProcessor.h -- add member:**
```cpp
#include "DSP/TiltFilter.h"

// ... inside class TextureProcessor:
private:
    TiltFilter tiltFilter;
```

**In PluginProcessor.cpp -- prepareToPlay:**
```cpp
void TextureProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    setLatencySamples (6144);

    // Initialize tilt filter
    tiltFilter.prepare (sampleRate, 2);  // Stereo
    tiltFilter.setCenterFrequency (800.0f);  // 800 Hz pivot
    tiltFilter.reset();
}
```

**In PluginProcessor.cpp -- processBlock (after overlap-add output):**
```cpp
void TextureProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // ... (inference, overlap-add, etc.) ...

    // Post-processing: Tilt EQ (BRIGHTNESS parameter)
    const float brightness = brightnessParam->load();
    tiltFilter.setBrightness (brightness);
    tiltFilter.processBlock (buffer);
}
```

## Alternative: Dual-Shelf JUCE Approach

If a steeper tilt slope is desired, use JUCE's built-in shelf filters (matching the O-AnalogEQ pattern):

```cpp
// In PluginProcessor.h:
using IIRFilter = juce::dsp::IIR::Filter<float>;
using IIRCoefficients = juce::dsp::IIR::Coefficients<float>;
using StereoFilter = juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients>;

StereoFilter lowShelf, highShelf;

// In prepareToPlay:
juce::dsp::ProcessSpec spec;
spec.sampleRate = sampleRate;
spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
spec.numChannels = 2;

lowShelf.prepare (spec);
highShelf.prepare (spec);
lowShelf.reset();
highShelf.reset();

// In processBlock:
const float brightness = brightnessParam->load();
const float gainDB = brightness * 6.0f;  // +/-6 dB range
const float gainFactor = juce::Decibels::decibelsToGain (gainDB);

// Low shelf: reciprocal gain (cuts when brightness boosts highs)
*lowShelf.state = *IIRCoefficients::makeLowShelf (
    currentSampleRate, 800.0f, 0.707f, 1.0f / gainFactor);

// High shelf: direct gain
*highShelf.state = *IIRCoefficients::makeHighShelf (
    currentSampleRate, 800.0f, 0.707f, gainFactor);

juce::dsp::AudioBlock<float> block (buffer);
juce::dsp::ProcessContextReplacing<float> context (block);
lowShelf.process (context);
highShelf.process (context);
```

**Tradeoffs vs. 1-pole approach:**

| Aspect | 1-Pole (Recommended) | Dual-Shelf |
|--------|----------------------|------------|
| CPU cost | ~2 mul + 2 add / sample | ~10 mul + 8 add / sample |
| Slope | ~6 dB/oct (gentle) | ~12 dB/oct (steeper) |
| Phase shift | Minimal | More (2nd order x2) |
| Code complexity | Self-contained class | Uses JUCE ProcessorDuplicator |
| Existing pattern | New | Matches O-AnalogEQ |
| Suitability for texture | Better (subtle shaping) | More aggressive |

**Recommendation: Use the 1-pole approach.** For a texture synthesizer where BRIGHTNESS is a subtle tonal shaping control (not a surgical EQ), the gentler 6 dB/oct slope is more appropriate. The dual-shelf approach is overkill and introduces unnecessary phase distortion.

## Parameter Mapping

### BRIGHTNESS (-1.0 to +1.0) to Filter Gains

The mapping uses exponential scaling for perceptually smooth response:

| BRIGHTNESS | Behavior | Low Gain | High Gain | Perceived Effect |
|------------|----------|----------|-----------|------------------|
| -1.0 | Full dark | +6 dB boost | -24 dB cut | Muffled, warm |
| -0.5 | Moderately dark | +3 dB boost | -12 dB cut | Slightly warm |
| 0.0 | Flat | 0 dB | 0 dB | No change |
| +0.5 | Moderately bright | -12 dB cut | +3 dB boost | Slightly airy |
| +1.0 | Full bright | -24 dB cut | +6 dB boost | Crisp, airy |

### Pivot Frequency Selection

The center frequency determines where the tilt crosses unity gain.

| Frequency | Character | Best For |
|-----------|-----------|----------|
| 500 Hz | Low crossover, affects more of spectrum | Broad tonal shifts |
| 800 Hz | Classic tilt point (recommended) | General-purpose, natural-sounding |
| 1000 Hz | Mid crossover | Balanced between bass and treble |
| 2000 Hz | High crossover | Mostly affects brightness perception |

**Recommendation:** Use 800 Hz as the default pivot frequency. This is the standard choice for tilt filters (used in the Quad 34 preamplifier, the original hardware tilt control). It provides a balanced pivot where the tilt feels "natural" -- neither too bassy nor too trebly at neutral positions.

### Gain Factor (gfactor) Selection

The gfactor controls asymmetry between the boosted and cut sides:

| gfactor | Max Boost | Max Cut | Character |
|---------|-----------|---------|-----------|
| 1 | 6 dB | 6 dB | Symmetric, mild |
| 2 | 6 dB | 12 dB | Moderate asymmetry |
| 4 | 6 dB | 24 dB | Strong asymmetry (recommended) |
| 5 | 6 dB | 30 dB | Original musicdsp value, very aggressive |

**Recommendation:** Use gfactor = 4. This provides a noticeable effect at extreme settings without making the filter feel like a lowpass/highpass. At gfactor = 5 (the musicdsp original), extreme settings essentially become a first-order lowpass/highpass which may be too aggressive for a texture synth.

## Stereo Processing

The filter processes each channel independently with its own lowpass state. This is critical because:

1. The left and right channels may have different content (stereo decorrelation via dual-decode)
2. Shared state would create mono artifacts
3. The `lpState` vector in the implementation holds one state per channel

The `processBlock` method handles this automatically by iterating over channels with separate state variables.

## Smoothing Strategy

### Why Smoothing is Needed

Without smoothing, changing filter gain abruptly causes "zipper noise" -- audible discontinuities in the output. This is especially noticeable with the tilt filter because gain changes affect the entire spectrum.

### Implementation: SmoothedValue

The implementation uses `juce::SmoothedValue<float, Linear>` for both `lgain` and `hgain`:

```cpp
smoothedLGain.reset (sampleRate, 0.05);  // 50ms ramp time
smoothedHGain.reset (sampleRate, 0.05);
```

- **Ramp time:** 50ms (2400 samples at 48kHz). Fast enough to feel responsive, slow enough to prevent artifacts.
- **Type:** Linear smoothing. Appropriate for gain values that can cross zero.
- **Per-sample advance:** `getNextValue()` is called once per sample in the inner loop, advancing the ramp.

### What Does NOT Need Smoothing

- **Filter coefficients (a0, b1):** These only change when `setCenterFrequency()` is called, which happens at initialization only (center frequency is fixed at 800 Hz). No runtime smoothing needed.
- **The lowpass state:** Filter state carries over naturally between blocks. No special handling needed.

### Important: SmoothedValue Advance Per Sample, Not Per Channel

In the `processBlock` implementation, `getNextValue()` is called once per sample (outer loop), and the resulting gain is applied to all channels for that sample. This ensures:
1. Both channels receive identical gain at each sample instant
2. The smoother advances at the correct rate (once per sample, not once per channel)

## Filter State Reset

### In prepareToPlay

```cpp
tiltFilter.prepare (sampleRate, 2);
tiltFilter.setCenterFrequency (800.0f);
tiltFilter.reset();
```

The `reset()` method:
1. Zeros all per-channel lowpass states
2. Snaps SmoothedValue to target (no ramp on first use)

### On Parameter Change

No explicit reset is needed when BRIGHTNESS changes -- the SmoothedValue handles smooth transitions automatically.

### On Transport Start/Stop (Optional)

If the DAW notifies the plugin of transport changes, resetting the filter state prevents "ringing" from the previous audio. However, for a texture synthesizer that runs continuously, this is typically not needed.

## Common Pitfalls

### Pitfall 1: Denormals

**What goes wrong:** The 1-pole lowpass state can decay toward very small floating-point values (denormals), causing severe CPU spikes on some processors.

**Prevention:** Always use `juce::ScopedNoDenormals noDenormals;` at the top of `processBlock()`. The existing TextureProcessor code already does this.

### Pitfall 2: SmoothedValue Not Reset on prepareToPlay

**What goes wrong:** If SmoothedValue is not reset with the new sample rate, the ramp duration will be wrong (too fast or too slow).

**Prevention:** Always call `smoothedLGain.reset(sampleRate, rampTime)` in `prepare()`.

### Pitfall 3: Division by Zero in Gain Calculation

**What goes wrong:** If gfactor or amp is zero, the gain calculation produces NaN.

**Prevention:** Both `gfactor` and `amp` are compile-time constants, so this cannot happen. But if gfactor is ever made user-configurable, clamp it to a minimum of 1.0.

### Pitfall 4: Coefficient Update on Audio Thread

**What goes wrong:** Calling `setCenterFrequency()` from the audio thread with complex coefficient recalculation.

**Prevention:** The center frequency is fixed at initialization (800 Hz). If it ever becomes runtime-configurable, the coefficient math is simple enough (2 multiplies, 1 divide) that it can safely run on the audio thread.

### Pitfall 5: Applying Tilt Before Overlap-Add

**What goes wrong:** Applying the tilt filter to individual blocks before overlap-add crossfading introduces spectral artifacts at block boundaries.

**Prevention:** Always apply the tilt filter AFTER the overlap-add output buffer is assembled. The tilt filter operates on the final continuous audio stream, not on individual synthesis blocks.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parameter smoothing | Custom linear ramp | `juce::SmoothedValue<float, Linear>` | Handles edge cases (target change mid-ramp, reset, etc.) |
| dB to gain | Custom pow function | `juce::Decibels::decibelsToGain()` | Consistent, correct |
| Stereo filter processing | Manual channel loop | The `processBlock` method above | Already handles arbitrary channel counts |

**The tilt filter itself IS hand-rolled** (intentionally). There is no JUCE built-in tilt filter. The musicdsp.org algorithm is the standard approach, well-proven and simple enough that a hand-rolled implementation is appropriate.

## Sources

### Primary (HIGH confidence)
- JUCE 8.0.4 source code (local): `juce_IIRFilter.h`, `juce_IIRFilter.cpp` -- Verified `makeLowShelf` / `makeHighShelf` API, confirmed gainFactor is linear gain (not dB)
- JUCE 8.0.4 source code (local): `juce_ProcessorDuplicator.h` -- Verified stereo processing pattern
- JUCE 8.0.4 source code (local): `juce_SmoothedValue.h` -- Verified SmoothedValue API
- JUCE 8.0.4 source code (local): `juce_FirstOrderTPTFilter.h` -- Reviewed but not recommended (only lowpass/highpass/allpass types, no tilt mode)
- O-AnalogEQ source code (local): `PluginProcessor.cpp` -- Verified existing shelf filter usage pattern in this codebase

### Secondary (MEDIUM confidence)
- [musicdsp.org: Simple Tilt Equalizer](https://www.musicdsp.org/en/latest/Filters/267-simple-tilt-equalizer.html) -- Algorithm source, well-established community reference
- [Audio EQ Cookbook](https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html) -- Robert Bristow-Johnson's biquad cookbook (basis for JUCE shelf filter implementations)

### Historical Reference
- [EDN: Implement an audio-frequency tilt-equalizer filter](https://www.edn.com/implement-an-audio-frequency-tilt-equalizer-filter/) -- Original Quad 34 tilt control design (1970s analog hardware origin)

## Metadata

**Confidence breakdown:**
- Algorithm correctness: HIGH -- verified against musicdsp.org source and mathematical analysis
- JUCE API usage: HIGH -- verified against local JUCE 8.0.4 source code
- Parameter mapping: HIGH -- mathematically derived, confirmed with O-AnalogEQ pattern
- Smoothing strategy: HIGH -- uses standard JUCE SmoothedValue pattern
- Center frequency choice: MEDIUM -- 800 Hz is standard but may need tuning by ear for neural textures

**Research date:** 2026-02-14
**Valid until:** Indefinite (fundamental DSP, not version-dependent)
