---
phase: 04-controls-refinement
plan: 01
subsystem: dsp-tuning
tags: [saturation, harmonics, crossover, intensity-scaling]
dependencies:
  requires:
    - 03-01-PLAN.md  # ColoredModeProcessor implementation
    - 03-02-PLAN.md  # Mode switching integration
  provides:
    - Stronger bass enhancement at low crossover frequencies
    - Comparable intensity between Clean and Colored modes
    - T4 4th harmonic in Colored mode
  affects:
    - 04-02-PLAN.md  # Output Mix control builds on tuned enhancement
    - 05-xx-PLAN.md  # Gain staging will validate these tuning changes
tech-stack:
  added: []
  patterns:
    - Chebyshev T4 polynomial for 4th harmonic
    - Frequency-dependent intensity scaling
key-files:
  created: []
  modified:
    - plugins/OBass/Source/DSP/ColoredModeProcessor.h
    - plugins/OBass/Source/DSP/ColoredModeProcessor.cpp
    - plugins/OBass/Source/DSP/CleanModeProcessor.h
    - plugins/OBass/Source/DSP/CleanModeProcessor.cpp
    - plugins/OBass/Source/DSP/HarmonicGenerator.cpp
    - plugins/OBass/Source/PluginProcessor.cpp
decisions:
  - id: "04-01-drive-range"
    choice: "Drive range 2.0-6.0 for Colored mode"
    rationale: "Original 1.0-4.0 was too subtle compared to Clean mode"
  - id: "04-01-bias-value"
    choice: "Bias 0.3 for stronger even harmonics"
    rationale: "Original 0.2 produced too subtle warmth"
  - id: "04-01-intensity-formula"
    choice: "intensityScale = 1.0 + sqrt(1.0 - normalized) * 0.7"
    rationale: "sqrt curve provides natural boost at low frequencies"
  - id: "04-01-bandpass-cutoff"
    choice: "40Hz lowpass cutoff for HarmonicGenerator"
    rationale: "Allows 2nd harmonic of 20Hz fundamentals"
metrics:
  duration: "3m 47s"
  tasks_completed: 3
  completed: "2026-01-25"
---

# Phase 4 Plan 01: Bass Enhancement Intensity Tuning Summary

## One-Liner

Boosted Colored mode with T4 4th harmonic and added frequency-dependent intensity scaling (1.7x at 40Hz) to both processors.

## What Was Built

### Task 1: ColoredModeProcessor Intensity Boost
- Increased drive range from 1.0-4.0 to 2.0-6.0
- Increased DC bias from 0.2 to 0.3 for stronger even harmonics
- Added explicit Chebyshev T4 polynomial for 4th harmonic generation
  - Formula: T4(x) = 8x^4 - 8x^2 + 1
  - 15% level scaled by enhance amount
- Result: Colored mode now produces noticeably warmer, more intense saturation

### Task 2: Frequency-Dependent Intensity Scaling
- Added `setIntensityScale(float scale)` method to both CleanModeProcessor and ColoredModeProcessor
- CleanModeProcessor uses `scaledEnhance = enhanceAmount * intensityScale` for harmonic mix
- ColoredModeProcessor uses intensityScale for BOTH:
  - Drive calculation: `drive = (2.0 + enhance * 4.0) * intensityScale`
  - Wet/dry mix: `scaledEnhance = enhanceAmount * intensityScale`
- PluginProcessor calculates scale from crossover frequency:
  ```cpp
  normalized = (crossoverHz - 40.0f) / 160.0f
  intensityScale = 1.0 + sqrt(1.0 - normalized) * 0.7
  ```
  - 40Hz crossover -> 1.7x intensity boost
  - 200Hz crossover -> 1.0x (no boost)

### Task 3: HarmonicGenerator Bandpass Extension
- Lowered highpass cutoff from 80Hz to 40Hz
- Allows 2nd harmonic of 20Hz fundamentals to pass through
- Enables bass enhancement to work at lower crossover frequencies

## Verification Status

All verification criteria passed:
- [x] Build compiles with no new errors
- [x] ColoredModeProcessor uses drive 2.0-6.0, bias 0.3, and T4 harmonic
- [x] Both processors have setIntensityScale() method
- [x] CleanModeProcessor uses scaledEnhance for harmonic mix
- [x] ColoredModeProcessor uses intensityScale for both drive AND wet/dry mix
- [x] PluginProcessor calculates and passes intensity scale to both processors
- [x] HarmonicGenerator bandpass starts at 40Hz
- [x] Plugin installed to system folders

## Commits

| Hash | Description |
|------|-------------|
| b195523 | feat(04-01): boost ColoredModeProcessor intensity with T4 harmonic |
| 681f3a3 | feat(04-01): add frequency-dependent intensity scaling |
| 77c5fa0 | feat(04-01): extend HarmonicGenerator bandpass to 40Hz |

## Deviations from Plan

None - plan executed exactly as written.

## Next Phase Readiness

**Ready for:** 04-02-PLAN.md (Output Mix control)

**Human listening test needed to verify:**
1. Colored mode produces comparable intensity to Clean mode at 50% Enhance
2. Enhancement is noticeably stronger at 40Hz crossover than 200Hz
3. Low crossover frequencies (40-80Hz) produce audible bass enhancement
4. No new artifacts (clicks, distortion, DC offset)

## Technical Notes

**Psychoacoustic reasoning for intensity scaling:**
- Sub-bass frequencies (20-60Hz) are less audible on typical speakers
- The ear perceives upper bass (80-200Hz) more easily
- Adding more enhancement at lower crossover frequencies compensates for this
- The sqrt curve (1.0 + sqrt(1-x) * 0.7) provides a gradual, natural-feeling boost

**Why T4 was added to Colored mode:**
- Asymmetric tanh produces 2nd harmonic primarily
- T4 (4th harmonic) adds warmth in the psychoacoustic "sweet spot"
- 15% scaling prevents muddiness while adding character
