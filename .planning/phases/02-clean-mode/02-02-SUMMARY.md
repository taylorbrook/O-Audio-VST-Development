---
phase: 02-clean-mode
plan: 02
subsystem: dsp
tags: [chebyshev-waveshaper, oversampling, harmonic-generation, psychoacoustic-bass]

# Dependency graph
requires:
  - phase: 01-core-dsp-foundation
    provides: CrossoverFilter, MonoSummer, RT-safe mode switching pattern
  - phase: 02-01
    provides: EnvelopeFollower, PitchTracker for adaptive enhancement
provides:
  - HarmonicGenerator with Chebyshev polynomial waveshaping
  - 4x oversampling for alias-free harmonic generation
  - Adaptive harmonic count based on fundamental frequency
affects: [02-03, 02-04, clean-mode-processor]

# Tech tracking
tech-stack:
  added: []
  patterns: [chebyshev-waveshaping, dual-oversampler-mode-switching, output-bandpass-limiting]

key-files:
  created:
    - plugins/OBass/Source/DSP/HarmonicGenerator.h
    - plugins/OBass/Source/DSP/HarmonicGenerator.cpp
  modified:
    - plugins/OBass/CMakeLists.txt

key-decisions:
  - "Chebyshev T2-T5 polynomials for controlled 2nd-5th harmonic generation"
  - "Harmonic weights: h2=0.7 (warm), h3=0.5 (body), h4=0.3 (clarity), h5=0.15 (presence)"
  - "Output bandpass 60-400Hz limits harmonics to psychoacoustically useful range"
  - "tanh soft clipping normalizes input to [-1,1] before waveshaping"
  - "Adaptive count: <40Hz=5, <80Hz=4, <120Hz=3, else=2 harmonics"

patterns-established:
  - "Dual oversampler pattern: IIR (low-latency) + FIR (high-fidelity) both always prepared"
  - "Atomic mode flag for RT-safe oversampler switching"
  - "Weight normalization maintains consistent output level across harmonic counts"

# Metrics
duration: 4min
completed: 2026-01-23
---

# Phase 02 Plan 02: Harmonic Generator Summary

**Chebyshev polynomial waveshaper with 4x oversampling for alias-free psychoacoustic harmonic generation (2nd-5th harmonics)**

## Performance

- **Duration:** ~4 min
- **Started:** 2026-01-23T18:20:15Z
- **Completed:** 2026-01-23T18:23:49Z
- **Tasks:** 2/2
- **Files created:** 2
- **Files modified:** 1

## Accomplishments
- Created HarmonicGenerator class with Chebyshev polynomial waveshaping (T2-T5)
- Implemented dual 4x oversamplers (IIR/FIR) following CrossoverFilter pattern
- Output bandpass filter (60-400Hz) limits harmonics to useful psychoacoustic range
- Adaptive harmonic count method adjusts harmonics based on input fundamental frequency
- RT-safe mode switching via atomic flag (both oversamplers always prepared)

## Task Commits

Each task was committed atomically:

1. **Task 1: Create HarmonicGenerator with Chebyshev waveshaper** - `a43cd2a` (feat)
   - Chebyshev polynomials T2-T5 for controlled harmonic generation
   - Dual oversamplers (IIR + FIR) for latency mode support
   - Output bandpass (60-400Hz) limits to useful range
   - Default weights from psychoacoustic research

2. **Task 2: Add adaptive harmonic count** - Included in Task 1 commit
   - setAdaptiveHarmonics(float fundamentalHz) method
   - Frequency-based harmonic count: sub-bass uses more, upper bass fewer
   - process() respects activeHarmonicCount setting

## Files Created/Modified

**Created:**
- `plugins/OBass/Source/DSP/HarmonicGenerator.h` - Class declaration with dual oversamplers, weights, adaptive count
- `plugins/OBass/Source/DSP/HarmonicGenerator.cpp` - Chebyshev implementation, oversampled processing

**Modified:**
- `plugins/OBass/CMakeLists.txt` - Added HarmonicGenerator.cpp to build

## Decisions Made
- **Chebyshev polynomials:** T2-T5 for 2nd-5th harmonics (fundamental not included)
- **Default harmonic weights:** h2=0.7, h3=0.5, h4=0.3, h5=0.15 (even harmonics favored for warmth)
- **Output bandpass:** 60Hz highpass + 400Hz lowpass (psychoacoustic bass range)
- **Soft clipping:** tanh() normalizes input to [-1,1] before polynomial application
- **Weight normalization:** Output scaled by sum of active weights for consistent level

## Deviations from Plan
None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- HarmonicGenerator ready for integration in CleanModeProcessor
- setAdaptiveHarmonics() can be called with PitchTracker output
- Dual oversamplers support same modes as CrossoverFilter (LowLatency/HighFidelity)
- Ready for Plan 02-03: CleanModeProcessor integration

## Technical Details

### Chebyshev Polynomials
```cpp
// Generate nth harmonic from sinusoidal input
inline float T2(float x) { return 2.0f*x*x - 1.0f; }           // 2nd harmonic
inline float T3(float x) { return 4.0f*x*x*x - 3.0f*x; }       // 3rd harmonic
inline float T4(float x) { return 8.0f*x*x*x*x - 8.0f*x*x + 1.0f; }  // 4th
inline float T5(float x) { return 16.0f*x*x*x*x*x - 20.0f*x*x*x + 5.0f*x; }  // 5th
```

### Adaptive Harmonic Count
```cpp
void setAdaptiveHarmonics(float fundamentalHz) {
    if (fundamentalHz < 40.0f)       // Sub-bass: nearly inaudible
        activeHarmonicCount = 5;     // Maximum harmonics
    else if (fundamentalHz < 80.0f)  // Deep bass
        activeHarmonicCount = 4;
    else if (fundamentalHz < 120.0f) // Mid-bass
        activeHarmonicCount = 3;
    else                             // Upper bass: already audible
        activeHarmonicCount = 2;     // Minimal
}
```

### Dual Oversampler Pattern
```cpp
// IIR: minimal latency, some phase distortion
oversamplerIIR = make_unique<Oversampling<float>>(
    1, 2,  // mono, 4x (2^2)
    filterHalfBandPolyphaseIIR, true, true);

// FIR: linear phase, more latency
oversamplerFIR = make_unique<Oversampling<float>>(
    1, 2,
    filterHalfBandFIREquiripple, true, true);

// Both initialized in prepare(), atomic flag selects active one
```

---
*Phase: 02-clean-mode*
*Plan: 02*
*Completed: 2026-01-23*
