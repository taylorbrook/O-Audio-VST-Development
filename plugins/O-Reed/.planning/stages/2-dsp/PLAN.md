# Stage 2: DSP - Phase 3.1 Execution Plan

**Plugin:** O-Reed
**Phase:** 3.1 — Core Engine (Dynamic Reed + Strategy C Bore)
**Date:** 2026-04-05
**Source:** CONTEXT.md, RESEARCH.md

---

## Goal

Implement the core physical modeling engine: full mass-spring-damper reed ODE with symplectic Euler integration, Bernoulli flow junction (Psi=0), Strategy C conical bore waveguide with spherical wave scaling, bell reflection filter, viscothermal loss filter, and breath envelope. A single MIDI note should produce an audible, pitch-accurate reed wind tone with 11 active parameters.

---

## Tasks

### 1. [ ] Create ReedModel DSP class
- **Files:** `Source/DSP/ReedModel.h`, `Source/DSP/ReedModel.cpp`
- **Depends on:** None
- **Details:**
  - `ReedParams` struct: mu_r, g_r, k_r, H, embouchure, breathPressure, A_reed, w_reed, rho_air
  - `ReedState` struct: x (displacement), x_dot (velocity)
  - `updateParams(breathPressure, embouchure, reedHardness, reedOpening, reedMass, reedDamping, boreDiameter)` — maps APVTS [0-1] to physical units per RESEARCH.md Section 3 table
  - `processSample(p_mouth, p_bore_minus, Z_c)` → returns `p_bore_plus`
    - Symplectic Euler ODE: velocity-first then position update
    - Embouchure modifiers: g_eff, k_eff, H_eff
    - Reed closure clamp with velocity zeroing
    - Static reed fallback when mu_r < 1e-4f
    - Bernoulli flow: `u = copysign(1,dp) * S_opening * sqrt(2*|dp|/rho)` (Psi=0)
    - Flow-to-wave conversion: `p_bore_plus = Z_c * u + p_bore_minus`
  - `reset()` — zero x, x_dot
  - `isActive()` — true if |x_dot| > epsilon or external breath > 0

### 2. [ ] Create BoreWaveguide DSP class
- **Files:** `Source/DSP/BoreWaveguide.h`, `Source/DSP/BoreWaveguide.cpp`
- **Depends on:** None (parallel with Task 1)
- **Details:**
  - Two `juce::dsp::DelayLine<float, Thiran>` — forward (reed→bell) and backward (bell→reed), max 40000 samples
  - `juce::dsp::IIR::Filter<float>` for bell reflection (first-order allpass)
  - `juce::dsp::IIR::Filter<float>` for viscothermal loss (one-pole lowpass)
  - Conical scale factors: `scaleForward = r_in/r_out`, `scaleBackward = r_out/r_in`
  - `prepare(sampleRate, maxBlockSize)` — init delay lines, allocate filters
  - `setFrequency(hz)` — compute delay length with filter group delay compensation, split between forward/backward, clamp minimum 2.0f
  - `updateParams(boreCharacter, bellSize, boreDiameter, boreLength)` — compute cone half-angle, scale factors, update filter coefficients
    - Bell allpass: `a = (1-t)/(1+t)` where `t = tan(pi * bellCutoff / sr)`
    - Viscothermal: `p = exp(-2*pi * viscCutoff / sr)`, `g = 0.995`
    - bellCutoff from bellSize: 800 Hz (0.0) to 6000 Hz (1.0)
    - viscCutoff from boreDiameter: bore_diameter_mm * 150 Hz
  - `processSample(p_reed_out)` → returns `p_bore_minus` (wave arriving at reed)
    - Pop forward → apply scaleForward → bell reflection + radiated output
    - Pop backward → apply scaleBackward → this is p_bore_minus
    - Push p_reed_out into forward delay
    - Push reflected (after viscothermal loss) into backward delay
  - `getRadiatedOutput()` — last radiated sample (p_at_bell + reflected)
  - `reset()` — zero delay lines, reset filters
  - `getEnergy()` — `0.999f * energy + 0.001f * abs(output)` for voice cleanup
  - `snapFiltersToZero()` — call at end of each block
  - Per-block smooth scale factors with one-pole (~50ms) to prevent clicks during bore morphing

### 3. [ ] Create BreathEnvelope DSP class
- **Files:** `Source/DSP/BreathEnvelope.h`, `Source/DSP/BreathEnvelope.cpp`
- **Depends on:** None (parallel with Tasks 1-2)
- **Details:**
  - States: Off, Attack, Sustain, Release
  - `noteOn(velocity)` — trigger attack with velocity-scaled chiff overshoot
    - Attack time: 50ms (vel=0) → 5ms (vel=1)
    - Chiff overshoot: 0% to 30% above target (from ATTACK_CHIFF param)
  - `noteOff()` — trigger release (~150ms exponential decay)
  - `setTarget(breathPressure)` — sustain level from APVTS
  - `processSample()` → returns current breath pressure (smoothed)
  - `isActive()` — false when release envelope reaches near-zero
  - `reset()` — immediate off

### 4. [ ] Integrate DSP components into ReedWindVoice
- **Files:** `Source/ReedWindVoice.h`, `Source/ReedWindVoice.cpp`
- **Depends on:** Tasks 1, 2, 3
- **Details:**
  - Add member instances: `ReedModel reedModel`, `BoreWaveguide bore`, `BreathEnvelope breathEnv`
  - Add `prepare(double sampleRate, int maxBlockSize)` — forward to all DSP components
  - `noteStarted()`:
    - Reset reed state, delay lines, envelope
    - Set frequency from `getCurrentlyPlayingNote().initialNote` (12-TET for now)
    - Trigger breath envelope with note velocity
  - `noteStopped(allowTailOff)`:
    - If allowTailOff: trigger release, let bore ring down
    - Else: immediate clear
  - `notePressureChanged()`: update breath target from MPE pressure
  - `notePitchbendChanged()`: update bore frequency with pitchbend offset
  - `noteTimbreChanged()`: update embouchure from MPE slide
  - `renderNextBlock()`:
    - `ScopedNoDenormals`
    - Per-block: read 11 active APVTS params via atomic loads, update DSP params
    - Per-sample loop:
      1. `p_mouth = breathEnv.processSample()`
      2. `p_bore_minus = bore.processSample_popOnly()` (pop backward wave)
      3. `p_bore_plus = reedModel.processSample(p_mouth, p_bore_minus, Z_c)`
      4. `bore.processSample_push(p_bore_plus)` (push forward, do bell, push backward)
      5. `output = bore.getRadiatedOutput() * outputGain`
      6. Add to both channels of outputBuffer (mono→stereo)
    - Post-block: `bore.snapFiltersToZero()`
    - Voice cleanup: if `!breathEnv.isActive() && bore.getEnergy() < 1e-6f` → `clearCurrentNote()`
  - Safety clip: `std::tanh(sample)` on output

### 5. [ ] Wire voice prepare() from PluginProcessor
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** Task 4
- **Details:**
  - In `prepareToPlay()`: iterate all voices, dynamic_cast to ReedWindVoice, call `prepare(sampleRate, samplesPerBlock)`
  - Verify synthesiser.setCurrentPlaybackSampleRate is called (already exists)
  - No other processor changes needed — synthesiser.renderNextBlock already delegates to voices

### 6. [ ] Verify build and basic functionality
- **Files:** None (build + test pass)
- **Depends on:** Task 5
- **Details:**
  - `ninja O-Reed_VST3 O-Reed_AU` — zero errors, zero warnings treated as errors
  - Install and verify AU cache cleared per CLAUDE.md protocol
  - Verify MIDI note produces audible output
  - Verify pitch accuracy (middle C = ~261.6 Hz)
  - Verify breath pressure controls dynamics
  - Verify bore_character at 0 produces cylindrical behavior
  - Verify bore_character > 0 introduces taper character
  - Verify note-off ring-down (not abrupt cutoff)
  - Verify no clicks during parameter sweeps

---

## Active Parameters (Phase 3.1)

| Parameter | APVTS ID | Default | Maps To |
|-----------|----------|---------|---------|
| BREATH_PRESSURE | breathPressure | 0.5 | p_mouth (3000-12000 Pa) |
| EMBOUCHURE | embouchure | 0.4 | g_lip, k_lip, x_lip modifiers |
| REED_HARDNESS | reedHardness | 0.5 | k_r (2e6-20e6 N/m³) |
| REED_OPENING | reedOpening | 0.4 | H (0.1-1.5 mm) |
| REED_MASS | reedMass | 0.3 | mu_r (1e-4 to 0.06 kg/m²) |
| REED_DAMPING | reedDamping | 0.5 | g_r (500-6000 s⁻¹) |
| BORE_CHARACTER | boreCharacter | 0.0 | Cone half-angle (0-1.6°) |
| BELL_SIZE | bellSize | 0.5 | Bell reflection cutoff (800-6000 Hz) |
| BORE_DIAMETER | boreDiameter | 0.5 | Throat radius (2-20 mm), viscothermal cutoff |
| BORE_LENGTH | boreLength | 0.5 | Tube length scale (0.5x-2.0x) |
| OUTPUT_GAIN | outputGain | 0.0 dB | Linear gain (dBtoGain) |

---

## Files Created/Modified

| File | Action | Task |
|------|--------|------|
| Source/DSP/ReedModel.h | Create | 1 |
| Source/DSP/ReedModel.cpp | Create | 1 |
| Source/DSP/BoreWaveguide.h | Create | 2 |
| Source/DSP/BoreWaveguide.cpp | Create | 2 |
| Source/DSP/BreathEnvelope.h | Create | 3 |
| Source/DSP/BreathEnvelope.cpp | Create | 3 |
| Source/ReedWindVoice.h | Modify | 4 |
| Source/ReedWindVoice.cpp | Modify | 4 |
| Source/PluginProcessor.h | Modify | 5 |
| Source/PluginProcessor.cpp | Modify | 5 |

---

## Success Criteria

- [ ] MIDI note-on produces audible reed wind tone
- [ ] Pitch accurate across MIDI range (within ±2 Hz of 12-TET)
- [ ] BREATH_PRESSURE controls dynamics (silence at 0, loud at 1)
- [ ] EMBOUCHURE affects brightness and attack
- [ ] REED_HARDNESS changes attack character and brightness
- [ ] REED_MASS near 0 behaves like static reed; high values show sluggish onset
- [ ] BORE_CHARACTER 0 = cylindrical (odd harmonics); >0 = conical character
- [ ] Note-off produces natural bore ring-down (not abrupt cutoff)
- [ ] Sustained tone is stable (no runaway oscillation or decay to silence)
- [ ] No clicks or pops during parameter changes
- [ ] Responds to CC2 (breath controller) mapped to breath pressure
- [ ] VST3 + AU build with zero errors
- [ ] No denormals (ScopedNoDenormals in renderNextBlock)

---

## Pitfalls to Watch

1. **Pop before push** — always pop both delay lines before pushing either
2. **Reed mass near zero** — switch to static reed when mu_r < 1e-4f
3. **Loop gain >= 1** — bell_gain * viscothermal_gain must stay < 1.0
4. **Filter group delay** — subtract from total delay for pitch accuracy
5. **Delay < 2 samples** — clamp minimum for Thiran stability
6. **Z_c amplitude scaling** — raw physical values (~2.3e6) may need normalization; use output gain + tanh safety
7. **Bore morphing clicks** — smooth scale factors with one-pole (~50ms)
8. **Denormals** — ScopedNoDenormals + snapToZero on filters
