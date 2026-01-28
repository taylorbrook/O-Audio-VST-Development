# Phase 7: Oversampling & Adaptive Harmonics - Research

**Researched:** 2026-01-26
**Domain:** Audio DSP - Oversampling, Waveshaping, Pitch Tracking
**Confidence:** HIGH

## Summary

This phase addresses documented tech debt from the v1.0 milestone audit. The codebase has fully-implemented DSP components (HarmonicGenerator with oversamplers, PitchTracker with YIN algorithm) that were bypassed during Phase 2 development when a simpler tanh saturation approach "worked well enough" per human verification.

The gap closure requires:
1. Wiring the existing 4x oversamplers into HarmonicGenerator.process()
2. Calling PitchTracker.detectPitch() to drive adaptive harmonic count
3. Removing dead code (processOversampled unreachable, latency returning 0)
4. Updating documentation to match implementation

**Primary recommendation:** Wire existing components - do not reimplement. The DSP logic exists; this is integration work.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| juce::dsp::Oversampling | JUCE 7+ | Multi-stage oversampling with IIR/FIR options | JUCE's built-in, already in codebase |
| Chebyshev Polynomials T2-T5 | N/A | Controlled harmonic generation | Already implemented in HarmonicGenerator.cpp |
| YIN Algorithm | N/A | Monophonic pitch detection | Already implemented in PitchTracker.cpp |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| juce::dsp::IIR::Filter | JUCE 7+ | Output bandpass filtering | Already used for 40-400Hz limiting |
| std::atomic | C++17 | Thread-safe mode switching | Already used for activeMode |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| JUCE Oversampling | Custom polyphase filter | More control but significant implementation effort |
| YIN pitch detection | FFT-based pitch | FFT has higher latency for bass frequencies |

**No installation needed** - all components already exist in the codebase.

## Architecture Patterns

### Existing Project Structure
```
plugins/OBass/Source/DSP/
├── HarmonicGenerator.h/cpp  # Chebyshev waveshaper - needs 4x oversampling wired in
├── PitchTracker.h/cpp       # YIN algorithm - needs detectPitch() called
├── CleanModeProcessor.h/cpp # Orchestrator - needs to wire pitch -> harmonics
├── EnvelopeFollower.h/cpp   # Working correctly
└── ColoredModeProcessor.h/cpp # Not affected by this phase
```

### Pattern 1: JUCE Oversampling Pipeline
**What:** Process audio at elevated sample rate to prevent aliasing from nonlinear operations
**When to use:** Any waveshaping, saturation, or harmonic generation
**Example:**
```cpp
// Source: JUCE Official Documentation
// In process() method:
juce::dsp::AudioBlock<float> inputBlock(buffer);
auto oversampledBlock = oversampler->processSamplesUp(inputBlock);

// Process at higher sample rate
processOversampled(oversampledBlock.getChannelPointer(0),
                   static_cast<int>(oversampledBlock.getNumSamples()));

// Downsample back to original rate
oversampler->processSamplesDown(inputBlock);
```

### Pattern 2: Pitch-Adaptive Processing
**What:** Adjust processing parameters based on detected fundamental frequency
**When to use:** Bass enhancement where harmonic count should vary with pitch
**Example:**
```cpp
// Source: Existing setAdaptiveHarmonics() in HarmonicGenerator.cpp
// Already implemented - just needs to be called:
float fundamental = pitchTracker.detectPitch(input, numSamples);
if (fundamental > 0.0f) {
    harmonicGenerator.setAdaptiveHarmonics(fundamental);
}
```

### Anti-Patterns to Avoid
- **Rewriting working DSP:** The Chebyshev polynomials and YIN algorithm are correctly implemented. Don't refactor them.
- **Changing the tanh saturation path:** Keep the existing working path as fallback. Wire oversampling in parallel for A/B comparison.
- **Breaking pluginval:** The plugin currently passes strictness 10. Any changes must maintain this.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Oversampling filters | Custom polyphase filter | juce::dsp::Oversampling | Already created and prepared in HarmonicGenerator |
| Pitch detection | Custom autocorrelation | PitchTracker (YIN) | Already implemented with parabolic interpolation |
| Anti-aliasing | Manual low-pass before waveshaping | JUCE oversampler's built-in decimation filter | Handles both directions |
| Latency compensation | Manual buffer delays | Oversampler.getLatencyInSamples() | JUCE computes it automatically |

**Key insight:** This phase is wiring, not implementation. The DSP components exist but aren't connected.

## Common Pitfalls

### Pitfall 1: Incorrect Oversampling Factor Parameter
**What goes wrong:** Requesting wrong oversampling amount
**Why it happens:** JUCE uses `factor` as power of 2, not direct multiplier
**How to avoid:** factor=2 means 2^2=4x oversampling, factor=1 means 2^1=2x
**Warning signs:** Hearing aliasing despite "enabling" oversampling

**Current code (WRONG for 4x):**
```cpp
// HarmonicGenerator.cpp line 41-43 - CREATES 2x, NOT 4x
oversamplerIIR = std::make_unique<juce::dsp::Oversampling<float>>(
    1,  // numChannels
    1,  // factor (2^1 = 2x) - THIS IS 2x NOT 4x
```

**Correct for 4x:**
```cpp
oversamplerIIR = std::make_unique<juce::dsp::Oversampling<float>>(
    1,  // numChannels
    2,  // factor (2^2 = 4x) - THIS IS 4x
```

### Pitfall 2: Forgetting to Scale Internal Sample Rate
**What goes wrong:** Processing at original sample rate despite upsampling
**Why it happens:** Upsampling creates more samples but doesn't auto-adjust processing
**How to avoid:** Inside processOversampled(), remember the sample rate is 4x higher
**Warning signs:** Filters and time constants behave incorrectly

### Pitfall 3: Latency Mismatch Breaking DAW Sync
**What goes wrong:** Plugin reports wrong latency, DAW compensation fails
**Why it happens:** getLatencyInSamples() currently returns 0 (debug placeholder)
**How to avoid:** Return actual oversampler latency from getLatencyInSamples()
**Warning signs:** Phase issues when mixing processed with dry signal

**Current code (WRONG):**
```cpp
// HarmonicGenerator.cpp line 205-209
int HarmonicGenerator::getLatencyInSamples() const
{
    // TEMPORARY: Return 0 to debug sample rate issue
    return 0;
}
```

**Correct:**
```cpp
int HarmonicGenerator::getLatencyInSamples() const
{
    auto* oversampler = getActiveOversampler();
    return oversampler ? static_cast<int>(oversampler->getLatencyInSamples()) : 0;
}
```

### Pitfall 4: Pitch Tracker Window Size vs Block Size Mismatch
**What goes wrong:** Pitch detection fails or gives unstable results
**Why it happens:** YIN needs ~3000 samples for 30Hz detection, but blocks may be 512
**How to avoid:** PitchTracker already accumulates samples in ring buffer - this is handled
**Warning signs:** Pitch jumping erratically or always returning 0

### Pitfall 5: Not Testing with pluginval After Changes
**What goes wrong:** Regression in plugin validation
**Why it happens:** DSP changes can introduce edge cases (buffer sizes, automation, etc.)
**How to avoid:** Run `pluginval --strictness-level 10` after each plan
**Warning signs:** Crashes during rapid parameter automation

## Code Examples

Verified patterns from the existing codebase:

### Wiring Oversampling into HarmonicGenerator.process()
```cpp
// Source: Pattern derived from JUCE docs + existing codebase structure
void HarmonicGenerator::process(juce::AudioBuffer<float>& monoBuffer)
{
    const int numSamples = monoBuffer.getNumSamples();
    if (numSamples == 0)
        return;

    // Get active oversampler based on mode
    auto* oversampler = getActiveOversampler();
    if (!oversampler)
        return;  // Fallback: no processing if oversamplers not ready

    // Create audio block from buffer
    juce::dsp::AudioBlock<float> inputBlock(monoBuffer);

    // Upsample to 4x sample rate
    auto oversampledBlock = oversampler->processSamplesUp(inputBlock);

    // Process at elevated sample rate (Chebyshev waveshaping)
    processOversampled(oversampledBlock.getChannelPointer(0),
                       static_cast<int>(oversampledBlock.getNumSamples()));

    // Downsample back to original rate
    oversampler->processSamplesDown(inputBlock);

    // Apply output bandpass filter (at original sample rate)
    float* data = monoBuffer.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i)
    {
        data[i] = outputBandpassLow.processSample(data[i]);
        data[i] = outputBandpassHigh.processSample(data[i]);
    }
}
```

### Wiring PitchTracker in CleanModeProcessor.process()
```cpp
// Source: Existing CleanModeProcessor + PitchTracker APIs
void CleanModeProcessor::process(juce::AudioBuffer<float>& monoBuffer)
{
    const int numSamples = monoBuffer.getNumSamples();
    if (numSamples == 0)
        return;

    // Store dry signal (already implemented)
    dryBuffer.copyFrom(0, 0, monoBuffer, 0, 0, numSamples);

    // Detect pitch for adaptive harmonics (NEW - currently missing)
    float detectedPitch = pitchTracker.detectPitch(
        monoBuffer.getReadPointer(0), numSamples);

    if (detectedPitch > 0.0f)
    {
        harmonicGenerator.setAdaptiveHarmonics(detectedPitch);
    }

    // Generate harmonics with oversampling
    harmonicGenerator.process(monoBuffer);

    // ... rest of existing processing (transient ducking, blending)
}
```

### Correct Latency Reporting
```cpp
// Source: JUCE Oversampling API
int HarmonicGenerator::getLatencyInSamples() const
{
    auto* oversampler = getActiveOversampler();
    if (!oversampler)
        return 0;

    // JUCE oversampler returns float latency - round to int
    return static_cast<int>(std::round(oversampler->getLatencyInSamples()));
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Direct tanh saturation | Chebyshev + oversampling | Planned in Phase 2, deferred | More controlled harmonics, less aliasing |
| Fixed 5 harmonics | Pitch-adaptive count | Planned in Phase 2, deferred | Better psychoacoustic effect across range |

**Current codebase state:**
- Chebyshev polynomials T2-T5: Implemented but unreachable (processOversampled dead code)
- Oversamplers: Created and prepared but processSamplesUp/Down never called
- PitchTracker: Prepared but detectPitch() never called
- Adaptive harmonics logic: Implemented in setAdaptiveHarmonics() but never invoked

## Open Questions

Things that couldn't be fully resolved:

1. **Should we A/B test oversampled vs current tanh path?**
   - What we know: Current tanh path passed human verification
   - What's unclear: Will 4x oversampled Chebyshev sound better or just different?
   - Recommendation: Wire oversampling in, test, keep if better. The audit says oversampling was a requirement (DSP-04).

2. **Latency impact on user experience**
   - What we know: 4x oversampling adds latency (IIR: ~few samples, FIR: more)
   - What's unclear: Will returning actual latency break DAW workflows?
   - Recommendation: Return correct latency. DAW latency compensation is designed for this.

3. **Should ColoredModeProcessor also get oversampling?**
   - What we know: Phase 7 scope only mentions HarmonicGenerator (Clean mode)
   - What's unclear: Does Colored mode also need aliasing protection?
   - Recommendation: Out of scope for Phase 7. Can add in Phase 8 if needed.

## Sources

### Primary (HIGH confidence)
- [JUCE Official Documentation - Oversampling Class](https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html) - API reference, constructor parameters, usage pattern
- Existing codebase: `plugins/OBass/Source/DSP/HarmonicGenerator.h/cpp` - Current implementation state
- Existing codebase: `plugins/OBass/Source/DSP/PitchTracker.h/cpp` - YIN implementation
- `.planning/v1.0-MILESTONE-AUDIT.md` - Gap identification and tech debt list

### Secondary (MEDIUM confidence)
- [JUCE Forum - Oversampling Discussion](https://forum.juce.com/t/juce-dsp-oversampling-how-to-add-oversampling-stages-and-different-filters/60796) - Community usage patterns
- [KVR Audio - Chebyshev Waveshaping](https://www.kvraudio.com/forum/viewtopic.php?t=70372) - Oversampling requirements for polynomial waveshapers
- [Introduction to Oversampling for Alias Reduction](https://www.nickwritesablog.com/introduction-to-oversampling-for-alias-reduction/) - Best practices

### Tertiary (LOW confidence)
- [KVR Audio - Polynomial shapers and oversampling](https://www.kvraudio.com/forum/viewtopic.php?t=208373) - Community discussion on required oversampling factors

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Using JUCE built-ins already in codebase
- Architecture: HIGH - Wiring existing components, not designing new ones
- Pitfalls: HIGH - Based on actual code review and official JUCE docs

**Research date:** 2026-01-26
**Valid until:** 60 days (stable DSP domain, no breaking changes expected)

---

## Phase-Specific Wiring Guide

This section summarizes exactly what needs to change:

### 07-01: Wire 4x Oversampling
**File:** `HarmonicGenerator.cpp`

1. Change oversampler factor from 1 to 2 (line ~43, ~51):
   ```cpp
   // Change: 1 -> 2 for 4x oversampling
   oversamplerIIR = std::make_unique<juce::dsp::Oversampling<float>>(1, 2, ...);
   ```

2. Replace process() body to use oversampling pipeline (line ~128-160):
   - Call processSamplesUp()
   - Call processOversampled() on upsampled block
   - Call processSamplesDown()

3. Fix getLatencyInSamples() to return actual latency (line ~205-209)

### 07-02: Wire PitchTracker
**File:** `CleanModeProcessor.cpp`

1. Add detectPitch() call in process() before harmonicGenerator.process():
   ```cpp
   float pitch = pitchTracker.detectPitch(monoBuffer.getReadPointer(0), numSamples);
   if (pitch > 0.0f) {
       harmonicGenerator.setAdaptiveHarmonics(pitch);
   }
   ```

2. Fix getLatencyInSamples() to return actual latency (line ~240-245)

### 07-03: Dead Code Cleanup
**Files:** `HarmonicGenerator.cpp`, `CleanModeProcessor.cpp`

1. Remove debug comments like "TEMPORARY: Return 0"
2. Ensure processOversampled() is no longer dead code (should be called now)
3. Update header comments if they don't match implementation
4. Verify STATE.md decisions match what code actually does
