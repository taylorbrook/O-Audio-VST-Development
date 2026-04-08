# Phase 3.2 Execution Plan: Dynamic Reed Psi + Breath Noise + Mouthpiece Volume

**Created:** 2026-04-05
**Stage:** 2 (DSP)
**Phase:** 3.2
**Goal:** Activate Guillemain Psi confinement for single-to-double-reed morphing, add flow-dependent breath noise, and implement mouthpiece chamber compliance. All three features bypass cleanly at default values — Phase 3.1 regression guaranteed.

---

## Tasks

### 1. [ ] Modify ReedModel: add Psi confinement + return u_reed

**Files:** `Source/DSP/ReedModel.h`

**Changes:**
- Add `psi` (float, 0-0.8) and `S_reed` (float, m^2) to `ReedParams`
- Add `doubleReed` parameter to `updateParams()` signature
  - Map DOUBLE_REED 0-1 to `psi` 0-0.8: `params.psi = doubleReed * 0.8f`
  - Set `params.S_reed = A_bore` (bore cross-section at throat)
- Change `processSample()` return from `p_bore_plus` to `u_reed`:
  - Remove `Z_c` from signature (no longer needed internally)
  - After reed ODE and S_opening computation, add Guillemain Psi confinement:
    ```cpp
    float psi_denom = 1.0f;
    if (params.psi > 0.0f && S_opening > 0.0f)
    {
        float ratio = S_opening / params.S_reed;
        psi_denom = 1.0f + params.psi * ratio * ratio;
    }
    float u_reed = copysign(1,dp) * S_opening * sqrt(2*|dp| / (rho * psi_denom));
    ```
  - Return `u_reed` instead of `p_bore_plus` (remove the `Z_c * u_reed + p_bore_minus` line)

**Depends on:** None

**Regression guarantee:** At Psi=0, `psi_denom=1.0f`, equation is identical to Phase 3.1 line 130-131.

---

### 2. [ ] Update ReedWindVoice sample loop for new ReedModel API

**Files:** `Source/ReedWindVoice.cpp`

**Changes:**
- Update `renderNextBlock` sample loop to call `reedModel.processSample(p_mouth, p_bore_minus)` (no Z_c)
- Receive `u_reed` instead of `p_bore_plus`
- Add flow-to-wave conversion in voice: `p_bore_plus = Z_c * u_reed + p_bore_minus`
- Add `float prevUReed = 0.0f` member to `ReedWindVoice.h`
- Store `prevUReed = u_reed` each sample for noise scaling (Task 4)
- Update `noteStarted()` to pass `doubleReed` to `reedModel.updateParams()`
- Add per-block read of `pDoubleReed->load()` in `renderNextBlock`
- Pass `doubleReed` to `reedModel.updateParams()` call

**Depends on:** Task 1

**Verification:** Build + run at doubleReed=0 must produce identical output to Phase 3.1.

---

### 3. [ ] Create BreathNoise class

**Files:** `Source/DSP/BreathNoise.h` (new, header-only)

**Changes:**
- Constructor takes `int seed` for per-voice PRNG decorrelation
- Members: `juce::Random noiseRng`, `juce::dsp::StateVariableTPTFilter<float> noiseFilter`
- `prepare(double sampleRate, int maxBlockSize)`:
  - Prepare filter with ProcessSpec
  - Set type bandPass, cutoff 1700 Hz, resonance 0.707f (Butterworth-flat)
- `processSample(float airNoise, float u_reed_prev, float Z_c, float p_mouth) -> float`:
  - Early return 0 if `airNoise < 1e-6f`
  - Generate white noise `[-1, 1]` via `noiseRng.nextFloat() * 2 - 1`
  - Bandpass filter the noise
  - Compute flow-dependent amplitude: `flowNorm = min(|u_reed_prev| * Z_c / 12000, 1.0)`
  - Return `airNoise * flowNorm * max(p_mouth, 0) * 0.05f * filtered`
- `reset()`: reset filter state

**Depends on:** None (can be done in parallel with Tasks 1-2)

---

### 4. [ ] Create MouthpieceChamber class

**Files:** `Source/DSP/MouthpieceChamber.h` (new, header-only)

**Changes:**
- Members: `float dt, C_m, M_m, damping, p_chamber, u_bore; bool active`
- `prepare(double sampleRate)`: store `dt = 1/sr`, call reset
- `setParams(float volume_m3, float boreArea_m2, float rho, float c)`:
  - If `volume_m3 < 1e-8f`: set `active = false`, return
  - Set `active = true`
  - `L_m = volume_m3 / boreArea_m2`
  - `C_m = volume_m3 / (rho * c * c)`
  - `M_m = rho * L_m / boreArea_m2`
  - `damping = 0.001f`
- `processSample(float u_reed, float p_bore_minus, float Z_c) -> float`:
  - If `!active`: return `Z_c * u_reed + p_bore_minus` (bypass)
  - Symplectic Euler:
    - `u_bore += (p_chamber - p_bore_minus) / M_m * dt`
    - `u_bore *= (1 - damping)` (viscous loss)
    - `p_chamber += (u_reed - u_bore) / C_m * dt`
  - Return `Z_c * u_bore + p_bore_minus`
- `initializeState(float p_mouth)`: set `p_chamber = p_mouth, u_bore = 0` (for smooth activation)
- `reset()`: zero state, `active = false`

**Depends on:** None (can be done in parallel with Tasks 1-3)

---

### 5. [ ] Wire BreathNoise + MouthpieceChamber into ReedWindVoice

**Files:** `Source/ReedWindVoice.h`, `Source/ReedWindVoice.cpp`

**Header changes:**
- `#include "DSP/BreathNoise.h"` and `#include "DSP/MouthpieceChamber.h"`
- Add members: `BreathNoise breathNoise;`, `MouthpieceChamber chamber;`
- Add smoothing state: `float smoothedPsi = 0.0f`, `float smoothedMouthpieceVol = 0.0f`, `float paramSmoothCoeff = 0.001f`

**Cpp changes — prepare():**
- `breathNoise.prepare(sampleRate, maxBlockSize)` (pass voice index as seed via constructor)
- `chamber.prepare(sampleRate)`
- Compute `paramSmoothCoeff = 1 - exp(-1/(sr * 0.020))` (~20ms time constant)

**Cpp changes — noteStarted():**
- Reset `breathNoise`, `chamber`, `prevUReed = 0`
- Reset smoothing state to 0

**Cpp changes — noteStopped():**
- Reset `breathNoise`, `chamber` in hard-stop path

**Cpp changes — renderNextBlock():**
- Per-block: read `airNoise`, `doubleReed`, `mouthpieceVol` from APVTS
- Compute chamber physical params:
  - `float V_m = mouthpieceVol * 1.5e-5f` (0-1 -> 0-15 cm^3)
  - `float A_bore = pi * throatRadius^2` (from boreDiameter param, same mapping as ReedModel)
  - `chamber.setParams(V_m, A_bore, 1.2f, 343.0f)`
- Per-sample loop restructure:
  1. `p_mouth_raw = breathEnv.processSample() * 12000`
  2. `noisePressure = breathNoise.processSample(airNoise, prevUReed, Z_c, p_mouth_raw)`
  3. `p_mouth = p_mouth_raw + noisePressure`
  4. Smooth Psi: `smoothedPsi += (targetPsi - smoothedPsi) * paramSmoothCoeff`
  5. `u_reed = reedModel.processSample(p_mouth, prevBoreMinus)`
  6. `prevUReed = u_reed`
  7. Smooth mouthpieceVol for chamber activation logic
  8. `p_bore_plus = chamber.processSample(u_reed, prevBoreMinus, Z_c)`
  9. `prevBoreMinus = bore.processSample(p_bore_plus)`
  10. Output = `bore.getRadiatedOutput() * normalization * outputGain`
- When chamber transitions from inactive to active, call `chamber.initializeState(p_mouth)` to prevent transient

**Depends on:** Tasks 1, 2, 3, 4

---

### 6. [ ] Build and verify regression at default parameters

**Files:** None (build + test only)

**Steps:**
- `ninja O-Reed_VST3 O-Reed_AU`
- Clear AU cache, install to system folders
- `auval -v aumu ORed Ouar`
- Load in DAW, play MIDI:
  - DOUBLE_REED=0, AIR_NOISE=0, MOUTHPIECE_VOL=0 -> identical to Phase 3.1
  - Verify clean tone, stable pitch, no artifacts

**Depends on:** Task 5

---

### 7. [ ] Test new parameters individually and combined

**Files:** None (listening test only)

**Steps:**
- DOUBLE_REED sweep 0->1: single-reed -> nasal oboe -> piercing zurna
- AIR_NOISE sweep 0->1: clean -> breathy -> airy (quiet in silence, loud when playing)
- MOUTHPIECE_VOL sweep 0->1: neutral -> sub-resonance coloring -> Helmholtz effect
- All three at moderate values simultaneously: stable, musically interesting
- Extreme combined: Psi=0.8, noise=1.0, chamber=1.0: stable (no blowup, tanh catches transients)
- Rapid parameter automation: no clicks (smoothing working)

**Depends on:** Task 6

---

## Files Summary

| File | Action | Description |
|------|--------|-------------|
| `Source/DSP/ReedModel.h` | Modify | Add Psi confinement, return u_reed |
| `Source/DSP/BreathNoise.h` | Create | Noise generator with BPF + flow-dependent amplitude |
| `Source/DSP/MouthpieceChamber.h` | Create | Two-state Helmholtz chamber with symplectic Euler |
| `Source/ReedWindVoice.h` | Modify | Add new DSP members + smoothing state |
| `Source/ReedWindVoice.cpp` | Modify | Restructured sample loop, 3 new params wired |

## Success Criteria

- [ ] DOUBLE_REED at 0: identical to Phase 3.1 (regression)
- [ ] DOUBLE_REED at 0.4: nasal, oboe-like character
- [ ] DOUBLE_REED at 0.7+: piercing, zurna/shehnai character
- [ ] Psi stable across full 0-1 range
- [ ] AIR_NOISE at 0: no noise (regression)
- [ ] AIR_NOISE at moderate: audible breathiness scaling with dynamics
- [ ] Noise quiet during silence, loud during forte
- [ ] MOUTHPIECE_VOL at 0: bypass (regression)
- [ ] MOUTHPIECE_VOL at moderate: sub-resonance coloring
- [ ] MOUTHPIECE_VOL at high: noticeable Helmholtz effect
- [ ] All three features interact correctly simultaneously
- [ ] No clicks during parameter changes
- [ ] VST3 + AU build zero errors
- [ ] auval passes
- [ ] 14 parameters active (11 from 3.1 + 3 new)

## Parallelization

Tasks 1+3+4 can execute in parallel (no dependencies). Task 2 depends on Task 1. Task 5 depends on all of 1-4. Tasks 6-7 are sequential verification.
