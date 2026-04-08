# O-Reed Phase 3.2 Summary: Guillemain Psi + Breath Noise + Mouthpiece Chamber

## What Was Implemented

### 1. Guillemain Psi Confinement (Double Reed Morphing)
- **Parameter:** `doubleReed` (0-1) maps to Psi (0-0.8)
- **Physics:** `psi_denom = 1 + psi * (S_opening / S_reed)^2` reduces effective pressure drop through reed channel
- **Effect:** Higher Psi = more confined jet = oboe/bassoon character (brighter, more focused tone)
- **Regression:** At doubleReed=0, psi_denom=1.0, equation identical to Phase 3.1

### 2. Flow-Dependent Breath Noise
- **Parameter:** `airNoise` (0-1) controls noise amplitude
- **Physics:** Bandpass-filtered white noise (1700 Hz center, Q=0.707) scaled by flow magnitude
- **Flow coupling:** `flowNorm = min(|u_reed_prev| * Z_c / 12000, 1)` -- noise only present when air is flowing
- **Pressure scaling:** Noise proportional to mouth pressure (no noise during silence)
- **Per-voice decorrelation:** Each voice seeds its PRNG with voiceIndex
- **Regression:** At airNoise=0, processSample returns 0.0f immediately

### 3. Mouthpiece Chamber Compliance
- **Parameter:** `mouthpieceVol` (0-1) maps to volume 0-15 cm^3
- **Physics:** Lumped-element acoustic compliance + inertance, symplectic Euler integration
- **Effect:** Adds formant resonance between reed and bore, affects attack transient character
- **Activation handling:** Smooth parameter transition with 20ms time constant, state initialization on activation to prevent clicks
- **Regression:** At mouthpieceVol=0, chamber.active=false, bypasses to direct `Z_c * u_reed + p_bore_minus`

### 4. ReedModel API Change
- `processSample()` now returns `u_reed` (volume flow) instead of `p_bore_plus`
- Z_c removed from processSample signature -- flow-to-wave conversion moved to caller (voice or chamber)
- This separation enables the mouthpiece chamber to intercept flow before wave conversion

## Files Created
- `Source/DSP/BreathNoise.h` -- Header-only, flow-dependent bandpass noise generator
- `Source/DSP/MouthpieceChamber.h` -- Header-only, lumped-element mouthpiece compliance

## Files Modified
- `Source/DSP/ReedModel.h` -- Added psi/S_reed to ReedParams, doubleReed to updateParams, changed processSample return type
- `Source/ReedWindVoice.h` -- Added includes, BreathNoise + MouthpieceChamber members, smoothing state
- `Source/ReedWindVoice.cpp` -- Updated constructor, prepare, noteStarted, noteStopped, renderNextBlock

## Active Parameters After Phase 3.2: 14
Phase 3.1 (11): breathPressure, embouchure, reedHardness, reedOpening, reedMass, reedDamping, boreCharacter, bellSize, boreDiameter, boreLength, outputGain
Phase 3.2 (+3): airNoise, doubleReed, mouthpieceVol

## Build Status
VST3 + AU build: zero errors, zero warnings
auval (aumu ORed OuDv): PASSED
Installed to ~/Library/Audio/Plug-Ins/

## Deviations from Plan
None. Implementation follows execution plan exactly.

## Real-Time Safety Audit
- No memory allocation in processBlock path
- ScopedNoDenormals present
- All parameter reads via atomic load()
- All buffers preallocated in prepare()
- Bounded loop (numSamples iterations only)
- No locks, I/O, exceptions, or std::function
