# Phase 3.2: Body Resonator + Stereo Width - Research

**Researched:** 2026-04-05
**Domain:** Parallel biquad body resonator with morphable coefficients + stereo decorrelation
**Confidence:** HIGH

## Summary

Phase 3.2 adds two processing stages to the existing mono bowed string signal path: (1) an 8-section parallel peaking EQ filter bank that models instrument body resonance with material morphing and size scaling, and (2) a stereo width processor that creates a stereo image from the mono waveguide output.

The body resonator follows the proven O-Lyrica `BodyResonance` pattern: an array of `juce::dsp::IIR::Filter<float>` objects, each set via `makePeakFilter`, processing the same input in parallel with outputs summed. The key difference from O-Lyrica is the morphing system -- O-Bowed interpolates between 4 preset banks (Membrane/Wood/Metal/Glass) by interpolating the **parameters** (frequency, Q, gain) in their natural domains (frequency in log, Q and gain in linear) and calling `makePeakFilter` to generate new coefficients. This is safer than raw coefficient interpolation because it guarantees filter stability at every morph position.

For stereo creation from the mono body resonator output, the recommended approach is a single allpass decorrelator on the right channel. This creates frequency-dependent phase differences between L and R without changing the magnitude spectrum, producing a natural stereo spread that the mid-side WIDTH processor can then widen or narrow.

**Primary recommendation:** Implement `BodyResonator` as a standalone DSP class (like O-Lyrica's `BodyResonance`) with the O-Lyrica thread-safety pattern (atomic pending parameters + `applyPendingFilterUpdates()` at start of process call). Implement `StereoWidthProcessor` as a simple class operating on stereo buffers at the processor level (post-voice summing), not per-voice.

---

## Project Constraints (from CLAUDE.md)

- JUCE version: 8.0.4
- Build: CMake + Ninja, macOS targets VST3 + AU
- Plugin cache clearing required after every build
- Research documents go in `research/` but planning research goes in `.planning/`
- Test in DAW after installation

---

## JUCE API Patterns (Verified from Local Source)

### makePeakFilter Exact Signature

Source: `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_IIRFilter.h` line 239

```cpp
static Ptr makePeakFilter (double sampleRate,
                           NumericType centreFrequency,
                           NumericType Q,
                           NumericType gainFactor);
```

**CRITICAL: `gainFactor` is a LINEAR scale factor, NOT decibels.**

From the JUCE doc comment: "The gain is a scale factor that the centre frequencies are multiplied by, so values greater than 1.0 will boost the centre frequencies, values less than 1.0 will attenuate them."

The implementation (line 270) does: `A = sqrt(Decibels::gainWithLowerBound(gainFactor, minimumDecibels))` -- it treats the input as a linear gain value, not dB.

**Since the ARCHITECTURE.md specifies gains in dB, convert before calling:**
```cpp
float gainLinear = juce::Decibels::decibelsToGain (gainDb);
// e.g., +12 dB -> 3.981, +8 dB -> 2.512, +2 dB -> 1.259
```

**Assertions enforced internally:**
- `sampleRate > 0`
- `frequency > 0 && frequency <= sampleRate * 0.5` (must not exceed Nyquist)
- `Q > 0`
- `gainFactor > 0` (must be positive)

### IIR::Filter<float> Usage for Parallel Bank

Verified from O-Lyrica `BodyResonance.cpp`:

```cpp
// Declaration: array of 8 parallel biquad filters
static constexpr int NUM_MODES = 8;
std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModes;

// Preparation
void prepare (double sampleRate, int maxBlockSize)
{
    juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32> (maxBlockSize),
        1  // mono processing per voice
    };

    for (auto& filter : bodyModes)
    {
        filter.prepare (spec);
        filter.reset();
    }
}

// Coefficient update (called on audio thread only)
void updateFilterCoefficients (double sampleRate, const ModeParams* params)
{
    for (int i = 0; i < NUM_MODES; ++i)
    {
        float freq = params[i].frequency;
        float Q    = params[i].q;
        float gain = juce::Decibels::decibelsToGain (params[i].gainDb);

        // Clamp frequency to valid range (must be < Nyquist)
        freq = juce::jlimit (20.0f, static_cast<float> (sampleRate * 0.45), freq);

        bodyModes[i].coefficients =
            juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                sampleRate, freq, Q, gain);
    }
}

// Per-sample processing: parallel topology
float process (float input)
{
    float output = 0.0f;
    for (auto& filter : bodyModes)
        output += filter.processSample (input);
    return output;
}
```

**Confidence:** HIGH -- verified against local JUCE 8.0.4 source AND proven working in O-Lyrica v1.32.6.

---

## Coefficient Morphing Strategy

### The Problem: Two Approaches

The ARCHITECTURE.md mentions both approaches:
1. Line 449: "lerp(bankA[i], bankB[i], t)" (raw coefficient interpolation)
2. Line 451: "Recalculate peaking EQ coefficients with scaled frequencies using makePeakFilter"

### Recommendation: Parameter Interpolation + Recalculation

**Use parameter interpolation (freq, Q, gain), then call makePeakFilter.** Do NOT interpolate raw biquad coefficients.

**Why raw coefficient lerp is dangerous:**
- A biquad has 6 coefficients (b0, b1, b2, a1, a2 normalized by a0). Linear interpolation between two stable filter coefficient sets can produce an UNSTABLE filter at intermediate positions.
- The poles of the interpolated filter are NOT the linear interpolation of the individual poles. A stable filter has poles inside the unit circle, but lerp can push them outside.
- This is a well-known DSP pitfall. It manifests as sudden oscillation or explosion at certain morph positions.

**Why parameter interpolation is safe:**
- Interpolating frequency, Q, and gain produces parameters that `makePeakFilter` will always turn into a stable filter.
- `makePeakFilter` derives coefficients from the Audio EQ Cookbook formulas which guarantee stability for any valid input (freq > 0 < Nyquist, Q > 0, gain > 0).
- The morph sounds more natural because perceptual parameters change linearly.

**Frequency interpolation domain:**
Interpolate frequencies in **log domain** for perceptually linear morphing:
```cpp
float lerpedFreq = std::exp (
    std::log (freqA) * (1.0f - t) + std::log (freqB) * t);
```
This gives equal musical interval at every morph position (e.g., octave spacing stays consistent).

Q and gain can be interpolated linearly:
```cpp
float lerpedQ    = qA * (1.0f - t) + qB * t;
float lerpedGain = gainDbA * (1.0f - t) + gainDbB * t;
```

**Performance note:** `makePeakFilter` is called only on parameter change (NOT per-sample). With 8 filters, that is 8 calls each containing a few trig operations. At <1us total, this is negligible.

### Complete Morph Algorithm

```cpp
// BODY_MATERIAL 0.0-1.0 maps to 4 presets at positions 0, 0.333, 0.666, 1.0
// Determine which two presets to interpolate between

struct ModePreset {
    float freq[8];
    float q[8];
    float gainDb[8];
};

static constexpr int NUM_PRESETS = 4;
static const ModePreset presets[NUM_PRESETS] = { /* membrane, wood, metal, glass */ };

void computeMorphedParams (float material, float bodySize, double sampleRate,
                           ModeParams* output)
{
    // Find two adjacent presets
    float scaled = material * (NUM_PRESETS - 1);   // 0.0 to 3.0
    int idxA = juce::jlimit (0, NUM_PRESETS - 2, static_cast<int> (scaled));
    int idxB = idxA + 1;
    float t = scaled - static_cast<float> (idxA);  // fractional position 0-1

    const auto& bankA = presets[idxA];
    const auto& bankB = presets[idxB];

    for (int i = 0; i < 8; ++i)
    {
        // Log-domain frequency interpolation
        float freq = std::exp (
            std::log (bankA.freq[i]) * (1.0f - t)
          + std::log (bankB.freq[i]) * t);

        // Linear Q and gain interpolation
        float q      = bankA.q[i]      * (1.0f - t) + bankB.q[i]      * t;
        float gainDb = bankA.gainDb[i] * (1.0f - t) + bankB.gainDb[i] * t;

        // Apply body size scaling
        // f_scaled = f_base * pow(2.0, (0.5 - bodySize) * 3.0)
        // bodySize=0 (small) -> 2^1.5 = 2.83x up
        // bodySize=0.5 (mid) -> 2^0 = 1.0x
        // bodySize=1.0 (large) -> 2^-1.5 = 0.354x down
        freq *= std::pow (2.0f, (0.5f - bodySize) * 3.0f);

        // Nyquist clamp
        freq = juce::jlimit (20.0f, static_cast<float> (sampleRate * 0.45), freq);

        output[i] = { freq, q, gainDb };
    }
}
```

**Confidence:** HIGH -- parameter interpolation is the standard approach in audio EQ morphing. The log-domain frequency interpolation is proven in O-Formant's VowelMorpher. The body size formula is directly from ARCHITECTURE.md.

---

## Body Size Scaling Formula

From ARCHITECTURE.md line 450:
```
f_scaled = f_base * pow(2.0, (0.5 - bodySize) * 3.0)
```

| bodySize | Multiplier | Musical Meaning |
|----------|-----------|-----------------|
| 0.0 (small) | 2.83x | Violin-sized body, ~1.5 octaves up |
| 0.25 | 1.68x | Small viola |
| 0.5 (medium) | 1.0x | Baseline (no shift) |
| 0.75 | 0.595x | Cello-range |
| 1.0 (large) | 0.354x | Bass-range, ~1.5 octaves down |

Total range: ~3 octaves (1.5 up + 1.5 down from center).

**Important:** After scaling, clamp each frequency to `[20, sampleRate * 0.45]` to avoid `makePeakFilter` assertions. At 44.1kHz, Nyquist limit is 19845 Hz. The highest preset frequency is Glass mode 8 at 9600 Hz; at bodySize=0: 9600 * 2.83 = 27168 Hz which exceeds Nyquist. The clamp prevents this.

**Confidence:** HIGH -- formula is locked in ARCHITECTURE.md.

---

## Gain Normalization

### The Problem

Different presets have vastly different total energy. The Membrane preset has peaks at +12, +8, +5, +3, +6, +4, +3, +2 dB. The Metal preset has peaks at +8, +6, +5, +4, +7, +5, +3, +2 dB. When morphing between them, the total output level could jump.

### Approach: Per-Preset Normalization Factor

Compute a normalization factor for each preset based on the sum of linear gains:

```cpp
float computePresetGainSum (const ModePreset& preset)
{
    float sum = 0.0f;
    for (int i = 0; i < 8; ++i)
        sum += juce::Decibels::decibelsToGain (preset.gainDb[i]);
    return sum;
}

// At initialization:
float gainSums[NUM_PRESETS];
for (int i = 0; i < NUM_PRESETS; ++i)
    gainSums[i] = computePresetGainSum (presets[i]);

// Reference: normalize relative to wood preset (the "standard" body)
float referenceSum = gainSums[1]; // wood

// During morph: compute interpolated gain sum, apply compensation
float morphedSum = gainSumA * (1.0f - t) + gainSumB * t;
float normGain = referenceSum / morphedSum;
```

Apply `normGain` as a scalar multiplier to the summed body resonator output. This keeps the overall level roughly constant regardless of material position.

**Alternative (simpler):** Use a fixed normalization divisor. Since 8 parallel peaking filters are summing, and each has a peak gain of 2-14 dB, the worst-case sum could be very large at resonant frequencies. A simpler approach is to divide the output by a fixed number (e.g., 4.0) that was tuned by ear.

**Recommendation:** Start with the per-preset normalization approach. It is more principled and handles the morph correctly. If it sounds unnatural (loss of dynamics at resonant peaks), switch to a fixed divisor tuned by ear.

### Signal Path Integration

The body resonator processes mono input and produces mono output. The wet/dry mix is NOT needed here (unlike O-Lyrica where bodyResonance amount was variable). In O-Bowed, the body resonator is ALWAYS in the signal path (it IS the body). The BODY_MATERIAL parameter morphs between body characters; it does not fade between dry and wet.

However, a small amount of dry signal mixed in prevents the sound from becoming too "colored" at extreme resonances. The O-Lyrica pattern of `dryAmount = 1.0 - (amount * MAX_DRY_REDUCTION)` works well. For O-Bowed, a fixed mix ratio is appropriate:

```cpp
// Process body resonator
float resonant = processParallelFilters (input);
resonant *= normGain;

// Mix: 60% resonant body + 40% dry string signal
// This preserves string attack character while adding body coloring
float output = input * 0.4f + resonant * 0.6f;
```

The exact ratio should be tuned by ear.

**Confidence:** MEDIUM -- the normalization formula is sound but the exact mix ratio needs ear-tuning.

---

## Stereo Decorrelation Strategy

### The Challenge

The body resonator outputs a mono signal (one string voice). The stereo width processor needs L/R difference to work with. A mono signal duplicated to both channels has zero side content -- M/S width processing does nothing.

### Recommended Approach: Allpass Decorrelator on R Channel

Apply a 2nd-order allpass filter to the right channel only. The allpass has unity magnitude response (no coloring) but frequency-dependent phase shift. This creates L/R decorrelation -- the signals are different enough for M/S processing to create width, but identical in spectral content.

```cpp
// Declaration
juce::dsp::IIR::Filter<float> decorrelator;

// Preparation
void prepare (double sampleRate, int maxBlockSize)
{
    juce::dsp::ProcessSpec spec { sampleRate, uint32 (maxBlockSize), 1 };
    decorrelator.prepare (spec);

    // 2nd-order allpass at ~800 Hz, Q = 0.7
    // This frequency gives good decorrelation across the audible range
    // Lower frequency = more decorrelation in the low-mids
    decorrelator.coefficients =
        juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, 800.0f, 0.7f);
}

// Processing (called per sample or per block)
// Input: mono sample from body resonator
// Output: L and R samples
void processStereo (float monoInput, float& outL, float& outR)
{
    outL = monoInput;
    outR = decorrelator.processSample (monoInput);
}
```

### Width Processing (Mid-Side)

After decorrelation, apply mid-side width control:

```cpp
void applyWidth (float& left, float& right, float widthFactor)
{
    // Encode to mid-side
    float mid  = (left + right) * 0.5f;
    float side = (left - right) * 0.5f;

    // Apply width
    // widthFactor: 0.0 = mono, 1.0 = natural, 2.0 = wide
    side *= widthFactor;

    // Decode back to L/R
    left  = mid + side;
    right = mid - side;
}
```

**WIDTH parameter mapping:**
| WIDTH value | widthFactor | Effect |
|-------------|-------------|--------|
| 0.0 | 0.0 | Mono (L=R=mid) |
| 1.0 | 1.0 | Natural decorrelation width |
| 2.0 | 2.0 | Exaggerated width (side boosted) |

### Where in the Signal Chain

Per the ARCHITECTURE.md processing order (Section "Processing Order Requirements"):
1. Voice produces mono sample via waveguide
2. Body resonator processes mono -> mono
3. Output level applied
4. **Stereo width is LAST** (operates on final stereo signal)

The decorrelator and width processor should be at the **processor level** (in `processBlock`), not per-voice. Rationale:
- All voices sum into a single stereo buffer
- The decorrelator needs consistent state (one allpass instance, not 8 for 8 voices)
- Width is a global effect, not per-voice

**Signal flow in processBlock:**
```
voices -> mono sum -> body resonator -> output gain
       -> decorrelate (L=mono, R=allpass(mono))
       -> M/S width processing
       -> stereo output
```

Wait -- there is a complication. Currently voices write directly to the output buffer via `addSample`. The body resonator needs to process the summed voice output. This means we need a processing step AFTER `synthesiser.renderNextBlock`.

**Revised processBlock flow:**
```cpp
void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    buffer.clear();

    // 1. Render voices into buffer (mono duplicated to both channels)
    synthesiser.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    // 2. Process body resonator on channel 0 (mono)
    //    This operates on the summed voice output
    auto* channelData = buffer.getWritePointer (0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        channelData[i] = bodyResonator.process (channelData[i]);

    // 3. Copy processed mono to R channel via decorrelator
    auto* leftData  = buffer.getWritePointer (0);
    auto* rightData = buffer.getWritePointer (1);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        rightData[i] = decorrelator.processSample (leftData[i]);
        // leftData[i] stays as-is
    }

    // 4. Apply stereo width
    float w = widthParam->load();
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float mid  = (leftData[i] + rightData[i]) * 0.5f;
        float side = (leftData[i] - rightData[i]) * 0.5f;
        side *= w;
        leftData[i]  = mid + side;
        rightData[i] = mid - side;
    }
}
```

**Important consideration:** Currently, BowedStringVoice writes the same mono sample to ALL channels via `addSample`. Once we add stereo processing at the processor level, the voice should only write to channel 0, and the processor handles stereo creation. OR we keep the voice writing to both channels and overwrite channel 1 in step 3. The second approach is simpler and avoids changing the existing voice code.

**Confidence:** HIGH for the allpass decorrelator approach. This is a standard technique used in reverb pre-decorrelation and spatial audio. The `makeAllPass` API is verified in local JUCE source.

---

## Thread Safety Pattern

### The O-Lyrica Atomic Flag Pattern (Verified Working)

From `BodyResonance.h/.cpp` (O-Lyrica v1.32.6):

```cpp
// Member variables
std::atomic<bool> filterUpdatePending { false };
std::atomic<float> pendingBodyMaterial { 0.4f };
std::atomic<float> pendingBodySize { 0.5f };

// Called from message thread (via APVTS listener or voice updateParams)
void setParameters (float material, float size)
{
    bool needsUpdate = false;

    if (std::abs (pendingBodyMaterial.load (std::memory_order_relaxed) - material) > 0.001f)
    {
        pendingBodyMaterial.store (material, std::memory_order_relaxed);
        needsUpdate = true;
    }
    if (std::abs (pendingBodySize.load (std::memory_order_relaxed) - size) > 0.001f)
    {
        pendingBodySize.store (size, std::memory_order_relaxed);
        needsUpdate = true;
    }

    if (needsUpdate)
        filterUpdatePending.store (true, std::memory_order_release);
}

// Called at start of process() on audio thread
void applyPendingFilterUpdates()
{
    if (filterUpdatePending.load (std::memory_order_acquire))
    {
        float material = pendingBodyMaterial.load (std::memory_order_relaxed);
        float size = pendingBodySize.load (std::memory_order_relaxed);

        // Compute morphed parameters
        // Call makePeakFilter for each mode
        // Assign to filter.coefficients (safe: we're on audio thread)

        filterUpdatePending.store (false, std::memory_order_relaxed);
    }
}

float process (float input)
{
    applyPendingFilterUpdates();  // Check at start of every process call
    // ... filter processing ...
}
```

**Why this is safe:**
- `filter.coefficients` is a `ReferenceCountedObjectPtr`. JUCE docs say: "It's up to the caller to ensure that these coefficients are modified in a thread-safe way."
- By only assigning `filter.coefficients = makePeakFilter(...)` on the audio thread (inside `applyPendingFilterUpdates` which is called from `process`), we guarantee no data race.
- The atomic flag ensures the message thread can signal the audio thread without locks.
- Memory ordering: `release` on store guarantees the pending parameter values are visible before the flag. `acquire` on load guarantees we see the latest parameter values when we read the flag.

**Alternative: APVTS atomic reads directly.** Since `BowedStringVoice::updateParametersFromAPVTS()` already reads bodyMaterial and bodySize atomically from APVTS every block, we could skip the separate atomic pattern and just pass the values to the body resonator directly. The body resonator would then check if values changed and recalculate. This is simpler and matches the existing Phase 3.1 pattern.

**Recommended:** Use the simpler approach -- read from APVTS in `updateParametersFromAPVTS`, pass to body resonator, let body resonator check-and-recalculate internally. This is consistent with how brightness/bowPosition/infiniteSustain already work in Phase 3.1.

**HOWEVER** -- the body resonator sits at the processor level (post-voice-summing), not per-voice. So the APVTS reading should happen in `processBlock`, not in the voice. This is a minor architectural note the planner should account for.

**Confidence:** HIGH -- atomic flag pattern is proven in O-Lyrica, and the simpler APVTS-read approach is proven in Phase 3.1.

---

## Architecture Patterns

### Recommended New File Structure

```
plugins/O-Bowed/Source/
  |-- DSP/
  |   |-- WaveguideString.h/.cpp     (existing)
  |   |-- BowModel.h/.cpp            (existing)
  |   |-- HyperbolicFriction.h       (existing)
  |   |-- BodyResonator.h            (NEW - class declaration)
  |   |-- BodyResonator.cpp          (NEW - morph, filter update, process)
  |   |-- StereoWidthProcessor.h     (NEW - header-only, small class)
```

### BodyResonator Class Design

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

class BodyResonator
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Called from processBlock with current APVTS values
    void setMaterial (float material);   // 0.0 - 1.0
    void setSize (float size);           // 0.0 - 1.0

    // Per-sample processing (parallel biquad bank)
    float process (float input);

private:
    static constexpr int NUM_MODES = 8;
    static constexpr int NUM_PRESETS = 4;

    struct ModePreset {
        float freq[NUM_MODES];
        float q[NUM_MODES];
        float gainDb[NUM_MODES];
    };

    static const ModePreset presets[NUM_PRESETS];

    // Filter bank
    std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModes;

    // Current state
    double currentSampleRate = 44100.0;
    float currentMaterial = -1.0f;   // -1 forces initial update
    float currentSize = -1.0f;

    // Normalization
    float normGain = 1.0f;
    static constexpr float DRY_MIX = 0.4f;    // dry string signal
    static constexpr float WET_MIX = 0.6f;    // body resonance

    void updateCoefficients();
};
```

### StereoWidthProcessor Class Design

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>

class StereoWidthProcessor
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Process a stereo buffer in-place
    // Applies decorrelation (creates stereo from mono) + width
    void processBlock (juce::AudioBuffer<float>& buffer, float widthFactor);

private:
    juce::dsp::IIR::Filter<float> decorrelator;
};
```

### ProcessBlock Integration

The body resonator and stereo width processor are processor-level effects, NOT per-voice. They should be members of `OBowedAudioProcessor` and called after `synthesiser.renderNextBlock`.

```cpp
// In PluginProcessor.h -- add members:
BodyResonator bodyResonator;
StereoWidthProcessor stereoWidth;

// In prepareToPlay:
bodyResonator.prepare (sampleRate, samplesPerBlock);
stereoWidth.prepare (sampleRate, samplesPerBlock);

// In processBlock (after synthesiser.renderNextBlock):
// 1. Read body params from APVTS
float material = parameters.getRawParameterValue ("bodyMaterial")->load();
float size     = parameters.getRawParameterValue ("bodySize")->load();
float width    = parameters.getRawParameterValue ("width")->load();

bodyResonator.setMaterial (material);
bodyResonator.setSize (size);

// 2. Apply body resonator to channel 0 (mono)
auto* data = buffer.getWritePointer (0);
for (int i = 0; i < numSamples; ++i)
    data[i] = bodyResonator.process (data[i]);

// 3. Apply stereo decorrelation + width
stereoWidth.processBlock (buffer, width);
```

**Confidence:** HIGH -- follows proven O-Lyrica pattern with appropriate adaptations for the O-Bowed signal path.

---

## Preset Coefficient Banks

From ARCHITECTURE.md, 4 morph presets with 8 modes each:

```cpp
static const BodyResonator::ModePreset BodyResonator::presets[4] = {
    // Membrane (erhu-like): odd-harmonic resonances
    {{ 2000, 6000, 10000, 14000, 600, 1200, 3500, 8000 },
     { 8, 6, 4, 3, 5, 6, 4, 3 },
     { 12, 8, 5, 3, 6, 4, 3, 2 }},

    // Wood (violin): A0 air, B1-, B1+, bridge hill
    {{ 272, 462, 551, 2500, 1200, 3200, 6000, 800 },
     { 12, 10, 10, 3, 5, 4, 2, 8 },
     { 10, 14, 12, 8, 4, 3, 2, 6 }},

    // Metal: dense inharmonic modes
    {{ 440, 1123, 1872, 3100, 680, 1560, 2400, 5200 },
     { 25, 20, 15, 12, 18, 14, 10, 8 },
     { 8, 6, 5, 4, 7, 5, 3, 2 }},

    // Glass: sparse tuned resonances
    {{ 800, 2400, 4800, 7200, 1200, 3600, 6000, 9600 },
     { 30, 25, 20, 15, 28, 22, 18, 12 },
     { 10, 8, 6, 4, 9, 7, 5, 3 }}
};
```

Note: The ARCHITECTURE.md also has a Wood (cello) preset with frequencies `95, 175, 570, 1500, 350, 700, 2000, 4000`. This is a separate reference for when BODY_SIZE is set to "large." The morph uses 4 anchor presets (not 5). The BODY_SIZE parameter handles the violin-vs-cello distinction via frequency scaling.

**Confidence:** HIGH -- data directly from ARCHITECTURE.md.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Peaking EQ biquad | Manual biquad coefficient derivation | `IIR::Coefficients<float>::makePeakFilter()` | Audio EQ Cookbook implementation built into JUCE, handles all edge cases |
| Allpass filter | Custom allpass implementation | `IIR::Coefficients<float>::makeAllPass()` | Built into JUCE, handles coefficient computation correctly |
| dB-to-linear conversion | `pow(10, dB/20)` | `juce::Decibels::decibelsToGain()` | Handles edge cases (very low dB, denormals) |
| Thread-safe parameter passing | Mutex or lock-free queue | Atomic flag pattern (O-Lyrica proven) | Simple, proven, zero-allocation, lock-free |

---

## Common Pitfalls

### Pitfall 1: makePeakFilter gainFactor Confusion

**What goes wrong:** Passing dB values directly to `makePeakFilter` instead of linear gain. +12 dB as gainFactor=12.0 would be interpreted as a massive 21.6 dB boost (sqrt(12) = 3.46, applied as A^2 in the biquad formula).

**Why it happens:** The parameter name "gainFactor" sounds like it could be dB. The ARCHITECTURE.md specifies gains in dB.

**How to avoid:** Always convert: `float gain = juce::Decibels::decibelsToGain(gainDb);`

**Warning signs:** Body resonator sounds extremely aggressive/distorted, or sounds too subtle.

### Pitfall 2: Frequency Exceeds Nyquist After Size Scaling

**What goes wrong:** Glass preset mode 8 at 9600 Hz with bodySize=0 scales to 9600 * 2.83 = 27168 Hz. At 44.1kHz sample rate, Nyquist is 22050 Hz. `makePeakFilter` will jassert and produce garbage.

**Why it happens:** The 3-octave body size range combined with high preset frequencies.

**How to avoid:** Clamp ALL scaled frequencies: `freq = jlimit(20.0f, float(sampleRate * 0.45), freq);` The 0.45 factor (instead of 0.5) provides margin.

**Warning signs:** Debug builds assert. Release builds produce crackles or digital noise on extreme body size settings.

### Pitfall 3: Raw Coefficient Interpolation Instability

**What goes wrong:** If someone tries to interpolate the raw b0/b1/b2/a1/a2 coefficients between two filter presets, intermediate positions can produce unstable filters (poles outside unit circle).

**Why it happens:** The mapping from physical parameters to biquad coefficients is nonlinear. Linear interpolation in coefficient space does not preserve stability.

**How to avoid:** ALWAYS interpolate the parameters (freq, Q, gain) and call makePeakFilter. Never lerp raw coefficients.

**Warning signs:** Sudden oscillation or explosion at certain material morph positions (not 0 or 1, but in between).

### Pitfall 4: Body Resonator Feedback at High Q + High Gain

**What goes wrong:** The Glass preset has Q values up to 30 and gains up to +10 dB. Eight such filters in parallel, all boosting resonant peaks, could produce very high output levels at specific frequencies.

**Why it happens:** Parallel topology sums all filter outputs. If input has energy at multiple resonant frequencies, all 8 filters boost simultaneously.

**How to avoid:** The gain normalization system (referenceSum / morphedSum) prevents this. Also, the hard-clip at +/-2.0 in the voice provides a safety net. Add a safety limiter after body resonator output if needed.

**Warning signs:** Output clips when playing certain notes that excite multiple body modes simultaneously.

### Pitfall 5: Decorrelator State After Voice-Off

**What goes wrong:** The allpass decorrelator has internal state. If all voices go silent, the decorrelator state decays to zero normally. But if a voice starts playing again, the decorrelator transient from "clean state" sounds different from "mid-stream." This is usually inaudible but can cause a subtle stereo image shift on note attack.

**How to avoid:** Leave the decorrelator state alone (don't reset on voice silence). The allpass settles within a few samples. This is a non-issue in practice.

### Pitfall 6: Voice Writes to Both Channels Before Processor Stereo

**What goes wrong:** Currently `BowedStringVoice::renderNextBlock` writes the same sample to ALL channels. If we process body resonator on channel 0 only, channel 1 still has the un-resonated signal until we overwrite it.

**Why it happens:** Phase 3.1 design assumed mono duplication for stereo output.

**How to avoid:** Two options:
1. Change voice to write channel 0 only (breaking change, more invasive)
2. Keep voice writing both channels, then overwrite channel 1 in stereo processing (simpler)

**Recommendation:** Option 2. In `processBlock`, after body resonator processes channel 0, the stereo width processor overwrites channel 1 with the decorrelated signal. The original channel 1 data (which was identical to channel 0 pre-resonator) is discarded.

---

## Code Examples

### Complete BodyResonator::process (per-sample)

```cpp
float BodyResonator::process (float input)
{
    // Sum parallel filter outputs
    float resonant = 0.0f;
    for (auto& filter : bodyModes)
        resonant += filter.processSample (input);

    // Apply normalization
    resonant *= normGain;

    // Mix dry + wet
    return input * DRY_MIX + resonant * WET_MIX;
}
```

### Complete StereoWidthProcessor::processBlock

```cpp
void StereoWidthProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          float widthFactor)
{
    if (buffer.getNumChannels() < 2)
        return;

    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        // Step 1: Create decorrelated R channel from L (mono body resonator output)
        right[i] = decorrelator.processSample (left[i]);

        // Step 2: Mid-side width processing
        float mid  = (left[i] + right[i]) * 0.5f;
        float side = (left[i] - right[i]) * 0.5f;

        side *= widthFactor;

        left[i]  = mid + side;
        right[i] = mid - side;
    }
}
```

### Normalization Calculation

```cpp
void BodyResonator::computeNormGain (const ModeParams* morphedParams)
{
    // Sum linear gains of morphed preset
    float morphedSum = 0.0f;
    for (int i = 0; i < NUM_MODES; ++i)
        morphedSum += juce::Decibels::decibelsToGain (morphedParams[i].gainDb);

    // Reference sum (wood preset -- the "standard" body)
    static float referenceSum = -1.0f;
    if (referenceSum < 0.0f)
    {
        referenceSum = 0.0f;
        for (int i = 0; i < NUM_MODES; ++i)
            referenceSum += juce::Decibels::decibelsToGain (presets[1].gainDb[i]);
    }

    normGain = (morphedSum > 0.001f) ? (referenceSum / morphedSum) : 1.0f;
}
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Single wood type for body | Morphable multi-material presets | Modern physical modeling ~2015+ | Much richer timbral palette |
| Raw coefficient lerp | Parameter interpolation + recalc | Audio EQ community consensus | Guarantees stability during morphs |
| Mono output | Allpass decorrelation + M/S width | Standard since early 2000s | Natural stereo without comb filtering |
| Convolution IR body | Parallel biquad bank | Always valid for real-time | Lower latency, morphable, lower CPU |

---

## Open Questions

1. **Dry/wet ratio tuning**
   - What we know: O-Lyrica uses 0.6 wet, 0.7 wet gain multiplier with variable amount
   - What's unclear: Optimal fixed ratio for O-Bowed where body is always active
   - Recommendation: Start with 0.4 dry / 0.6 wet, tune by ear after first build

2. **Decorrelator frequency**
   - What we know: 800 Hz is a common choice for allpass decorrelation
   - What's unclear: Whether 800 Hz is optimal for bowed string timbre
   - Recommendation: Start with 800 Hz Q=0.7. If stereo image sounds "phasey" or thin, try 500 Hz or 1200 Hz.

3. **Body resonator placement: per-voice or processor-level?**
   - What we know: ARCHITECTURE.md says "Multi-string: each active string has its own waveguide + friction junction, shares body resonator" (line 276). This implies processor-level (shared).
   - What's unclear: For Phase 3.2 (single string), per-voice would be simpler since there's only one voice sounding at once
   - Recommendation: Implement at processor level now. This is future-proof for Phase 3.3 multi-string where ALL strings share one body resonator (like a real instrument). It also avoids duplicating 8 filters per voice (8 voices x 8 filters = 64 filters vs 8 filters shared).

---

## Sources

### Primary (HIGH confidence)
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_IIRFilter.h` -- makePeakFilter signature, gainFactor semantics, coefficients thread safety comment
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_IIRFilter.cpp` -- makePeakFilter implementation (lines 260-278), allpass implementation
- `plugins/O-Lyrica/Source/DSP/BodyResonance.h/.cpp` -- Proven parallel biquad bank pattern, thread-safe atomic flag pattern, frequency scaling, makePeakFilter usage with linear gain
- `plugins/O-Bowed/.planning/research/ARCHITECTURE.md` -- Body resonator spec (lines 107-138), morph algorithm (lines 440-453), body size formula, stereo width spec (lines 183-198), processing order (lines 505-535), thread boundaries (lines 537-559)
- `plugins/O-Bowed/Source/BowedStringVoice.cpp` -- Current voice implementation (renderNextBlock mono output, updateParametersFromAPVTS pattern)
- `plugins/O-Bowed/Source/PluginProcessor.cpp` -- Current processBlock flow, APVTS parameter IDs for bodyMaterial/bodySize/width already defined

### Secondary (MEDIUM confidence)
- O-Formant VowelMorpher log-domain frequency interpolation pattern (verified working in production)
- Allpass decorrelation as standard stereo imaging technique (well-established in audio DSP literature)

### Tertiary (LOW confidence)
- Dry/wet ratio 0.4/0.6 -- reasonable starting point but needs ear-tuning
- Decorrelator at 800 Hz Q=0.7 -- common default but may need adjustment for bowed string timbre

## Metadata

**Confidence breakdown:**
- JUCE API patterns: HIGH -- verified from local JUCE 8.0.4 source headers + proven O-Lyrica code
- Coefficient morphing strategy: HIGH -- parameter interpolation is the established safe approach
- Body size formula: HIGH -- directly from locked ARCHITECTURE.md
- Gain normalization: MEDIUM -- formula is sound but exact ratios need tuning
- Stereo decorrelation: HIGH -- standard technique, JUCE allpass API verified
- Thread safety: HIGH -- O-Lyrica v1.32.6 pattern proven in production

**Research date:** 2026-04-05
**Valid until:** 2026-05-05 (stable domain, JUCE 8 API is mature)
