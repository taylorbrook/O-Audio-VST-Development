# Stage 2: DSP - Phase 3.1 Execute Summary

**Plugin:** O-Reed
**Phase:** 3.1 — Core Engine (Dynamic Reed + Strategy C Bore)
**Date:** 2026-04-05
**Result:** Build verified, zero errors, zero warnings, installed

---

## Components Implemented

### ReedModel (Source/DSP/ReedModel.h)
- Mass-spring-damper ODE with symplectic Euler discretization (velocity-first)
- Bernoulli flow junction (Psi=0 single-reed)
- Static reed fallback when mu_r < 1e-4f
- Embouchure modifiers: g_eff, k_eff, H_eff
- Reed closure clamp with velocity zeroing
- APVTS [0-1] to physical unit mapping for all reed parameters

### BoreWaveguide (Source/DSP/BoreWaveguide.h)
- Strategy C conical bore with spherical wave scaling (2 multiplications per sample)
- Two Thiran-interpolated delay lines (forward/backward, max 40000 samples)
- Bell reflection: first-order allpass filter (frequency-dependent)
- Viscothermal loss: one-pole lowpass (bore diameter dependent)
- Filter group delay compensation for pitch accuracy
- Scale factor smoothing (~50ms) for click-free bore morphing
- Energy tracking for voice cleanup

### BreathEnvelope (Source/DSP/BreathEnvelope.h)
- Four states: Off, Attack, Sustain, Release
- Velocity-scaled attack time (5ms fast → 50ms slow)
- Chiff overshoot (0-30% above target, velocity + param controlled)
- ~150ms exponential release with natural decay
- Smooth sustain tracking for breath controller changes

### Voice Integration (ReedWindVoice.h/.cpp)
- All 3 DSP components orchestrated per-sample
- 11 active APVTS parameters read per-block via atomic loads
- MPE expression: pressure → breath, timbre → embouchure, pitchbend → frequency
- Pop-before-push waveguide ordering
- Z_c normalization (reference impedance 2.67e6)
- Safety clip via std::tanh
- Energy-based voice cleanup (no timeout)
- ScopedNoDenormals + snapFiltersToZero

### Processor Wiring (PluginProcessor.cpp)
- Voice prepare() called via dynamic_cast iteration in prepareToPlay

---

## Parameters Active (11)

| Parameter | APVTS ID | Connected To |
|-----------|----------|-------------|
| Breath Pressure | breathPressure | BreathEnvelope target, ReedModel p_mouth |
| Embouchure | embouchure | ReedModel g_eff/k_eff/H_eff modifiers |
| Reed Hardness | reedHardness | ReedModel k_r stiffness |
| Reed Opening | reedOpening | ReedModel H rest opening |
| Reed Mass | reedMass | ReedModel mu_r (static fallback < 1e-4) |
| Reed Damping | reedDamping | ReedModel g_r damping |
| Bore Character | boreCharacter | BoreWaveguide cone half-angle (0=cyl) |
| Bell Size | bellSize | BoreWaveguide bell allpass cutoff |
| Bore Diameter | boreDiameter | BoreWaveguide viscothermal cutoff, Z_c |
| Bore Length | boreLength | BoreWaveguide effective tube length |
| Output Gain | outputGain | dB to linear on radiated output |

---

## Files Created/Modified

| File | Action |
|------|--------|
| Source/DSP/ReedModel.h | Created (header-only) |
| Source/DSP/BoreWaveguide.h | Created (header-only) |
| Source/DSP/BreathEnvelope.h | Created (header-only) |
| Source/ReedWindVoice.h | Modified (DSP members, prepare()) |
| Source/ReedWindVoice.cpp | Modified (full DSP integration) |
| Source/PluginProcessor.cpp | Modified (voice prepare wiring) |

---

## Build Result

- VST3 + AU: zero errors, zero warnings
- Installed to system plugin folders
- AU cache cleared
