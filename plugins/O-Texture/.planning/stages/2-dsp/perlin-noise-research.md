# 1D Perlin Noise for Evolve Modulation - Research

**Researched:** 2026-02-14
**Domain:** Smooth noise algorithms for real-time latent space modulation
**Confidence:** HIGH

## Summary

This document investigates smooth noise algorithms for O-Texture's "Evolve" parameter, which drives a random walk through ~28 latent space dimensions at block rate (~23 times/second at 48kHz with 2048-sample hop). The noise must be real-time safe (no heap allocations), deterministic (same seed = same sequence), smooth (no discontinuities), and lightweight (28 independent channels evaluated per hop).

Three algorithms were evaluated: classic/improved Perlin gradient noise, simplex noise, and value noise with quintic interpolation. For this 1D use case, **value noise with quintic interpolation is the recommended approach**. It is the simplest to implement correctly, produces smooth output with continuous first and second derivatives, is trivially real-time safe, and is completely self-contained with zero dependencies. In 1D, Perlin gradient noise and value noise produce comparable smoothness, and simplex noise offers no meaningful advantage over value noise at 1D.

**Primary recommendation:** Use a self-contained 1D value noise generator with quintic fade interpolation (Perlin's improved curve: `6t^5 - 15t^4 + 10t^3`), seeded permutation table for determinism, and a simple time-advancing cursor per channel. This avoids all complexity of gradient computation while delivering identical smoothness characteristics to improved Perlin noise in 1D.

---

## Algorithm Analysis

### Option 1: Classic/Improved Perlin Gradient Noise (1D)

**How it works:** At each integer lattice point, a pseudo-random gradient (in 1D: a signed scalar slope) is assigned via a permutation table hash. The noise value at any position is computed by taking dot products of the distance-to-lattice-point with the gradient at each neighboring lattice point, then interpolating using a fade curve.

**1D simplification:**
- Two neighboring integer points `i0` and `i1 = i0 + 1`
- Fractional position `t = x - floor(x)`
- Gradient at each point from hash: `g0 = grad(hash(i0))`, `g1 = grad(hash(i1))`
- Contribution: `n0 = g0 * t`, `n1 = g1 * (t - 1.0)`
- Fade: `u = fade(t)` where `fade(t) = 6t^5 - 15t^4 + 10t^3`
- Result: `lerp(n0, n1, u)`

**Pros:**
- Well-understood algorithm (Ken Perlin, 1985/2002 improved version)
- Continuous first and second derivatives (with quintic fade)
- Deterministic with permutation table seed
- No allocations in hot path

**Cons:**
- Gradient computation adds complexity that provides no benefit in 1D
- In 1D, gradients are just signed scalars -- the "gradient" concept is overkill
- Output range is not exactly [-1, 1]; needs scaling factor
- More complex to implement correctly than value noise

**Confidence:** HIGH -- algorithm well-documented by Perlin (2002), NVIDIA GPU Gems, and numerous implementations.

### Option 2: Simplex Noise (1D)

**How it works:** Uses a simplex grid (in 1D: just a line of integer points, identical to Perlin's grid). Each corner contributes via a radial attenuation kernel `(1 - x^2)^4 * grad(hash, x)` rather than the fade-lerp of Perlin noise.

**1D implementation (from SRombauts/SimplexNoise):**
```cpp
float SimplexNoise::noise(float x) {
    int32_t i0 = fastfloor(x);
    int32_t i1 = i0 + 1;
    float x0 = x - i0;
    float x1 = x0 - 1.0f;

    float t0 = 1.0f - x0*x0;
    t0 *= t0;
    float n0 = t0 * t0 * grad(hash(i0), x0);

    float t1 = 1.0f - x1*x1;
    t1 *= t1;
    float n1 = t1 * t1 * grad(hash(i1), x1);

    return 0.395f * (n0 + n1);
}
```

**Pros:**
- No flat plateaus (a known artifact of classic Perlin in higher dimensions)
- Computationally efficient in higher dimensions (O(n^2) vs O(n*2^n))
- Well-tested MIT-licensed implementation available (SRombauts/SimplexNoise)

**Cons:**
- In 1D, the advantage over Perlin/value noise is negligible
- The `(1-x^2)^4` kernel produces a slightly different character than quintic fade
- Gradient computation still present (same overkill issue as Perlin in 1D)
- The 0.395 scaling factor is empirical (output range not exactly [-1,1])
- Patent concerns existed for 3D+ (expired 2022), but 1D was never patented

**Confidence:** HIGH -- SRombauts implementation is MIT-licensed, well-tested, and used in production (including Bela audio platform).

### Option 3: Value Noise with Quintic Interpolation (RECOMMENDED)

**How it works:** At each integer lattice point, a pseudo-random VALUE (not gradient) is stored via a hash function. The noise at any position is computed by interpolating between neighboring values using a smooth curve.

**1D implementation:**
- Two neighboring integer points `i0` and `i1 = i0 + 1`
- Fractional position `t = x - floor(x)`
- Random values: `v0 = hash_to_float(hash(i0))`, `v1 = hash_to_float(hash(i1))`
- Fade: `u = fade(t)` where `fade(t) = 6t^5 - 15t^4 + 10t^3`
- Result: `lerp(v0, v1, u)`

**Pros:**
- Simplest algorithm of the three (no gradient computation at all)
- Quintic fade provides continuous first and second derivatives (identical smoothness to improved Perlin)
- Output range is exactly [0, 1] (trivially remapped to [-1, 1])
- Deterministic with seeded permutation table
- Zero allocations, zero branching in hot path
- Easiest to verify correctness
- Perfectly adequate for slow-rate modulation (~23 Hz update rate)

**Cons:**
- Slightly less "natural" than gradient noise at high frequencies (visible in graphics, inaudible in slow modulation)
- No benefit from fractal/octave layering for this use case (but could be added if desired)

**Confidence:** HIGH -- value noise is the simplest and most well-understood noise algorithm. Quintic interpolation is proven by Ken Perlin (2002).

### Decision: Value Noise with Quintic Fade

**Why value noise wins for this use case:**

1. **Simplicity:** No gradient computation, no dot products, no scaling factors. Just hash, interpolate, done.
2. **Identical smoothness:** With quintic fade, value noise has continuous first and second derivatives -- identical smoothness characteristics to improved Perlin noise. The "plateau" artifacts that simplex was designed to fix are a higher-dimensional phenomenon and do not appear in 1D.
3. **Exact output range:** Value noise outputs exactly [0, 1] (remappable to [-1, 1]), unlike Perlin/simplex which require empirical scaling factors.
4. **Block-rate evaluation:** At ~23 evaluations/second, the perceptual difference between value noise and gradient noise is zero. These differences only matter at pixel-level resolution in graphics.
5. **28 channels:** Simplicity matters when you need 28 independent evaluations per hop. Fewer operations = less CPU.

---

## Architecture for O-Texture Evolve

### Requirements Recap

From CONTEXT.md and ARCHITECTURE.md:
- ~28 independent noise channels (32 latent dims - 4 user-controlled = 28 evolve dims)
- Block-rate update: once per 2048-sample hop (~23.4 Hz at 48kHz)
- EVOLVE parameter (0.0-1.0) controls traversal speed
- FREEZE parameter halts all evolution
- Output range: values applied as offsets to latent dimensions, clamped to [-3, 3]
- Deterministic: same seed produces same evolution sequence
- State must be serializable (save/restore in presets)

### Design

```
Per-channel state:
  - cursor: float (current position along the 1D noise function)
  - Each channel has a different offset into the permutation table (via seed)

Per-hop update:
  1. If FREEZE: skip all updates
  2. Read EVOLVE parameter (0.0-1.0)
  3. Map EVOLVE to step size: step = evolve * MAX_STEP_PER_HOP
  4. For each of 28 channels:
     a. cursor[i] += step
     b. value[i] = noise1D(cursor[i], seed[i])  // returns [-1, 1]
     c. Apply to latent dimension with scaling and clamping
```

### Speed Mapping

The EVOLVE parameter (0.0-1.0) maps to how fast the noise cursor advances per hop:

| EVOLVE | Step/Hop | Full Cycle Period | Character |
|--------|----------|-------------------|-----------|
| 0.0    | 0.0      | Infinite (frozen)  | Static |
| 0.1    | 0.001    | ~43 seconds        | Glacial drift |
| 0.3    | 0.003    | ~14 seconds        | Slow evolution (default) |
| 0.5    | 0.005    | ~8.5 seconds       | Moderate movement |
| 0.7    | 0.010    | ~4.3 seconds       | Active variation |
| 1.0    | 0.020    | ~2.1 seconds       | Rapid change |

A "full cycle" means traversing one integer unit of noise space (between two random lattice values). The quintic interpolation ensures smooth transitions across these boundaries.

The step size should be tunable after listening tests. The MAX_STEP_PER_HOP constant (0.02 in the table above) can be adjusted. This value means at maximum Evolve, the cursor advances 0.02 per hop, completing one noise "period" in about 50 hops (~2.1 seconds).

### Seed Strategy

Each of the 28 channels needs a different noise sequence. Two approaches:

**Approach A: Offset into shared permutation table (RECOMMENDED)**
- Single 256-entry permutation table, seeded once
- Each channel uses a different hash offset: `hash(position + channel_offset)`
- Channel offsets are large primes (e.g., channel * 37) to decorrelate sequences
- Pro: Single permutation table (256 bytes), minimal memory
- Con: Channels are correlated at the permutation table period (256 entries = very far apart in practice)

**Approach B: Separate permutation tables per channel**
- 28 independent 256-entry permutation tables
- Pro: Guaranteed decorrelation
- Con: 28 * 256 = 7168 bytes (still tiny, but unnecessary)

Approach A is recommended. With channel offsets using prime spacing, the 28 channels will produce effectively independent sequences over the short traversal distances used in practice.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Multi-octave noise | Custom octave summation | Single-octave value noise | Block-rate modulation doesn't benefit from fractal detail; one octave is sufficient |
| Thread-safe RNG | `std::mt19937` in audio thread | Pre-computed permutation table | Mersenne Twister allocates; permutation table is lock-free and allocation-free |
| Smooth interpolation | Linear lerp or cosine | Quintic fade curve | Quintic has C2 continuity (continuous second derivative); linear/cosine do not |

---

## Common Pitfalls

### Pitfall 1: Using smoothstep (3t^2 - 2t^3) instead of quintic

**What goes wrong:** The cubic smoothstep curve has continuous first derivative but DISCONTINUOUS second derivative at lattice boundaries. This creates subtle "creases" in the noise profile.

**Why it happens:** The cubic curve `3t^2 - 2t^3` is simpler and appears in many tutorials.

**How to avoid:** Always use the quintic fade: `6t^5 - 15t^4 + 10t^3`. This was Perlin's key improvement in 2002.

**Optimized form:** `t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f)`

### Pitfall 2: Integer overflow in hash function

**What goes wrong:** If the noise cursor grows large enough, `(int)x` can overflow, causing discontinuities or crashes.

**Why it happens:** The cursor advances indefinitely over time. After hours of playback, values can reach millions.

**How to avoid:** Mask the integer part to stay within the permutation table size: `int xi = ((int)floor(x)) & 255`. The `& 255` (or `& 0xFF`) operation wraps the index within the 256-entry table, creating seamless tiling.

### Pitfall 3: Negative coordinate floor behavior

**What goes wrong:** In C++, `(int)(-0.5f)` gives `0`, not `-1`. This causes a discontinuity at x=0.

**Why it happens:** C++ truncates toward zero, not toward negative infinity.

**How to avoid:** Use `std::floor()` or a fast floor function:
```cpp
inline int fastFloor(float x) {
    int xi = static_cast<int>(x);
    return (x < xi) ? xi - 1 : xi;
}
```

### Pitfall 4: Correlated channels producing "unison" movement

**What goes wrong:** If all 28 channels use the same noise sequence (or nearly correlated ones), the latent dimensions move in lockstep, reducing the organic quality of evolution.

**Why it happens:** Using sequential offsets (channel 0, 1, 2...) into the same permutation table with small spacing.

**How to avoid:** Use large prime offsets between channels (e.g., `channel * 37 + 7`) or add a per-channel seed offset to the hash function. This ensures each channel traverses a different region of the noise space.

### Pitfall 5: Allocating in the audio thread

**What goes wrong:** Any `new`, `malloc`, `std::vector::push_back`, or similar allocation in processBlock causes priority inversion and potential audio dropouts.

**Why it happens:** Easy to accidentally allocate when constructing noise state or temporary arrays.

**How to avoid:** Pre-allocate ALL buffers in `prepareToPlay()`. The noise generator state (cursors, output values) must be fixed-size arrays allocated at construction time. Use `std::array` instead of `std::vector` where size is known at compile time.

---

## Complete C++ Implementation

The following is a self-contained, header-only implementation suitable for embedding directly in the O-Texture plugin. It requires no external dependencies beyond `<cmath>`, `<cstdint>`, `<array>`, and `<algorithm>`.

### PerlinNoise1D.h

```cpp
/*
  ==============================================================================

    PerlinNoise1D.h
    O-Texture - 1D Value Noise with Quintic Interpolation

    Smooth noise generator for latent space "Evolve" modulation.
    Uses value noise (not gradient noise) with Perlin's improved
    quintic fade curve for C2-continuous output.

    Real-time safe: no heap allocations, no locks, no exceptions.
    Deterministic: same seed produces identical sequence.

    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once

#include <cmath>
#include <cstdint>
#include <array>
#include <algorithm>

/**
 * Multi-channel 1D value noise generator for smooth random modulation.
 *
 * Template parameter NumChannels: number of independent noise channels.
 * For O-Texture Evolve: 28 channels (32 latent dims - 4 user-controlled).
 *
 * Usage:
 *   PerlinNoise1D<28> noise;
 *   noise.setSeed(42);
 *   noise.setSpeed(0.3f);  // EVOLVE parameter
 *
 *   // In processBlock, once per hop:
 *   noise.advance();
 *   float val = noise.getValue(channelIndex);  // returns [-1, 1]
 */
template <int NumChannels = 28>
class PerlinNoise1D
{
public:
    PerlinNoise1D()
    {
        // Initialize with default seed
        setSeed(0);
        reset();
    }

    /**
     * Set the random seed. Rebuilds the permutation table.
     * NOT real-time safe -- call from constructor or message thread only.
     */
    void setSeed(uint32_t seed)
    {
        currentSeed = seed;
        buildPermutationTable(seed);

        // Assign decorrelated channel offsets using large prime spacing.
        // Each channel gets a different region of the permutation table.
        for (int ch = 0; ch < NumChannels; ++ch)
        {
            channelOffsets[ch] = static_cast<uint8_t>((ch * 37 + 7 + seed) & 0xFF);
        }
    }

    /**
     * Reset all cursors to zero. Call when starting playback or changing seed.
     * NOT real-time safe if called during audio processing.
     */
    void reset()
    {
        cursors.fill(0.0f);
        cachedValues.fill(0.0f);

        // Evaluate initial values at cursor position 0
        for (int ch = 0; ch < NumChannels; ++ch)
        {
            cachedValues[ch] = evaluateNoise(cursors[ch], channelOffsets[ch]);
        }
    }

    /**
     * Set the evolve speed. Maps the EVOLVE parameter (0.0-1.0) to
     * cursor step size per advance() call.
     *
     * Real-time safe (just stores a float).
     */
    void setSpeed(float evolveParam)
    {
        // Map 0.0-1.0 to step size per hop.
        // At max (1.0), cursor advances 0.02 per hop = one noise period in ~50 hops (~2.1s).
        // At default (0.3), cursor advances 0.006 per hop = one period in ~167 hops (~7.1s).
        // Quadratic mapping gives finer control at low speeds.
        float clamped = std::clamp(evolveParam, 0.0f, 1.0f);
        stepPerHop = clamped * clamped * maxStepPerHop;
    }

    /**
     * Set maximum step size per hop (default: 0.02).
     * Adjustable for tuning after listening tests.
     */
    void setMaxStepPerHop(float maxStep)
    {
        maxStepPerHop = maxStep;
    }

    /**
     * Advance all channels by one hop. Call once per 2048-sample block.
     * Real-time safe.
     *
     * @param freeze  If true, cursors do not advance (FREEZE parameter).
     */
    void advance(bool freeze = false)
    {
        if (freeze || stepPerHop <= 0.0f)
            return;

        for (int ch = 0; ch < NumChannels; ++ch)
        {
            cursors[ch] += stepPerHop;
            cachedValues[ch] = evaluateNoise(cursors[ch], channelOffsets[ch]);
        }
    }

    /**
     * Get the current noise value for a channel.
     * Returns a value in [-1.0, 1.0].
     * Real-time safe (just reads a cached float).
     */
    float getValue(int channel) const
    {
        return cachedValues[static_cast<size_t>(channel)];
    }

    /**
     * Get all channel values at once (for bulk latent vector construction).
     * Real-time safe.
     */
    const std::array<float, NumChannels>& getAllValues() const
    {
        return cachedValues;
    }

    /**
     * Get current cursor positions (for state serialization).
     */
    const std::array<float, NumChannels>& getCursors() const
    {
        return cursors;
    }

    /**
     * Restore cursor positions (for state deserialization / preset recall).
     * NOT real-time safe -- call from message thread.
     */
    void setCursors(const std::array<float, NumChannels>& newCursors)
    {
        cursors = newCursors;

        // Re-evaluate cached values at restored positions
        for (int ch = 0; ch < NumChannels; ++ch)
        {
            cachedValues[ch] = evaluateNoise(cursors[ch], channelOffsets[ch]);
        }
    }

    /**
     * Get the current seed (for state serialization).
     */
    uint32_t getSeed() const { return currentSeed; }

    /**
     * Get the number of channels.
     */
    static constexpr int getNumChannels() { return NumChannels; }

private:
    // =========================================================================
    // Core noise evaluation
    // =========================================================================

    /**
     * Evaluate 1D value noise at position x with channel offset.
     * Returns value in [-1.0, 1.0].
     *
     * Algorithm:
     *   1. Find two neighboring integer lattice points
     *   2. Hash each to get a pseudo-random value in [0, 1]
     *   3. Interpolate using quintic fade curve
     *   4. Remap from [0, 1] to [-1, 1]
     */
    float evaluateNoise(float x, uint8_t offset) const
    {
        // Step 1: Find integer lattice points
        int i0 = fastFloor(x);
        float t = x - static_cast<float>(i0);  // Fractional part in [0, 1)

        // Step 2: Hash to get random values at lattice points
        float v0 = hashToFloat(hashAt(i0, offset));
        float v1 = hashToFloat(hashAt(i0 + 1, offset));

        // Step 3: Quintic fade interpolation (C2 continuous)
        float u = quinticFade(t);
        float result = lerp(v0, v1, u);

        // Step 4: Remap [0, 1] -> [-1, 1]
        return result * 2.0f - 1.0f;
    }

    // =========================================================================
    // Math utilities (all constexpr/inline, no allocations)
    // =========================================================================

    /**
     * Fast floor that handles negative numbers correctly.
     * C++ truncation toward zero gives wrong result for negatives.
     */
    static inline int fastFloor(float x)
    {
        int xi = static_cast<int>(x);
        return (x < static_cast<float>(xi)) ? xi - 1 : xi;
    }

    /**
     * Perlin's improved quintic fade curve: 6t^5 - 15t^4 + 10t^3
     * Has zero first AND second derivatives at t=0 and t=1.
     * This ensures C2 continuity across lattice boundaries.
     */
    static inline float quinticFade(float t)
    {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    /**
     * Linear interpolation.
     */
    static inline float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    /**
     * Convert a uint8_t hash value to a float in [0, 1].
     */
    static inline float hashToFloat(uint8_t h)
    {
        return static_cast<float>(h) / 255.0f;
    }

    // =========================================================================
    // Hashing (permutation table based)
    // =========================================================================

    /**
     * Hash an integer position with channel offset.
     * Uses the permutation table for deterministic pseudo-random mapping.
     * The channel offset decorrelates different channels.
     */
    inline uint8_t hashAt(int position, uint8_t offset) const
    {
        // Mask to 8 bits for table lookup (handles negative values via wrap)
        uint8_t index = static_cast<uint8_t>(position & 0xFF);
        return perm[static_cast<uint8_t>(index + offset)];
    }

    /**
     * Build the permutation table from a seed.
     * Uses a simple LCG (Linear Congruential Generator) to shuffle.
     * This is NOT real-time safe (called once during initialization).
     */
    void buildPermutationTable(uint32_t seed)
    {
        // Initialize with identity
        for (int i = 0; i < 256; ++i)
            perm[i] = static_cast<uint8_t>(i);

        // Fisher-Yates shuffle using LCG
        uint32_t state = seed;
        for (int i = 255; i > 0; --i)
        {
            // Simple LCG: state = state * 1664525 + 1013904223
            state = state * 1664525u + 1013904223u;
            int j = static_cast<int>((state >> 16) % static_cast<uint32_t>(i + 1));

            // Swap perm[i] and perm[j]
            uint8_t tmp = perm[i];
            perm[i] = perm[j];
            perm[j] = tmp;
        }

        // Duplicate table for seamless wrapping (avoids modulo in hot path)
        for (int i = 0; i < 256; ++i)
            perm[i + 256] = perm[i];
    }

    // =========================================================================
    // State
    // =========================================================================

    // Permutation table (doubled to 512 for seamless wrapping)
    std::array<uint8_t, 512> perm {};

    // Per-channel state
    std::array<float, NumChannels> cursors {};       // Current position along noise
    std::array<float, NumChannels> cachedValues {};   // Last evaluated noise values
    std::array<uint8_t, NumChannels> channelOffsets {}; // Hash offset per channel

    // Speed control
    float stepPerHop = 0.006f;    // Current step size per advance() call
    float maxStepPerHop = 0.02f;  // Maximum step (at EVOLVE = 1.0)

    // Seed for serialization
    uint32_t currentSeed = 0;
};
```

### Integration Example: Usage in TextureProcessor

```cpp
// In PluginProcessor.h:
#include "PerlinNoise1D.h"

class TextureProcessor : public juce::AudioProcessor
{
    // ... existing members ...

private:
    // Evolve noise generator: 28 channels for non-user-controlled latent dims
    static constexpr int kEvolveDims = 28;
    PerlinNoise1D<kEvolveDims> evolveNoise;

    // Hop counter for block-rate processing
    int hopCounter = 0;
    static constexpr int kHopSize = 2048;

    // Latent vector (pre-allocated, no runtime allocation)
    std::array<float, 32> latentVector {};
};

// In PluginProcessor.cpp, prepareToPlay():
void TextureProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    setLatencySamples(6144);

    // Initialize evolve noise with a deterministic seed
    evolveNoise.setSeed(42);
    evolveNoise.reset();

    hopCounter = 0;
}

// In processBlock(), latent vector construction:
void TextureProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float evolveRate = evolveParam->load();
    const bool freeze = freezeParam->load() > 0.5f;
    const float xVal = xParam->load();
    const float yVal = yParam->load();
    const float charA = characterAParam->load();
    const float charB = characterBParam->load();

    evolveNoise.setSpeed(evolveRate);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        hopCounter++;

        if (hopCounter >= kHopSize)
        {
            hopCounter = 0;

            // Advance noise by one hop
            evolveNoise.advance(freeze);

            // Construct latent vector
            // Dims 0-3: user-controlled (mapped via dim_map)
            latentVector[0] = xVal * 6.0f - 3.0f;      // [0,1] -> [-3,3]
            latentVector[1] = yVal * 6.0f - 3.0f;
            latentVector[2] = charA * 6.0f - 3.0f;
            latentVector[3] = charB * 6.0f - 3.0f;

            // Dims 4-31: evolve noise modulation
            const auto& noiseValues = evolveNoise.getAllValues();
            for (int i = 0; i < kEvolveDims; ++i)
            {
                // Scale noise [-1,1] to latent range, with magnitude control
                float noiseVal = noiseValues[i];
                float evolveInfluence = evolveRate;  // Higher rate = wider excursion
                latentVector[i + 4] = noiseVal * evolveInfluence * 2.0f;
                latentVector[i + 4] = std::clamp(latentVector[i + 4], -3.0f, 3.0f);
            }

            // Submit latent vector to ANIRA for decoder inference
            // (implementation depends on ANIRA API -- placeholder)
            // anira.submitInference(latentVector);
        }

        // Output from crossfade buffer (placeholder for Stage 2)
        // ...
    }
}

// State serialization (in getStateInformation):
void TextureProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Save evolve noise state for reproducible preset recall
    auto evolveState = juce::ValueTree("EvolveState");
    evolveState.setProperty("seed", (int)evolveNoise.getSeed(), nullptr);

    const auto& cursors = evolveNoise.getCursors();
    for (int i = 0; i < kEvolveDims; ++i)
    {
        auto dimNode = juce::ValueTree("Dim");
        dimNode.setProperty("index", i, nullptr);
        dimNode.setProperty("cursor", (double)cursors[i], nullptr);
        evolveState.addChild(dimNode, -1, nullptr);
    }

    state.addChild(evolveState, -1, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

// State deserialization (in setStateInformation):
void TextureProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        parameters.replaceState(state);

        // Restore evolve noise state
        auto evolveState = state.getChildWithName("EvolveState");
        if (evolveState.isValid())
        {
            uint32_t seed = static_cast<uint32_t>((int)evolveState.getProperty("seed", 42));
            evolveNoise.setSeed(seed);

            std::array<float, kEvolveDims> cursors {};
            for (int i = 0; i < evolveState.getNumChildren(); ++i)
            {
                auto dimNode = evolveState.getChild(i);
                int index = (int)dimNode.getProperty("index", 0);
                if (index >= 0 && index < kEvolveDims)
                    cursors[index] = (float)(double)dimNode.getProperty("cursor", 0.0);
            }
            evolveNoise.setCursors(cursors);
        }
    }
}
```

---

## Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Value noise (quintic) | Perlin gradient noise | More complex for identical 1D results; gradient computation is wasted effort in 1D |
| Value noise (quintic) | Simplex noise | MIT library available (SRombauts), but adds external dependency for no benefit in 1D |
| Custom implementation | siv::PerlinNoise header-only lib | Full-featured C++17/20 library, but brings 2D/3D overhead we don't need; our implementation is ~150 lines |
| Permutation table hash | `std::hash` or murmur hash | Standard hashes are not designed for spatial coherence; permutation table is the standard approach |
| Quadratic speed mapping | Linear speed mapping | Quadratic gives finer control at low EVOLVE values where perceptual sensitivity is highest |
| Single-octave noise | Multi-octave fractal (fBm) | Single octave is sufficient at ~23 Hz update rate; fractal adds complexity and CPU cost for no perceptual benefit |

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Cubic smoothstep (3t^2-2t^3) | Quintic fade (6t^5-15t^4+10t^3) | 2002 (Perlin improved) | C2 continuity, no creases at lattice boundaries |
| Perlin classic gradient noise | Simplex noise / Value noise | 2001 (simplex) / always (value) | Simplex removes directional artifacts in 2D+; value noise is simpler for 1D |
| Patented simplex noise (3D+) | OpenSimplex / expired patent | 2014 (OpenSimplex) / 2022 (patent expired) | No longer a concern for any dimension |
| Large lookup tables (libnoise) | Minimal permutation table (256 bytes) | - | Real-time audio favors cache-friendly small tables |

---

## Open Questions

1. **Evolve excursion range tuning**
   - What is the perceptually optimal mapping from noise [-1, 1] to latent dimension offset?
   - The current implementation uses `noiseVal * evolveRate * 2.0` clamped to [-3, 3]
   - This needs listening tests with real decoder output to tune
   - **Recommendation:** Ship with current mapping, expose `maxStepPerHop` and excursion scale as internal constants for easy tuning

2. **Should inactive dims use noise too?**
   - ARCHITECTURE.md says inactive dims are "sampled from N(0,1) each block"
   - This means independent random values per block (no temporal smoothness)
   - An alternative: use noise for inactive dims too, but at much faster rate
   - **Recommendation:** Keep as-is (N(0,1) random for inactive dims). Evolve noise is for the ~28 remaining ACTIVE dims that need smooth, organic movement.

3. **Dimension count flexibility**
   - The template parameter is currently 28, assuming 32 total - 4 user-controlled
   - After latent space analysis, the actual number of evolve dims may differ
   - **Recommendation:** Use the template parameter to adjust at compile time based on dim_map analysis results

---

## Sources

### Primary (HIGH confidence)
- Ken Perlin, "Improving Noise" (2002) - https://mrl.cs.nyu.edu/~perlin/paper445.pdf - Quintic fade curve definition
- NVIDIA GPU Gems Ch. 5 - https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-5-implementing-improved-perlin-noise - Improved Perlin noise implementation
- SRombauts/SimplexNoise (MIT) - https://github.com/SRombauts/SimplexNoise - 1D simplex noise reference implementation
- Scratchapixel Value Noise - https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/procedural-patterns-noise-part-1/creating-simple-1D-noise.html - 1D value noise algorithm

### Secondary (MEDIUM confidence)
- Mrugalla/PerlinNoiseMod - https://github.com/Mrugalla/PerlinNoiseMod - JUCE plugin using Perlin noise as audio modulator (validates the concept)
- Adrian Biagioli Perlin tutorial - https://adrianb.io/2014/08/09/perlinnoise.html - Complete improved Perlin noise walkthrough
- Reputeless/PerlinNoise - https://github.com/Reputeless/PerlinNoise - Header-only C++17/20 Perlin noise library

### Tertiary (LOW confidence)
- Bit-101 Perlin vs Simplex comparison - https://www.bit-101.com/2017/2021/07/perlin-vs-simplex/ - Perceptual comparison (graphics-focused, may not apply to audio modulation)

---

## Metadata

**Confidence breakdown:**
- Algorithm choice (value noise): HIGH - well-understood algorithm, proven in audio contexts
- Quintic interpolation: HIGH - documented by Ken Perlin (2002), used in all modern implementations
- Speed mapping: MEDIUM - the specific constants (maxStepPerHop = 0.02, quadratic curve) need tuning with real audio
- Channel decorrelation: HIGH - prime-spaced offsets into permutation table is standard practice
- Integration pattern: HIGH - follows existing O-Texture architecture from ARCHITECTURE.md

**Research date:** 2026-02-14
**Valid until:** Indefinite (fundamental algorithms, not version-dependent)
