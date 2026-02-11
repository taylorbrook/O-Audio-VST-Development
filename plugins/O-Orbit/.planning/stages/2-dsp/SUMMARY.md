# Stage 2: DSP — Execution Summary

> **Plugin:** O-Orbit
> **Stage:** 2 (DSP)
> **Date:** 2026-02-10
> **Status:** Complete

---

## What Was Built

### Phase 2.1: Motion Engine + Distance Model + Stereo VBAP

1. **PerlinNoise.h** — Custom 1D Perlin noise + fBm, header-only (~70 lines)
   - Fisher-Yates permutation table shuffle
   - Quintic fade interpolation
   - fBm with configurable octaves, lacunarity, persistence
   - Output range normalized to [-1, 1]

2. **MotionEngine** — 4 motion path algorithms
   - **Orbit:** Elliptical path using cos/sin with phase offset, elevation, distance modulation
   - **Pendulum:** Single-axis sinusoidal swing
   - **Linear:** Constant velocity sweep with wrap
   - **Drift:** Perlin fBm noise with decorrelated az/el/distance channels
   - Tempo sync: 15 musical divisions (1/16T through 4 Bars) using host BPM
   - Per-block state update with `getCurrentState()` for interpolation

3. **DistanceModel** — Attenuation + Air Absorption LPF
   - 3 attenuation curves: Linear, Inverse, Inverse Square
   - Air absorption via `juce::dsp::IIR::Filter<float>` low-pass
   - Cutoff formula: `20000 / (1 + airAbsorption * 0.01 * (distance / 10))`
   - Per-source processing (separate L/R instances for Split mode)

4. **VBAPRenderer** — Stereo equal-power panning
   - 2-speaker: equal-power panning from azimuth
   - 3-speaker: pair-wise nearest-speaker panning
   - 4+ speakers: delegates to SAF via external gain table

5. **processBlock** — Full DSP chain wired
   - Per-sample linear interpolation of motion state (az, el, dist)
   - Shortest-arc azimuth wrapping at ±180°
   - Mono and L+R Split source modes
   - Per-speaker gain smoothing (linear ramp)
   - Dry/wet mix

### Phase 2.2: Full 2D VBAP + Multi-Channel Output

6. **VBAPDataExchange** — Thread-safe gain table swap
   - SpinLock + ScopedTryLock pattern (matches JUCE's RenderSequenceExchange)
   - Audio thread: ScopedTryLock (single CAS, never blocks)
   - Background thread: writes new VBAPData via SpinLock
   - Timer callback cleans up old data on message thread

7. **VBAPComputeThread** — Background SAF computation
   - `juce::Thread` with wait/notify pattern
   - Calls SAF `generateVBAPgainTable3D()` with `enableDummies=1`
   - Excludes LFE speakers from triangulation
   - Supports both 2D and 3D layouts

8. **Processor integration** — AsyncUpdater for layout changes
   - Audio thread detects layout param change → `triggerAsyncUpdate()`
   - Message thread: updates VBAPRenderer, triggers background recomputation
   - Audio thread: `updateAudioThreadData()` at top of processBlock

9. **Center Diverge** — Spreads signal to adjacent speakers
   - Angular proximity weighting
   - Energy-preserving normalization

### Phase 2.3: 3D VBAP + Auto-Downmix + LFE

10. **3D VBAP** — Elevation support via SAF gain table
    - Detects 3D layouts (5.1.4, 7.1.4) via `layout.is3D`
    - 3D lookup: `elevIndex * N_azi + aziIndex` into gain table

11. **DownmixEngine** — Auto-downmix to stereo
    - Builds downmix matrix at prepare time
    - Stereo: pan each speaker to L/R based on azimuth
    - Energy-preserving normalization
    - LFE distributed equally to both channels

12. **LFE Handling** — `isLFE` flag on Speaker struct
    - Marked in 5.1, 7.1, 5.1.4, 7.1.4 presets
    - Excluded from VBAP gain table generation
    - Zero gain from VBAP output

---

## Files Created

| File | Lines | Description |
|------|-------|-------------|
| `Source/DSP/PerlinNoise.h` | 70 | 1D Perlin noise + fBm, header-only |
| `Source/DSP/VBAPDataExchange.h` | 90 | Thread-safe VBAP data exchange + compute thread |
| `Source/DSP/VBAPDataExchange.cpp` | 115 | SAF gain table generation on background thread |
| `Source/DSP/DownmixEngine.h` | 135 | Auto-downmix engine, header-only |

## Files Modified

| File | Changes |
|------|---------|
| `Source/DSP/MotionEngine.h` | Added PerlinNoise, getCurrentState(), advance(), tempoMultipliers |
| `Source/DSP/MotionEngine.cpp` | Implemented 4 path algorithms, tempo sync |
| `Source/DSP/DistanceModel.h` | Full implementation: IIR LPF, 3 attenuation curves |
| `Source/DSP/DistanceModel.cpp` | Reduced to include-only (implementation in header) |
| `Source/DSP/VBAPRenderer.h` | Added SAF support, center diverge, LFE exclusion, external gain table |
| `Source/DSP/VBAPRenderer.cpp` | Stereo/pairwise/SAF panning, center diverge, LFE handling |
| `Source/PluginProcessor.h` | Added AsyncUpdater, VBAPDataExchange, DownmixEngine, gain smoothing arrays |
| `Source/PluginProcessor.cpp` | Full processBlock DSP chain, auto-downmix, layout change handling |
| `Source/Data/SpeakerLayout.h` | Added `isLFE` flag to Speaker struct |
| `Source/Data/SpeakerPresets.h` | Marked LFE channels in 5.1, 7.1, 5.1.4, 7.1.4 presets |
| `CMakeLists.txt` | Added new source files, SAF include path |

---

## Build Results

- **VST3:** Built successfully
- **AU:** Built successfully
- **Standalone:** Built successfully, launches without crash
- **Warnings from O-Orbit source:** Zero

---

## Architecture Notes

- Distance model coefficients update per-block (not per-sample) to avoid excessive computation
- VBAP gains computed at block boundaries, linearly interpolated per-sample for smooth motion
- Background VBAP thread uses wait/notify pattern (not polling) for minimal CPU when idle
- All SAF memory allocations happen on background thread; `free()` after copying to `std::vector`
- LFE exclusion happens at both VBAP table generation (background thread) and gain application (audio thread)
