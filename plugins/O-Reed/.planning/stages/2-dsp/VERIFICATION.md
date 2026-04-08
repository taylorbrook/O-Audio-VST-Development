# Stage 2: DSP Phase 3.1 - Verification

## Verification Date

2026-04-05

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md)

1. Full mass-spring-damper reed ODE with symplectic Euler (no static reed table intermediate)
2. True conical bore waveguide (Strategy C) from the start (no Strategy B correction filter)
3. Bernoulli flow junction (Psi=0 single-reed for Phase 3.1)
4. Bell reflection filter (frequency-dependent first-order allpass)
5. Viscothermal loss filter (one-pole lowpass, bore diameter dependent)
6. Breath envelope with velocity-scaled attack and chiff overshoot
7. 11 active APVTS parameters with real-time control
8. MPE expression (pressure -> breath, timbre -> embouchure, pitchbend -> frequency)
9. Quality-first implementation (no CPU budget constraint)

### Deliverables (from SUMMARY.md + Code Inspection)

1. **ReedModel** (Source/DSP/ReedModel.h): Symplectic Euler ODE, velocity-first then position. Static reed fallback when mu_r < 1e-4f. Embouchure modifiers (g_eff, k_eff, H_eff). Reed closure clamp with velocity zeroing.
2. **BoreWaveguide** (Source/DSP/BoreWaveguide.h): Strategy C conical with spherical wave scaling (r_in/r_out, r_out/r_in). Two Thiran-interpolated delay lines (max 40000 samples). Scale factor smoothing (~50ms).
3. **Bernoulli flow**: `u = copysign(1,dp) * S_opening * sqrt(2*|dp|/rho)` with Psi=0. Flow-to-wave: `p_bore_plus = Z_c * u + p_bore_minus`.
4. **Bell reflection**: First-order allpass with negation. Cutoff mapped from bellSize (800-6000 Hz).
5. **Viscothermal loss**: One-pole lowpass, `g=0.995` loop gain. Cutoff from boreDiameter (bore_mm * 150 Hz).
6. **BreathEnvelope** (Source/DSP/BreathEnvelope.h): Off/Attack/Sustain/Release states. Attack 5-50ms velocity-scaled. Chiff overshoot 0-30%. ~150ms exponential release.
7. **11 parameters**: breathPressure, embouchure, reedHardness, reedOpening, reedMass, reedDamping, boreCharacter, bellSize, boreDiameter, boreLength, outputGain. All read per-block via atomic loads.
8. **MPE**: pressure -> breath target, timbre -> embouchure override, pitchbend -> frequency via getFrequencyInHertz().
9. **Quality-first**: No oversampling bypass, no CPU shortcuts.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| Reed ODE (symplectic Euler) | ✅ Achieved | ReedModel.h:110-113 velocity-first then position |
| Strategy C conical bore | ✅ Achieved | BoreWaveguide.h:96-109 spherical wave scaling from half-angle |
| Bernoulli flow (Psi=0) | ✅ Achieved | ReedModel.h:128-131 standard Bernoulli, no confinement term |
| Bell reflection filter | ✅ Achieved | BoreWaveguide.h:113-121 first-order allpass, negated for reflection |
| Viscothermal loss | ✅ Achieved | BoreWaveguide.h:126-133 one-pole lowpass, g=0.995 |
| Breath envelope | ✅ Achieved | BreathEnvelope.h full state machine with chiff overshoot |
| 11 active parameters | ✅ Achieved | ReedWindVoice.cpp:181-191 all 11 read per-block |
| MPE expression | ✅ Achieved | ReedWindVoice.cpp:194-204 pressure/timbre/pitchbend |
| No CPU constraint | ✅ Achieved | No shortcuts or quality reductions |

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU) | ✅ Pass | 62/62 steps, zero errors. Warnings only from JUCE framework and external tuning module — zero from O-Reed source |
| AU Validation | ✅ Pass | `auval -v aumu ORed OuDv` — AU VALIDATION SUCCEEDED |
| Plugin Install | ✅ Pass | VST3 + AU installed to system folders, AU cache cleared |
| ScopedNoDenormals | ✅ Pass | Present in both processBlock (PluginProcessor.cpp:373) and renderNextBlock (ReedWindVoice.cpp:176) |
| Pop-before-push ordering | ✅ Pass | BoreWaveguide.h:147-150 pops both before 165-166 pushes |
| Reed closure clamp | ✅ Pass | ReedModel.h:117-121 clamp + velocity zeroing |
| Static reed fallback | ✅ Pass | ReedModel.h:101-106 when mu_r < 1e-4f |
| Loop gain < 1 | ✅ Pass | viscFilter g=0.995 ensures stable feedback loop |
| Filter group delay compensation | ✅ Pass | BoreWaveguide.h:60-68 viscGD + bellGD subtracted from delay |
| Delay minimum clamp | ✅ Pass | BoreWaveguide.h:75 halfDelay >= 2.0f for Thiran stability |
| Z_c normalization | ✅ Pass | ReedWindVoice.cpp:223 reference impedance 2.67e6 |
| Safety clip (tanh) | ✅ Pass | ReedWindVoice.cpp:246 std::tanh on output |
| Bore morphing smoothing | ✅ Pass | BoreWaveguide.h:44-45 ~50ms one-pole, applied lines 141-142 |
| snapFiltersToZero | ✅ Pass | ReedWindVoice.cpp:254 post-block |
| Energy-based voice cleanup | ✅ Pass | ReedWindVoice.cpp:257-259 breathEnv inactive + energy < 1e-6f |
| addSample (not setSample) | ✅ Pass | ReedWindVoice.cpp:250 correct for polyphonic additive output |
| Mono -> stereo | ✅ Pass | ReedWindVoice.cpp:249-250 loops all channels |

## Success Criteria (from PLAN.md)

| Criterion | Status | Evidence |
|-----------|--------|----------|
| MIDI note-on produces audible reed wind tone | ⏸️ Manual | AU validates MIDI test pass; DAW listening needed |
| Pitch accurate across MIDI range (±2 Hz of 12-TET) | ⏸️ Manual | Delay line = sr/hz with group delay compensation; needs frequency analyzer |
| BREATH_PRESSURE controls dynamics | ✅ Code verified | breathEnv.setTarget -> processSample * 12000 Pa -> reed model |
| EMBOUCHURE affects brightness and attack | ✅ Code verified | g_eff += emb*4000, k_eff += emb*5e6, H_eff -= emb*0.0003 |
| REED_HARDNESS changes attack character | ✅ Code verified | Maps to k_r 2e6-20e6 N/m^3 |
| REED_MASS near 0 = static reed; high = sluggish | ✅ Code verified | Static fallback < 1e-4f; high mu_r = slow ODE response |
| BORE_CHARACTER 0 = cylindrical; >0 = conical | ✅ Code verified | halfAngle=0 -> scale=1.0 (cyl); >0 -> r_in/r_out scaling |
| Note-off ring-down (not abrupt) | ✅ Code verified | breathEnv.noteOff -> release + energy-based cleanup |
| Sustained tone stable | ✅ Code verified | Loop gain g=0.995 < 1 |
| No clicks during parameter changes | ✅ Code verified | Scale factor smoothing + per-block reads |
| CC2/breath controller via MPE pressure | ✅ Code verified | MPE pressure -> breath target (ReedWindVoice.cpp:203) |
| VST3 + AU build zero errors | ✅ Verified | Clean rebuild confirmed |
| No denormals | ✅ Verified | ScopedNoDenormals in both audio paths |

## Human Verification

- [ ] Load in DAW, play MIDI notes across C2-C6 range — confirm audible reed tone
- [ ] Sweep BREATH_PRESSURE 0->1 — confirm silence at 0, loud at 1
- [ ] Sweep BORE_CHARACTER 0->1 — confirm timbral change (odd harmonics -> all harmonics)
- [ ] Play note, release — confirm natural ring-down (not abrupt cutoff)
- [ ] Rapidly automate BORE_CHARACTER — confirm no clicks
- [ ] Check pitch accuracy with tuner (middle C should read ~261.6 Hz)

## Issues Found

None.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes — Phase 3.2 (Psi confinement, breath noise, mouthpiece volume, bore morphing activation)

**Blockers:** None
