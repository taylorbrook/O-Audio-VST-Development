# Stage 2: DSP - Full Stage Verification

## Verification Date

2026-04-05

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md + Phase CONTEXTs)

1. Full mass-spring-damper reed ODE with symplectic Euler integration
2. True conical waveguide sections (Strategy C, spherical wave scaling)
3. Bernoulli nonlinear flow junction with Guillemain Psi confinement
4. Breath envelope with velocity-scaled attack and chiff overshoot
5. Flow-dependent breath noise generator
6. Mouthpiece chamber (Helmholtz resonator, lumped element)
7. Tone holes + register vents (Keefe 3-port scattering)
8. Expression system (vibrato, growl, flutter tongue, subtone)
9. Legato mode with bore state preservation
10. Impossible physics (reverse bore, dual bore, feedback, infinite sustain)
11. Per-voice oversampling (2x default, 4x option)
12. TuningEngine integration (Scala/MTS-ESP/12-TET)
13. MPE completion (pressure, timbre, pitchbend with tuning-aware frequency)
14. 33 of 35 parameters active (instrumentPreset deferred to GUI morph)

### Deliverables (from SUMMARY documents)

1. ReedModel.h: mass-spring-damper ODE, symplectic Euler, Bernoulli flow, Psi confinement, flow clamp
2. BoreWaveguide.h: 5-segment conical bore, Thiran delay lines, spherical wave scaling, bell allpass, viscothermal loss, tone hole scattering, reverse bore, energy tracking
3. BreathEnvelope.h: attack/sustain/release, velocity-scaled chiff overshoot
4. BreathNoise.h: white noise -> bandpass 1700 Hz, flow-modulated amplitude
5. MouthpieceChamber.h: lumped 2nd-order ODE, compliance/inertance, bypass at volume=0
6. ReedWindVoice.cpp: full DSP chain integration, 33 cached parameters, legato with bore state preservation, per-voice oversampling (2x/4x), expression LFOs, dual bore mode, MPE expression, tuning-aware frequency
7. PluginProcessor.cpp: MPESynthesiser (16 voices), TuningEngine wiring, latency reporting

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| Reed ODE | ✅ Achieved | ReedModel.h: symplectic Euler, dp-driven force, closure clamp |
| Conical bore (Strategy C) | ✅ Achieved | BoreWaveguide.h: r_in/r_at_seg spherical scaling per segment |
| Bernoulli + Guillemain Psi | ✅ Achieved | ReedModel.h: psi_denom confinement, 0-0.8 range |
| Breath envelope | ✅ Achieved | BreathEnvelope.h: velocity-scaled attack, chiff overshoot |
| Flow-dependent noise | ✅ Achieved | BreathNoise.h: |u_reed| * Z_c amplitude scaling |
| Mouthpiece chamber | ✅ Achieved | MouthpieceChamber.h: lumped ODE, bypass when volume < 1e-8 |
| Tone holes + register | ✅ Achieved | BoreWaveguide.h: 5-segment Keefe 3-port scatter, register hole |
| Expression system | ✅ Achieved | ReedWindVoice.cpp: 3-source vibrato, growl, flutter, subtone |
| Legato mode | ✅ Achieved | ReedWindVoice.cpp: bore energy check, state preservation |
| Impossible physics | ✅ Achieved | BoreWaveguide.h: reverse bore, dual bore, feedback path, infinite sustain |
| Per-voice oversampling | ✅ Achieved | ReedWindVoice: 2x/4x Oversampling, runtime switching |
| Tuning integration | ✅ Achieved | PluginProcessor: setMasterTune/setMode, voice getBaseFrequencyFromTuning |
| MPE completion | ✅ Achieved | pressure -> breath, timbre -> embouchure, pitchbend -> frequency |
| 33/35 params active | ✅ Achieved | instrumentPreset deferred to GUI morph (by design) |

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU) | ✅ Pass | Zero errors, zero warnings, ninja no-op (already built) |
| auval (aumu/ORed/OuDv) | ✅ Pass | Full AU validation succeeded |
| pluginval L10 (VST3) | ✅ Pass | All tests including parameter fuzz |
| TODO/FIXME markers | ✅ Clean | No markers found in Source/ |
| Heap allocations in audio | ✅ Clean | No new/malloc/make_shared in ReedWindVoice.cpp |
| Real-time safety | ✅ Pass | Atomic param loads, ScopedNoDenormals, snapFiltersToZero |

## Phase-by-Phase Summary

| Phase | Scope | Params Added | Status |
|-------|-------|-------------|--------|
| 3.1 | Core engine (Reed + Bore + Breath) | 11 | ✅ Verified |
| 3.2 | Guillemain Psi + Noise + Chamber | 3 (14 total) | ✅ Verified |
| 3.3 | Tone holes + Expression + Legato | 10 (24 total) | ✅ Verified |
| 3.4 | Impossible physics + Dual bore | 6 (30 total) | ✅ Verified |
| 3.5 | Oversampling + Tuning + MPE | 3 (33 total) | ✅ Verified |

## DSP Architecture Summary

```
p_mouth (breath envelope + chiff)
  → + flow-dependent noise (BreathNoise)
  → vibrato/growl/flutter modulation
  → Reed model (Bernoulli + Guillemain Psi → u_reed)
  → Mouthpiece chamber (Helmholtz resonator → p_bore_plus)
  → Bore waveguide (5-segment conical, tone holes, register vent)
  → [Dual bore cross-coupling if active]
  → Bell radiation + tone hole radiation
  → Z_c normalize → outputGain → tanh soft clip
  → Oversample down → Stereo mix
```

## Human Verification

- [ ] Load in DAW, play MIDI — verify reed onset and sustained tone
- [ ] Sweep embouchure — hear brightness change
- [ ] Sweep boreCharacter 0→1 — hear cylindrical → conical taper
- [ ] Enable doubleReed — hear Psi confinement effect
- [ ] Enable airNoise — hear flow-dependent breath turbulence
- [ ] Test tone hole cutoff sweep — hear spectral filtering
- [ ] Enable vibrato (all 3 sources) — verify pitch/breath/throat modulation
- [ ] Test legato mode — glissando without attack restart
- [ ] Enable dualBore + dronePitch — verify parallel waveguide drone
- [ ] Enable infiniteSustain — verify infinite decay
- [ ] Switch 2x → 4x oversampling — no clicks or dropouts
- [ ] Change referencePitch — verify global tuning shift
- [ ] Test MPE controller — pressure/timbre/pitchbend respond

## Issues Found

None. All automated checks pass. No code issues detected.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes

**Summary:**
- 5 DSP phases completed (3.1 through 3.5)
- 33 of 35 APVTS parameters active (instrumentPreset deferred to GUI morph by design)
- 5 DSP header-only classes + integrated voice + processor
- Complete physical modeling reed-bore system with advanced extensions
- All validation tools pass (build, auval, pluginval L10)
- Real-time safe (no allocations, atomic params, denormal protection)
