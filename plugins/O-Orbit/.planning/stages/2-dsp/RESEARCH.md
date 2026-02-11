# Stage 2: DSP - Research

> **Stage:** 2 (DSP)
> **Date:** 2026-02-10
> **Status:** Complete

---

## 1. SAF VBAP API for 2-Speaker Stereo

### Problem

SAF's 3D VBAP uses `convhull_3d_build()` for Delaunay triangulation, which requires **nVert > 3** (returns NULL for 3 or fewer vertices). This means SAF's `generateVBAPgainTable3D()` cannot handle a 2-speaker stereo layout directly.

### SAF API Reference (from source inspection)

**Key functions:**

```c
// 3D gain table generation (needs 4+ speakers)
void generateVBAPgainTable3D(
    float* ls_dirs_deg,     // Speaker dirs; FLAT: L x 2 (az, el in degrees)
    int L,                  // Number of speakers
    int az_res_deg,         // Azimuth resolution (1 degree recommended)
    int el_res_deg,         // Elevation resolution (1 degree)
    int omitLargeTriangles, // 1 = remove triangles > 120 degrees
    int enableDummies,      // 1 = add imaginary speakers to fill gaps
    float spread,           // Spreading in degrees (0 = point source)
    float** gtable,         // Output: gain table; FLAT: N_gtable x L
    int* N_gtable,          // Output: number of table entries
    int* nTriangles         // Output: number of speaker triangles
);

// For specific source directions (avoids full table generation)
void generateVBAPgainTable3D_srcs(
    float* src_dirs_deg,    // Source directions; FLAT: S x 2
    int S,                  // Number of sources
    float* ls_dirs_deg,     // Speaker positions; FLAT: L x 2
    int L,                  // Number of speakers
    int omitLargeTriangles,
    int enableDummies,
    float spread,
    float** gtable,         // Output: gain table; FLAT: S x L
    int* N_gtable,
    int* nTriangles
);
```

**Memory management:** SAF allocates output arrays with `malloc()`. Caller must `free()` them after copying.

**Gain table lookup pattern (from SPARTA panner source):**

```c
// 3D lookup:
N_azi = (int)(360.0f / aziRes + 0.5f) + 1;
aziIndex = (int)(fmodf(azimuth + 180.0f, 360.0f) / aziRes + 0.5f);
elevIndex = (int)((elevation + 90.0f) / elevRes + 0.5f);
idx3d = elevIndex * N_azi + aziIndex;
gains = vbap_gtable[idx3d * nLoudspeakers ...];

// 2D lookup:
idx2D = (int)(fmodf(azimuth + 180.0f, 360.0f) / aziRes + 0.5f);
gains = vbap_gtable[idx2D * nLoudspeakers ...];
```

### Azimuth Convention

SAF uses: **0 = front, positive = counter-clockwise** (atan2(y, x) convention). This matches O-Orbit's convention exactly (confirmed in Stage 1 discuss).

### Solution: Hybrid Approach for 2-Speaker Stereo

**For 2-3 speakers:** Use custom equal-power pair-wise panning (no SAF dependency):

```cpp
// 2-speaker: simple equal-power panning derived from azimuth
// azimuth convention: 0=front, +90=left, -90=right (counter-clockwise)
// Stereo speakers at -30 (L) and +30 (R) degrees
float panAngle = (azimuthDeg + 90.0f) / 180.0f;  // normalize to [0, 1]
panAngle = std::clamp(panAngle, 0.0f, 1.0f);
float gainL = std::cos(panAngle * M_PI * 0.5f);
float gainR = std::sin(panAngle * M_PI * 0.5f);
```

**For 4+ speakers (2D, no elevation):** Use SAF `generateVBAPgainTable3D()` with `enableDummies=1` to add imaginary speakers that fill the convex hull. SAF handles this gracefully.

**For 4+ speakers with elevation (3D):** Use SAF `generateVBAPgainTable3D()` directly. The convex hull triangulation works correctly.

### Implementation Strategy

```cpp
void VBAPRenderer::prepare(const SpeakerLayout& layout)
{
    numSpeakers = layout.getChannelCount();

    if (numSpeakers <= 3)
    {
        // Custom pair-wise panning (no SAF)
        useSAF = false;
        precomputePairwiseTable(layout);
    }
    else
    {
        // SAF VBAP gain table
        useSAF = true;
        precomputeSAFGainTable(layout);
    }
}
```

### No Blocking Issues

The `enableDummies=1` flag in SAF adds imaginary speakers at the poles (top/bottom) to complete the convex hull for 2D-only layouts like Quad and 5.1, ensuring the triangulation succeeds.

---

## 2. Perlin Noise for Drift Path

### Approach: Custom Minimal 1D Perlin Noise

After evaluating several options:

| Library | Lines | 1D Support | License | Verdict |
|---------|-------|-----------|---------|---------|
| stb_perlin.h | 430 | No (3D only) | Public domain | Too heavy, no 1D |
| siv::PerlinNoise | ~300 | Yes | MIT | Too heavy for one use |
| db::perlin | ~200 | Yes | MIT | Good but includes 2D/3D we don't need |
| FastNoiseLite | ~4000 | Yes | MIT | Way too heavy |
| **Custom 1D** | **~50** | **Yes** | **N/A** | **Best fit** |

### Recommended Implementation

A minimal 1D Perlin noise + fBm (fractal Brownian motion) in a single header:

```cpp
// PerlinNoise.h
#pragma once
#include <cmath>
#include <cstdint>

class PerlinNoise
{
public:
    // Seed the noise generator (deterministic for given seed)
    void seed(uint32_t s)
    {
        // Fisher-Yates shuffle of [0..255]
        for (int i = 0; i < 256; ++i)
            perm[i] = (uint8_t)i;

        for (int i = 255; i > 0; --i)
        {
            s = s * 1664525u + 1013904223u;  // LCG
            int j = (int)((s >> 16) % (uint32_t)(i + 1));
            uint8_t tmp = perm[i];
            perm[i] = perm[j];
            perm[j] = tmp;
        }

        // Double the table for overflow-free wrapping
        for (int i = 0; i < 256; ++i)
            perm[256 + i] = perm[i];
    }

    // 1D Perlin noise, output in approximately [-1, 1]
    float noise(float x) const
    {
        int xi = (int)std::floor(x) & 255;
        float xf = x - std::floor(x);
        float u = fade(xf);

        float a = grad(perm[xi],     xf);
        float b = grad(perm[xi + 1], xf - 1.0f);
        return lerp(a, b, u);
    }

    // Fractal Brownian motion: stacked octaves for richer texture
    float fbm(float x, int octaves = 4, float lacunarity = 2.0f,
              float persistence = 0.5f) const
    {
        float value = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxAmplitude = 0.0f;

        for (int i = 0; i < octaves; ++i)
        {
            value += noise(x * frequency) * amplitude;
            maxAmplitude += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return value / maxAmplitude;  // normalize to [-1, 1]
    }

private:
    uint8_t perm[512] {};

    static float fade(float t)
    {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);  // quintic
    }

    static float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    static float grad(int hash, float x)
    {
        return (hash & 1) ? -x : x;  // gradient: +1 or -1
    }
};
```

### Properties

- **Output range:** approximately [-1, 1] (noise), exactly [-1, 1] (fbm, normalized)
- **Deterministic:** Same seed produces same sequence
- **Real-time safe:** No allocations, no branches causing cache misses, pure arithmetic
- **~50 lines** including fBm
- **No dependencies** (only `<cmath>`, `<cstdint>`)

### Mapping to Orbital Drift

In the MotionEngine `process()` function for the Drift path:

```cpp
// noiseTime advances each block by (speed / sampleRate) * numSamples
// Speed parameter (0.01-20 Hz) maps directly to noise traverse rate
noiseTime += (speed * numSamples) / sampleRate;

// Use different noise offsets for azimuth and elevation
float azNoise = perlin.fbm(noiseTime, 4);          // [-1, 1]
float elNoise = perlin.fbm(noiseTime + 1000.0f, 4); // offset to decorrelate

azimuth = azNoise * (width * 0.5f);                 // scale by Width param
elevation = elevationEnabled
    ? elNoise * elevationRange
    : 0.0f;
distance = baseDistance + perlin.fbm(noiseTime + 2000.0f, 3) * (depth * 0.5f);
```

**Why fBm over raw noise:** Single-octave Perlin noise is too smooth for interesting drift. fBm with 3-4 octaves creates a fractal structure: slow sweeping motion overlaid with faster micro-variations, which feels organic and musical.

---

## 3. Thread Safety for VBAP Data Swap

### Problem

VBAP gain table precomputation (triangulation + gain table generation) takes 100-500ms for complex layouts. This must run on a background thread. The audio thread needs lock-free access to the gain table every processBlock.

### Recommended Pattern: SpinLock + ScopedTryLock

This is the exact pattern JUCE uses internally in two critical systems:

1. **`AudioProcessorGraph::RenderSequenceExchange`** (`juce_AudioProcessorGraph.cpp:1663-1711`)
2. **`Convolution::TryLockedPtr`** (`juce_Convolution.cpp:678-696`)

### Why This Pattern

| Pattern | Lock-free audio? | Memory safe | Complexity | JUCE precedent |
|---------|-----------------|-------------|------------|----------------|
| Double-buffer + atomic swap | Yes | Fragile (lifetime) | Medium | No |
| `atomic<shared_ptr>` (C++20) | **No** (hidden spinlock) | Good | Low | No |
| **SpinLock + ScopedTryLock** | **Effectively yes** | **Excellent** | **Medium** | **Yes** |

### How It Works

```
Background Thread: Computes new VBAPData (100-500ms)
    |
    v
VBAPDataExchange::setNewData()
    - SpinLock::ScopedLockType lock (blocks, but background thread can wait)
    - Stores unique_ptr<VBAPData> into pending slot
    |
    v
Audio Thread: processBlock() → updateAudioThreadData()
    - SpinLock::ScopedTryLockType lock (NEVER blocks)
    - If lock acquired: swap pending ↔ active
    - If lock busy: skip, use existing data (one block delay, imperceptible)
    |
    v
Timer (message thread): Cleans up old VBAPData
    - Destroys the old unique_ptr on message thread (not audio thread)
```

**Key insight:** `ScopedTryLockType` on the audio thread performs a single atomic compare-and-swap. If it fails (writer is active), the audio thread immediately falls through — no spinning, no blocking, no priority inversion.

### Implementation

Two new classes:

1. **`VBAPDataExchange`** — Thread-safe exchange point (modeled on JUCE's `RenderSequenceExchange`):
   - `setNewData(unique_ptr<VBAPData>)` — writer (background thread)
   - `updateAudioThreadData()` — reader (audio thread, top of processBlock)
   - `getAudioThreadData()` — accessor (audio thread only)
   - Timer callback — cleanup (message thread)

2. **`VBAPComputeThread`** — Background `juce::Thread` with:
   - `requestRecomputation(layout)` — non-audio-thread trigger
   - `run()` — wait for requests, compute gain table, call `setNewData()`
   - `threadShouldExit()` checks for clean shutdown

**Triggering from audio thread:** The audio thread detects layout parameter changes but can't call `requestRecomputation` (uses `std::mutex`). Solution: use `juce::AsyncUpdater::triggerAsyncUpdate()` which is lock-free and audio-safe, then call `requestRecomputation` from the `handleAsyncUpdate()` callback on the message thread.

### Gain Crossfading

When new VBAP data arrives, the per-speaker gains may change abruptly. The existing per-sample linear ramp between `previousGain[speaker]` and `currentGain[speaker]` (already planned in ARCHITECTURE.md) handles this automatically. No additional crossfade mechanism needed.

---

## 4. Motion Engine Path Algorithms

### Orbit (Elliptical)

Per-block update:

```cpp
float t = phaseAccumulator + (phaseDeg * M_PI / 180.0f);
azimuth = (width * 0.5f) * std::cos(t);
elevation = elevEnabled ? (elevRange * std::sin(t)) : 0.0f;
distance = baseDist + (depth * 0.01f) * baseDist * 0.5f * (std::sin(t) + 1.0f);
phaseAccumulator += 2.0f * M_PI * speed * numSamples / sampleRate;
if (phaseAccumulator >= 2.0f * M_PI)
    phaseAccumulator -= 2.0f * M_PI;
```

### Pendulum (Single-Axis Swing)

```cpp
azimuth = (width * 0.5f) * std::sin(phaseAccumulator);
elevation = 0.0f;
distance = baseDist;
// Same phase accumulator update as Orbit
```

### Linear (Constant Velocity Sweep)

```cpp
azimuth = width * (phaseAccumulator / (2.0f * M_PI)) - width * 0.5f;
// Wraps via phase accumulator modulo
elevation = 0.0f;
distance = baseDist;
```

### Drift (Perlin fBm)

```cpp
float azNoise = perlin.fbm(noiseTime, 4);
float elNoise = perlin.fbm(noiseTime + 1000.0f, 4);
azimuth = azNoise * (width * 0.5f);
elevation = elevEnabled ? (elNoise * elevRange) : 0.0f;
distance = baseDist + perlin.fbm(noiseTime + 2000.0f, 3) * (depth * 0.01f) * baseDist;
noiseTime += speed * numSamples / sampleRate;
```

### Tempo Sync

Read host BPM, convert musical divisions to Hz:

```cpp
static constexpr float tempoMultipliers[] = {
    0.0f,        // Off
    4.0f/3.0f,   // 1/16T (triplet)
    1.0f,        // 1/16
    2.0f/3.0f,   // 1/16D (dotted)
    2.0f/3.0f,   // 1/8T
    0.5f,        // 1/8
    1.0f/3.0f,   // 1/8D
    1.0f/3.0f,   // 1/4T
    0.25f,       // 1/4
    1.0f/6.0f,   // 1/4D
    0.125f,      // 1/2
    1.0f/12.0f,  // 1/2D
    0.0625f,     // 1 Bar
    0.03125f,    // 2 Bars
    0.015625f    // 4 Bars
};

// tempo_multiplier converts: Hz = BPM / 60.0 * (1.0 / division_in_beats)
// E.g., 1/4 at 120 BPM = 120/60 * 0.25 = 0.5 Hz (one cycle per 2 seconds)
float effectiveSpeed = (tempoSyncIndex > 0)
    ? (float)(hostBpm / 60.0) * tempoMultipliers[tempoSyncIndex]
    : speed;
```

### Per-Sample Linear Interpolation

Motion state updated per-block; per-sample interpolation prevents zipper noise:

```cpp
// In processBlock:
MotionState startState = motionEngine.getCurrentState();
motionEngine.advance(numSamples);
MotionState endState = motionEngine.getCurrentState();

for (int sample = 0; sample < numSamples; ++sample)
{
    float t = (float)sample / (float)numSamples;
    float az  = startState.azimuth   + t * (endState.azimuth   - startState.azimuth);
    float el  = startState.elevation + t * (endState.elevation - startState.elevation);
    float dist = startState.distance + t * (endState.distance  - startState.distance);
    // ... process sample with interpolated position
}
```

---

## 5. Distance Model Implementation

### Attenuation Curves

```cpp
float computeDistanceGain(float distance, int curveType, float refDist = 1.0f)
{
    switch (curveType)
    {
        case 0: // Linear
            return std::clamp(1.0f - (distance - refDist) / (30.0f - refDist), 0.0f, 1.0f);
        case 1: // Inverse
            return refDist / std::max(distance, 0.1f);
        case 2: // Inverse Square
            return (refDist * refDist) / std::max(distance * distance, 0.01f);
        default:
            return 1.0f;
    }
}
```

### Air Absorption LPF

Single-pole IIR low-pass whose cutoff decreases with distance:

```cpp
float cutoff = 20000.0f / (1.0f + airAbsorption * 0.01f * (distance / 10.0f));
cutoff = std::clamp(cutoff, 100.0f, 20000.0f);
// Use juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoff)
```

**Per-source processing:** One `juce::dsp::IIR::Filter<float>` per source (mono mode = 1, L+R split = 2). LPF coefficients updated per-block (not per-sample) to avoid excessive computation.

### Processing Order

1. Compute distance gain from attenuation curve
2. Apply gain to source signal
3. Apply air absorption LPF to source signal
4. Feed into VBAP renderer

---

## 6. Source Mode Handling

### Mono Mode

```cpp
float mono = (inputL + inputR) * 0.5f;
// Process mono through distance model and VBAP at motionState position
```

### L+R Split Mode

```cpp
// L source at current azimuth
processSource(inputL, azimuth, elevation, distance);

// R source at azimuth + offset
float offsetAz = wrapAngle(azimuth + lrOffsetDeg);
processSource(inputR, offsetAz, elevation, distance);
```

**wrapAngle:** Wrap to [-180, 180] range for consistent VBAP lookup.

---

## 7. Potential Issues Identified

### Issue 1: SAF `extern "C"` Wrapping

SAF is C89. All includes must be wrapped:

```cpp
extern "C" {
#include "saf.h"
}
```

**Already confirmed in Stage 1 research.** No new issues.

### Issue 2: SAF Memory Management

SAF's `generateVBAPgainTable3D()` allocates output with `malloc()`. Must `free()` after copying to our `std::vector<float>`. This happens on the background thread (non-real-time), so it's fine.

### Issue 3: LFE Channel in 5.1/7.1

The 5.1 and 7.1 presets include an LFE speaker at azimuth=0, elevation=0 (same as Center). VBAP will treat this as a regular speaker, which is incorrect — LFE should receive a bass-filtered version of the signal, not standard VBAP gains.

**Mitigation:** For Phase 2.1/2.2, exclude LFE from VBAP calculation (mark it in the SpeakerLayout). LFE handling can be added as a refinement in Phase 2.3 or Stage 4 polish.

### Issue 4: Azimuth Wrapping at +/-180 Boundary

When source azimuth crosses the ±180° boundary (rear), interpolation must handle wrapping correctly to avoid jumping 360° in one block.

**Mitigation:** Use shortest-arc interpolation:

```cpp
float shortestArc(float from, float to)
{
    float diff = std::fmod(to - from + 180.0f, 360.0f) - 180.0f;
    return from + diff;
}
```

---

## 8. Module Opportunities

No existing Ouaricon modules are applicable to O-Orbit's DSP. The PerlinNoise utility is too specialized to extract as a shared module (only useful for spatial drift).

---

## 9. Research Summary

### Ready to Plan

| Topic | Finding | Confidence |
|-------|---------|------------|
| SAF VBAP for stereo | Custom pair-wise for 2-3 speakers, SAF for 4+ | High |
| Perlin noise | Custom 1D + fBm, ~50 lines, header-only | High |
| Thread safety | SpinLock + ScopedTryLock (JUCE pattern) | High |
| Motion algorithms | 4 paths with per-block update + per-sample interp | High |
| Distance model | 3 curves + 1-pole LPF, per-source | High |
| Tempo sync | 15 divisions with BPM conversion | High |
| LFE handling | Defer to Phase 2.3 / Stage 4 | Medium |
| Azimuth wrapping | Shortest-arc interpolation | High |

### No Blocking Issues Found

All open questions from the discuss phase are resolved. Ready to proceed to plan phase.

---

## Sources

- SAF source: `plugins/O-Orbit/libs/SAF/framework/` (direct inspection)
- SAF `convhull_3d.c` line 383: `if(nVert<=3)` early return
- SAF `saf_vbap.h`: `generateVBAPgainTable3D()`, `vbap3D()` signatures
- SPARTA panner source: `saf_examples/panner/src/panner.c` (gain table lookup pattern)
- JUCE `juce_AudioProcessorGraph.cpp:1663-1711`: `RenderSequenceExchange`
- JUCE `juce_Convolution.cpp:678-696`: `TryLockedPtr`
- JUCE `juce_SpinLock.h:70-74`: `tryEnter()` single CAS
- Ken Perlin's improved noise (2002): quintic fade function
- db::perlin (daniilsjb/perlin-noise, MIT): 1D algorithm reference
- `research/sound-spatialization-algorithms.md` (local)
- `research/saf-juce-integration-guide.md` (local)
- Stage 1 RESEARCH.md: SAF build patterns, VBAP API reference
