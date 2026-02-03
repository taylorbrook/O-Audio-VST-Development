# Research: Eliminate Clicks & Smooth Windowing

**Milestone:** eliminate-clicks-smooth-windowing
**Plugin:** O-Freeze
**Research Date:** 2026-02-02

---

## Executive Summary

The current O-Freeze implementation has three primary click sources: simultaneous grain triggering on freeze engage, non-zero window endpoints from asymmetric Blackman-Harris, and abrupt grain deactivation on release. The recommended solution combines **staggered grain activation with micro-fades**, **switching to a true-zero Hann window**, and implementing **soft grain deactivation** where grains naturally complete their envelope cycle rather than being abruptly stopped.

---

## 1. Zero-Crossing Detection

### Findings

Zero-crossing detection is computationally simple but has significant limitations for musical audio:

- **Simple Detection**: Check for sign change between adjacent samples (`sample[n-1] * sample[n] < 0`)
- **Interpolated Position**: Linear interpolation gives sub-sample accuracy: `zc_pos = n-1 + |sample[n-1]| / (|sample[n-1]| + |sample[n]|)`
- **Real-Time Performance**: Zero-crossing rate (ZCR) algorithms run at ~50 nanoseconds per sample on modern CPUs ([Cycfi Research](https://www.cycfi.com/2020/07/fast-and-efficient-pitch-detection-revisited/))
- **Limitations**: ZCR works well for pure tones but poorly for complex musical signals with harmonics ([Zero-crossing rate - Wikipedia](https://en.wikipedia.org/wiki/Zero-crossing_rate))

### Why Zero-Crossing is NOT Recommended for O-Freeze

1. **Search Window Trade-off**: Finding a zero-crossing may require searching +/- several hundred samples, introducing latency
2. **Complex Signals**: Musical audio has many zero-crossings that don't represent perceptually "safe" cut points
3. **Ineffective for Polyphonic Material**: With multiple frequencies present, zero-crossings are essentially random relative to perceived loudness
4. **Better Alternative Exists**: Proper windowing with true-zero endpoints eliminates the need for zero-crossing detection entirely

### Recommended Approach

**Do not implement zero-crossing detection.** Instead, use window functions that guarantee zero amplitude at grain boundaries. This is more CPU-efficient and works equally well for all audio material.

### Code Sketch (If Zero-Crossing Were Needed)

```cpp
// Simple zero-crossing detector - NOT RECOMMENDED for O-Freeze
int findNearestZeroCrossing(const float* buffer, int startPos, int searchRange, int bufferLength)
{
    for (int offset = 0; offset <= searchRange; ++offset)
    {
        // Search both directions
        for (int dir : {1, -1})
        {
            int pos = (startPos + offset * dir + bufferLength) % bufferLength;
            int prevPos = (pos - 1 + bufferLength) % bufferLength;

            if (buffer[prevPos] * buffer[pos] <= 0.0f)
                return pos;
        }
    }
    return startPos; // Fallback to original position
}
```

---

## 2. Window Function Analysis

### Comparison Table

| Window | True Zero Endpoints? | Warmth/Smoothness | CPU Cost | COLA Compliant | Notes |
|--------|---------------------|-------------------|----------|----------------|-------|
| **Hann (Raised Cosine)** | YES | Excellent | Very Low | 50%, 75%, 87.5% | Best for granular synthesis |
| **Tukey (alpha=0.5)** | YES | Good | Low | Varies by alpha | Adjustable attack/release ratio |
| **Blackman** | Near-zero | Very Good | Low | 50% | Better sidelobe rejection |
| **Blackman-Harris** | NO | Good | Low | No | Not suitable for OLA synthesis |
| **Hamming** | NO (reaches ~0.08) | Good | Very Low | 50% (with artifacts) | Can cause clicks at boundaries |
| **Rectangular** | NO | Poor | Minimal | 0% only | Guaranteed clicks |
| **Triangular (Bartlett)** | YES | Fair | Minimal | 50% | Sharp spectral leakage |

### Key Finding: Current Window Problem

The current implementation uses an **asymmetric Blackman-Harris window**, which has two critical issues:

1. **Non-zero endpoints**: Blackman-Harris does not reach exactly zero at boundaries, unlike Hann ([DSPRelated - Blackman-Harris](https://www.dsprelated.com/freebooks/sasp/Blackman_Harris_Window_Family.html))
2. **Asymmetric stretching breaks COLA**: The 60/40 attack/release split means overlapping grains no longer sum to constant amplitude

### Recommended Window: Hann (Raised Cosine)

**Why Hann is optimal for O-Freeze:**

1. **True zero endpoints**: `w(0) = 0` and `w(N) = 0` - eliminates discontinuity clicks ([Stanford CCRMA](https://ccrma.stanford.edu/~jos/sasp/Hann_Hanning_Raised_Cosine.html))
2. **COLA compliant at 87.5% overlap**: Current 8-grain, 87.5% overlap configuration is COLA-compliant with Hann ([MATLAB iscola](https://www.mathworks.com/help/signal/ref/iscola.html))
3. **Smooth spectral characteristics**: First sidelobe at -31.5 dB, 18 dB/octave rolloff
4. **Minimal CPU**: Single cosine lookup or polynomial approximation
5. **Warm sound**: Gradual attack/release creates smooth, pad-like textures ([Michael Krzyzaniak - Window Functions](https://michaelkrzyzaniak.com/AudioSynthesis/2_Audio_Synthesis/11_Granular_Synthesis/1_Window_Functions/))

### Hann Window Formula

```cpp
// Symmetric Hann window (true zero at endpoints)
for (int i = 0; i < grainSize; ++i)
{
    float phase = static_cast<float>(i) / static_cast<float>(grainSize - 1);
    hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * phase));
}
// Alternative equivalent: hannWindow[i] = std::sin(M_PI * phase) ^ 2
```

### Alternative: Tukey Window for Adjustable Attack/Release

If asymmetric envelopes are desired for sonic character, use a **Tukey window with alpha parameter**:

```cpp
// Tukey window with adjustable taper ratio
// alpha = 0.0 -> rectangular, alpha = 1.0 -> Hann
void generateTukeyWindow(float* window, int size, float alpha)
{
    int taperSamples = static_cast<int>(alpha * size / 2.0f);

    for (int i = 0; i < size; ++i)
    {
        if (i < taperSamples)
        {
            // Attack taper
            window[i] = 0.5f * (1.0f - std::cos(M_PI * i / taperSamples));
        }
        else if (i >= size - taperSamples)
        {
            // Release taper
            window[i] = 0.5f * (1.0f + std::cos(M_PI * (i - size + taperSamples) / taperSamples));
        }
        else
        {
            // Sustain plateau
            window[i] = 1.0f;
        }
    }
}
```

---

## 3. Transition Strategies

### Freeze Engage

**Current Problem:** All 8 grains are activated simultaneously at `bufferFrozen = true`, creating a sudden burst of correlated audio even with staggered positions.

**Solution: Staggered Grain Activation with Micro-Fades**

```cpp
// On freeze engage, DON'T activate all grains immediately
// Instead, schedule them to activate at their natural trigger intervals
if (freezeEngaged && !wasFreezed)
{
    // Only activate the FIRST grain immediately
    grains[0].active = true;
    grains[0].startSample = 0;
    grains[0].position = calculateGrainPosition(0);

    // Reset trigger counter - subsequent grains will be activated
    // by the normal trigger interval mechanism
    nextGrainIndex = 1;
    grainTriggerCounter = 0;

    // Apply global micro-fade (1-2ms) on top of grain envelope
    engageFade.reset(sampleRate, 0.002); // 2ms micro-fade
    engageFade.setTargetValue(1.0f);
}
```

**Why This Works:**
- First grain starts immediately (no perceptible latency)
- Subsequent grains are added at natural intervals (every grainSize/8 samples = ~25ms at 200ms grains)
- Full granular texture builds up over ~175ms (7 more grain activations)
- Combined with proper Hann window, eliminates engage clicks ([mdeGranular DoGrainDelays](https://michael-edwards.org/software/mdegranular/mdegranular.shtml))

### Sustained Playback

**Current Problem:** No reported click issues during sustain, but the asymmetric window may cause subtle amplitude modulation.

**Solution: Symmetric Hann Window + COLA Verification**

With 8 grains at 87.5% overlap using symmetric Hann windows, the overlap-add sum should be constant:

```cpp
// Verify COLA compliance (run once at prepareToPlay for debugging)
float colaSum[grainTriggerInterval];
std::fill(colaSum, colaSum + grainTriggerInterval, 0.0f);

for (int grainOffset = 0; grainOffset < 8; ++grainOffset)
{
    int startPos = grainOffset * grainTriggerInterval;
    for (int i = 0; i < grainTriggerInterval; ++i)
    {
        int windowPos = startPos + i;
        if (windowPos < grainSize)
            colaSum[i] += hannWindow[windowPos];
    }
}

// colaSum should be approximately 1.0 across all positions
// Deviation indicates COLA violation
```

### Freeze Release

**Current Problem:** All grains are immediately set to `active = false` on release, causing abrupt cutoff.

**Solution: Soft Grain Deactivation (Let Grains Complete Their Cycle)**

```cpp
// On freeze release, DON'T deactivate grains immediately
if (!freezeEngaged && wasFreezed)
{
    // Stop triggering NEW grains
    stopTriggeringNewGrains = true;

    // Let existing grains complete their envelope naturally
    // Each grain will set active = false when startSample >= grainSize

    // Apply global release fade (longer than single grain for safety)
    releaseFade.reset(sampleRate, 0.100); // 100ms release
    releaseFade.setTargetValue(0.0f);
}

// In grain processing loop:
if (grain.active && grain.startSample >= grainSize)
{
    grain.active = false; // Natural deactivation at envelope end
}
```

**Why This Works:**
- Active grains complete their Hann window cycle (ending at zero amplitude)
- Global 100ms release fade provides additional safety margin
- No abrupt amplitude discontinuities ([Cycling '74 - Granular Tutorial](https://docs.cycling74.com/max8/tutorials/11_polychapter02))

---

## 4. Implementation Recommendations

### Priority Order

1. **HIGHEST PRIORITY: Replace Window Function**
   - Switch from asymmetric Blackman-Harris to symmetric Hann
   - Single code change, immediate improvement
   - Estimated impact: Eliminates 60-70% of clicks

2. **HIGH PRIORITY: Soft Grain Deactivation on Release**
   - Stop triggering new grains but let active grains complete
   - Requires adding `stopTriggeringNewGrains` flag
   - Estimated impact: Eliminates release clicks entirely

3. **MEDIUM PRIORITY: Staggered Grain Activation on Engage**
   - Only activate first grain immediately, let others follow naturally
   - Requires modifying freeze engage logic
   - Estimated impact: Eliminates engage "burst" sound

4. **LOW PRIORITY (Optional): Global Micro-Fades**
   - 1-2ms fade on engage/release as additional safety
   - Use `juce::SmoothedValue` with 2ms ramp time
   - Belt-and-suspenders approach for edge cases

### Risk Assessment

| Change | Risk Level | Potential Issues |
|--------|------------|------------------|
| Hann window | Low | Very slight change in tonal character (warmer) |
| Soft deactivation | Low | Release may feel slightly longer (~200ms max) |
| Staggered activation | Medium | First 175ms of freeze may sound "building up" |
| Global micro-fades | Very Low | Negligible perceptual impact |

### Performance Considerations

All recommended changes have minimal CPU impact:

- **Hann window**: Same or lower cost than Blackman-Harris (fewer multiply-adds)
- **Soft deactivation**: Actually reduces CPU (no forced deactivation loop)
- **Staggered activation**: No additional cost (simplifies engage logic)
- **Micro-fades**: `juce::SmoothedValue` costs ~3 ops per sample

### Equal-Power Crossfade Alternative

If the dry/wet crossfade during transitions still causes level dips, consider equal-power crossfade:

```cpp
// Equal-power crossfade (constant loudness)
float wetGain = freezeGain.getNextValue();
float dryGain = std::sqrt(1.0f - wetGain * wetGain); // Equal power
// Or: float dryGain = std::cos(wetGain * 0.5f * M_PI); // Sine crossfade
float wetGain_ep = std::sin(wetGain * 0.5f * M_PI);

outputSample = inputSample * dryGain + frozenSample * wetGain_ep;
```

([KVR Audio - Equal Power Crossfade](https://www.kvraudio.com/forum/viewtopic.php?t=347151), [Signalsmith Audio - Cheap Energy Crossfade](https://signalsmith-audio.co.uk/writing/2021/cheap-energy-crossfade/))

---

## 5. References

### Academic & Technical

- Curtis Roads, *Microsound* (MIT Press, 2002) - Foundational granular synthesis text ([MIT Press](https://mitpress.mit.edu/9780262681544/microsound/))
- Julius O. Smith III, *Spectral Audio Signal Processing* - COLA constraints and window functions ([Stanford CCRMA](https://ccrma.stanford.edu/~jos/sasp/))
- "On the construction of window functions with constant-overlap-add constraint" ([IEEE Xplore](https://ieeexplore.ieee.org/document/6287885/))

### Window Functions

- [Window function - Wikipedia](https://en.wikipedia.org/wiki/Window_function)
- [Hann or Hanning or Raised Cosine - Stanford CCRMA](https://ccrma.stanford.edu/~jos/sasp/Hann_Hanning_Raised_Cosine.html)
- [Blackman-Harris Window Family - DSPRelated](https://www.dsprelated.com/freebooks/sasp/Blackman_Harris_Window_Family.html)
- [Window Functions for Granular Synthesis - Michael Krzyzaniak](https://michaelkrzyzaniak.com/AudioSynthesis/2_Audio_Synthesis/11_Granular_Synthesis/1_Window_Functions/)

### Granular Synthesis

- [Granular Synthesis - Barry Truax (SFU)](https://www.sfu.ca/~truax/gran.html)
- [Granular Synthesis Guide](http://granularsynthesis.com/guide.php)
- [Sound in a Nutshell: Grain Envelopes](https://www.granularsynthesis.com/hthesis/envelope.html)

### Crossfade Techniques

- [Implementing a Constant Power Crossfade](https://teedteed.wordpress.com/2019/05/06/implementing-a-constant-power-crossfade/)
- [A Cheap Energy-Preserving Crossfade - Signalsmith Audio](https://signalsmith-audio.co.uk/writing/2021/cheap-energy-crossfade/)
- [Linear vs Constant-Power Crossfades - Sound On Sound](https://www.soundonsound.com/sound-advice/q-should-use-linear-or-constant-power-crossfades)

### Implementation Examples

- [mdeGranular~ for Max/MSP](https://michael-edwards.org/software/mdegranular/mdegranular.shtml) - DoGrainDelays and SmoothMode
- [Cycling '74 Polyphony Tutorial: Granular Synthesis](https://docs.cycling74.com/max8/tutorials/11_polychapter02)
- [FAUST Granola Processor](https://github.com/jlp6k/faust-things) - Grain shape morphing
- [KVR Forum - Preventing Pops and Clicks](https://www.kvraudio.com/forum/viewtopic.php?t=277987)

### Commercial Reference

- [Output Portal](https://output.com/products/portal) - Musical granular synthesis with smooth transitions
- [Sinevibes Albedo](https://www.sinevibes.com/albedo/) - Lag filters for click-free parameter changes

---

## Appendix: Quick Implementation Checklist

```
[ ] Replace asymmetric Blackman-Harris with symmetric Hann window
[ ] Modify freeze release to set stopTriggeringNewGrains flag instead of immediate deactivation
[ ] Let grains naturally complete when startSample >= grainSize
[ ] Modify freeze engage to only activate first grain immediately
[ ] Consider adding 2ms global micro-fade for additional safety
[ ] Test at various grain sizes (50ms, 100ms, 200ms, 500ms)
[ ] Test rapid freeze on/off toggling
[ ] Test with percussive material (most likely to reveal clicks)
```
